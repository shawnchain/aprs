/*
 * \file nmea.c
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

#include "nmea.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define _GPRMC_TERM   "$GPRMC,"		// GPRMC datatype identifier

static int nmea_dehex(char a);


void nmea_init(NMEA *pnmea, char* buf, gps_nmea_callback cb){
	memset(pnmea,0,sizeof(NMEA));
	pnmea->_sentence = buf;
	pnmea->callback = cb;
//	pnmea->_sentence[0] = 0;
}

int nmea_decode(NMEA *pnmea, char c){
        // avoid runaway sentences (>99 chars or >29 terms) and terms (>14 chars)
        if ((pnmea->n >= MAX_SENTENCEN_CHARS) || (pnmea->_terms >= MAX_TERMS) ) {
                pnmea->_state = 0;
        }
        // LF and CR always reset parser
        if ((c == 0x0A) || (c == 0x0D)) {
                pnmea->_state = 0;
        }
        // '$' always starts a new sentence
        if (c == '$') {
                pnmea->_gprmc_tag = 0;
                pnmea->_parity = 0;
                pnmea->_terms = 0;
                pnmea->_sentence[0] = c;
                pnmea->n = 1;
                pnmea->_state = 1;
                return 0;
        }
        // parse other chars according to parser state
        switch (pnmea->_state) {
        case 0:
                // waiting for '$', do nothing
                break;
        case 1:
                // decode chars after '$' and before '*' found
                if (pnmea->n < 7) {
                        // see if first seven chars match "$GPRMC,"
                        if (c == _GPRMC_TERM[pnmea->n]) {
                                pnmea->_gprmc_tag++;
                        }
                }
                // add received char to sentence
                switch (c) {
                case ',':
                        // ',' delimits the individual terms
                        pnmea->_sentence[pnmea->n++] = 0;
                        // point to the next term
                        pnmea->_term[pnmea->_terms++] = pnmea->_sentence + pnmea->n;
                        pnmea->_parity = pnmea->_parity ^ c;
                        break;
                case '*':
                        // '*' delimits term and precedes checksum term
                        pnmea->_sentence[pnmea->n++] = 0;
                        pnmea->_state++;
                        break;
                default:
                        // all other chars between '$' and '*' are part of a term
                        pnmea->_sentence[pnmea->n++] = c;
                        pnmea->_parity = pnmea->_parity ^ c;
                        break;
                }
                break;
        case 2:
                // first char following '*' is checksum MSB
                pnmea->_term[pnmea->_terms++] = pnmea->_sentence + pnmea->n;
                pnmea->_sentence[pnmea->n++] = c;
                pnmea->_parity = pnmea->_parity - (16 * nmea_dehex(c));         // replace with bitshift?
                pnmea->_state++;
                break;
        case 3:
                // second char after '*' completes the checksum (LSB)
                pnmea->_sentence[pnmea->n++] = c;
                pnmea->_sentence[pnmea->n++] = 0;
				pnmea->_state = 0;
				pnmea->_parity = pnmea->_parity - nmea_dehex(c);
				// when parity is zero, checksum was correct!
				if (pnmea->_parity == 0) {
					// accept only GPRMC datatype?
					if (pnmea->_gprmc_tag == 6 /*it's GPRMC tag!*/) {
						// when sentence is of datatype GPRMC
						// store values of relevant GPRMC terms
						pnmea->_utc = pnmea->_term[0];
						pnmea->_status = (pnmea->_term[1])[0];

						// lat/lon in APRS  is always: hhmm.ssN/hhhmm.ssE
						pnmea->_lat = pnmea->_term[2];
						pnmea->_lat[7] = (pnmea->_term[3])[0]; // N or S
						pnmea->_lat[8] = 0;

						pnmea->_lon = pnmea->_term[4];
						pnmea->_lon[8] = (pnmea->_term[5])[0]; // W or E
						pnmea->_lon[9] = 0;

						// speed/heading/date
						pnmea->_speed = pnmea->_term[6];
						pnmea->_heading = pnmea->_term[7];
						pnmea->_date = pnmea->_term[8];

						//sentence accepted!
						if(pnmea->callback){
							pnmea->callback(pnmea);
						}
						return 1;
					}
				}
				break;
		default:
				pnmea->_state = 0;
				break;
		}
		return 0;
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

#if 0
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
#endif
