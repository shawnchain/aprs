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

#include <cpu/irq.h>
#include <net/ax25.h>
#include "utils.h"

#define DEFAULT_BEACON_INTERVAL 20 * 60 // 20 minutes of beacon send interval
#define DEFAULT_BEACON_TEXT "!3014.00N/12009.00E>TinyAPRS Rocks!" //

/*
 * Helper macros
 */
#undef ABS
#undef MIN
#undef MAX
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
//#include <cfg/macros.h>

// Instance of the settings data. // TODO - Store default settings in the PROGMEM
SettingsData g_settings = {
		.symbol="/>",
		//.location={30,14,0,'N',120,0,9,'E'},
		//.phgd={0,0,0,0},
		//.comments="TinyAPRS Rocks!",
		.beacon_interval = 0, // by default beacon is disabled;
		.beacon_type=0, // 0 = smart, 1 = fixed interval
};

#define NV_SETTINGS_HEAD_BYTE_VALUE 0x88
#define NV_SETTINGS_BLOCK_SIZE SETTINGS_SIZE
uint8_t EEMEM nvSetHeadByte;
uint8_t EEMEM nvSettings[NV_SETTINGS_BLOCK_SIZE];
uint8_t EEMEM nvSetCrcByte;

uint8_t EEMEM nvMyCallHeadByte;
uint8_t EEMEM nvMyCall[7];
uint8_t EEMEM nvDestCallHeadByte;
uint8_t EEMEM nvDestCall[7];
uint8_t EEMEM nvPath1CallHeadByte;
uint8_t EEMEM nvPath1Call[7];
uint8_t EEMEM nvPath2CallHeadByte;
uint8_t EEMEM nvPath2Call[7];

// beacon text(raw packet)
#define NV_BEACON_TEXT_HEAD_BYTE_VALUE 0x99
#define NV_BEACON_TEXT_BLOCK_SIZE SETTINGS_BEACON_TEXT_MAX
uint8_t EEMEM nvBeaconTextHeadByte;
uint8_t EEMEM nvBeaconText[NV_BEACON_TEXT_BLOCK_SIZE];

/*
 * Copy the data into settings and save to eeprom
 */
bool settings_set_bytes(uint8_t *bytes, uint16_t size){
	if(size != sizeof(SettingsData)){
		// size mismatch
		return false;
	}
	//FIXME - swap the byte order ?
	ATOMIC( \
		memcpy(&g_settings,bytes,size) \
	);

	return true;
}

/*
 * Load settings
 */
bool settings_load(void){
	uint8_t magicHead = eeprom_read_byte((void*)&nvSetHeadByte);
	if (magicHead != NV_SETTINGS_HEAD_BYTE_VALUE) {
		// fill up zero values
		return false;
	}
	eeprom_read_block((void*)&g_settings, (void*)nvSettings, NV_SETTINGS_BLOCK_SIZE);
	uint8_t sum = eeprom_read_byte((void*)&nvSetCrcByte);
	if(sum != calc_crc((uint8_t*)&g_settings,sizeof(SettingsData))){
		// if sum check failed, clear the setting data
		settings_clear();
		// TODO - roll back to the default values ?
		// memset(&g_settings,0,sizeof(SettingsData));
		// reboot()!
		return false;
	}
	return true;
}

/*
 * Save settings
 */
bool settings_save(void){
	eeprom_update_block((void*)&g_settings, (void*)nvSettings, NV_SETTINGS_BLOCK_SIZE);
	eeprom_update_byte((void*)&nvSetHeadByte, NV_SETTINGS_HEAD_BYTE_VALUE);
	uint8_t sum = calc_crc((uint8_t*)&g_settings,sizeof(SettingsData));
	eeprom_update_byte((void*)&nvSetCrcByte, sum);
	return true;
}

/*
 * Clear settings
 */
void settings_clear(void){
	eeprom_update_byte((void*)&nvSetHeadByte, 0xFF);
	eeprom_update_byte((void*)&nvBeaconTextHeadByte, 0xFF);
	eeprom_update_byte((void*)&nvMyCallHeadByte,0xFF);
	eeprom_update_byte((void*)&nvDestCallHeadByte,0xFF);
	eeprom_update_byte((void*)&nvPath1CallHeadByte,0xFF);
	eeprom_update_byte((void*)&nvPath2CallHeadByte,0xFF);
	eeprom_update_byte((void*)&nvSetCrcByte, 0xFF);
}

/*
 * Get settings value, fill up the valueOut buffer, and update the valueOutLen of the actual value lenth
 */
