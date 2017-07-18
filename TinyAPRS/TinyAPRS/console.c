#define ENABLE_HELP true

#include "console.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "settings.h"
#include "reader.h"

#include <net/ax25.h>

#if MOD_BEACON
#include "beacon.h"
#endif

#include <cfg/cfg_afsk.h> // afst configuration info
#include <cfg/cfg_kiss.h> // kiss config

#include <drv/timer.h>
#include <drv/ser.h>

#include "buildrev.h"

#include "global.h"

#if CONSOLE_HELP_COMMAND_ENABLED
static bool cmd_help(Serial* pSer, char* command, size_t len);
#endif

static bool cmd_info(Serial* pSer, char* value, size_t len);

#if MOD_BEACON && CONSOLE_SEND_COMMAND_ENABLED
static bool cmd_send(Serial* pSer, char* command, size_t len);
#endif

#if MOD_BEACON && CFG_BEACON_TEST
static bool cmd_test_send(Serial* pSer, char* command, size_t len);
#endif

struct COMMAND_ENTRY{
	PGM_P cmdName;
	PFUN_CMD_HANDLER cmdHandler;
};
static struct COMMAND_ENTRY cmdEntries[CONSOLE_MAX_COMMAND];
static uint8_t cmdCount;

/*
 * Add command handler to the internal register
 */
