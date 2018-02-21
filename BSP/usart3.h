#ifndef __USART3_H
#define	__USART3_H

#include "stm32f10x.h"
#include <stdio.h>

void USART3_Config(void);
void UART3Test(void);
 void UART3SendByte(unsigned char SendData);
void USART3_NVIC_Configuration(void);
void UART3_SendBuffer(uint8_t* SendData, int bufferLen);
void USART3_SendString(char *ch);
#endif /* __USART3_H */
