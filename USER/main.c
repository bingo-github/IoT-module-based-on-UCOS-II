/******************** ��ʢ���ӹ����� ********************
 * �ļ���  ��main.c
 * ����    ������2������ÿ���������һ��LED���Թ̶���Ƶ��������˸��Ƶ�ʿɵ�����         
 * ʵ��ƽ̨��MINI STM32������ ����STM32F103C8T6
 * ��汾  ��ST3.0.0
 * �Ա��꣺http://shop66177872.taobao.com
**********************************************************************************/	

#include "includes.h"

OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];		  //����ջ
  
int main(void)
{
  	BSP_Init();			/* ��Ƭ�������ʼ��������USRT�����Ź� */
	OSInit();					/* ����ϵͳ��ʼ�������Բ��ù� */
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


