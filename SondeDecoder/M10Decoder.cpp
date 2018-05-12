#include "stdafx.h"
#include "M10Decoder.h"
#include "GPS.h"
#include "Utils.h"
#include "ShibauraSensor.h"
#include <Mmreg.h>
#include <iostream>

/*
alternative demodulation: M10 problematic
bits_per_sample small, sync is falling apart
exact baud rate is crucial
9600 baud -> 9616 baud?
*/

M10Decoder::M10Decoder ()
    : m_bitsPerSample( 0 )
    , m_samplePerSec( 0 )
    , m_nChannels( 0 )
    , m_samplePerBit( 0 )
    // m_configuration.b: exact baud rate important!
    // in principle identifiable in sync-preamble
    , m_baudRate( 9616 )
    , m_sampleCount( 0 )
    , m_bitSeparator( 0 )
    , m_par( 1 )
    , m_parAlt( 1 )
    , m_bitStart( false )
    , m_sCount( 0 )
    , m_headerBufferPos( -1 )
    , m_auxlen( 0 )
    , m_isHeaderFound( false )
    , m_sampleType( ST_INVALID )
{
    m_configuration.verbose = 1 ;
    m_configuration.ptu = 1 ;

    std::cout << "SondeDecoder v1.0." << std::endl ;
}


M10Decoder::~M10Decoder ()
{
}

