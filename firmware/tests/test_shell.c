#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#include "shell.h"


MunitResult test_lf(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    ShellState state;
    shell_init(&state);
    shell_feed(&state, 'H');
    shell_feed(&state, 'E');
    shell_feed(&state, 'L');
    assert_false(shell_feed(&state, 'P'));
    assert_true(shell_feed(&state, '\n'));
    assert_true(state.found_cmd == true);
    munit_assert_memory_equal(state.cmd_pos, state.cmd_buffer, "HELP");
    shell_complete(&state);
    assert_int8(state.cmd, ==, CMD_HELP);
    return MUNIT_OK;
}


MunitResult test_crlf(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);
    
    ShellState state;
    shell_init(&state);
    shell_feed(&state, 'H');
    shell_feed(&state, 'E');
    shell_feed(&state, 'L');
    shell_feed(&state, 'P');
    shell_feed(&state, '\r');
    shell_feed(&state, '\n');
    assert_true(state.found_cmd);
    munit_assert_memory_equal(state.cmd_pos, state.cmd_buffer, "HELP");
    shell_complete(&state);
    assert_int8(state.cmd, ==, CMD_HELP);
    return MUNIT_OK;
}

ShellState parse_line(char * line) {
    ShellState state;
    shell_init(&state);
    for (uint8_t i=0; i<strlen(line); i++) {
        assert_false(shell_feed(&state, line[i]));
    }
    assert_true(shell_feed(&state, '\n'));
    shell_complete(&state);
    return state;
}

MunitResult test_parser(const MunitParameter params[], void* user_data_or_fixture) {
    UNUSED(params);
    UNUSED(user_data_or_fixture);

    assert_int8(parse_line("HELP").cmd, ==, CMD_HELP);
    assert_int8(parse_line("?").cmd, ==, CMD_HELP);
    assert_int8(parse_line("CONFIG").cmd, ==, CMD_CONFIG);
    assert_int8(parse_line("C").cmd, ==, CMD_CONFIG);
    assert_int8(parse_line("SAVE").cmd, ==, CMD_SAVE);
    assert_int8(parse_line("S").cmd, ==, CMD_SAVE);
    return MUNIT_OK;
}


MunitTest tests[] = {
  {
    "/test_lf", /* name */
    test_lf, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_crlf", /* name */
    test_crlf, /* test */
    NULL, /* setup */
    NULL, /* tear_down */
    MUNIT_TEST_OPTION_NONE, /* options */
    NULL /* parameters */
  },
  {
    "/test_parser", /* name */
    test_parser, /* test */
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
  "/shell-tests", /* name */
  tests, /* tests */
  NULL, /* suites */
  1, /* iterations */
  MUNIT_SUITE_OPTION_NONE /* options */
};

int main (int argc, char *const * argv) {
  return munit_suite_main(&suite, NULL, argc, argv);
}
