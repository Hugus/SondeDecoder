#pragma once

#include <stdint.h>

class ShibauraSensor
{
public:
    ShibauraSensor ();
    ~ShibauraSensor ();

    static float get_Temp ( int csOK, char * frame_bytes, int verbose ) ;
    static float get_Tntc2 ( int csOK, char * frame_bytes ) ;
    static float get_count_RH ( char * frame_bytes ) ;
    static float get_TLC555freq ( char * frame_bytes ) ;
};