HRESULT M10Decoder::SetFormat ( WAVEFORMATEX * pwfx )
{
    m_bitsPerSample = pwfx->wBitsPerSample ;
    m_samplePerSec = pwfx->nSamplesPerSec ;
    m_nChannels = pwfx->nChannels ;
    m_samplePerBit = m_samplePerSec / m_baudRate;

    std::cout << "Bits per sample : " << m_bitsPerSample << std::endl ;
    std::cout << "Samples per second : " << m_samplePerSec << std::endl ;
    std::cout << "Samples per bit : " << m_samplePerBit << std::endl ;
    std::cout << "Number of channels : " << m_nChannels << std::endl ;

    if ( pwfx->wFormatTag == WAVE_FORMAT_PCM )
    {
        m_sampleType = ST_INT ;
        std::cout << "Sample type : INT" << std::endl ;
    }
    else if ( pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT )
    {
        m_sampleType = ST_FLOAT ;
        std::cout << "Sample type : FLOAT" << std::endl ;
    }
    else if ( pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
    {
        WAVEFORMATEXTENSIBLE * extension = reinterpret_cast<WAVEFORMATEXTENSIBLE *>( pwfx ) ;
        if ( extension->SubFormat == KSDATAFORMAT_SUBTYPE_PCM )
        {
            m_sampleType = ST_INT ;
            std::cout << "Sample type : subformat INT" << std::endl ;
        }
        else if ( extension->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT )
        {
            m_sampleType = ST_FLOAT ;
            std::cout << "Sample type : subformat FLOAT" << std::endl ;
        }
        else
        {
            throw "Nap" ;
        }
    }
    else
    {
        throw "Nip" ;
    }
    return NOERROR;
}


/*
Header = Sync-Header + Sonde-Header:
1100110011001100 1010011001001100  1101010011010011 0100110101010101 0011010011001100
uudduudduudduudd ududduuddudduudd  uudududduududduu dudduudududududu dduududduudduudd (oder:)
dduudduudduudduu duduudduuduudduu  ddududuudduduudd uduuddududududud uudduduudduudduu (komplement)
0 0 0 0 0 0 0 0  1 1 - - - 0 0 0   0 1 1 0 0 1 0 0  1 0 0 1 1 1 1 1  0 0 1 0 0 0 0 0
*/
/*
110101001101001101001101010101010011010011001100
101010011010011010011010101010100110100110011001011001100101011001100110011001100110011001100101100110011001100110011001100101010101010101010101010101101010100110011010011001011001011010101010010101100110100110010101100110011001011001100110100101011010101001100101010101101010011001010101100110010110100110010101100101100101100110101010011001100110010110011001100110011001100110100101011010101001101010100101100101100110011001100110011001100110011001100110011001011001101010011001100110011010101001100101100110100110011001010101101001100110010110011010100110100110011010100110011001010101101001100110010101100110011010101001100110101001100110011001100101100110011001100110011001100110011001100110011001100110011001100110011001100110011001100110011010011010100110100110011001100110011001100110011001100101101001011010100101101001101001100110100110100110010110010110101010010110010110011001011001010101010101010110011001100110011010011010100110011001011001100110011001100110011001100110011001100110011001100110101001011010011010010110010110010101010110010110011001101001101010101010011010100110011010011010011010100110101001100110011010100110010110011001010110101001101001100110100101011001100110011010011001100110011001100110010101011001100110011001100110100110101001100110011001100110011001100110011001100110011001100110011001100110011001100110011001100110011010101001101001011001100101010101100110101001011001100110011001100110101001010110011001100110010110011001010110011001100110011001100110011001011001100101011010100101100110010110101001101001011001101001011001101010011010010110101001010101100110011001101010100110011001100000
*/

/* -------------------------------------------------------------------------- */

#define stdFLEN        0x64  // pos[0]=0x64
#define pos_GPSTOW     0x0A  // 4 byte
#define pos_GPSlat     0x0E  // 4 byte
#define pos_GPSlon     0x12  // 4 byte
#define pos_GPSalt     0x16  // 4 byte
#define pos_GPSweek    0x20  // 2 byte
//Velocity East-North-Up (ENU)
#define pos_GPSvE      0x04  // 2 byte
#define pos_GPSvN      0x06  // 2 byte
#define pos_GPSvU      0x08  // 2 byte
#define pos_SN         0x5D  // 2+3 byte
#define pos_Check     (stdFLEN-1)  // 2 byte

HRESULT M10Decoder::CopyData ( BYTE * pData, UINT32 numFramesAvailable, BOOL * bDone )
{
    // Increment audio buffer counter so as to bufferize 0.9s
    if ( ( m_audioBuffer.currentPosition * 8.0 / m_bitsPerSample / m_samplePerSec / m_nChannels ) > 0.9 )
    {
        m_audioBuffer.currentPosition = 0 ;
        // Handle buffer

        float * pFloats = reinterpret_cast<float *>( m_audioBuffer.pData ) ;

        // Now handle buffer
        demodulateBuffer () ;
        
        // Reset audio buffer
        m_audioBuffer.currentPosition = 0 ;
        m_audioBuffer.size = 0 ;
    }
    else
    {
        // Buffer current buffer
        // Realloc data buffer
        uint32_t sizeIncrement = numFramesAvailable * m_bitsPerSample / 8 * m_nChannels ;
        m_audioBuffer.size += sizeIncrement ;
        m_audioBuffer.pData = (uint8_t*) realloc ( m_audioBuffer.pData, m_audioBuffer.size ) ;
        // Copy audio data
        memcpy ( m_audioBuffer.pData + m_audioBuffer.currentPosition, pData, sizeIncrement ) ;
        // Update current position
        m_audioBuffer.currentPosition += sizeIncrement ;
    }

    return NOERROR;
}

void M10Decoder::demodulateBuffer ()
{
    // Position in frame_rawbits buffer
    int pos = FRAMESTART ;
    // Current bit
    int bit ;
    // Number of bits read
    int len ;
    int bit0 ;
    while ( !readBitsFsk ( &bit, &len ) ) {

        if ( len == 0 ) { // reset_frame();
            if ( pos > ( pos_GPSweek + 2 ) * 2 * BITS ) {
                for ( unsigned int i = pos; i < RAWBITFRAME_LEN + RAWBITAUX_LEN; i++ ) frame_rawbits[i] = 0x30 + 0;
                print_frame ( pos );//byte_count
                m_isHeaderFound = false;
                pos = FRAMESTART;
            }
            continue;
        }

        for ( unsigned int i = 0; i < len; i++ ) {

            incrementBufferIndex ();
            header_buffer[m_headerBufferPos] = 0x30 + bit;  // Ascii

            if ( !m_isHeaderFound ) {
                m_isHeaderFound = IsThisAHeader ();
            }
            else {
                frame_rawbits[pos] = 0x30 + bit;  // Ascii
                pos++;
                // If a full frame has been read
                if ( pos == RAWBITFRAME_LEN + RAWBITAUX_LEN ) {
                    frame_rawbits[pos] = '\0';
                    print_frame ( pos );//FRAME_LEN
                    m_isHeaderFound = false;
                    pos = FRAMESTART;
                }
            }

        }
    }
}


/* big endian forest
*
* gcc -o m10x m10x.c -lm
* M10 w/ trimble GPS
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef CYGWIN
#include <fcntl.h>  // cygwin: _setmode()
#include <io.h>
#endif

typedef unsigned char  uint8_t;
typedef unsigned short ui16_t;
typedef unsigned int   ui32_t;

#define EOF_INT  0x1000000

int M10Decoder::readSignedSample
(
) 
{
    int byte, i, s = 0;       // EOF -> 0x1000000

    for ( i = 0; i < m_nChannels; i++ ) {
        // i = 0: left, mono
        byte = m_audioBuffer.pData[m_audioBuffer.currentPosition] ;

        if ( m_bitsPerSample == 8 ) {
            if ( m_audioBuffer.currentPosition == m_audioBuffer.size )
            {
                return EOF_INT;
            }
            s = getUInt8 ( m_audioBuffer.pData, m_audioBuffer.currentPosition ) - 128 ; // 8bit: 00..FF, centerpoint 0x80=128
            m_audioBuffer.currentPosition += 1 ;
        }
        else if ( m_bitsPerSample == 16 ) {
            if ( m_audioBuffer.currentPosition +2 > m_audioBuffer.size )
            {
                return EOF_INT;
            }
            s = getInt16 ( m_audioBuffer.pData, m_audioBuffer.currentPosition ) ;// -32768 ; // minus 2^15
            m_audioBuffer.currentPosition += 2 ;
        } 
        else if ( m_bitsPerSample == 32 ) 
        {
            if ( m_audioBuffer.currentPosition + 4 > m_audioBuffer.size )
            {
                return EOF_INT;
            }
            if ( m_sampleType == ST_FLOAT )
            {
                float f = *reinterpret_cast<float *>( m_audioBuffer.pData + m_audioBuffer.currentPosition ) ;
                s = f * 1000;
                m_audioBuffer.currentPosition += 4 ;
            }
            else
            {
                s = getInt32 ( m_audioBuffer.pData, m_audioBuffer.currentPosition ) ;// -pow ( 2, 22 ) ;// minus 2^22
                m_audioBuffer.currentPosition += 4 ;
            }
        }
        else
        {
            throw "Nope" ;
        }
    }
    //std::cout << s << std::endl;
    return s;
}

int M10Decoder::readBitsFsk ( int *bit, int *len ) {
    // TODO why static int sample ?
    int sample = 0;
    int n, y0;
    float l, x1;
    static float x0;

    n = 0;
    do {
        y0 = sample;
        sample = readSignedSample ();
        if ( sample == EOF_INT ) return EOF;

        m_parAlt = m_par;
        m_par = ( sample >= 0 ) ? 1 : -1;    // 8bit: 0..127,128..255 (-128..-1,0..127)
        n++;
    } while ( m_par*m_parAlt > 0 );

    if ( !m_configuration.res ) l = (float)n / m_samplePerBit;
    else {                                       // more accurate bit length measurement
        x1 = sample / (float)( sample - y0 );    // helps with low sample rate
        l = ( n + x0 - x1 ) / m_samplePerBit;    // usually more frames (not always)
        x0 = x1;
    }

    *len = (int)( l + 0.5 );

    if ( !m_configuration.inv ) *bit = ( 1 + m_parAlt ) / 2;  // above 1, below -1
    else             *bit = ( 1 - m_parAlt ) / 2;  // sdr#<rev1381?, inverse: bottom 1, top -1
                                                  // *bit = (1+inv*m_parAlt)/2; // except inv=0

                                                  /* Y-offset ? */
    return 0;
}


