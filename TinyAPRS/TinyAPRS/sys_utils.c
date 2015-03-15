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

#include <avr/wdt.h>

//void soft_reboot() {
//	wdt_disable();
//	wdt_enable(WDTO_2S);
//	while (1) {
//	}
//}


#if SOFT_RESET_ENABLED
// need to disable watch dog after reset on XMega
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();
    return;
}
#endif
