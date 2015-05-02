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


#define SETTINGS_SUPPORT_BEACON_TEXT 1

/*
 * The setting types
 */
typedef enum {
	SETTINGS_MY_CALL = 1,
	SETTINGS_DEST_CALL,
	SETTINGS_PATH1_CALL,
	SETTINGS_PATH2_CALL,
	SETTINGS_SYMBOL,
	SETTINGS_RUN_MODE,
	SETTINGS_BEACON_INTERVAL,
}SETTINGS_TYPE;

#define SETTINGS_MAX_SSID 99
#define SETTINGS_SIZE 8

#define SETTINGS_BEACON_TEXT_MAX 128

typedef struct{
	uint8_t symbol[2];		// Symbol table and the index

	uint8_t run_mode;		// the run mode ,could be 0|1|2

	uint16_t beacon_interval; // Beacon send interval

	uint8_t beacon_type;	// 0 = smart, 1 = fixed interval

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
 * get the beacon text
 */
uint8_t settings_get_beacon_text(char* buf, uint8_t bufLen);

/*
 * set the beacon text
 */
uint8_t settings_set_beacon_text(char* data, uint8_t dataLen);

struct AX25Call;
/*
 * Get call object from settings
 */
void settings_get_call(SETTINGS_TYPE callType, struct AX25Call *call);

/*
 * Set call object from settings
 */
void settings_set_call(SETTINGS_TYPE callType, struct AX25Call *call);

#endif /* SETTINGS_H_ */
