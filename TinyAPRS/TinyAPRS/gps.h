/*
 * \file gps.h
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

/*
  nmea.cpp - NMEA 0183 sentence decoding library for Wiring & Arduino
  Copyright (c) 2008 Maarten Lamers, The Netherlands.
  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef NMEA_H_
#define NMEA_H_

#define	ALL					0								// connect to all datatypes
#define	GPRMC				1								// connect only to GPRMC datatype
#define	MTR					1.0							// meters per meter
#define	KM					0.001						// kilometers per meter
#define	MI					0.00062137112		// miles per meter
#define	NM					0.00053995680		// nautical miles per meter
#define	PARSEC				0.000000000000	// parsecs per meter (approximation)
#define	MPS					0.51444444 			// meters-per-second in one knot
#define	KMPH				1.852  					// kilometers-per-hour in one knot
#define	MPH					1.1507794				// miles-per-hour in one knot
#define	KTS					1.0 						// knots in one knot
#define	LIGHTSPEED			0.000000001716	// lightspeeds in one knot

#define MAX_TERMS 			16
#define MAX_SENTENCEN_CHARS 80

typedef void (*gps_nmea_callback)(void*);

typedef struct GPS{
	char*	_utc;
	char	_status;
	char*   _lat;
	char*   _lon;
	char*   _speed;
	char*   _heading;

	char*	_date;

	int		_terms;
	char*	_term[MAX_TERMS];
//	char	_sentence[MAX_SENTENCEN_CHARS];
	char*   _sentence;

	int		n;
	int		_gprmc_tag;
	int		_state;
	int		_parity;

	gps_nmea_callback callback;
}GPS;



/*
 *
 */
void gps_init(GPS *gps, char* buf, gps_nmea_callback cb);

/*
 *
 */
int gps_decode(GPS *gps,char c);

#endif /* NMEA_H_ */
