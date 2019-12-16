#include <stdlib.h>
#include <stdio.h>
#include "endian.h"

uint32_t mtobi(uint32_t i) {
	if (ENDIANNESS == LITTLE_ENDIAN_) return (
		(uint32_t)(((uint8_t*)&i)[0] << 24) +
		(uint32_t)(((uint8_t*)&i)[1] << 16) +
		(uint32_t)(((uint8_t*)&i)[2] << 8) +
		(uint32_t)(((uint8_t*)&i)[3])
	);
	else return i;
}
uint32_t btomi(uint32_t i) {
	return mtobi(i);
}

uint32_t mtoli(uint32_t i) {
	if (ENDIANNESS == BIG_ENDIAN_) return (
		(uint32_t)(((uint8_t*)&i)[0] << 24) +
		(uint32_t)(((uint8_t*)&i)[1] << 16) +
		(uint32_t)(((uint8_t*)&i)[2] << 8) +
		(uint32_t)(((uint8_t*)&i)[3])
	);
	else return i;
}
uint32_t ltomi(uint32_t i) {
	return mtoli(i);
}

uint16_t mtobs(uint16_t s) {
	if (ENDIANNESS == LITTLE_ENDIAN_) return (
		(uint16_t)(((uint8_t*)&s)[0] << 8) +
		(uint16_t)(((uint8_t*)&s)[1])
	);
	else return s;
}
uint16_t btoms(uint16_t s) {
	return mtobs(s);
}

uint16_t mtols(uint16_t s) {
	if (ENDIANNESS == BIG_ENDIAN_) return (
		(uint16_t)(((uint8_t*)&s)[0] << 8) +
		(uint16_t)(((uint8_t*)&s)[1])
	);
	else return s;
}
uint16_t ltoms(uint16_t s) {
	return mtols(s);
}


uint16_t swape(uint16_t s, int n) {
	short rs = 0;
	if (ENDIANNESS == BIG_ENDIAN_) {
		for (int i=0;i<n;i++) {
			rs|=(((char*)&s)[i] << 8*i);
		}
	} else if (ENDIANNESS == LITTLE_ENDIAN_) {
		for (int i=0;i<n;i++) {
			rs|=(((char*)&s)[i] << 8*((n-1)-i));
		}
	}
	return rs;
}

void print_bits(char* v, int n) {
	for (int i=0;i<n;i++) {
		for (int j=7;j>=0;j--) {
			fprintf(stderr,"%d",(v[i] >> j) % 2);
		}
		if (i<n-1) fprintf(stderr," ");
	}
	fprintf(stderr,"\n");
}