int M10Decoder::readRawbit ( int *bit ) {
    int sample;
    int sum;

    sum = 0;

    if ( m_bitStart ) {
        m_sCount = 0;         // actually m_sCount = 1
        m_bitSeparator = 0;   //   or m_bitSeparator = -1
        m_bitStart = false;
    }
    m_bitSeparator += m_samplePerBit;

    do {
        sample = readSignedSample ();
        if ( sample == EOF_INT ) return EOF;
        sum += sample;
        m_sCount++;
    } while ( m_sCount < m_bitSeparator );  // n < m_samplePerBit

    if ( sum >= 0 ) *bit = 1;
    else            *bit = 0;

    if ( m_configuration.inv ) *bit ^= 1;

    return 0;
}

int M10Decoder::readRawbit2 ( int *bit ) {
    int sample;
    int sum;

    sum = 0;

    if ( m_bitStart ) {
        m_sCount = 0;    // eigentlich m_sCount = 1
        m_bitSeparator = 0; //   oder m_bitSeparator = -1
        m_bitStart = false;
    }

    m_bitSeparator += m_samplePerBit;
    do {
        sample = readSignedSample ();
        if ( sample == EOF_INT ) return EOF;
        sum += sample;
        m_sCount++;
    } while ( m_sCount < m_bitSeparator );  // n < m_samplePerBit

    m_bitSeparator += m_samplePerBit;
    do {
        sample = readSignedSample ();
        if ( sample == EOF_INT ) return EOF;
        sum -= sample;
        m_sCount++;
    } while ( m_sCount < m_bitSeparator );  // n < m_samplePerBit

    if ( sum >= 0 ) *bit = 1;
    else            *bit = 0;

    if ( m_configuration.inv ) *bit ^= 1;

    return 0;
}

