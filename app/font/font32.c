#include "stm32f4xx.h"
#include "led_desc.h"
#include "led.h"
#include "aht20_desc.h"
#include "aht20.h"
#include "usart_desc.h"
#include "usart.h"
#include "lcd_desc.h"
#include "lcd.h"
#include "delay.h"
static struct led_desc led1_desc = {GPIOA, GPIO_Pin_6, Bit_RESET, Bit_SET};
static struct led_desc led2_desc = {GPIOA, GPIO_Pin_7, Bit_RESET, Bit_SET};

led_desc_t led1 = &led1_desc;
led_desc_t led2 = &led2_desc;
static struct aht20_desc aht20_desc = 
{
	.I2C = I2C1,
	.GPort = GPIOB,
	.SCLPin = GPIO_Pin_6,
	.SDAPin = GPIO_Pin_7,
	.SCLPinSource = GPIO_PinSource6,
	.SDAPinSource = GPIO_PinSource7,
};
aht20_desc_t aht20 = &aht20_desc;

static struct usart_desc usart1_desc = 
{
	.Port = USART1,
	.GPort = GPIOA,
	.TxPin = GPIO_Pin_9,
	.RxPin = GPIO_Pin_10,
	.IRQn =  USART1_IRQn,
};
usart_desc_t usart1 = &usart1_desc;

static struct lcd_desc lcd_desc = 
{
	.SPI = SPI2,
	.Port = GPIOB,
	.RSTPin = GPIO_Pin_9,
	.DCPin = GPIO_Pin_10,
	.CSPin = GPIO_Pin_12,
	.BLPin = GPIO_Pin_0,
	.SCKPin = GPIO_Pin_13,
	.MOSIPin = GPIO_Pin_15,
	.SCKPinsource = GPIO_PinSource13,
	.MOSIPinsource = GPIO_PinSource15,
	.CSPinsource = GPIO_PinSource12,
};
lcd_desc_t lcd = &lcd_desc;


void board_lowlevel_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
}

