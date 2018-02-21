#ifndef  __APP_CFG_H__
#define  __APP_CFG_H__


/*******************设置任务优先级*******************/
#define STARTUP_TASK_PRIO       4	   
#define	TASK_ALARM_PRIO			5
#define	TASK_ESP8266_PRIO			6 
#define TASK_TEMP_PRIO			7

/************设置栈大小（单位为 OS_STK ）************/
#define STARTUP_TASK_STK_SIZE   80   
#define	TASK_ALARM_STK_SIZE		80
#define	TASK_ESP8266_STK_SIZE		80
#define TASK_TEMP_STK_SIZE		80

#endif

