#include "../include/bencode.h"
#include "../include/util.h"

/* Here we parse strings according to the bittorrent specification.
 * Strings are length-prefixed base ten followed by a colon and the string. 
 * For example 4:spam corresponds to 'spam'. This function should return spam in this case
 */
char *parse_str(const char* bencoded_value)
{
   // verify that we are indeed parsing a string as first char we encounter should be len 
   if (is_digit(bencoded_value[0])){

        // will give all possible numeric char until ":"
        int str_len = atoi(bencoded_value);

        if (str_len == 0){
            fprintf(stderr, "Empty String detected or atoi() failed\n");
        }

        const char* colon = strchr(bencoded_value, ':');
    
        if (colon != NULL){
            // Find position of word. malloc, string copy and then return
            const char* word = colon + 1;
            char* decoded_word = malloc(str_len + 1);
            strncpy(decoded_word, word, str_len);
            decoded_word[str_len] = '\0';
            return decoded_word;
        }

        else{
            fprintf(stderr, "Invalid encoded value: \':\' not found in parse_str()\n");
            exit(1);
        }

   }

   else{
      fprintf(stderr, "parse_str() failed: Length of string not present\n");
      exit(1);
   }

  return NULL;
}


/* Here we parse integers according to the bittorrent specification
 * Integers are represented by an 'i' followed by the number in base 10 followed by an 'e'.
 * For example i3e corresponds to 3 and i-3e corresponds to -3. Integers have no size limitation.
 * i-0e is invalid. All encodings with a leading zero, such as i03e, are invalid, other than i0e,
 * which of course corresponds to 0.
 * */
long int parse_int(const char* bencoded_value){
   long int result = 0;
   
   // First, we strip i from the bencoded_value
   const char* num = bencoded_value + 1;

   // Invalid scenarios
   const char* neg = strchr(num, '-');

   if (neg != NULL && neg[1] != '\0' && neg[1] == '0'){
        fprintf(stderr, "Invalid encoded value: -0 found in parse_int()\n");
        exit(1);
   }

   else if(num[0] == '0' && num[1] != '\0' && is_digit(num[1])){
        fprintf(stderr, "Invalid encoded value: encoding with leading zero in parse_int()\n");
        exit(1);
   }

   else if (num[1] == '\0'){
       fprintf(stderr, "Invalid encoded value: delimiter 'e' not found in parse_int()\n");
       exit(1);
   }

   // valid
   result = atol(num); 

   return result;
}

/* This function parse a bencoding based on the fact that a string and an 
* integer can be in a list, at its core. This can be taught of as the base case * for lists, If we find a nested list, we employ recursion. At some point, 
* there will be a list with either a string ot int at its last level. 
* This parses the whole encoding as a list. i.e
 * [1, "2", "3", [3,4], {3,"hello"}]
 */
