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

static const char PROGMEM DEFAULT_BEACON_TEXT[] = "!3014.00N/12009.00E>TinyAPRS Rocks!";

static const PROGMEM CallData default_calldata = {
		.myCall = {
				.call={'N','O','C','A','L','L'},
				.ssid=0,
		},
		.destCall = {
				.call={'A','P','T','B','0','1'},
				.ssid=0,
		},
		.path1 = {
				.call={'W','I','D','E','1',0},
				.ssid=1,
		},
		.path2 = {
				.call={'W','I','D','E','2',0},
				.ssid=2,
		},
};


/*
 * Helper macros
 */
#undef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

// Instance of the settings data. // TODO - Store default settings in the PROGMEM
SettingsData g_settings = {
		.beacon={
			.symbol="/>",
			.interval = 0, // by default beacon is disabled;
			.type=0, // 0 = smart, 1 = fixed interval
			//.location={30,14,0,'N',120,0,9,'E'},
			//.phgd={0,0,0,0},
			//.comments="TinyAPRS Rocks!",
		},
		.rf = {
			.txdelay = 50,
			.persistence = 63,
			.txtail = 5,
			.slot_time = 10,
			.duplex = RF_DUPLEX_HALF
		},
		.run_mode = 1
};

#define NV_SETTINGS_HEAD_BYTE_VALUE 0x88
uint8_t EEMEM nvSetHeadByte;
uint8_t EEMEM nvSettings[sizeof(SettingsData)];
uint8_t EEMEM nvSetCrcByte;

uint8_t EEMEM nvCallDataHeadByte;
uint8_t EEMEM nvCallData[sizeof(CallData)];

// beacon text(raw packet)
#define NV_BEACON_TEXT_HEAD_BYTE_VALUE 0x99
uint8_t EEMEM nvBeaconTextHeadByte;
uint8_t EEMEM nvBeaconText[SETTINGS_BEACON_TEXT_MAX_LEN];

/*
 * Copy the data into settings and save to eeprom
 */
bool settings_set_params_bytes(uint8_t *bytes, uint16_t size){
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
	eeprom_read_block((void*)&g_settings, (void*)nvSettings, sizeof(SettingsData));
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
	eeprom_update_block((void*)&g_settings, (void*)nvSettings, sizeof(SettingsData));
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
	eeprom_update_byte((void*)&nvCallDataHeadByte, 0xFF);
	eeprom_update_byte((void*)&nvBeaconTextHeadByte, 0xFF);
	eeprom_update_byte((void*)&nvSetCrcByte, 0xFF);
}

/*
 * Get settings value, fill up the valueOut buffer, and update the valueOutLen of the actual value lenth
 */
void settings_get_params(SettingsParamKey type, void* valueOut, uint8_t* pValueOutLen){
	if(*pValueOutLen <= 0) return;
	switch(type){
		case SETTINGS_SYMBOL:
			*((uint8_t*)valueOut) = g_settings.beacon.symbol[0];
			*((uint8_t*)valueOut + 1) = g_settings.beacon.symbol[1];
			*pValueOutLen = 2;
			break;
		case SETTINGS_RUN_MODE:
			*((uint8_t*)valueOut) = g_settings.run_mode;
			*pValueOutLen = 1;
			break;
		case SETTINGS_BEACON_INTERVAL:
			*((uint16_t*)valueOut) = g_settings.beacon.interval;
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
void settings_set_params(SettingsParamKey type, void* value, uint8_t valueLen){
	(void)valueLen;
	switch(type){
		case SETTINGS_SYMBOL:
			g_settings.beacon.symbol[0] = *((uint8_t*)value);
			g_settings.beacon.symbol[1] = *(((uint8_t*)value)+1);
			break;
		case SETTINGS_RUN_MODE:
			g_settings.run_mode = *((uint8_t*)value);
			break;
		case SETTINGS_BEACON_INTERVAL:
			g_settings.beacon.interval =  *((uint16_t*)value);
			break;
		default:
			break;
	}
}

void settings_set_call_data(CallData *callData){
	eeprom_update_block((void*)callData,(void*)nvCallData,sizeof(CallData));
	eeprom_update_byte((void*)&nvCallDataHeadByte,NV_SETTINGS_HEAD_BYTE_VALUE);
}

void settings_get_call_data(CallData *callData){
	memset(callData,0,sizeof(CallData));
	uint8_t head = eeprom_read_byte((void*)&nvCallDataHeadByte);
	if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
		// read the EEPROM back
		eeprom_read_block((void*)callData,(void*)nvCallData,sizeof(CallData));
	}else{
		//read the default parameters
		memcpy_P((void*)callData,(const void*)&default_calldata,sizeof(CallData));
	}
}

void settings_get_mycall(AX25Call *call){
	memset(call,0,sizeof(AX25Call));
	uint8_t head = eeprom_read_byte((void*)&nvCallDataHeadByte);
	if(head == NV_SETTINGS_HEAD_BYTE_VALUE){
		// read the EEPROM back
		uint8_t *p = nvCallData + sizeof(AX25Call); // mycall index=1, offset is sizeof(AX25Call)
		eeprom_read_block((void*)call,(void*)p,sizeof(AX25Call));
	}else{
		//read the default parameters
		memcpy_P((void*)call,(const void*)&default_calldata.myCall,sizeof(AX25Call));
	}
}

//DEFAULT_BEACON_TEXT "!3014.00N/12009.00E>000/000/A=000087TinyAPRS Rocks!"
/*
 * get beacon text from settings
 */
uint8_t settings_get_beacon_text(char* buf, uint8_t bufLen){
	buf[0] = 0;
	uint8_t verification = eeprom_read_byte((void*)&nvBeaconTextHeadByte);
	uint8_t bytesToRead = MIN(bufLen - 1,SETTINGS_BEACON_TEXT_MAX_LEN);
	if (verification != NV_BEACON_TEXT_HEAD_BYTE_VALUE) {
		// using default beacon text
		return snprintf_P(buf,bufLen,DEFAULT_BEACON_TEXT);
	}

	eeprom_read_block((void*)buf, (void*)nvBeaconText, bytesToRead);
	// get the actual text length, like strlen()
	uint8_t i = 0;
	while(buf[i] != 0 && i < bytesToRead){
		i++;
	}
	buf[i] = 0;
	return i;
}

/*
 * set beacon text to settings
 */
uint8_t settings_set_beacon_text(char* data, uint8_t dataLen){
	uint8_t bytesToWrite = MIN(dataLen,(SETTINGS_BEACON_TEXT_MAX_LEN - 1));
	eeprom_update_block((void*)data, (void*)nvBeaconText, bytesToWrite);
	eeprom_update_byte((void*)(nvBeaconText + bytesToWrite), 0);
	eeprom_update_byte((void*)&nvBeaconTextHeadByte, NV_BEACON_TEXT_HEAD_BYTE_VALUE);
	return bytesToWrite;
}
