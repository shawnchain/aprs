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
 * \brief The main routines
 *
 * \author Shawn Chain <shawn.chain@gmail.com>
 * \date 2015-02-07
 */

#include <cfg/compiler.h>

#include <cpu/irq.h>
#include <cpu/pgm.h>     /* PROGMEM */
#include <avr/pgmspace.h>

#include <net/afsk.h>
#include <net/ax25.h>
#include <net/kiss.h>

#include <drv/ser.h>
#include <drv/timer.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "global.h"

#include "utils.h"

#include "reader.h"

#include "settings.h"
#include "console.h"
#include "beacon.h"
#include "radio.h"
#if CFG_RADIO_ENABLED
static SoftSerial softSer;
#endif

#include <cfg/cfg_gps.h>
#if CFG_GPS_ENABLED
#include "gps.h"
GPS g_gps;
#endif

Afsk g_afsk;
AX25Ctx g_ax25;

Serial g_serial;
static Reader serialReader;

#define ADC_CH 0
#define DAC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

typedef enum{
	MODE_CFG  = 0,
	MODE_KISS = 1,
	MODE_DIGI = 2,
	MODE_TRACKER_BEACON = 3,
	MODE_TEST_BEACON = 0xf
}RunMode;


static RunMode currentMode = MODE_CFG;

///////////////////////////////////////////////////////////////////////////////////
// Message Callbacks
static void print_call_P(KFile *ch, const AX25Call *call,char* buf) {
	sprintf_P(buf,PSTR("%.6s"),call->call);
	kfile_print(ch, buf);
	if (call->ssid){
		sprintf_P(buf,PSTR("-%d"),call->ssid);
		kfile_print(ch, buf);
	}
}

static uint16_t count = 1;
INLINE void print_ax25_message(Serial *pSer, AX25Msg *msg){
	#if 0
		ax25_print(&(pSer->fd),msg); // less code but need 16 bytes of ram
	#else
		char buf[16];
		KFile *ch = &(pSer->fd);
		sprintf(buf,"%d ",count++);
		kfile_print(ch,buf);
		// CALL/RPT/DEST
		print_call_P(ch, &msg->src,buf);
		kfile_putc('>', ch);
		print_call_P(ch, &msg->dst,buf);
		#if CONFIG_AX25_RPT_LST
		for (int i = 0; i < msg->rpt_cnt; i++)
		{
			kfile_putc(',', ch);
			print_call_P(ch, &msg->rpt_lst[i],buf);
			/* Print a '*' if packet has already been transmitted
			 * by this repeater */
			if (AX25_REPEATED(msg, i))
				kfile_putc('*', ch);
		}
		#endif
		// DATA PAYLOAD
		SERIAL_PRINTF_P(pSer, PSTR(":%.*s\n\r"), msg->len, msg->info);
	#endif
}

/*
 * Print on console the message that we have received.
 */
static void ax25_msg_callback(struct AX25Msg *msg){
	switch(currentMode){
	case MODE_CFG:{
		// Print received message to serial
		print_ax25_message(&g_serial,msg);
		break;
	}
	case MODE_KISS:
		kiss_send_host(0x00/*kiss port id*/,g_ax25.buf,g_ax25.frm_len - 2);
		break;

	default:
		break;

	}
}

static void kiss_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit KISS mode\r\n"));
}

static void beacon_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit Beacon mode\r\n"));
}

static void _serial_reader_callback(char* line, uint8_t len){
	switch(currentMode){
		case MODE_CFG:
			console_parse_command(line,len);
			break;
		case MODE_TRACKER_BEACON:
//			gps_parse(line,len);
			break;
		default:
			break;
	}
}