/* -------------------------------------------------------------------------- */

/*
Header = Sync-Header + Sonde-Header:
1100110011001100 1010011001001100  1101010011010011 0100110101010101 0011010011001100
uudduudduudduudd ududduuddudduudd  uudududduududduu dudduudududududu dduududduudduudd (oder:)
dduudduudduudduu duduudduuduudduu  ddududuudduduudd uduuddududududud uudduduudduudduu (komplement)
0 0 0 0 0 0 0 0  1 1 - - - 0 0 0   0 1 1 0 0 1 0 0  1 0 0 1 1 1 1 1  0 0 1 0 0 0 0 0
*/


// Sync-Header (raw)               // Sonde-Header (bits)
//char head[] = "11001100110011001010011001001100"; //"011001001001111100100000"; // M10: 64 9F 20 , M2K2: 64 8F 20
//"011101101001111100100000"; // M10: 76 9F 20 , aux-data?
//"011001000100100100001001"; // M10-dop: 64 49 09
const char header[] = "10011001100110010100110010011001";

// int m_auxlen = 0; // 0 .. 0x76-0x64


void M10Decoder::incrementBufferIndex () {
    m_headerBufferPos = ( m_headerBufferPos + 1 ) % HEADLEN;
}

char cb_inv ( char c ) {
    if ( c == '0' ) return '1';
    if ( c == '1' ) return '0';
    return c;
}

// Danger in Manchester coding: inverse header is easily misrecognized
// because manchester1 and manchester2 are only shifted by 1 bit
int M10Decoder::IsThisAHeader () {
    int i, j;

    i = 0;
    j = m_headerBufferPos;
    while ( i < HEADLEN ) {
        if ( j < 0 ) j = HEADLEN - 1;
        if ( header_buffer[j] != header[HEADOFS + HEADLEN - 1 - i] ) break;
        j--;
        i++;
    }
    if ( i == HEADLEN ) return 1;

    i = 0;
    j = m_headerBufferPos;
    while ( i < HEADLEN ) {
        if ( j < 0 ) j = HEADLEN - 1;
        if ( header_buffer[j] != cb_inv ( header[HEADOFS + HEADLEN - 1 - i] ) ) break;
        j--;
        i++;
    }
    if ( i == HEADLEN ) return -1;

    return 0;
}



#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define XTERM_COLOR_BROWN   "\x1b[38;5;94m"  // 38;5;{0..255}m

#define col_GPSweek    "\x1b[38;5;20m"  // 2 byte
#define col_GPSTOW     "\x1b[38;5;27m"  // 4 byte
#define col_GPSdate    "\x1b[38;5;94m" //111
#define col_GPSlat     "\x1b[38;5;34m"  // 4 byte
#define col_GPSlon     "\x1b[38;5;70m"  // 4 byte
#define col_GPSalt     "\x1b[38;5;82m"  // 4 byte
#define col_GPSvel     "\x1b[38;5;36m"  // 6 byte
#define col_SN         "\x1b[38;5;58m"  // 3 byte
#define col_Check      "\x1b[38;5;11m"  // 2 byte
#define col_TXT        "\x1b[38;5;244m"
#define col_FRTXT      "\x1b[38;5;244m"
#define col_CSok       "\x1b[38;5;2m"
#define col_CSno       "\x1b[38;5;1m"



