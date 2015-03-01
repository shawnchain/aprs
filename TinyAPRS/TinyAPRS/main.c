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

#include "settings.h"
#include <ctype.h>

#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>

#include "sys_utils.h"

#include "beacon.h"


static Afsk afsk;
static AX25Ctx ax25;
static Serial ser;

#define ADC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

typedef enum{
	MODE_CMD = 0,
	MODE_KISS,
	MODE_DIGI,
	MODE_TRAC,
}RunMode;


static RunMode runMode = MODE_CMD;

///////////////////////////////////////////////////////////////////////////////////
// Message Callbacks

/*
 * Print on console the message that we have received.
 */
static void ax25_msg_callback(struct AX25Msg *msg){
	switch(runMode){
	case MODE_CMD:{
#if 1
		// Print received message to serial
		SERIAL_PRINTF_P((&ser), PSTR("\r\n>[%.6s-%d]->[%.6s-%d]"), msg->src.call, msg->src.ssid, msg->dst.call, msg->dst.ssid);
		if(msg->rpt_cnt > 0){
			SERIAL_PRINT_P((&ser),PSTR(", via["));
			for (int i = 0; i < msg->rpt_cnt; i++){
				SERIAL_PRINTF_P((&ser), PSTR("%.6s-%d,"), msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);
			}
			SERIAL_PRINTF((&ser),"]");
		}
		// DATA part
		SERIAL_PRINTF_P((&ser), PSTR("\r\n%.*s\r\n"), msg->len, msg->info);
#else
		if(msg) ss_messageCallback(msg,&ser);
#endif
		break;
	}
	case MODE_KISS:
		kiss_send_host(0x00/*channel ID*/,ax25.buf,ax25.frm_len - 2);
		break;

	default:
		break;

	}
}

///////////////////////////////////////////////////////////////////////////////////
// Command handlers

#if CONSOLE_TEST_COMMAND_ENABLED
/*
 * !{n} - send {n} test packets
 */
static bool cmd_test(Serial* pSer, char* command, size_t len){
	#define DEFAULT_REPEATS 5
	uint8_t repeats = 0;
	if(len > 0){
		repeats = atoi((const char*)command);
	}
	if(repeats == 0) repeats = DEFAULT_REPEATS;
	beacon_send_test(repeats);

	SERIAL_PRINTF_P(pSer,PSTR("Sending %d test packet...\r\n"),repeats);
	return true;
}
#endif

/*
 * AT+MODE=[0|1|2]
 */
static bool cmd_mode(Serial* pSer, char* value, size_t len){
	bool modeOK = false;
	if(len > 0 ){
		int i = atoi(value);
		if(i == (int)runMode){
			// already in this mode, bail out.
			return true;
		}

		modeOK = true;
		switch(i){
		case MODE_CMD:
			// COMMAND/CONFIG MODE
			runMode = MODE_CMD;
			ax25.pass_through = 0;		// parse ax25 frames
			kiss_set_enabled(false);	// kiss off
			beacon_set_enabled(false); 	// beacon off
			ser_purge(pSer);  			// clear all rx/tx buffer
			break;

		case MODE_KISS:
			// KISS MODE
			runMode = MODE_KISS;
			ax25.pass_through = 1;		// don't parse ax25 frames
			kiss_set_enabled(true);		// kiss on
			beacon_set_enabled(false); 	// beacon off
			ser_purge(pSer);  			// clear serial rx/tx buffer
			SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
			break;
/*
		case MODE_DIGI:
			// DIGI MODE
			runMode = MODE_DIGI;
			ax25.pass_through = 0;		// parse ax25 frames
			kiss_set_enabled(false);	// kiss off
			beacon_set_enabled(true);	// beacon on
			ser_purge(pSer);  			// clear serial rx/tx buffer
			break;

		case MODE_TRAC:
			// TRACKER MODE
			runMode = MODE_TRAC;
			// disable the ax25 module
			ax25.pass_through = 0;		// parse ax25 frames
			kiss_set_enabled(false);	// kiss off
			tracker_set_enabled(true);	// gps on
			break;
*/
		default:
			// unknown mode
			modeOK = false;
			break;
		}
	}


	if(!modeOK){
		SERIAL_PRINTF_P(pSer,PSTR("Invalid value %s, only int value [0|1|2] is accepted\r\n"),value);
	}

	return true;
}



