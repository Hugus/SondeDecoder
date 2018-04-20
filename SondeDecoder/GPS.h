#pragma once

#include <stdint.h>

typedef struct {
    int week; int gpssec;
    int jahr; int monat; int tag;
    int wday;
    int std; int min; int sek;
    double lat; double lon; double alt;
    double vH; double vD; double vV;
    double vx; double vy; double vD2;
    char SN[12];
} Date ;

class GPS
{
public:
    GPS ();
    ~GPS ();

    static void Gps2Date ( long GpsWeek, long GpsSeconds, int *Year, int *Month, int *Day ) ;
    static int get_GPSweek ( char * frame_bytes, uint32_t posGpsWeek, Date & date ) ;
    static int get_GPStime ( char * frame_bytes, uint32_t posGpsTow, Date & date ) ;
    static int get_GPSlat ( char * frame_bytes, uint32_t posGpsLat, Date & date ) ;
    static int get_GPSlon ( char * frame_bytes, uint32_t posGpsLon, Date & date ) ;
    static int get_GPSalt ( char * frame_bytes, uint32_t posGpsAlt, Date & date ) ;
    static int get_GPSvel (
            char * frame_bytes,
            uint32_t posGpsVel,
            uint32_t posGpsVelN,
            uint32_t posGpsVelU,
            Date & date  ) ;
    static int get_SN ( char * frame_bytes, uint32_t posGpsAlt, Date & date ) ;




};