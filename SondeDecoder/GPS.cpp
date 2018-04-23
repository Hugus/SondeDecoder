#include "stdafx.h"
#include "GPS.h"
#include "Utils.h"
#define _USE_MATH_DEFINES
#include <cmath> 




GPS::GPS ()
{
}


GPS::~GPS ()
{
}

/* -------------------------------------------------------------------------- */
/*
* Convert GPS Week and Seconds to Modified Julian Day.
* - Adapted from sci.astro FAQ.
* - Ignores UTC leap seconds.
*/
void GPS::Gps2Date ( long GpsWeek, long GpsSeconds, int *Year, int *Month, int *Day ) {

    long GpsDays, Mjd;
    long J, C, Y, M;

    GpsDays = GpsWeek * 7 + ( GpsSeconds / 86400 );
    Mjd = 44244 + GpsDays;

    J = Mjd + 2468570;
    C = 4 * J / 146097;
    J = J - ( 146097 * C + 3 ) / 4;
    Y = 4000 * ( J + 1 ) / 1461001;
    J = J - 1461 * Y / 4 + 31;
    M = 80 * J / 2447;
    *Day = J - 2447 * M / 80;
    J = M / 11;
    *Month = M + 2 - ( 12 * J );
    *Year = 100 * ( C - 49 ) + Y + J;
}

int GPS::get_GPSweek ( uint8_t * frame_bytes, uint32_t posGpsWeek, Date & date ) {
    int i;
    unsigned byte;
    uint8_t gpsweek_bytes[2];
    int gpsweek;

    for ( i = 0; i < 2; i++ ) {
        byte = frame_bytes[posGpsWeek + i];
        gpsweek_bytes[i] = byte;
    }

    gpsweek = ( gpsweek_bytes[0] << 8 ) + gpsweek_bytes[1];
    date.week = gpsweek;

    if ( gpsweek < 0 || gpsweek > 3000 ) return -1;

    return 0;
}


int GPS::get_GPStime ( uint8_t * frame_bytes, uint32_t posGpsTow, Date & date ) {

    int gpstime = getInt32 ( frame_bytes, posGpsTow ) ;
    //ms = gpstime % 1000;
    gpstime /= 1000;
    date.gpssec = gpstime;

    int day = gpstime / ( 24 * 3600 );
    gpstime %= ( 24 * 3600 );

    if ( ( day < 0 ) || ( day > 6 ) ) return -1;
    date.wday = day;
    date.std = gpstime / 3600;
    date.min = ( gpstime % 3600 ) / 60;
    date.sek = gpstime % 60;

    return 0;
}

double B60B60 = 0xB60B60;  // 2^32/360 = 0xB60B60.xxx

void GPS::get_GPSlat ( uint8_t * frame_bytes, uint32_t posGpsLat, Date & date  )
{
    date.lat = getInt32 ( frame_bytes, posGpsLat ) / B60B60;
}

void GPS::get_GPSlon ( uint8_t * frame_bytes, uint32_t posGpsLon, Date & date )
{
    date.lon = getInt32 ( frame_bytes, posGpsLon ) / B60B60;
}

void GPS::get_GPSalt ( uint8_t * frame_bytes, uint32_t posGpsAlt, Date & date )
{
    date.alt = getInt32 ( frame_bytes, posGpsAlt ) / 1000.0;
}

int 
GPS::get_GPSvel 
( 
    uint8_t * frame_bytes,
    uint32_t posGpsVelE,
    uint32_t posGpsVelN,
    uint32_t posGpsVelU, 
    Date & date 
) 
{
    int i;
    unsigned byte;
    uint8_t gpsVel_bytes[2];
    short vel16;
    double vx, vy, dir, alpha;
    const double ms2kn100 = 2e2;  // m/s -> knots: 1 m/s = 3.6/1.852 kn = 1.94 kn

    for ( i = 0; i < 2; i++ ) {
        byte = frame_bytes[posGpsVelE + i];
        gpsVel_bytes[i] = byte;
    }
    vel16 = gpsVel_bytes[0] << 8 | gpsVel_bytes[1];
    vx = vel16 / ms2kn100; // ost

    for ( i = 0; i < 2; i++ ) {
        byte = frame_bytes[posGpsVelN + i];
        gpsVel_bytes[i] = byte;
    }
    vel16 = gpsVel_bytes[0] << 8 | gpsVel_bytes[1];
    vy = vel16 / ms2kn100; // nord

    date.vx = vx;
    date.vy = vy;
    date.vH = sqrt ( vx*vx + vy*vy );
    ///*
    alpha = atan2 ( vy, vx ) * 180 / M_PI;  // ComplexPlane (von x-Achse nach links) - GeoMeteo (von y-Achse nach rechts)
    dir = 90 - alpha;                  // z=x+iy= -> i*conj(z)=y+ix=re(i(pi/2-t)), Achsen und Drehsinn vertauscht
    if ( dir < 0 ) dir += 360;         // atan2(y,x)=atan(y/x)=pi/2-atan(x/y) , atan(1/t) = pi/2 - atan(t)
    date.vD2 = dir;
    //*/
    dir = atan2 ( vx, vy ) * 180 / M_PI;
    if ( dir < 0 ) dir += 360;
    date.vD = dir;

    for ( i = 0; i < 2; i++ ) {
        byte = frame_bytes[posGpsVelU + i];
        gpsVel_bytes[i] = byte;
    }
    vel16 = gpsVel_bytes[0] << 8 | gpsVel_bytes[1];
    date.vV = vel16 / ms2kn100;

    return 0;
}

int GPS::get_SN ( uint8_t * frame_bytes, uint32_t posGpsAlt, Date & date  ) {
    int i;
    unsigned byte;
    uint8_t sn_bytes[5];

    for ( i = 0; i < 11; i++ ) date.SN[i] = ' '; date.SN[11] = '\0';

    for ( i = 0; i < 5; i++ ) {
        byte = frame_bytes[posGpsAlt + i];
        sn_bytes[i] = byte;
    }

    byte = sn_bytes[2];
    sprintf ( date.SN, "%1X%02u", ( byte >> 4 ) & 0xF, byte & 0xF );
    byte = sn_bytes[3] | ( sn_bytes[4] << 8 );
    sprintf ( date.SN + 3, " %1X %1u%04u", sn_bytes[0] & 0xF, ( byte >> 13 ) & 0x7, byte & 0x1FFF );

    return 0;
}
