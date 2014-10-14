#ifndef HW_LCD_4884_H
#define HW_LCD_4884_H

#include "cfg/cfg_arch.h"
#include <stdint.h>

void lcd_4884_init(void);

void lcd_4884_sendCommand(uint8_t type, uint8_t data);


void lcd_4884_home(void);
void lcd_4884_setCursor(uint8_t column, uint8_t line);
void lcd_4884_write(uint8_t achar);
void lcd_4884_clear(void);
void lcd_4884_clearLine(uint8_t aline);

void lcd_4884_writeString(const char* str,uint8_t line);

void lcd_4884_setEnable(uint8_t enable);


#endif
