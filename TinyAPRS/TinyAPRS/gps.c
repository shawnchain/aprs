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
#include <io/kfile.h>
#include <drv/ser.h>
#include <drv/timer.h>

#include "global.h"


#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define METER_TO_FEET 3.2808399

#define RADIANS(x) ((x) * DEG_TO_RAD)
#define sq(x) ((x)*(x))

void gps_init(GPS *gps){
	uint8_t i;

	// initialize the GPS port
	memset(gps,0,sizeof(GPS));

	//Assume SIRF chip GPS using 4800 baud rate by default
	// So use SIRF command to set to 9600
	PGM_P pstr_baudrate_P = PSTR("$PSRF100,1,9600,8,1,0*0D\r\n");  /** Sets baud to 9600*/
	ser_setbaudrate(&g_serial, 4800L);
	timer_delay(150);
	ser_purge(&g_serial);
	for (i = 0; i < strlen_P(pstr_baudrate_P); i++){
		kfile_putc(pgm_read_byte(pstr_baudrate_P + i),&(g_serial.fd));
	}
	timer_delay(150);
	ser_setbaudrate(&g_serial, SER_DEFAULT_BAUD_RATE);
	timer_delay(150);
	ser_purge(&g_serial);

	// Disable unused data
	PGM_P pstr_config_P = PSTR("$PSRF103,1,0,0,1*25\r\n" 		/** Disables GPGLL */
							   "$PSRF103,2,0,0,1*26\r\n" 		/** Disables GPGSA */
							   "$PSRF103,3,0,0,1*27\r\n" 		/** Disables GPGSV */
							   "$PSRF103,5,0,0,1*21\r\n" 		/** Disables GPVTG */
							   );
	/* Write the configuration sentences to the module */
	for (i = 0; i < strlen_P(pstr_config_P); i++){
		//soft_uart_putchar(pgm_read_byte(pstr_config_P + i));
		kfile_putc(pgm_read_byte(pstr_config_P + i),&(g_serial.fd));
	}
	timer_delay(50);
	ser_purge(&g_serial);

	// Initialize the pin13(PORTB BV(5)) for GPS signal indicator
	GPS_LED_INIT();
}

INLINE int nmea_dehex(char a) {
	// returns base-16 value of chars '0'-'9' and 'A'-'F';
	// does not trap invalid chars!
	if ((int)a >= 65) {
		return (int)a - 55;
	} else {
		return (int)a - 48;
	}
}
//$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n

#define GPRMC_PARITY_INITIAL 0x4b
#define GPGGA_PARITY_INITIAL 0x56
int gps_parse(GPS *gps, char *sentence, uint8_t len){
	uint8_t sentenceType = 0;

	GPS_LED_OFF();
	if(len <= MIN_SENTENCEN_CHARS || len > MAX_SENTENCEN_CHARS){
		return 0;
	}
	int parity = 0;
	// make sure it's started with '$GPRMC'
	if(strncmp_P(sentence,PSTR("$GPRMC"),6) == 0){
		sentenceType = 1; // GPRMC type
		parity = GPRMC_PARITY_INITIAL; // check sum of GPRMC: 0x4b
	}else if(strncmp_P(sentence,PSTR("$GPGGA"),6) == 0){
		sentenceType = 2; // GPGGA type
		parity = GPGGA_PARITY_INITIAL; // check sum of GPGGA: 0x56
	}else{
		return 0;
	}

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
				if(sentenceType == 1){
					gps->valid = ((gps->_term[GPRMC_TERM_STATUS])[0] == 'A');
				}else if(sentenceType == 2){
					gps->valid = ((gps->_term[GPGGA_TERM_FIXQUALITY])[0] != '0');
				}
				/*
				if(terms >5){
					// lat/lon in APRS  is always: hhmm.ssN/hhhmm.ssE
					gps->_lat = gps->_term[2];
					gps->_lat[7] = (gps->_term[3])[0]; // N or S
					gps->_lat[8] = 0;

					gps->_lon = gps->_term[4];
					gps->_lon[8] = (gps->_term[5])[0]; // W or E
					gps->_lon[9] = 0;
				}
				*/
				if(gps->valid){
					GPS_LED_ON();
					if(sentenceType == 2){
					// read the altitude values
						float alt = nmea_decimal_float(gps->_term[GPGGA_TERM_ALTITUDE]);
						gps->altitude = lroundf(alt * METER_TO_FEET);
						return 0;
					}
					return 1;
				}
			}
			break;

		default:
			break;
		}

	} // end of for loop
	return 0;
}

