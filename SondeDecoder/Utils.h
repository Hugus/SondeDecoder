#pragma once
#include <stdint.h>
#include <stdio.h>

int bits2bytes ( char *bitstr, uint8_t *bytes, unsigned int size ) ;

void psk_bpm ( char* frame_rawbits, char *frame_bits, unsigned int frame_size ) ;

int dpsk_bpm ( char* frame_rawbits, char *frame_bits, int len ) ;

int32_t getInt32 ( uint8_t *frame_bytes, uint32_t position ) ;
int16_t getInt16 ( uint8_t *frame_bytes, uint32_t position ) ;
uint8_t getUInt8 ( uint8_t *frame_bytes, uint32_t position ) ;
float   getFloat ( uint8_t *frame_bytes, uint32_t position ) ;

int read_wav_header(FILE     * fp,
                    double     baudRate,
                    uint64_t * nChannels,
                    uint64_t * bitsPerSample,
                    uint64_t * samplePerSec,
                    double   * samplePerBit ) ;
