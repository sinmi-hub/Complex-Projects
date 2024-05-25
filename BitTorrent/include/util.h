#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


bool is_digit(char c);
// char *read_torrent_file(const char *filename, unsigned char **sha1_buffer,size_t *hash_len);
char *read_torrent_file(const char *filename);

int countDigits(int number);
void list_iterate(const char **bencoded_value);
void dict_iterate(const char **bencoded_value);

// printing
void printList(ListArr *list);
void printDict(DictArr *dict);
void printTracker(Tracker *track);
void printData(void *list, Type type);
void print_sha1_hex(unsigned char *hash);

//encoding functions
char *bencode(DictArr *info, size_t* temp_len);
char *bencode_helper(char *key, char *key_bencode, DictArr *info, int i, size_t* length);
