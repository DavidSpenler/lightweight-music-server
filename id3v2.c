#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "id3v2.h"

#include "utf8_encoder.h"
#include "endian.h"
#include "util.h"

char* get_genre_string(char* genre_data, int size, struct id3v2_header* h) {
	char v = h->version_major;
	char* genre_string;
	char *p, *start = genre_data;

	unsigned char genre_char;
	bool genre_char_set = false;
	long buffer;

	if (v <= 3) {
		while (start < genre_data+size) {
			if (
				(*start != '(') ||  
				(start < genre_data+size-1 && *(start+1) == '(')
			) { 
				//if not a valid unsigned char number
				int genre_string_size = size-(start-genre_data);	
				genre_string = malloc(genre_string_size+1);
				strncpy(genre_string,start,genre_string_size);
				genre_string[genre_string_size] = '\0';
				return genre_string;
			} else {
				start++;
				buffer = strtol(start,&p,10);
				if (p != start && buffer < 255) {
					genre_char = (unsigned char)buffer;
					if (!genre_char_set) genre_char_set = true;
				}
			}
			//implicity assume *p points to ')'
			start = p+1;
		}
		if (genre_char_set) {
			//if that was the number genre, process it
			return get_genre_from_char(buffer);		
		}
		else { 
			//Else, just return 'None' as the genre
			char genre_none[] = "None";
			genre_string = malloc(5);
			strncpy(genre_string,genre_none,5);
			return genre_string;
		}
	} else if (v == 4) {
		//if a proper string is found, assume it is the authoritative genre
		//otherwise, assume the first numeric string is
		
		bool first_char_set = false;
		buffer = strtol(start,&p,10);

		while (start < genre_data+size) {
			buffer = strtol(start,&p,10);
			//if the first string is non-numerical, then it doesn't matter that
			//this isn't valid
			if (!first_char_set) {
				genre_char = (char)genre_char;
				first_char_set = true;
			}

			if (p == start || buffer > 255) {
				//note: if this the last string and a valid char, we must assume
				//that either the start of the next frame or the subsequent
				//padding is not any of the characters 0-9
				int genre_string_size;
				if (p == start) {
					//if string is a proper string
					char* p2;
					for (p2=p;p2<=genre_data+size && *p2 != '\0';p2++);
					genre_string_size = (p2-start)/sizeof(char);
				} else {
					//if string appears to be an int, but not a char
					genre_string_size = (p-start)/sizeof(char);
				}

				genre_string = malloc(genre_string_size+1);	
				//strncat(genre_string,start,genre_string_size);
				memcpy(genre_string,start,genre_string_size);
				genre_string[genre_string_size] = '\0';
				fprintf(stderr,"genre_string: %s\n",genre_string);
				return genre_string;
			}
			start = p+1;
		}
		return get_genre_from_char(genre_char);
	} else return NULL;
}

char get_id3_version(FILE *f) {
	char tag[4];
	tag[3] = '\0';

	if (f == NULL) fprintf(stderr,"f is null\n");
	fprintf(stderr,"seeking beginning\n");
	fprintf(stderr,"%ld\n",ftell(f));
	fseek(f,0,SEEK_SET);
	fprintf(stderr,"sought\n");
	fread(tag,1,3,f);
	fprintf(stderr,"returning\n");
	if (strcmp(tag,"ID3") == 0) return 2;

	fprintf(stderr,"seeking end\n");
	fseek(f,-128,SEEK_END);		
	fread(tag,1,3,f);
	if (strcmp(tag,"TAG") == 0) return 1;

	return -1;
}

struct id3v2_header* get_id3v2_header(FILE *f) {
	fseek(f,0,SEEK_SET);

	char id[4];
	id[3] = '\0';
	fread(&id,1,3,f);
	if (strcmp(id,"ID3") != 0) {
		return NULL;
	}

	fprintf(stderr,"about to malloc id3v2 header\n");
	struct id3v2_header* h = malloc(sizeof(struct id3v2_header));
	fprintf(stderr,"mallocced id3v2 header\n");

	int tag_size;

	fread(&(h->version_major),1,1,f);
	fread(&(h->version_minor),1,1,f);
	fread(&(h->flags),1,1,f);

	fread(&tag_size,4,1,f);
	//tag size is always synchsafe
	h->tag_size = (
		(((unsigned char*)&tag_size)[0] << 21) +
		(((unsigned char*)&tag_size)[1] << 14) +
		(((unsigned char*)&tag_size)[2] << 7) +
		((unsigned char*)&tag_size)[3]
	)-1;
	return h;
}

