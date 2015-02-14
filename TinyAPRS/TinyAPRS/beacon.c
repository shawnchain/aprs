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


static bool beaconEnabled;
static ticks_t start;
static ticks_t test_start;
static AX25Ctx *ax25Ctx;

/*
 * Initialize the beacon module
 */
void beacon_init(AX25Ctx *ctx){
	start = timer_clock();
	beaconEnabled = false;
	ax25Ctx = ctx;
}

bool beacon_enabled(void){
	return beaconEnabled;
}

void beacon_set_enabled(bool flag){
	start = timer_clock();
	test_start = start;
	beaconEnabled = flag;
}

static uint8_t beaconTestCount = 0;
void beacon_send_test(uint8_t count){
	if(beaconTestCount == 0){
		beaconTestCount = count;
	}
}

void beacon_poll(void){
#if CONFIG_BEACON_ENABLED
	if(beaconEnabled){
		// Send out message every 5sec
		// TODO - configurable time interval
		// TODO - generate beacon message from settings
		if (timer_clock() - start > ms_to_ticks(5000L))
		{
			start = timer_clock();
			beacon_send();
		}
	}
#endif
	if(beaconTestCount > 0){
		if(timer_clock() - test_start > ms_to_ticks(2000L)){

			beacon_send();
			beaconTestCount--;
			test_start = timer_clock();
		}
	}
}

void beacon_send(void){
#if CONFIG_BEACON_ENABLED
	static AX25Call path[] = AX25_PATH(AX25_CALL("BR5AA", 0) /*dst*/, AX25_CALL("BG5HHP", 10)/*src*/, AX25_CALL("wide1", 1), AX25_CALL("wide2", 2));
	#define APRS_TEST_MSG "!3011.47N/12009.10E>000/000/A=000087Rolling! 3.6V 1011.0pa"
	//#define APRS_TEST_MSG ">Test Tiny APRS "

	ax25_sendVia(ax25Ctx, path, countof(path), APRS_TEST_MSG, sizeof(APRS_TEST_MSG));
#endif
}


