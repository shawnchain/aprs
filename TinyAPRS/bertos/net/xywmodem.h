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
 * \defgroup xywmodem G4XYW modem module.
 * \ingroup net
 * \{
 *
 * \brief External modem.
 *
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 * $WIZ$ module_name = "xywmodem"
 * $WIZ$ module_configuration = "bertos/cfg/cfg_xywmodem.h"
 * $WIZ$ module_depends = "timer", "kfile", "hdlc"
 * $WIZ$ module_hw = "bertos/hw/hw_xywmodem.h"
 */

#ifndef NET_XYW_H
#define NET_XYW_H

#include "cfg/cfg_xywmodem.h"
#include "hw/hw_xywmodem.h"

#include <net/hdlc.h>

#include <cfg/compiler.h>

#include <io/kfile.h>

#include <struct/fifobuf.h>



/**
 *  modem context.
 */
typedef struct XYW
{
	/** Base "class" */
	KFile fd;

	/** FIFO for received data */
	FIFOBuffer rx_fifo;

	/** FIFO rx buffer */
	uint8_t rx_buf[CONFIG_XYW_RX_BUFLEN];

	/** FIFO for transmitted data */
	FIFOBuffer tx_fifo;

	/** FIFO tx buffer */
	uint8_t tx_buf[CONFIG_XYW_TX_BUFLEN];

		/** True while modem sends data */
	volatile bool sending;

	/**
	 * AFSK modem status.
	 * If 0 all is ok, otherwise errors are present.
	 */
	volatile int status;

		/** Hdlc context */
	Hdlc tx_hdlc;
	Hdlc rx_hdlc;

	/* keep a note of what speed we were started off at */
	uint16_t speed;
} XYW;

#define KFT_XYW MAKE_ID('X', '9', 'K', '6')

INLINE XYW *XYW_CAST(KFile *fd)
{
  ASSERT(fd->_type == KFT_XYW);
  return (XYW *)fd;
}

void xyw_head (KFile * fd, int c);
void xyw_tail (KFile * fd, int c);
void xyw_tx_int(void);
void xyw_rx_int(uint8_t this_bit);

void xyw_init (XYW * _xyw, int bps);

/** \} */ //defgroup xywmodem


#endif /* NET_XYW_H */
