/*
 * \file digi.c
 * <!--
 * This file is part of TinyAPRS.
 * Released under GPL License
 *
 * Copyright 2015 Shawn Chain (shawn.chain@gmail.com)
 *
 * -->
 *
 * \brief 
 *
 * \author shawn
 * \date 2015-4-13
 */

#include "digi.h"

#include <net/afsk.h>
#include <net/ax25.h>
#include <drv/ser.h>
#include <drv/timer.h>
#include <io/kfile.h>

#include "global.h"
#include "settings.h"

typedef struct CacheEntry{
	uint16_t hash;
	uint32_t timestamp; // in seconds
}CacheEntry;

#define DIGI_DEBUG CFG_DIGI_DEBUG
#define DUP_CHECK_INTERVAL CFG_DIGI_DUP_CHECK_INTERVAL
#define CURRENT_TIME_SECONDS() (ticks_to_ms(timer_clock()) / 1000)
#define CACHE_SIZE CFG_DIGI_DUP_CHECK_CACHE_SIZE
static CacheEntry cache[CACHE_SIZE];
static uint8_t cacheIndex;

void digi_init(void){
	memset(&cache,0,sizeof(CacheEntry) * CACHE_SIZE);
	cacheIndex = 0;
}

static bool _digi_repeat_message(AX25Msg *msg){
	// force delay 150ms
	timer_delayTicks(ms_to_ticks(150));
#if DIGI_DEBUG
	{
	char fmt[16];
	sprintf_P(fmt,PSTR("digipeat:\r\n"));
	kfile_printf(&g_serial.fd,fmt);
	}
	ax25_print(&g_serial.fd, msg);
#endif
	ax25_sendMsg(&g_ax25, msg);
	return true;
}


/*
 * Calculate the digi message hashcode
 */
static uint16_t _digi_calc_hash(AX25Msg *msg){
	uint16_t hash = 0;
	size_t i = 0;

	for(i = 0;i < 6;i++){	//APRS src/dst call size is fixed to 6 bytes
		hash = hash * 31 +  msg->src.call[i];
	}
	hash = hash * 31 + msg->src.ssid;

	for(i = 0;i < 6;i++){
		hash = hash * 31 +  msg->dst.call[i];
	}
	hash = hash * 31 + msg->dst.ssid;

	for(i = 0;i < msg->len;i++){
		hash = hash * 31 + msg->info[i];
	}
	return hash;
}

/*
 * duplication checks
 */
static bool _digi_check_is_duplicated(AX25Msg *msg){
	bool dup = false;
	uint16_t hash = _digi_calc_hash(msg);
	// check starts from the latest cache entry
	for(uint8_t i = CACHE_SIZE ;cacheIndex > 0 &&  i >0 ;i--){
		uint8_t j = (cacheIndex - 1 + i) % CACHE_SIZE;
		if((cache[j].hash == hash) && (CURRENT_TIME_SECONDS() - cache[j].timestamp) < DUP_CHECK_INTERVAL ){
			dup = true;
			break;
		}
	}
	if(!dup){
		cacheIndex = (cacheIndex + 1) % CACHE_SIZE;
		cache[cacheIndex - 1].hash = hash;
		cache[cacheIndex - 1].timestamp = ticks_to_ms(timer_clock()) / 1000;
	}
	return dup;
}

bool digi_handle_aprs_message(struct AX25Msg *msg){
	for(int i = 0;i < msg->rpt_cnt;i++){
		AX25Call *rpt = msg->rpt_lst + i;
		//uint8_t len = 5;
		if( ((strncasecmp_P(rpt->call,PSTR("WIDE1"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE2"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE3"),5) == 0))
				&& (rpt->ssid > 0)
				&& !(AX25_REPEATED(msg,i)) ){

			// check duplications;
			if(_digi_check_is_duplicated(msg)){
				// seems duplicated in cache, drop
				return false;
			}

			if(rpt->ssid >1){
				rpt->ssid--; // SSID-1
				if(i < AX25_MAX_RPT - 1){
					// copy WIDEn-(N-1) to next rpt list
					AX25Call *c = rpt + 1;
					memset(&c->call,0,6);
					memcpy(&c->call,&rpt->call,5);
					c->ssid = rpt->ssid;
					msg->rpt_cnt++; // we inserted my-call as one of the rpt address.
				}else{
					// no space left for the new digi call, drop;
					return false;
				}
			}
			// replace the path with digi call and mark repeated.
			settings_get_call(SETTINGS_MY_CALL,rpt);
			AX25_SET_REPEATED(msg,i,1);
			return _digi_repeat_message(msg);
		}
	}// end for

	return false;
}

