#include "make_db.h"

#include "id3v2.h"

#include <stdlib.h>
#include <sqlite3.h>

int create_db(char* path) {
	sqlite3* db;
	sqlite3_stmt* res;

	if (sqlite3_open(path,&db) != SQLITE_OK) {
		perror("Error connecting to db");	
		return 1;
	}

	fprintf(stderr,"db path: %s\n",path);

	int rc = sqlite3_prepare_v2(db, "CREATE TABLE tracks(title TEXT,artist TEXT,album TEXT,year INTEGER,track INTEGER,genre TEXT,duration INTEGER,path BLOB NOT NULL);", -1, &res, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr,"error code: %d\n",rc);
		perror("Error making table");	
		sqlite3_close(db);
		return 1;
	}

	if (sqlite3_step(res) != SQLITE_DONE) {
		perror("Error execing statement");
		return 1;
	}
	sqlite3_finalize(res);
	sqlite3_close(db);
	return 0;
}

int add_row(struct metadata* m, char* db_path, char* file_path) {
	int query_str_size = snprintf(NULL,0,
		"INSERT INTO tracks VALUES(\"%s\",\"%s\",\"%s\",%s,%s,\"%s\",%s,\"%s\");",
		m->title,
		m->artist,
		m->album,
		m->year,
		m->track,
		m->genre,
		m->duration,
		file_path
	)+1;
	char* query_str = malloc(query_str_size);
	sprintf(query_str,
		"INSERT INTO tracks VALUES(\"%s\",\"%s\",\"%s\",%s,%s,\"%s\",%s,\"%s\");",
		m->title,
		m->artist,
		m->album,
		m->year,
		m->track,
		m->genre,
		m->duration,
		file_path
	);

	sqlite3* db;
	sqlite3_stmt* res;

	if (sqlite3_open(db_path,&db) != SQLITE_OK) {
		perror("Error connecting to db");	
		return 1;
	}

	if (sqlite3_prepare_v2(db, query_str, -1, &res, NULL) != SQLITE_OK) {
		perror("Error making query");	
		sqlite3_close(db);
		return 1;
	}

	if (sqlite3_step(res) != SQLITE_DONE) {
		perror("Error execing statement");
		return 1;
	}

	free(query_str);
	sqlite3_finalize(res);
	sqlite3_close(db);
	return 0;
}
