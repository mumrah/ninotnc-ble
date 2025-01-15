#include <string.h>

#include "shell.h"

const uint8_t num_commands = 4;

const char * shell_commands[] = {
  "", "HELP", "CONFIG", "SAVE"
};

void shell_init(ShellState * shell) {
    shell->cmd_pos = 0;
    shell->found_cmd = false;
    shell->pos = 0;
    shell->cmd = CMD_UNKNOWN;
    memset(shell->buffer, 0, 32);   
    memset(shell->cmd_buffer, 0, 10);   
}

bool shell_feed(ShellState * shell, int byte) {
    if (!shell->found_cmd) {
        if (shell->cmd_pos == 0 && byte == ' ') {
            // Consume leading spaces
            return false;
        }
        if (byte == '\n' && (shell->cmd_pos > 0) && shell->cmd_buffer[shell->cmd_pos-1] == '\r') {
            // Space found after command
            shell->cmd_pos--;
            shell->found_cmd = true;
            return true;
        }
        if (shell->cmd_pos > 0 && byte == '\n') {
            // Space found after command
            shell->found_cmd = true;
            return true;
        }
        shell->cmd_buffer[shell->cmd_pos++] = byte;
        return false;
    } else {
        if (byte == ' ' && shell->pos == 0) {
            // consume spaces between command and data
            return false;
        } else if (byte == '\n' && (shell->pos > 0) && shell->buffer[shell->pos-1] == '\r') {
            // If we read a \n and it's preceeded by a \r, decrease the buffer size by 1 and
            // dispatch the event
            shell->pos--;
            return true;
        } else if (byte == '\n') { 
            return true;
        } else {
            // Just consume into the buffer, protect against overflow
            if (shell->pos < 31) { 
                shell->pos++;
            }
            shell->buffer[shell->pos - 1] = byte;
            return false;
        }
    }
}

void shell_complete(ShellState * shell) {
    shell->cmd = CMD_UNKNOWN;
    if (shell->cmd_pos == 1) {
        switch (shell->cmd_buffer[0]) {
            case 'C':
                shell->cmd = CMD_CONFIG;
                break;
            case 'S':
                shell->cmd = CMD_SAVE;
                break;
            case '?':
            default:
                shell->cmd = CMD_HELP;
                break;
        }
    }

    for (uint8_t i=1; i<num_commands; i++) {
        if (memcmp(shell->cmd_buffer, shell_commands[i], shell->cmd_pos) == 0) {
            shell->cmd = i; // Index of shell_commands array must match enum
        }
    }
}
