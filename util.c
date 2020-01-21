#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "util.h"

char* get_genre_from_char(char c) {
	FILE* f = fopen("genres.bin","r");
	if (f == NULL) {
		perror("Could not open file");
		return NULL;
	}

	short offset, loc;
	char* string;

	fread(&offset,sizeof(short),1,f);
	//do these without inet later
	offset = ntohs(offset);

	fseek(f,c*sizeof(short),SEEK_CUR);	
	fread(&loc,sizeof(short),1,f);
	loc = ntohs(loc);

	fseek(f,(long)loc+(long)offset+sizeof(short),SEEK_SET);
	fscanf(f,"%m[^\n]\n",&string);

	fclose(f);
	return string;
}

char* get_raw_track(char* old_track,int old_len) {
	char* start = old_track;
	char* error = old_track;
	char* end = old_track+old_len;

	strtol(start,&error,10);
	while (error == start && start != end) {
		start++;
		strtol(start,&error,10);
	}

	int len = (error-start)/sizeof(char);

	char* track = malloc(len+1);
	memcpy(track,start,len);
	track[len] = '\0';
	free(old_track);
	return track;
	
}

void truncate_string(char* s) {
	char *null_char = NULL;
	for (char *c = s;*c;c++) {
		if (*c == ' ' && null_char == NULL) {
			null_char = c;
		} else if (*c != ' ') {
			null_char = NULL;
		}
	}
	if (null_char) {
		*null_char=  '\0';
	}
	return;
}

char* get_duration(FILE* f, char* path, int tag_size) {
	long current_pos = ftell(f);
	//god help us if we have a file greater than 2.4GB
	fseek(f,0,SEEK_END);
	long file_size = ftell(f)-tag_size;
	//do this yourself later!
	int bitrate = 0;


	int command_len = snprintf(NULL,0,
		"/usr/bin/file \"%s\" | sed 's/^.* \\([0-9]\\+\\) kbps.*$/\\1/'",
		path
	)+1;
	//must account for null terminator
	fprintf(stderr,"size of command: %d\n",command_len);
	char* command = malloc(command_len);
	sprintf(command,
		"/usr/bin/file \"%s\" | sed 's/^.* \\([0-9]\\+\\) kbps.*$/\\1/'",
		path
	);
	fprintf(stderr,"actual size of command: %d\n",strlen(command));

	fprintf(stderr,"%s\n",command);

	FILE* bitrate_f = popen(command,"r");
	fscanf(bitrate_f,"%d",&bitrate);
	fclose(bitrate_f);
	free(command);
	fprintf(stderr,"bitrate: %d\n",bitrate);
	int duration = (int)file_size/((long)bitrate*1000/8);

	//must account for null terminator
	int str_duration_size = snprintf(NULL,0,"%d",duration)+1;
	char* str_duration = malloc(str_duration_size);
	sprintf(str_duration,"%d",duration);

	//restore file pointer cursor
	fseek(f,current_pos,SEEK_SET);
	return str_duration;
}

FILE* fix_unsynch_stream(FILE* f, long size) {
	fprintf(stderr,"%ld\n",size);
	fseek(f,0,SEEK_SET);
	char buffer[1024];
	char* new_f_stream = malloc(size);
	unsigned char last_char = 1;

	int buf_size = fread(buffer,1,1024,f);

	long j = 0;
	while (buf_size == 1024) {
		for (int i=0;i<buf_size;i++) {
			if (!(buffer[i] == 0 && last_char == 255)) {
				new_f_stream[j]=buffer[i];
				j++;
			}
			last_char = buffer[i];
		}
		buf_size = fread(buffer,1,1024,f);
	}
	fclose(f);
	FILE* new_f = fmemopen(new_f_stream,size,"rb");
	//free(new_f_stream);
	return new_f;
}

struct string* fix_unsynchronization(char* data, int size) {
	char* fixed_data = malloc(size);
	unsigned char last_char = 0;
	int j = 0;
	for (int i=0;i<size;i++) {
		if (data[i] != 0 || last_char != 255) {
			fixed_data[j] = data[i];
			j++;
		} else last_char = 1;
		last_char = data[i];
	}
	free(data);
	struct string* fixed_string = malloc(sizeof(struct string));
	fixed_string->data = fixed_data;
	fixed_string->length = j;
	return fixed_string;
}

