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

#define DEBUG_BEACON_PAYLOAD 0

static void _do_send(char* payload, uint8_t payloadLen);

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
		_do_send(payload,payloadLen);
	}
}

/*
 * Get timer clock count in seconds
 */
INLINE mtime_t timer_clock_seconds(void){
	return ticks_to_ms(timer_clock()) / 1000;
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

static void _do_send(char* payload, uint8_t payloadLen){
	AX25Call path[3];
	memset(path,0,sizeof(AX25Call) * 3);

	// dest call: APTI01
	uint8_t len = 6;
	memcpy_P((char*)&(path[0].call),PSTR("APTI01"),6);

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
//#define APRS_DEFAULT_TEXT "!3014.00N/12009.00E>TinyAPRS Rocks!"

#define SB_FAST_RATE 45		// 45 seconds

#if CFG_BEACON_SMART
//TODO - save in settings
#define SB_SLOW_RATE 120	// 3 minutes
#define SB_LOW_SPEED 5		// 5KM/h
#define SB_HI_SPEED 70		// 80KM/H
#define SB_TURN_TIME 15
#define SB_TURN_MIN 10
#define SB_TURN_SLOPE 240.f

static Location lastLocation = {
		.latitude=0.f,
		.longitude=0.f,
		.speedInKMH=0.f,
		.heading=0.f,
		.timestamp = 0
};

INLINE float _calc_heading(Location *l1, Location *l2){
	float d = abs(l1->heading - l2->heading) / 360;
	return (d <= 180.f)? d : 360 - d;
}

/*
 * returns the max speed since last location
 */
INLINE uint16_t _calc_speed_kmh(Location *l1, Location *l2){
	float dist = gps_distance_between(l1,l2,1); // distance in meters
	int16_t time_diff = abs(l1->timestamp - l2->timestamp); // in seconds, in one day
	float s = dist / time_diff * 3.6; // convert to kmh
	//kfile_printf(&g_serial.fd,"dist: %f, time_diff: %d, calculated spd:%f\r\n",dist,time_diff,s);
	float s2 = MAX(l1->speedInKMH, l2->speedInKMH);
	return lroundf( MAX(s, s2) );
}
#endif

static bool _fixed_interval_beacon_check(void){
	mtime_t rate = SB_FAST_RATE;
	if(lastSendTimeSeconds == 0){
		return true;
	}
	mtime_t currentTimeStamp = timer_clock_seconds();
	return (currentTimeStamp - lastSendTimeSeconds > (rate));
}

/*
 * smart beacon algorithm - http://www.hamhud.net/hh2/smartbeacon.html
 */
static bool _smart_beacon_check(Location *location){
#if CFG_BEACON_SMART
	if(lastSendTimeSeconds == 0 || lastLocation.timestamp == 0){
		return true;
	}

	// SMART HEADING CHECK
	// get the delta of time/speed/heading for current location vs last location
	int16_t secs_since_beacon = location->timestamp - lastLocation.timestamp; //[second]
	if(secs_since_beacon <= 0){
		//	that could happen when current and last spot spans one day, so drop that
		return false;
	}

	uint16_t speed_kmh = _calc_speed_kmh(location,&lastLocation);    //[meters per second]
	if(speed_kmh == 0){
		// we are stopped, drop
		return false;
	}

	uint16_t heading_change_since_beacon =lroundf(_calc_heading(location,&lastLocation)); //degress
	uint16_t turn_threshold = SB_TURN_MIN + lroundf(SB_TURN_SLOPE/speed_kmh); // slope/speed [kmh]

	//DEBUG
	//kfile_printf(&g_serial.fd,"%d,%d,%d,%d\r\n",secs_since_beacon,speed_kmh,heading_change_since_beacon,turn_threshold);

	if(secs_since_beacon >= SB_TURN_TIME && heading_change_since_beacon > turn_threshold){
		return true;
	}

	// SMART TIME CHECK
	float beaconRate;
	if(speed_kmh/*location->speedInKMH*/ < SB_LOW_SPEED){
		beaconRate = SB_SLOW_RATE;
	}else{
		if(speed_kmh /*location->speedInKMH*/ > SB_HI_SPEED){
			beaconRate = SB_FAST_RATE;
		}else{
			//beaconRate = (float)SB_FAST_RATE * (SB_HI_SPEED / location.speedInKMH);
			beaconRate = SB_FAST_RATE + (SB_SLOW_RATE - SB_FAST_RATE) * (SB_HI_SPEED - speed_kmh/*location->speedInKMH*/) / (SB_HI_SPEED-SB_LOW_SPEED);
		}
	}
	mtime_t rate = lroundf(beaconRate);
	return (timer_clock_seconds() - lastSendTimeSeconds) > (rate);
#else
	(void)location;
	return _fixed_interval_beacon_check();
#endif
}


/*
 * smart beacon algorithm
 */
void beacon_send_location(struct GPS *gps){
	if(!gps->valid){
		return;
	}

	// check heading direction changes
	// get location data
	Location location; // sizeof(Location) = 16;
	gps_get_location(gps,&location);

	bool _use_smart_beacon = true; //TODO read from settings
	bool shouldSend = false;
	if(_use_smart_beacon){
		shouldSend = _smart_beacon_check(&location);
	}else{
		shouldSend = _fixed_interval_beacon_check();
	}

	if(shouldSend){
		// prepare payload and send
		char payload[64];
		char s1 = g_settings.symbol[0];
		if(s1 == 0) s1 = '/';
		char s2 = g_settings.symbol[1];
		if(s2 == 0) s2 = '>';
		uint8_t payloadLen = snprintf_P(payload,63,PSTR("!%.7s%c%c%.8s%c%c%03d/%03dTinyAPRS"),
				gps->_term[GPRMC_TERM_LATITUDE],gps->_term[GPRMC_TERM_LATITUDE_NS][0],
				s1,
				gps->_term[GPRMC_TERM_LONGITUDE],gps->_term[GPRMC_TERM_LONGITUDE_WE][0],
				s2,
				nmea_decimal_int(gps->_term[GPRMC_TERM_HEADING]), // CSE
				nmea_decimal_int(gps->_term[GPRMC_TERM_SPEED])  // SPD, see APRS101 P27
				);

		// TODO support /A=aaaaaa altitude?
		_do_send(payload,payloadLen);
#if CFG_BEACON_SMART // heading support
		// save current position & timestamp
		memcpy(&lastLocation,&location,sizeof(Location));
#endif
		lastSendTimeSeconds = timer_clock_seconds();
	}

//	float beaconRate = SB_FAST_RATE;

//#if CFG_SMART_BEACON_ENABLED
//	// smart beacon algorithm - http://www.hamhud.net/hh2/smartbeacon.html
//	if(location.speedInKMH < SB_LOW_SPEED){
//		beaconRate = SB_SLOW_RATE;
//	}else{
//		if(location.speedInKMH > SB_HI_SPEED){
//			beaconRate = SB_FAST_RATE;
//		}else{
//			//beaconRate = (float)SB_FAST_RATE * (SB_HI_SPEED / location.speedInKMH);
//			beaconRate = SB_FAST_RATE + (SB_SLOW_RATE - SB_FAST_RATE) * (SB_HI_SPEED - location.speedInKMH) / (SB_HI_SPEED-SB_LOW_SPEED);
//		}
//	}
//
//	//SERIAL_PRINTF_P((&g_serial),PSTR("rate: %d, speed: %d\r\n"),(uint16_t)rate, lroundf(location.speedInKMH));
//#endif
//
//	mtime_t rate = lroundf(beaconRate);
//	static ticks_t ts = 0;
//	// first time will always trigger the send
//	if(ts == 0 || timer_clock() - ts > ms_to_ticks(rate * 1000)){
//		// payload
//		char payload[64];
//		char s1 = g_settings.symbol[0];
//		if(s1 == 0) s1 = '/';
//		char s2 = g_settings.symbol[1];
//		if(s2 == 0) s2 = '>';
//		uint8_t payloadLen = snprintf_P(payload,63,PSTR("!%.7s%c%c%.8s%c%c%03d/%03dTinyAPRS"),
//				gps->_term[GPRMC_TERM_LATITUDE],gps->_term[GPRMC_TERM_LATITUDE_NS][0],
//				s1,
//				gps->_term[GPRMC_TERM_LONGITUDE],gps->_term[GPRMC_TERM_LONGITUDE_WE][0],
//				s2,
//				nmea_decimal_int(gps->_term[GPRMC_TERM_HEADING]), // CSE
//				nmea_decimal_int(gps->_term[GPRMC_TERM_SPEED])  // SPD, see APRS101 P27
//				);
//
//		// TODO support /A=aaaaaa altitude?
//		_do_send(payload,payloadLen);
//		ts = timer_clock();


#if DEBUG_BEACON_PAYLOAD   // DEBUG DUMP
		kfile_print((&(g_serial.fd)),payload);
		kfile_putc('\r', &(g_serial.fd));
		kfile_putc('\n', &(g_serial.fd));
#endif
//	}

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

