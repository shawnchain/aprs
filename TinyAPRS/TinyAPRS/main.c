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
#include "command.h"

#include "buildrev.h"


static Afsk afsk;
static AX25Ctx ax25;
static Serial ser;

#define ADC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

typedef enum{
	MODE_CMD = 0,
	MODE_KISS,
}RunMode;

static RunMode runMode = MODE_CMD;

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
// Console message print

/*
 * print the welcome message to console
 */
static void _print_greeting_banner(Serial *pSer){
#if CONFIG_KISS_ENABLED
	SERIAL_PRINTF_P(pSer, PSTR("\r\nTinyAPRS TNC (KISS) 1.0 (f%da%dr%d)\r\n"),CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
#else
	SERIAL_PRINTF_P(pSer, PSTR("\r\nTinyAPRS TNC (Demo) 1.0 (f%da%dr%d)\r\n"),CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
#endif
}

/*
 * Print the settings to console
 */
static void _print_settings(void){
	// DEBUG Purpose
	char buf[7];
	memset(buf,0,7);
	uint8_t bufLen = 6;
	settings_get(SETTINGS_CALLSIGN,buf,&bufLen);
	SERIAL_PRINTF_P((&ser), PSTR("Beacon: %s-%d\r\n"),buf,g_settings.ssid);
}

static inline void _print_freemem(Serial *pSer){
	// Print free ram
	uint16_t ram = freeRam();
	SERIAL_PRINTF(pSer,"Free RAM: %u\r\n",ram);
}
///////////////////////////////////////////////////////////////////////////////////
// Command handlers

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

static bool cmd_help(Serial* pSer, char* command, size_t len){
	(void)command;
	(void)len;
	SERIAL_PRINT_P(pSer,PSTR("\r\nAT commands supported\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("-----------------------------------------------\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+INFO\t\t\t;Display modem info\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+CALL=[CALLSIGN]\t;Set callsign\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+SSID=[SSID]\t\t;Set ssid\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+KISS=1\t\t;Enter kiss mode\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+HELP\t\t\t;Display help messages\r\n"));

	SERIAL_PRINT_P(pSer,  PSTR("\r\nCopyRights 2015, BG5HHP(shawn.chain@gmail.com)\r\n\r\n"));

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

static bool cmd_settings_callsign(Serial* pSer, char* value, size_t len){
	if(len > 0){
		settings_set(SETTINGS_CALLSIGN,value,len);
		settings_save();
	}
	char buf[7];
	memset(buf,0,sizeof(buf));
	uint8_t bufLen = sizeof(buf) - 1;
	settings_get(SETTINGS_CALLSIGN,buf,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("CALLSIGN: %s\r\n"),buf);
	return true;
}

static bool cmd_settings_ssid(Serial* pSer, char* value, size_t len){
	if(len > 0){
		uint8_t ssid = atoi((const char*)value);
		settings_set(SETTINGS_SSID,&ssid,1);
		settings_save();
	}

	uint8_t iSSID = 0;
	uint8_t bufLen = 1;
	settings_get(SETTINGS_SSID,&iSSID,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("SSID: %d\r\n"),iSSID);

	return true;
}

static bool cmd_reset(Serial* pSer, char* value, size_t len){
	if(len > 0 && value[0] == '1'){
		// clear the settings if AT+RESET=1
		SERIAL_PRINT_P(pSer,PSTR("Settings cleared\r\n"));
		settings_clear();
	}
	//TODO - reboot the device
	//soft_reset();
	return true;
}

static bool cmd_info(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	_print_greeting_banner(pSer);
	_print_settings();
	_print_freemem(pSer);
	return true;
}

static inline void _init_console(void){
    console_init(&ser);
    console_add_command(PSTR("INFO"),cmd_info);
    console_add_command(PSTR("CALL"),cmd_settings_callsign);
    console_add_command(PSTR("SSID"),cmd_settings_ssid);
    console_add_command(PSTR("KISS"),cmd_kiss);
    console_add_command(PSTR("RESET"),cmd_reset);
    console_add_command(PSTR("BEACON"),cmd_beacon);
    console_add_command(PSTR("HELP"),cmd_help);

    // experimental commands
    console_add_command(PSTR("SEND"),cmd_send);
    console_add_command(PSTR("TEST"),cmd_test);
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


#define APRS_TEST_SEND 1

int main(void)
{

	init();

	// Initialization done, display the welcome banner and settings info
	cmd_info((&ser),0,0);

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
