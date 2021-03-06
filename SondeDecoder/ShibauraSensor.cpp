#include "stdafx.h"
#include "ShibauraSensor.h"
#include "math.h"


ShibauraSensor::ShibauraSensor ()
{
}


ShibauraSensor::~ShibauraSensor ()
{
}



// Temperatur Sensor
// NTC-Thermistor Shibaura PB5-41E
//
float ShibauraSensor::get_Temp ( uint8_t * frame_bytes, int verbose ) {
    // NTC-Thermistor Shibaura PB5-41E
    // T00 = 273.15 +  0.0 , R00 = 15e3
    // T25 = 273.15 + 25.0 , R25 = 5.369e3
    // B00 = 3450.0 Kelvin // 0C..100C, poor fit low temps
    // [  T/C  , R/1e3 ] ( [P__-43]/2.0 ):
    // [ -50.0 , 204.0 ]
    // [ -45.0 , 150.7 ]
    // [ -40.0 , 112.6 ]
    // [ -35.0 , 84.90 ]
    // [ -30.0 , 64.65 ]
    // [ -25.0 , 49.66 ]
    // [ -20.0 , 38.48 ]
    // [ -15.0 , 30.06 ]
    // [ -10.0 , 23.67 ]
    // [  -5.0 , 18.78 ]
    // [   0.0 , 15.00 ]
    // [   5.0 , 12.06 ]
    // [  10.0 , 9.765 ]
    // [  15.0 , 7.955 ]
    // [  20.0 , 6.515 ]
    // [  25.0 , 5.370 ]
    // [  30.0 , 4.448 ]
    // [  35.0 , 3.704 ]
    // [  40.0 , 3.100 ]
    // -> Steinhart�Hart coefficients (polyfit):
    float p0 = 1.07303516e-03,
        p1 = 2.41296733e-04,
        p2 = 2.26744154e-06,
        p3 = 6.52855181e-08;
    // T/K = 1/( p0 + p1*ln(R) + p2*ln(R)^2 + p3*ln(R)^3 )

    // range/scale 0, 1, 2:                        // M10-pcb
    float Rs[3] = { 12.1e3 ,  36.5e3 ,  475.0e3 }; // bias/series
    float Rp[3] = { 1e20   , 330.0e3 , 3000.0e3 }; // parallel, Rp[0]=inf

    uint8_t  scT;     // {0,1,2}, range/scale voltage divider
    uint16_t ADC_RT;  // ADC12 P6.7(A7) , adr_0377h,adr_0376h
    uint16_t Tcal[2]; // adr_1000h[scT*4]

    float adc_max = 4095.0; // ADC12
    float x, R;
    float T = 0;    // T/Kelvin

    scT = frame_bytes[0x3E]; // adr_0455h
    ADC_RT = ( frame_bytes[0x40] << 8 ) | frame_bytes[0x3F];
    ADC_RT -= 0xA000;
    Tcal[0] = ( frame_bytes[0x42] << 8 ) | frame_bytes[0x41];
    Tcal[1] = ( frame_bytes[0x44] << 8 ) | frame_bytes[0x43];

    x = ( adc_max - ADC_RT ) / ADC_RT;  // (Vcc-Vout)/Vout
    if ( scT < 3 ) R = Rs[scT] / ( x - Rs[scT] / Rp[scT] );
    else         R = -1;

    if ( R > 0 )  T = 1 / ( p0 + p1*log ( R ) + p2*log ( R )*log ( R ) + p3*log ( R )*log ( R )*log ( R ) );

    if ( verbose >= 3 ) { // on-chip temperature
        uint16_t ADC_Ti_raw = ( frame_bytes[0x49] << 8 ) | frame_bytes[0x48]; // int.temp.diode, ref: 4095->1.5V
        float vti, ti;
        // INCH1A (temp.diode), slau144
        vti = ADC_Ti_raw / 4095.0 * 1.5; // V_REF+ = 1.5V, no calibration
        ti = ( vti - 0.986 ) / 0.00355;      // 0.986/0.00355=277.75, 1.5/4095/0.00355=0.1032
        fprintf ( stdout, "  (Ti:%.1fC)", ti );
        // SegmentA-Calibration:
        //uint16_t T30 = adr_10e2h; // CAL_ADC_15T30
        //uint16_t T85 = adr_10e4h; // CAL_ADC_15T85
        //float  tic = (ADC_Ti_raw-T30)*(85.0-30.0)/(T85-T30) + 30.0;
        //fprintf(stdout, "  (Tic:%.1fC)", tic);
    }

    return  T - 273.15; // Celsius
}

float ShibauraSensor::get_Tntc2 ( uint8_t * frame_bytes ) {
    // SMD ntc
    float Rs = 22.1e3;          // P5.6=Vcc
                                //  float R25 = 2.2e3;
                                //  float b = 3650.0;           // B/Kelvin
                                //  float T25 = 25.0 + 273.15;  // T0=25C, R0=R25=5k
                                // -> Steinhart�Hart coefficients (polyfit):
    float p0 = 4.42606809e-03,
        p1 = -6.58184309e-04,
        p2 = 8.95735557e-05,
        p3 = -2.84347503e-06;
    float T = 0.0;              // T/Kelvin
    uint16_t ADC_ntc2;            // ADC12 P6.4(A4)
    float x, R;

    {
        ADC_ntc2 = ( frame_bytes[0x5A] << 8 ) | frame_bytes[0x59];
        x = ( 4095.0 - ADC_ntc2 ) / ADC_ntc2;  // (Vcc-Vout)/Vout
        R = Rs / x;
        //if (R > 0)  T = 1/(1/T25 + 1/b * log(R/R25));
        if ( R > 0 )  T = 1 / ( p0 + p1*log ( R ) + p2*log ( R )*log ( R ) + p3*log ( R )*log ( R )*log ( R ) );
    }
    return T - 273.15;
}

// Humidity Sensor
// U.P.S.I.
//
#define FREQ_CAPCLK (8e6/2)      // 8 MHz XT2 crystal, InputDivider IDx=01 (/2)
#define LN2         0.693147181
#define ADR_108A    1000.0       // 0x3E8=1000

float ShibauraSensor::get_count_RH ( uint8_t * frame_bytes  ) {  // capture 1000 rising edges
    uint32_t TBCCR1_1000 = frame_bytes[0x35] | ( frame_bytes[0x36] << 8 ) | ( frame_bytes[0x37] << 16 );
    return TBCCR1_1000 / ADR_108A;
}
float ShibauraSensor::get_TLC555freq ( uint8_t * frame_bytes  ) {
    return FREQ_CAPCLK / get_count_RH ( frame_bytes  );
}