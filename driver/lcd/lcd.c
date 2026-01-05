#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "lcd_desc.h"
#include "delay.h"
#include "font.h"
#include "usart.h"
#include "img.h"
// SCK-> PB13 SPI2_SCK 时钟线（硬件SPI固定）
// SDA-> PB15 SPI2_MOSI 数据线（硬件SPI固定）
// CS-> PB12 SPI2_NSS 片选线（可软件控制）
// DC-> PB10 GPIO输出 数据/命令控制
// RST-> PB9 GPIO输出 复位控制
// BL-> PB0 GPIO输出 背光控制（PWM可选）

#define ST7789_COLUMN 240
#define ST7789_LINE 320
// volatile bool dma_transfer_complete = false;

// 添加DMA相关变量
static volatile bool dma_transfer_complete = false;
static uint16_t dma_buffer[240]; // 最大支持一行240像素的DMA传输

static void io_init(lcd_desc_t lcd)
{

    GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = lcd->SCKPin | lcd->MOSIPin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	
	GPIO_PinAFConfig(lcd->Port, lcd->SCKPinsource, GPIO_AF_SPI2);
	GPIO_PinAFConfig(lcd->Port, lcd->MOSIPinsource, GPIO_AF_SPI2);

	GPIO_Init(lcd->Port, &GPIO_InitStruct);

	// 配置CS、DC、RST、BL为普通GPIO输出模式
	GPIO_InitStruct.GPIO_Pin = lcd->CSPin | lcd->DCPin | lcd->RSTPin | lcd->BLPin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(lcd->Port, &GPIO_InitStruct);

}

static void spi_init(lcd_desc_t lcd)
{

	SPI_InitTypeDef SPI_InitStructure;
	SPI_StructInit(&SPI_InitStructure);
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	// SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(lcd->SPI, &SPI_InitStructure);

	SPI_Cmd(lcd->SPI, ENABLE);
}

static void dma_init(lcd_desc_t lcd)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	 // 配置DMA参数
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;          // 通道0
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR); // 外设地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; // 外设到内存
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // 外设地址不递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 内存地址递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 外设数据宽度，16位（2字节）
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;         // 内存数据宽度
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;          // 普通模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;   // 高优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable; // 使能FIFO
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC8; // 存储器8字节突发传输
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // 外设单次传输
    DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE); // 传输完成中断使能
    DMA_Init(DMA1_Stream4, &DMA_InitStructure);
}

static void nvic_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn; // DMA1 Stream4中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5; // 抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // 子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
	NVIC_Init(&NVIC_InitStructure);
	NVIC_SetPriority(DMA1_Stream4_IRQn, 5);
}


static void st7789_write(lcd_desc_t lcd, uint8_t reg, uint8_t* data, uint16_t len)
{
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_DataSizeConfig(lcd->SPI, SPI_DataSize_8b);
	SPI_Cmd(lcd->SPI, ENABLE);

	GPIO_ResetBits(lcd->Port, lcd->CSPin); // CS = 0，片选使能
	GPIO_ResetBits(lcd->Port, lcd->DCPin); // DC = 0，命令模式

	SPI_SendData(lcd->SPI, reg);
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET); // 等待发送缓冲区空
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待传输完成

	if(len > 0) {
		GPIO_SetBits(lcd->Port, lcd->DCPin);   // DC = 1，数据模式
		for (uint16_t i = 0; i < len; i++)
		{
			SPI_SendData(lcd->SPI, data[i]);
			while (SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET); // 等待发送缓冲区空
		}
		while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待传输完成
	}

	GPIO_SetBits(lcd->Port, lcd->CSPin);   // CS = 1，片选禁能
}

