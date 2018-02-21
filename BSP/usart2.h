#ifndef __USART2_H
#define	__USART2_H

#include "stm32f10x.h"
#include <stdio.h>

void USART2_Config(void);
void USART2_NVIC_Configuration(void);
void USART2_SendString(char *ch);
#endif /* __USART3_H */
