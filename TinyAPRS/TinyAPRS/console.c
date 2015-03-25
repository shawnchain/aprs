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
#include "buildrev.h"
#include "config.h"

// Internal console command prototype
static void console_parse_command(Serial *pSer, char* command, size_t commandLen);
static void console_init_command(void);

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

static Serial *pSerial;

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

	console_init_command();

	// Initialization done, display the welcome banner and settings info
	cmd_info(ser,0,0);
}

// Increase the console read buffer for the raw package text input
#if SETTINGS_SUPPORT_RAW_PACKET
#undef CONSOLE_SERIAL_BUF_LEN
#define CONSOLE_SERIAL_BUF_LEN 128
#endif

/*
 * The console will always read console input char until met CRLF or buffer is full
 */
void console_parse(int c){
	//FIXME check memory usage
	static uint8_t serialBuffer[CONSOLE_SERIAL_BUF_LEN+1]; 	// Buffer for holding incoming serial data
	static size_t serialLen = 0;                    		// Counter for counting length of data from serial

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
	SERIAL_PRINTF_P(pSer, PSTR("\r\nTinyAPRS TNC (KISS) 1.1-SNAPSHOT (f%da%d-%d)\r\n"),CONFIG_AFSK_FILTER,CONFIG_AFSK_ADC_USE_EXTERNAL_AREF,VERS_BUILD);

	// print settings
	char buf[16];
	uint8_t bufLen = sizeof(buf);
	settings_get_call_fullstring(SETTINGS_MY_CALL,SETTINGS_MY_SSID,buf,bufLen);
	SERIAL_PRINTF_P(pSerial, PSTR("MyCall: %s\r\n"),buf);

	// print free memory
	SERIAL_PRINTF_P(pSer,PSTR("Free RAM: %u\r\n"),freeRam());

	return true;
}

#if CONSOLE_HELP_COMMAND_ENABLED
static bool cmd_help(Serial* pSer, char* command, size_t len){
	(void)command;
	(void)len;
	SERIAL_PRINT_P(pSer,PSTR("\r\nAT commands supported\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("-----------------------------------------------\r\n"));
#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	SERIAL_PRINT_P(pSer,PSTR("AT+MYCALL=[CALLSIGN]-[SSID]\t;Set my callsign\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+MYSSID=[SSID]\t\t;Set my ssid only\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+DEST=[CALLSIGN]-[SSID]\t;Set destination callsign only\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+PATH=[PATH1],[PATH2]\t\t;Set PATH, max 2 allowed\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+LOCA=[DDMM.mmN/DDDMM.mmE]\t;Set location \r\n"));
	SERIAL_PRINT_P(pSer,PSTR("AT+CMNTS=[BEACON COMMENTS]\t;Set beacon comments \r\n"));
#endif

	SERIAL_PRINT_P(pSer,PSTR("AT+KISS=1\t\t\t;Enter kiss mode\r\n"));
	SERIAL_PRINT_P(pSer,PSTR("?\t\t\t\t;Display help messages\r\n"));

	SERIAL_PRINT_P(pSer,  PSTR("\r\nCopyrights 2015, BG5HHP(shawn.chain@gmail.com)\r\n\r\n"));

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
	(void)value;
	(void)len;

	SERIAL_PRINT_P(pSer, PSTR("Not implemented yet"));
	return true;
}

static bool cmd_settings_location(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	//TODO - parse input and store back
	//3014.00N,12009.00E

	char buf[32];
	uint8_t bufLen = sizeof(buf);
	settings_get_location_string(buf,bufLen);
	SERIAL_PRINTF_P(pSer, PSTR("Location:%s\r\n"),buf);
	return true;
}

static bool cmd_settings_comments_text(Serial* pSer, char* value, size_t valueLen){
	if(valueLen > 0){
		// set the beacon text
		settings_set(SETTINGS_COMMENTS_TEXT,value,valueLen);
		settings_save();
	}

	char buf[SETTINGS_COMMENTS_TEXT_MAX + 3];
	uint8_t bufLen = SETTINGS_COMMENTS_TEXT_MAX - 1;
	buf[0] = '>';
	settings_get(SETTINGS_COMMENTS_TEXT,buf + 1,&bufLen);
	buf[bufLen++] = '\r';
	buf[bufLen++] = '\n';
	buf[bufLen] = 0;
	kfile_print((&(pSer->fd)),buf);
	return true;
}

#if SETTINGS_SUPPORT_RAW_PACKET
static bool cmd_settings_raw_packet(Serial* pSer, char* value, size_t valueLen){
	if(valueLen > 0){
		// set the beacon text
		settings_set(SETTINGS_RAW_PACKET_TEXT,value,valueLen);
		settings_save();
	}

	char buf[SETTINGS_RAW_PACKET_TEXT_MAX + 3];
	uint8_t bufLen = SETTINGS_RAW_PACKET_TEXT_MAX - 1;
	buf[0] = '>';
	settings_get(SETTINGS_RAW_PACKET_TEXT,buf+1,&bufLen);
	kfile_print((&(pSer->fd)),buf);
	buf[bufLen] = '\r';
	kfile_print((&(pSer->fd)),buf);
	return true;
}
#endif

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
	if(repeats == 0 || repeats > 9) repeats = DEFAULT_REPEATS;
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
		beacon_send();
		SERIAL_PRINT_P(pSer,PSTR("SEND OK\r\n"));
	}else{
		//TODO send user input message out, build the ax25 path according settings
		SERIAL_PRINT_P(pSer,PSTR("NOT SUPPRTED YET\r\n"));
	}

	return true;
}
#endif

static void console_init_command(void){

#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	// settings
    console_add_command(PSTR("MYCALL"),cmd_settings_mycall);	// setup my callsign-ssid
    console_add_command(PSTR("MYSSID"),cmd_settings_myssid);	// setup callsign-ssid
    console_add_command(PSTR("DEST"),cmd_settings_destcall);		// setup destination call-ssid
    console_add_command(PSTR("PATH"),cmd_settings_path);		// setup path like WIDEn-N for beaco
    console_add_command(PSTR("RESET"),cmd_settings_reset);				// reset the tnc

    console_add_command(PSTR("SYMBL"),cmd_settings_symbol);	// setup the beacon symbol
    console_add_command(PSTR("LOCA"),cmd_settings_location);	// setup the fixed location
    console_add_command(PSTR("CMNTS"),cmd_settings_comments_text);	// setup the beacon comment

	#if SETTINGS_SUPPORT_RAW_PACKET
    console_add_command(PSTR("RAW"),cmd_settings_raw_packet);
	#endif
#endif

#if CONSOLE_TEST_COMMAND_ENABLED
    console_add_command(PSTR("TEST"),cmd_test);
#endif

#if CONSOLE_SEND_COMMAND_ENABLED
    console_add_command(PSTR("SEND"),cmd_send);
#endif
}
