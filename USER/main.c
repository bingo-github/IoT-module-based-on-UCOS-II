/******************** 鑫盛电子工作室 ********************
 * 文件名  ：main.c
 * 描述    ：建立2个任务，每个任务控制一个LED，以固定的频率轮流闪烁（频率可调）。         
 * 实验平台：MINI STM32开发板 基于STM32F103C8T6
 * 库版本  ：ST3.0.0
 * 淘宝店：http://shop66177872.taobao.com
**********************************************************************************/	

#include "includes.h"

OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];		  //定义栈
  
int main(void)
{
  	BSP_Init();			/* 单片机外设初始化，包括USRT，看门狗 */
	OSInit();					/* 操作系统初始化，可以不用管 */
	OSTaskCreateExt(Task_Start,
									(void *)0,
									&startup_task_stk[STARTUP_TASK_STK_SIZE-1],
									STARTUP_TASK_PRIO,
									STARTUP_TASK_PRIO,
									&startup_task_stk[0],
									STARTUP_TASK_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);

	OSStart();
    return 0;
 }


