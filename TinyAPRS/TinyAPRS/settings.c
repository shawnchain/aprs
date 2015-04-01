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
		.my_call="N0CALL",
		.my_ssid=0,
		.dest_call="APTI01",
		.dest_ssid=0,
		.path1_call="WIDE1",
		.path1_ssid=1,
		.path2_call="",
		.path2_ssid=0,
		.symbol="/>",
		//.location={30,14,0,'N',120,0,9,'E'},
		//.phgd={0,0,0,0},
		//.comments="TinyAPRS Rocks!",
		.smart_beacon=0,
};

#define NV_SETTINGS_HEAD_BYTE_VALUE 0x88
#define NV_SETTINGS_BLOCK_SIZE SETTINGS_SIZE
uint8_t EEMEM nvSetHeadByte;
uint8_t EEMEM nvSettings[NV_SETTINGS_BLOCK_SIZE];

// TODO - support raw packet
#define NV_RAW_PACKET_HEAD_BYTE_VALUE 0x99
#define NV_RAW_PACKET_BLOCK_SIZE SETTINGS_BEACON_TEXT_MAX
uint8_t EEMEM nvRawPacketHeadByte;
uint8_t EEMEM nvRawPacket[NV_RAW_PACKET_BLOCK_SIZE];

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
	eeprom_update_byte((void*)&nvRawPacketHeadByte, 0xFF);
}

#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

static void settings_copy_call_value(const char* call, char* buf, uint8_t *len){
	memset(buf, 0, 7);
	int i = 0;
	while (i < 6 && call[i] != 0) {
		buf[i] = call[i];
		i++;
	}
	*len = i;
}

/*
 * Get settings value, fill up the valueOut buffer, and update the valueOutLen of the actual value lenth
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* pValueOutLen){
	if(*pValueOutLen <= 0) return;
	switch(type){
		case SETTINGS_MY_CALL:
			settings_copy_call_value((const char*)g_settings.my_call,valueOut,pValueOutLen);
			break;
		case SETTINGS_MY_SSID:
			*((uint8_t*)valueOut) = g_settings.my_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_DEST_CALL:
			settings_copy_call_value((const char*)g_settings.dest_call,valueOut,pValueOutLen);
			break;
		case SETTINGS_DEST_SSID:
			*((uint8_t*)valueOut) = g_settings.dest_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_PATH1_CALL:
			settings_copy_call_value((const char*)g_settings.path1_call,valueOut,pValueOutLen);
			break;
		case SETTINGS_PATH1_SSID:
			*((uint8_t*)valueOut) = g_settings.path1_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_PATH2_CALL:
			settings_copy_call_value((const char*)g_settings.path2_call,valueOut,pValueOutLen);
			break;
		case SETTINGS_PATH2_SSID:
			*((uint8_t*)valueOut) = g_settings.path2_ssid;
			*pValueOutLen = 1;
			break;
		case SETTINGS_SYMBOL:
			*((uint8_t*)valueOut) = g_settings.symbol[0];
			*((uint8_t*)valueOut + 1) = g_settings.symbol[1];
			*pValueOutLen = 2;
			break;
		case SETTINGS_RUN_MODE:
			*((uint8_t*)valueOut) = g_settings.run_mode;
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
		case SETTINGS_SYMBOL:
			g_settings.symbol[0] = *((uint8_t*)value);
			g_settings.symbol[1] = *(((uint8_t*)value)+1);
			break;
		case SETTINGS_RUN_MODE:
			g_settings.run_mode = *((uint8_t*)value);
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
		if(ssid > 0){
			buf[call_len++] = '-';
			itoa(ssid,(char*)(buf + call_len),10);
		}
	}
}

//.raw_packet_text="!3014.00N/12009.00E>000/000/A=000087TinyAPRS Rocks!",
uint8_t settings_get_raw_packet(char* buf, uint8_t bufLen){
	uint8_t verification = eeprom_read_byte((void*)&nvRawPacketHeadByte);
	if (verification != NV_RAW_PACKET_HEAD_BYTE_VALUE) {
		buf[0] = 0;
		return 0;
	}
	uint8_t bytesToRead = MIN(bufLen,SETTINGS_BEACON_TEXT_MAX);
	eeprom_read_block((void*)buf, (void*)nvRawPacket, bytesToRead);

	// like strlen
	uint8_t i = 0;
	while(buf[i] != 0 && i < bytesToRead){
		i++;
	}
	return i;
}

uint8_t settings_set_raw_packet(char* data, uint8_t dataLen){
	uint8_t bytesToWrite = MIN(dataLen,SETTINGS_BEACON_TEXT_MAX - 1);
	eeprom_update_block((void*)data, (void*)nvRawPacket, bytesToWrite);
	eeprom_update_byte((void*)(nvRawPacket + bytesToWrite), 0);
	eeprom_update_byte((void*)&nvRawPacketHeadByte, NV_RAW_PACKET_HEAD_BYTE_VALUE);
	return bytesToWrite;
}
