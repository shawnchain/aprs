/*
 * \file hw_soft_ser.h
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
 * \date 2015-3-13
 */

#ifndef HW_SOFT_SER_H_
#define HW_SOFT_SER_H_

#include "cfg/cfg_softser.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

typedef struct {
	uint8_t _receivePin;
	uint8_t _receiveBitMask;
	volatile uint8_t *_receivePortRegister;
	uint8_t _transmitBitMask;
	volatile uint8_t *_transmitPortRegister;

	uint16_t _rx_delay_centering;
	uint16_t _rx_delay_intrabit;
	uint16_t _rx_delay_stopbit;
	uint16_t _tx_delay;

	bool _buffer_overflow;
	bool _inverse_logic;
} SoftSerial;

void hw_softser_init(SoftSerial *pSSer, uint8_t pinRX, uint8_t pinTX);

void hw_softser_start(SoftSerial *pSSer, long speed);
void hw_softser_stop(SoftSerial *pSSer);
int hw_softser_avail(SoftSerial *pSSers);
int hw_softser_read(SoftSerial *pSSers);
int hw_softser_write(SoftSerial *pSSer, uint8_t b);
int hw_softser_print(SoftSerial *pSSer, char* str);

#define softser_init(pSSer,pinRx,pinTx) hw_softser_init(pSSer, pinRx,pinTx)
#define softser_start(pSSer,baud) hw_softser_start(pSSer,baud)
#define softser_stop(pSSer) hw_softser_stop(pSSer)
#define softser_avail(pSSer) hw_softser_avail(pSSer)
#define softser_read(pSSer) hw_softser_read(pSSer)
#define softser_write(pSSer,b) hw_softser_write(pSSer, b)
#define softser_print(pSSer,s) hw_softser_print(pSSer, s)


#endif /* HW_SOFT_SER_H_ */