#define TO_NUM(X) (X - '0'/*48*/)
void gps_get_location(GPS *gps, Location *pLoc){

	// calculate signed degree-decimal value of latitude term
	float _gprmc_lat = nmea_decimal_float(gps->_term[GPRMC_TERM_LATITUDE]) / 100.0;
	float _degs = floor(_gprmc_lat);
	_gprmc_lat = (100.0 * (_gprmc_lat - _degs)) / 60.0;
	_gprmc_lat += _degs;
	// southern hemisphere is negative-valued
	if ((gps->_term[GPRMC_TERM_LATITUDE_NS])[0] == 'S') {
		_gprmc_lat = 0.0 - _gprmc_lat;
	}
	pLoc->latitude = _gprmc_lat;

	// calculate signed degree-decimal value of longitude term
	float _gprmc_long = nmea_decimal_float(gps->_term[GPRMC_TERM_LONGITUDE]) / 100.0;
	_degs = floor(_gprmc_long);
	_gprmc_long = (100.0 * (_gprmc_long - _degs)) / 60.0;
	_gprmc_long += _degs;
	// western hemisphere is negative-valued
	if ((gps->_term[GPRMC_TERM_LONGITUDE_WE])[0] == 'W') {
		_gprmc_long = 0.0 - _gprmc_long;
	}
	pLoc->longitude = _gprmc_long;
//	pLoc->latitude = nmea_decimal_float(gps->_term[GPRMC_TERM_LATITUDE]);
//	pLoc->longitude = nmea_decimal_float(gps->_term[GPRMC_TERM_LONGITUDE]);

	pLoc->speedInKMH = nmea_decimal_float(gps->_term[GPRMC_TERM_SPEED]) * KMPH;
	pLoc->heading = nmea_decimal_int(gps->_term[GPRMC_TERM_HEADING]);

	// convert the utc in 1 day into seconds
	// ignoring the years
	// timestamp is 6 bytes long;
	char *utc = gps->_term[GPRMC_TERM_UTC_TIME];
	if(utc && strlen(utc) >= 6){
		pLoc->timestamp = (TO_NUM(utc[0]) * 10 + TO_NUM(utc[1]) ) * 3600;	// hour
		pLoc->timestamp+= (TO_NUM(utc[2]) * 10 + TO_NUM(utc[3]) ) * 60;		// minute
		pLoc->timestamp+= (TO_NUM(utc[4]) * 10 + TO_NUM(utc[5]) );			// second
	}else{
		pLoc->timestamp = 0;
	}

	pLoc->altitude = gps->altitude;
}

uint16_t nmea_decimal_int(char* s) {
	// returns base-10 value of zero-termindated string
	// that contains only chars '+','-','0'-'9','.';
	// does not trap invalid strings!
	uint16_t rl = 0;
	int i = 0;

	if ((s[i] == '-') || (s[i] == '+')) {
		i++;
	}
	while (s[i] != 0 && s[i] != '.') {
		rl = (10 * rl) + (s[i] - 48);
		i++;
	}

	return rl;
}

float nmea_decimal_float(char* s) {
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

float gps_distance_between(Location *loc1, Location *loc2, float units_per_meter) {
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothised sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be upto 0.5%.
	float lat1 = loc1->latitude;
	float lat2 = loc2->latitude;

	float delta = RADIANS(loc1->longitude - loc2->longitude);
	float sdlong = sin(delta);
	float cdlong = cos(delta);
	lat1 = RADIANS(lat1);
	lat2 = RADIANS(lat2);
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
	return fabs(delta) * 6372795 * units_per_meter;
}

#if 0
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
