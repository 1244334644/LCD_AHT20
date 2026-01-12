#include "board.h"
#include <stdio.h>
#include "lvgl.h"
#include "lv_port_disp.h"
// #include "lv_port_indev.h"
// #include "lv_demo_widgets.h"
#include "events_init.h"
#include "gui_guider.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" 


lv_ui guider_ui;
SemaphoreHandle_t init_done_sem;  // 初始化完成信号量

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
void init(void *param)
{	
	aht20_init(aht20);
	lv_init();
	lv_port_disp_init();
	TIM3_Int_Init(999, 83);
	setup_ui(&guider_ui);

	// 释放信号量，通知 lvgl_task 可以开始了
	xSemaphoreGive(init_done_sem);

	vTaskDelete(NULL);
}

void lvgl_task(void *pvParameters)
{	
	// 等待初始化完成
    xSemaphoreTake(init_done_sem, portMAX_DELAY);
	lv_timer_create(update_sensor_data, 1000, NULL);
    while(1)
    {
        lv_task_handler();                    // 处理LVGL任务
        vTaskDelay(pdMS_TO_TICKS(5));         // 任务延时5ms（不占用CPU）
    }
}

int main(void)
{

	board_lowlevel_init();

	
	 // 创建信号量
    init_done_sem = xSemaphoreCreateBinary();

	usart_init(usart1);

	// 创建lvgl任务
	xTaskCreate(init, "init", 1024, NULL, 9, NULL);
    xTaskCreate(lvgl_task, "lvgl_task", 1024, NULL, 5, NULL);
	vTaskStartScheduler();
	while (1)
	{
		
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

void vAssertCalled(const char *file, int line)
{
	portDISABLE_INTERRUPTS();
    printf("Assert Called: %s(%d)\n", file, line);
	

}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack Overflowed: %s\n", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook( void )
{
    printf("Malloc Failed\n");
    configASSERT(0);
}

