#include "stdafx.h"
#include "GPS.h"


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

int GPS::get_GPSweek () {
    int i;
    unsigned byte;
    ui8_t gpsweek_bytes[2];
    int gpsweek;

    for ( i = 0; i < 2; i++ ) {
        byte = frame_bytes[pos_GPSweek + i];
        gpsweek_bytes[i] = byte;
    }

    gpsweek = ( gpsweek_bytes[0] << 8 ) + gpsweek_bytes[1];
    m_date.week = gpsweek;

    if ( gpsweek < 0 || gpsweek > 3000 ) return -1;

    return 0;
}

