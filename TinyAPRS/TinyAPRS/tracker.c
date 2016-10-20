/*
 * \file tracker.c
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
 * \date 2016-10-8
 */


#include "tracker.h"

#include <math.h>
#include <stdlib.h>
#include <cfg/macros.h>

#include <net/afsk.h>
#include <net/ax25.h>
#include <drv/timer.h>
#include <drv/ser.h>

#include "beacon.h"
#include "settings.h"
#include "global.h"
#include "gps.h"
#include "utils.h"

#include "reader.h"

#define CFG_BEACON_SMART 1  // Beacon smart mode: 0 disabled, 1 by speed and heading

#define DUMP_BEACON_PAYLOAD 0
#define DUMP_GPS_INFO 0
#define DEBUG_GPS_OUTPUT 0

static mtime_t lastSendTimeSeconds = 0; // in seconds

#define SB_FAST_RATE 45		// 45 seconds

#if CFG_BEACON_SMART
//TODO - save in settings
#define SB_SLOW_RATE 120	// 3 minutes
#define SB_LOW_SPEED 5		// 5KM/h
#define SB_HI_SPEED 70		// 80KM/H
#define SB_TURN_TIME 15
#define SB_TURN_MIN 10.f
#define SB_TURN_SLOPE 240.f

static Location lastLocation = {
		.latitude=0.f,
		.longitude=0.f,
		.speedInKMH=0.f,
		.heading=0,
		.timestamp = 0
};

INLINE uint16_t _calc_heading(Location *l1, Location *l2){
	uint16_t d = abs(l1->heading - l2->heading) % 360;
	return (d <= 180)? d : 360 - d;
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

static bool _smart_beacon_turn_angle_check(Location *location,uint16_t secs_since_beacon){
	// we're stopped.
	if(location->heading == 0 || location->speedInKMH == 0){
		return false;
	}

	// previous location.heading == 0 means we're just started from last stop point.
	if(lastLocation.heading == 0){
		return secs_since_beacon >=  SB_TURN_TIME;
	}

	uint16_t heading_change_since_beacon =_calc_heading(location,&lastLocation); // (0~180 degrees)
	uint16_t turn_threshold = lroundf(SB_TURN_MIN + (SB_TURN_SLOPE/location->speedInKMH)); // slope/speed [kmh]
	if(secs_since_beacon >= SB_TURN_TIME && heading_change_since_beacon > turn_threshold){
		return true;
	}
	//DEBUG
	//kfile_printf(&g_serial.fd,"%d,%d,%d,%d\r\n",secs_since_beacon,speed_kmh,heading_change_since_beacon,turn_threshold);
	return false;
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
 *
 * reference aprsdroid - https://github.com/ge0rg/aprsdroid/blob/master/src/location/SmartBeaconing.scala
 */
static bool _smart_beacon_check(Location *location){
#if CFG_BEACON_SMART
	if(lastSendTimeSeconds == 0 || lastLocation.timestamp == 0){
		return true;
	}
	// get the delta of time/speed/heading for current location vs last location
	int16_t secs_since_beacon = location->timestamp - lastLocation.timestamp; //[second]
	if(secs_since_beacon <= 0){
		//	that could happen when current and last spot spans one day, so drop that
		return false;
	}

	// SMART HEADING CHECK
	if(_smart_beacon_turn_angle_check(location,secs_since_beacon))
		return true;

	// SMART TIME CHECK
	float beaconRate = 0.f;
	uint16_t calculated_speed_kmh = _calc_speed_kmh(location,&lastLocation);    //calcluated speed based on current/previous locations
	if(calculated_speed_kmh/*location->speedInKMH*/ < SB_LOW_SPEED){
		beaconRate = SB_SLOW_RATE;
	}else{
		if(calculated_speed_kmh /*location->speedInKMH*/ > SB_HI_SPEED){
			beaconRate = SB_FAST_RATE;
		}else{
			//beaconRate = (float)SB_FAST_RATE * (SB_HI_SPEED / location.speedInKMH);
			beaconRate = SB_FAST_RATE + (SB_SLOW_RATE - SB_FAST_RATE) * (SB_HI_SPEED - calculated_speed_kmh/*location->speedInKMH*/) / (SB_HI_SPEED-SB_LOW_SPEED);
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
static void tracker_update_location(struct GPS *gps){
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
		char s1 = g_settings.beacon.symbol[0];
		if(s1 == 0) s1 = '/';
		char s2 = g_settings.beacon.symbol[1];
		if(s2 == 0) s2 = '>';
		uint8_t len = snprintf_P(payload,63,PSTR("!%.7s%c%c%.8s%c%c%03d/%03d"),
				gps->_term[GPRMC_TERM_LATITUDE],gps->_term[GPRMC_TERM_LATITUDE_NS][0],
				s1,
				gps->_term[GPRMC_TERM_LONGITUDE],gps->_term[GPRMC_TERM_LONGITUDE_WE][0],
				s2,
				nmea_decimal_int(gps->_term[GPRMC_TERM_HEADING]), // CSE
				nmea_decimal_int(gps->_term[GPRMC_TERM_SPEED])  // SPD, see APRS101 P27
				);

		//TODO get text from settings!
		if(gps->altitude > 0){
			len += snprintf_P((char*)payload + len,63 - len,PSTR("/A=%06d"),gps->altitude);
		}

		len += snprintf_P((char*)payload + len, 63 - len, PSTR(" TinyAPRS"));

		beacon_send(payload,len);
#if CFG_BEACON_SMART // heading support
		// save current position & time stamp
		memcpy(&lastLocation,&location,sizeof(Location));
#endif
		lastSendTimeSeconds = timer_clock_seconds();

#if DUMP_BEACON_PAYLOAD   // DEBUG DUMP
		kfile_printf_P(((KFile*)(g_serialreader.ser)),PSTR("%s,\r\n"),payload);
#endif
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
//		beacon_send(payload,payloadLen);
//		ts = timer_clock();
//}

#if DUMP_GPS_INFO
	char* lat = gps->_term[GPRMC_TERM_LATITUDE];
	char* lon = gps->_term[GPRMC_TERM_LONGITUDE];
	char* spd = gps->_term[GPRMC_TERM_SPEED];
	kfile_printf_P((KFile*)g_serialreader.ser,PSTR("lat:%s, lon:%s, speed:%s\r\n"),lat,lon,spd);
	//kfile_printf_P((KFile*)g_serialreader.ser,PSTR("lat:%f, lon:%s, speed:%s\r\n"),lat,lon,spd);
#endif
}

void tracker_init(void){
	//TODO - initialize the GPS modules
}

void tracker_poll(void){
	if(serialreader_readline(&g_serialreader) > 0){
#if DEBUG_GPS_OUTPUT
		kfile_printf_P((KFile*)(g_serialreader.ser),PSTR("%s\r\n"),(char*)g_serialreader.data);
#endif

		// got the gps line!
		if(gps_parse(&g_gps,(char*)g_serialreader.data,g_serialreader.dataLen) && g_gps.valid){
			tracker_update_location(&g_gps);
		}
	}
}
