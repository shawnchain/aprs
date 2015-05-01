/*
 * \file digi.h
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

#ifndef DIGI_H_
#define DIGI_H_

#include "cfg/cfg_digi.h"
#include <stdbool.h>

void digi_init(void);

struct AX25Msg *msg;
bool digi_handle_aprs_message(struct AX25Msg *msg);


#endif /* DIGI_H_ */
