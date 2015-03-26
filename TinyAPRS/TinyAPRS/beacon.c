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

static AX25Ctx *ax25Ctx;

static beacon_exit_callback_t exitCallback = 0;

static ticks_t lastSend;
static int8_t beaconSendCount = 0;

/*
 * Initialize the beacon module
 */
void beacon_init(AX25Ctx *ctx, beacon_exit_callback_t exitcb){
	ax25Ctx = ctx;
	exitCallback = exitcb;
}

void beacon_set_repeats(int8_t repeats){
	beaconSendCount = repeats;
}

void beacon_poll(void){
	// Broadcast beacon message n times in every 2s
	if(beaconSendCount > 0){
		if(timer_clock() - lastSend > ms_to_ticks(2000L)){
			beacon_send();
			beaconSendCount--;
			lastSend = timer_clock();

			// job finished, notify caller
			if(beaconSendCount == 0 && exitCallback){
				exitCallback();
			}
		}
	}else if (beaconSendCount < 0){
		// always send
		beacon_send();
	}
}

void beacon_send(void){
	AX25Call path[] = AX25_PATH(/*dst*/AX25_CALL("APTI01"/*TinyAPRS id, the destination field*/, 0) , /*src*/AX25_CALL("N0CALL", 0), AX25_CALL("wide1", 1)/*, AX25_CALL("wide2", 2)*/);
	// update callsign and ssid from settings
	uint8_t len = 6;
	settings_get(SETTINGS_MY_CALL,&(path[1].call),&len);
	settings_get(SETTINGS_MY_SSID,&(path[1].ssid),&len);

	// digi-path
	// payload
	char payload[128];
	uint8_t payloadLen = settings_get_raw_packet(payload,128);
	if(payloadLen == 0){
		payloadLen = snprintf_P(payload,127,PSTR("!3014.00N/12009.00E>TinyAPRS Rocks!")); // the default one for test purpose
	}
	if(payloadLen > 0)
		ax25_sendVia(ax25Ctx, path, countof(path), payload, payloadLen);
	// TODO construct the beacon payload from settings
  	//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087Rolling! 3.6V 1011.0pa" // 六和塔
	//#define APRS_TEST_MSG "!3014.00N/12009.00E>000/000/A=000087Rolling!"   // Hangzhou
	//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087TinyAPRS Rocks!"
	//#define APRS_TEST_MSG ">Test Tiny APRS "
	//ax25_sendVia(ax25Ctx, path, countof(path), APRS_TEST_MSG, sizeof(APRS_TEST_MSG));
}