static void st7789_init(lcd_desc_t lcd)
{
	// 复位LCD
	GPIO_ResetBits(lcd->Port, lcd->RSTPin); // RST = 0，复位
	cpu_delay_ms(100);
	GPIO_SetBits(lcd->Port, lcd->RSTPin);   // RST = 1，释放复位
	cpu_delay_ms(100);

	st7789_write(lcd, 0x11, NULL, 0); // 退出睡眠模式
	cpu_delay_ms(120);
	
	st7789_write(lcd, 0x36, (uint8_t[]){0x00}, 1); // 内存访问控制
	st7789_write(lcd, 0x3a, (uint8_t[]){0x55}, 1); // 16位颜色模式
	
	st7789_write(lcd, 0xb2, (uint8_t[]){0x0c, 0x0c, 0x00, 0x33, 0x33}, 5); // 帧率控制
	st7789_write(lcd, 0xb7, (uint8_t[]){0x35}, 1); // Gate control

	st7789_write(lcd, 0xbb, (uint8_t[]){0x19}, 1); // VCOM setting
	st7789_write(lcd, 0xc0, (uint8_t[]){0x2c}, 1); // LCM control
	st7789_write(lcd, 0xc2, (uint8_t[]){0x01}, 1); // VDV and VRH command enable
	st7789_write(lcd, 0xc3, (uint8_t[]){0x12}, 1); // VRH set
	st7789_write(lcd, 0xc4, (uint8_t[]){0x20}, 1); // VDV set
	st7789_write(lcd, 0xc6, (uint8_t[]){0x0f}, 1); // Frame rate control in normal mode
	
	st7789_write(lcd, 0xd0, (uint8_t[]){0xa4, 0xa1}, 2); // Power control

	st7789_write(lcd, 0xe0, (uint8_t[]){0xD0,0x0D,0x14, 0x0B,0x0B,0x07, 0x3A,0x44,0x50, 0x08,0x13,0x13, 0x2D,0x32}, 14);
	st7789_write(lcd, 0xe1, (uint8_t[]){0xD0,0x0D,0x14, 0x0B,0x0B,0x07, 0x3A,0x44,0x50, 0x08,0x13,0x13, 0x2D,0x32}, 14);

	st7789_write(lcd, 0x21, NULL, 0); // 反显关闭（正常显示）
	cpu_delay_ms(10);
	
	st7789_write(lcd, 0x29, NULL, 0); // 显示开启
	cpu_delay_ms(20);
}

void lcd_init(lcd_desc_t lcd)
{
	io_init(lcd);
	spi_init(lcd);
	dma_init(lcd);
	nvic_init();
	st7789_init(lcd);
	GPIO_SetBits(lcd->Port, lcd->BLPin); // 开启背光
}

static void lcd_set_window(lcd_desc_t lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	uint8_t data[4];
	if(x1-x0 >= ST7789_COLUMN||y1-y0 >= ST7789_LINE||x1 < x0||y1 < y0)
	{
		//printf("lcd_set_window error: out of range\r\n");
		return;
	}
	// 设置列地址
	data[0] = (x0 >> 8) & 0xFF;
	data[1] = x0 & 0xFF;
	data[2] = (x1 >> 8) & 0xFF;
	data[3] = x1 & 0xFF;
	st7789_write(lcd, 0x2a, data, 4);
	
	// 设置行地址
	data[0] = (y0 >> 8) & 0xFF;
	data[1] = y0 & 0xFF;
	data[2] = (y1 >> 8) & 0xFF;
	data[3] = y1 & 0xFF;
	st7789_write(lcd, 0x2b, data, 4);
	
	// 写入内存命令
	st7789_write(lcd, 0x2c, NULL, 0);
}

// 修复颜色显示函数（16位颜色模式）
void lcd_fillcolor(lcd_desc_t lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	unsigned int row, column;

	lcd_set_window(lcd, x0, y0, x1, y1);
	
	// 切换到数据模式后连续发送颜色数据
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_DataSizeConfig(lcd->SPI, SPI_DataSize_16b);//直接传递16位的像素数据
	SPI_Cmd(lcd->SPI, ENABLE);
	
	GPIO_ResetBits(lcd->Port, lcd->CSPin); // CS = 0，片选使能
	GPIO_SetBits(lcd->Port, lcd->DCPin);   // DC = 1，数据模式
	
	// 填充整个屏幕
	for(row = 0; row < (y1 - y0 + 1); row++)
	{
		for(column = 0; column < (x1 - x0 + 1); column++)
		{
			SPI_SendData(lcd->SPI, color);
			while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
		}
	}
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待最后的传输完成
	
	GPIO_SetBits(lcd->Port, lcd->CSPin);   // CS = 1，片选禁能
}

