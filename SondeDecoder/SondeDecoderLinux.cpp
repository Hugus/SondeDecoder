#include "M10Decoder.h"
#include "Utils.h"
#include <stdio.h>
#include <iostream>


int main( int argc, char ** argv ) {

    FILE *fp = stdin;

    if ( argc == 2 ) {
       fp = fopen( argv[1],  "rb" ) ;
    }

    uint64_t bitsPerSample ;
    uint64_t samplePerSec ;
    uint64_t nChannels ;
    double samplePerBit ;
    SampleType sampleType = ST_INT ;
  
    M10Decoder m10Decoder ;

    if ( read_wav_header( fp,
                          m10Decoder.getBaudRate(),
			  &nChannels,
                          &bitsPerSample,
                          &samplePerSec,
                          &samplePerBit ) != 0 )
   {
      std::cerr << "Error while reading input file header. Aborting." << std::endl ;
   }


    m10Decoder.SetFormat( bitsPerSample,
                          samplePerSec,
                          nChannels,
                          sampleType ) ;

   int byte = fgetc( fp ) ;

   while ( byte != EOF )
   {
      int sample = 0 ; 
      for (uint64_t i = 0; i < nChannels; i++) {
          if (i != 0)
          {
             continue ;
          }
          sample = byte;

          if (bitsPerSample == 16) {
              byte = fgetc(fp);
              if (byte == EOF) 
              {
                  break ;
              }
              sample +=  byte << 8;
          }

      }
  
      int s = 0 ;
      if (bitsPerSample ==  8)  s = sample-128;   // 8bit: 00..FF, centerpoint 0x80=128
      if (bitsPerSample == 16)  s = (short)sample;

      m10Decoder.CopyData( reinterpret_cast< uint8_t *>( s ), 1, NULL ) ;

      byte = fgetc( fp ) ;
   }

   return 0 ;
}
