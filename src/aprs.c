#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "ax25.h"
#include "aprs.h"

#define APRS_DATA_TYPE_BYTE_POS 2

#define APRS_TIME_TYPE_BYTE_POS 9
#define APRS_TIME_1_BYTE_POS 3
#define APRS_TIME_2_BYTE_POS 5
#define APRS_TIME_3_BYTE_POS 7

#define APRS_LOC_NOTIME_LAT_BYTE_POS 3
#define APRS_LOC_TIME_LAT_BYTE_POS 10
#define APRS_LOC_NOTIME_LON_BYTE_POS 12
#define APRS_LOC_TIME_LON_BYTE_POS 19

#define APRS_TIME_SYM_TBL_BYTE_POS 18
#define APRS_NOTIME_SYM_TBL_BYTE_POS 11
#define APRS_TIME_SYM_CODE_BYTE_POS 28
#define APRS_NOTIME_SYM_CODE_BYTE_POS 21

#define APRS_COMMENT_BYTE_POS 29

static void aprs_latitude(uint8_t offset, uint8_t * buffer, Loc * latitude) {
    char buf[3];

    memset(buf, '\0', 3);
    memcpy(buf, &buffer[offset], 2);
    printf("LAT DEG %s\n", buf);
    latitude->deg = atoi(buf);

    memset(buf, '\0', 3);
    memcpy(buf, &buffer[offset + 2], 2);
    printf("LAT MIN %s\n", buf);
    latitude->min = atoi(buf);
    
    memset(buf, '\0', 3);
    memcpy(buf, &buffer[offset + 5], 2);
    printf("LAT SEC %s\n", buf);
    latitude->sec = (uint8_t)(atof(buf) * 60.0 / 100.0);

    latitude->north_or_east = buffer[offset + 7] == 'N';
}

static void aprs_longitude(uint8_t offset, uint8_t * buffer, Loc * longitude) {
    char buf[4];

    memset(buf, '\0', 4);
    memcpy(buf, &buffer[offset], 3);
    printf("LON DEG %s\n", buf);
    longitude->deg = atoi(buf);

    memset(buf, '\0', 4);
    memcpy(buf, &buffer[offset + 3], 2);
    printf("LON MIN %s\n", buf);
    longitude->min = atoi(buf);
    
    memset(buf, '\0', 4);
    memcpy(buf, &buffer[offset + 6], 2);
    printf("LON SEC %s\n", buf);
    longitude->sec = (uint8_t)(atof(buf) * 60.0 / 100.0);

    longitude->north_or_east = buffer[offset + 8] == 'E';
}


const uint8_t status[] = {
  0xc0, 0x00, // FEND DATA
  0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80,     // APFIIO-0
  0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01,     // K4DBZ-9
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20,  // 0x03 0xF0 ">NinoTNC BLE "
  0x69, 0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0 // "is working!" FEND
};
const uint8_t status_len = 43;

uint16_t aprs_status(uint8_t * buffer, AX25Call * src_call, AX25Call * dest_call, char * status) {
    ax25_encode_call(&buffer[0], dest_call);
    ax25_encode_call(&buffer[7], src_call);
    buffer[14] = 0x03; // UI frame
    buffer[15] = 0xF0; // No Layer 3
    buffer[16] = '>'; // Status message type
    memcpy(&buffer[17], status, strlen(status));
    return 17 + strlen(status);
}

char aprs_data_type(AX25Packet * packet, uint8_t * buffer) {
    return buffer[packet->control_byte_pos + APRS_DATA_TYPE_BYTE_POS];
}

void print_loc_dms(Loc * loc, uint8_t * buffer) {
    sprintf((char *) buffer, "%02d°%02d'%02d\"", loc->deg, loc->min, loc->sec);
}

double loc_dd(Loc * loc) {
    double sign = 1.0;
    if (!loc->north_or_east) {
        sign = -1.0;
    }
    double res = loc->deg + (loc->min / 60.) + (loc->sec / 3600.);
    res = res * sign;
    return res;
}

void print_loc_dd(Loc * loc, uint8_t * buffer) {
    float dd = loc_dd(loc);
    sprintf((char *) buffer, "%+2.2f", dd);
}

void aprs_time_latitude(AX25Packet * packet, uint8_t * buffer, Loc * latitude) {
    uint8_t offset = packet->control_byte_pos + APRS_LOC_TIME_LAT_BYTE_POS;
    aprs_latitude(offset, buffer, latitude);
}

