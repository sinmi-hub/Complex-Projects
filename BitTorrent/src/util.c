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
    long file_size = ftell(file);
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
