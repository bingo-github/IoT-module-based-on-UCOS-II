#include "includes.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "globals.h"
//#include "RS485.h"
#include "Iwdg.h"		//独立看门狗函数

/*
 * 函数名：BSP_Init
 * 描述  ：时钟初始化、硬件初始化
 * 输入  ：无
 * 输出  ：无
 */
void BSP_Init(void)
{
    SystemInit();		/* 配置系统时钟为72M */	
    LED_GPIO_Config();  /* LED 端口初始化 */			/* 初始化PC13管脚 */
	
	USART1_Config();
	USART1_NVIC_Configuration();
	USART2_Config();
	USART2_NVIC_Configuration();
	USART3_Config();
	USART3_NVIC_Configuration();
	
	IWDG_Init();
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:System Start!\r\n");
		#endif
}

/*
 * 函数名：SysTick_init
 * 描述  ：配置SysTick定时器
 * 输入  ：无
 * 输出  ：无
 */
void SysTick_init(void)
{
    SysTick_Config(SystemFrequency/OS_TICKS_PER_SEC);//初始化并使能SysTick定时器
}
