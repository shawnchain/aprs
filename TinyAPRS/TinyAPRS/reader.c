/*
 * \file reader.c
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
 * \date 2015-3-29
 */

#include "reader.h"
#include <string.h>
#include <cfg/compiler.h>
#include <drv/timer.h>
#include "global.h"
#include <drv/ser.h>


static Reader g_reader;


void reader_init(uint8_t *buf, uint16_t bufLen, ReaderCallback callback){
	memset(&g_reader, 0, sizeof(Reader));
	Reader *reader = &g_reader;
	reader->buf = buf;
	reader->bufLen = bufLen;
	reader->readLen = 0;
	reader->callback = callback;
}

void reader_poll(Serial *pSerial){
	//NOTE - make sure that CONFIG_SER_RXTIMEOUT = 0 in cfg_ser.h
	int c = ser_getchar(pSerial);
	//int c = ser_getchar_nowait(pSerial);
	if(c == EOF)  return;

	Reader *reader = &g_reader;
	uint8_t *readBuffer = reader->buf;
#if READ_TIMEOUT > 0
	static ticks_t lastReadTick = 0;
	if((reader->readLen > 0) && (timer_clock() - lastReadTick > ms_to_ticks(READ_TIMEOUT)) ){
		//LOG_INFO("Console - Timeout\n");
		reader->readLen = 0;
	}
#endif

	// read until met CR/LF/EOF or buffer is full
	if ((reader->readLen >= reader->bufLen) || (c == '\r') || (c == '\n') || (c == EOF) ) {
		if(reader->readLen > 0){
			readBuffer[reader->readLen] = 0; // complete the buffered string
			if(reader->callback){
				reader->callback((char*)readBuffer, reader->readLen);
			}
			reader->readLen = 0;
		}
	} else {
		// keep in buffer
		readBuffer[reader->readLen++] = c;
#if READ_TIMEOUT > 0
		lastReadTick = timer_clock();
#endif
	}
}
