#ifndef __LCD_H__
#define __LCD_H_

#include <stdint.h>
#include <stdbool.h>
#include "font.h"
#include "img.h"
#define mkcolor(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
struct lcd_desc;

typedef struct lcd_desc *lcd_desc_t;

void lcd_init(lcd_desc_t lcd);
void lcd_fillcolor(lcd_desc_t lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void lcd_clear(lcd_desc_t lcd);  // ÇåÆÁº¯Êý
void lcd_show_string(lcd_desc_t lcd,uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, const font_t* font);
void lcd_show_img(lcd_desc_t lcd,uint16_t x, uint16_t y, const img_t* img);

#endif // !__LCD_H_
