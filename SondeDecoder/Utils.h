#pragma once
#include <stdint.h>

int bits2bytes ( char *bitstr, uint8_t *bytes, unsigned int size ) ;

void psk_bpm ( char* frame_rawbits, char *frame_bits, unsigned int frame_size ) ;

int dpsk_bpm ( char* frame_rawbits, char *frame_bits, int len ) ;

int32_t getInt32 ( uint8_t *frame_bytes, uint32_t position ) ;