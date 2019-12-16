#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "endian.h"
#include "utf8_encoder.h"

char* write_utf8_char(char* dest, int num_bytes, int codepoint) {
	if (num_bytes == 1) {
		*dest = (char)codepoint;
	} else {
		int k = 7;
		int offset = 0;
		int num_bits = (num_bytes-2)*5 + 11;
	
		dest[offset] = 0;

		//write prefix
		for (int j=0;j<=num_bytes;j++) {
			if (k < 0) {
				offset++;
				dest[offset] = 0;
				k = 7;
			}
			//do one pass at the end to write 0 - the end of the prefix
			if (j != num_bytes) dest[offset] |= (1 << k);
			k--;
		}

		for (int j=num_bits-1;j>=0;j--) {
			if (k < 0) {
				offset++;
				//set prefix of 10xxxxxx
				dest[offset] = 128;
				k = 5;
			}
			dest[offset] |= ((codepoint >> j)%2) << k;
			k--;
		}
	}
	return dest;
}


int utf8_byte_size(int codepoint) {
	char codepoint_size = sizeof(int)*8;
	for (int j=sizeof(int)*8-1;j>=0;j--) {
		if ((codepoint >> j)%2 == 0) codepoint_size--;
		else break;
	}
	int num_bytes = codepoint_size <= 7 ? 1 : ceil((codepoint_size-11)/5.0)+2;
	return num_bytes;
}


struct utf8_string* ucs2_to_utf8_wbom(char* ucs2, int size, bool endian) {
	//dont allow odd sized char arrays
	if (size%2 == 1) return NULL;

	struct utf8_string* string = malloc(sizeof(struct utf8_string));
	//char* utf8_string = NULL;
	//int utf8_string_size = 0;
	int new_string_size;
	//int utf8_string_length = 0;
	string->data = NULL;
	string->size = 0;
	string->length = 0;

	unsigned short ucs2_char;
	for (int i=0;i<size/2;i++) {
		ucs2_char = ((unsigned short*)ucs2)[i];

		if (endian == LITTLE_ENDIAN_) ucs2_char = ltoms(ucs2_char);
		else if (endian == BIG_ENDIAN_)  ucs2_char = btoms(ucs2_char);

		int num_bytes = utf8_byte_size((int)ucs2_char);

		new_string_size = string->length+num_bytes+1;
		if (new_string_size > string->size) {
			if (string->size*2 >= new_string_size) {
				string->size*=2;
			} else {
				string->size = new_string_size;
			}
			string->data = realloc(string->data,string->size);
		}
		write_utf8_char(string->data+string->length,num_bytes,(int)ucs2_char);
		string->length+=num_bytes;
	}
	string->data[string->length] = '\0';
	fprintf(stderr,"final utf8 string: %s\n",string->data);
	return string;
}


struct utf8_string* ucs2_to_utf8(char* ucs2, int size) {
	//must have at least 2 bytes
	if (size < 2) return NULL;

	bool endian;
	//check BOM
	if ((unsigned char)ucs2[0] == 0xff && (unsigned char)ucs2[1] == 0xfe) {
		endian = LITTLE_ENDIAN_;
	}
	else if ((unsigned char)ucs2[0] == 0xfe && (unsigned char)ucs2[1] == 0xff) {
		endian = BIG_ENDIAN_;
	}
	else return NULL;

	//increment pointer to start after BOM, adjust size and specify endianness
	struct utf8_string* s = ucs2_to_utf8_wbom(ucs2+2,size-2,endian);
	fprintf(stderr,"final utf8 string: %s\n",s->data);
	return ucs2_to_utf8_wbom(ucs2+2,size-2,endian);
}


struct utf8_string* utf16be_to_utf8(char* utf16be, int size) {
	//dont allow odd sizes
	if (size%2 == 1) return NULL;

	struct utf8_string* string = malloc(sizeof(struct utf8_string));
	string->data = NULL;
	string->size = 0;
	string->length = 0;
	//char* utf8_string = NULL;
	//int utf8_string_size = 0;
	int new_string_size;
	//int utf8_string_length = 0;

	unsigned short utf16_char;
	unsigned int int_utf16_char;

	int num_bytes;
	//read two bytes at a time
	for (int i=0;i<size/2;i++) {
		utf16_char = btoms(((short*)utf16be)[i]);
		if (utf16_char >= 0xd800 && utf16_char <= 0xdfff) {
			//...except if it's a surrogate pair	
			int_utf16_char = (utf16_char-0xd800) << 10;
			//get next of pair
			i++;
			utf16_char = btoms(((short*)utf16be)[i]);
			int_utf16_char |= (int)(utf16_char-0xdc00);
			int_utf16_char += 0x10000;
		} else {
			//shorts outside of this range are backwards compatible with ucs2
			int_utf16_char = (int)utf16_char;
		}
		num_bytes = utf8_byte_size(int_utf16_char);

		new_string_size = string->length+num_bytes+1;
		if (new_string_size > string->size) {
			if (string->size*2 >= new_string_size) {
				string->size*=2;
			} else {
				string->size = new_string_size;
			}
			string->data = realloc(string->data,string->size);
		}
		write_utf8_char(string->data+string->length,num_bytes,int_utf16_char);
		string->length+=num_bytes;
	}
	string->data[string->length] = '\0';
	return string;
}
