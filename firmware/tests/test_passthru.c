#include <stdio.h>
#include <stdlib.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "passthru/passthru.h"

static uint8_t LOREM_IPSUM_1[124] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static uint8_t LOREM_IPSUM_2[108] = "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";

uint8_t uart_rx_buffer[4096];
uint32_t uart_rx_write_pos = 0;
uint32_t uart_rx_read_pos = 0;

void fill_uart_rx(void * buffer, uint32_t len) {
    memcpy(&uart_rx_buffer[uart_rx_write_pos], buffer, len);
    uart_rx_write_pos += len;
}

void reset_uart_rx(void) {
    uart_rx_write_pos = 0;
    uart_rx_read_pos = 0;
}

bool uart_has_data(void) {
    return uart_rx_write_pos > uart_rx_read_pos;
}

uint32_t read_uart(void) {
    uint32_t byte = uart_rx_buffer[uart_rx_read_pos++];
    return byte;
}

uint32_t usb_write_full(void) {
    return 0;
}

uint32_t usb_write_empty(void) {
    return 4096;
}

uint32_t usb_write_32(void) {
    return 32;
}

void usb_write_flush(void) {
    return;
}

uint8_t usb_write_capture[4096];

uint32_t usb_write_capture_all(void * buffer, uint32_t len) {
    memcpy(usb_write_capture, buffer, len);
    return len;
}

MunitResult test_usb_write(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    passthru_init(uart_has_data, read_uart);

    uint32_t usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);
    
    fill_uart_rx(LOREM_IPSUM_1, 124);
    passthru_uart_irq();

    usb_write_capture_len = passthru_usb_write(usb_write_full, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(32, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(32, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(32, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(28, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);

    reset_uart_rx();
    fill_uart_rx(LOREM_IPSUM_1, 124);
    passthru_uart_irq();

    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(124, ==, usb_write_capture_len);

    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);

    return MUNIT_OK;
}

MunitResult test_uart_to_usb(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    passthru_init(uart_has_data, read_uart);

    // Copy text into our mock UART
    fill_uart_rx(LOREM_IPSUM_1, 124);

    // Fire the IRQ to drain the UART
    passthru_uart_irq();

    // Copy more data into UART, but don't read it
    fill_uart_rx(LOREM_IPSUM_2, 108);

    // Run the USB write loop which should copy these bytes
    uint32_t usb_write_capture_len = passthru_usb_write(usb_write_32, usb_write_capture_all, usb_write_flush);
    assert_int32(32, ==, usb_write_capture_len);
    assert_memory_equal(32, usb_write_capture, LOREM_IPSUM_1);

    // USB write again, get rest of buffer
    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(92, ==, usb_write_capture_len);
    assert_memory_equal(92, usb_write_capture, &LOREM_IPSUM_1[32]);

    // USB write again, nothing
    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);

    // Drain remaining bytes
    passthru_uart_irq();

    // See remaining bytes read by USB
    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(108, ==, usb_write_capture_len);
    assert_memory_equal(108, usb_write_capture, LOREM_IPSUM_2);

    usb_write_capture_len = passthru_usb_write(usb_write_empty, usb_write_capture_all, usb_write_flush);
    assert_int32(0, ==, usb_write_capture_len);

    return MUNIT_OK;
}


uint8_t usb_rx_buffer[4096];
uint32_t usb_rx_write_pos = 0;
uint32_t usb_rx_read_pos = 0;

void fill_usb_rx(void * buffer, uint32_t len) {
    memcpy(&usb_rx_buffer[usb_rx_write_pos], buffer, len);
    usb_rx_write_pos += len;
}

void reset_usb_rx(void) {
    usb_rx_write_pos = 0;
    usb_rx_read_pos = 0;
}

uint32_t usb_rx_size(void) {
    return usb_rx_write_pos - usb_rx_read_pos;
}

uint32_t read_usb_into_buffer(void * buffer, uint32_t len) {
    uint32_t available = usb_rx_write_pos - usb_rx_read_pos;
    if (len > available) {
        len = available;
    }
    memcpy(buffer, &usb_rx_buffer[usb_rx_read_pos], len);
    usb_rx_read_pos += len;
    return len;
}

uint8_t uart_write_capture[4096];

uint32_t uart_write_capture_all(void * buffer, uint32_t len) {
    memcpy(uart_write_capture, buffer, len);
    return len;
}


MunitResult test_usb_to_uart(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    fill_usb_rx(LOREM_IPSUM_1, 124);
    uint32_t usb_read = passthru_usb_read(usb_rx_size, read_usb_into_buffer);
    assert_int32(usb_read, ==, 124);

    uint32_t uart_written = passthru_uart_loop(uart_write_capture_all);
    assert_int32(uart_written, ==, 124);
    assert_memory_equal(124, uart_write_capture, LOREM_IPSUM_1);

    return MUNIT_OK;
}


MunitTest tests[] = {
  {
    "/test_usb_write", /* name */
    test_usb_write, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_uart_to_usb", /* name */
    test_uart_to_usb, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_usb_to_uart", /* name */
    test_usb_to_uart, /* test */
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
  "/passthru-tests", /* name */
  tests, /* tests */
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE /* options */
};

int main (int argc, char *const * argv) {
  return munit_suite_main(&suite, NULL, argc, argv);
}
