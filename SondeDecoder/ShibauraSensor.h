#pragma once

#include <stdint.h>

class ShibauraSensor
{
public:
    ShibauraSensor ();
    ~ShibauraSensor ();

    static float ShibauraSensor::get_Temp ( int csOK, char * frame_bytes, int verbose ) ;
};

