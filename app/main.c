#include "board.h"
#include <stdio.h>
#include "lvgl.h"
#include "lv_port_disp.h"
// #include "lv_port_indev.h"
// #include "lv_demo_widgets.h"
#include "events_init.h"
#include "gui_guider.h"

lv_ui guider_ui;

static void update_sensor_data(lv_timer_t * timer)
{
    float temp, humi;
    
    // 读取传感器
    aht20_measure(aht20, &temp, &humi);
    printf("temp: %.1f, humi: %.1f\n", temp, humi);
    // 更新显示
    lv_label_set_text_fmt(guider_ui.screen_label_1, "%.1f", temp);
    lv_label_set_text_fmt(guider_ui.screen_label_3, "%.1f%%", humi);

}

int main(void)
{
	// uint32_t count = 0;
	// float temp, humi;
	board_lowlevel_init();
	cpu_tick_init();
	aht20_init(aht20);
	
	//lv_scr_load(guider_ui.screen);  // 直接加载屏幕
	// 初始化lvgl
   	lv_init();

    // 初始化lvgl显示设备
    lv_port_disp_init();
	
    // 初始化lvgl输入设备
    //lv_port_indev_init();

    // 初始化lvgl demo
    //lv_demo_widgets();
	
	TIM3_Int_Init(999, 83);
	// led_init(led1);
	// led_init(led2);
	setup_ui(&guider_ui);
	usart_init(usart1);
	
	// lcd_init(lcd);
	// lcd_clear(lcd);
	

	//lcd_show_string(lcd, 29, 0, "Hello World!温度湿度温度湿度温度温", mkcolor(255, 255, 255), mkcolor(0, 0, 0), &font16);
	// lcd_show_img(lcd, 0, 0, &img_welcome);
	// lcd_fillcolor(lcd, 0,0,239,319,mkcolor(255, 0, 0));
	// lcd_fillcolor(lcd, 0,0,239,319,mkcolor(0, 255, 0));
	// lcd_fillcolor(lcd, 0,0,239,319,mkcolor(0, 0, 255));
	// lcd_fillcolor(lcd, 0,0,79,319,mkcolor(255, 0, 0));
	// lcd_fillcolor(lcd, 79,0,159,319,mkcolor(0, 255, 0));
	// lcd_fillcolor(lcd, 159,0,239,319,mkcolor(0, 0, 255));
	// printf("aht20 init done\n");
	// printf("System started!\n");
	lv_timer_create(update_sensor_data, 1000, NULL);
	while (1)
	{
		lv_task_handler();
		cpu_delay_ms(5);
		
		// if (count % 2 == 0)
		// {
		// 	printf("\n--- Measurement #%lu ---\n", count / 2);
		// 	aht20_measure(aht20, &temp, &humi);
		// 	led_on(led2);
		// }
		// else
		// {
		// 	led_off(led2);
		// }
		
		
		// cpu_delay_ms(500);
		// count++;
	}
}

void __aeabi_assert(const char *expr, const char *file, int line)
{
    printf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    while (1)
    {
        // 死循环，停止程序运行
    }
}

void TIM3_IRQHandler(void)
{
       /* 检测时间更新中断的标志位是否置位 */
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        lv_tick_inc(1); // 告诉LVGL：时间过了1ms
        /* 清空标志位 */
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
