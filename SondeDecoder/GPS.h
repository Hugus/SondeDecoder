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
    static int get_GPSweek ( uint8_t * frame_bytes, uint32_t posGpsWeek, Date & date ) ;
    static int get_GPStime ( uint8_t * frame_bytes, uint32_t posGpsTow, Date & date ) ;
    static void get_GPSlat ( uint8_t * frame_bytes, uint32_t posGpsLat, Date & date ) ;
    static void get_GPSlon ( uint8_t * frame_bytes, uint32_t posGpsLon, Date & date ) ;
    static void get_GPSalt ( uint8_t * frame_bytes, uint32_t posGpsAlt, Date & date ) ;
    static int get_GPSvel (
        uint8_t * frame_bytes,
            uint32_t posGpsVelE,
            uint32_t posGpsVelN,
            uint32_t posGpsVelU,
            Date & date  ) ;
    static int get_SN ( uint8_t * frame_bytes, uint32_t posGpsAlt, Date & date ) ;




};