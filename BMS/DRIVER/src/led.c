#include "stm32f10x.h"
#include "stdlib.h"
#include "delay.h"
#include "led.h"
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;   
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); 

	/*PA15 引脚在系统复位后默认功能不是普通的GPIO，而是作为 JTAG 调试接口的 JTDI 引脚。所以要禁用JTAG功能，释放PA15*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); //开启AFIO时钟
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//使能JTAGDisable，即禁用JTAG接口
	
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_15; 
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;	//推挽输出
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;  
	GPIO_Init(GPIOA,&GPIO_InitStruct);
		
}

void LED_ON(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_15);
}

void LED_OFF(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_15);
}

void LED_TOGGLE(void)
{
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15))
	{
		 GPIO_ResetBits(GPIOA, GPIO_Pin_15);
	}
	else
	{
			GPIO_SetBits(GPIOA,GPIO_Pin_15);
	}
}
