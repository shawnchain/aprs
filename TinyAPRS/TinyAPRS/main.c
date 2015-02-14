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

//#include <mware/parser.h>

#include "settings.h"
#include <ctype.h>

#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>

#include "sys_utils.h"

#include "beacon.h"

#include "buildrev.h"


static Afsk afsk;
static AX25Ctx ax25;
static Serial ser;

static void console_serial_poll(void);
static void console_parse_command(char* command, size_t len);

#define ADC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

typedef enum{
	MODE_CMD = 0,
	MODE_KISS,
}RunMode;

static RunMode runMode = MODE_CMD;

/*
static const char string_1[] PROGMEM = "String 1";
static const char string_2[] PROGMEM = "String 2";
static const char string_3[] PROGMEM = "String 3";
static const char string_4[] PROGMEM = "String 4";
static const char string_5[] PROGMEM = "String 5";

const char *string_table[] =
{
string_1,
string_2,
string_3,
string_4,
string_5,
};

static void _testPGMString(void){
	char buffer[10];
	for (unsigned char i = 0; i < 5; i++)
	{
//	strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[i])));
//	strcpy_P(buffer, string_table[i]);
	// Display buffer on LCD.
//	SERIAL_PRINTF((&ser),"%s\r\n",buffer);

	SERIAL_PRINTF((&ser),"%d\r\n",(uint16_t)string_table[i]);
	}

	return;
}
*/

/*
 * Print on console the message that we have received.
 */
