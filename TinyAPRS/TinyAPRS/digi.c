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

void digi_init(void){

}

static bool _digi_repeat_message(AX25Msg *msg){
	{
	char fmt[16];
	sprintf_P(fmt,PSTR("digipeat:\r\n"));
	kfile_printf(&g_serial.fd,fmt);
	}
	ax25_print(&g_serial.fd, msg);
	ax25_sendMsg(&g_ax25, msg);
	return true;
}

bool digi_handle_aprs_message(struct AX25Msg *msg){
	for(int i = 0;i < msg->rpt_cnt;i++){
		AX25Call *rpt = msg->rpt_lst + i;
		uint8_t len = 5;
		if( ((strncasecmp_P(rpt->call,PSTR("WIDE1"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE2"),5) == 0) || (strncasecmp_P(rpt->call,PSTR("WIDE3"),5) == 0))
				&& (rpt->ssid > 0)
				&& !(AX25_REPEATED(msg,i)) ){
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
			settings_get(SETTINGS_MY_CALL,rpt->call,&len);
			settings_get(SETTINGS_MY_SSID,&rpt->ssid,&len);
			AX25_SET_REPEATED(msg,i,1);
			return _digi_repeat_message(msg);
		}
	}// end for

	return false;
}

