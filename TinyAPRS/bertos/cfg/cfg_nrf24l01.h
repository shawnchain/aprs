/*
nrf24l01 lib 0x02

copyright (c) Davide Gironi, 2012

References:
  -  This library is based upon nRF24L01 avr lib by Stefan Engelke
     http://www.tinkerer.eu/AVRLib/nRF24L01
  -  and arduino library 2011 by J. Coliz
     http://maniacbug.github.com/RF24

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#ifndef _CFG_NRF24L01_H_
#define _CFG_NRF24L01_H_

/**
 * Power setting
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "nrf24l01_rf24_power"
 */
#define NRF24L01_RF24_PA NRF24L01_RF24_PA_MAX

/**
 * Speed setting
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "nrf24l01_rf24_speed"
 */
#define NRF24L01_RF24_SPEED NRF24L01_RF24_SPEED_1MBPS

/**
 * crc setting
 * $WIZ$ type = "enum"
 * $WIZ$ value_list = "nrf24l01_rf24_crc"
 */
#define NRF24L01_RF24_CRC NRF24L01_RF24_CRC_16


/**
 * 2.4GHz channel number - used for both transmit and receive
 *
 * $WIZ$ type = "int"; min = 1; max = 126
 */
#define NRF24L01_CH 76

/**
 * Payload length. The device is quite limited in packet size but its fast!
 *
 * $WIZ$ type = "int"; min = 1; max = 32
 */
#define NRF24L01_PAYLOAD 32

/**
 * Auto ack - allows the transmit side to check for delivery
 *
 * $WIZ$ type = "boolean"
 */
#define NRF24L01_ACK 1

/**
 * Maximum number of retries in auto acknowledge mode
 * A value of 0 disables ree-transmission
 *
 * $WIZ$ type = "int"; min = 0; max = 15
 */
#define NRF24L01_ARC_RETRIES 7

/**
 * Timeout before attempting a retry in auto acknowldge mode
 * The value is in units of (n * 250) + 250
 * Recommended minimum value is 5 (1500uS) to allow transmit to take place
 *
 * $WIZ$ type = "int"; min = 0; max = 15
 */
#define NRF24L01_ARD_TIME 5


//enable / disable pipe
#define NRF24L01_ENABLEDP0 1    //pipe 0
#define NRF24L01_ENABLEDP1 1    //pipe 1
#define NRF24L01_ENABLEDP2 0    //pipe 2
#define NRF24L01_ENABLEDP3 0    //pipe 3
#define NRF24L01_ENABLEDP4 0    //pipe 4
#define NRF24L01_ENABLEDP5 0    //pipe 5

//pipe address. For pipes 1-5, only the last 8 bits can be changed
#define NRF24L01_ADDRP0 {0xE8, 0xE8, 0xF0, 0xF0, 0xE2}  //pipe 0, 5 byte address
#define NRF24L01_ADDRP1 {0xC1, 0xC2, 0xC2, 0xC2, 0xC2}  //pipe 1, 5 byte address
#define NRF24L01_ADDRP2 {0xC1, 0xC2, 0xC2, 0xC2, 0xC3}  //pipe 2, 5 byte address
#define NRF24L01_ADDRP3 {0xC1, 0xC2, 0xC2, 0xC2, 0xC4}  //pipe 3, 5 byte address
#define NRF24L01_ADDRP4 {0xC1, 0xC2, 0xC2, 0xC2, 0xC5}  //pipe 4, 5 byte address
#define NRF24L01_ADDRP5 {0xC1, 0xC2, 0xC2, 0xC2, 0xC6}  //pipe 5, 5 byte address
#define NRF24L01_ADDRTX {0xE8, 0xE8, 0xF0, 0xF0, 0xE2}  //tx default address*/

 //enable print info function
#define NRF24L01_PRINTENABLE 1


#endif
