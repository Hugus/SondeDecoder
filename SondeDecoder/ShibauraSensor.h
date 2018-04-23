#pragma once

#include <stdint.h>

class ShibauraSensor
{
public:
    ShibauraSensor ();
    ~ShibauraSensor ();

    static float get_Temp ( int csOK, uint8_t * frame_bytes, int verbose ) ;
    static float get_Tntc2 ( int csOK, uint8_t * frame_bytes ) ;
    static float get_count_RH ( uint8_t * frame_bytes ) ;
    static float get_TLC555freq ( uint8_t * frame_bytes ) ;
};

