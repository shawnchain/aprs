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

#include "settings.h"

// TODO - Use PROGMEM to store default settings
SettingsData g_settings = {.callsign="NOCALL",.ssid=0};

#define NV_SETTINGS_BLOCK_SIZE 32
#define NV_SETTINGS_MAGIC_BYTE 0x88
uint8_t EEMEM nvSetMagicByte;
uint8_t EEMEM nvSettings[NV_SETTINGS_BLOCK_SIZE];


/*
 * Load settings
 */
bool settings_load(void){
	uint8_t verification = eeprom_read_byte((void*)&nvSetMagicByte);
	if (verification != NV_SETTINGS_MAGIC_BYTE) {
		// fill up zero values
		return false;
	}
	eeprom_read_block((void*)&g_settings, (void*)nvSettings, NV_SETTINGS_BLOCK_SIZE);
	return true;
}

/*
 * Save settings
 */
bool settings_save(void){
	eeprom_update_block((void*)&g_settings, (void*)nvSettings, NV_SETTINGS_BLOCK_SIZE);
	eeprom_update_byte((void*)&nvSetMagicByte, NV_SETTINGS_MAGIC_BYTE);
	return true;
}

/*
 * Clear settings
 */
void settings_clear(void){
	eeprom_update_byte((void*)&nvSetMagicByte, 0xFF);
}

#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

/*
 * Get settings value, fill up the valueOut buffer, and update the valueOutLen of the actual value lenth
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* pValueOutLen){
	if(*pValueOutLen <= 0) return;
	uint8_t outBufferSize = *pValueOutLen;

	switch(type){
		case SETTINGS_CALLSIGN:
			memcpy(valueOut,g_settings.callsign,MIN(outBufferSize,6));
			*pValueOutLen = 6;
			break;
		case SETTINGS_SSID:
			*((uint8_t*)valueOut) = g_settings.ssid;
			*pValueOutLen = 1;
			break;
		default:
			*pValueOutLen = 0;
			break;
	}
}

/*
 *
 */
void settings_set(SETTINGS_TYPE type, void* value, uint8_t valueLen){
	switch(type){
		case SETTINGS_CALLSIGN:
			memset(g_settings.callsign,0,6);
			memcpy(g_settings.callsign,value,MIN(6,valueLen));
			break;
		case SETTINGS_SSID:
			g_settings.ssid = *((uint8_t*)value);
			break;
		default:
			break;
	}
}