void console_add_command(PGM_P cmd, PFUN_CMD_HANDLER handler){
	if(cmdCount >= CONSOLE_MAX_COMMAND)return;
	cmdEntries[cmdCount].cmdName = cmd;
	cmdEntries[cmdCount].cmdHandler = handler;
	cmdCount++;
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

void console_parse_command(char* command, size_t commandLen){
	char *key = NULL, *value = NULL;
	uint8_t valueLen = 0;
	Serial *pSer = &g_serial;

	// A simple hack to command "!5"
#if MOD_BEACON && CFG_BEACON_TEST
	if(commandLen > 0 && command[0] == '!'){
		cmd_test_send(pSer, command+1,commandLen - 1);
		return;
	}
#endif

	if(commandLen ==1 && command[0] == '?'){
		cmd_info(pSer,0,1);
		return;
	}

	if(commandLen ==2 && command[0] == '?' && command[1] == '?'){
		cmd_info(pSer,0,1);
#if CONSOLE_HELP_COMMAND_ENABLED
		cmd_help(pSer,0,0);
#endif
		return;
	}


	//TinyAPRS AT Command Handler
	if(commandLen >=6 && (command[0] == 'A' || command[0] == 'a') && (command[1] == 'T' || command[1] == 't') && (command[2] == '+') ){
		// looking for the '='
		char* t = NULL;
		uint8_t i = 3;
		for(;i < commandLen;i++){
			if(command[i] == '='){
				t = command + i;
				break;
			}
		}
		if(t != NULL){
			*t = 0; // split the key=value string into 2 strings.
			key = command + 3;
			value = t + 1;
			valueLen = strlen(value);
		}
	}

	// Compatible with OT2/Other TNCs KISS init command
	else if( (commandLen >=10) && (strcasecmp_P(command,PSTR("AMODE KISS")) == 0)){
		// enter the kiss mode
		// reuse the existing command buffer
		key = command + 6;
		key[4] = 0;
		value = command;
		value[0] = '1';
		value[1] = 0;
		valueLen = 1;
	}
	else if( (commandLen >=7) && (strcasecmp_P(command,PSTR("KISS ON")) == 0)){
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
	// convert to upper case
	strupr(key);
	PFUN_CMD_HANDLER fun = console_lookup_command(key);
	if(fun){
		if(!fun(pSer, value, valueLen)){
			// handle failure, should be invalid values
			SERIAL_PRINT_P(pSer,PSTR("INVALID CMD VALUE\r\n")); // user input command is parsed but the value is not valid
		}
	}else{
		SERIAL_PRINTF_P(pSer,PSTR("UNKNOWN CMD: %.*s\r\n"),commandLen,command);
	}

	return;
}

////////////////////////////////////////////////////////////////////////////////////
// Command Handlers
////////////////////////////////////////////////////////////////////////////////////

static const PROGMEM char BANNER[] = "\r\nTinyAPRS Firmware 1.1.1 (f%da%d-%d) BG5HHP\r\n";
static const PROGMEM char RESTRICTIONS[] = "Non-Commercial Use Only, All Rights Reserved.\r\n";
static const PROGMEM char COPYRIGHTS[] = "Copyright 2015-2017, BG5HHP(shawn.chain@gmail.com)\r\n";
static bool cmd_info(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	uint16_t freemem = freeRam();
	// print welcome banner
	kfile_printf_P((KFile*)pSer,BANNER,CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);
	//SERIAL_PRINT_P(pSer,  RESTRICTIONS);
	//SERIAL_PRINT_P(pSer,  COPYRIGHTS);

	// print settings only when len > 0
	if(len == 0){
		goto exit;
	}

	// print settings
	{
	char buf[10];
	AX25Call myCall;
	settings_get_mycall(&myCall);
	ax25call_to_string(&myCall,buf);
	SERIAL_PRINTF_P(pSer, PSTR("MyCall: %s\r\n"),buf);
	}

	SERIAL_PRINTF_P(pSer, PSTR("Mode: %d\r\n"),g_settings.run_mode);

	// print the ax25 stat
#if CONFIG_AX25_STAT
	SERIAL_PRINTF_P(pSer, PSTR("RX:%d, TX:%d, ERR: %d\r\n"),g_ax25.stat.rx_ok,g_ax25.stat.tx_ok,g_ax25.stat.rx_err);
#endif

	// print free memory
	kfile_printf_P((KFile*)pSer,PSTR("Free RAM: %u\r\n"),freemem);

exit:
	kfile_flush((KFile*)pSer);
	return true;
}

#if CONSOLE_HELP_COMMAND_ENABLED
static bool cmd_help(Serial* pSer, char* command, size_t len){
	(void)command;
	(void)len;
	SERIAL_PRINT_P(pSer,PSTR("\r\nAT commands supported\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("-----------------------------------------------------------\r\n"));
#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	SERIAL_PRINT_P(pSer,PSTR("AT+CALL=[CALLSIGN-SSID]\t\t;Set my callsign\r\n"));
#if CONSOLE_SETTINGS_COMMAND_DEST_ENABLED
	SERIAL_PRINT_P(pSer,PSTR("AT+DEST=[CALLSIGN-SSID]\t\t;Set destination callsign\r\n"));
#endif
	SERIAL_PRINT_P(pSer,PSTR("AT+PATH=[WIDE1-1,WIDE2-2]\t;Set PATH, max 2 allowed\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+SYMBOL=[SYMBOL_TABLE/IDX]\t;Set beacon symbol\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+BEACON=[45]\t\t\t;Set beacon interval, 0 to disable \r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+TEXT=[!3011.54N/12007.35E>]\t;Set beacon text \r\n"));
#endif
	SERIAL_PRINT_P(pSer,PSTR("AT+MODE=[0|1|2]\t\t\t;Set device run mode, see manual\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+KISS=[1]\t\t\t;Enter kiss mode\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("??\t\t\t\t;Display this help messages\r\n"));

	SERIAL_PRINT_P(pSer,  PSTR("\r\n"));
	SERIAL_PRINT_P(pSer,  RESTRICTIONS);
	SERIAL_PRINT_P(pSer,  COPYRIGHTS);


	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// AT SETTINGS COMMAND SUPPORT
#if CONSOLE_SETTINGS_COMMANDS_ENABLED

/*
 * parse AT+CALL=CALL-ID
 */
static bool cmd_settings_mycall(Serial* pSer, char* value, size_t valueLen){
	CallData calldata;
	settings_get_call_data(&calldata);

	if(valueLen > 0){
		ax25call_from_string(&calldata.myCall,value);
		settings_set_call_data(&calldata);
	}
	char callString[16];
	ax25call_to_string(&calldata.myCall,callString);
	SERIAL_PRINTF_P(pSer,PSTR("My Call: %s\r\n"),callString);
	return true;
}

/*
 * parse AT+DEST=CALL-ID
 */
#if CONSOLE_SETTINGS_COMMAND_DEST_ENABLED
static bool cmd_settings_destcall(Serial* pSer, char* value, size_t valueLen){
	CallData calldata;
	settings_get_call_data(&calldata);
	if(valueLen > 0){
		ax25call_from_string(&calldata.destCall,value);
		settings_set_call_data(&calldata);
	}
	char callString[16];
	ax25call_to_string(&calldata.destCall,callString);
	SERIAL_PRINTF_P(pSer,PSTR("DEST Call: %s\r\n"),callString);
	return true;
}
#endif

/*
 * parse AT+PATH=PATH1,PATH2
 */
static bool cmd_settings_path(Serial* pSer, char* value, size_t valueLen){
	CallData calldata;
	settings_get_call_data(&calldata);
	if(valueLen > 0){
		// convert to upper case
		//strupr(value);
		const char s[] = ",";
		char *path1 = NULL, *path2=NULL;
		uint8_t path1Len = 0, path2Len = 0;
		// split the "path1,path2" string
		char* t = strtok((value),s);
		if(t != NULL){
			// path1
			path1 = t;
			path1Len = strlen(t);
			t = strtok(NULL,s);
			if(t){
				path2 = t;
				path2Len = strlen(t);
			}
			if(path1Len > 0){
				ax25call_from_string(&calldata.path1,path1);
				if(path2Len > 0){
					ax25call_from_string(&calldata.path2,path2);
				}else{
					// no path2, so set an empty call object
					memset(&calldata.path2,0,sizeof(AX25Call));
				}
				settings_set_call_data(&calldata);
			}
		}
	}

	// display the path
	char p1[10],p2[10];
	ax25call_to_string(&calldata.path1,p1);
	ax25call_to_string(&calldata.path2,p2);
	SERIAL_PRINTF_P(pSer,PSTR("PATH: %s,%s\r\n"),p1,p2);
	return true;
}

static bool cmd_settings_reset(Serial* pSer, char* value, size_t len){
	if(len > 0 && (value[0] == '1' || value[0] == '2' )){
		if(value[0] == '2'){
			// clear the settings if AT+RESET=1
			settings_clear();
			SERIAL_PRINT_P(pSer,PSTR("Settings cleared, "));
		}
		SERIAL_PRINT_P(pSer,PSTR("Restarting...\r\n"));
		//reboot the device
		soft_reset();
	}
	return true;
}

//TODO -implement me
static bool cmd_settings_symbol(Serial* pSer, char* value, size_t len){
	if(len >= 2){
		settings_set_params(SETTINGS_SYMBOL,value,2);
		settings_save();
	}

	uint8_t symbol[2];
	uint8_t bufLen = 2;
	settings_get_params(SETTINGS_SYMBOL,symbol,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("SYMBOL: %c%c\r\n"),symbol[0],symbol[1]);

	return true;
}

#if SETTINGS_SUPPORT_BEACON_TEXT
static bool cmd_settings_beacon_text(Serial* pSer, char* value, size_t valueLen){
	if(valueLen > 0){
		// set the beacon text
		settings_set_beacon_text(value,valueLen);
		//SERIAL_PRINT_P(pSer, "OK\n\r");
		//return true;
	}

#define DEBUG_BEACON_TEXT 1
#if DEBUG_BEACON_TEXT
	char buf[64]; // see SETTINGS_BEACON_TEXT_MAX_LEN
	/*uint8_t bytesRead = */settings_get_beacon_text(buf,64);
	kfile_printf_P((KFile*)pSer,PSTR(">%s\n\r"),buf);
	uint16_t free = freeRam();
	SERIAL_PRINTF_P(pSer, PSTR("free mem: %d\n\r"),free)
#endif
	return true;
}
#endif

static bool cmd_settings_beacon_interval(Serial* pSer, char* value, size_t valueLen){
	uint16_t i = 0;
	if(valueLen > 0){
		i = atoi((const char*) value) & 0xffff;
		settings_set_params(SETTINGS_BEACON_INTERVAL,&i,2);
		settings_save();
	}

	uint8_t bufLen = 2;
	settings_get_params(SETTINGS_BEACON_INTERVAL,&i,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("Beacon Interval: %d seconds\r\n"),i);

	return true;
}


/*
 * enable/disable smart beacon
 */
//static bool cmd_settings_smartbeacon(Serial* pSer, char* value, size_t valueLen){
//	(void)pSer;
//	if(valueLen == 1){
//		if(value[0] == '0' && g_settings.smart_beacon == 1){
//			g_settings.smart_beacon = 0;
//			settings_save();
//		}else if(value[0] == '1' && g_settings.smart_beacon == 0){
//			g_settings.smart_beacon = 1;
//			settings_save();
//		}
//	}
//	SERIAL_PRINTF_P(pSer, PSTR("smart beacon: %d\n\r"),g_settings.smart_beacon);
//	return true;
//}


#endif // end of #if ENABLE_CONSOLE_AT_COMMANDS

#if MOD_BEACON && CFG_BEACON_TEST
/*
 * !{n} - send {n} test packets
 */
static bool cmd_test_send(Serial* pSer, char* command, size_t len){
	#define DEFAULT_REPEATS 3
	uint8_t repeats = 0;
	if(len > 0){
		repeats = atoi((const char*)command);
	}
	if(repeats == 0 || repeats > 99) repeats = DEFAULT_REPEATS;
	SERIAL_PRINTF_P(pSer,PSTR("Sending %d test packet\r\n"),repeats);
	beacon_send_test_message_immediate(repeats,NULL);
	SERIAL_PRINTF_P(pSer,PSTR("Done!\r\n"),repeats);
	return true;
}
#endif


#if MOD_BEACON && CONSOLE_SEND_COMMAND_ENABLED
/*
 * AT+SEND - just send the beacon message once
 */
static bool cmd_send(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	SERIAL_PRINT_P(pSer,PSTR("NOT SUPPRTED YET\r\n"));

//	if(len == 0){
//		// send test message
//		beacon_send_text();
//		SERIAL_PRINT_P(pSer,PSTR("SEND OK\r\n"));
//	}else{
//		//TODO send user input message out, build the ax25 path according settings
//		SERIAL_PRINT_P(pSer,PSTR("NOT SUPPRTED YET\r\n"));
//	}

	return true;
}
#endif

/*
 * Console Initialization Routine
 */
void console_init(){
	memset(cmdEntries,0, sizeof(struct COMMAND_ENTRY) * CONSOLE_MAX_COMMAND);
	cmdCount = 0;

	// Initialize the commands
#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	// settings
    console_add_command(PSTR("CALL"),cmd_settings_mycall);		// setup my callsign-ssid
#if CONSOLE_SETTINGS_COMMAND_DEST_ENABLED
    console_add_command(PSTR("DEST"),cmd_settings_destcall);	// setup destination call-ssid
#endif
    console_add_command(PSTR("PATH"),cmd_settings_path);		// setup path like WIDEn-N for beaco
    console_add_command(PSTR("RESET"),cmd_settings_reset);				// reset the tnc
    console_add_command(PSTR("SYMBOL"),cmd_settings_symbol);	// setup the beacon symbol

    console_add_command(PSTR("BEACON"), cmd_settings_beacon_interval); // setup beacon interval

	#if SETTINGS_SUPPORT_BEACON_TEXT
    console_add_command(PSTR("TEXT"),cmd_settings_beacon_text);
	#endif
#endif

#if CONSOLE_SEND_COMMAND_ENABLED
    console_add_command(PSTR("SEND"),cmd_send);
#endif

	// Initialization done, display the welcome banner and settings info
	cmd_info(&g_serial,0,0);
}

void console_poll(){
	if(serialreader_readline(&g_serialreader) > 0){
		// handle the line
		console_parse_command((char*)g_serialreader.data,g_serialreader.dataLen);
	}
}
