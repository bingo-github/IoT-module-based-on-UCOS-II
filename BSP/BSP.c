#include "includes.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "globals.h"
//#include "RS485.h"
#include "Iwdg.h"		//�������Ź�����

/*
 * ��������BSP_Init
 * ����  ��ʱ�ӳ�ʼ����Ӳ����ʼ��
 * ����  ����
 * ���  ����
 */
void BSP_Init(void)
{
    SystemInit();		/* ����ϵͳʱ��Ϊ72M */	
    LED_GPIO_Config();  /* LED �˿ڳ�ʼ�� */			/* ��ʼ��PC13�ܽ� */
	
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
 * ��������SysTick_init
 * ����  ������SysTick��ʱ��
 * ����  ����
 * ���  ����
 */
void SysTick_init(void)
{
    SysTick_Config(SystemFrequency/OS_TICKS_PER_SEC);//��ʼ����ʹ��SysTick��ʱ��
}
