#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "read_dir.h"

#include "make_db.h"
#include "id3v2.h"


struct metadata* get_flac_metadata(char* path) {
	return NULL;
}

int read_dir(char* base_dir) {
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
			if (read_dir(sub_dir) != 0) {
				if (slash_added) free(base_dir);
				free(sub_dir);
				perror("Error parsing dir");
				return 1;
			}
			dir = opendir(base_dir);
			seekdir(dir,loc);
		} else if (S_ISREG(ent_stat.st_mode)) {
			fprintf(stderr,"%s\n",file_name);
			for (char* c = file_name;*c;c++) {
				if (*c == '.') file_ext = c;
			}
			if (strcmp(file_ext,".mp3") == 0) {
				printf("%s\n",sub_dir);
				data = get_mp3_metadata(sub_dir); 	
				fprintf(stderr,"adding row\n");
				if (add_row(data,"test.db",sub_dir) != 0) {
					fprintf(stderr,"An error occurred\n");
					return 1;
				}
				fprintf(stderr,"added row\n");
				fprintf(stderr,"got data\n");
				printf("%s,%s,%s,%s,%s,%s,%s\n",
					data->title,
					data->artist,
					data->album,
					data->year,
					data->track,
					data->genre,
					data->duration
				);
				free_metadata(data);
			} else if (strcmp(file_ext,".flac") == 0) { 
				data = get_flac_metadata(sub_dir); 	
				free(data);
			}
			fprintf(stderr,"done\n");
		}
		ent = readdir(dir);
	}
	closedir(dir);
	if (slash_added) free(base_dir);
	free(sub_dir);
	return 0;
}
