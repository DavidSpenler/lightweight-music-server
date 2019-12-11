#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

struct string {
	char* data;
	int size;
	char encoding;
};

struct metadata {
	struct string* title;
	struct string* album;
	struct string* artist;
	struct string* year;
	struct string* track;
	struct string* genre;
	int duration;
};

struct id3v2_header {
	char version_major;
	char version_minor;
	short flags;
	int tag_size;
};

struct id3v2_frame {
	char version;
	char flags;
	char* id;
	struct string* data;
};

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

char* get_genre_string(char c) {
	FILE* f = fopen("genres.bin","r");
	if (f == NULL) {
		perror("Could not open file");
		return NULL;
	}

	short offset, loc;
	char* string;

	fread(&offset,sizeof(short),1,f);
	offset = ntohs(offset);

	fseek(f,c*sizeof(short),SEEK_CUR);	
	fread(&loc,sizeof(short),1,f);
	loc = ntohs(loc);

	fseek(f,(long)loc+(long)offset+sizeof(short),SEEK_SET);
	fscanf(f,"%m[^\n]\n",&string);

	fclose(f);
	return string;
}

char get_id3_version(FILE *f) {
	char tag[4];
	tag[3] = '\0';

	fseek(f,0,SEEK_SET);
	fread(tag,1,3,f);
	if (strcmp(tag,"ID3") == 0) return 2;

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

	struct id3v2_header* h = malloc(sizeof(struct id3v2_header));

	char flags;
	unsigned char header_size_chunks[4];
	int tag_size;

	fread(&(h->version_major),1,1,f);
	fread(&(h->version_minor),1,1,f);
	fread(&(h->flags),1,1,f);

	fread(&tag_size,4,1,f);
	tag_size = ntohl(tag_size);
	h->tag_size = (
		((unsigned char*)&tag_size)[0] << 0 |
		((unsigned char*)&tag_size)[1] << 7 |
		((unsigned char*)&tag_size)[2] << 14 |
		((unsigned char*)&tag_size)[3] << 21
	);
	return h;
}

struct id3v2_frame* get_id3v2_frame(FILE *f, char v) {

	struct id3v2_frame* fr = malloc(sizeof(struct id3v2_frame));
	fr->version=v;

	if (v >= 3) {
		fr->id = malloc(5);
		fread(fr->id,1,4,f);
		if (fr->id[0] == '\0') {
			free(fr->id);
			free(fr);
			return NULL;
		}
		fr->id[4] = '\0';
	} else if (v == 2) {
		fr->id = malloc(4);
		fread(fr->id,1,3,f);
		if (fr->id[0] == '\0') {
			free(fr->id);
			free(fr);
			return NULL;
		}
		fr->id[3] = '\0';
	}

	fr->data = malloc(sizeof(struct string));
	int frame_size = 0;
	if (v >= 3) {
		fread(&frame_size,4,1,f);
		frame_size = ntohl(frame_size);
		//exclude encoding byte
		fr->data->size = (
			((unsigned char*)&frame_size)[0] << 0 | 
			((unsigned char*)&frame_size)[1] << 7 | 
			((unsigned char*)&frame_size)[2] << 14 | 
			((unsigned char*)&frame_size)[3] << 21 
		)-1;
	} else if (v == 2) {
		fread(&frame_size,1,3,f);
		frame_size = ntohl(frame_size);
		//exclude encoding byte
		fr->data->size = (
			((unsigned char*)&frame_size)[0] << 0 | 
			((unsigned char*)&frame_size)[1] << 7 | 
			((unsigned char*)&frame_size)[2] << 14 | 
		)-1;
	}
	if (v >= 3) {
		fread(&(fr->flags),2,1,f);
	}

	char encoding;
	fread(&encoding,1,1,f);

	fr->data->encoding = encoding;
	fr->data->data = malloc(fr->size+1);
	fread(fr->data->data,1,fr->size,f);
	fr->data->data[fr->size] = '\0';
		
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
	free(m);
}

