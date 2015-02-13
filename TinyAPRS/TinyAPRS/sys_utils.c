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

#include "sys_utils.h"



// Free ram test
uint16_t freeRam (void) {
  extern int __heap_start, *__brkval;
  uint8_t v;
  return (uint16_t) (&v - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}

// Software reset
//
//inline void soft_reset(void){
//	do
//	{
//	    wdt_enable(WDTO_15MS);
//	    for(;;)
//	    {
//	    }
//	} while(0);
//}
//

// need to disable watch dog after reset on XMega
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}
