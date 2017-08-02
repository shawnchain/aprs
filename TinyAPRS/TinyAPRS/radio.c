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

SoftSerial *radioPort = NULL;
static SoftSerial softSer;

//431/0400, 144/3900
void radio_init(uint32_t freq){
	radioPort = &softSer;
	softser_init(radioPort, CFG_RADIO_RX_PIN,CFG_RADIO_TX_PIN);
	softser_start(radioPort,CFG_RADIO_PORT_BAUDRATE /*9600 by default*/);

	// split the frequency
	uint16_t freqH, freqL;
	freqH = freq / 10000;
	freqL = freq - (freqH * 10000);

	char buf[64];
	// Set freq, sq=0,high_power=true "AT+DMOSETGROUP=GBW,TFV,RFV,RXCXCSS,SQ,TXCXCSS,FLA"
	//AT+DMOSETGROUP=0,450.0250,450.0250,1,2,1,1
	sprintf_P(buf,PSTR("\r\nAT+DMOSETGROUP=1,%d.%04d,%d.%04d,0,0,0,0\r\n"),freqH,freqL,freqH,freqL);
	//sprintf_P(buf,PSTR("AT+DMOSETGROUP=1,431.0400,431.0400,0,0,0,0\r\n"));
	hw_softser_print(radioPort, buf);
	timer_delay(300);

	// Disable auto power saving
	sprintf_P(buf,PSTR("AT+DMOAUTOPOWCONTR=1\r\n"));
	hw_softser_print(radioPort, buf);
	timer_delay(300);

	// Set mic = 3 "AT+DMOSETMIC=MICLVL, SCRAMLVL,TOT"
	sprintf_P(buf,PSTR("AT+DMOSETMIC=3,0,0\r\n"));
	hw_softser_print(radioPort, buf);
	timer_delay(300);

	// Set volume = 5 "AT+DMOSETVOLUME=5"
	sprintf_P(buf,PSTR("AT+DMOSETVOLUME=5\r\n"));
	hw_softser_print(radioPort, buf);
	timer_delay(300);
}
