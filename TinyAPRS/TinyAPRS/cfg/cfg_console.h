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


#define CONSOLE_TEST_COMMAND_ENABLED 0				// enable test command "!n" or "AT+TEST=n"
#define CONSOLE_HELP_COMMAND_ENABLED 0				// enable help command "?" or "AT+HELP"
#define CONSOLE_SETTINGS_COMMANDS_ENABLED 0			// Disable console when the config tool is ready

#if CONSOLE_SETTINGS_COMMANDS_ENABLED
	#define CONSOLE_MAX_COMMAND	12					// How many AT commands to support
	#define CONSOLE_SERIAL_BUF_LEN 64 				// The serial console command buffer
#else
	#define CONSOLE_MAX_COMMAND	4					// How many AT commands to support
	#define CONSOLE_SERIAL_BUF_LEN 32 				// The serial console command buffer
#endif

#endif /* CFG_CONSOLE_H_ */
