#ifndef __BOARD_H__
#define __BOARD_H__

#include "led.h"
#include "aht20.h"
#include "usart.h"
// #include "delay.h"
#include "lcd.h"
#include "font.h"
#include "img.h"
extern led_desc_t led1;
extern led_desc_t led2;
extern aht20_desc_t aht20;
extern usart_desc_t usart1;
extern lcd_desc_t lcd;
void board_lowlevel_init(void);
void TIM3_Int_Init(uint16_t arr, uint16_t psc);
#endif