ListArr* parse_as_list(const char* bencoded_value){

    if (*bencoded_value != 'l') {
        fprintf(stderr, "Error: Not a list.\n");
        return NULL;  // Early return if the input is not a list
    }

    ListArr* list = NULL;
    int len = 0;

    // strip 'l'
    const char* items = bencoded_value + 1;
 
    // we will iterate through the list to see what is present
    while(*items != 'e' && *items != '\0'){
        // string: pointer arithmetioc
        if (is_digit(items[0])){
            int str_len = atoi(items);
            // account for colon, and len of string
            items += str_len + 1 + countDigits(str_len); 
        }

        else if (items[0] == 'i') { // Handling integers
            items++; //
            while (*items != 'e') 
                items++; //move to end of int
            items++; // skip 'e'
        }

        else if (*items == 'l'){
            list_iterate(&items);
        }

        else{
            dict_iterate(&items);
        }

        len++;
    }
    
    // initializing all fields
    list = calloc(1, sizeof(ListArr));
    list->size = len;
    list->items = calloc(list->size, sizeof(List*));

    if (!list->items) {
        free(list);
        fprintf(stderr, "Malloc failed in parse_as_list()\n");
        return NULL;
    }

    // initialize all prt of the rray of list
    for (int i = 0; i < list->size; i++) {
        list->items[i] = malloc(sizeof(ListArr));
        list->items[i]->data.str = NULL; // Initialize
        list->items[i]->data.num = 0;
        list->items[i]->data.list = NULL;
        list->items[i]->data.dict = NULL;
        list->items[i]->type = UNDEF;
    }

    const char *iter_ptr = bencoded_value + 1;
    int idx = 0;

    // we will parse through bencoded_value again to get each data type and a
    // add that to the union list
    while (*iter_ptr != 'e' && *iter_ptr != '\0')
    {
        // found a string:  <string len encoded in base ten ASCII>:<str data>
        if(is_digit(iter_ptr[0])){
            int skip = atoi(iter_ptr);
            skip += 1 + countDigits(skip);
            char *decoded_str = parse_str(iter_ptr); // decode str
            list->items[idx]->data.str = decoded_str; // add to the list
            
            list->items[idx]->type = STR;
            iter_ptr = iter_ptr + skip; //move to next thing to parse
            idx++;

            continue;
        }

        // found an int
        else if (iter_ptr[0] == 'i'){
            long int decoded_num = parse_int(iter_ptr);
            list->items[idx]->data.num = decoded_num;
            list->items[idx]->type = INT;
            idx++;

            iter_ptr++; //
            while (*iter_ptr != 'e') 
                iter_ptr++; //move to end of int
            iter_ptr++; // skip 'e'
        }

        // found a list: l<bencoded values>e
        else if (iter_ptr[0] == 'l'){
            ListArr* nstd_lst = parse_as_list(iter_ptr);
            list->items[idx]->data.list = (struct ListArr *)nstd_lst;
            list->items[idx]->type = LIST;
            list_iterate(&iter_ptr); // updates position of iter_ptr
            idx++;
        }

        else{
            DictArr* nstd_dict = parse_as_dict(iter_ptr);
            list->items[idx]->data.dict = (struct DictArr*)nstd_dict;
            list->items[idx]->type = DICT;
            dict_iterate(&iter_ptr); // updates position of iter_ptr
            idx++;
        }
    }

    return list;
}

/* This function parse a bencoding based on a dictionary, it takes key and 
* value pairs and binds them. This can be taught of as a Python Dictionary 

* This parses the whole encoding as a dict. i.e
* {"1":[2,3], "2":"rat", "3":79, "me":[3,4]}
*/
DictArr* parse_as_dict(const char* bencoded_value){
    
   // we check if this was indeed a dict passed in
    if (*bencoded_value != 'd') {
        fprintf(stderr, "Error: Not a list.\n");
        return NULL;  // Early return if the input is not a list
    }

    DictArr* dict = NULL;
    int len = 0;
    int tracker = 0;// we know that every other thing starting is a value
    bool flag = false;

    // strip 'd'
    const char* items = bencoded_value + 1;

    /* we will iterate through the dict to see what is present
    * we will work based of the assumption that length of keys is length of dict
    */
    while(*items != 'e' && *items != '\0'){
        // string: pointer arithmetioc
        if (is_digit(items[0])){
            int str_len = atoi(items);
            items += str_len + 1 + countDigits(str_len);

            // all keys must be strings
            if(tracker % 2 == 0)
                flag = true;
        }

        else if (items[0] == 'i') { // Handling integers
            items++; //
            while (*items != 'e') 
                items++; //move to end of int
            items++; // skip 'e'
        }

        else if (*items == 'l'){
            list_iterate(&items);
        }

        else{
            dict_iterate(&items);
        }

        if(tracker % 2 == 0){
            if (flag == true)
                len++;
            else{
                fprintf(stderr, "Error: Only strings can be keys\n");
                exit(1);
            }
        }
        
        tracker++;
        flag = false; // reset flag every iteration
    }

    // initializing all fields
    dict = calloc(1, sizeof(DictArr));
    dict->size = len;
    dict->items = calloc(dict->size, sizeof(Dict*));

    if (!dict->items) {
        free(dict);
        fprintf(stderr, "Malloc failed in parse_as_dict()\n");
        return NULL;
    }

    // initialize all prt of the dictionary
    for (int i = 0; i < dict->size; i++) {
        dict->items[i] = malloc(sizeof(DictArr));
        dict->items[i]->value.str = NULL; // Initialize
        dict->items[i]->value.num = 0;
        dict->items[i]->value.list = NULL;
        dict->items[i]->value.dict = NULL;
        dict->items[i]->val_type = UNDEF;
    }

    const char *iter_ptr = bencoded_value + 1;
    int idx = 0;
    tracker = 0;

    // we will parse through bencoded_value again to get each data type and a
    // add that to the union dict
    while (*iter_ptr != 'e' && *iter_ptr != '\0')
    {
        // found a string:  <string len encoded in base ten ASCII>:<str data>
        if(is_digit(iter_ptr[0])){
            int skip = atoi(iter_ptr);
            skip += 1 + countDigits(skip);
            char *decoded_str = parse_str(iter_ptr); // decode str

            // stores string as key or value as those are possible
            if (tracker % 2 == 0)
                dict->items[idx]->key = decoded_str; 
            else{
                dict->items[idx]->value.str = decoded_str; 
                dict->items[idx]->val_type = STR;
            }

            iter_ptr = iter_ptr + skip; //move to next thing to parse

            // update our tracker since logic of skipping strings is entirely different
            tracker++;
            if (tracker % 2 == 0)
                idx++;

            continue;
        }

        // found an int
        else if (iter_ptr[0] == 'i'){
            long int decoded_num = parse_int(iter_ptr);
            dict->items[idx]->value.num = decoded_num;
            dict->items[idx]->val_type = INT;

            iter_ptr++; //
            while (*iter_ptr != 'e') 
                iter_ptr++; //move to end of int
            iter_ptr++; // skip 'e'
        }

        // found a list: l<bencoded values>e
        else if (iter_ptr[0] == 'l'){
            ListArr* nstd_lst = parse_as_list(iter_ptr);
            dict->items[idx]->value.list = (struct ListArr *)nstd_lst;
            dict->items[idx]->val_type = LIST;
            list_iterate(&iter_ptr); // updates position of iter_ptr
          
        }

        else{
            DictArr* nstd_dict = parse_as_dict(iter_ptr);
            dict->items[idx]->value.dict = (struct DictArr*)nstd_dict;
            dict->items[idx]->val_type = DICT;
            dict_iterate(&iter_ptr); // updates position of iter_ptr
        }

        tracker++;

        if (tracker % 2 == 0)
            idx++;
    }

    return dict;
}

