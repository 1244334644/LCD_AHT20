#include <stdlib.h>
#include <stdbool.h>
#include "led.h"
#include "led_desc.h"
void led_init(led_desc_t led)
{
	if (led == NULL)
	{
		return;
	}
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = led->Pin;
	GPIO_Init(led->Port, &GPIO_InitStructure);

	GPIO_WriteBit(led->Port, led->Pin, led->OffBit);
}

/**
 * @brief 设置指定LED的开关状态
 * @param idx LED索引，当前支持1和2两个LED
 * @param onoff LED状态，true为开启，false为关闭
 * @return 无
 */
void led_set(led_desc_t led, bool onoff)
{
    // 根据LED索引选择对应的LED进行控制
	if (led == NULL)
	{
		return;
	}
	
	GPIO_WriteBit(led->Port, led->Pin, onoff ? led->OnBit : led->OffBit);
}

void led_on(led_desc_t led)
{
	if (led == NULL)
	{
		return;
	}
	GPIO_WriteBit(led->Port, led->Pin, led->OnBit);
}

void led_off(led_desc_t led)
{
	if (led == NULL)
	{
		return;
	}
	GPIO_WriteBit(led->Port, led->Pin, led->OffBit);
}

void led_toggle(led_desc_t led)
{
	if (led == NULL)
	{
		return;
	}
	// 读取当前状态并翻转
	if (GPIO_ReadOutputDataBit(led->Port, led->Pin) == led->OnBit)
	{
		GPIO_WriteBit(led->Port, led->Pin, led->OffBit);
	}
	else
	{
		GPIO_WriteBit(led->Port, led->Pin, led->OnBit);
	}
}

// void led_all_off()
// {
// 	led_set(&led1, false);
// 	led_set(&led2, false);
// }

