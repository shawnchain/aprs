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
#include <stdio.h>
#include <string.h>

#include <drv/timer.h>

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
static size_t serialLen = 0;                    		// Counter for counting length of data from serial

/*
 * The console will always read console input char until met CRLF or buffer is full
 */
void console_parse(int c){
#if CONSOLE_SERIAL_READ_TIMEOUT > 0
	static ticks_t lastReadTick = 0;
	if((serialLen > 0) && (timer_clock() - lastReadTick > ms_to_ticks(CONSOLE_SERIAL_READ_TIMEOUT)) ){
		//LOG_INFO("Console - Timeout\n");
		serialLen = 0;
	}
#endif

	// read until met CR/LF/EOF or buffer is full
	if ((serialLen >= CONSOLE_SERIAL_BUF_LEN) || (c == '\r') || (c == '\n') || (c == EOF) ) {
		if(serialLen > 0){
			serialBuffer[serialLen] = 0; // complete the buffered string
			// parsing the command
			console_parse_command(pSerial,(char*)serialBuffer, serialLen);
			serialLen = 0;
		}
	} else {
		// keep in buffer
		serialBuffer[serialLen++] = c;
#if CONSOLE_SERIAL_READ_TIMEOUT > 0
		lastReadTick = timer_clock();
#endif
	}
}

void console_poll(void){
	int c;
	if(ser_available(pSerial)){
		c = ser_getchar_nowait(pSerial);
		console_parse(c);
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

	// A simple hack to command "!5"
#if CONSOLE_TEST_COMMAND_ENABLED
	if(commandLen >0 && command[0] == '!'){
		uint8_t repeats = 0;
		if(commandLen > 1)
			repeats = atoi((const char*)(command + 1));
		if(repeats == 0){
			repeats = 3;
		}else if(repeats > 9){
			repeats = 9;
		}
		commandLen = snprintf_P(command,11,PSTR("AT+TEST=%d\r"),repeats);
	}
#endif

	//TinyAPRS AT Command Handler
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

	// Compatible with OT2/Other TNCs KISS init command
	else if( (commandLen >=10) && (strcmp_P(command,PSTR("AMODE KISS")) == 0)){
		// enter the kiss mode
		// reuse the existing command buffer
		key = command + 6;
		key[4] = 0;
		value = command;
		value[0] = '1';
		value[1] = 0;
		valueLen = 1;
	}
	else if( (commandLen >=7) && (strcmp_P(command,PSTR("KISS ON")) == 0)){
		key = command;
		key[4] = 0;
		value = command + 5;
		value[0] = '1';
		value[1] = 0;
		valueLen = 1;
	}

	if(key == NULL && value == NULL){
		// bail out
		SERIAL_PRINTF_P(pSer,PSTR("INVALID CMD: %.*s\r\n"),commandLen,command);
		return;
	}

	// look the command registry
	PFUN_CMD_HANDLER fun = console_lookup_command(key);
	if(fun){
		if(!fun(pSer, value, valueLen)){
			// handle failure, should be invalid values
			SERIAL_PRINT_P(pSer,PSTR("INVALID CMD VALUE\r\n")); // user input command is parsed but the value is not valid
		}
	}else{
		SERIAL_PRINTF_P(pSer,PSTR("UNKNOWN CMD: %.*s\r\n"),commandLen,command);
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

