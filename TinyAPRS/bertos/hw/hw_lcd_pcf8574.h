/*
lcdpcf8574 lib 0x01

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.

References:
  + based on lcd library by Peter Fleury
    http://jump.to/fleury
*/


#ifndef HW_LCD_PCF8574_H
#define HW_LCD_PCF8574_H

#define LCD_PCF8574_INIT 1      //init pcf8574

#define LCD_PCF8574_DEVICEID  0 //device id, addr = pcf8574 base addr + LCD_PCF8574_DEVICEID


#define LCD_DATA0_PIN    /* Implement me! */    /**< pin for 4bit data bit 0     */
#define LCD_DATA1_PIN    /* Implement me! */    /**< pin for 4bit data bit 1     */
#define LCD_DATA2_PIN    /* Implement me! */    /**< pin for 4bit data bit 2     */
#define LCD_DATA3_PIN    /* Implement me! */    /**< pin for 4bit data bit 3     */
#define LCD_RS_PIN       /* Implement me! */    /**< pin  for RS line            */
#define LCD_RW_PIN       /* Implement me! */    /**< pin  for RW line            */
#define LCD_E_PIN        /* Implement me! */    /**< pin  for Enable line        */
#define LCD_LED_PIN      /* Implement me! */    /**< pin  for Led                */
#define LCD_LED_POL      /* Implement me! */    /**< polarity of Led pin (1=+ve) */


/*@}*/
#endif //HW_LCD_PCF8574_H