/* -------------------------------------------------------------------------- */
/*
g : F^n -> F^16      // checksum, linear
g(m||b) = f(g(m),b)

// update checksum
f : F^16 x F^8 -> F^16 linear

010100001000000101000000
001010000100000010100000
000101000010000001010000
000010100001000000101000
000001010000100000010100
100000100000010000001010
000000011010100000000100
100000000101010000000010
000000001000000000000000
000000000100000000000000
000000000010000000000000
000000000001000000000000
000000000000100000000000
000000000000010000000000
000000000000001000000000
000000000000000100000000
*/

int M10Decoder::update_checkM10 ( int c, uint8_t b ) {
    int c0, c1, t, t6, t7, s;

    c1 = c & 0xFF;

    // B
    b = ( b >> 1 ) | ( ( b & 1 ) << 7 );
    b ^= ( b >> 2 ) & 0xFF;

    // A1
    t6 = ( c & 1 ) ^ ( ( c >> 2 ) & 1 ) ^ ( ( c >> 4 ) & 1 );
    t7 = ( ( c >> 1 ) & 1 ) ^ ( ( c >> 3 ) & 1 ) ^ ( ( c >> 5 ) & 1 );
    t = ( c & 0x3F ) | ( t6 << 6 ) | ( t7 << 7 );

    // A2
    s = ( c >> 7 ) & 0xFF;
    s ^= ( s >> 2 ) & 0xFF;


    c0 = b ^ t ^ s;

    return ( ( c1 << 8 ) | c0 ) & 0xFFFF;
}

int M10Decoder::checkM10 ( uint8_t *msg, int len ) {
    int i, cs;

    cs = 0;
    for ( i = 0; i < len; i++ ) {
        cs = update_checkM10 ( cs, msg[i] );
    }

    return cs & 0xFFFF;
}


/* -------------------------------------------------------------------------- */

/*
frame[0x32]: adr_1074h
frame[0x33]: adr_1075h
frame[0x34]: adr_1076h

frame[0x35..0x37]: TBCCR1 ; relHumCap-freq

frame[0x38]: adr_1078h
frame[0x39]: adr_1079h
frame[0x3A]: adr_1077h
frame[0x3B]: adr_100Ch
frame[0x3C..3D]: 0


frame[0x3E]: scale_index ; scale/range-index
frame[0x3F..40] = ADC12_A7 | 0xA000, V_R+=AVcc ; Thermistor

frame[0x41]: adr_1000h[scale_index*4]
frame[0x42]: adr_1000h[scale_index*4+1]
frame[0x43]: adr_1000h[scale_index*4+2]
frame[0x44]: adr_1000h[scale_index*4+3]

frame[0x45..46]: ADC12_A5/4, V_R+=2.5V
frame[0x47]: ADC12_A2/16 , V_R+=2.5V
frame[0x48..49]: ADC12_iT, V_R+=1.5V (int.Temp.diode)
frame[0x4C..4D]: ADC12_A6, V_R+=2.5V
frame[0x4E..4F]: ADC12_A3, V_R+=AVcc
frame[0x50..54]: 0;
frame[0x55..56]: ADC12_A1, V_R+=AVcc
frame[0x57..58]: ADC12_A0, V_R+=AVcc
frame[0x59..5A]: ADC12_A4, V_R+=AVcc  // ntc2: R(25C)=2.2k, Rs=22.1e3 (relHumCap-Temp)

frame[0x5B]:
frame[0x5C]: adr_108Eh


frame[0x5D]: adr_1082h (SN)
frame[0x5E]: adr_1083h (SN)
frame[0x5F]: adr_1084h (SN)
frame[0x60]: adr_1080h (SN)
frame[0x61]: adr_1081h (SN)
*/

/* -------------------------------------------------------------------------- */
char weekday[7][3] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };

