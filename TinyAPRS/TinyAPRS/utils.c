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

#include "utils.h"
#include <stdio.h>
#include <string.h>

#include <cfg/compiler.h>
#include <ctype.h>

#include <avr/wdt.h>
#include <net/ax25.h>
#include <cpu/pgm.h>
#if CPU_AVR
#include <avr/pgmspace.h>

#endif

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




// Free ram test
uint16_t freeRam (void) {
  extern int __heap_start, *__brkval;
  uint8_t v;
  uint16_t vaddr = (uint16_t)(&v);
  return (uint16_t) (vaddr - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}

/*
 * AX25Call object to String
 */
uint8_t ax25call_to_string(struct AX25Call *call, char* buf){
	uint8_t i = 0;
	for(;i < 6;i++){
		char c = call->call[i];
		if(c == 0)
			break;
		buf[i] = c;
	}
	uint8_t ssid = call->ssid & 0x7f;
	if(ssid > 0){
		buf[i++] = '-';
		if(ssid < 10){
			buf[i++] = ssid + '0';
		}else{
			uint8_t s = (ssid / 10);
			buf[i++] = s + '0';
			buf[i++] = (ssid - (s * 10)) + '0';
		}
	}
	if(call->ssid & 0x80){
		buf[i++] = '*'; // repeated
	}
	buf[i++] = 0;
	return i;
}


/*
 * Call String to AX25Call object
 */
void ax25call_from_string(struct AX25Call *call, char* buf){
	memset(call,0,sizeof(AX25Call));
	uint8_t len = strlen(buf);
	uint8_t i = 0;
	uint8_t state = 0;
	for(; i < len && buf[i] != 0;i++){
		char c = buf[i];
		if(state == 0){
			// call
			if(c == '-'){
				state = 1;
				continue;
			}else{
				if(i >= 6){
					// invalid call name
					continue;
				}
				call->call[i] = toupper(c);
			}
		}else{
			// ssid
			if(c >='0' && c <='9')
				call->ssid = call->ssid * 10 + (c - '0');
		}
	}

}
