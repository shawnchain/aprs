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
 * Copyright 2013 Robin Gilks <g8ecj@gilks.org>
 *
 * -->
 *
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 */

#ifndef HW_XYWMODEM_H
#define HW_XYWMODEM_H

#include <stdint.h>
#include <avr/io.h>

#include "cfg/cfg_arch.h"



#define XYW_HW_INIT(bps)     do { (void)bps; /* Implement me */ } while (0)
#define XYW_TX_START         do { /* Implement me */ } while (0)
#define XYW_TX_DATA(data)    do { (void)data; /* Implement me */ } while (0)
#define XYW_TX_STOP          do { /* Implement me */ } while (0)



#endif
