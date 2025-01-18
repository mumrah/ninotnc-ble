#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "kiss.h"

void parse_kiss(KissState * kiss, uint8_t * buffer, uint16_t len) {
    for (uint16_t i=0; i<len; i++) {
        kiss_feed(kiss, buffer[i], 0);
    }
}

MunitResult test_sample_frame(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    uint8_t status[43] = {
        0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
        0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
        0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
    };
    uint16_t status_len = 43;

    KissState kiss;
    kiss_init(&kiss, 0);
    parse_kiss(&kiss, status, status_len);

    assert_true(kiss.haveFrame);
    assert_int16(kiss.pos, ==, 41);

    KissFrame frame;
    kiss_read(&kiss, &frame);
    assert_int8(frame.type, ==, KISS_DATA);
    assert_int16(frame.len, ==, 40);
    assert_int8(frame.hdlc, ==, 0);

    return MUNIT_OK;
}

MunitResult test_escape(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    // Taken from wikipedia example https://en.wikipedia.org/wiki/KISS_(amateur_radio_protocol)#Send_the_bytes_0xC0,_0xDB_out_of_TNC_port_0
    uint8_t status[7] = {
        0xC0, 0x00, 0xDB, 0xDC, 0xDB, 0xDD, 0xC0
    };
    uint16_t status_len = 7;

    KissState kiss;
    kiss_init(&kiss, 0);
    parse_kiss(&kiss, status, status_len);

    assert_true(kiss.haveFrame);
    assert_int16(kiss.pos, ==, 3);

    KissFrame frame;
    kiss_read(&kiss, &frame);
    assert_int8(frame.type, ==, KISS_DATA);
    assert_int16(frame.len, ==, 2);
    assert_int8(frame.hdlc, ==, 0);
    assert_int8(frame.buffer[0], ==, 0xC0);
    assert_int8(frame.buffer[1], ==, 0xDB);

    return MUNIT_OK;
}

KissFrame test_encode_decode(uint8_t * data, uint16_t data_len, uint8_t expected_frame_type, uint8_t expected_hdlc) {
    KissState kiss;
    kiss_init(&kiss, 0);
    parse_kiss(&kiss, data, data_len);

    // Ensure we actually parsed a frame
    assert_true(kiss.haveFrame);
    KissFrame frame;
    kiss_read(&kiss, &frame);

    assert_int8(frame.type, ==, expected_frame_type);
    assert_int8(frame.hdlc, ==, expected_hdlc);
    
    uint8_t new_data[100];
    uint16_t new_len = kiss_encode(&frame, new_data);

    assert_int16(new_len, ==, data_len);
    assert_memory_equal(new_len, new_data, data);

    return frame;
}

MunitResult test_hdlc(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);

    uint8_t data[8] = {
        0xC0, 0x50, 'H', 'e', 'l', 'l', 'o', 0xC0
    };
    uint16_t data_len = 8;

    KissFrame frame = test_encode_decode(data, data_len, KISS_DATA, 5);
    assert_memory_equal(frame.len, frame.buffer, "Hello");

    return MUNIT_OK;
}

MunitResult test_encode(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    uint8_t status[43] = {
        0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
        0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
        0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
    };
    uint16_t status_len = 43;

    KissState kiss;
    kiss_init(&kiss, 0);
    parse_kiss(&kiss, status, status_len);

    KissFrame frame;
    kiss_read(&kiss, &frame);

    uint8_t newStatus[100];
    uint16_t newLen = kiss_encode(&frame, newStatus);

    assert_int16(newLen, ==, status_len);
    assert_memory_equal(newLen, newStatus, status);

    return MUNIT_OK;
}

MunitResult test_timeout(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    KissState kiss;
    kiss_init(&kiss, 0);
    kiss_feed(&kiss, FEND, 0);
    kiss_feed(&kiss, KISS_DATA, 1);
    kiss_feed(&kiss, 'H', 2);
    kiss_feed(&kiss, 'e', 3);
    kiss_feed(&kiss, 'l', 4);
    kiss_feed(&kiss, 'l', 5);
    kiss_feed(&kiss, 'o', 6);

    assert_false(kiss.haveFrame);
    kiss_feed(&kiss, FEND, 3007);
    assert_false(kiss.haveFrame);
    
    return MUNIT_OK;
}


MunitResult test_no_timeout(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture); KissState kiss;
    
    kiss_init(&kiss, 0);
    kiss_feed(&kiss, FEND, 0);
    kiss_feed(&kiss, KISS_DATA, 1);
    kiss_feed(&kiss, 'H', 2);
    kiss_feed(&kiss, 'e', 3);
    kiss_feed(&kiss, 'l', 4);
    kiss_feed(&kiss, 'l', 5);
    kiss_feed(&kiss, 'o', 6);

    assert_false(kiss.haveFrame);
    kiss_feed(&kiss, FEND, 3006);
    assert_true(kiss.haveFrame);

    return MUNIT_OK;
}



MunitTest tests[] = {
  {
    "/test_sample_frame", /* name */
    test_sample_frame, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_escape", /* name */
    test_escape, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_hdlc", /* name */
    test_hdlc, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_encode", /* name */
    test_encode, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_no_timeout", /* name */
    test_no_timeout, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_timeout", /* name */
    test_timeout, /* test */
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
  "/kiss-tests", /* name */
  tests, /* tests */
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE /* options */
};

int main (int argc, char *const * argv) {
  return munit_suite_main(&suite, NULL, argc, argv);
}
