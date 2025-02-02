#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define BUFSIZE 1024
char *directory = NULL; // global var. NOT A GREAT APPROACH


typedef struct {
    int client_sock;
} client_data;


void *handleClient(void *arg) {
    client_data *data = (client_data *)arg;
    int sock = data->client_sock;
    
    // Buffer to read from client - now thread-local
    char buffer[BUFSIZE];
    int read_bytes = recv(sock, buffer, BUFSIZE - 1, 0);
    
    // Check that buffer is not empty
    if (read_bytes > 0) 
    {
        buffer[read_bytes] = '\0';
        char *temp = strdup(buffer);
        char *method = NULL;
        
        // only 2 types of method for this
        if (strncmp(temp, "GET", 3) == 0)
            method = "GET";
        else
            method = "POST";

        // POST requests
        if (strcmp(method, "POST") == 0) 
        {
            char *action = buffer + 5;

            // empty route
            if (strncmp(action, "/files", 6) == 0) 
            {
                // extract filename
                char *temp = action + 7;
                char *end = strstr(temp, " ");
                int len = end - temp;
                
                // create filename
                char filename[len + 1];
                strncpy(filename, temp, len);
                filename[len] = '\0';

                // directory is used in CLI argument
                char filepath[BUFSIZE];
                snprintf(filepath, sizeof(filepath), "%s/%s", directory, filename);

                // Now parse the headers for Content-Length
                char *start = strstr(action, "Content-Length: ");
                start += strlen("Content-Length: ");
            
                // get content length
                char *temp_len = strstr(start, "\r\n");
                char len_str[BUFSIZE];
                strncpy(len_str, start, temp_len - start);
                int content_len = atoi(len_str);
        
                // Now read the body content (after "\r\n\r\n")
                char *content_start = strstr(start, "\r\n\r\n");
                content_start += 4; // skip the "\r\n\r\n"

                // Check if body length matches Content-Length
                if (content_len <= BUFSIZE) {
                    // Create file and write the content
                    FILE *file = fopen(filepath, "w");
                    if (file) {
                        fwrite(content_start, 1, content_len, file);
                        fclose(file);
                        // Send response after file creation
                        char resp[] = "HTTP/1.1 201 Created\r\n\r\n";
                        send(sock, resp, strlen(resp), 0);
                    } else {
                        // Handle file creation error
                        char resp[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                        send(sock, resp, strlen(resp), 0);
                    }
                } 
                else {
                    char resp[] = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\n\r\n";
                    send(sock, resp, strlen(resp), 0);
                }
                    
            } 
            else 
            {
                // 405 Method Not Allowed for other POST requests
                char resp[] = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
                send(sock, resp, strlen(resp), 0);
            }
        }

        // invalid method
        else if (strcmp(method, "GET") != 0) {
            char resp[] = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
            send(sock, resp, strlen(resp), 0);
        }

        // GET requests
        else
        {
            char *action = buffer + 4;

            // empty route
            if (strncmp(action, "/ ", 2) == 0) {
                char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
                send(sock, resp, strlen(resp), 0);
            }

            // echo route : /echo/{string}
            else if (strncmp(action, "/echo", 5) == 0) {
                // Extract url path
                char *url_path = buffer + 4;
                for (int i = 0; i < strlen(url_path); i++) {
                    if (url_path[i] == ' ') {
                        url_path[i] = '\0';
                        break;
                    }
                }

                // get the string after /echo
                char *str = url_path + 6;
                int len = strlen(str);
                
                // prep to send back response
                char response[BUFSIZE];
                char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
                
                snprintf(response, sizeof(response), "%s%d\r\n\r\n%s", header, len, str);
                send(sock, response, strlen(response), 0);
            }

            // user-agent route
            else if (strncmp(action, "/user-agent", 11) == 0) 
            {
                printf("here\n");
                // Extract user agent
                char *needle = "User-Agent: ";
                char *temp = strstr(buffer, needle);
                char *end = strstr(temp, "\r\n");

                if (temp && end) {
                    temp += strlen(needle);
                    int len = end - temp;
                    // Prep to send back response
                    char response[BUFSIZE];
                    char header[] =
                        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";

                    snprintf(response, sizeof(response), "%s%d\r\n\r\n%.*s", header, len,
                            len, temp);
                    printf("response: %s\n", response);

                    send(sock, response, strlen(response), 0);
                }

                // empty user-agent encountered
                else {
                    char resp[] = "HTTP/1.1 200 OK\r\nContent-Type: "
                                "text/plain\r\nContent-Length: 0\r\n\r\n";
                    send(sock, resp, strlen(resp), 0);
                }
            }


            // files route: /files/{filename}
            else if (strncmp(action, "/files", 6) == 0){
                // extract filename
                char *temp = buffer + 11;
                char *end = strstr(temp, " ");
                int len = end - temp;

                char filename[len + 1];
                strncpy(filename, temp, len);
                filename[len] = '\0';

                // absolute path of file name
                char filepath[BUFSIZE];
                snprintf(filepath, sizeof(filepath), "%s/%s", directory, filename);
                printf("filename: %s\n", filepath);

                // check if file exists
                FILE *file = fopen(filepath, "rb");
                if (file)
                {
                    // getting file size
                    fseek(file, 0, SEEK_END);
                    long size = ftell(file);

                    // getting file contents
                    fseek(file, 0, SEEK_SET); 
                    char file_content[size + 1];
                    size_t bytes_read = fread(file_content, 1, size, file);
                    file_content[bytes_read] = '\0';

                    // prep to send back response
                    char response[BUFSIZE + size];
                    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: ";
                
                    snprintf(response, sizeof(response), "%s%ld\r\n\r\n%s", header, size, file_content);
                    send(sock, response, strlen(response), 0);
                }

                else{
                    char resp[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(sock, resp, strlen(resp), 0);
                }

            }

            // 404 route
            else {
                char resp[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(sock, resp, strlen(resp), 0);
            }
        }

        free(temp);
    }
    
    close(sock);
    free(data);
    return NULL;
}

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--directory") == 0) {
			directory = argv[i + 1];
			break;
		}
	}
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4221),
        .sin_addr = { htonl(INADDR_ANY) },
    };

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Server listening for connections...\n");

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_sock = accept(server_fd, (struct sockaddr *)&client_addr, 
                               &client_addr_len);
        
        if (client_sock < 0) {
            printf("Accept failed: %s\n", strerror(errno));
            continue;
        }

        // set up thread data
        client_data *data = malloc(sizeof(client_data));
        data->client_sock = client_sock;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handleClient, data) != 0) {
            printf("Failed to create thread: %s\n", strerror(errno));
            free(data);
            close(client_sock);
            continue;
        }
        
        // // Detach thread to clean up automatically when it's done
        // pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}


