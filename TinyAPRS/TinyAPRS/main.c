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

#include <drv/ser.h>
#include <drv/timer.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "global.h"

#include "utils.h"

#include "settings.h"
#include "reader.h"

#if MOD_CONSOLE
#include "console.h"
#endif

#if MOD_KISS
#include <net/kiss.h>
#endif

#if MOD_DIGI
#include "cfg/cfg_digi.h"
#include "digi.h"
#endif

#if MOD_RADIO
#include "cfg/cfg_radio.h"
#include "radio.h"
static SoftSerial softSer;
#endif

#if MOD_TRACKER
#include "cfg/cfg_gps.h"
#include "gps.h"
#include "tracker.h"
GPS g_gps;
#endif

#if MOD_BEACON
#include "cfg/cfg_beacon.h"
#include "beacon.h"
#endif

Afsk g_afsk;
AX25Ctx g_ax25;
Serial g_serial;

#if MOD_KISS
#define SHARED_BUF_LEN CONFIG_AX25_FRAME_BUF_LEN //shared buffer is 330 bytes for KISS module reading received AX25 frame.
#else
#define SHARED_BUF_LEN 128
#endif
uint8_t g_shared_buf[SHARED_BUF_LEN];

#define ADC_CH 0
#define DAC_CH 0
#define SER_BAUD_RATE_9600 9600L
#define SER_BAUD_RATE_115200 115200L

// DEBUG FLAGS
#define DEBUG_FREE_RAM 0
#define DEBUG_SOFT_SER 0
#define DEBUG_GPS_OUTPUT 0

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
	case MODE_CFG:
		// Print received message to serial
		ax25_print(&(g_serial.fd),msg);
		break;

#if MOD_KISS
	case MODE_KISS:
		kiss_send_host(0x00/*kiss port id*/,g_ax25.buf,g_ax25.frm_len - 2);
		break;
#endif

#if MOD_DIGI
	case MODE_DIGI:
		digi_handle_aprs_message(msg);
		break;
#endif

	default:
		break;

	}
}

/*
 * callback when kiss mode is end
 */
#if MOD_KISS
static void kiss_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit KISS mode\r\n"));
}
#endif

#if MOD_BEACON
/*
 * callback when beacon mode is end
 */
static void beacon_mode_exit_callback(void){
	currentMode = MODE_CFG;
	SERIAL_PRINT_P((&g_serial),PSTR("Exit Beacon mode\r\n"));
}
#endif


/*
 * Callback when a line is read from the serial port.
 */
static void serial_read_line_callback(char* line, uint8_t len){
	(void)line;
	(void)len;
	switch(currentMode){

#if MOD_TRACKER
		case MODE_TRACKER:
			if(gps_parse(&g_gps,line,len) && g_gps.valid){
				tracker_update_location(&g_gps);
			}
			#if DEBUG_GPS_OUTPUT
			kfile_print((&(g_serial.fd)),line);
			kfile_putc('\r', &(g_serial.fd));
			kfile_putc('\n', &(g_serial.fd));
			#endif
			break;
#endif
#if MOD_KISS
		case MODE_KISS:
			// do nothing for KISS mode
			break;
#endif
		default:
#if MOD_CONSOLE
			console_parse_command(line,len);
#endif
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
#if MOD_KISS
		case MODE_KISS:
			// Enter KISS MODE
			currentMode = MODE_KISS;
			g_ax25.pass_through = 1;		// don't parse ax25 frames
			ser_purge(pSer);  			// clear serial rx/tx buffer
			SERIAL_PRINT_P(pSer,PSTR("Enter KISS mode\r\n"));
			break;
#endif

#if MOD_TRACKER
		case MODE_TRACKER:
			currentMode = MODE_TRACKER;
			SERIAL_PRINT_P(pSer,PSTR("Enter Tracker mode\r\n"));
			break;
#endif

#if MOD_DIGI
		case MODE_DIGI:
			// DIGI MODE
			currentMode = MODE_DIGI;
			g_ax25.pass_through = 0;		// need parse ax25 frames
			SERIAL_PRINT_P(pSer,PSTR("Enter Digi mode\r\n"));
			break;
#endif
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
			SERIAL_PRINTF_P(pSer,PSTR("Invalid mode %s, [0|1|2|3] is accepted\r\n"),value);
		}
	}else{
		// no parameters, just dump the mode
		SERIAL_PRINTF_P(pSer,PSTR("Current mode %d/%d\r\n"),currentMode,g_settings.run_mode);
	}

	return true;
}

#if MOD_KISS
/*
 * AT+KISS=1 enter KISS mode, same as AT+MODE=1, where MODE_KISS = 1
 */
static bool cmd_enter_kiss_mode(Serial* pSer, char* value, size_t len){
	(void)value;
	(void)len;
	char c[] = "1"; //
	return cmd_switch_mode(pSer, c, 1);
}
#endif

static void check_run_mode(void){
	static ticks_t ts = 0;
	if(timer_clock_unlocked() -  ts > ms_to_ticks(10000)){
		if(currentMode != g_settings.run_mode){
			char c[2];
			c[0] = (g_settings.run_mode + 48);
			c[1] = 0;
			cmd_switch_mode(&g_serial,c,1);
		}
		ts = timer_clock_unlocked(); // update the timestamp
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
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // see ATMEGA328P datasheet P197, Table 20-11. UCSZn Bits Settings

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
	// NOTE - use shared memory buffer
#if MOD_KISS
	kiss_init(&(g_serial.fd),g_shared_buf, SHARED_BUF_LEN, kiss_mode_exit_callback);
#endif

#if MOD_BEACON
	// Initialize the beacon module
    beacon_init(beacon_mode_exit_callback);
#endif

    // Initialize the digi module
#if MOD_DIGI
    digi_init();
#endif

#if MOD_RADIO
    // Initialize the soft serial and radio
    softser_init(&softSer, CFG_RADIO_RX_PIN,CFG_RADIO_TX_PIN);
    softser_start(&softSer,9600);
    radio_init(&softSer,431, 400);
#endif

    reader_init(g_shared_buf, SHARED_BUF_LEN,serial_read_line_callback);

    // Initialize GPS NMEA/GPRMC parser
#if MOD_TRACKER
    gps_init(&g_gps);
#endif

#if MOD_CONSOLE
    //////////////////////////////////////////////////////////////
    // Initialize the console & commands
    console_init();
    console_add_command(PSTR("MODE"),cmd_switch_mode);			// setup tnc run mode
#if MOD_KISS
    console_add_command(PSTR("KISS"),cmd_enter_kiss_mode);		// enable KISS mode
#endif
#endif
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
				reader_poll(&g_serial);
#if MOD_BEACON
				beacon_broadcast_poll();
#endif
				break;
#if MOD_TRACKER
			case MODE_TRACKER:
				reader_poll(&g_serial);
				break;
#endif

#if MOD_KISS
			case MODE_KISS:{
				kiss_serial_poll();
				kiss_queue_process();
				break;
			}
#endif

#if MOD_DIGI
			case MODE_DIGI:{
				reader_poll(&g_serial);
				beacon_broadcast_poll();
				break;
			}
#endif

			default:
				break;
		}// end of switch(runMode)

#if DEBUG_FREE_RAM
		{
			static ticks_t ts = 0;

			if(timer_clock_unlocked() -  ts > ms_to_ticks(5000)){
				ts = timer_clock_unlocked();
				uint16_t ram = freeRam();
				SERIAL_PRINTF((&g_serial),"%u\r\n",ram);
			}
		}
#endif
#if DEBUG_SOFT_SER
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
