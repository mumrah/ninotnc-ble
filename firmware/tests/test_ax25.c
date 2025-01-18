#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "ax25.h"
#include "aprs.h"


uint8_t aprs_loc_notime[] = {
    0x82, 0xA0, 0x88, 0xAE, 0x62, 0x6E, 0xE0, 0x9C, 0x68, 0x8A, 0xAE, 0x8A, 0x40, 0xE0, 0xAE, 0x92, 
    0x88, 0x8A, 0x62, 0x40, 0x62, 0xAE, 0x92, 0x88, 0x8A, 0x64, 0x40, 0x63, 0x03, 0xF0, 0x21, 0x33, 
    0x36, 0x30, 0x34, 0x2E, 0x32, 0x33, 0x4E, 0x53, 0x30, 0x37, 0x38, 0x33, 0x34, 0x2E, 0x37, 0x35, 
    0x57, 0x23, 0x50, 0x48, 0x47, 0x37, 0x31, 0x34, 0x30, 0x44, 0x69, 0x67, 0x69, 0x70, 0x65, 0x61, 
    0x74, 0x65, 0x72, 0x20, 0x2D, 0x20, 0x47, 0x72, 0x61, 0x6E, 0x76, 0x69, 0x6C, 0x6C, 0x65, 0x20, 
    0x43, 0x6F, 0x75, 0x6E, 0x74, 0x79, 0x2C, 0x20, 0x4E, 0x43
};
uint16_t aprs_loc_notime_len = 90;


uint8_t aprs_loc_time[] = {
    0x82, 0xA0, 0xA8, 0xA8, 0x68, 0x40, 0x60, 0x96, 0x8A, 0x68, 0xAA, 0x40, 0x40, 0x62, 0x9C, 0x68, 
    0x8A, 0xAE, 0x8A, 0x40, 0xE1, 0x03, 0xF0, 0x40, 0x30, 0x36, 0x30, 0x38, 0x35, 0x32, 0x7A, 0x33, 
    0x36, 0x30, 0x34, 0x2E, 0x33, 0x32, 0x4E, 0x2F, 0x30, 0x37, 0x38, 0x34, 0x30, 0x2E, 0x38, 0x33, 
    0x57, 0x5F, 0x31, 0x36, 0x33, 0x2F, 0x30, 0x30, 0x31, 0x67, 0x30, 0x30, 0x36, 0x74, 0x30, 0x35, 
    0x30, 0x72, 0x30, 0x30, 0x30, 0x70, 0x30, 0x30, 0x30, 0x50, 0x30, 0x30, 0x30, 0x68, 0x33, 0x37, 
    0x62, 0x30, 0x39, 0x39, 0x37, 0x35, 0x2E, 0x44, 0x73, 0x56, 0x50, 0x0D
};
uint16_t aprs_loc_time_len = 92;


MunitResult test_location_notime(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture); 
    
    AX25Packet packet;
    Loc loc;
    ax25_init(&packet, aprs_loc_notime, aprs_loc_notime_len);

    assert_int8(ax25_frame_type(&packet, aprs_loc_notime), ==, AX25_UNNUMBERED);
    assert_int8(ax25_u_subtype(&packet, aprs_loc_notime), ==, AX25_U_UI);
    assert_int8(ax25_packet_pid_field(&packet, aprs_loc_notime), ==, AX25_PID_NO_LAYER_3);
    assert_char('!', ==, aprs_data_type(&packet, aprs_loc_notime));

    aprs_notime_latitude(&packet, aprs_loc_notime, &loc);
    assert_int8(loc.deg, ==, 36);
    assert_int8(loc.min, ==, 04);
    assert_int8(loc.sec, ==, 13);
    assert_true(loc.north_or_east);

    aprs_notime_longitude(&packet, aprs_loc_notime, &loc);
    assert_int8(loc.deg, ==, 78);
    assert_int8(loc.min, ==, 34);
    assert_int8(loc.sec, ==, 45);
    assert_false(loc.north_or_east);

    // Wide digi symbol
    assert_char(aprs_notime_symbol_table(&packet, aprs_loc_notime), ==, 'S');
    assert_char(aprs_notime_symbol_code(&packet, aprs_loc_notime), ==, '#');

    return MUNIT_OK;
}

MunitResult test_location_time(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture); 
    
    AX25Packet packet;
    Loc loc;
    ax25_init(&packet, aprs_loc_time, aprs_loc_time_len);

    assert_int8(ax25_frame_type(&packet, aprs_loc_time), ==, AX25_UNNUMBERED);
    assert_int8(ax25_u_subtype(&packet, aprs_loc_time), ==, AX25_U_UI);
    assert_int8(ax25_packet_pid_field(&packet, aprs_loc_time), ==, AX25_PID_NO_LAYER_3);
    assert_char('@', ==, aprs_data_type(&packet, aprs_loc_time));

    aprs_time_latitude(&packet, aprs_loc_time, &loc);
    assert_int8(loc.deg, ==, 36);
    assert_int8(loc.min, ==, 04);
    assert_int8(loc.sec, ==, 19);
    assert_true(loc.north_or_east);

    aprs_time_longitude(&packet, aprs_loc_time, &loc);
    assert_int8(loc.deg, ==, 78);
    assert_int8(loc.min, ==, 40);
    assert_int8(loc.sec, ==, 49);
    assert_false(loc.north_or_east);

    // WX symbol
    assert_char(aprs_time_symbol_table(&packet, aprs_loc_time), ==, '/');
    assert_char(aprs_time_symbol_code(&packet, aprs_loc_time), ==, '_');

    return MUNIT_OK;
}



MunitTest tests[] = {
  {
    "/test_location_notime", /* name */
    test_location_notime, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_location_time", /* name */
    test_location_time, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  /* Mark the end of the array with an entry where the test
   * function is NULL */
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite suite = {
  "/ax25-tests", /* name */
  tests, /* tests */
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE /* options */
};

int main (int argc, char *const * argv) {
  return munit_suite_main(&suite, NULL, argc, argv);
}
