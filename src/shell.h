#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CMD_UNKNOWN,
    CMD_HELP,
    CMD_CONFIG,
    CMD_SAVE
} ShellCmd;

typedef struct {  
    ShellCmd cmd;
    char cmd_buffer[10];  
    uint8_t cmd_pos;
    bool found_cmd;

    char buffer[32];  // Bytes read after the command code and a space
    uint8_t pos;
} ShellState;

void shell_init(ShellState * shell);

bool shell_feed(ShellState * shell, int byte);

void shell_complete(ShellState * shell);

#endif
