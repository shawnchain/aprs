/*
 * \file command.c
 * <!--
 * This file is part of TinyAPRS.
 * Released under GPL License
 *
 * Copyright 2015 Shawn Chain (shawn.chain@gmail.com)
 *
 * -->
 *
 * \brief 
 *
 * \author shawn
 * \date 2015-2-19
 */

#include "command.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "sys_utils.h"
#include <string.h>

#include <drv/timer.h>

#include "cfg/cfg_console.h"

static Serial *pSerial;
static void console_parse_command(Serial *pSer, char* command, size_t commandLen);

struct COMMAND_ENTRY{
	PGM_P cmdName;
	PFUN_CMD_HANDLER cmdHandler;
};
static struct COMMAND_ENTRY cmdEntries[CONSOLE_MAX_COMMAND];
static uint8_t cmdCount;

void console_init(Serial *ser){
	pSerial = ser;
	memset(cmdEntries,0, sizeof(struct COMMAND_ENTRY) * CONSOLE_MAX_COMMAND);
	cmdCount = 0;
}

//FIXME check memory usage
static uint8_t serialBuffer[CONSOLE_SERIAL_BUF_LEN+1]; 	// Buffer for holding incoming serial data
static int sbyte;                               		// For holding byte read from serial port
static size_t serialLen = 0;                    		// Counter for counting length of data from serial
static bool sertx = false;                      		// Flag signifying whether it's time to send data
// received on the serial port.
#define SER_BUFFER_FULL (serialLen < CONSOLE_SERIAL_BUF_LEN-1)
void console_poll(void){
	ticks_t start = timer_clock();
	// Poll for incoming serial data
	if (!sertx && ser_available(pSerial)) {
		// We then read a byte from the serial port.
		// Notice that we use "_nowait" since we can't
		// have this blocking execution until a byte
		// comes in.
		sbyte = ser_getchar_nowait(pSerial);

		// If SERIAL_DEBUG is specified we'll handle
		// serial data as direct human input and only
		// transmit when we get a LF character
		#if CONSOLE_SERIAL_DEBUG
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
		if (!CONSOLE_SERIAL_DEBUG && serialLen > 0 && timer_clock() - start > ms_to_ticks(CONSOLE_TX_MAXWAIT)) {
			sertx = true;
		}
	}

	if (sertx) {
		serialBuffer[serialLen] = 0; // end of the command string
		// parse serial input
		if(serialLen > 0)
			console_parse_command(pSerial,(char*)serialBuffer, serialLen);
		sertx = false;
		serialLen = 0;
	}
}

static PFUN_CMD_HANDLER console_lookup_command(const char* command){
	for(int i = 0; i < CONSOLE_MAX_COMMAND;i++){
		if(cmdEntries[i].cmdName == NULL){
			break;
		}
		if(strcasecmp_P(command,cmdEntries[i].cmdName) == 0){
			return cmdEntries[i].cmdHandler;
		}
	}
	return NULL;
}

static void console_parse_command(Serial *pSer, char* command, size_t commandLen){
	char *key = NULL, *value = NULL;
	uint8_t valueLen = 0;

	// convert to upper case
	strupr(command);

	/*
#if APRS_TEST_SEND && CONFIG_BEACON_ENABLED
	if(commandLen > 0 && command[0] == '!'){
		cmd_test(pSer, command, commandLen);
		return;
	}
#endif
	*/

	//AT+X=Y
	if(commandLen >=6 && command[0] == 'A' && command[1] == 'T' && command[2] == '+' ){
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
		SERIAL_PRINTF_P(pSer,PSTR("INVALID CMD: %.*s\r\n"),commandLen,command);
		return;
	}

	// look the command registry
	PFUN_CMD_HANDLER fun = console_lookup_command(key);
	if(fun && fun(pSer, value, valueLen)){
		return;
	}

/*
	if(strcmp((const char*)key,"KISS") == 0){
		if(cmd_kiss(pSer, value, valueLen)){
			return;
		}
	}

#if CONFIG_BEACON_ENABLED
	else if(strcmp((const char*)key,"BEACON") == 0){
		if(cmd_beacon(pSer, value, valueLen)){
			return;
		}
	}
	else if(strcmp((const char*)key,"SEND") == 0){
		if(cmd_send(pSer, value, valueLen)){
			return;
		}
	}
#endif
	else if(strcmp((const char*)key,"CALLSIGN") == 0){
		if(cmd_settings_callsign(pSer, value, valueLen)){
			return;
		}
	}
	else if(strcmp((const char*)key,"SSID") == 0){
		if(cmd_settings_ssid(pSer, value, valueLen)){
			return;
		}
	}
	else if(strcmp((const char*)key,"RESET") == 0){
		if(cmd_reset(pSer, value, valueLen)){
			return;
		}
	}
	else if(strcmp((const char*)key,"INFO") == 0){
		if(cmd_info(pSer, value, valueLen)){
			return;
		}
	}
*/

	// Unknown
	//else{
		SERIAL_PRINTF_P(pSer,PSTR("UNKNOWN CMD: %.*s\r\n"),commandLen,command);
	//}

//	if(settingsChanged){
//		settings_save();
//	}
	return;
}


/*
 * Add command handler to the internal register
 */
void console_add_command(PGM_P cmd, PFUN_CMD_HANDLER handler){
	if(cmdCount >= CONSOLE_MAX_COMMAND)return;
	cmdEntries[cmdCount].cmdName = cmd;
	cmdEntries[cmdCount].cmdHandler = handler;
	cmdCount++;
}

