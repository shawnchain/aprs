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

//Reader parameters
#define READ_TIMEOUT 0

typedef void (*ReaderCallback)(char* line, uint8_t len);

typedef struct Reader{
//	struct KFile *fd;
	uint8_t* buf;
	uint16_t bufLen;
	uint8_t readLen; // Counter for counting length of data from serial
	ReaderCallback callback;
}Reader;

void reader_init(Reader *pReader,uint8_t *buf, uint16_t bufLen, struct KFile *fd,ReaderCallback callback);

void reader_poll(Reader *pReader);

#endif /* READER_H_ */
