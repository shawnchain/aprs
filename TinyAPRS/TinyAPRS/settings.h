/**
 * \file
 * <!--
 * This file is part of TinyAPRS.
 * Released under GPL License
 *
 * Copyright 2015 Shawn Chain (shawn.chain@gmail.com)
 *
 * -->
 *
 * \brief The modem settings routines
 *
 * \author Shawn Chain <shawn.chain@gmail.com>
 * \date 2015-02-07
 */


#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>


#define SETTINGS_SUPPORT_RAW_PACKET 0

/*
 * The setting types
 */
typedef enum {
	SETTINGS_MY_CALL = 1,
	SETTINGS_MY_SSID,
	SETTINGS_DEST_CALL,
	SETTINGS_DEST_SSID,
	SETTINGS_PATH1_CALL,
	SETTINGS_PATH1_SSID,
	SETTINGS_PATH2_CALL,
	SETTINGS_PATH2_SSID,

	SETTINGS_PHGD,
	SETTINGS_SYMBOL,
	SETTINGS_LOCATION,
	SETTINGS_COMMENTS_TEXT,

#if SETTINGS_SUPPORT_RAW_PACKET
	SETTINGS_RAW_PACKET_TEXT,
#endif

}SETTINGS_TYPE;

#define SETTINGS_MAX_SSID 99
#define SETTINGS_COMMENTS_TEXT_MAX 48

#if SETTINGS_SUPPORT_RAW_PACKET
	#define SETTINGS_RAW_PACKET_TEXT_MAX 96
	#define SETTINGS_SIZE 172
#else
	#define SETTINGS_SIZE 76
#endif

typedef struct{
	uint8_t my_call[6]; 	// the call sign like BG5HHP
	uint8_t my_ssid;		// the call ssid from 1-10

	uint8_t dest_call[6];	// the destination callsign and ssid
	uint8_t dest_ssid;

	uint8_t path1_call[6];	// the path1 callsign and ssid
	uint8_t path1_ssid;

	uint8_t path2_call[6];	// the path2
	uint8_t path2_ssid;

	uint8_t phgd[4];		// the POWER,HEIGHT,GAIN,DIRECTIVITY index values
	uint8_t symbol;			// Symbols
	uint8_t symbol_table;
	uint8_t location[8];	// [30,14,00,'N',120,09,00,'E'] - "3014.00N,12009.00E"

	uint8_t comments[SETTINGS_COMMENTS_TEXT_MAX];

#if SETTINGS_SUPPORT_RAW_PACKET
	uint8_t raw_packet_text[SETTINGS_RAW_PACKET_TEXT_MAX]; //!3014.00N/12009.00E>000/000/A=000087Rolling! 3.6V 1011.0pa
#endif
	uint8_t unused[2];
	/*
#define NV_MAGIC_BYTE 0x69
uint8_t EEMEM nvMagicByte;
uint8_t EEMEM nvCALL[6];
uint8_t EEMEM nvDST[6];
uint8_t EEMEM nvPATH1[6];
uint8_t EEMEM nvPATH2[6];
uint8_t EEMEM nvCALL_SSID;
uint8_t EEMEM nvDST_SSID;
uint8_t EEMEM nvPATH1_SSID;
uint8_t EEMEM nvPATH2_SSID;
bool EEMEM nvPRINT_SRC;
bool EEMEM nvPRINT_DST;
bool EEMEM nvPRINT_PATH;
bool EEMEM nvPRINT_DATA;
bool EEMEM nvPRINT_INFO;
bool EEMEM nvVERBOSE;
bool EEMEM nvSILENT;
uint8_t EEMEM nvPOWER;
uint8_t EEMEM nvHEIGHT;
uint8_t EEMEM nvGAIN;
uint8_t EEMEM nvDIRECTIVITY;
uint8_t EEMEM nvSYMBOL_TABLE;
uint8_t EEMEM nvSYMBOL;
uint8_t EEMEM nvAUTOACK;
*/
} SettingsData;



extern SettingsData g_settings;

/**
 * Load settings from EEPROM
 */
bool settings_load(void);

/**
 * Save settings to EEPROM
 */
bool settings_save(void);

/**
 *
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* valueOutLen);

/*
 *
 */
bool settings_set_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* callString, uint8_t callStringLen);

/*
 *
 */
void settings_get_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* buf, uint8_t bufLen);


/*
 *
 */
void settings_get_location_string(char* buf, uint8_t bufLen);

/**
 * Clear settings
 */
void settings_clear(void);

/**
 * Set value of a specific setting
 */
void settings_set(SETTINGS_TYPE type, void* value, uint8_t valueLen);

#endif /* SETTINGS_H_ */
