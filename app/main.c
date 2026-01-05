#include "board.h"
#include <stdio.h>

int main(void)
{
	uint32_t count = 0;
	float temp, humi;
	board_lowlevel_init();
	led_init(led1);
	led_init(led2);
	cpu_tick_init();
	usart_init(usart1);
	aht20_init(aht20);
	lcd_init(lcd);
	lcd_clear(lcd);
	// lcd_fillcolor(lcd, 0,0,239,319,mkcolor(255, 0, 0));

	lcd_show_string(lcd, 29, 0, "Hello World!温度湿度温度湿度温度温", mkcolor(255, 255, 255), mkcolor(0, 0, 0), &font16);
	// lcd_show_img(lcd, 0, 0, &img_welcome);
	printf("aht20 init done\n");
	printf("System started!\n");
	
	while (1)
	{
		
		if (count % 2 == 0)
		{
			printf("\n--- Measurement #%lu ---\n", count / 2);
			aht20_measure(aht20, &temp, &humi);
			led_on(led2);
		}
		else
		{
			led_off(led2);
		}
		
		
		cpu_delay_ms(500);
		count++;
	}
}