int M10Decoder::print_pos ( int csOK ) {
    int err;

    err = 0;
    GPS::get_GPSweek ( frame_bytes, pos_GPSweek, m_date);
    GPS::get_GPStime ( frame_bytes, pos_GPSTOW, m_date );
    GPS::get_GPSlat ( frame_bytes, pos_GPSlat, m_date );
    GPS::get_GPSlon ( frame_bytes, pos_GPSlon, m_date );
    GPS::get_GPSalt ( frame_bytes, pos_GPSalt, m_date );



    GPS::Gps2Date ( m_date.week, m_date.gpssec, &m_date.jahr, &m_date.monat, &m_date.tag );

    if ( m_configuration.color ) {
        /*
        fprintf ( stdout, col_TXT );
        fprintf ( stdout, " (W "col_GPSweek"%d"col_TXT") ", m_date.week );
        fprintf ( stdout, col_GPSTOW"%s"col_TXT" ", weekday[m_date.wday] );
        fprintf ( stdout, col_GPSdate"%04d-%02d-%02d"col_TXT" ("col_GPSTOW"%02d:%02d:%02d"col_TXT") ",
            m_date.jahr, m_date.monat, m_date.tag, m_date.std, m_date.min, m_date.sek );
        fprintf ( stdout, " lat: "col_GPSlat"%.6f"col_TXT" ", m_date.lat );
        fprintf ( stdout, " lon: "col_GPSlon"%.6f"col_TXT" ", m_date.lon );
        fprintf ( stdout, " alt: "col_GPSalt"%.2f"col_TXT" ", m_date.alt );
        if ( m_configuration.verbose ) {
            err |= GPS::get_GPSvel ();
            if ( !err ) {
                //if (m_configuration.verbose == 2) fprintf(stdout, "  "col_GPSvel"(%.1f , %.1f : %.1f)"col_TXT" ", m_date.vx, m_date.vy, m_date.vD2);
                fprintf ( stdout, "  vH: "col_GPSvel"%.1f"col_TXT"  D: "col_GPSvel"%.1f"col_TXT"°  vV: "col_GPSvel"%.1f"col_TXT" ", m_date.vH, m_date.vD, m_date.vV );
            }
            if ( m_configuration.verbose >= 2 ) {
                GPS::get_SN ();
                fprintf ( stdout, "  SN: "col_SN"%s"col_TXT, m_date.SN );
            }
            if ( m_configuration.verbose >= 2 ) {
                fprintf ( stdout, "  # " );
                if ( csOK ) fprintf ( stdout, " "col_CSok"[OK]"col_TXT );
                else      fprintf ( stdout, " "col_CSno"[NO]"col_TXT );
            }
        }
        if ( m_configuration.ptu ) {
            float t = ShibauraSensor::get_Temp ( csOK, frame_bytes, m_configuration.verbose );
            if ( t > -270.0 ) fprintf ( stdout, "  T=%.1fC ", t );
            if ( m_configuration.verbose >= 3 ) {
                float t2 = ShibauraSensor::get_Tntc2 ( csOK, frame_bytes );
                float fq555 = ShibauraSensor::get_TLC555freq ( frame_bytes );
                if ( t2 > -270.0 ) fprintf ( stdout, " (T2:%.1fC) (%.3fkHz) ", t2, fq555 / 1e3 );
            }
        }
        fprintf ( stdout, ANSI_COLOR_RESET"" );
        */
    }
    else 
    {
        fprintf ( stdout, " (W %d) ", m_date.week );
        if ( ( m_date.wday >= 0 ) && ( m_date.wday < 7 ) )
        {
            fprintf ( stdout, "%s ", weekday[m_date.wday] );
        }
        else
        {
            fprintf ( stdout, "invalid " );
        }
        fprintf ( stdout, "%04d-%02d-%02d (%02d:%02d:%02d) ",
            m_date.jahr, m_date.monat, m_date.tag, m_date.std, m_date.min, m_date.sek );
        fprintf ( stdout, " lat: %.6f ", m_date.lat );
        fprintf ( stdout, " lon: %.6f ", m_date.lon );
        fprintf ( stdout, " alt: %.2f ", m_date.alt );
        if ( m_configuration.verbose ) {
            err |= GPS::get_GPSvel ( frame_bytes, 
                pos_GPSvE,
                pos_GPSvN,
                pos_GPSvU,
                m_date );
            if ( !err ) {
                //if (m_configuration.verbose == 2) fprintf(stdout, "  (%.1f , %.1f : %.1f°) ", m_date.vx, m_date.vy, m_date.vD2);
                fprintf ( stdout, "  vH: %.1f  D: %.1f  vV: %.1f ", m_date.vH, m_date.vD, m_date.vV );
            }
            if ( m_configuration.verbose >= 2 ) {
                GPS::get_SN ( frame_bytes, pos_GPSalt, m_date );
                fprintf ( stdout, "  SN: %s", m_date.SN );
            }
            if ( m_configuration.verbose >= 2 ) {
                fprintf ( stdout, "  # " );
                if ( csOK ) fprintf ( stdout, " [OK]" ); else fprintf ( stdout, " [NO]" );
            }
        }
        if ( m_configuration.ptu ) {
            float t = ShibauraSensor::get_Temp ( csOK, frame_bytes, m_configuration.verbose );
            if ( t > -270.0 ) fprintf ( stdout, "  T=%.1fC ", t );
            if ( m_configuration.verbose >= 3 ) {
                float t2 = ShibauraSensor::get_Tntc2 ( csOK, frame_bytes );
                float fq555 = ShibauraSensor::get_TLC555freq ( frame_bytes  );
                if ( t2 > -270.0 ) fprintf ( stdout, " (T2:%.1fC) (%.3fkHz) ", t2, fq555 / 1e3 );
            }
        }
    }
    fprintf ( stdout, "\n" );

    

    return err;
}