void lcd_clear(lcd_desc_t lcd)  // 清屏函数
{
	lcd_fillcolor(lcd, 0, 0, ST7789_COLUMN - 1, ST7789_LINE - 1, mkcolor(0, 0, 0));
}


static void lcd_show_char(lcd_desc_t lcd, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgcolor, const font_t* font)
{
	uint8_t index = c - ' ';
	if(index >= 95) return; // 超出范围直接返回
	
	uint16_t fheight = font->size, fwidth = font->size / 2;
	uint8_t row_byte = (fwidth + 7) / 8;//每行传输的字节数，向上取整当 fwidth 是8的倍数时：(8n + 7) / 8 = n；当 fwidth 不是8的倍数时：(8n+k + 7) / 8 = n+1（其中k=1-7）
	const uint8_t *model = font->ascii_model + index * fheight * row_byte;//找到要显示字符的起始位
	
	// 设置显示窗口
	lcd_set_window(lcd, x, y, x + fwidth - 1, y + fheight - 1);
	
	// 切换到数据模式
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_DataSizeConfig(lcd->SPI, SPI_DataSize_16b);//直接传递16位的像素数据
	SPI_Cmd(lcd->SPI, ENABLE);
	
	GPIO_ResetBits(lcd->Port, lcd->CSPin); // CS = 0，片选使能
	GPIO_SetBits(lcd->Port, lcd->DCPin);   // DC = 1，数据模式
	
	
	// 发送字模数据
	for(uint8_t row = 0; row < fheight; row++)
	{
		const uint8_t *row_data = model + row * row_byte;//当前行的字模数据
		
		for(uint8_t column = 0; column < fwidth; column++)
		{
			// 正确的位运算：从字节数组中提取对应位
			uint8_t byte_index = column / 8;
			uint8_t bit_index = 7 - (column % 8); // MSB优先
			uint8_t pixel = (row_data[byte_index] >> bit_index) & 0x01;
			// 字节数据: 0 0 1 1 1 1 0 0
			// 位索引:   7 6 5 4 3 2 1 0
			// 像素值:   □ □ ■ ■ ■ ■ □ □  （□=背景色，■=前景色）

			if(pixel)
			{
				SPI_SendData(lcd->SPI, color);
				while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
			}
			else
			{
				SPI_SendData(lcd->SPI, bgcolor);
				while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
			}
		}
	}
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待最后的传输完成
	
	GPIO_SetBits(lcd->Port, lcd->CSPin);   // CS = 1，片选禁能
}


static bool is_gb2312(char ch)
{
    return ((unsigned char)ch >= 0xa1) && ((unsigned char)ch <= 0xf7);
}

static void lcd_show_chinese(lcd_desc_t lcd, uint16_t x, uint16_t y,const char *ch, uint16_t color, uint16_t bgcolor, const font_t* font)
{
	//找到字符在字体表中的索引
    if(font==NULL||ch==NULL)
        return;
   
    const font_chinese_t *c = font->chinese;
    for(;c->name!=NULL;c++)
    {
        if (c->name[0] == ch[0] && c->name[1] == ch[1])//由于传输的gb2312显示一个汉字需要有两个字节，所以应该两个字节都比较
            break;
    }
    if(c->name==NULL)
        return;

    uint16_t fheight = font->size, fwidth = font->size;
	uint8_t row_byte = (fwidth + 7) / 8;//每行传输的字节数，向上取整当 fwidth 是8的倍数时：(8n + 7) / 8 = n；当 fwidth 不是8的倍数时：(8n+k + 7) / 8 = n+1（其中k=1-7）
	const uint8_t *model = c->model;
	// 设置显示窗口
	lcd_set_window(lcd, x, y, x + fwidth - 1, y + fheight - 1);

	// 切换到数据模式
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_DataSizeConfig(lcd->SPI, SPI_DataSize_16b);//直接传递16位的像素数据
	SPI_Cmd(lcd->SPI, ENABLE);
	
	GPIO_ResetBits(lcd->Port, lcd->CSPin); // CS = 0，片选使能
	GPIO_SetBits(lcd->Port, lcd->DCPin);   // DC = 1，数据模式


	// 发送字模数据
	for(uint8_t row = 0; row < fheight; row++)
	{
		const uint8_t *row_data = model + row * row_byte;//当前行的字模数据
		
		for(uint8_t column = 0; column < fwidth; column++)
		{
			// 正确的位运算：从字节数组中提取对应位
			uint8_t byte_index = column / 8;
			uint8_t bit_index = 7 - (column % 8); // MSB优先
			uint8_t pixel = (row_data[byte_index] >> bit_index) & 0x01;
			// 字节数据: 0 0 1 1 1 1 0 0
			// 位索引:   7 6 5 4 3 2 1 0
			// 像素值:   □ □ ■ ■ ■ ■ □ □  （□=背景色，■=前景色）
			if(pixel)
			{
				SPI_SendData(lcd->SPI, color);
				while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
			}
			else
			{
				SPI_SendData(lcd->SPI, bgcolor);
				while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
			}
		}
	}
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待最后的传输完成

	GPIO_SetBits(lcd->Port, lcd->CSPin);   // CS = 1，片选禁能
}

