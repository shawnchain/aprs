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
#define SETTINGS_BEACON_TEXT_MAX_LEN 128

/*
 * The settings types
 */
typedef enum{
	SETTINGS_CALL = 1,
	SETTINGS_TEXT,
	SETTINGS_PARAMS,
	SETTINGS_COMMIT = 15
}SettingsType;

/*
 * The settings parameter keys
 */
typedef enum {
	SETTINGS_MY_CALL = 1,
	SETTINGS_DEST_CALL,
	SETTINGS_PATH1_CALL,
	SETTINGS_PATH2_CALL,
	SETTINGS_SYMBOL,
	SETTINGS_RUN_MODE,
	SETTINGS_BEACON_INTERVAL,
}SettingsParamKey;

typedef struct BeaconParams{
	uint8_t		symbol[2];		// Symbol table and the index
	uint16_t	interval; 		// Beacon send interval
	uint8_t		type;			// 0 = smart, 1 = fixed interval
}BeaconParams;

typedef struct RfParams{
	uint8_t txdelay;
	uint8_t txtail;
	uint8_t persistence;
	uint8_t slot_time;
	uint8_t duplex;
}RfParams;

typedef struct{
	uint8_t run_mode;		// the run mode ,could be 0|1|2
	BeaconParams beacon;	// the beacon parameters
	RfParams rf;			// the rf parameters
} SettingsData;


enum {
	RF_DUPLEX_HALF = 0,
	RF_DUPLEX_FULL
};

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
void settings_get_params(SettingsParamKey type, void* valueOut, uint8_t* valueOutLen);

/**
 * Set value of a specific setting
 */
void settings_set_params(SettingsParamKey type, void* value, uint8_t valueLen);

/**
 * Set/copy raw bytes into settingsData memory.
 */
bool settings_set_params_bytes(uint8_t *bytes, uint16_t size);

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
void settings_get_call(SettingsParamKey callType, struct AX25Call *call);

/*
 * Set call object from settings
 */
void settings_set_call(SettingsParamKey callType, struct AX25Call *call);

#endif /* SETTINGS_H_ */
