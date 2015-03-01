#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <drv/ser.h>
#include <avr/io.h>
#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>

#include "cfg/cfg_console.h"

void console_init(Serial *ser);

void console_poll(void);

void console_parse(int c);

typedef bool (*PFUN_CMD_HANDLER)(Serial *ser, char* value, size_t valueLen);

void console_add_command(PGM_P cmd, PFUN_CMD_HANDLER handler);

#endif
