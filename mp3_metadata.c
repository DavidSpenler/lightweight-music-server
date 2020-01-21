#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp3_metadata.h"

#include "util.h"

struct metadata* get_mp3_metadata(char* path) {
	fprintf(stderr,"getting metadata\n");
	fprintf(stderr,"The path is now: '%s'\n",path);
	FILE* f = fopen(path,"rb");
	if (f == NULL) {
		return NULL;
	}
	fprintf(stderr,"opened file\n");

	fprintf(stderr,"initing id3 version\n");
	char id3_version = get_id3_version(f);
	fprintf(stderr,"inited\n");
	fprintf(stderr,"initing metadata\n");
	struct metadata* data = init_metadata();
	fprintf(stderr,"inited\n");

	if (id3_version == 1) {
		data->title = malloc(31);
		fread(data->title,1,30,f);
		data->title[30] = '\0';
		truncate_string(data->title);	
		//get artist
		data->artist = malloc(31);
		fread(data->artist,1,30,f);
		data->artist[30] = '\0';
		truncate_string(data->artist);	
		//get album
		data->album = malloc(31);
		fread(data->album,1,30,f);
		data->album[30] = '\0';
		truncate_string(data->album);	
		//get year
		data->year = malloc(5);
		fread(data->year,1,4,f);
		data->year[4] = '\0';
		truncate_string(data->year);	
		//get tracklist
		fseek(f,28,SEEK_CUR);		
		char null_byte;
		fread(&null_byte,1,1,f);
		if (null_byte == '\0') {
			data->track = malloc(10);
			fread(data->track,1,1,f);
		} else {
			fseek(f,1,SEEK_CUR);		
		}
		//get genre
		char genre_char;
		fread(&genre_char,sizeof(char),1,f);
		data->genre = get_genre_from_char(genre_char);
	} else if (id3_version == 2) {
		struct id3v2_header* header = get_id3v2_header(f);
		struct id3v2_frame* fr;

		if (header->flags & (1<<7) && header->version_major < 4) {
			fprintf(stderr,"fixing unsynch\n");
			fseek(f,0,SEEK_END);	
			f = fix_unsynch_stream(f,ftell(f));
			fprintf(stderr,"return\n");
			fseek(f,0,SEEK_SET);
		} else if (header->flags & (1<<6)) {
			fprintf(stderr,"extended header, bail!\n");
			return NULL;
		} else if (header->flags & (1<<5)) {
			fprintf(stderr,"experimental tag, bail!\n");
			return NULL;
		} else if (header->flags & (1<<4)) {
			fprintf(stderr,"footer present, bail!\n");
			return NULL;
		}


		if (header->version_major >= 3) {
			char highest_TPE = 10;
			char highest_YEAR = 3;

			fseek(f,10,SEEK_SET);
			while (ftell(f) < (long)header->tag_size+10) {
				fprintf(stderr,"ftell says: %ld, tag size: %d\n",ftell(f),header->tag_size+10);
				fprintf(stderr,"getting frame\n");
				fr = get_id3v2_frame(f,header);
				//debug tools - remove later
				//fprintf(stderr,"%s\n",fr->id);
				//fprintf(stderr,"%d\n",fr->size);
				if (fr == NULL) {
					fprintf(stderr,"reached padding\n");
					break;
				}
				else if (strcmp(fr->id,"TIT2") == 0) {
					free(data->title);
					data->title = fr->data;
				}
				else if (strcmp(fr->id,"TALB") == 0) {
					free(data->album);
					data->album = fr->data;
					fprintf(stderr,"album title is: %s\n",fr->data);
				}
				else if (strcmp(fr->id,"TYER") == 0) {
					free(data->year);
					data->year = fr->data;
					highest_YEAR = 1;
				}
				else if (strcmp(fr->id,"TDRC") == 0 && highest_YEAR > 2) {
					free(data->year);
					data->year = fr->data;
					highest_YEAR = 2;
				}
				else if (strcmp(fr->id,"TRCK") == 0) {
					free(data->track);
					data->track = get_raw_track(fr->data,fr->size);
				}
				else if (strcmp(fr->id,"TPE1") == 0) {
					free(data->artist);
					data->artist = fr->data;
					highest_TPE = 1;
				}
				else if (strcmp(fr->id,"TPE2") == 0 && highest_TPE > 2) {
					free(data->artist);
					data->artist = fr->data;
					highest_TPE = 2;
				}
				else if (strcmp(fr->id,"TPE3") == 0 && highest_TPE > 3) {
					free(data->artist);
					data->artist = fr->data;
					highest_TPE = 3;
				}
				else if (strcmp(fr->id,"TPE4") == 0 && highest_TPE > 4) {
					free(data->artist);
					data->artist = fr->data;
					highest_TPE = 4;
				}
				else if (strcmp(fr->id,"TCON") == 0) {
					data->genre = get_genre_string(fr->data,fr->size,header);
					free(fr->data);	
				} else {
					//don't free data if it's being used
					free(fr->data);
				}
				free(fr->id);
				free(fr);
			}
		} else if (header->version_major == 2) {
			char highest_TP = 10;

			fseek(f,10,SEEK_SET);
			while (ftell(f) < (long)header->tag_size+10) {
				fr = get_id3v2_frame(f,header);
				if (fr == NULL) {
					//reached padding
					break;
				}
				else if (strcmp(fr->id,"TT2") == 0) {
					free(data->title);
					data->title = fr->data;
				}
				else if (strcmp(fr->id,"TAL") == 0) {
					free(data->album);
					data->album = fr->data;
				}
				else if (strcmp(fr->id,"TYE") == 0) {
					free(data->year);
					data->year = fr->data;
				}
				else if (strcmp(fr->id,"TRK") == 0) {
					free(data->track);
					data->track = get_raw_track(fr->data,fr->size);
				}
				else if (strcmp(fr->id,"TP1") == 0) {
					free(data->artist);
					data->artist = fr->data;
					highest_TP = 1;
				}
				else if (strcmp(fr->id,"TP2") == 0 && highest_TP > 2) {
					free(data->artist);
					data->artist = fr->data;
					highest_TP = 2;
				}
				else if (strcmp(fr->id,"TP3") == 0 && highest_TP > 3) {
					free(data->artist);
					data->artist = fr->data;
					highest_TP = 3;
				}
				else if (strcmp(fr->id,"TP4") == 0 && highest_TP > 4) {
					free(data->artist);
					data->artist = fr->data;
					highest_TP = 4;
				}
				else if (strcmp(fr->id,"TCO") == 0) {
					data->genre = get_genre_string(fr->data,fr->size,header);
					free(fr->data);	
				} else {
					//don't free data if it's being used
					free(fr->data);
				}
				free(fr->id);
				free(fr);
			}
		}
		data->duration = get_duration(f,path,header->tag_size);
		fprintf(stderr,"freeing header\n");
		free(header);
	}
	
	fprintf(stderr,"closing file\n");
	fclose(f);
	return data;
}
