/*
 * \file gps.c
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
 * \date 2015-3-27
 */

#include "gps.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <avr/pgmspace.h>

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

static int nmea_dehex(char a);
static float nmea_decimal(char* s);

void gps_init(GPS *gps){
	memset(gps,0,sizeof(GPS));
}

//$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n
int gps_parse(GPS *gps, char *sentence, uint8_t len){

	if(len <= MIN_SENTENCEN_CHARS || len > MAX_SENTENCEN_CHARS){
		return 0;
	}
	// make sure it's started with '$GPRMC'
	if(strncmp_P(sentence,PSTR("$GPRMC"),6) != 0){
		// not started with that, so return
		return 0;
	}

	int parity = 0x4b; // the checksum of 'GPRMC'

	uint8_t state = 0;
	int terms = 0;

	for(int n = 6;n < len;n++){
		char c = sentence[n];
		if ((c == 0x0A) || (c == 0x0D) || terms == MAX_TERMS) {
			// we're done
			break;
		}
		switch(state){

		case 0:
			switch(c){
				case ',':
					// ',' delimits the individual terms
					sentence[n] = 0;
					// point to the next term
					gps->_term[terms++] = sentence + n + 1;
					parity = parity ^ c;
					break;
				case '*':
					// '*' delimits term and precedes checksum term
					sentence[n] = 0;
					state++; // 1
					break;
				default:
					// all other chars between '$' and '*' are part of a term
					parity = parity ^ c;
					break;
			}
			break;

		case 1:
			// first char following '*' is checksum MSB
			gps->_term[terms++] = sentence + n;
			parity = parity - (16 * nmea_dehex(c));         // replace with bitshift?
			state++;
			break;

		case 2:
			// second char after '*' completes the checksum (LSB)
			state = 0;
			parity = parity - nmea_dehex(c);
			// when parity is zero, checksum was correct!
			if (parity == 0) {
				// store values of relevant GPRMC terms
				gps->_status = (gps->_term[GPRMC_TERM_STATUS])[0];

				if(terms >5){
					// lat/lon in APRS  is always: hhmm.ssN/hhhmm.ssE
					gps->_lat = gps->_term[2];
					gps->_lat[7] = (gps->_term[3])[0]; // N or S
					gps->_lat[8] = 0;

					gps->_lon = gps->_term[4];
					gps->_lon[8] = (gps->_term[5])[0]; // W or E
					gps->_lon[9] = 0;
				}

				return 1;
			}
			break;

		default:
			break;
		}

	} // end of for loop
	return 0;
}

float gps_get_speed_kmh(GPS *gps){
	char *s = gps->_term[GPRMC_TERM_SPEED];
	return nmea_decimal(s) * KMPH;
}

float gps_get_heading(GPS *gps){
	char *s = gps->_term[GPRMC_TERM_HEADING];
	return nmea_decimal(s);
}


static int nmea_dehex(char a) {
	// returns base-16 value of chars '0'-'9' and 'A'-'F';
	// does not trap invalid chars!
	if ((int)a >= 65) {
		return (int)a - 55;
	} else {
		return (int)a - 48;
	}
}

static float nmea_decimal(char* s) {
	// returns base-10 value of zero-termindated string
	// that contains only chars '+','-','0'-'9','.';
	// does not trap invalid strings!
	long rl = 0;
	float rr = 0.0;
	float rb = 0.1;
	bool dec = false;
	int i = 0;

	if ((s[i] == '-') || (s[i] == '+')) {
		i++;
	}
	while (s[i] != 0) {
		if (s[i] == '.') {
			dec = true;
		} else {
			if (!dec) {
				rl = (10 * rl) + (s[i] - 48);
			} else {
				rr += rb * (float) (s[i] - 48);
				rb /= 10.0;
			}
		}
		i++;
	}
	rr += (float) rl;
	if (s[0] == '-') {
		rr = 0.0 - rr;
	}
	return rr;
}

#if 0
float gps_distance_between(float lat1, float long1, float lat2, float long2,
		float units_per_meter) {
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothised sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be upto 0.5%.
	float delta = radians(long1-long2);
	float sdlong = sin(delta);
	float cdlong = cos(delta);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	float slat1 = sin(lat1);
	float clat1 = cos(lat1);
	float slat2 = sin(lat2);
	float clat2 = cos(lat2);
	delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
	delta = sq(delta);
	delta += sq(clat2 * sdlong);
	delta = sqrt(delta);
	float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
	delta = atan2(delta, denom);
	return delta * 6372795 * units_per_meter;
}

float gps_initial_course(float lat1, float long1, float lat2, float long2) {
	// returns initial course in degrees (North=0, West=270) from
	// position 1 to position 2, both specified as signed decimal-degrees
	// latitude and longitude.
	float dlon = radians(long2-long1);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	float a1 = sin(dlon) * cos(lat2);
	float a2 = sin(lat1) * cos(lat2) * cos(dlon);
	a2 = cos(lat1) * sin(lat2) - a2;
	a2 = atan2(a1, a2);
	if (a2 < 0.0) {
		a2 += TWO_PI;		// modulo operator doesn't seem to work on floats
	}
	return degrees(a2);
}
#endif
