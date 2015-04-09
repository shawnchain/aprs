#define ENABLE_HELP true

#include "console.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "settings.h"

#include <net/ax25.h>
#include "beacon.h"

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

#if CONSOLE_SEND_COMMAND_ENABLED
static bool cmd_send(Serial* pSer, char* command, size_t len);
#endif

#if CONSOLE_TEST_COMMAND_ENABLED
static bool cmd_test(Serial* pSer, char* command, size_t len);
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
#if CONSOLE_TEST_COMMAND_ENABLED
	if(commandLen > 0 && command[0] == '!'){
		cmd_test(pSer, command+1,commandLen - 1);
		return;
	}
#endif

	if(commandLen >0 && command[0] == '?'){
		cmd_info(pSer,0,0);
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
		//cmd_info(pSer,0,0);
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

// Free ram test
INLINE uint16_t freeRam (void) {
  extern int __heap_start, *__brkval;
  uint8_t v;
  uint16_t vaddr = (uint16_t)(&v);
  return (uint16_t) (vaddr - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}

static bool cmd_info(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;

	// print welcome banner
	SERIAL_PRINTF_P(pSer, PSTR("\r\nTinyAPRS (KISS-TNC/GPS-Beacon) 1.1-SNAPSHOT (f%da%d-%d)\r\n"),CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);

	// print settings
	{
	char buf[16];
	uint8_t bufLen = sizeof(buf);
	settings_get_call_fullstring(SETTINGS_MY_CALL,SETTINGS_MY_SSID,buf,bufLen);
	SERIAL_PRINTF_P(pSer, PSTR("MyCall: %s\r\n"),buf);
	}

	SERIAL_PRINTF_P(pSer, PSTR("Mode: %d\r\n"),g_settings.run_mode);

	// print the ax25 stat
#if CONFIG_AX25_STAT
	SERIAL_PRINTF_P(pSer, PSTR("RX:%d, TX:%d, ERR: %d\r\n"),g_ax25.stat.rx_ok,g_ax25.stat.tx_ok,g_ax25.stat.rx_err);
#endif

	// print free memory
	SERIAL_PRINTF_P(pSer,PSTR("Free RAM: %u\r\n"),freeRam());

	return true;
}

#if CONSOLE_HELP_COMMAND_ENABLED
static bool cmd_help(Serial* pSer, char* command, size_t len){
	(void)command;
	(void)len;
	SERIAL_PRINT_P(pSer,PSTR("\r\nAT commands supported\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("-----------------------------------------------------------\r\n"));
#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	SERIAL_PRINT_P(pSer,PSTR("AT+MYCALL=[CALLSIGN-SSID]\t;Set my callsign\r\n"));
	//SERIAL_PRINT_P(pSer,PSTR("AT+MYSSID=[SSID]\t\t;Set my ssid only\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+DEST=[CALLSIGN-SSID]\t\t;Set destination callsign\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+PATH=[WIDE1-1,WIDE2-2]\t;Set PATH, max 2 allowed\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+SYMBOL=[SYMBOL_TABLE/IDX]\t;Set beacon symbol\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+RAW=[!3011.54N/12007.35E>..]\t;Set beacon raw text \r\n"));
#endif
	SERIAL_PRINT_P(pSer,PSTR("AT+MODE=[0|1|2]\t\t\t;Set device run mode\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+KISS=[1]\t\t\t;Enter kiss mode\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("?\t\t\t\t;Display help messages\r\n"));

	SERIAL_PRINT_P(pSer,  PSTR("\r\nCopyright 2015, BG5HHP(shawn.chain@gmail.com)\r\n\r\n"));

	return true;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// AT SETTINGS COMMAND SUPPORT
#if CONSOLE_SETTINGS_COMMANDS_ENABLED

static bool cmd_settings_myssid(Serial* pSer, char* value, size_t len){
	if(len > 0){
		uint8_t ssid = atoi((const char*)value);
		if(ssid > SETTINGS_MAX_SSID){
			return false;
		}
		settings_set(SETTINGS_MY_SSID,&ssid,1);
		settings_save();
	}

	uint8_t iSSID = 0;
	uint8_t bufLen = 1;
	settings_get(SETTINGS_MY_SSID,&iSSID,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("MY_SSID: %d\r\n"),iSSID);

	return true;
}

#define _TEMPLATE_SET_SETTINGS_CALL_SSID_(NAME,CALL_TYPE,SSID_TYPE,VALUE,LEN) \
	if(LEN > 0){ \
		strupr(VALUE); \
		if(!settings_set_call_fullstring(CALL_TYPE,SSID_TYPE,VALUE,LEN)){ \
			return false; \
		} \
		settings_save(); \
	} \
	char buf[16]; \
	uint8_t bufLen = sizeof(buf); \
	settings_get_call_fullstring(CALL_TYPE,SSID_TYPE,buf,bufLen); \
	SERIAL_PRINTF_P(pSer,PSTR( #NAME ": %s\r\n"),buf);

/*
 * parse AT+MYCALL=CALL-ID
 */
static bool cmd_settings_mycall(Serial* pSer, char* value, size_t valueLen){
	_TEMPLATE_SET_SETTINGS_CALL_SSID_(MYCALL,SETTINGS_MY_CALL,SETTINGS_MY_SSID,value,valueLen);
	return true;
}

/*
 * parse AT+DEST=CALL-ID
 */
static bool cmd_settings_destcall(Serial* pSer, char* value, size_t valueLen){
	_TEMPLATE_SET_SETTINGS_CALL_SSID_(DEST,SETTINGS_DEST_CALL,SETTINGS_DEST_SSID,value,valueLen);
	return true;
}

/*
 * parse AT+PATH=PATH1,PATH2
 */
static bool cmd_settings_path(Serial* pSer, char* value, size_t valueLen){
	if(valueLen > 0){
		// convert to upper case
		strupr(value);
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
				if(!settings_set_call_fullstring(SETTINGS_PATH1_CALL,SETTINGS_PATH1_SSID,path1,path1Len)){
					return false;
				}
				if(path2Len > 0){
					if(!settings_set_call_fullstring(SETTINGS_PATH2_CALL,SETTINGS_PATH2_SSID,path2,path2Len)){
						return false;
					}
				}
				settings_save();
			}
		}
	}

	// display the path
	char p1[10],p2[10];
	uint8_t len = 10;
	settings_get_call_fullstring(SETTINGS_PATH1_CALL,SETTINGS_PATH1_SSID,p1,len);
	if(strlen(p1) > 0){
		settings_get_call_fullstring(SETTINGS_PATH2_CALL,SETTINGS_PATH2_SSID,p2,len);
		if(strlen(p2) > 0){
			SERIAL_PRINTF_P(pSer,PSTR("PATH: %s,%s\r\n"),p1,p2);
		}else{
			SERIAL_PRINTF_P(pSer,PSTR("PATH: %s\r\n"),p1);
		}
	}else{
		SERIAL_PRINT_P(pSer,PSTR("PATH:\r\n"));
	}
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
		settings_set(SETTINGS_SYMBOL,value,2);
		settings_save();
	}

	uint8_t symbol[2];
	uint8_t bufLen = 2;
	settings_get(SETTINGS_SYMBOL,symbol,&bufLen);
	SERIAL_PRINTF_P(pSer,PSTR("SYMBOL: %c%c\r\n"),symbol[0],symbol[1]);

	return true;
}

#if SETTINGS_SUPPORT_BEACON_TEXT
static bool cmd_settings_raw_packet(Serial* pSer, char* value, size_t valueLen){
	if(valueLen > 0){
		// set the beacon text
		settings_set_raw_packet(value,valueLen);
		//SERIAL_PRINT_P(pSer, "OK\n\r");
		//return true;
	}

#define DEBUG_BEACON_TEXT 1
#if DEBUG_BEACON_TEXT
	#define BUF_LEN SETTINGS_BEACON_TEXT_MAX + 4
	char buf[BUF_LEN];
	buf[0] = '>';
	uint8_t bytesRead = settings_get_raw_packet(buf+1,BUF_LEN - 4);
	buf[bytesRead + 1] = '\n';
	buf[bytesRead+2] = '\r';
	buf[bytesRead+3] = 0;
	kfile_print((&(pSer->fd)),buf);

	uint16_t free = freeRam();
	SERIAL_PRINTF_P(pSer, PSTR("free mem: %d\n\r"),free)
#endif
	return true;
}
#endif


/*
 * AT+SB=1 enable/disable smart beacon
 */
static bool cmd_settings_smartbeacon(Serial* pSer, char* value, size_t valueLen){
	(void)pSer;
	if(valueLen == 1){
		if(value[0] == '0' && g_settings.smart_beacon == 1){
			g_settings.smart_beacon = 0;
			settings_save();
		}else if(value[0] == '1' && g_settings.smart_beacon == 0){
			g_settings.smart_beacon = 1;
			settings_save();
		}
	}
	SERIAL_PRINTF_P(pSer, PSTR("smart beacon: %d\n\r"),g_settings.smart_beacon);
	return true;
}


#endif // end of #if ENABLE_CONSOLE_AT_COMMANDS

#if CONSOLE_TEST_COMMAND_ENABLED
/*
 * !{n} - send {n} test packets
 */
static bool cmd_test(Serial* pSer, char* command, size_t len){
	#define DEFAULT_REPEATS 3
	uint8_t repeats = 0;
	if(len > 0){
		repeats = atoi((const char*)command);
	}
	if(repeats == 0 || repeats > 99) repeats = DEFAULT_REPEATS;
	beacon_set_repeats(repeats);

	SERIAL_PRINTF_P(pSer,PSTR("Sending %d test packet...\r\n"),repeats);
	return true;
}
#endif



#if CONSOLE_SEND_COMMAND_ENABLED
/*
 * AT+SEND - just send the beacon message once
 */
static bool cmd_send(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	if(len == 0){
		// send test message
		beacon_send_fixed();
		SERIAL_PRINT_P(pSer,PSTR("SEND OK\r\n"));
	}else{
		//TODO send user input message out, build the ax25 path according settings
		SERIAL_PRINT_P(pSer,PSTR("NOT SUPPRTED YET\r\n"));
	}

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
    console_add_command(PSTR("MYCALL"),cmd_settings_mycall);	// setup my callsign-ssid
    console_add_command(PSTR("MYSSID"),cmd_settings_myssid);	// setup callsign-ssid
    console_add_command(PSTR("DEST"),cmd_settings_destcall);		// setup destination call-ssid
    console_add_command(PSTR("PATH"),cmd_settings_path);		// setup path like WIDEn-N for beaco
    console_add_command(PSTR("RESET"),cmd_settings_reset);				// reset the tnc
    console_add_command(PSTR("SYMBOL"),cmd_settings_symbol);	// setup the beacon symbol

    console_add_command(PSTR("SB"),cmd_settings_smartbeacon);	// setup the beacon symbol

	#if SETTINGS_SUPPORT_BEACON_TEXT
    console_add_command(PSTR("RAW"),cmd_settings_raw_packet);
	#endif
#endif

#if CONSOLE_TEST_COMMAND_ENABLED
    console_add_command(PSTR("TEST"),cmd_test);
#endif

#if CONSOLE_SEND_COMMAND_ENABLED
    console_add_command(PSTR("SEND"),cmd_send);
#endif

	// Initialization done, display the welcome banner and settings info
	cmd_info(&g_serial,0,0);
}