void M10Decoder::print_frame ( int pos ) {
    int i;
    uint8_t byte;
    int cs1, cs2;
    int flen = stdFLEN; // stdFLEN=0x64, auxFLEN=0x76

    // Decode Manchester coding
    if ( m_configuration.b < 2 ) {
        dpsk_bpm ( frame_rawbits, frame_bits, RAWBITFRAME_LEN + RAWBITAUX_LEN );
    }
    // Convert ascii packet to binary packet
    bits2bytes ( frame_bits, frame_bytes, FRAME_LEN + AUX_LEN );
    // Get packet length
    flen = frame_bytes[0];
    // If frame length is what we expected, auxlen is 0
    if ( flen == stdFLEN ) m_auxlen = 0;
    else {
        // Else auxlen is the remaining number of bytes
        m_auxlen = flen - stdFLEN;
        if ( m_auxlen < 0 || m_auxlen > AUX_LEN ) m_auxlen = 0;
    }

    // Received frame CRC
    cs1 = ( frame_bytes[pos_Check + m_auxlen] << 8 ) | frame_bytes[pos_Check + m_auxlen + 1];
    // Computed frame CRC
    cs2 = checkM10 ( frame_bytes, pos_Check + m_auxlen );

    if ( cs1 == cs2 ) fprintf ( stdout, " [OK]" );
    else              fprintf ( stdout, " [NOT OK]" );
    // If option raw, print raw frame
    if ( m_configuration.raw ) {
        for ( i = 0; i < FRAME_LEN + m_auxlen; i++ ) {
            byte = frame_bytes[i];
            fprintf ( stdout, "%02x", byte );
        }
        if ( m_configuration.verbose ) {
            fprintf ( stdout, " # %04x", cs2 );
            if ( cs1 == cs2 ) fprintf ( stdout, " [OK]" ); else fprintf ( stdout, " [NO]" );
        }
        fprintf ( stdout, "\n" );
    }
    /*else if ( frame_bytes[1] == 0x49 ) {
        if ( m_configuration.verbose == 3 ) {
            for ( i = 0; i < FRAME_LEN + m_auxlen; i++ ) {
                byte = frame_bytes[i];
                fprintf ( stdout, "%02x", byte );
            }
            fprintf ( stdout, "\n" );
        }
    }*/
    else print_pos ( cs1 == cs2 );

    fprintf ( stdout, "\n" );
}

