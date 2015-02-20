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
#include <avr/wdt.h>

// Software reset
 #define soft_reset()        \
 do                          \
 {                           \
     wdt_enable(WDTO_15MS);  \
     for(;;)                 \
     {                       \
     }                       \
 } while(0)


/////////////////////////////////////////////////////////////////////////
// Serial print/printf macros
/////////////////////////////////////////////////////////////////////////
/*
 * Macro that simplifies printing strings to serial port
 */
#define SERIAL_PRINTF(pSER,...) kfile_printf(&(pSER->fd),__VA_ARGS__)

/*
 * Macro that reads format string from program memory and print out to serial port
 * support variable arguments
 */
#define SERIAL_PRINTF_P(pSER,pSTR,...) 			\
{												\
	char __ser_printf_p_buf[64];								\
	strncpy_P(__ser_printf_p_buf,pSTR,63);						\
	kfile_printf((&(pSER->fd)),__ser_printf_p_buf,__VA_ARGS__);	\
}

/*
 * Macro that reads string from program memory and print out to serial port
 */
#define SERIAL_PRINT_P(pSER,pSTR) 			\
{												\
	char __ser_print_p_buf[64];								\
	strncpy_P(__ser_print_p_buf,pSTR,63);						\
	kfile_printf((&(pSER->fd)),__ser_print_p_buf);	\
}

#endif /* SYS_UTILS_H_ */
