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
#if CPU_AVR
#include <avr/pgmspace.h>
#endif

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

#include "cfg/cfg_radio.h"
#if CFG_RADIO_ENABLED
#include "radio.h"
static SoftSerial softSer;
#endif

#include "cfg/cfg_gps.h"
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
	MODE_TRACKER = 2,
	MODE_DIGI = 3,
	MODE_TEST_BEACON = 0xf
}RunMode;
static RunMode currentMode = MODE_CFG;


///////////////////////////////////////////////////////////////////////////////////
// Callbacks
///////////////////////////////////////////////////////////////////////////////////

/*
 * callback when ax25 message received from radio
 */
static void ax25_msg_callback(struct AX25Msg *msg){
	switch(currentMode){
	case MODE_CFG:{
		// Print received message to serial
		ax25_print(&(g_serial.fd),msg); // less code but need 16 bytes of ram
		break;
	}
	case MODE_KISS:
		kiss_send_host(0x00/*kiss port id*/,g_ax25.buf,g_ax25.frm_len - 2);
		break;

	default:
		break;

	}
}

/*
 * callback when kiss mode is end
 */
static void kiss_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit KISS mode\r\n"));
}

/*
 * callback when beacon mode is end
 */
static void beacon_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit Beacon mode\r\n"));
}


//static void _smart_beacon_location(GPS *gps){
//	beacon_update_location(gps)
//}

/*
 * Callback when a line is read from the serial port.
 */
static void serial_read_line_callback(char* line, uint8_t len){
	switch(currentMode){
		case MODE_CFG:
			console_parse_command(line,len);
			break;

		case MODE_TRACKER:
#if CFG_GPS_ENABLED
			if(gps_parse(&g_gps,line,len) && g_gps.valid){
				beacon_update_location(&g_gps);
			}
#endif
			break;

		default:
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// Command handlers
///////////////////////////////////////////////////////////////////////////////////

/*
 * AT+MODE=[0|1|2]
 */
static bool cmd_switch_mode(Serial* pSer, char* value, size_t len){
	bool modeOK = false;
	if(len > 0 ){
		int i = atoi(value);
		if(i == (int)currentMode && i == g_settings.run_mode){
			// already in this mode, bail out.
			return true;
		}

		modeOK = true;
		switch(i){
		case MODE_CFG:
			// Enter COMMAND/CONFIG MODE
			currentMode = MODE_CFG;
			g_ax25.pass_through = 0;		// parse ax25 frames
			ser_purge(pSer);  			// clear all rx/tx buffer
			SERIAL_PRINT_P(pSer,PSTR("Enter Config mode\r\n"));
			break;

		case MODE_KISS:
			// Enter KISS MODE
			currentMode = MODE_KISS;
			g_ax25.pass_through = 1;		// don't parse ax25 frames
			ser_purge(pSer);  			// clear serial rx/tx buffer
			SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
			break;

		case MODE_TRACKER:
			currentMode = MODE_TRACKER;
			SERIAL_PRINT_P(pSer,PSTR("Enter Tracker mode\r\n"));
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
		*/

		default:
			// unknown mode
			modeOK = false;
			break;
		} // end of switch

		// save to settings/run_mode
		if(modeOK){
			if(currentMode != g_settings.run_mode){
				settings_set(SETTINGS_RUN_MODE,&currentMode,1);
				settings_save();
			}
		}else{
			SERIAL_PRINTF_P(pSer,PSTR("Invalid mode %s, [0|1|2] is accepted\r\n"),value);
		}
	}else{
		// no parameters, just dump the mode
		SERIAL_PRINTF_P(pSer,PSTR("Current mode %d/%d\r\n"),currentMode,g_settings.run_mode);
	}

	return true;
}

/*
 * AT+KISS=1 enter KISS mode, same as AT+MODE=1, where MODE_KISS = 1
 */
static bool cmd_enter_kiss_mode(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	char c[] = "1"; //
	return cmd_switch_mode(pSer, c, 1);
}

static void check_run_mode(void){
	static ticks_t ts = 0;
	if(timer_clock_unlocked() -  ts > ms_to_ticks(10000)){
		if(currentMode != g_settings.run_mode){
			char c[2];
			c[0] = (g_settings.run_mode + 48);
			c[1] = 0;
			cmd_switch_mode(&g_serial,c,1);
		}
	}
}

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

    // Load settings first
    settings_load();

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

#if CFG_RADIO_ENABLED
    // Initialize the soft serial and radio
    softser_init(&softSer, CFG_RADIO_RX_PIN,CFG_RADIO_TX_PIN);
    softser_start(&softSer,9600);
    radio_init(&softSer,431, 400);
#endif

    reader_init(&serialReader,&(g_serial.fd),serial_read_line_callback);

    // Initialize GPS NMEA/GPRMC parser
#if CFG_GPS_ENABLED
    gps_init(&g_gps);
#endif

    //////////////////////////////////////////////////////////////
    // Initialize the console & commands
    console_init();
    console_add_command(PSTR("MODE"),cmd_switch_mode);			// setup tnc run mode
    console_add_command(PSTR("KISS"),cmd_enter_kiss_mode);		// enable KISS mode
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
			case MODE_CFG:
			case MODE_TRACKER:
				reader_poll(&serialReader);
				break;

			case MODE_KISS:{
				kiss_serial_poll();
				kiss_queue_process();
				break;
			}

			case MODE_DIGI:{
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
			static ticks_t ts = 0;

			if(timer_clock_unlocked() -  ts > ms_to_ticks(5000)){
				ts = timer_clock_unlocked();
				uint16_t ram = freeRam();
				SERIAL_PRINTF((&g_serial),"%u\r\n",ram);
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

		check_run_mode();
	} // end of while(1)
	return 0;
}
