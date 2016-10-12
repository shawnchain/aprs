/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * copyright (c) Davide Gironi, 2012
 *
 * -->
 *
 * \brief 
 *
 * \author Davide Gironi
 *
 *
 * $WIZ$ module_name = "pcf8574"
 * $WIZ$ module_depends = "i2c"
 */

/*
pcf8574 lib 0x02


Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef PCF8574_H_
#define PCF8574_H_

#define PCF8574_ADDRBASE (0x27) //device base address

#define PCF8574_I2CINIT 1       //init i2c

#define PCF8574_MAXDEVICES 2    //max devices, depends on address (3 bit)
#define PCF8574_MAXPINS 8       //max pin per device


//pin status
volatile uint8_t pcf8574_pinstatus[PCF8574_MAXDEVICES];


//functions
void pcf8574_init (void);
extern int8_t pcf8574_getoutput (uint8_t deviceid);
extern int8_t pcf8574_getoutputpin (uint8_t deviceid, uint8_t pin);
extern int8_t pcf8574_setoutput (uint8_t deviceid, uint8_t data);
extern int8_t pcf8574_setoutputpins (uint8_t deviceid, uint8_t pinstart,
                                     uint8_t pinlength, int8_t data);
extern int8_t pcf8574_setoutputpin (uint8_t deviceid, uint8_t pin,
                                    uint8_t data);
extern int8_t pcf8574_setoutputpinhigh (uint8_t deviceid, uint8_t pin);
extern int8_t pcf8574_setoutputpinlow (uint8_t deviceid, uint8_t pin);
extern int8_t pcf8574_getinput (uint8_t deviceid);
extern int8_t pcf8574_getinputpin (uint8_t deviceid, uint8_t pin);
#endif
