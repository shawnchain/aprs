/*
 * \file radio.c
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
 * \date 2015-3-21
 */


#include "radio.h"
#include "hw/hw_softser.h"

#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <drv/timer.h>


static SoftSerial *pSer;
//431/0400, 144/3900
void radio_init(SoftSerial *p,uint16_t freqHigh,uint16_t freqLow){
	pSer = p;
	char buf[64];

	// Set frequency "AT+DMOSETGROUP=GBW,TFV,RFV,RXCXCSS,SQ,TXCXCSS,FLA"
	//AT+DMOSETGROUP=0,450.0250,450.0250,1,2,1,1
	sprintf_P(buf,PSTR("\r\nAT+DMOSETGROUP=1,%d.%04d,%d.%04d,0,0,0,0\r\n"),freqHigh,freqLow,freqHigh,freqLow);
	//sprintf_P(buf,PSTR("AT+DMOSETGROUP=1,431.0400,431.0400,0,0,0,0\r\n"));
	hw_softser_print(pSer, buf);
	timer_delay(300);

	// Set mic "AT+DMOSETMIC=MICLVL, SCRAMLVL,TOT"
	sprintf_P(buf,PSTR("AT+DMOSETMIC=3,0,0\r\n"));
	hw_softser_print(pSer, buf);
	timer_delay(300);

	// Set volume "AT+DMOSETVOLUME=5"
	sprintf_P(buf,PSTR("AT+DMOSETVOLUME=5\r\n"));
	hw_softser_print(pSer, buf);
	timer_delay(300);
}
