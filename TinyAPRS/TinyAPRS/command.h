/*
 * \file command.h
 * <!--
 * This file is part of TinyAPRS.
 * Released under GPL License
 *
 * Copyright 2015 Shawn Chain (shawn.chain@gmail.com)
 *
 * -->
 *
 * \brief 
 *
 * \author shawn
 * \date 2015-2-19
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <drv/ser.h>
#include <avr/io.h>
#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>

void console_init(Serial *ser);

void console_poll(void);

typedef bool (*PFUN_CMD_HANDLER)(Serial *ser, char* value, size_t valueLen);

void console_add_command(PGM_P cmd, PFUN_CMD_HANDLER handler);

#endif /* COMMAND_H_ */
