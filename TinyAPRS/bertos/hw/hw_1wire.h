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
 *  Copyright (C) 2012 Robin Gilks
 * -->
 *
 * \addtogroup ow_driver 1-wire driver
 * \ingroup drivers
 * \{
 *
 *
 * \brief Driver for Dallas 1-wire devices
 *
 *
 * \author Peter Dannegger (danni(at)specs.de)
 * \author Martin Thomas (mthomas(at)rhrk.uni-kl.de)
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 * $WIZ$ module_name = "hw_1wire"
 */

#ifndef HW_1WIRE_H_
#define HW_1WIRE_H_

#include <stdint.h>

#include "cfg/cfg_arch.h"
#include "cfg/compiler.h"

#warning TODO:This is an example implementation, you must implement it!

	/**
	 * \defgroup 1wirehw_api Hardware API
	 * Access to this low level driver is mostly from the device specific layer. However, some functions - especially the 
	 * ow_set_bus() function operates at the lowest level.
	 *
	 * This functionality is especially useful when devices are hardwired and so removes the need to scan them for their addresses.
	 *
	 * API usage example:
	 * \code
	 * switch (sensor)
	 * {
	 * case SENSOR_LOW:
	 *    // low level sensor (ground) on PE4
	 *    ow_set_bus (&PINE, &PORTE, &DDRE, PE4);
	 *    if (!ow_busy ())                 // see if the conversion is complete
	 *    {
	 *       ow_ds18X20_read_temperature (NULL, &temperature_low);       // read the result
	 *       ow_ds18X20_start (NULL, false]);            // start the conversion process again
	 *    }
	 *    break;
	 * case SENSOR_HIGH:
	 *    // high level (roof) sensor on PE5
	 *    ow_set_bus (&PINE, &PORTE, &DDRE, PE5);
	 *    if (!ow_busy ())                 // see if the conversion is complete
	 *    {
	 *       ow_ds18X20_read_temperature (NULL, &temperature_hi);       // read the result
	 *       ow_ds18X20_start (NULL, false);            // start the conversion process again
	 *    }
	 *    break;
	 * \endcode
	 * \{
	 */


#if OW_ONE_BUS == 1

/* when all devices are on the same I/O pin */
#define OW_GET_IN()   ( /* Implement me! */ )
#define OW_OUT_LOW()  ( /* Implement me! */ )
#define OW_OUT_HIGH() ( /* Implement me! */ )
#define OW_DIR_IN()   ( /* Implement me! */ )
#define OW_DIR_OUT()  ( /* Implement me! */ )

/* no extra overhead to allow for */
#define OW_CONF_DELAYOFFSET 0

#else

#if ( CPU_FREQ < 1843200 )
#warning | Experimental multi-bus-mode is not tested for
#warning | frequencies below 1,84MHz. Use OW_ONE_WIRE or
#warning | faster clock-source (i.e. internal 2MHz R/C-Osc.).
#endif
#define OW_CONF_CYCLESPERACCESS 13
#define OW_CONF_DELAYOFFSET ( (uint16_t)( ((OW_CONF_CYCLESPERACCESS) * 1000000L) / CPU_FREQ ) )

/**
 * Set the port/data direction input pin dynamically
 *
 * \param in input port
 * \param out output port
 * \param ddr data direction register
 * \param pin I/O pin (bit number on port)
 *
 */
void ow_set_bus (volatile void * in, volatile void * out, volatile void * ddr, uint8_t pin)
{
	(void) in;
	(void) out;
	(void) ddr;
	(void) pin;
}

#define OW_GET_IN()   ( /* Implement me! */ )
#define OW_OUT_LOW()  ( /* Implement me! */ )
#define OW_OUT_HIGH() ( /* Implement me! */ )
#define OW_DIR_IN()   ( /* Implement me! */ )
#define OW_DIR_OUT()  ( /* Implement me! */ )

#endif

	/** \} */ //defgroup 1wirehw_api

/** \} */ //addtogroup ow_driver

#endif
