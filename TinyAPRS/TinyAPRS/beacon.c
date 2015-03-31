/*
 * \file beacon.c
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
 * \date 2015-2-13
 */

#include <net/afsk.h>
#include <net/ax25.h>
#include <drv/timer.h>

#include "beacon.h"
#include "settings.h"
#include "global.h"
#include "gps.h"
#include "utils.h"
#include <drv/ser.h>
#include <drv/timer.h>

static beacon_exit_callback_t exitCallback = 0;

static ticks_t lastSend;
static int8_t beaconSendCount = 0;

/*
 * Initialize the beacon module
 */
void beacon_init(beacon_exit_callback_t exitcb){
	exitCallback = exitcb;
}

void beacon_set_repeats(int8_t repeats){
	beaconSendCount = repeats;
}

void beacon_poll(void){
	// Broadcast beacon message n times in every 2s
	if(beaconSendCount > 0){
		if(timer_clock() - lastSend > ms_to_ticks(2000L)){
			beacon_send_fixed();
			beaconSendCount--;
			lastSend = timer_clock();

			// job finished, notify caller
			if(beaconSendCount == 0 && exitCallback){
				exitCallback();
			}
		}
	}else if (beaconSendCount < 0){
		// always send
		beacon_send_fixed();
	}
}

static void _beacon_send(char* payload, uint8_t payloadLen){
	AX25Call path[3];
	memset(path,0,sizeof(AX25Call) * 3);

	// dest call: APTI01
	uint8_t len = 6;
	snprintf_P((char*)&(path[0].call),6,PSTR("APTI01"));

	// src call: MyCALL-SSID
	settings_get(SETTINGS_MY_CALL,&(path[1].call),&len);
	len = 1;
	settings_get(SETTINGS_MY_SSID,&(path[1].ssid),&len);
	// path1: WIDE1-1
	len = 6;
	settings_get(SETTINGS_PATH1_CALL,&(path[2].call),&len);
	len = 1;
	settings_get(SETTINGS_PATH1_SSID,&(path[2].ssid),&len);

	//TODO supports WIDE2-2?
	ax25_sendVia(&g_ax25, path, countof(path), payload, payloadLen);

#if CFG_BEACON_DEBUG
	kfile_putc('.',&(g_serial.fd));
#endif

}

//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087Rolling! 3.6V 1011.0pa" // 六和塔
//#define APRS_TEST_MSG "!3014.00N/12009.00E>000/000/A=000087Rolling!"   // Hangzhou
//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087TinyAPRS Rocks!"
//#define APRS_TEST_MSG ">Test Tiny APRS "
#define APRS_DEFAULT_TEXT "!3014.00N/12009.00E>TinyAPRS Rocks!"

void beacon_send_fixed(void){
	// payload
	char payload[80];
	uint8_t payloadLen = settings_get_raw_packet(payload,80);
	if(payloadLen == 0){
		payloadLen = snprintf_P(payload,127,PSTR(APRS_DEFAULT_TEXT)); // the default one for test purpose
	}
	if(payloadLen > 0){
		_beacon_send(payload,payloadLen);
	}
}

/*
 * smart beacon algorithm
 */
void beacon_update_location(struct GPS *gps){
	if(!gps->valid){
		return;
	}

	static ticks_t ts = 0;
	// first time will always trigger the send
	if(ts == 0 || timer_clock() - ts > ms_to_ticks(30000)){
		// payload
		char payload[48];
		char s1 = g_settings.symbol[0];
		if(s1 == 0) s1 = '/';
		char s2 = g_settings.symbol[1];
		if(s2 == 0) s2 = '>';
		uint8_t payloadLen = snprintf_P(payload,63,PSTR("!%.7s%c%c%.8s%c%cTinyAPRS Rocks"),
				gps->_term[GPRMC_TERM_LATITUDE],gps->_term[GPRMC_TERM_LATITUDE_NS][0],
				s1,
				gps->_term[GPRMC_TERM_LONGITUDE],gps->_term[GPRMC_TERM_LONGITUDE_WE][0],
				s2);

		_beacon_send(payload,payloadLen);
		//ax25_sendVia(&g_ax25, path, countof(path), payload, payloadLen);

#if 0	// DEBUG DUMP
		kfile_print((&(g_serial.fd)),payload);
		kfile_putc('\r', &(g_serial.fd));
		kfile_putc('\n', &(g_serial.fd));
#endif

		ts = timer_clock();
	}

#if 0
	Location location;
	gps_get_location(gps,&location);
	// TODO - send the location message
	char* lat = g_gps._term[GPRMC_TERM_LATITUDE];
	char* lon = g_gps._term[GPRMC_TERM_LONGITUDE];
	char* spd = g_gps._term[GPRMC_TERM_SPEED];
	SERIAL_PRINTF_P((&g_serial),PSTR("lat:%s, lon:%s, speed:%s\n"),lat,lon,spd);
#endif
}

