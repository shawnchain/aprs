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
#include <cfg/cfg_ax25.h>

#if MOD_KISS
#define READER_BUF_LEN CONFIG_AX25_FRAME_BUF_LEN //shared buffer is 330 bytes for KISS module reading received AX25 frame.
#else
#define READER_BUF_LEN 128
#endif
static uint8_t read_buffer[READER_BUF_LEN];

void serialreader_init(SerialReader *reader, Serial *ser){
	memset(reader, 0, sizeof(SerialReader));
	reader->ser = ser;
	reader->buf = read_buffer;
	reader->bufLen = READER_BUF_LEN;
}

void serialreader_reset(SerialReader *reader){
	reader->dataLen = 0;
	reader->readLen = 0;
	kfile_clearerr((struct KFile*)reader->ser);
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
