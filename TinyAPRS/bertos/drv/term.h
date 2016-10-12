/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Copyright 2012 Robin Gilks (g8ecj at gilks.org)
 *
 * -->
 *
 *
 * \brief Terminal emulator driver.
 *
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 * $WIZ$ module_name = "term"
 * $WIZ$ module_configuration = "bertos/cfg/cfg_term.h"
 *
 */


#ifndef TERM_H_
#define TERM_H_


#include <cfg/cfg_lcd_hd44780.h>
#include "lcd_hd44780.h"

#include "cfg/cfg_term.h"

#include "cfg/compiler.h"
#include "io/kfile.h"


/**
 * \name Values for CONFIG_TERM_ROWS.
 *
 * Select the number of rows which are available
 * on the terminal Display.
 * $WIZ$ terminal_rows = "TERMINAL_ROWS_2", "TERMINAL_ROWS_4"
 */
#define TERMINAL_ROWS_2 2
#define TERMINAL_ROWS_4 4

/**
 * \name Values for CONFIG_TERM_COLS.
 *
 * Select the number of columns which are available
 * on the terminal Display.
 * $WIZ$ terminal_cols = "TERMINAL_COLS_16", "TERMINAL_COLS_20"
 */
#define TERMINAL_COLS_16 16
#define TERMINAL_COLS_20 20



/**
 * \defgroup term_driver Terminal Emulator driver
 * \ingroup drivers
 * \{
 */

#define TERM_CPC     0x16     /**< Cursor position prefix - followed by row + column */
#define TERM_ROW     0x20     /**< cursor position row offset */
#define TERM_COL     0x20     /**< cursor position column offset */
#define TERM_CLR     0x1f     /**< Clear screen */
#define TERM_HOME    0x1d     /**< Home */
#define TERM_UP      0x0b     /**< Cursor up */
#define TERM_DOWN    0x06     /**< Cursor down */
#define TERM_LEFT    0x08     /**< Cursor left */
#define TERM_RIGHT   0x18     /**< Cursor right */
#define TERM_CR      0x0d     /**< Carriage return */
#define TERM_LF      0x0a     /**< Line feed (scrolling version of cursor down!) */
#define TERM_CURS_ON    0x0f     /**< Cursor ON */
#define TERM_CURS_OFF   0x0e     /**< Cursor OFF */
#define TERM_BLINK_ON   0x1c     /**< Cursor blink ON */
#define TERM_BLINK_OFF  0x1e     /**< Cursor blink OFF */

/**
 * ID for terminal.
 */
#define KFT_TERM MAKE_ID('T', 'E', 'R', 'M')



/** Terminal handle structure */
typedef struct Term
{
	KFile fd;                 /** Terminal has a KFile struct implementation */
	uint8_t state;            /** What to expect next in the data stream */
	uint8_t tmp;              /** used whilst calculating new address from row/column */
	int16_t addr;             /** LCD address to write to */
	uint8_t cursor;           /** state of cursor (ON/OFF, blink) */
#if CONFIG_TERM_SCROLL == 1
	uint8_t scrollbuff[CONFIG_TERM_COLS * CONFIG_TERM_ROWS];
	int16_t readptr;
#endif
} Term;


	/**
	 * \defgroup term_api Terminal API
	 * With this driver you can stream text to an LCD screen and control its appearance with control codes.
	 * As well as the simple CR/LF codes, there are clear, up/down/left/right and direct cursor addesses codes available.
	 *
	 * API usage example:
	 * \code
	 * static Term term;
	 * static const char lcd_sdcard[8] = { 0x1f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x1c };   // sd card - bent rectangle!
	 * lcd_hw_init ();
	 * lcd_display (1, 0, 0);                // display on, cursor off, blink off
	 * lcd_remapChar (lcd_sdcard, 2);        // put the sd card symbol on character 0x02
	 * term_init(&term);
	 * // clear screen, print text
	 * kfile_printf(&term.fd, "%c", TERM_CLR);
	 * kfile_printf(&term.fd, "On line 1 I hope!!\r\n");
	 * kfile_printf(&term.fd, "On line 2\r\nand now line 3\r\n");
	 * kfile_printf(&term.fd, "On line 4\r");
	 * kfile_printf(&term.fd, "\nScrolled - this on 4");
	 * // indicate there is an sd card plugged in (or not!!) with icon or space
	 * kfile_printf (&term.fd, "%c%c%c%c", TERM_CPC, TERM_ROW + 0, TERM_COL + 19, sd_ok ? 0x02 : 0x20);
	 * \endcode
	 * \{
	 */




INLINE Term * TERM_CAST(KFile *fd)
{
	ASSERT(fd->_type == KFT_TERM);
	return (Term *)fd;
}


void term_init(struct Term *fds);

		/** \} */ //defgroup term_api
	/** \} */ //ingroup drivers
/** \} */ //defgroup term_driver



#endif /* TERM_H_ */