/*
int main ( int argc, char **argv ) {

    FILE *fp;
    char *fpname;
    int i, len;
    int bit, bit0;
    int pos;
    int header_found = 0;

    
    fpname = argv[0];
    ++argv;
    while ( ( *argv ) && ( !m_configuration.wavloaded ) ) {
        if ( ( strcmp ( *argv, "-h" ) == 0 ) || ( strcmp ( *argv, "--help" ) == 0 ) ) {
            fprintf ( stderr, "%s [options] audio.wav\n", fpname );
            fprintf ( stderr, "  options:\n" );
            //fprintf(stderr, "       -v, --verbose\n");
            fprintf ( stderr, "       -r, --raw\n" );
            fprintf ( stderr, "       -c, --color\n" );
            //fprintf(stderr, "       -o, --offset\n");
            return 0;
        }
        else if ( ( strcmp ( *argv, "-v" ) == 0 ) || ( strcmp ( *argv, "--verbose" ) == 0 ) ) {
            m_configuration.verbose = 1;
        }
        else if ( ( strcmp ( *argv, "-vv" ) == 0 ) ) m_configuration.verbose = 2;
        else if ( ( strcmp ( *argv, "-vvv" ) == 0 ) ) m_configuration.verbose = 3;
        else if ( ( strcmp ( *argv, "-r" ) == 0 ) || ( strcmp ( *argv, "--raw" ) == 0 ) ) {
            m_configuration.raw = 1;
        }
        else if ( ( strcmp ( *argv, "-i" ) == 0 ) || ( strcmp ( *argv, "--invert" ) == 0 ) ) {
            m_configuration.inv = 1;  // nicht noetig
        }
        else if ( ( strcmp ( *argv, "-c" ) == 0 ) || ( strcmp ( *argv, "--color" ) == 0 ) ) {
            m_configuration.color = 1;
        }
        else if ( strcmp ( *argv, "--res" ) == 0 ) { m_configuration.res = 1; }
        else if ( ( strcmp ( *argv, "--avg" ) == 0 ) ) {
            m_configuration.avg = 1;
        }
        else if ( strcmp ( *argv, "-b" ) == 0 ) { m_configuration.b = 1; }
        else if ( strcmp ( *argv, "-b2" ) == 0 ) { m_configuration.b = 2; }
        else if ( ( strcmp ( *argv, "--ptu" ) == 0 ) ) {
            m_configuration.ptu = 1;
        }
        else {
            fp = fopen ( *argv, "rb" );
            if ( fp == NULL ) {
                fprintf ( stderr, "%s konnte nicht geoeffnet werden\n", *argv );
                return -1;
            }
            m_configuration.wavloaded = 1;
        }
        ++argv;
    }
    

    pos = FRAMESTART;

    while ( !read_bits_fsk ( fp, &bit, &len ) ) {

        if ( len == 0 ) { // reset_frame();
            if ( pos > ( pos_GPSweek + 2 ) * 2 * BITS ) {
                for ( i = pos; i < RAWBITFRAME_LEN + RAWBITAUX_LEN; i++ ) frame_rawbits[i] = 0x30 + 0;
                print_frame ( pos );//byte_count
                header_found = 0;
                pos = FRAMESTART;
            }
            //inc_m_bufPos();
            //buf[m_bufPos] = 'x';
            continue;   // ...
        }

        for ( i = 0; i < len; i++ ) {

            inc_m_bufPos ();
            buf[m_bufPos] = 0x30 + bit;  // Ascii

            if ( !header_found ) {
                header_found = IsThisAHeader ();
            }
            else {
                frame_rawbits[pos] = 0x30 + bit;  // Ascii
                pos++;

                if ( pos == RAWBITFRAME_LEN + RAWBITAUX_LEN ) {
                    frame_rawbits[pos] = '\0';
                    print_frame ( pos );//FRAME_LEN
                    header_found = 0;
                    pos = FRAMESTART;
                }
            }

        }
        if ( header_found && m_configuration.b == 1 ) {
            m_bitStart = true;

            while ( pos < RAWBITFRAME_LEN + RAWBITAUX_LEN ) {
                if ( read_rawbit ( fp, &bit ) == EOF ) break;
                frame_rawbits[pos] = 0x30 + bit;
                pos++;
            }
            frame_rawbits[pos] = '\0';
            print_frame ( pos );
            header_found = 0;
            pos = FRAMESTART;
        }
        if ( header_found && m_configuration.b >= 2 ) {
            m_bitStart = true;
            bit0 = 0;

            if ( pos % 2 ) {
                if ( read_rawbit ( fp, &bit ) == EOF ) break;
                frame_rawbits[pos] = 0x30 + bit;
                pos++;
            }

            bit0 = dpsk_bpm ( frame_rawbits, frame_bits, pos );
            pos /= 2;

            while ( pos < BITFRAME_LEN + BITAUX_LEN ) {
                if ( read_rawbit2 ( fp, &bit ) == EOF ) break;
                frame_bits[pos] = 0x31 ^ ( bit0 ^ bit );
                pos++;
                bit0 = bit;
            }
            frame_bits[pos] = '\0';
            print_frame ( pos );
            header_found = 0;
            pos = FRAMESTART;
        }
    }

    fprintf ( stdout, "\n" );

    fclose ( fp );

    return 0;
}
*/

