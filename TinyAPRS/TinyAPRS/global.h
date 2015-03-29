/*
 * \file global.h
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
 * \date 2015-3-30
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

struct Serial;
extern struct Serial g_serial;

struct AX25Ctx;
extern struct AX25Ctx g_ax25;

struct Afsk;
extern struct Afsk g_afsk;

#endif /* GLOBAL_H_ */