struct id3v2_frame* get_id3v2_frame(FILE *f, struct id3v2_header* h) {
	
	char v = h->version_major;

	struct id3v2_frame* fr = malloc(sizeof(struct id3v2_frame));
	fr->version=v;

	if (v >= 3) {
		fr->id = malloc(5);
		if (fr->id == NULL) fprintf(stderr,"fr->id is null\n");
		fprintf(stderr,"reading\n");
		fread(fr->id,1,4,f);
		fprintf(stderr,"read\n");
		fr->id[4] = '\0';
		fprintf(stderr,"set null terminator\n");
	} else if (v == 2) {
		fr->id = malloc(4);
		fread(fr->id,1,3,f);
		fr->id[3] = '\0';
	}
	fprintf(stderr,"checking if null:\n");
	//check if null
	if (fr->id[0] == '\0') {
		fprintf(stderr,"is null, freeing:\n");
		free(fr->id);
		fprintf(stderr,"freed id, freeing fr\n");
		free(fr);
		return NULL;
	}

	fprintf(stderr,"id: %s\n",fr->id);

	int frame_size = 0;
	if (v >= 3) {
		fread(&frame_size,4,1,f);
		
		//synch-safe ints only used in v4
		if (v == 4) {
			fr->size = (
				(((unsigned char*)&frame_size)[0] << 21) +
				(((unsigned char*)&frame_size)[1] << 14) +
				(((unsigned char*)&frame_size)[2] << 7) +
				((unsigned char*)&frame_size)[3]
			)-1;
		} else {
			fr->size = btomi(frame_size)-1;
		}
		fprintf(stderr,"frame size: %d\n",fr->size);
	} else if (v == 2) {
		//check to make sure this works
		fread(&frame_size,1,3,f);
		//synch-safe ints not used in v2, fix this when you aren't lazy
		fr->size = (
			(((unsigned char*)&frame_size)[0] << 16) +
			(((unsigned char*)&frame_size)[1] << 8) +
			((unsigned char*)&frame_size)[2]
		)-1;
	}
	

	if (v >= 3) {
		fread(&(fr->flags),2,1,f);
		fr->flags = btoms(fr->flags);
	} else {
		//come back to this, need to handle nonexistant flags
		fr->flags = 0;
	}
	
	fprintf(stderr,"got size and flags\n");

	//if no encoding byte
	if (fr->size < 0) {
		//pack it up
		fr->size = 0;	
		fr->data = malloc(1);
		fr->data[0] = '\0';
		return fr;
	}

	char encoding;
	fread(&encoding,1,1,f);
	fprintf(stderr,"encoding: %d\n",encoding);
	fprintf(stderr,"size: %d\n",fr->size);
	fr->data = malloc(fr->size+1);
	fprintf(stderr,"mallocced data\n");
	fread(fr->data,1,fr->size,f);
	fr->data[fr->size] = '\0';
	fprintf(stderr,"got data\n");

	//frame unsynchronization
	if (v == 4 && fr->flags & (1<<1) && h->flags & (1<<7)) {
		fprintf(stderr,"Fixing unsynchronization\n");
		struct string* fixed_data = fix_unsynchronization(fr->data,fr->size);
		fr->data = fixed_data->data;
		fr->size = fixed_data->length; 
	}

	if (encoding > 0 && encoding < 3) {
		fprintf(stderr,"non-utf8 compatible text encoding\n");
		struct utf8_string* s;
		//convert from ucs2 to utf8
		fprintf(stderr,"converting ucs2 string\n");
		if (encoding == 1) s = ucs2_to_utf8(fr->data,fr->size);
		//convert from utf16BE to utf8
		if (encoding == 2) s = utf16be_to_utf8(fr->data,fr->size);
		fprintf(stderr,"string we got was: %s\n",s->data);

		free(fr->data);
		fr->data = s->data;
		fprintf(stderr,"string we got was: %s\n",fr->data);
		//note: struct id3v2_frame size refers does not include null terminator
		fr->size = s->length;

		free(s);
		fprintf(stderr,"string we got was: %s\n",fr->data);
 
	}
	fprintf(stderr,"got header, returning\n");
		
	return fr;
}

struct metadata* init_metadata() {
	struct metadata* m = malloc(sizeof(struct metadata));
	m->title = NULL;
	m->artist = NULL;
	m->album = NULL;
	m->year = NULL;
	m->track = NULL;
	m->genre = NULL;
	return m;
}

void free_metadata(struct metadata* m) {
	free(m->album);
	free(m->title);
	free(m->artist);
	free(m->year);
	free(m->track);
	free(m->genre);
	free(m->duration);
	free(m);
}
