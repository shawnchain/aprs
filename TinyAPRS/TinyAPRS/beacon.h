/*
 * \file beacon.h
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
 * \date 2015-2-13
 */

#ifndef BEACON_H_
#define BEACON_H_

#include <stdbool.h>

#define CONFIG_BEACON_ENABLED 1

struct AX25Ctx; // forward declaration

/*
 * Initialize the beacon module
 */
void beacon_init(AX25Ctx *ctx);

/*
 * Runloop of the beacon module
 */
void beacon_poll(void);

/*
 * Force send the beacon message
 */
void beacon_send(void);

/*
 * Enable the beacon module
 */
bool beacon_enabled(void);

/*
 * Set the beacon module state
 */
void beacon_set_enabled(bool flag);

/*
 * Send test beacon
 */
void beacon_send_test(uint8_t count);

#endif /* BEACON_H_ */
