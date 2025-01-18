#ifndef APRS_H
#define APRS_H

#include "ax25.h"

/*
APRS.fi behavior:

beacon is @
message is :

TNC TX test is =
*/

#define APRS_TYPE_POS_NOTIME_NOMSG      '!'
#define APRS_TYPE_POS_TIME_NOMSG        '/'
#define APRS_TYPE_MESSAGE               ':'
#define APRS_TYPE_OBJECT                ';'
#define APRS_TYPE_STATION_CAPABILITIES  '<'
#define APRS_TYPE_POS_NOTIME_MSG        '='
#define APRS_TYPE_STATUS                '>'
#define APRS_TYPE_POS_TIME_MSG          '@'
#define APRS_TYPE_TELEMETRY             'T'
#define APRS_TYPE_WEATHER_NOPOS         '_'

typedef struct {
    uint8_t deg;
    uint8_t min;
    uint8_t sec;
    bool north_or_east;
} Loc;

typedef struct {
    int16_t year;    ///< 0..4095
    int8_t month;    ///< 1..12, 1 is January
    int8_t day;      ///< 1..28,29,30,31 depending on month
    int8_t hour;     ///< 0..23
    int8_t min;      ///< 0..59
    int8_t sec;      ///< 0..59
} aprs_datetime_t;

uint16_t aprs_status(uint8_t * buffer, AX25Call * src_call, AX25Call * dest_call, char * status);

void print_loc_dms(Loc * loc, uint8_t * buffer);

void print_loc_dd(Loc * loc, uint8_t * buffer);

char aprs_data_type(AX25Packet * packet, uint8_t * buffer);

void aprs_time_latitude(AX25Packet * packet, uint8_t * buffer, Loc * latitude);

void aprs_notime_latitude(AX25Packet * packet, uint8_t * buffer, Loc * latitude);

void aprs_time_longitude(AX25Packet * packet, uint8_t * buffer, Loc * longitude);

void aprs_notime_longitude(AX25Packet * packet, uint8_t * buffer, Loc * longitude);

char aprs_time_symbol_table(AX25Packet * packet, uint8_t * buffer);

char aprs_notime_symbol_table(AX25Packet * packet, uint8_t * buffer);

char aprs_time_symbol_code(AX25Packet * packet, uint8_t * buffer);

char aprs_notime_symbol_code(AX25Packet * packet, uint8_t * buffer);

aprs_datetime_t aprs_time(AX25Packet * packet, uint8_t * buffer);

uint16_t aprs_comment(AX25Packet * packet, uint8_t * buffer, uint16_t len, uint8_t * dest);

double loc_dd(Loc * loc);

double loc_distance(double lat1_dd, double lon1_dd, double lat2_dd, double lon2_dd);

#endif
