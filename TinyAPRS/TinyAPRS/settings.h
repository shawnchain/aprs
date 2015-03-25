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
}SETTINGS_TYPE;

#define SETTINGS_MAX_SSID 99
#define SETTINGS_COMMENTS_TEXT_MAX 48
#define SETTINGS_SIZE 76

#define SETTINGS_RAW_PACKET_MAX 96

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

	uint8_t unused[2];
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
 * Clear all settings
 */
void settings_clear(void);



/**
 * Get value of a specific settings
 * @type the type of setting to get
 * @valueOut out buffer to store the settings
 * @valueOutLen max out buffer len and will be set to the actual value length when function returned.
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* valueOutLen);
/**
 * Set value of a specific setting
 */
void settings_set(SETTINGS_TYPE type, void* value, uint8_t valueLen);


/*
 * Accept CALL string like "BG5HHP-1" and update specific setting values
 */
bool settings_set_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* callString, uint8_t callStringLen);

/*
 * Get specific CALL setting values and format to a string like "BG5HHP-1"
 */
void settings_get_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* buf, uint8_t bufLen);


/*
 * Get location like "DDMM.mmN/DDDMM.mmE" and update specific setting values.
 */
void settings_get_location_string(char* buf, uint8_t bufLen);

void settings_set_location_string(char* buf, uint8_t bufLen);


uint8_t settings_get_raw_packet(char* buf, uint8_t bufLen);

uint8_t settings_set_raw_packet(char* data, uint8_t dataLen);
#endif /* SETTINGS_H_ */
