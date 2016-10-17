/**
 * \file utils.h
 * <!--
 * This file is part of TinyAPRS.
 * Released under GPL License
 *
 * Copyright 2015 Shawn Chain (shawn.chain@gmail.com)
 *
 * -->
 *
 * \brief Util code
 *
 * \author Shawn Chain <shawn.chain@gmail.com>
 * \date 2015-2-11
 */

#ifndef SYS_UTILS_H_
#define SYS_UTILS_H_

#include <stdlib.h>

#include <io/kfile.h>

uint16_t freeRam (void);

void soft_reset(void);

/////////////////////////////////////////////////////////////////////////
// Serial print/printf macros
/////////////////////////////////////////////////////////////////////////
/*
 * Macro that simplifies printing strings to serial port
 */
#define SERIAL_PRINTF(pSER,...) kfile_printf(&(pSER->fd),__VA_ARGS__)

/**
 * Write a PROGMEM string to kfile \a fd.
 * \return 0 if OK, EOF in case of error.
 */
int kfile_print_P(struct KFile *fd, const char *s);

/*
 * Macro that reads format string from program memory and print out to serial port
 * support variable arguments
 */
#define SERIAL_PRINTF_P(pSER,pSTR,...) kfile_printf_P((&(pSER->fd)),pSTR,__VA_ARGS__);

/*
 * Macro that reads string from program memory and print out to serial port
 */
#define SERIAL_PRINT_P(pSER,pSTR) kfile_printf_P((&(pSER->fd)),pSTR);

struct AX25Call;
uint8_t ax25call_to_string(struct AX25Call *call, char* buf);
void ax25call_from_string(struct AX25Call *call, char* buf);

/*
 * Get timer clock count in seconds
 */
#define timer_clock_seconds(void) ticks_to_ms(timer_clock()) / 1000

uint8_t calc_crc(uint8_t *data, uint16_t size);

#endif /* SYS_UTILS_H_ */
