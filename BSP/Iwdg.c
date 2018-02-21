/*****************************************************************************************
**	  ��ţ�����壨V1.0��
**	  �������Ź������ļ�
**
**	  ��    ̳��bbs.openmcu.com
**	  ��    ����www.openmcu.com
**	  ��    �䣺support@openmcu.com
**
**    ��    ����V1.0
**	  ��    �ߣ�openmcu
**	  �������:	2014.06.05
*******************************************************************************************/
#include "stm32f10x.h"
#include <stdio.h>
#include "Iwdg.h"
#include "led.h"
#include "stm32f10x_iwdg.h"
/********************************************************************************************
*�������ƣ�void IWDG_Init(u8 prer,u16 rlr)
*
*��ڲ�������
*
*���ڲ�������
*
*����˵�������ڿ��Ź���ʼ��
*******************************************************************************************/ 
void IWDG_Init(void)
{	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����
	
	IWDG_SetPrescaler(IWDG_Prescaler_128);         //LSI��128��Ƶ	
	IWDG_SetReload(3599);                           //����IWDG����ֵΪ399
	IWDG_ReloadCounter();                          //����IWDG������	
	IWDG_Enable();                                 //����IWDG
}

/********************************************************************************************
*�������ƣ�void IWDG_Feed(void)
*
*��ڲ�������
*
*���ڲ�������
*
*����˵����ι��
*******************************************************************************************/
void IWDG_Feed(void)
{   
 	IWDG_ReloadCounter();                              //ι��									   
}