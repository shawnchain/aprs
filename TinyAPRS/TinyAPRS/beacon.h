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

#include "cfg/cfg_beacon.h"

#include <stdbool.h>

// Beacon module callback
typedef void (*beacon_exit_callback_t)(void);

/*
 * Initialize the beacon module
 */
void beacon_init(beacon_exit_callback_t exitcb);

/*
 * Runloop of the beacon module
 */
void beacon_poll(void);

/*
 * Send the beacon text payload
 */
void beacon_send_text(void);

struct GPS;
/*
 * Send the beacon location
 */
void beacon_send_location(struct GPS *gps);

/*
 * Set beacon repeats. 0 means stop, -1 means infinite send
 * Beacon will exit when reaches the max repeat count
 */
void beacon_set_repeats(int8_t repeats);

#endif /* BEACON_H_ */
