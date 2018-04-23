#pragma once

#include "GPS.h"
#include <Mmdeviceapi.h>
#include <stdint.h>

typedef struct Configuration_s {
    int verbose ;  // Detailed display
    int raw ;      // Raw Frames
    int inv ;      // Inverted Signal
    int res ;      // more accurate bit measurement
    int b ;
    int color ;
    int ptu ;
    int wavloaded ;
    Configuration_s () // Constructor initialises structure
        : verbose ( 0 )
        , raw ( 0 )
        , inv ( 0 )
        , res ( 0 )
        , b ( 0 )
        , color ( 0 )
        , ptu ( 0 )
        , wavloaded ( 0 ) {}
} Configuration_t;

typedef struct AudioBuffer {
    BYTE * pData ;
    UINT32 size ;
    UINT32 currentPosition ;
    AudioBuffer ()
        : pData ( NULL )
        , size ( 0 )
        , currentPosition ( 0 ) {}
} ;

#define BITS 8
#define HEADLEN 32  // HEADLEN+HEADOFS=32 <= strlen(header)
#define HEADOFS  0

#define FRAME_LEN       (100+1)   // 0x64+1
#define BITFRAME_LEN    (FRAME_LEN*BITS)
#define RAWBITFRAME_LEN (BITFRAME_LEN*2)

#define FRAMESTART 0
#define AUX_LEN        20
#define BITAUX_LEN    (AUX_LEN*BITS)
#define RAWBITAUX_LEN (BITAUX_LEN*2)

class M10Decoder
{
public:
    M10Decoder ();
    ~M10Decoder ();

    HRESULT SetFormat ( WAVEFORMATEX *pwfx );
    HRESULT CopyData ( BYTE * pData, UINT32 numFramesAvailable, BOOL * bDone );

    int print_pos ( int csOK ) ;

    void print_frame ( int pos ) ;


private:
    /// Read a sample regardless of its size (8 or 16 bits)
    /// @returns signed sample as an integer
    int readSignedSample () ;

    /// Read one or more identical consecutive bits
    int readBitsFsk ( int *bit, int *len ) ;

    // Reads samples on one bit duration
    // If sum of samples > 0 bit is 1, else it is 0
    int readRawbit  ( int *bit ) ;
    // Reads samples on two bits duration
    // Sums first half of sample
    // Substract second half
    // If sum of these sums > 0 bit is 1, else it is 0
    int readRawbit2 ( int *bit ) ;

    // Increment header buffer index
    void incrementBufferIndex () ;

    /// Search in header circular buffer for a correct header
    /// @returns 1 if header found, -1 if found but inverted, 0 if not found
    int IsThisAHeader () ;

    /// Compute frame CRC
    int checkM10 ( uint8_t *msg, int len ) ;
    /// Sub function used by checkM10
    int update_checkM10 ( int c, uint8_t b ) ;

private:
    WORD m_bitsPerSample ;
    DWORD m_samplePerSec ;
    WORD m_nChannels ;

    double m_samplePerBit ;
    double m_baudRate ;

    Date m_date ;

    Configuration_t m_configuration ;

    unsigned long m_sampleCount ;
    double m_bitSeparator ;

    // Input buffer
    AudioBuffer m_audioBuffer ;

    int m_par ;
    int m_parAlt ;

    // Are we dealing with first bit of frame after heading ?
    bool m_bitStart ;
    // Bit index in frame starting after header
    unsigned long m_sCount ;

    // Output buffers
    uint8_t frame_bytes[FRAME_LEN + AUX_LEN + 2];
    char frame_rawbits[RAWBITFRAME_LEN + RAWBITAUX_LEN + 16];  // frame_rawbits-32="11001100110011001010011001001100";
    char frame_bits[BITFRAME_LEN + BITAUX_LEN + 8];
    char buf[HEADLEN];

    int m_bufPos ;

    int m_auxlen = 0; // 0 .. 0x76-0x64

    bool m_isHeaderFound ;
};

