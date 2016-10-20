#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "cfg/cfg_console.h"
#include <avr/pgmspace.h>
#include <drv/ser.h>


typedef bool (*PFUN_CMD_HANDLER)(Serial *ser, char* value, size_t valueLen);

/*
 * Initialize the console
 */
void console_init(void);

/*
 * Add command to console
 */
void console_add_command(PGM_P cmd, PFUN_CMD_HANDLER handler);

/*
 * Parse a command string, the corresponding cmd_xxx function will be called if it matches.
 */
void console_parse_command(char* command, size_t commandLen);

/*
 *
 */
void console_poll(void);
#endif