void lcd_show_string(lcd_desc_t lcd,uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, const font_t* font)
{
    uint16_t fwidth = font->size / 2; // 字符宽度
    uint16_t fheight = font->size;    // 字符高度
    uint16_t current_x = x;           // 当前x坐标
    uint16_t current_y = y;           // 当前y坐标
    
    for(uint8_t i = 0; str[i] != '\0'; i++)
    {
		 // 预计算当前字符需要的宽度（汉字宽，英文窄）
        uint16_t char_width = is_gb2312(str[i]) ? font->size : fwidth;
		// 处理换行符或超出屏幕宽度
        if(str[i] == '\n' || char_width + current_x > ST7789_COLUMN)
        {
            current_y += fheight;     // 换行
            current_x = 0;            // 重置到0坐标
            
            // 如果换行后超出屏幕高度，停止显示
            if(current_y + fheight > ST7789_LINE)
                break;
            
            // 如果是换行符，跳过不显示
            if(str[i] == '\n')
                continue;
        }
		

        // 处理中文字符
        if(is_gb2312(str[i]))
        {
			if(str[i+1] == '\0') break; 
			// 【调试代码】打印当前要显示的汉字编码
            //printf("Trying to show: %02X %02X\r\n", (uint8_t)str[i], (uint8_t)str[i+1]);
            lcd_show_chinese(lcd, current_x, current_y, str+i, color, bgcolor, font);
            current_x += font->size; // 中文字符宽度为2个字符宽度
			i++; 
            continue;
        }
        else
		{
			// 显示字符
        	lcd_show_char(lcd, current_x, current_y, str[i], color, bgcolor, font);
			// 更新x坐标
        	current_x += fwidth;
		}
        
    }
}

void lcd_show_img(lcd_desc_t lcd,uint16_t x, uint16_t y, const img_t* img)
{
	uint32_t total_pixels = (uint32_t)img->width * img->height; // 使用uint32_t防止溢出
	const uint8_t* pixel_data = (const uint8_t*)img->data;
	// 设置显示窗口
	lcd_set_window(lcd, x, y, x + img->width - 1, y + img->height - 1);
	// 切换到数据模式
	SPI_Cmd(lcd->SPI, DISABLE);
	SPI_DataSizeConfig(lcd->SPI, SPI_DataSize_16b);//直接传递16位的像素数据
	SPI_Cmd(lcd->SPI, ENABLE);

	GPIO_ResetBits(lcd->Port, lcd->CSPin); // CS = 0，片选使能
	GPIO_SetBits(lcd->Port, lcd->DCPin);   // DC = 1，数据模式
	// 发送图像数据 - 将8位字节数据组合成16位像素

	for(uint32_t i = 0; i < total_pixels; i++)
	{
		uint16_t pixel = (pixel_data[i * 2] << 8) | pixel_data[i * 2 + 1];
		SPI_SendData(lcd->SPI, pixel);
		while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_TXE) == RESET);
	}
	while(SPI_GetFlagStatus(lcd->SPI, SPI_FLAG_BSY) == SET); // 等待最后的传输完成
	GPIO_SetBits(lcd->Port, lcd->CSPin);   // CS = 1，片选禁能

}


// DMA传输完成中断处理函数
void DMA1_Stream4_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4))
    {
        DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);
        dma_transfer_complete = true;
    }
}

	