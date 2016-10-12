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
 * \brief External modem.
 *
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 */

#include "xywmodem.h"
#include <hw/hw_xywmodem.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <drv/timer.h>
#include "cfg/cfg_arch.h"
#include "cfg/module.h"



static XYW *xyw;


/**
 * Transmit interrupt entry point from hardware layer
 * Interfaces to hdlc layer to get next bit to transmit, stops transmitter if no more data
 */

void xyw_tx_int(void)
{
	int16_t this_bit;
	/* get a new TX bit. */
	/* note that hdlc module does all the NRZI as well as bit stuffing etc */
	/* if run out of data then clear sending flag, drop PTT, stop TX clock, stop interrupts */
	this_bit = hdlc_encode (&xyw->tx_hdlc, &xyw->tx_fifo);
	switch (this_bit)
	{
	case -1:
		xyw->sending = false;
		// stop TX clock, drop PTT, stop TX interrupt
		XYW_TX_STOP;
		return;
	case 0:
	case 1:
		// output bit
		XYW_TX_DATA (this_bit);
		break;
	}

}

/**
 * Receive interrupt entry point from hardware layer
 * \param this_bit the received bit from an I/O line
 */

void xyw_rx_int(uint8_t this_bit)
{
	xyw->status = hdlc_decode (&xyw->rx_hdlc, this_bit, &xyw->rx_fifo);
}

static void xyw_txStart(XYW *xyw)
{
	if (!xyw->sending)
	{
		xyw->sending = true;
		// output TX clock, raise PTT line,  enable interrupt to send data
		XYW_TX_START;
	}
}

static size_t xyw_read(KFile *fd, void *_buf, size_t size)
{
	XYW *xyw = XYW_CAST (fd);
	uint8_t *buf = (uint8_t *)_buf;

	#if CONFIG_XYW_RXTIMEOUT == 0
	while (size-- && !fifo_isempty_locked(&xyw->rx_fifo))
	#else
	while (size--)
	#endif
	{
		#if CONFIG_XYW_RXTIMEOUT != -1
		ticks_t start = timer_clock();
		#endif

		while (fifo_isempty_locked(&xyw->rx_fifo))
		{
			cpu_relax();
			#if CONFIG_XYW_RXTIMEOUT != -1
			if (timer_clock() - start > ms_to_ticks(CONFIG_XYW_RXTIMEOUT))
				return buf - (uint8_t *)_buf;
			#endif
		}

		*buf++ = fifo_pop_locked(&xyw->rx_fifo);
	}

	return buf - (uint8_t *)_buf;
}

static size_t xyw_write(KFile *fd, const void *_buf, size_t size)
{
	XYW *xyw = XYW_CAST (fd);
	const uint8_t *buf = (const uint8_t *)_buf;

	while (size--)
	{
		while (fifo_isfull_locked(&xyw->tx_fifo))
			cpu_relax();

		fifo_push_locked(&xyw->tx_fifo, *buf++);
		xyw_txStart(xyw);
	}

	return buf - (const uint8_t *)_buf;
}

static int xyw_flush(KFile *fd)
{
	XYW *xyw = XYW_CAST (fd);
	while (xyw->sending)
		cpu_relax();
	return 0;
}

static int xyw_error(KFile *fd)
{
	XYW *xyw = XYW_CAST (fd);
	int err;

	ATOMIC(err = xyw->status);
	return err;
}

static void xyw_clearerr (KFile * fd)
{
	XYW *xyw = XYW_CAST (fd);
	ATOMIC (xyw->status = 0);
}


/**
 * Sets head timings by defining the number of flags to output
 * Has to be done here as this is the only module that interfaces directly to hdlc!!
 *
 * \param fd caste xyw context.
 * \param c value
 *
 */
void xyw_head (KFile * fd, int c)
{
	XYW *xyw = XYW_CAST (fd);
	hdlc_head (&xyw->tx_hdlc, c, xyw->speed);
}

/**
 * Sets tail timings by defining the number of flags to output
 * Has to be done here as this is the only module that interfaces directly to hdlc!!
 *
 * \param fd caste xyw context.
 * \param c value
 *
 */
void xyw_tail (KFile * fd, int c)
{
	XYW *xyw = XYW_CAST (fd);
	hdlc_tail (&xyw->tx_hdlc, c, xyw->speed);
}


/**
 * Initialize an g4xyw 9600 modem.
 * \param _xyw XYW context to operate on.
 * \param bps speed to operate at
 */

void xyw_init(XYW *_xyw, int bps)
{
	xyw = _xyw;
	#if CONFIG_XYW_RXTIMEOUT != -1
	MOD_CHECK(timer);
	#endif
	memset(xyw, 0, sizeof(*xyw));

	XYW_HW_INIT(bps);

	fifo_init(&xyw->rx_fifo, xyw->rx_buf, sizeof(xyw->rx_buf));
	fifo_init(&xyw->tx_fifo, xyw->tx_buf, sizeof(xyw->tx_buf));

	hdlc_init (&xyw->rx_hdlc);
	hdlc_init (&xyw->tx_hdlc);
	// set initial defaults for timings
	hdlc_head (&xyw->tx_hdlc, CONFIG_XYW_PREAMBLE_LEN, bps);
	hdlc_tail (&xyw->tx_hdlc, CONFIG_XYW_TRAILER_LEN, bps);
	DB (xyw->fd._type = KFT_XYW);
	xyw->fd.write = xyw_write;
	xyw->fd.read = xyw_read;
	xyw->fd.flush = xyw_flush;
	xyw->fd.error = xyw_error;
	xyw->fd.clearerr = xyw_clearerr;
	xyw->speed = bps;
}
