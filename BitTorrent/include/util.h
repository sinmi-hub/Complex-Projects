#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


bool is_digit(char c);
char *read_torrent_file(const char *filename);
int countDigits(int number);
void list_iterate(const char **bencoded_value);
void dict_iterate(const char **bencoded_value);

// printing
void printList(ListArr *list);
void printDict(DictArr *dict);
void printTracker(Tracker *track);
void printData(void *list, Type type);