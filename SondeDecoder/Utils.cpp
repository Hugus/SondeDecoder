#include "stdafx.h"
#include "Utils.h"

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
    int i;
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
        result |= outputArray[i] << ( 8 * ( 3 - i ) );
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