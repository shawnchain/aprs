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


struct GPS;

/*
 * Send the beacon location
 */
void tracker_update_location(struct GPS *gps);


#endif /* TRACKER_H_ */
