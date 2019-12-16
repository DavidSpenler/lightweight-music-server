#ifndef ENDIAN_FUNCS
#define ENDIAN_FUNCS

#include <stdint.h>

static uint32_t secret = 1;
enum endian { BIG_ENDIAN_, LITTLE_ENDIAN_ };
#define ENDIANNESS *(char*)&secret

uint32_t mtobi(uint32_t i);
uint32_t btomi(uint32_t i);
uint32_t mtoli(uint32_t i);
uint32_t ltomi(uint32_t i);
uint16_t mtobs(uint16_t s);
uint16_t btoms(uint16_t s);
uint16_t mtols(uint16_t s);
uint16_t ltoms(uint16_t s);

void print_bits(char* v, int n);

#endif