void settings_get(SETTINGS_TYPE type, void* valueOut, uint8_t* pValueOutLen){
	if(*pValueOutLen <= 0) return;
	switch(type){
		case SETTINGS_SYMBOL:
			*((uint8_t*)valueOut) = g_settings.symbol[0];
			*((uint8_t*)valueOut + 1) = g_settings.symbol[1];
			*pValueOutLen = 2;
			break;
		case SETTINGS_RUN_MODE:
			*((uint8_t*)valueOut) = g_settings.run_mode;
			*pValueOutLen = 1;
			break;
		case SETTINGS_BEACON_INTERVAL:
			*((uint16_t*)valueOut) = g_settings.beacon_interval;
			*pValueOutLen = 2;
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
	(void)valueLen;
	switch(type){
		case SETTINGS_SYMBOL:
			g_settings.symbol[0] = *((uint8_t*)value);
			g_settings.symbol[1] = *(((uint8_t*)value)+1);
			break;
		case SETTINGS_RUN_MODE:
			g_settings.run_mode = *((uint8_t*)value);
			break;
		case SETTINGS_BEACON_INTERVAL:
			g_settings.beacon_interval =  *((uint16_t*)value);
			break;
		default:
			break;
	}
}

/*
 * Get call object from settings
 */
void settings_get_call(SETTINGS_TYPE callType, struct AX25Call *call){
	memset(call,0,sizeof(struct AX25Call));
	uint8_t head = 0;
	switch(callType){
	case SETTINGS_MY_CALL:
		head = eeprom_read_byte((void*)&nvMyCallHeadByte);
		if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
			eeprom_read_block((void*)call,(void*)nvMyCall,7);
		}else{
			// read the default values "N0CALL"
			snprintf_P(call->call,7,PSTR("N0CALL"));
		}
		break;
	case SETTINGS_DEST_CALL:
		head = eeprom_read_byte((void*)&nvDestCallHeadByte);
		if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
			eeprom_read_block((void*)call,(void*)nvDestCall,7);
		}else{
			// read the default values "N0CALL"
			snprintf_P(call->call,7,PSTR("APTI01"));
		}
		break;
	case SETTINGS_PATH1_CALL:
		head = eeprom_read_byte((void*)&nvPath1CallHeadByte);
		if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
			eeprom_read_block((void*)call,(void*)nvPath1Call,7);
		}else{
			// read the default values "N0CALL"
			snprintf_P(call->call,6,PSTR("WIDE1"));
			call->ssid = 1;
		}
		break;
	case SETTINGS_PATH2_CALL:
		head = eeprom_read_byte((void*)&nvPath2CallHeadByte);
		if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
			eeprom_read_block((void*)call,(void*)nvPath2Call,7);
		}else{
			// read the default values "N0CALL"
			snprintf_P(call->call,6,PSTR("WIDE2"));
			call->ssid = 2;
		}
		break;
	default:
		break;
	}
}


/*
 * Set call object to settings
 */
void settings_set_call(SETTINGS_TYPE callType, struct AX25Call *call){
	switch(callType){
	case SETTINGS_MY_CALL:
		eeprom_update_block((void*)call,(void*)nvMyCall,7);
		eeprom_update_byte((void*)&nvMyCallHeadByte,NV_SETTINGS_HEAD_BYTE_VALUE);
		break;
	case SETTINGS_DEST_CALL:
		eeprom_update_block((void*)call,(void*)nvDestCall,7);
		eeprom_update_byte((void*)&nvDestCallHeadByte,NV_SETTINGS_HEAD_BYTE_VALUE);
		break;
	case SETTINGS_PATH1_CALL:
		eeprom_update_block((void*)call,(void*)nvPath1Call,7);
		eeprom_update_byte((void*)&nvPath1CallHeadByte,NV_SETTINGS_HEAD_BYTE_VALUE);
		break;
	case SETTINGS_PATH2_CALL:
		eeprom_update_block((void*)call,(void*)nvPath2Call,7);
		eeprom_update_byte((void*)&nvPath2CallHeadByte,NV_SETTINGS_HEAD_BYTE_VALUE);
		break;
	default:
		break;
	}
}

//#define DEFAULT_BEACON_TEXT "!3014.00N/12009.00E>000/000/A=000087TinyAPRS Rocks!"
/*
 * get beacon text from settings
 */
uint8_t settings_get_beacon_text(char* buf, uint8_t bufLen){
	buf[0] = 0;
	uint8_t verification = eeprom_read_byte((void*)&nvBeaconTextHeadByte);
	uint8_t bytesToRead = MIN(bufLen,SETTINGS_BEACON_TEXT_MAX);
	if (verification == NV_BEACON_TEXT_HEAD_BYTE_VALUE) {
		eeprom_read_block((void*)buf, (void*)nvBeaconText, bytesToRead);
	}

	// get the actual text length, like strlen()
	uint8_t i = 0;
	while(buf[i] != 0 && i < bytesToRead){
		i++;
	}
	if(i > 0)
		return i;
	else
		return snprintf_P(buf,bufLen,PSTR(DEFAULT_BEACON_TEXT));
}

/*
 * set beacon text to settings
 */
uint8_t settings_set_beacon_text(char* data, uint8_t dataLen){
	uint8_t bytesToWrite = MIN(dataLen,(SETTINGS_BEACON_TEXT_MAX - 1));
	eeprom_update_block((void*)data, (void*)nvBeaconText, bytesToWrite);
	eeprom_update_byte((void*)(nvBeaconText + bytesToWrite), 0);
	eeprom_update_byte((void*)&nvBeaconTextHeadByte, NV_BEACON_TEXT_HEAD_BYTE_VALUE);
	return bytesToWrite;
}
