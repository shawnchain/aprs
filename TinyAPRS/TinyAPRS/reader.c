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

void serialreader_init(SerialReader *reader, Serial *ser, uint8_t *buf, uint16_t bufLen){
	memset(reader, 0, sizeof(SerialReader));
	reader->ser = ser;
	reader->buf = buf;
	reader->bufLen = bufLen;
}

int serialreader_readline(SerialReader *reader){
	uint8_t *readBuffer = reader->buf;

#if CFG_READER_READ_TIMEOUT > 0
	// discard the buffer if read timeout;
	if((reader->readLen > 0) && (timer_clock() - reader->lastReadTick > ms_to_ticks(CFG_READER_READ_TIMEOUT)) ){
		//LOG_INFO("Console - Timeout\n");
		reader->readLen = 0;
	}
#endif

	//Check the "cfg_ser.h" file for CONFIG_SER_RXTIMEOUT = 0
	int c = ser_getchar(reader->ser);
	if(c == EOF)  return 0;

	if(c != '\r' && c != '\n'){
		readBuffer[reader->readLen++] = c;
		// check if buffer is full
		if(reader->readLen < reader->bufLen - 1){
	#if CFG_READER_READ_TIMEOUT > 0
			reader->lastReadTick = timer_clock();
	#endif
			return 0;
		}
	}

	// if run here, we got \r \n or buffer is full
	if(reader->readLen > 0){
		readBuffer[reader->readLen] = 0; // complete the buffered string
		reader->data = reader->buf;
		reader->dataLen = reader->readLen;
		reader->readLen = 0; // reset the counter!
		return reader->dataLen;
	}

	return 0;
}
