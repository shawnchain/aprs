/*
 * \file tracker.h
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
 * \date 2016-10-8
 */

#ifndef TRACKER_H_
#define TRACKER_H_

void tracker_init(void);

/*
 * initialize the gps module
 * triggered by the main loop when mode switched to MOD_TRACKER
 */
void tracker_init_gps(void);

void tracker_poll(void);

#endif /* TRACKER_H_ */
