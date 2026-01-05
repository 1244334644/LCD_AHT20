#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "aht20.h"
#include "aht20_desc.h"
#include "delay.h"
#include "usart.h"
#include "math.h"
//I2C1  CLK->PB6
//I2C1  SDA->PB7

static void aht20_io_init(aht20_desc_t aht20)
{
    GPIO_InitTypeDef GPIO_Init_Structure;
    GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init_Structure.GPIO_Pin = aht20->SCLPin|aht20->SDAPin;
    GPIO_Init_Structure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init_Structure.GPIO_OType = GPIO_OType_OD;
    GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(aht20->GPort,&GPIO_Init_Structure);

    GPIO_PinAFConfig(aht20->GPort, aht20->SCLPinSource, GPIO_AF_I2C1);
    GPIO_PinAFConfig(aht20->GPort, aht20->SDAPinSource, GPIO_AF_I2C1);

}

static void aht20_I2C_init(aht20_desc_t aht20)
{
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
  
    I2C_Cmd(aht20->I2C, ENABLE);
    I2C_Init(aht20->I2C, &I2C_InitStructure);

}

void aht20_init(aht20_desc_t aht20)
{
    aht20_io_init(aht20);
    aht20_I2C_init(aht20);
    cpu_delay_ms(40);
}

static bool I2C_Start(aht20_desc_t aht20)
{
    // 检查总线状态，如果繁忙则等待释放
    uint32_t timeout = 1000;
    while(I2C_GetFlagStatus(aht20->I2C, I2C_FLAG_BUSY))
    {
        if(timeout-- == 0)
        {
            printf("I2C bus busy timeout, forcing release\n");
            
            // 强制释放总线
            I2C_GenerateSTOP(aht20->I2C, ENABLE);
            cpu_delay_us(100);
            
            // 如果仍然繁忙，重置I2C外设
            if(I2C_GetFlagStatus(aht20->I2C, I2C_FLAG_BUSY))
            {
                I2C_Cmd(aht20->I2C, DISABLE);
                cpu_delay_us(10);
                I2C_Cmd(aht20->I2C, ENABLE);
                cpu_delay_us(10);
            }
            break;
        }
        cpu_delay_us(1);
    }
    
    // 清除所有错误标志
    //I2C_ClearFlag(aht20->I2C, I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR);
    
    // 生成起始信号
    I2C_GenerateSTART(aht20->I2C, ENABLE);
    
    // 等待主模式选择
    timeout = 1000;
    while(!I2C_CheckEvent(aht20->I2C, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if(timeout-- == 0)
        {
            printf("I2C start timeout\n");
            I2C_GenerateSTOP(aht20->I2C, ENABLE);
            return false;
        }
    }
    
    return true;
}

static void I2C_Stop(aht20_desc_t aht20)
{
    I2C_GenerateSTOP(aht20->I2C, ENABLE);
    while(I2C_GetFlagStatus(aht20->I2C, I2C_FLAG_STOPF));
    cpu_delay_us(100);  // 增加100微秒延时
}

// AHT20的7位地址是 0x38
// 转换为8位写地址：0x38 << 1 = 0x70
// 转换为8位读地址：0x38 << 1 | 0x01 = 0x71
static void aht20_write(aht20_desc_t aht20, uint8_t* data, uint8_t len)
{
    I2C_AcknowledgeConfig(aht20->I2C, ENABLE);
	I2C_Start(aht20);
	
	I2C_Send7bitAddress(aht20->I2C, 0x70, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(aht20->I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    for (uint8_t i = 0; i < len; i++)
    {
        I2C_SendData(aht20->I2C, data[i]);
        while(!I2C_CheckEvent(aht20->I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    I2C_Stop(aht20);
}

static bool aht20_read(aht20_desc_t aht20, uint8_t* data, uint8_t len)
{
    I2C_AcknowledgeConfig(aht20->I2C, ENABLE);//启动应答机制
	I2C_Start(aht20);
	
	I2C_Send7bitAddress(aht20->I2C, 0x71, I2C_Direction_Receiver);//发送 7 位从设备地址 0x38（转换为 8 位读地址为 0x71），并设置为主设备接收模式
	while(!I2C_CheckEvent(aht20->I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));//等待接收模式选择完成

    for (uint8_t i = 0; i < len; i++)
    {
        if(i == len-1)
		{
			I2C_AcknowledgeConfig(aht20->I2C, DISABLE); //最后一个字节，关闭应答，通知从设备数据传输结束
		}
        while(!I2C_CheckEvent(aht20->I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
        data[i] = I2C_ReceiveData(aht20->I2C);//执行完这句发送ACK和NACK
        
    }
    I2C_Stop(aht20);
    return true;
}

bool aht20_measure(aht20_desc_t aht20, float *temp, float *humi)
{
    uint8_t buf[8] = {0};
    uint8_t csh[3] = {0xBE, 0x08, 0x00};
    uint8_t status_cmd[1] = {0x71};
    float row_temp, row_humi;
    aht20_write(aht20, status_cmd, 1);
    aht20_read(aht20, buf, 1);
    if(((buf[0] >> 3) & 0x01) != 1)
    {
        aht20_write(aht20, csh, 3);
        cpu_delay_ms(10);
    }

    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    aht20_write(aht20, cmd, 3);
    cpu_delay_ms(80);
    aht20_read(aht20, buf, 6);
    
    // 等待传感器测量完成（bit7为0表示就绪）
    while(((buf[0] >> 7) & 0x01) == 1)
    {
        cpu_delay_ms(10);
        aht20_read(aht20, buf, 6);
    }
    
    // 传感器就绪，计算温湿度
    row_humi = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)buf[3] >> 4);
    row_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];
    *temp = row_temp * 200.0f / (float)0x100000 - 50.0f;
    *humi = row_humi * 100.0f / (float)0x100000;
    printf("Temp:%.1f, Humi:%.1f\n", *temp, *humi);
    return true;
}




