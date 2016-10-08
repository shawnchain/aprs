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
#include <math.h>
#include <stdlib.h>
#include <cfg/macros.h>


static mtime_t lastSendTimeSeconds = 0; // in seconds

/*
 * Initialize the beacon module
 */
void beacon_init(beacon_exit_callback_t exitcb){
	(void)exitcb;
}


INLINE void _send_fixed_text(void){
	char payload[80 + 1];
	uint8_t payloadLen = settings_get_beacon_text(payload,80);
	if(payloadLen > 0){
		beacon_send(payload,payloadLen);
	}
}

#if CFG_BEACON_TEST
void beacon_send_test_message_immediate(uint8_t repeats, const char* text){
	(void)text;
	while(repeats > 0){
		_send_fixed_text();
		if(--repeats > 0){
			timer_delay(2000); // delay 2s
		}
	}
}
#endif

void beacon_broadcast_poll(void){
	uint16_t beaconSendInterval = g_settings.beacon_interval;
	if(beaconSendInterval == 0){
		//it's disabled;
		return;
	}
	mtime_t currentTimestamp = timer_clock_seconds();
	if(lastSendTimeSeconds == 0 ||  currentTimestamp - lastSendTimeSeconds > beaconSendInterval){
		_send_fixed_text();
		lastSendTimeSeconds = timer_clock_seconds();
	}
}

void beacon_send(char* payload, uint8_t payloadLen){
	AX25Call path[4];// dest,src,rpt1,rpt2

	// dest call: APTI01
	settings_get_call(SETTINGS_DEST_CALL,&(path[0]));
	// src call: MyCALL-SSID
	settings_get_call(SETTINGS_MY_CALL,&(path[1]));
	// path1: WIDE1-1
	settings_get_call(SETTINGS_PATH1_CALL,&(path[2]));
	// path2: WIDE2-2
	settings_get_call(SETTINGS_PATH2_CALL,&(path[3]));

	// if the digi path is set, just increase that
	uint8_t pathCount = 2;
	if(path[2].call[0] > 0){
		pathCount++;
	}
	if(path[3].call[0] > 0){
		pathCount++;
	}

	ax25_sendVia(&g_ax25, path, pathCount, payload, payloadLen);

#if CFG_BEACON_DEBUG
	kfile_putc('.',&(g_serial.fd));
#endif

}

//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087Rolling! 3.6V 1011.0pa" // 六和塔
//#define APRS_TEST_MSG "!3014.00N/12009.00E>000/000/A=000087Rolling!"   // Hangzhou
//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087TinyAPRS Rocks!"
//#define APRS_TEST_MSG ">Test Tiny APRS "
//#define APRS_DEFAULT_TEXT "!3014.00N/12009.00E>TinyAPRS Rocks!"
