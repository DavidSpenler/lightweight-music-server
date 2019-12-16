#ifndef UTF8ENCODER
#define UTF8ENCODER

struct utf8_string {
	char* data;
	int length;
	int size;
};

int utf8_byte_size(int);
char* write_utf8_char(char*, int, int);

struct utf8_string* ucs2_to_utf8_wbom(char*, int, bool);
struct utf8_string* ucs2_to_utf8(char*, int);
struct utf8_string* utf16be_to_utf8(char*, int);



#endif
