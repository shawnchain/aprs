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
 * \brief Terminal emulator driver.
 *
 * Uses kfile interface to stream data to an LCD display device such as an HD44780.
 *
 * Control codes and cursor addressing based on the old Newbury Data Recording 8000
 * series dumb terminal which is nice and simple.
 *
 * Cursor positioning is done with [0x16][0x20+row][0x20+col]. All other codes are single control characters.
 * The application generates a stream with the following control codes, the terminal emulator layer interpretss
 * the codes for a specific device -
 * eg. ANSI sequencies to send to a serial terminal,
 *     direct cursor addressing on an LCD panel
 *
 * \author Robin Gilks <g8ecj@gilks.org>
 *
 */

#include "term.h"

#include "lcd_hd44780.h"
#include "timer.h"
#include <string.h>


#define TERM_STATE_NORMAL   0x00    /**< state that indicates we're passing data straight through */
#define TERM_STATE_ROW      0x01    /**< state that indicates we're waiting for the row address */
#define TERM_STATE_COL      0x02    /**< state that indicates we're waiting for the column address */


#define CURSOR_ON           1
#define BLINK_ON            2



#if 1
#include "cfg/cfg_lcd_hd44780.h"
#if ((CONFIG_TERM_COLS != CONFIG_LCD_COLS) || (CONFIG_TERM_ROWS != CONFIG_LCD_ROWS))
	#error "Terminal module only supports HD44780 LCD displays at present and they must be the same size!"
#endif
#endif

/**
 * \brief Write a character to the display, interpreting control codes in the data stream.
 * Uses a simple set of control codes from an ancient dumb terminal.
 *
 */
static void term_putchar(uint8_t c, struct Term *fds)
{
	uint8_t i;

	switch (fds->state)
	{
	case TERM_STATE_NORMAL: /* state that indicates we're passing data straight through */
		switch (c)
		{
		case TERM_CPC:     /* Cursor position prefix - followed by row + column */
			fds->state = TERM_STATE_ROW;      // wait for row value
			break;

		case TERM_CLR:     /* Clear screen */
			fds->addr = 0;
			lcd_command(LCD_CMD_CLEAR);
			timer_delay(2);
#if CONFIG_TERM_SCROLL == 1
			for (i = 0; i < CONFIG_TERM_COLS * CONFIG_TERM_ROWS; i++)
				fds->scrollbuff[i] = ' ';
#endif
			break;

		case TERM_HOME:    /* Home */
			fds->addr = 0;
			break;

		case TERM_UP:      /* Cursor up  - no scroll but wraps to bottom */
			fds->addr -= CONFIG_TERM_COLS;
			if (fds->addr < 0)
				fds->addr += (CONFIG_TERM_COLS * CONFIG_TERM_ROWS);
			break;

#if CONFIG_TERM_SCROLL == 1
		case TERM_DOWN:    /* Cursor down - no scroll but wraps to top */
			fds->addr += CONFIG_TERM_COLS;
			fds->addr %= CONFIG_TERM_COLS * CONFIG_TERM_ROWS;
			break;
#endif

		case TERM_LEFT:    /* Cursor left - wrap top left to bottom right  */
			if (--fds->addr < 0)
				fds->addr += (CONFIG_TERM_COLS * CONFIG_TERM_ROWS);
			break;

		case TERM_RIGHT:   /* Cursor right */
			if (++fds->addr >= (CONFIG_TERM_COLS * CONFIG_TERM_ROWS))
				fds->addr = 0;               // wrap bottom right to top left
			break;

		case TERM_CR:    /* Carriage return */
				for (i = fds->addr; (i % CONFIG_TERM_COLS) !=0; i++)
				{
#if CONFIG_TERM_SCROLL == 1
					c = fds->scrollbuff[i] = ' ';
#endif
					lcd_putc(i, ' ');
				}
			fds->addr -= (fds->addr % CONFIG_TERM_COLS);
			break;

		case TERM_LF:    /* Line feed. Does scroll on last line if enabled else does cursor down */
#if CONFIG_TERM_SCROLL == 1
			if ((fds->addr / CONFIG_TERM_COLS) == (CONFIG_TERM_ROWS - 1))         // see if on last row
			{
				lcd_command(LCD_CMD_CLEAR);
				timer_delay(2);
				for (i = 0; i < CONFIG_TERM_COLS * (CONFIG_TERM_ROWS - 1); i++)
				{
					c = fds->scrollbuff[i + CONFIG_TERM_COLS];
					lcd_putc(i, c);
					fds->scrollbuff[i] = c;
				}
			}
			else
#endif
			{
				if (fds->addr < (CONFIG_TERM_COLS * (CONFIG_TERM_ROWS - 1)))
					fds->addr += CONFIG_TERM_COLS;
			}
			break;

		case TERM_CURS_ON:     /* Cursor ON */
			fds->cursor |= CURSOR_ON;
			lcd_display(1, fds->cursor & CURSOR_ON, fds->cursor & BLINK_ON);
			break;

		case TERM_CURS_OFF:    /* Cursor OFF */
			fds->cursor &= ~CURSOR_ON;
			lcd_display(1, fds->cursor & CURSOR_ON, fds->cursor & BLINK_ON);
			break;

		case TERM_BLINK_ON:    /* Cursor blink ON */
			fds->cursor |= BLINK_ON;
			lcd_display(1, fds->cursor & CURSOR_ON, fds->cursor & BLINK_ON);
			break;

		case TERM_BLINK_OFF:   /* Cursor blink OFF */
			fds->cursor &= ~BLINK_ON;
			lcd_display(1, fds->cursor & CURSOR_ON, fds->cursor & BLINK_ON);
			break;

		default:
			lcd_putc(fds->addr, c);
#if CONFIG_TERM_SCROLL == 1
			fds->scrollbuff[fds->addr] = c;
#endif
			if (++fds->addr >= (CONFIG_TERM_COLS * CONFIG_TERM_ROWS))
				fds->addr = 0;               // wrap bottom right to top left
		}
		break;


	case TERM_STATE_ROW:  /* state that indicates we're waiting for the row address */
		fds->tmp = c - TERM_ROW;         /* cursor position row offset (0 based) */
		fds->state = TERM_STATE_COL;     // wait for row value
		break;

	case TERM_STATE_COL:  /* state that indicates we're waiting for the column address */
		i = (fds->tmp * CONFIG_TERM_COLS) + (c - TERM_COL);
		if (i < (CONFIG_TERM_COLS * CONFIG_TERM_ROWS))
			fds->addr = i;
		fds->state = TERM_STATE_NORMAL;  // return to normal processing - cursor address complete
		break;
	}
}