/*
 * AT+KISS=1 - enable the KISS mode
 */
static bool cmd_kiss(Serial* pSer, char* value, size_t len){
	if(len > 0 && value[0] == '1'){
		runMode = MODE_KISS;
		kiss_set_enabled(true);
		ax25.pass_through = 1;
		ser_purge(pSer);  // clear all rx/tx buffer

		// disable beacon mode
		beacon_set_enabled(false);
		SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
	}else{
		SERIAL_PRINTF_P(pSer,PSTR("Invalid value %s, only value 1 is accepted\r\n"),value);
	}
	return true;
}

/*
 * AT+BEACON=[1|0] - enable the Beacon mode.
 */
static bool cmd_beacon(Serial* pSer, char* value, size_t len){
	(void)len;
	//FIXME - also support KISS mode when BEACON = 1
	if(value[0] == '0'){
		beacon_set_enabled(false);
		SERIAL_PRINT_P(pSer,PSTR("Beacon mode disabled\r\n"));
	}else if(value[0] == '1'){
		beacon_set_enabled(true);
		SERIAL_PRINT_P(pSer,PSTR("Beacon mode enabled\r\n"));
	}else{
		SERIAL_PRINTF_P(pSer,PSTR("Invalid value %s, only value 0 and 1 is accepted\r\n"),value);
	}
	return true;
}

/*
 * AT+SEND - just send the beacon message once
 */
static bool cmd_send(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	if(len == 0){
		// send test message
		beacon_send();
	}else{
		//TODO send user input message out
		//TODO build the ax25 path according settings
	}
	SERIAL_PRINT_P(pSer,PSTR("SEND OK\r\n"));
	return true;
}



static inline void _init_console(void){
    console_init(&ser);

    console_add_command(PSTR("BEACON"),cmd_beacon); 	// setup beacon text

    console_add_command(PSTR("MODE"),cmd_mode);					// setup tnc run mode
    console_add_command(PSTR("KISS"),cmd_kiss);					// enable KISS mode
    console_add_command(PSTR("DIGI"),cmd_kiss);					// enable DIGI mode


    // experimental commands
    console_add_command(PSTR("SEND"),cmd_send);

#if CONSOLE_TEST_COMMAND_ENABLED
    console_add_command(PSTR("TEST"),cmd_test);
#endif

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

	/*
	 * Here we initialize AX25 context, the channel (KFile) we are going to read messages
	 * from and the callback that will be called on incoming messages.
	 */
	ax25_init(&ax25, &afsk.fd, ax25_msg_callback);

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//FIXME - should both support KISS mode and CONFIG mode
#if CONFIG_KISS_ENABLED
	// passthrough(don't decode the frame) only when debug disabled
	ax25.pass_through = !SERIAL_DEBUG;
	kiss_init(&ser,&ax25,&afsk);
#else
	// Initialize the serial console
    ss_init(&ax25,&ser);
#endif

#if CONFIG_BEACON_ENABLED
    beacon_init(&ax25);
#endif

    //////////////////////////////////////////////////////////////
    // Initialize the console & commands
    _init_console();

    // Load settings
    settings_load();
}



int main(void)
{

	init();

	while (1)
	{
		/*
		 * This function will look for new messages from the AFSK channel.
		 * It will call the message_callback() function when a new message is received.
		 * If there's nothing to do, this function will call cpu_relax()
		 */
		ax25_poll(&ax25);

		switch(runMode){
		case MODE_CMD:{
			console_poll();
			break;
		}

#if CONFIG_KISS_ENABLED
		case MODE_KISS:{
			if(!kiss_enabled()){
				runMode = MODE_CMD;
				SERIAL_PRINT_P((&ser),PSTR("Exit KISS mode\r\n"));
				break;
			}
			kiss_serial_poll();
			kiss_queue_process();
			break;
		}
#endif

		default:
			break;
		}// end of switch(runMode)

// BEACON ROUTINS
#if CONFIG_BEACON_ENABLED
		beacon_poll();
#endif

#define FREE_RAM_DEBUG 0
#if FREE_RAM_DEBUG
		static uint32_t i = 0;
		if(i++ == 100000){
			i = 0;
			// log the stack size
			uint16_t ram = freeRam();
			SERIAL_PRINTF((&ser),"%u\r\n",ram);
		}
#endif
	} // end of while(1)
	return 0;
}
