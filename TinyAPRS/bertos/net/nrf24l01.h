/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright (c) Davide Gironi, 2012
 *
 * -->
 *
 * \brief NRF24L01+ radio driver
 *
 * \author Davide Gironi
 *
 *
 * $WIZ$ module_name = "nrf24l01"
 * $WIZ$ module_configuration = "bertos/cfg/cfg_nrf24l01.h"
 * $WIZ$ module_depends = "spi"
 * $WIZ$ module_hw = "bertos/hw/hw_nrf24l01.h"
 */

/*
nrf24l01 lib 0x02



References:
  -  This library is based upon nRF24L01 avr lib by Stefan Engelke
     http://www.tinkerer.eu/AVRLib/nRF24L01
  -  and arduino library 2011 by J. Coliz
     http://maniacbug.github.com/RF24

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#ifndef _NRF24L01_H_
#define _NRF24L01_H_

#include "cfg/cfg_nrf24l01.h"


//address size
#define NRF24L01_ADDRSIZE 5

/**
 * \name Values for CONFIG_NRF24L01_RF24_PA.
 *
 * Select the power output of the transmitter
 * $WIZ$ nrf24l01_rf24_power = "NRF24L01_RF24_PA_MIN", "NRF24L01_RF24_PA_LOW", "NRF24L01_RF24_PA_HIGH", "NRF24L01_RF24_PA_MAX"
 */
#define NRF24L01_RF24_PA_MIN 1
#define NRF24L01_RF24_PA_LOW 2
#define NRF24L01_RF24_PA_HIGH 3
#define NRF24L01_RF24_PA_MAX 4

/**
 * \name Values for CONFIG_NRF24L01_RF24_SPEED
 *
 * Select the power output of the transmitter
 * $WIZ$ nrf24l01_rf24_speed = "NRF24L01_RF24_SPEED_250KBPS", "NRF24L01_RF24_SPEED_1MBPS", "NRF24L01_RF24_SPEED_2MBPS"
 */
#define NRF24L01_RF24_SPEED_250KBPS 1
#define NRF24L01_RF24_SPEED_1MBPS 2
#define NRF24L01_RF24_SPEED_2MBPS 3

/**
 * \name Values for CONFIG_NRF24L01_RF24_CRC
 *
 * Select the power output of the transmitter
 * $WIZ$ nrf24l01_rf24_crc = "NRF24L01_RF24_CRC_DISABLED", "NRF24L01_RF24_CRC_8", "NRF24L01_RF24_CRC_16"
 */
#define NRF24L01_RF24_CRC_DISABLED 1
#define NRF24L01_RF24_CRC_8 2
#define NRF24L01_RF24_CRC_16 3

// set retries and ack timeout
#define NRF24L01_RETR (NRF24L01_ARD_TIME << NRF24L01_REG_ARD) | (NRF24L01_ARC_RETRIES << NRF24L01_REG_ARC)

extern void nrf24l01_init(void);
extern uint8_t nrf24l01_getstatus(void);
extern uint8_t nrf24l01_readready(uint8_t* pipe);
extern void nrf24l01_read(uint8_t *data);
extern uint8_t nrf24l01_write(uint8_t *data);
extern void nrf24l01_setrxaddr(uint8_t channel, uint8_t *addr);
extern void nrf24l01_settxaddr(uint8_t *addr);
extern uint8_t nrf24_retransmissionCount(void);
extern void nrf24l01_setRX(void);
#if NRF24L01_PRINTENABLE == 1
extern void nrf24l01_printinfo(void(*prints)(const char *));
#endif

#endif
