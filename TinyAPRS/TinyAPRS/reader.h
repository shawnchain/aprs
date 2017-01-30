/*
 * \file reader.h
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

#ifndef READER_H_
#define READER_H_

#include <stdio.h>
#include <io/kfile.h>
#include <drv/ser.h>

//Reader parameters
#define CFG_READER_READ_TIMEOUT 0

typedef void (*ReaderCallback)(char* line, uint8_t len);

typedef struct SerialReader{
	Serial *ser;
	uint8_t* buf;
	uint16_t bufLen; // The total buffer length;
	uint8_t readLen; // Counter for counting length of data from serial;
	uint8_t* data;
	uint16_t dataLen;

#if CFG_READER_READ_TIMEOUT > 0
	ticks_t lastReadTick;
#endif
}SerialReader;


void serialreader_init(SerialReader *reader, Serial *ser);

/*
 * purge all receive data
 */
void serialreader_reset(SerialReader *reader);

/*
 * read a line from the underlying serial
 * returns:
 *   n  bytes
 *   0  no data
 *  -1  error
 */
int serialreader_readline(SerialReader *reader);

#endif /* READER_H_ */
