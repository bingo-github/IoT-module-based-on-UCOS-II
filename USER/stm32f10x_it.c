/**
  ******************************************************************************
  * @file    Project/Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.0.0
  * @date    04/06/2009
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
//#include "stm32f10x_it.h"
//#include  "ucos_ii.h" 
#include "includes.h"
#include "globals.h"
#include  "app_cfg.h"
#include  "os_cfg.h"
#include "stm32f10x_it.h"
/** @addtogroup Template_Project
  * @{
  */


//extern OS_EVENT* Com1_MBOX;
volatile unsigned int RxCounter1 = 0; 
unsigned char glb_msg1[MSG1_LEN] = {0};
volatile unsigned int RxCounter2 = 0; 
unsigned char glb_msg2[MSG2_LEN];
volatile unsigned int RxCounter3 = 0; 
unsigned char glb_msg3[MSG3_LEN];

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval : None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval : None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval : None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval : None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval : None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval : None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval : None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval : None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval : None
  */
void SysTick_Handler(void)
{
    OSIntEnter(); 
    OSTimeTick(); 
    OSIntExit(); 
}

/*********************************************
*函数名称：void USART1_IRQHandler(void)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：USART1 中断服务函数
**********************************************/
void USART1_IRQHandler(void)
{
//	uint8_t temp1;
//	unsigned char msg[50];
//	OS_CPU_SR  cpu_sr;
//  	
//	OS_ENTER_CRITICAL();  //保存全局中断标志,关总中断// Tell uC/OS-II that we are starting an ISR
//  	OSIntNesting++;
//	
//  	OS_EXIT_CRITICAL();	  //恢复全局中断标志
	 if(USART_GetITStatus(USART1, USART_IT_RXNE)==SET)
	{	 
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//		msg[RxCounter1++]=USART_ReceiveData(USART1);
		if((MSG1_LEN-3) >= RxCounter1)
		{
			glb_msg1[RxCounter1] = USART_ReceiveData(USART1);
			RxCounter1++;		
		}
//		OSMboxPost(Com1_MBOX,(void *)&msg); 
	}
}

/*********************************************
*函数名称：void USART2_IRQHandler(void)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：USART2 中断服务函数
**********************************************/
void USART2_IRQHandler(void)
{
//	uint8_t temp1;
//	unsigned char msg[50];
//	OS_CPU_SR  cpu_sr;
//  	
//	OS_ENTER_CRITICAL();  //保存全局中断标志,关总中断// Tell uC/OS-II that we are starting an ISR
//  	OSIntNesting++;
//	
//  	OS_EXIT_CRITICAL();	  //恢复全局中断标志
	 if(USART_GetITStatus(USART2, USART_IT_RXNE)==SET)
	{	 
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
//		msg[RxCounter1++]=USART_ReceiveData(USART1);
		if((MSG2_LEN-3) >= RxCounter2)
		{
			glb_msg2[RxCounter2] = USART_ReceiveData(USART2);
			RxCounter2++;		
		}
//		OSMboxPost(Com1_MBOX,(void *)&msg); 
	}
}

/*********************************************
*函数名称：void USART3_IRQHandler(void)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：USART3 中断服务函数
**********************************************/
void USART3_IRQHandler(void)
{
//	uint8_t temp1;
//	unsigned char msg[50];
//	OS_CPU_SR  cpu_sr;
//  	
//	OS_ENTER_CRITICAL();  //保存全局中断标志,关总中断// Tell uC/OS-II that we are starting an ISR
//  	OSIntNesting++;
//	
//  	OS_EXIT_CRITICAL();	  //恢复全局中断标志
	 if(USART_GetITStatus(USART3, USART_IT_RXNE)==SET)
	{	 
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
//		msg[RxCounter1++]=USART_ReceiveData(USART1);
		if((MSG3_LEN-3) >= RxCounter3)
		{
			glb_msg3[RxCounter3] = USART_ReceiveData(USART3);
			RxCounter3++;		
		}
//		OSMboxPost(Com1_MBOX,(void *)&msg); 
	}
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval : None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
