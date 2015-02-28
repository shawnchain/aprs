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

// Instance of the settings data. // TODO - Store default settings in the PROGMEM
SettingsData g_settings = {
		.my_call="NOCALL",
		.my_ssid=0,
		.dest_call="",
		.dest_ssid=0,
		.path1_call="",
		.path1_ssid=0,
		.path2_call="",
		.path2_ssid=0
};

#define NV_SETTINGS_BLOCK_SIZE SETTINGS_SIZE
#define NV_SETTINGS_BEACON_TEXT_SIZE 32

#define NV_SETTINGS_HEAD_BYTE_VALUE 0x88

uint8_t EEMEM nvSetHeadByte;
uint8_t EEMEM nvSettings[NV_SETTINGS_BLOCK_SIZE];
//uint8_t EEMEM nvBeaconText[NV_SETTINGS_BEACON_TEXT_SIZE];

/*
 * Load settings
 */
bool settings_load(void){
	uint8_t verification = eeprom_read_byte((void*)&nvSetHeadByte);
	if (verification != NV_SETTINGS_HEAD_BYTE_VALUE) {
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
	eeprom_update_byte((void*)&nvSetHeadByte, NV_SETTINGS_HEAD_BYTE_VALUE);
	return true;
}

/*
 * Clear settings
 */
void settings_clear(void){
	eeprom_update_byte((void*)&nvSetHeadByte, 0xFF);
}

#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

// The call might be 5 or 6 char length
#define COPY_CALL_VALUE(X) \
		memcpy(valueOut,X,MIN(outBufferSize,6)); \
		while(valueLen<6){ \
			if(((char*)valueOut)[valueLen] == 0){ \
				break; \
			} \
			valueLen++; \
		}; \
		*pValueOutLen = valueLen;
/*
 * Get settings value, fill up the valueOut buffer, and update the valueOutLen of the actual value lenth
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* pValueOutLen){
	if(*pValueOutLen <= 0) return;
	uint8_t outBufferSize = *pValueOutLen;
	uint8_t valueLen = 0;
	switch(type){
		case SETTINGS_MY_CALL:
			COPY_CALL_VALUE(g_settings.my_call);
			break;
		case SETTINGS_MY_SSID:
			*((uint8_t*)valueOut) = g_settings.my_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_DEST_CALL:
			COPY_CALL_VALUE(g_settings.dest_call);
			break;
		case SETTINGS_DEST_SSID:
			*((uint8_t*)valueOut) = g_settings.dest_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_PATH1_CALL:
			COPY_CALL_VALUE(g_settings.path1_call);
			break;
		case SETTINGS_PATH1_SSID:
			*((uint8_t*)valueOut) = g_settings.path1_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_PATH2_CALL:
			COPY_CALL_VALUE(g_settings.path2_call);
			break;
		case SETTINGS_PATH2_SSID:
			*((uint8_t*)valueOut) = g_settings.path2_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_PHGD:
			memcpy(valueOut,g_settings.phgd,MIN(outBufferSize,4));
			*pValueOutLen = 4; // yes they're fixed
			break;
		case SETTINGS_SYMBOL:
			*((uint8_t*)valueOut) = g_settings.symbol;
			*pValueOutLen = 1;
			break;
		default:
			*pValueOutLen = 0;
			break;
	}
}

/*
 * Set settings value
 */
void settings_set(SETTINGS_TYPE type, void* value, uint8_t valueLen){
	switch(type){
		case SETTINGS_MY_CALL:
			memset(g_settings.my_call,0,6);
			memcpy(g_settings.my_call,value,MIN(6,valueLen));
			break;
		case SETTINGS_MY_SSID:
			g_settings.my_ssid = *((uint8_t*)value);
			break;
		case SETTINGS_DEST_CALL:
			memset(g_settings.dest_call,0,6);
			memcpy(g_settings.dest_call,value,MIN(6,valueLen));
			break;
		case SETTINGS_DEST_SSID:
			g_settings.dest_ssid = *((uint8_t*)value);
			break;
		case SETTINGS_PATH1_CALL:
			memset(g_settings.path1_call,0,6);
			memcpy(g_settings.path1_call,value,MIN(6,valueLen));
			break;
		case SETTINGS_PATH1_SSID:
			g_settings.path1_ssid = *((uint8_t*)value);
			break;
		case SETTINGS_PATH2_CALL:
			memset(g_settings.path2_call,0,6);
			memcpy(g_settings.path2_call,value,MIN(6,valueLen));
			break;
		case SETTINGS_PATH2_SSID:
			g_settings.path2_ssid = *((uint8_t*)value);
			break;
		case SETTINGS_PHGD:
			memset(g_settings.phgd,0,4);
			memcpy(g_settings.phgd,value,MIN(4,valueLen));
			break;
		case SETTINGS_SYMBOL:
			g_settings.symbol = *((uint8_t*)value);
			break;

		default:
			break;
	}
}

/*
 * This method will accept like BG5HHP, BG5HHP-99 and BG5HHP-X. but will not accept "BG5HHP-" or BG5HHP-100
 */
bool settings_set_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* callString, uint8_t callStringLen){
	const char s[] = "-";
	char *call = NULL;
	uint8_t ssid=0, callLen = 0;
	if(callStringLen > 0 && callStringLen < 10){
		// split the "call-ssid" string
		char* t = strtok((callString),s);
		if(t != NULL){
			call = t;
			callLen = strlen(t);
			t = strtok(NULL,s);
			if(t){
				ssid = atoi((const char*)t);
				if(ssid > SETTINGS_MAX_SSID){
					return false; // bail out as ssid is invalid
				}
				settings_set(ssidType,&ssid,1);
			}
			settings_set(callType,call,callLen);
			return true;
		}
	}
	return false;
}

void settings_get_call_fullstring(SETTINGS_TYPE callType, SETTINGS_TYPE ssidType, char* buf, uint8_t bufLen){
	// read the call and ssid
	memset(buf,0,bufLen);
	uint8_t call_len = bufLen - 4;
	settings_get(callType,buf,&call_len);
	if(call_len > 0){
		// read ssid only when buffer is available
		uint8_t ssid = 0, ssid_len = 1;
		settings_get(ssidType,&ssid,&ssid_len);
		if(true/*ssid > 0*/){
			buf[call_len++] = '-';
			itoa(ssid,(char*)(buf + call_len),10);
		}
	}

}
