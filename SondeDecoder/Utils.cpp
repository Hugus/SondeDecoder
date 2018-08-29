#include "Utils.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>

#define BITS 8

// Convert bits (encoded as ascii string) to bytes
int bits2bytes ( char *bitstr, uint8_t *bytes, unsigned int size ) {
    int i, bit, d, byteval;
    unsigned int bitpos, bytepos;

    bitpos = 0;
    bytepos = 0;

    // For each input 'bit' of bitstr
    while ( bytepos < size ) {
        byteval = 0;
        d = 1;
        for ( i = 0; i < BITS; i++ ) {
            // Take one 'bit' (in ascii)
            //bit=*(bitstr+bitpos+i);            /* little endian */
            bit = *( bitstr + bitpos + 7 - i );  /* big endian */
            // If bit is one, add 2^i to byte value, else add 0
            if ( bit == '1' )                     byteval += d;
            else /*if ((bit == '0') || (bit == 'x'))*/  byteval += 0;
            d <<= 1;
        }
        bitpos += BITS;
        bytes[bytepos++] = byteval & 0xFF;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
// PSK  (or biphase-M (or differential Manchester?))
// after Synchronisation: 00,11->0 ; 01,10->1 (phase change)
void psk_bpm ( char* frame_rawbits, char *frame_bits, unsigned int frame_size ) {
    unsigned int i;
    char bit;
    //int err = 0;

    for ( i = 0; i < frame_size; i++ ) {

        //if (i > 0 && (frame_rawbits[2*i] == frame_rawbits[2*i-1])) err = 1;

        if ( frame_rawbits[2 * i] == frame_rawbits[2 * i + 1] ) bit = '0';
        else                                            bit = '1';

        //if (err) frame_bits[i] = 'x'; else
        frame_bits[i] = bit;
        //err = 0;

    }
}

// Bit conversion (manchester coding):
// Take two bits, apply the following function
// 10 -> 1
// 01 -> 0
// Then, if bit(i) == bit(i-1) -> 1
// Else  0
// When i = 0, bit(i-1) = frame_rawbits[0] xor 1
int dpsk_bpm ( char* frame_rawbits, char *frame_bits, int len ) {
    int i;
    char bit;
    char bit0;
    //int err = 0;

    bit0 = ( frame_rawbits[0] & 1 ) ^ 1;

    for ( i = 0; i < len / 2; i++ ) {
        if ( ( frame_rawbits[2 * i] & 1 ) == 1 &&
            ( frame_rawbits[2 * i + 1] & 1 ) == 0 ) bit = 1;
        else if ( ( frame_rawbits[2 * i] & 1 ) == 0 &&
            ( frame_rawbits[2 * i + 1] & 1 ) == 1 ) bit = 0;
        else {
            bit = 2;
            frame_bits[i] = 'x';
            bit0 = bit & 1;
            continue;
            //err = 1;
        }

        if ( bit0 == bit ) frame_bits[i] = '1';
        else             frame_bits[i] = '0';
        // frame_bits[i] = 0x31 ^ (bit0 ^ bit);
        bit0 = bit;
    }

    return bit0;
}

int32_t getInt32 ( uint8_t *frame_bytes, uint32_t position )
{

    uint8_t outputArray[4];
    for ( unsigned int i = 0; i < 4; i++ ) {
        outputArray[i] = frame_bytes[position + i];;
    }

    int32_t result = 0 ;
    for ( unsigned int i = 0; i < 4; i++ ) {
        result |= outputArray[i] << ( 8 * ( 3 - i ) );
    }

    return result ;
}

int16_t getInt16 ( uint8_t *frame_bytes, uint32_t position )
{

    uint8_t outputArray[2];
    for ( unsigned int i = 0; i < 2; i++ ) {
        outputArray[i] = frame_bytes[position + i];;
    }

    int16_t result = 0 ;
    for ( unsigned int i = 0; i < 2; i++ ) {
        result |= outputArray[i] << ( 8 * ( 1 - i ) );
    }

    return result ;
}

uint8_t getUInt8 ( uint8_t *frame_bytes, uint32_t position )
{
    return frame_bytes[position] ;
}

float getFloat ( uint8_t *frame_bytes, uint32_t position )
{

    uint8_t outputArray[4];
    for ( unsigned int i = 0; i < 4; i++ ) {
        outputArray[i] = frame_bytes[position + i];;
    }

    int32_t result = 0 ;
    for ( unsigned int i = 0; i < 4; i++ ) {
        result |= outputArray[i] << ( 8 * (3 - i ) );
    }

    return reinterpret_cast<float&>( result ) ;
}

int findstr(const char *buf, const char *str, int pos) {
    int i;
    for (i = 0; i < 4; i++) {
        if (buf[(pos+i)%4] != str[i]) break;
    }
    return i;
}

int read_wav_header(FILE     * fp,
                    double     baudRate,
                    uint64_t * nChannels,
                    uint64_t * bitsPerSample,
                    uint64_t * samplePerSec,
                    double   * samplePerBit ) {
    char txt[4+1] = "\0\0\0\0";
    unsigned char dat[4];
    int byte, p=0;

    if (fread(txt, 1, 4, fp) < 4) return -1;
    if (strncmp(txt, "RIFF", 4)) return -1;
    if (fread(txt, 1, 4, fp) < 4) return -1;
    // pos_WAVE = 8L
    if (fread(txt, 1, 4, fp) < 4) return -1;
    if (strncmp(txt, "WAVE", 4)) return -1;
    // pos_fmt = 12L
    for ( ; ; ) {
        if ( (byte=fgetc(fp)) == EOF ) return -1;
        txt[p % 4] = byte;
        p++; if (p==4) p=0;
        if (findstr(txt, "fmt ", p) == 4) break;
    }
    if (fread(dat, 1, 4, fp) < 4) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;

    if (fread(dat, 1, 2, fp) < 2) return -1;
    *nChannels = dat[0] + (dat[1] << 8);

    if (fread(dat, 1, 4, fp) < 4) return -1;
    
    int sampleRate = 0 ;
    memcpy(&sampleRate, dat, 4); 
    *samplePerSec = sampleRate ;

    if (fread(dat, 1, 4, fp) < 4) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;
    //byte = dat[0] + (dat[1] << 8);

    if (fread(dat, 1, 2, fp) < 2) return -1;
    *bitsPerSample = dat[0] + (dat[1] << 8);

    // pos_dat = 36L + info
    for ( ; ; ) {
        if ( (byte=fgetc(fp)) == EOF ) return -1;
        txt[p % 4] = byte;
        p++; if (p==4) p=0;
        if (findstr(txt, "data", p) == 4) break;
    }
    if (fread(dat, 1, 4, fp) < 4) return -1;


    std::cout << "sample_rate: " <<  *samplePerSec << std::endl ;
    std::cout << "bits       : " <<  *bitsPerSample << std::endl ;
    std::cout << "channels   : " <<  *nChannels << std::endl ;

    if ((*bitsPerSample != 8) && (*bitsPerSample != 16)) return -1;

    *samplePerBit = *samplePerSec/baudRate;

    std::cout << "samples/bit: " << *samplePerBit << std::endl ;

    return 0;
}
