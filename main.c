#include <stdio.h>

#include "read_dir.h"
#include "make_db.h"

int main(int argc, char** argv) {
	if (create_db("test.db") != 0) {
		return 1;
	}
	fprintf(stderr,"created db\n");
	if (argc < 2) {
		perror("Must supply an argument");
		return 1;
	}
	if (read_dir(argv[1]) != 0) {
		perror("Error");
		return 1;
	}
	return 0;
}

