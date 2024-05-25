#include "bencode.h"
#include "util.h"

bool is_digit(char c){
    return c >= '0' && c <= '9';
}

char* read_torrent_file(const char* filename) {
    FILE *file = fopen(filename, "rb"); // Open the file in binary mode
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET); // Seek back to the start of the file

    // Allocate memory for the entire content
    char *content = malloc(file_size + 1); // +1 for the null-terminator
    if (content == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read the file into the allocated memory
    size_t read_size = fread(content, 1, file_size, file);
    if (read_size != file_size) {
        fprintf(stderr, "Error reading file\n");
        free(content);
        fclose(file);
        return NULL;
    }

    // Null-terminate the string
    content[file_size] = '\0';

    // Close the file
    fclose(file);
    
    return content;
}

// counts number of digits in an integer
int countDigits(int number){
    int count = 0;
    if (number == 0) {
        return 1;  
    }
    if (number < 0) {
        number = -number;  // negative numbers
    }
    while (number != 0) {
        number /= 10;  
        count++;       
    }
    return count;
}

// used to simply iterate over a list
void list_iterate(const char** bencoded_value){
    const char* items = *bencoded_value + 1;  // initial 'l'

    while(*items != 'e' && *items != '\0'){
        if (is_digit(items[0])){
            int str_len = atoi(items);
            while (is_digit(*items)) items++;  // skip the length digits
            items++;  // skip ':'
            items += str_len;  
        } else if (items[0] == 'i') {
            items++;  // skip 'i'
            while (*items != 'e') items++;  
            items++;  // skip 'e'
        } else if (*items == 'l'){
            list_iterate(&items);  // recursive call 
        } else {
            dict_iterate(&items);
        }
    }

    if (*items == 'e') 
        items++;  // move past the list-ending 'e'

    *bencoded_value = items;  // modify pointer passed in
}

void dict_iterate(const char** bencoded_value){
    const char* items = *bencoded_value + 1;  // initial 'd' stripped

    while(*items != 'e' && *items != '\0'){
        if (is_digit(items[0])){
            int str_len = atoi(items);
            while (is_digit(*items)) 
                items++;  // skip the length digits

            items++;  // skip ':'
            items += str_len;  
        } else if (items[0] == 'i') {
            items++;  // skip 'i'
            while (*items != 'e') items++;  
            items++;  // skip 'e'
        } else if (*items == 'l'){
            list_iterate(&items);  
        } else {
            dict_iterate(&items);
        }
    }

    if (*items == 'e') 
        items++;  //move past the dict-ending 'e'

    *bencoded_value = items;  
}

// prints a list out
void printList(ListArr* list) {
    if (!list) return;  // Check for NULL list
    printf("[");

    for (size_t i = 0; i < list->size; i++){
        switch (list->items[i]->type) {
            case STR:
                printf("\"%s\"", list->items[i]->data.str);
                break;
            case INT:
                printf("%ld", list->items[i]->data.num);
                break;
            case LIST:
                printList((ListArr* )list->items[i]->data.list);
                break;
            case DICT:
                printDict((DictArr *)list->items[i]->data.dict);
                break;
            default:
                fprintf(stderr, "Error detected: Undefined case reached\n");
                break; // UNDEF enum
        }

        if (i < list->size - 1)
            printf(",");
    }
    printf("]");
}

// printing a dictionary
void printDict(DictArr* dict){
    if (!dict) return;  // Check for NULL dict
    printf("{");

    for (size_t i = 0; i < dict->size; i++){
        printf("\"%s\":", dict->items[i]->key); // print key

        switch (dict->items[i]->val_type)
        {
            case STR:
                printf("\"%s\"", dict->items[i]->value.str);
                break;
            case INT:
                printf("%ld", dict->items[i]->value.num);
                break;
            case LIST:
                printList((ListArr* )dict->items[i]->value.list);  
                break;
            case DICT:
                printDict((DictArr* )dict->items[i]->value.dict);
                break;
            default:
                fprintf(stderr, "Error detected: Undefined case reached\n");
                break; // UNDEF enum
        }

        if (i < dict->size - 1)
            printf(",");
    }
    printf("}");
}

void printTracker(Tracker* track){
    // printf("------Printing Tracker------\n");
    printf("Tracker URL: %s\n", track->announce);
    // printf("announce list: \n");
    // printData(track->announce_list, LIST);
    // printf("comment: %s\n", track->comment);
    // printf("created by: %s\n", track->created_by);
    // printf("creation date: %ld\n", track->creation_date);
    // printf("info dict: \n");
    // printData(track->info, DICT);
    printf("Length: %ld\n", track->info->items[0]->value.num);
    // printf("url-list: \n");
    // printData(track->url_list, LIST);
}

// creating a generic function to be able to print, whether it is a list or a dictionary
void printData(void* val, Type type) {
    switch (type) 
     {
        case LIST:
            printList((ListArr*)val);
            break;
        case DICT:
            printDict((DictArr*)val);
            break;
        default:
            fprintf(stderr, "Error in printData(): unknown data type\n");
    }
    printf("\n");  
}

