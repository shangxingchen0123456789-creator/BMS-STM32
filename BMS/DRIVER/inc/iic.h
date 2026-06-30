#ifndef   _IIC_H
#define   _IIC_H
#include "stdint.h"
#include "stm32f10x.h"   

#define IIC_SCL_PORT GPIOB
#define IIC_SCL_PIN GPIO_Pin_8
#define IIC_SCL_CLK RCC_APB2Periph_GPIOB

#define IIC_SDA_PORT GPIOB
#define IIC_SDA_PIN GPIO_Pin_9
#define IIC_SDA_CLK RCC_APB2Periph_GPIOB

//GPIO操作宏定义
#define IIC_SCL_HIGH() GPIO_SetBits(IIC_SCL_PORT,IIC_SCL_PIN)
#define IIC_SCL_LOW() GPIO_ResetBits(IIC_SCL_PORT,IIC_SCL_PIN)
#define IIC_SDA_HIGH() GPIO_SetBits(IIC_SDA_PORT,IIC_SDA_PIN)
#define IIC_SDA_LOW() GPIO_ResetBits(IIC_SDA_PORT,IIC_SDA_PIN)

#define IIC_SDA_READ() GPIO_ReadInputDataBit(IIC_SDA_PORT,IIC_SDA_PIN)



//函数声明
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_Write_Byte(uint8_t data);
uint8_t IIC_Read_Byte(uint8_t ack);
uint8_t IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Write_Byte(uint8_t data);
uint8_t IIC_Write_Data(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data,uint16_t len);
uint8_t IIC_Read_Data(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

#endif