struct metadata* get_mp3_metadata(char* path) {
	FILE* f = fopen(path,"rb");
	if (f == NULL) {
		return NULL;
	}

	struct metadata* data = init_metadata();
	char id3_version = get_id3_version(f);

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
		data->genre = get_genre_string(genre_char);
	} else if (id3_version == 2) {
		struct id3v2_header* header = get_id3v2_header(f);
		struct id3v2_frame* fr;

		if (header->version_major >= 3) {
			char highest_TPE = 10;
			char highest_YEAR = 3;

			fseek(f,10,SEEK_SET);
			while (ftell(f) < (long)header->tag_size+10) {
				fr = get_id3v2_frame(f,header->version_major);
				printf("%s\n",fr->id);
				printf("%d\n",fr->size);
				if (fr == NULL) {
					//reached padding
					break;
				}
				else if (strcmp(fr->id,"TIT2") == 0) {
					data->title = realloc(data->title,fr->size+1);
					strcpy(data->title,fr->data);
				}
				else if (strcmp(fr->id,"TALB") == 0) {
					data->album = realloc(data->album,fr->size+1);
					strcpy(data->album,fr->data);
				}
				else if (strcmp(fr->id,"TYER") == 0) {
					data->year = realloc(data->year,fr->size+1);
					strcpy(data->year,fr->data);
					highest_YEAR = 1;
				}
				else if (strcmp(fr->id,"TDRC") == 0 && highest_YEAR > 2) {
					data->year = realloc(data->year,fr->size+1);
					strcpy(data->year,fr->data);
					highest_YEAR = 2;
				}
				else if (strcmp(fr->id,"TRCK") == 0) {
					data->track = realloc(data->track,fr->size+1);
					strcpy(data->track,fr->data);
				}
				else if (strcmp(fr->id,"TPE1") == 0) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TPE = 1;
				}
				else if (strcmp(fr->id,"TPE2") == 0 && highest_TPE > 2) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TPE = 2;
				}
				else if (strcmp(fr->id,"TPE3") == 0 && highest_TPE > 3) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TPE = 3;
				}
				else if (strcmp(fr->id,"TPE4") == 0 && highest_TPE > 4) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TPE = 4;
				}
				else if (strcmp(fr->id,"TCON") == 0) {
					//numberic genre code
					if (fr->data->data[0] == '(') {
						fr->data[fr->data->size-1]='\0';
						data->genre = get_genre_string((char)atoi(fr->data->data+1));
					//string genre
					} else {
						data->genre = realloc(data->genre,fr->size+1);
						strcpy(data->genre,fr->data->data);
					}
				}
				//printf("s\n",fr->id);
				free(fr->id);
				free(fr->->data->data);
				free(fr->->data);
				free(fr);
			}
		} else if (header->version_major == 2) {
			char highest_TP = 10;

			fseek(f,10,SEEK_SET);
			while (ftell(f) < (long)header->tag_size+10) {
				fr = get_id3v2_frame(f,header->version_major);
				if (fr == NULL) {
					//reached padding
					break;
				}
				else if (strcmp(fr->id,"TT2") == 0) {
					data->title = realloc(data->title,fr->size+1);
					strcpy(data->title,fr->data);
				}
				else if (strcmp(fr->id,"TAL") == 0) {
					data->album = realloc(data->album,fr->size+1);
					strcpy(data->album,fr->data);
				}
				else if (strcmp(fr->id,"TYE") == 0) {
					data->year = realloc(data->year,fr->size+1);
					strcpy(data->year,fr->data);
				}
				else if (strcmp(fr->id,"TRK") == 0) {
					data->track = realloc(data->track,fr->size+1);
					strcpy(data->track,fr->data);
				}
				else if (strcmp(fr->id,"TP1") == 0) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TP = 1;
				}
				else if (strcmp(fr->id,"TP2") == 0 && highest_TP > 2) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TP = 2;
				}
				else if (strcmp(fr->id,"TP3") == 0 && highest_TP > 3) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TP = 3;
				}
				else if (strcmp(fr->id,"TP4") == 0 && highest_TP > 4) {
					data->artist = realloc(data->artist,fr->size+1);
					strcpy(data->artist,fr->data);
					highest_TP = 4;
				}
				else if (strcmp(fr->id,"TCO") == 0) {
					//numeric genre code
					if (fr->data->data[0] == '(') {
						fr->data->data[fr->data->size-1]='\0';
						data->genre = get_genre_string((char)atoi(fr->data->data+1));
					//string genre
					} else {
						data->genre = realloc(data->genre,fr->size+1);
						strcpy(data->genre,fr->data->data);
					}
				}
				free(fr->id);
				free(fr->data);
				free(fr);
			}
		}
		free(header);
	
	fclose(f);
	return data;
}

