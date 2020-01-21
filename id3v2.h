#ifndef ID3V2
#define ID3V2

#include <stdio.h>

struct id3v2_header {
	char version_major;
	char version_minor;
	char flags;
	int tag_size;
};

struct id3v2_frame {
	char version;
	char* id;
	short flags;
	int size;
	char* data;
};

struct metadata {
	char* title;
	char* album;
	char* artist;
	char* year;
	char* track;
	char* genre;
	char* duration;
};


struct metadata* init_metadata();
void free_metadata(struct metadata*);

char* get_genre_string(char*, int, struct id3v2_header*);
char get_id3_version(FILE*);
struct id3v2_header* get_id3v2_header(FILE*);
struct id3v2_frame* get_id3v2_frame(FILE*, struct id3v2_header*);

#endif