/**
 * \brief Write a buffer to LCD display.
 *
 * \param fd caste term context.
 * \param _buf pointer to buffer to write
 * \param size length of buffer.
 * \return 0 if OK, EOF in case of error.
 *
 */
static size_t term_write(struct KFile *fd, const void *_buf, size_t size)
{
	Term *fds = TERM_CAST(fd);
	const char *buf = (const char *)_buf;
	const size_t i = size;

	while (size--)
	{
		term_putchar(*buf++, fds);
	}
	return i;
}


#if CONFIG_TERM_SCROLL == 1
static size_t term_read(struct KFile *fd, void *_buf, size_t size)
{
	Term *fds = TERM_CAST(fd);
	uint8_t *buf = (uint8_t *)_buf;
	const size_t i = size;

	if (fds->readptr < 0)
	{
		fds->readptr = 0;
		return 0;
	}
	while (size--)
	{
		*buf++ = fds->scrollbuff[fds->readptr];
		fds->readptr++;
		if (fds->readptr >= (CONFIG_TERM_ROWS * CONFIG_TERM_COLS))
		{
			fds->readptr = -1;
			return i - size;
		}
	}
	return i;
}

static kfile_off_t term_seek(struct KFile *fd, kfile_off_t offset, KSeekMode whence)
{
	(void) offset;
	(void) whence;
	Term *fds = TERM_CAST(fd);
	return fds->addr;
}

#endif

/**
 * \brief Initialise the terminal context
 *
 * \param fds term context.
 */

void term_init(struct Term *fds)
{
	memset(fds, 0, sizeof(*fds));

	DB(fds->fd._type = KFT_TERM);
	fds->fd.write = term_write;            // leave all but the write function as default
#if CONFIG_TERM_SCROLL == 1
	fds->fd.read = term_read;              // provide a read function if we have a scroll buffer
	fds->fd.seek = term_seek;
#endif
	fds->state = TERM_STATE_NORMAL;        // start at known point
	lcd_display(1, 0, 0);                  // display on, cursor & blink off
	fds->cursor = 0;                       // local copy of cursor & blink state
	term_putchar(TERM_CLR, fds);           // clear screen, init address pointer

}


