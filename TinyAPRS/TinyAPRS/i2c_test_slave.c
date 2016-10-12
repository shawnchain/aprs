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

#include <drv/ser.h>
#include <drv/timer.h>
#include <drv/i2c.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"
#include "reader.h"

#include "twi.h"

I2c g_i2c;
Serial g_serial;

#define SER_BAUD_RATE_9600 9600L

#define SHARED_BUF_LEN 128
uint8_t g_shared_buf[SHARED_BUF_LEN];

// DEBUG FLAGS
#define DEBUG_FREE_RAM 1
#define DEBUG_SOFT_SER 0

/*
 * Callback when a line is read from the serial port.
 */
static void serial_read_line_callback(char* line, uint8_t len){
	(void)line;
	(void)len;
	//console_parse_command(line,len);
}

static void slave_rx_callback(uint8_t *data, uint8_t len){
	for(uint16_t i = 0;i<len;i++){
		SERIAL_PRINTF_P((&g_serial),PSTR("%c"),data[i]);
	}
}

static uint8_t data[] = {'a','b','c','d',0};
static void slave_tx_callback(void){
	SERIAL_PRINT_P((&g_serial),PSTR("I2c Slave to TX \n"));
	twi_transmit(data,5);
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

    /* Initialize the I2C part*/
    //i2c_init(&g_i2c,I2C0, CONFIG_I2C_FREQ);

    // as slave #1
    twi_setAddress(1);
    twi_attachSlaveRxEvent(slave_rx_callback);
    twi_attachSlaveTxEvent(slave_tx_callback);
    twi_init();

    reader_init(g_shared_buf, SHARED_BUF_LEN,serial_read_line_callback);
}


int main(void){

	init();

	while (1){

		reader_poll(&g_serial);

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
	} // end of while(1)
	return 0;
}