// iterate through every part of the buffer.
Tracker* make_tracker(const char* buffer){
    Tracker* track = malloc(sizeof(Tracker));
    track->announce = NULL;
    track->announce_list = NULL;
    track->comment = NULL;
    track->created_by = NULL;
    track->creation_date = 0;
    track->info = NULL;
    track->url_list = NULL;

    DictArr* all = parse_as_dict(buffer);

    for (size_t i = 0; i < all->size; i++)
    {
        // announce is mandatory and will always be first?
        char* key =  all->items[i]->key;

        if(strcmp(key, "announce") == 0)
            track->announce = all->items[i]->value.str;

        else if(strcmp(key, "announce-list") == 0)
            track->announce_list = (ListArr*)all->items[i]->value.list;

        else if(strcmp(key, "comment") == 0)
            track->comment = all->items[i]->value.str;

        else if(strcmp(key, "created by") == 0)
            track->created_by = all->items[i]->value.str;

        else if(strcmp(key, "creation date") == 0)
            track->creation_date = all->items[i]->value.num;
        
        else if(strcmp(key, "info") == 0)
            track->info = (DictArr*)all->items[i]->value.dict;
        
        else
            track->url_list = (ListArr*)all->items[i]->value.list;
        
    }

    // announce field and info is mandatory
    // if(track->announce == NULL || track->info == NULL){
    //     fprintf(stderr, "Error: Announce AND/OR info not found in tracker\n");
    //     exit(1);
    // }

    return track;
}

/* This is called in main and represents everything that bencode requires to work
*/
void decode_bencode(const char* bencoded_value)
{
    // getting first character to determine data type and parse based on that
    char first_char = bencoded_value[0];
    
    switch(first_char)
    {
        case 'i':{
                    long int decoded_int = parse_int(bencoded_value);
                    printf("%ld\n", decoded_int);
                    break;
                }
        
        case 'l':{
                    ListArr* lst = parse_as_list(bencoded_value);
                    printData(lst, LIST);
                    break;
                }
        
        case 'd':{
                    DictArr* dict = parse_as_dict(bencoded_value);
                    printData(dict, DICT);
                    break;
                }
        
        default:{
                    char* decoded_str = parse_str(bencoded_value);
                    printf("\"%s\"\n", decoded_str);
                    free(decoded_str);
                    break;
                }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
        const char* encoded_str = argv[2];
	    decode_bencode(encoded_str);
    }

    else if(strcmp(command, "info") == 0){
        const char* torrent_file = argv[2];
        const char *encoded_str = read_torrent_file(torrent_file);
        Tracker *track = make_tracker(encoded_str);
        printTracker(track);
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