void aprs_notime_latitude(AX25Packet * packet, uint8_t * buffer, Loc * latitude) {
    uint8_t offset = packet->control_byte_pos + APRS_LOC_NOTIME_LAT_BYTE_POS;
    aprs_latitude(offset, buffer, latitude);
}

void aprs_time_longitude(AX25Packet * packet, uint8_t * buffer, Loc * longitude) {
    uint8_t offset = packet->control_byte_pos + APRS_LOC_TIME_LON_BYTE_POS;
    aprs_longitude(offset, buffer, longitude);
}

void aprs_notime_longitude(AX25Packet * packet, uint8_t * buffer, Loc * longitude) {
    uint8_t offset = packet->control_byte_pos + APRS_LOC_NOTIME_LON_BYTE_POS;
    aprs_longitude(offset, buffer, longitude);
}

char aprs_notime_symbol_table(AX25Packet * packet, uint8_t * buffer) {
    return buffer[packet->control_byte_pos + APRS_NOTIME_SYM_TBL_BYTE_POS];
}

char aprs_time_symbol_table(AX25Packet * packet, uint8_t * buffer) {
    return buffer[packet->control_byte_pos + APRS_TIME_SYM_TBL_BYTE_POS];
}

char aprs_notime_symbol_code(AX25Packet * packet, uint8_t * buffer) {
    return buffer[packet->control_byte_pos + APRS_NOTIME_SYM_CODE_BYTE_POS];
}

char aprs_time_symbol_code(AX25Packet * packet, uint8_t * buffer) {
    return buffer[packet->control_byte_pos + APRS_TIME_SYM_CODE_BYTE_POS];
}

/*

@ 202737h 3604.22N \0 7834.65W k 051/000 /A=000417 msg capable. listening 444.525 82.5!waL!

*/

uint16_t aprs_comment(AX25Packet * packet, uint8_t * buffer, uint16_t len, uint8_t * dest) {
    uint16_t comment_len = len - (packet->control_byte_pos + APRS_COMMENT_BYTE_POS);
    memcpy(dest, &buffer[packet->control_byte_pos + APRS_COMMENT_BYTE_POS], comment_len);
    return comment_len;
}

aprs_datetime_t aprs_time(AX25Packet * packet, uint8_t * buffer) {
    char time_type = buffer[packet->control_byte_pos + APRS_TIME_TYPE_BYTE_POS];
    aprs_datetime_t dt;
    dt.day = 0;
    dt.month = 0;
    dt.year = 0;
    if (time_type == 'z') {
        // zulu DHM
    } else if (time_type == '/') {
        // local DHM
    } else if (time_type == 'h') {
        // zulu HMS
        dt.day = 0;
        dt.month = 0;
        dt.year = 0;
        char buf[3];
        buf[2] = '\0';
        memcpy(buf, &buffer[packet->control_byte_pos + APRS_TIME_1_BYTE_POS], 2);
        dt.hour = atoi(buf);
        memcpy(buf, &buffer[packet->control_byte_pos + APRS_TIME_2_BYTE_POS], 2);
        dt.min = atoi(buf);
        memcpy(buf, &buffer[packet->control_byte_pos + APRS_TIME_3_BYTE_POS], 2);
        dt.sec = atoi(buf);
        //memcpy(print_buffer, &buffer[packet->control_byte_pos + APRS_TIME_1_BYTE_POS], 6);
    }
    return dt;
}

/**
 * Taken from https://stackoverflow.com/a/21623206
 * 
 * function distance(lat1, lon1, lat2, lon2) {
  const r = 6371; // km
  const p = Math.PI / 180;

  const a = 0.5 - Math.cos((lat2 - lat1) * p) / 2
                + Math.cos(lat1 * p) * Math.cos(lat2 * p) *
                  (1 - Math.cos((lon2 - lon1) * p)) / 2;

  return 2 * r * Math.asin(Math.sqrt(a));
}
 */
double loc_distance(double lat1_dd, double lon1_dd, double lat2_dd, double lon2_dd) {
    double p =  M_PI / 180.;

    double a1 = 0.5;
    double a2 = cos((lat2_dd - lat1_dd) * p) / 2.0;
    double a3 = cos(lat1_dd * p) * cos(lat2_dd * p) * (1.0 - cos((lon2_dd - lon1_dd) * p)) / 2.0;
    double a = a1 - a2 + a3;
    double res = 12742.0 * asin(sqrt(a));
    return res;
}