static void ax25_msg_callback(struct AX25Msg *msg){
	switch(runMode){
	case MODE_CMD:{
#if 1
		// Print received message to serial
		SERIAL_PRINTF((&ser), "\r\n>[%.6s-%d]->[%.6s-%d]", msg->src.call, msg->src.ssid, msg->dst.call, msg->dst.ssid);
		if(msg->rpt_cnt > 0){
			SERIAL_PRINTF((&ser),", via[");
			for (int i = 0; i < msg->rpt_cnt; i++){
				SERIAL_PRINTF((&ser), "%.6s-%d,", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid);
			}
			SERIAL_PRINTF((&ser),"]");
		}
		// DATA part
		SERIAL_PRINTF((&ser), "\r\n%.*s\r\n", msg->len, msg->info);
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

/*
 *
 */
static void _print_greeting_banner(void){
#if CONFIG_KISS_ENABLED
	/*
	static const PROGMEM char p[] = "TinyAPRS TNC (KISS) 1.0 (f%da%dr%d) - init\r\n";
	char t[64];
	sprintf_P(t, p,CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
	kfile_printf(&ser.fd,"%10s",t);
	kfile_printf(&ser.fd,(const char*)t,CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
	*/
	kfile_printf(&ser.fd, "TinyAPRS TNC (KISS) 1.0 (f%da%dr%d)\r\n" ,CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
#else
	kfile_printf(&ser.fd, "TinyAPRS TNC (Demo) 1.0 (f%da%dr%d)\r\n",CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
#endif
}

static void _print_settings(void){
	// DEBUG Purpose
//		char buf[7];
//		memset(buf,0,7);
//		memcpy(buf,g_settings.callsign,6);
//		uint8_t bufLen = 7;
//		settings_get(SETTINGS_CALLSIGN,buf,&bufLen);
//		for(int i = 0;i < 6;i++){
//			SERIAL_PRINTF((&ser),"%d,",g_settings.callsign[i]);
//		}
//		SERIAL_PRINTF((&ser),"%d,",g_settings.ssid);
	//SERIAL_PRINTF((&ser),"%s-%d\r\n",buf,g_settings.ssid);

	SERIAL_PRINTF((&ser), "%6s-%d\r\n",g_settings.callsign,g_settings.ssid);
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

    _print_greeting_banner();
    settings_load();
    _print_settings();
}


#define APRS_TEST_SEND 1

int main(void)
{

	init();

	uint16_t ram = freeRam();
	SERIAL_PRINTF((&ser),"Free RAM: %u\r\n",ram);

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
			console_serial_poll();
			break;
		}

#if CONFIG_KISS_ENABLED
		case MODE_KISS:{
			if(!kiss_enabled()){
				runMode = MODE_CMD;
				SERIAL_PRINTF((&ser),"Exit KISS mode\r\n");
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


//FIXME check memory usage
#define CONSOLE_SERIAL_BUF_LEN 64 						//CONFIG_AX25_FRAME_BUF_LEN
static uint8_t serialBuffer[CONSOLE_SERIAL_BUF_LEN+1]; 	// Buffer for holding incoming serial data
static int sbyte;                               		// For holding byte read from serial port
static size_t serialLen = 0;                    		// Counter for counting length of data from serial
static bool sertx = false;                      		// Flag signifying whether it's time to send data
// received on the serial port.
#define SER_BUFFER_FULL (serialLen < CONSOLE_SERIAL_BUF_LEN-1)
static void console_serial_poll(void){
	ticks_t start = timer_clock();
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
			if ((serialLen < CONSOLE_SERIAL_BUF_LEN) && (sbyte != 10) && (sbyte != 13)) {
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
			if (serialLen >= CONSOLE_SERIAL_BUF_LEN-1) {
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
		serialBuffer[serialLen] = 0; // end of the command string
		// parse serial input
		/*
		ss_serialCallback(serialBuffer, serialLen, &ser, &ax25);
		*/
		console_parse_command((char*)serialBuffer, serialLen);
		sertx = false;
		serialLen = 0;
	}
}

/*
const char PROGMEM cmd_kiss []  = "KISS";
const char PROGMEM cmd_callsign []  = "CALLSIGN";
const char PROGMEM cmd_ssid []  = "SSID";
PGM_P string_table[] =
{
cmd_kiss,
cmd_callsign,
cmd_ssid
};
*/
/*
PROGMEM const char *cmd_table []  = {
		cmd_kiss,
		cmd_callsign,
		cmd_ssid
};
*/

static void console_parse_command(char* command, size_t len){
	bool settingsChanged = false;
	char *key = NULL, *value = NULL;
	uint8_t valueLen = 0;

#if APRS_TEST_SEND && CONFIG_BEACON_ENABLED
	if(len > 0 && command[0] == '!'){
		beacon_send_test(5);
		SERIAL_PRINTF((&ser),"TESTING...\r\n");
		return;
	}
#endif

	// convert to upper case
	strupr(command);
	//AT+X=Y
	if(len >=6 && command[0] == 'A' && command[1] == 'T' && command[2] == '+' ){
		const char s[2] = "=";
		char* t = strtok((command + 3),s);
		if(t != NULL){
			key = t;
			t = strtok(NULL,s);
			if(t){
				value = t;
				valueLen = strlen(value);
			}
		}
	}

	if(key == NULL && value == NULL){
		// bail out
		SERIAL_PRINTF((&ser),"INVALID CMD: %.*s\r\n",len,command);
		return;
	}

	if(strcmp((const char*)key,"KISS") == 0 && value[0] == '1'){
		runMode = MODE_KISS;
		kiss_set_enabled(true);
		ax25.pass_through = 1;
		ser_purge(&ser);  // clear all rx/tx buffer

		// disable beacon mode
		beacon_set_enabled(false);

		SERIAL_PRINTF((&ser),"Enter KISS mode\r\n");
	}
#if CONFIG_BEACON_ENABLED
	else if(strcmp((const char*)key,"BEACON") == 0 /*&& value[0] == '1'*/){
		//FIXME - also support KISS mode when BEACON = 1
		if(value[0] == '0'){
			beacon_set_enabled(false);
			SERIAL_PRINTF((&ser),"Beacon mode disabled\r\n");
		}else if(value[0] == '1'){
			beacon_set_enabled(true);
			SERIAL_PRINTF((&ser),"Beacon mode enabled\r\n");
		}else{
			SERIAL_PRINTF((&ser),"Invalid value %s, only value 0 and 1 is accepted\r\n",value);
		}
	}
	else if(strcmp((const char*)key,"SEND") == 0){
		if(valueLen == 0){
			// send test message
			beacon_send();
		}else{
			//TODO send user input message out
			//TODO build the ax25 path according settings
		}
		SERIAL_PRINTF((&ser),"SEND OK\r\n");
	}
#endif
	else if(strcmp((const char*)key,"CALLSIGN") == 0){
		settings_set(SETTINGS_CALLSIGN,value,valueLen);
		settingsChanged = true;
		SERIAL_PRINTF((&ser),"CALLSIGN: %s\r\n",value);
	}
	else if(strcmp((const char*)key,"SSID") == 0){
		SERIAL_PRINTF((&ser),"SSID: %s\r\n",value);
		uint8_t ssid = atoi((const char*)value);
		settings_set(SETTINGS_SSID,&ssid,1);
		settingsChanged = true;
	}
	else if(strcmp((const char*)key,"RESET") == 0){
		if(value[0] == '1'){
			// clear the settings if AT+RESET=1
			SERIAL_PRINTF((&ser),"Settings cleared\r\n");
			settings_clear();
		}
		//TODO - reboot the device
		//soft_reset();
	}
	else if(strcmp((const char*)key,"INFO") == 0){
		_print_greeting_banner();
		_print_settings();
	}
	// Unknown
	else{
		SERIAL_PRINTF((&ser),"UNKNOWN CMD: %.*s\r\n",len,command);
	}

	if(settingsChanged){
		settings_save();
	}
	return;
}
