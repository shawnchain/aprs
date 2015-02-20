/*
 * \file cfg_console.h
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
 * \date 2015-2-19
 */

#ifndef CFG_CONSOLE_H_
#define CFG_CONSOLE_H_


#define CONSOLE_SERIAL_DEBUG true					// Debugging flag of the console serial port

#define CONSOLE_SERIAL_READ_TIMEOUT 0UL             // Read timeout control. set 0 to disable

#define CONSOLE_SERIAL_BUF_LEN 64 					// The serial console command buffer

#define CONSOLE_MAX_COMMAND	16						// How many AT commands to support

#endif /* CFG_CONSOLE_H_ */
