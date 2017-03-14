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
#include "utils.h"

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


//FIXME - CSMA check ?

static uint32_t c = 1;
static bool _digi_repeat_message(AX25Msg *msg){
	// force delay 150ms
	timer_delay(150);
#if DIGI_DEBUG
	kfile_printf_P(&g_serial.fd,PSTR(">[%d]digipeat:\r\n"),c++);
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
		cacheIndex++;
		if(cacheIndex > CACHE_SIZE) cacheIndex = cacheIndex - CACHE_SIZE;
		cache[cacheIndex - 1].hash = hash;
		cache[cacheIndex - 1].timestamp = ticks_to_ms(timer_clock()) / 1000;
	}
	return dup;
}

#define copy_rpt_sta(src,dst) \
	memset(&(dst->call),0,6); \
	memcpy(&(dst->call),&(src->call),5); \
	dst->ssid = src->ssid;

#define rpt_required(rpt) \


bool digi_handle_aprs_message(struct AX25Msg *msg){
	/*
	AX25Call rptd[AX25_MAX_RPT];
	uint8_t i = 0,j=0;
	BOOL changed = false;
	for(;i < msg->rpt_cnt;i++,j++){
		AX25Call *rpt = msg->rpt_lst + i;
		if( ((strncasecmp_P(rpt->call,PSTR("WIDE1"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE2"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE3"),5) == 0))
				&& (rpt->ssid > 0)
				&& !(AX25_REPEATED(msg,i)) ){

			if(_digi_check_is_duplicated(msg)){
				// seems duplicated in cache, drop
				return false;
			}

			// insert mycall
			if()


			changed = true;
			continue;
		}
		// just copy
		copy_rpt_sta(&(msg->rpt_lst + i), &(rptd + j));
	}
	*/
#if 1
	// A quick-n-dirty digipeat logic based on the assumption: WIDE2-2 is always the last element in the path.
#if DIGI_DEBUG
	bool prt = false;
#endif

	for(int i = 0;i < msg->rpt_cnt;i++){
		AX25Call *sta = msg->rpt_lst + i;
		if( ((strncasecmp_P(sta->call,PSTR("WIDE1"),5) == 0) || (strncasecmp_P(sta->call,PSTR("WIDE2"),5) == 0) || (strncasecmp_P(sta->call,PSTR("WIDE3"),5) == 0))
				&& (sta->ssid > 0)
				&& !(AX25_REPEATED(msg,i)) ){

			// check duplications;
			if(_digi_check_is_duplicated(msg)){
				// seems duplicated in cache, drop
				return false;
			}
#if DIGI_DEBUG
			if(!prt){
				kfile_printf_P(&g_serial.fd,PSTR(">[%d]original:\r\n"),c);
				ax25_print(&g_serial.fd, msg);
				prt = true;
			}

#endif

			if(sta->ssid >1){ //    case: WIDE2-2
				sta->ssid--; // subtract: WIDE2-1
				if(i < AX25_MAX_RPT - 1){
					AX25Call *c = sta + 1; // the next element in path to be copied/overwritten to
					memset(&c->call,0,6);
					memcpy(&c->call,&sta->call,5);
					c->ssid = sta->ssid;
					AX25_SET_REPEATED(msg,i+1,0);
					if(i == msg->rpt_cnt-1){ // increase count if current element[i] is the last one in path list
						msg->rpt_cnt++; //
					} // otherwise the next element is overwritten :(
				}else{
					// no space left for the new digi call, drop;
					return false;
				}
			}
			// current element[i] will be replaced by THIS digi call and mark repeated.
			settings_get_mycall(sta);
			AX25_SET_REPEATED(msg,i,1);
			return _digi_repeat_message(msg);
		}
	}// end for
#endif
	return false;
}