// prints out hash in hex
void print_sha1_hex(unsigned char* hash) {
    for (int i = 0; i < 20; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}


/* This function iterates through info dictionary to find the 
* length of sha1 hash value in the info dictionary.
* This is the hash value of pieces. The formula is : 
* (length / pieces length) * 20
*/
int64_t _hashlen(DictArr* info){
    double len = -1;

    for (size_t i = 0; i < info->size; i++){
        if(strcmp(info->items[i]->key, "length") == 0)
            len = info->items[i]->value.num;
        else if (strcmp(info->items[i]->key, "piece length") == 0 && len > 0){
            len = ceil(len / info->items[i]->value.num);
        }
    }

    return len * 20;
}


/* Simply creates a bencoding of a dictionary passed in. That being said, this * function only consider cases where integers and strings are present. 
* Bencoding logic for nested dictionary or list has not been done yet 
* TODO:
*/
char* bencode(DictArr* info, size_t* temp_len) {
    char *result = NULL;
    int total_length = 3; // space for 'd' (will add 'e' '\0' later)

    result = malloc(3);
    if (!result) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // encoding of dictionary starts with d and ends with e
    result[0] = 'd'; 
    result[1] = 'e';
    result[2] = '\0';

    for (size_t i = 0; i < info->size; i++) {
        char* key = info->items[i]->key;
        long int key_len = strlen(key);

        // bencode the key
        int buf_size = snprintf(NULL, 0, "%ld:%s", key_len, key) + 1; 
        char *key_bencode = malloc(buf_size);
        if (!key_bencode) {
            fprintf(stderr, "Error: Malloc failed in bencoding\n");
            exit(1);
        }
        snprintf(key_bencode, buf_size, "%ld:%s", key_len, key);
        char *temp = bencode_helper(key, key_bencode, info, i, temp_len);

        total_length += (*temp_len - 1); // update total length
        result = realloc(result, total_length);
        if (!result) {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(1);
        }

        // combine strings. Add new encoded value to previous ones
        memcpy(result + (total_length - (*temp_len) - 1), temp, *temp_len - 1);
        result[total_length - 2] = 'e';
        result[total_length - 1] = '\0';
        free(temp);
    }
    *temp_len = total_length; // modify length to return
    return result;
}


/* This function is meant to serve as an helper for bencode(). Things to note
* - The corresponding value pair from bencode() is encoded here
* - When value is "string type", a combination of snprintf and memcpy is used
* - This is because for some key pairs like "pieces", whose values are raw binary stream of hashes, it can be quite cubersome using built in string functions
* - The function takes the encoded key from bencode() and then concatenates it
with the encoded value to produce a result.
- result is then returned.
*/
char* bencode_helper(char*key, char*key_bencode, DictArr* info, int i, size_t*len)
{
    char *result = NULL;

    // Keys that have integers as "values"
    if(strcmp(key, "length") == 0 || strcmp(key, "piece length") == 0)
    {
        long int value = info->items[i]->value.num;
        
        // bencode value
        int val_size = snprintf(NULL, 0, "i%lde", value) + 1;
        char *value_bencode = malloc(val_size);
        if (!value_bencode) {
            fprintf(stderr, "Error: Malloc failed in bencoding\n");
            free(key_bencode);
            exit(1);
        }

        // successfully write int beconding
        snprintf(value_bencode, val_size, "i%lde", value);

        int total_size = strlen(key_bencode) + val_size;
        // writing the concatenation to result
        result = malloc(total_size);
        if(!result){
            fprintf(stderr, "Error: Malloc failed in bencoding\n");
            free(key_bencode);
            free(value_bencode);
            exit(1);
        }

        // write to result
        snprintf(result, total_size, "%s%s", key_bencode, value_bencode);

        free(key_bencode);
        free(value_bencode);

        *len = total_size; // modify the length variable passed in
    }

    // Keys that have strings as "values"
    else{
        char* val = info->items[i]->value.str;
        long int val_len = 0;
        int val_size = 0;
        char *value_bencode = NULL;
        int total_size = 0;

        // dealing with sha1 hashes where we have to be more careful
        if (strcmp(key, "pieces") == 0){
            val_len = _hashlen(info);
            val_size = snprintf(NULL, 0, "%ld:", val_len);
            val_size += val_len + 1;
        }        

        else{
            val_len = strlen(val);
            val_size = snprintf(NULL, 0, "%ld:%s", val_len, val) + 1;
        }

        // malloc space for entire bencoding value
        value_bencode = malloc(val_size);
        if (!value_bencode) {
            fprintf(stderr, "Error: Malloc failed in bencoding\n");
            free(key_bencode);
            exit(1);
        }
        
        // successfully write beconding for strings
        snprintf(value_bencode, val_size - val_len, "%ld:", val_len);
        memcpy(value_bencode + (val_size - val_len - 1), val, val_len);
        total_size = strlen(key_bencode) + val_size;

        // writing the concatenation to result
        result = malloc(total_size);
        if(!result){
            fprintf(stderr, "Error: Malloc failed in bencoding\n");
            free(key_bencode);
            free(value_bencode);
            exit(1);
        }
    
        // writing concatenation to result
        memcpy(result, key_bencode, strlen(key_bencode));
        memcpy(result + strlen(key_bencode), value_bencode, val_size);

        free(key_bencode);
        free(value_bencode);

        *len = total_size;// modify len variable
    }

    return result;
}