#if CFG_GPS_ENABLED && CFG_GPS_TEST
static bool cmd_gps_test(Serial* pSer, char* value, size_t len){
	(void)pSer;
	if(len > 0){
		if(gps_parse(&g_gps,value,(int)len)){
			// print the result
			GPS *gps = &g_gps;
			SERIAL_PRINTF_P((&g_serial),PSTR("lat: %s, lon: %s, speed: %s, heading: %s\n\r"),gps->_lat,gps->_lon, gps->_term[GPRMC_TERM_SPEED],gps->_term[GPRMC_TERM_HEADING]);
		}
	}
	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////
// Command handlers
/*
 * AT+MODE=[0|1|2]
 */
static bool cmd_switch_mode(Serial* pSer, char* value, size_t len){
	bool modeOK = false;
	if(len > 0 ){
		int i = atoi(value);
		if(i == (int)currentMode){
			// already in this mode, bail out.
			return true;
		}

		modeOK = true;
		switch(i){
		case MODE_CFG:
			// COMMAND/CONFIG MODE
			currentMode = MODE_CFG;
			g_ax25.pass_through = 0;		// parse ax25 frames
			ser_purge(pSer);  			// clear all rx/tx buffer
			break;

		case MODE_KISS:
			// KISS MODE
			currentMode = MODE_KISS;
			g_ax25.pass_through = 1;		// don't parse ax25 frames
			ser_purge(pSer);  			// clear serial rx/tx buffer
			SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
			break;

/*
		case MODE_DIGI:
			// DIGI MODE
			runMode = MODE_DIGI;
			ax25.pass_through = 0;		// parse ax25 frames
			kiss_set_enabled(false);	// kiss off
			beacon_set_enabled(true);	// beacon on
			ser_purge(pSer);  			// clear serial rx/tx buffer
			break;

		case MODE_TRACKER_BEACON:
			// TRACKER MODE
			runMode = MODE_TRAC;
			// disable the ax25 module
			ax25.pass_through = 0;		// parse ax25 frames
			kiss_set_enabled(false);	// kiss off
			tracker_set_enabled(true);	// gps on
			break;
*/
		default:
			// unknown mode
			modeOK = false;
			break;
		}
	}

	if(!modeOK){
		SERIAL_PRINTF_P(pSer,PSTR("Invalid mode %s, [0|1|2] is accepted\r\n"),value);
	}

	return true;
}



/*
 * AT+KISS=1 - enable the KISS mode
 */
static bool cmd_enter_kiss_mode(Serial* pSer, char* value, size_t len){
	if(len > 0 && value[0] == '1'){
		currentMode = MODE_KISS;
		g_ax25.pass_through = 1;
		ser_purge(pSer);  // clear all rx/tx buffer

		SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
	}else{
		SERIAL_PRINTF_P(pSer,PSTR("Invalid value %s, only value 1 is accepted\r\n"),value);
	}
	return true;
}

#if 0
static void gps_callback(void *p){
	GPS *gps = (GPS*)p;
	SERIAL_PRINTF_P((&g_serial),PSTR("lat: %s, lon: %s, date: %s,%s\n\r"),gps->_lat,gps->_lon, gps->_date,gps->_utc);
}
#endif

static void init(void)
{

    IRQ_ENABLE;

	kdbg_init();
	timer_init();

	/* Initialize serial port, we are going to use it to show APRS messages*/
	ser_init(&g_serial, SER_UART0);
	ser_setbaudrate(&g_serial, SER_BAUD_RATE_9600);
    // For some reason BertOS sets the serial
    // to 7 bit characters by default. We set
    // it to 8 instead.
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	/*
	 * Init afsk demodulator. We need to implement the macros defined in hw_afsk.h, which
	 * is the hardware abstraction layer.
	 * We do not need transmission for now, so we set transmission DAC channel to 0.
	 */
	afsk_init(&g_afsk, ADC_CH, DAC_CH);

	/*
	 * Here we initialize AX25 context, the channel (KFile) we are going to read messages
	 * from and the callback that will be called on incoming messages.
	 */
	ax25_init(&g_ax25, &g_afsk.fd, ax25_msg_callback);
	g_ax25.pass_through = false;

	// Initialize the kiss module
	kiss_init(&(g_serial.fd),&g_ax25,&g_afsk,kiss_mode_exit_callback);

	// Initialize the beacon module
    beacon_init(beacon_mode_exit_callback);

    // Load settings first
    settings_load();

#if CFG_RADIO_ENABLED
    // Initialize the soft serial and radio
    softser_init(&softSer, CFG_RADIO_RX_PIN,CFG_RADIO_TX_PIN);
    softser_start(&softSer,9600);
    radio_init(&softSer,431, 400);
#endif

    reader_init(&serialReader,&(g_serial.fd),_serial_reader_callback);

    //////////////////////////////////////////////////////////////
    // Initialize the console & commands
    console_init();
    console_add_command(PSTR("MODE"),cmd_switch_mode);			// setup tnc run mode
    console_add_command(PSTR("KISS"),cmd_enter_kiss_mode);		// enable KISS mode

    // Initialize GPS NMEA/GPRMC parser
#if CFG_GPS_ENABLED && CFG_GPS_TEST
    console_add_command(PSTR("GPS"),cmd_gps_test);

    static char s[80];
    sprintf_P(s,PSTR("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n"));
	int l = strlen(s);

    gps_init(&g_gps);
    gps_parse(&g_gps,s,l);

//    for(int i = 0;i < 12;i++){
//    	SERIAL_PRINTF_P((&g_serial),PSTR("term: %s\n"),g_gps._term[i]);
//    }

    SERIAL_PRINTF_P((&g_serial),PSTR("lat: %s, lon: %s, speed: %s, heading: %s\n\r"),g_gps._lat,g_gps._lon, g_gps._term[GPRMC_TERM_SPEED],g_gps._term[GPRMC_TERM_HEADING]);
#endif

}

// Free ram test
INLINE uint16_t freeRam (void) {
	extern int __heap_start, *__brkval;
	uint8_t v;
	uint16_t vaddr = (uint16_t)(&v);
	return (uint16_t) (vaddr - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}


int main(void){

	init();

	while (1){
		/*
		 * This function will look for new messages from the AFSK channel.
		 * It will call the message_callback() function when a new message is received.
		 * If there's nothing to do, this function will call cpu_relax()
		 */
		ax25_poll(&g_ax25);

		switch(currentMode){
			case MODE_CFG:{
				reader_poll(&serialReader);
				break;
			}

			case MODE_KISS:{
				kiss_serial_poll();
				kiss_queue_process();
				break;
			}

			case MODE_DIGI:{
				break;
			}

			case MODE_TRACKER_BEACON:{
				reader_poll(&serialReader);
				break;
			}

			default:
				break;
		}// end of switch(runMode)

		// Enable beacon if not under KISS TNC mode
		if(currentMode != MODE_KISS)
			beacon_poll();

#define FREE_RAM_DEBUG 0
#if FREE_RAM_DEBUG
		{
			static uint32_t i = 0;
			if(i++ == 100000){
				i = 0;
				// log the stack size
				uint16_t ram = freeRam();
				SERIAL_PRINTF((&ser),"%u\r\n",ram);
			}
		}
#endif
#if 0
		// Dump the isr changes
		{
			static uint32_t i = 0;
			//static uint32_t j = 0;
			if(i++ == 30000){
				i = 0;

				char c;
				while(softser_avail(&softSer)){
					c = softser_read(&softSer);
					kfile_putc(c,&(ser.fd));
				}
				char buf[8];
				sprintf_P(buf,PSTR("0K\n\r"));
				softser_print(&softSer,buf);
			}
		}
#endif
	} // end of while(1)
	return 0;
}
