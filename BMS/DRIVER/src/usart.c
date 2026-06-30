#include <stdio.h>

#include "stm32f10x.h"
#include "usart.h"

void USART_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    init.USART_BaudRate = baudrate;
    init.USART_WordLength = USART_WordLength_8b;
    init.USART_StopBits = USART_StopBits_1;
    init.USART_Parity = USART_Parity_No;
    init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &init);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);

    USART_Cmd(USART1, ENABLE);
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
    }
    return ch;
}

void usart_send(uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
        }
        USART_SendData(USART1, data[i]);
    }

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
    }
}
