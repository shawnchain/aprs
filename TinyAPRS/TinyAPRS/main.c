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
 * Copyright 2010 Develer S.r.l. (http://www.develer.com/)
 *
 * -->
 *
 * \author Francesco Sacchi <batt@develer.com>
 * \author Luca Ottaviano <lottaviano@develer.com>
 * \author Daniele Basile <asterix@develer.com>
 *
 * \brief Arduino APRS radio demo.
 *
 * This example shows how to read and decode APRS radio packets.
 * It uses the following modules:
 * afsk
 * ax25
 * ser
 *
 * You will see how to use a serial port to output messages, init the afsk demodulator and
 * how to parse input messages using ax25 module.
 */

#include <cpu/irq.h>

#include <net/afsk.h>
#include <net/ax25.h>
#include <net/kiss.h>

#include <drv/ser.h>
#include <drv/timer.h>

#include <stdio.h>
#include <string.h>

#include "console.h"

#include "config.h"
#if SERIAL_DEBUG
#include <cfg/debug.h>
#endif

#include "buildrev.h"


static Afsk afsk;
static AX25Ctx ax25;
static Serial ser;

#define ADC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L


#if CONFIG_KISS_ENABLED

#else

//FIXME check memory
static uint8_t serialBuffer[CONFIG_AX25_FRAME_BUF_LEN+1]; // Buffer for holding incoming serial data
static int sbyte;                               // For holding byte read from serial port
static size_t serialLen = 0;                    // Counter for counting length of data from serial
static bool sertx = false;                      // Flag signifying whether it's time to send data
// received on the serial port.
#define SER_BUFFER_FULL (serialLen < CONFIG_AX25_FRAME_BUF_LEN-1)

#endif

/*
 * Print on console the message that we have received.
 */
static void ax25_msg_callback(struct AX25Msg *msg){
	/*
	kfile_printf(&ser.fd, "\n\nSRC[%.6s-%d], DST[%.6s-%d]\r\n", msg->src.call, msg->src.ssid, msg->dst.call, msg->dst.ssid);

	for (int i = 0; i < msg->rpt_cnt; i++)
		kfile_printf(&ser.fd, "via: [%.6s-%d]\r\n", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);

	kfile_printf(&ser.fd, "DATA: %.*s\r\n", msg->len, msg->info);
	*/
#if CONFIG_KISS_ENABLED
	kiss_send_host(0x00,ax25.buf,ax25.frm_len - 2);
#endif
	ss_messageCallback(msg,&ser);
}

static void init(void)
{
	IRQ_ENABLE;

	kdbg_init();
	timer_init();

	/* Initialize serial port, we are going to use it to show APRS messages*/
	ser_init(&ser, SER_UART0);
	ser_setbaudrate(&ser, SER_BAUD_RATE_9600);
    // For some reason BertOS sets the serial
    // to 7 bit characters by default. We set
    // it to 8 instead.
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	/*
	 * Init afsk demodulator. We need to implement the macros defined in hw_afsk.h, which
	 * is the hardware abstraction layer.
	 * We do not need transmission for now, so we set transmission DAC channel to 0.
	 */
	afsk_init(&afsk, ADC_CH, 0);

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//FIXME - should both support KISS mode and CONFIG mode
#if CONFIG_KISS_ENABLED
	kiss_init(&ser,&ax25,&afsk);
	ax25_init(&ax25, &afsk.fd, true /*keep the raw message*/, ax25_msg_callback);

	kfile_printf(&ser.fd, "TinyAPRS KISS TNC 1.0 (f%da%dk%dr%d) - init\r\n",CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,CONFIG_KISS_ENABLED,VERS_BUILD);
#else
	/*
	 * Here we initialize AX25 context, the channel (KFile) we are going to read messages
	 * from and the callback that will be called on incoming messages.
	 */
	ax25_init(&ax25, &afsk.fd, false, ax25_msg_callback);

	// Initialize the serial console
    ss_init(&ax25,&ser);
#endif
}


//static AX25Call path[] = AX25_PATH(AX25_CALL("BG5HHP", 0), AX25_CALL("nocall", 0), AX25_CALL("wide1", 1), AX25_CALL("wide2", 2));
//#define APRS_MSG    ">Test BeRTOS APRS http://www.bertos.org"


int main(void)
{
	init();
	ticks_t start = timer_clock();

	while (1)
	{
		/*
		 * This function will look for new messages from the AFSK channel.
		 * It will call the message_callback() function when a new message is received.
		 * If there's nothing to do, this function will call cpu_relax()
		 */
		ax25_poll(&ax25);

#if CONFIG_KISS_ENABLED
		kiss_serial_poll();
		kiss_queue_process();

#else

		#if 0	// - TEST ONLY, DISABLED -
		// Send out message every 5sec
		if (timer_clock() - start > ms_to_ticks(5000L))
		{
			start = timer_clock();
			ax25_sendVia(&ax25, path, countof(path), APRS_MSG, sizeof(APRS_MSG));
		}
		#endif

        // Poll for incoming serial data
        if (!sertx && ser_available(&ser)) {
            // We then read a byte from the serial port.
            // Notice that we use "_nowait" since we can't
            // have this blocking execution until a byte
            // comes in.
            sbyte = ser_getchar_nowait(&ser);

            // If SERIAL_DEBUG is specified we'll handle
            // serial data as direct human input and only
            // transmit when we get a LF character
            #if SERIAL_DEBUG
                // If we have not yet surpassed the maximum frame length
                // and the byte is not a "transmit" (newline) character,
                // we should store it for transmission.
                if ((serialLen < CONFIG_AX25_FRAME_BUF_LEN) && (sbyte != 10)) {
                    // Put the read byte into the buffer;
                    serialBuffer[serialLen] = sbyte;
                    // Increment the read length counter
                    serialLen++;
                } else {
                    // If one of the above conditions were actually the
                    // case, it means we have to transmit, se we set
                    // transmission flag to true.
                    sertx = true;
                }
            #else
                // Otherwise we assume the modem is running
                // in automated mode, and we push out data
                // as it becomes available. We either transmit
                // immediately when the max frame length has
                // been reached, or when we get no input for
                // a certain amount of time.

                serialBuffer[serialLen++] = sbyte;
                if (serialLen >= CONFIG_AX25_FRAME_BUF_LEN-1) {
                	sertx = true;
                }

                start = timer_clock();
            #endif
        } else {
            if (!SERIAL_DEBUG && serialLen > 0 && timer_clock() - start > ms_to_ticks(TX_MAXWAIT)) {
                sertx = true;
            }
        }

        if (sertx) {
        	// parse serial input
            ss_serialCallback(serialBuffer, serialLen, &ser, &ax25);
            sertx = false;
            serialLen = 0;
        }
#endif // end of #if CONFIG_KISS_ENABLED
	} // end of while(1)
	return 0;
}
