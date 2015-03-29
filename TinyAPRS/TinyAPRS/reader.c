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

static uint8_t _buf[READ_BUF_SIZE + 1];

void reader_init(Reader *reader, struct KFile *fd, ReaderCallback callback){
	memset(reader, 0, sizeof(Reader));
	reader->fd = fd;
	reader->buf = _buf;
	reader->readLen = 0;
	reader->callback = callback;
}

void reader_poll(Reader *reader){

	int c = kfile_getc(reader->fd);
	if(c == EOF)  return;

	uint8_t *readBuffer = reader->buf;
#if READ_TIMEOUT > 0
	static ticks_t lastReadTick = 0;
	if((reader->readLen > 0) && (timer_clock() - lastReadTick > ms_to_ticks(READ_TIMEOUT)) ){
		//LOG_INFO("Console - Timeout\n");
		reader->readLen = 0;
	}
#endif

	// read until met CR/LF/EOF or buffer is full
	if ((reader->readLen >= READ_BUF_SIZE) || (c == '\r') || (c == '\n') || (c == EOF) ) {
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
