/*
lcdpcf8574 lib 0x01

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <stdint.h>
#include <stdbool.h>

#include <drv/timer.h>
#include <drv/lcd_hd44.h>
#include <cfg/cfg_lcd_hd44.h>

#include "pcf8574.h"

#include "hw/hw_lcd_pcf8574.h"




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
static lcdpos_t lcd_current_addr;



volatile uint8_t dataport = 0;


/*
** local functions
*/



/* toggle Enable Pin to initiate write */
static void
lcd_e_toggle (void)
{
   pcf8574_setoutputpinhigh (LCD_PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   pcf8574_setoutputpinlow (LCD_PCF8574_DEVICEID, LCD_E_PIN);
}


/*************************************************************************
Low-level function to write byte to LCD controller
Input:    data   byte to write to LCD
          rs     1: write data    
                 0: write instruction
Returns:  none
*************************************************************************/
static void
lcd_write (uint8_t data, uint8_t rs)
{
   if (rs)                      /* write data        (RS=1, RW=0) */
      dataport |= BV (LCD_RS_PIN);
   else                         /* write instruction (RS=0, RW=0) */
      dataport &= ~BV (LCD_RS_PIN);
   dataport &= ~BV (LCD_RW_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);

   /* output high nibble first */
   dataport &= ~BV (LCD_DATA3_PIN);
   dataport &= ~BV (LCD_DATA2_PIN);
   dataport &= ~BV (LCD_DATA1_PIN);
   dataport &= ~BV (LCD_DATA0_PIN);
   if (data & 0x80)
      dataport |= BV (LCD_DATA3_PIN);
   if (data & 0x40)
      dataport |= BV (LCD_DATA2_PIN);
   if (data & 0x20)
      dataport |= BV (LCD_DATA1_PIN);
   if (data & 0x10)
      dataport |= BV (LCD_DATA0_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();

   /* output low nibble */
   dataport &= ~BV (LCD_DATA3_PIN);
   dataport &= ~BV (LCD_DATA2_PIN);
   dataport &= ~BV (LCD_DATA1_PIN);
   dataport &= ~BV (LCD_DATA0_PIN);
   if (data & 0x08)
      dataport |= BV (LCD_DATA3_PIN);
   if (data & 0x04)
      dataport |= BV (LCD_DATA2_PIN);
   if (data & 0x02)
      dataport |= BV (LCD_DATA1_PIN);
   if (data & 0x01)
      dataport |= BV (LCD_DATA0_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();

   /* all data pins high (inactive) */
   dataport |= BV (LCD_DATA0_PIN);
   dataport |= BV (LCD_DATA1_PIN);
   dataport |= BV (LCD_DATA2_PIN);
   dataport |= BV (LCD_DATA3_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);
}


/*************************************************************************
Low-level function to read byte from LCD controller
Input:    rs     1: read data    
                 0: read busy flag / address counter
Returns:  byte read from LCD controller
*************************************************************************/
static uint8_t
lcd_read (uint8_t rs)
{
   uint8_t data;

   if (rs)                      /* write data        (RS=1, RW=0) */
      dataport |= BV (LCD_RS_PIN);
   else                         /* write instruction (RS=0, RW=0) */
      dataport &= ~BV (LCD_RS_PIN);
   dataport |= BV (LCD_RW_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);

   pcf8574_setoutputpinhigh (LCD_PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   data = pcf8574_getoutputpin (LCD_PCF8574_DEVICEID, LCD_DATA0_PIN) << 4; /* read high nibble first */
   pcf8574_setoutputpinlow (LCD_PCF8574_DEVICEID, LCD_E_PIN);

   timer_udelay (1);;              /* Enable 500ns low       */

   pcf8574_setoutputpinhigh (LCD_PCF8574_DEVICEID, LCD_E_PIN);
   timer_udelay (1);;
   data |= pcf8574_getoutputpin (LCD_PCF8574_DEVICEID, LCD_DATA0_PIN) & 0x0F; /* read low nibble        */
   pcf8574_setoutputpinlow (LCD_PCF8574_DEVICEID, LCD_E_PIN);

   return data;
}


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
Set illumination pin
*************************************************************************/
void
lcd_backlight (uint8_t onoff)
{
   if ((onoff ^ LCD_LED_POL) & 1)
      dataport &= ~BV (LCD_LED_PIN);
   else
      dataport |= BV (LCD_LED_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);
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
void
lcd_hw_init (void)
{
#if LCD_PCF8574_INIT == 1
   //init pcf8574
   pcf8574_init ();
#endif

   dataport = 0;
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);

   timer_delay (16);               /* wait 16ms or more after power-on       */

   /* initial write to lcd is 8bit */
   dataport |= BV (LCD_DATA1_PIN); // BV(LCD_FUNCTION)>>4;
   dataport |= BV (LCD_DATA0_PIN); // BV(LCD_FUNCTION_8BIT)>>4;
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);

   lcd_e_toggle ();
   timer_udelay (4992);                /* delay, busy flag can't be checked here */

   /* repeat last command */
   lcd_e_toggle ();
   timer_udelay (64);                  /* delay, busy flag can't be checked here */

   /* repeat last command a third time */
   lcd_e_toggle ();
   timer_udelay (64);                  /* delay, busy flag can't be checked here */

   /* now configure for 4bit mode */
   dataport &= ~BV (LCD_DATA0_PIN);
   pcf8574_setoutput (LCD_PCF8574_DEVICEID, dataport);
   lcd_e_toggle ();
   timer_udelay (64);                  /* some displays need this additional delay */

   /* from now the LCD only accepts 4 bit I/O, we can use lcd_command() */

   lcd_command(LCD_CMD_SETFUNC);      /* function set: display lines  */

   lcd_command (LCD_CMD_DISPLAY_OFF);  /* display off                  */
   lcd_clrscr ();               /* display clear                */
   lcd_command (LCD_CMD_DISPLAYMODE);  /* set entry mode               */
   lcd_command (LCD_CMD_DISPLAY_OFF | LCD_CMD_ON_DISPLAY);      /* display/cursor control       */

}                               /* lcd_init */
