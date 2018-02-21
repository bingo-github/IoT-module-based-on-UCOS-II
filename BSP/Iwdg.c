/*****************************************************************************************
**	  火牛开发板（V1.0）
**	  独立看门狗配置文件
**
**	  论    坛：bbs.openmcu.com
**	  旺    宝：www.openmcu.com
**	  邮    箱：support@openmcu.com
**
**    版    本：V1.0
**	  作    者：openmcu
**	  完成日期:	2014.06.05
*******************************************************************************************/
#include "stm32f10x.h"
#include <stdio.h>
#include "Iwdg.h"
#include "led.h"
#include "stm32f10x_iwdg.h"
/********************************************************************************************
*函数名称：void IWDG_Init(u8 prer,u16 rlr)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：窗口看门狗初始化
*******************************************************************************************/ 
void IWDG_Init(void)
{	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //使能对寄存器IWDG_PR和IWDG_RLR的写操作
	
	IWDG_SetPrescaler(IWDG_Prescaler_128);         //LSI的128分频	
	IWDG_SetReload(3599);                           //设置IWDG计数值为399
	IWDG_ReloadCounter();                          //重载IWDG计数器	
	IWDG_Enable();                                 //启动IWDG
}

/********************************************************************************************
*函数名称：void IWDG_Feed(void)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：喂狗
*******************************************************************************************/
void IWDG_Feed(void)
{   
 	IWDG_ReloadCounter();                              //喂狗									   
}
