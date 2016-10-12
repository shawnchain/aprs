/*
lcdpcf8574 lib 0x01

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <stdint.h>
#include <stdbool.h>

#include <drv/timer.h>

#include "cfg/cfg_lcd_hd44780.h"
#include <drv/lcd_hd44780.h>

#include "hw/hw_lcd_hd44780.h"




#if CONFIG_LCD_ADDRESS_FAST == 1
#define lcd_address(x) lcd_address[x]
/**
 * Addresses of LCD display character positions, expanded
 * for faster access (DB7 = 1).
 */
static const uint8_t lcd_address[] =
{
	/* row 0 */
	0x80, 0x81, 0x82, 0x83,
	0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x8B,
	0x8C, 0x8D, 0x8E, 0x8F,
#if CONFIG_LCD_COLS > 16
	0x90, 0x91, 0x92, 0x93,
#endif

	/* row 1 */
	0xC0, 0xC1, 0xC2, 0xC3,
	0xC4, 0xC5, 0xC6, 0xC7,
	0xC8, 0xC9, 0xCA, 0xCB,
	0xCC, 0xCD, 0xCE, 0xCF,
#if CONFIG_LCD_COLS > 16
	0xD0, 0xD1, 0xD2, 0xD3,
#endif

#if CONFIG_LCD_ROWS > 2
	/* row 2 */
	0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0x9B,
	0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3,
#if CONFIG_LCD_COLS > 16
	0xA4, 0xA5, 0xA6, 0xA7,
#endif

	/* row 3 */
	0xD4, 0xD5, 0xD6, 0xD7,
	0xD8, 0xD9, 0xDA, 0xDB,
	0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3,
#if CONFIG_LCD_COLS > 16
	0xE4, 0xE5, 0xE6, 0xE7,
#endif

#endif /* CONFIG_LCD_ROWS > 2 */
};

STATIC_ASSERT(countof(lcd_address) == CONFIG_LCD_ROWS * CONFIG_LCD_COLS);
#else  /* CONFIG_LCD_ADDRESS_FAST == 0 */

static const uint8_t col_address[] =
{
	0x80,
	0xC0,
#if CONFIG_LCD_ROWS > 2
	0x94,
	0xD4
#endif
};
STATIC_ASSERT(countof(col_address) == CONFIG_LCD_ROWS);
/**
 * Addresses of LCD display character positions, calculated runtime to save RAM
 */
static uint8_t lcd_address(uint8_t addr)
{
	return col_address[addr / CONFIG_LCD_COLS] + addr % CONFIG_LCD_COLS;
}
#endif /* CONFIG_LCD_ADDRESS_FAST */

/**
 * Current display position. We remember this to optimize
 * LCD output by avoiding to set the address every time.
 */
static uint8_t lcd_current_addr;




/*************************************************************************
loops while lcd is busy, returns address counter
*************************************************************************/
static uint8_t
lcd_waitbusy (void)
{
   register uint8_t c;

   /* wait until busy flag is cleared */
   while ((c = lcd_read (0)) & LCD_RESP_BUSY)
   {
   }

   /* the address counter is updated 4us after the busy flag is cleared */
   timer_udelay (2);

   /* now read the address counter */
   return (lcd_read (0));       // return address counter

}                               /* lcd_waitbusy */

/*************************************************************************
Send data byte to LCD controller 
Input:   data to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
static void
lcd_data (uint8_t data)
{
   lcd_waitbusy ();
   lcd_write (data, 1);
}


/*
** PUBLIC FUNCTIONS 
*/

/*************************************************************************
Send LCD controller instruction command
Input:   instruction to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
void
lcd_command (uint8_t cmd)
{
   lcd_waitbusy ();
   lcd_write (cmd, 0);
}



/*************************************************************************
Clear display and set cursor to home position
*************************************************************************/
void
lcd_clrscr (void)
{
   lcd_command (LCD_CMD_CLEAR);
   lcd_current_addr = 0;
}



/*************************************************************************
Set cursor to home position
*************************************************************************/
void
lcd_home (void)
{
   lcd_command (LCD_CMD_HOME);
   lcd_current_addr = 0;
}



/*************************************************************************
 * Allow control of the display on/off, cursor on/off and if the 
 * cursor is on then if it blinks or not.
*************************************************************************/
void lcd_display(bool display, bool cursor, bool blink )
{
	uint8_t value = LCD_CMD_DISPLAY_OFF;
	value |= display ? LCD_CMD_ON_DISPLAY : 0; 
	value |= cursor ? LCD_CMD_ON_CURSOR : 0; 
	value |= blink ? LCD_CMD_ON_BLINK : 0; 
	lcd_command(value);
}


/*************************************************************************
Display character at defined cursor position 
Input:    address (offset from start of display), character to be displayed                                       
Returns:  none
*************************************************************************/
void
lcd_putc (uint8_t addr, uint8_t c)
{
	if (addr != lcd_current_addr)
		lcd_command(lcd_address(addr));

	lcd_waitbusy();
	lcd_data(c);
	lcd_current_addr = addr + 1;

	/* If we are at end of display wrap the address to 0 */
	if (lcd_current_addr == CONFIG_LCD_COLS * CONFIG_LCD_ROWS)
		lcd_current_addr = 0;

	/* If we are at the end of a row put the cursor at the beginning of the next */
	if (!(lcd_current_addr % CONFIG_LCD_COLS))
		lcd_command(lcd_address(lcd_current_addr));
}                               /* lcd_putc */


/**
 * Remap the glyph of a character.
 *
 * glyph - bitmap of 8x8 bits.
 * code - must be 0-7 for the Hitachi LCD-II controller.
 */
void lcd_remapChar(const char *glyph, char code)
{
	int i;

	/* Set CG RAM address */
	lcd_command((uint8_t)(LCD_CMD_SET_CGRAMADDR | (code << 3)));

	/* Write bitmap data */
	for (i = 0; i < 8; i++)
	{
		lcd_waitbusy();
		lcd_data(glyph[i]);
	}

	/* Move back to original address */
	lcd_command(lcd_address(lcd_current_addr));
}


/*************************************************************************
Initialize display and select type of cursor 
Input:    dispAttr LCD_DISP_OFF            display off
                   LCD_DISP_ON             display on, cursor off
                   LCD_DISP_ON_CURSOR      display on, cursor on
                   LCD_DISP_CURSOR_BLINK   display on, cursor on flashing
Returns:  none
*************************************************************************/
void lcd_init(void)
{
   lcd_hw_init();


   lcd_command(LCD_CMD_SETFUNC);      /* function set: display lines  */

   lcd_command (LCD_CMD_DISPLAY_OFF);  /* display off                  */
   lcd_clrscr ();               /* display clear                */
   lcd_command (LCD_CMD_DISPLAYMODE);  /* set entry mode               */
   lcd_command (LCD_CMD_DISPLAY_OFF | LCD_CMD_ON_DISPLAY);      /* display/cursor control       */
}
