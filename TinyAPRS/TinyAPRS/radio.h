/*
 * \file radio.h
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
 * \date 2015-3-21
 */

#ifndef RADIO_H_
#define RADIO_H_

#include "cfg/cfg_radio.h"
#include "hw/hw_softser.h"

extern SoftSerial *radioPort;
void radio_init(uint32_t freq);


#endif /* RADIO_H_ */
