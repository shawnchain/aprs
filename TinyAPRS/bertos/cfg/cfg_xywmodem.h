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
 * Copyright 2013 Robin Gilks
 *
 * -->
 *
 * \brief Configuration file for external modem.
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 */

#ifndef CFG_XYW_H
#define CFG_XYW_H

/**
 * Module logging level.
 *
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "log_level"
 */
#define XYW_LOG_LEVEL      LOG_LVL_WARN

/**
 * Module logging format.
 *
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "log_format"
 */
#define XYW_LOG_FORMAT     LOG_FMT_TERSE


/**
 * XYW receiver buffer fifo length.
 *
 * $WIZ$ type = "int"
 * $WIZ$ min = 2
 */
#define CONFIG_XYW_RX_BUFLEN 32

/**
 * XYW transimtter buffer fifo length.
 *
 * $WIZ$ type = "int"
 * $WIZ$ min = 2
 */
#define CONFIG_XYW_TX_BUFLEN 32

/**
 * XYW RX timeout in ms, set to -1 to disable.
 * $WIZ$ type = "int"
 * $WIZ$ min = -1
 */
#define CONFIG_XYW_RXTIMEOUT 0


/**
 * XYW Preamble length in [ms], before starting transmissions.
 * $WIZ$ type = "int"
 * $WIZ$ min = 1
 */
#define CONFIG_XYW_PREAMBLE_LEN 20UL


/**
 * XYW Trailer length in [ms], before stopping transmissions.
 * $WIZ$ type = "int"
 * $WIZ$ min = 1
 */
#define CONFIG_XYW_TRAILER_LEN 3UL

/**
 * Bitrate of generated TX clock and data
 *
 * $WIZ$ type = "int"
 */
#define CONFIG_XYW_BITRATE   9600


#endif /* CFG_XYW_H */
