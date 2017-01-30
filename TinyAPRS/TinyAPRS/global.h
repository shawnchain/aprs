/*
 * \file global.h
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
 * \date 2015-3-30
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

struct Serial;
extern struct Serial g_serial;

struct SerialReader;
extern struct SerialReader g_serialreader;

struct AX25Ctx;
extern struct AX25Ctx g_ax25;

struct Afsk;
extern struct Afsk g_afsk;

struct GPS;
extern struct GPS g_gps;


#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

#define SER_FAST_MODE 1

#if SER_FAST_MODE
#define SER_DEFAULT_BAUD_RATE SER_BAUD_RATE_115200
#define SER_GPS_BAUD_RATE SER_BAUD_RATE_9600
#else
#define SER_DEFAULT_BAUD_RATE SER_BAUD_RATE_9600
#define SER_GPS_BAUD_RATE SER_BAUD_RATE_9600
#endif

#endif /* GLOBAL_H_ */
