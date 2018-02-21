#ifndef	_APP_H_
#define	_APP_H_

/**************** 用户任务声明 *******************/
void Task_Start(void *p_arg);
void Task_ALARM(void *p_arg);
void Task_ESP8266(void *p_arg);
void Task_TEMP(void *p_arg);

#endif	//_APP_H_