struct metadata* get_flac_metadata(char* path) {
	return NULL;
}

int parse_dir(char* base_dir) {
	int base_dir_len = strlen(base_dir);
	bool slash_added = false;

	if (base_dir[base_dir_len-1] != '/') {
		slash_added = true;
		char* new_base_dir = malloc(base_dir_len+2);
		strcpy(new_base_dir,base_dir);
		strcat(new_base_dir,"/");
		base_dir = new_base_dir;
		base_dir_len++;
	}

	DIR *dir = opendir(base_dir);
	if (dir == NULL) {
		perror("Could not open");
		return 1;
	}

	struct stat ent_stat;
	long int loc;
	int file_name_len;
	char *file_ext;
	char *file_name;
	char *sub_dir = malloc(base_dir_len+1);
	struct metadata *data;
	strcpy(sub_dir,base_dir);

	struct dirent *ent = readdir(dir);
	while (ent != NULL) {
		if (
			strcmp(ent->d_name,".") == 0 || 
			strcmp(ent->d_name,"..") == 0 ||
			ent->d_name[0] == '.'
		) {
			ent = readdir(dir);
			continue;
		}

		file_name = ent->d_name;
		file_name_len = strlen(file_name);
		sub_dir = realloc(sub_dir,strlen(base_dir)+file_name_len+1);
		sub_dir[base_dir_len]='\0';
		strcat(sub_dir,file_name);

		if (stat(sub_dir,&ent_stat) != 0) {
			closedir(dir);
			if (slash_added) free(base_dir);
			free(sub_dir);
			perror("Error getting stat");
			return 1;
		}
		
		if (S_ISDIR(ent_stat.st_mode)) {
			loc = telldir(dir);
			closedir(dir);
			if (parse_dir(sub_dir) != 0) {
				if (slash_added) free(base_dir);
				free(sub_dir);
				perror("Error parsing dir");
				return 1;
			}
			dir = opendir(base_dir);
			seekdir(dir,loc);
		} else if (S_ISREG(ent_stat.st_mode)) {
			printf("%s\n",file_name);
			for (char* c = file_name;*c;c++) {
				if (*c == '.') file_ext = c;
			}
			if (strcmp(file_ext,".mp3") == 0) {
				data = get_mp3_metadata(sub_dir); 	
				printf("%s %s %s %s %s %s\n",
					data->title,
					data->artist,
					data->album,
					data->year,
					data->track,
					data->genre);
				free_metadata(data);
			} else if (strcmp(file_ext,".flac") == 0) { 
				data = get_flac_metadata(sub_dir); 	
				free(data);
			}
			printf("done\n");
		}
		ent = readdir(dir);
	}
	closedir(dir);
	if (slash_added) free(base_dir);
	free(sub_dir);
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		perror("Must supply an argument");
		return 1;
	}
	if (parse_dir(argv[1]) != 0) {
		perror("An error ocurred");
		return 1;
	}
	return 0;
}

