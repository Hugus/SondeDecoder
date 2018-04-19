#pragma once
class GPS
{
public:
    GPS ();
    ~GPS ();

    static void GPS::Gps2Date ( long GpsWeek, long GpsSeconds, int *Year, int *Month, int *Day ) ;
};

typedef struct {
    int week; int gpssec;
    int jahr; int monat; int tag;
    int wday;
    int std; int min; int sek;
    double lat; double lon; double alt;
    double vH; double vD; double vV;
    double vx; double vy; double vD2;
    char SN[12];
} Date_t;