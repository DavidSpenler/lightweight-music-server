#ifndef UTIL
#define UTIL

#include <stdio.h>

struct string {
	char* data;
	int length;
};


char* get_genre_from_char(char);
void truncate_string(char*);
char* get_duration(FILE*, char*, int);
FILE* fix_unsynch_stream(FILE*, long);
struct string* fix_unsynchronization(char*, int);
char* get_raw_track(char*,int);

#endif
