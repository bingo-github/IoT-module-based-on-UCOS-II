#ifndef __USART1_H
#define	__USART1_H

#include "stm32f10x.h"
#include <stdio.h>

void USART1_Config(void);
void USART1_NVIC_Configuration(void);
void USART1_SendString(char *ch);
#endif /* __USART1_H */
