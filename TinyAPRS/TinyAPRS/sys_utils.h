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


// Memory test
#define FREE_RAM_TEST 0
uint16_t freeRam (void);

// Serial printf helper macros
#define SERIAL_PRINTF(pSER,...) kfile_printf(&(pSER->fd),__VA_ARGS__)


#endif /* SYS_UTILS_H_ */
