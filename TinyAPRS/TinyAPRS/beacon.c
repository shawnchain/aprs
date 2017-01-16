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


static volatile mtime_t lastSendTimeSeconds = 0; // in seconds

/*
 * Initialize the beacon module
 */
void beacon_init(beacon_exit_callback_t exitcb){
	(void)exitcb;
	lastSendTimeSeconds = 0;
}

static void _send_fixed_text(void){
	char payload[128];
	uint8_t payloadLen = settings_get_beacon_text(payload,127);
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
	uint16_t beaconSendInterval = g_settings.beacon.interval;
	if(beaconSendInterval == 0){
		//it's disabled;
		return;
	}
	volatile mtime_t currentTimestamp = timer_clock_seconds();
	if(lastSendTimeSeconds == 0){
		// just started
		lastSendTimeSeconds = timer_clock_seconds();
		if(lastSendTimeSeconds > 0){ // possible be zero in less than 1 seconds
			_send_fixed_text();
		}
		return;
	}

	if(currentTimestamp - lastSendTimeSeconds > beaconSendInterval){
		_send_fixed_text();
		lastSendTimeSeconds = timer_clock_seconds();
	}
}

void beacon_send(char* payload, uint8_t payloadLen){
	CallData calldata;
	settings_get_call_data(&calldata);

	// if the digi path is set, just increase that
	uint8_t pathCount = 2;
	if(calldata.path1.call[0] > 0){
		pathCount++;
	}
	if(calldata.path2.call[0] > 0){
		pathCount++;
	}

	ax25_sendVia(&g_ax25, (AX25Call*)&calldata, pathCount, payload, payloadLen);

#if CFG_BEACON_DEBUG
	kfile_putc('.',&(g_serial.fd));
#endif

}

//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087Rolling! 3.6V 1011.0pa" // 六和塔
//#define APRS_TEST_MSG "!3014.00N/12009.00E>000/000/A=000087Rolling!"   // Hangzhou
//#define APRS_TEST_MSG "!3011.54N/12007.35E>000/000/A=000087TinyAPRS Rocks!"
//#define APRS_TEST_MSG ">Test Tiny APRS "
//#define APRS_DEFAULT_TEXT "!3014.00N/12009.00E>TinyAPRS Rocks!"
