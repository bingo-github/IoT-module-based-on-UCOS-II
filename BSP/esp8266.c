#include "includes.h"
#include "esp8266.h"
#include "stm32f10x.h"
#include <string.h>
#include "usart1.h"
#include <stdlib.h>
#include <stdio.h>
#include "flash.h"
#include "stm32f10x_it.h"
#include "globals.h"
#include "crc16.h"
#include "usart3.h"
#include "usart2.h"
#include "string.h"
#include "DS18B20.h"

#ifdef DEBUG
extern char tempBufferForDebug[250];
#endif
extern char g_chrpESP8266SetTime[20];
extern char g_chrpESP8266SN[25];
extern char g_chrpESP8266CO[8];
extern char g_chrSetValve;
extern char g_chrpESP8266SensorStatus[8];
extern char g_chrpSSID[40];
extern char g_chrpRouterPW[40];
extern char g_chrpESP8266InstruToServer[100];
extern char g_chr10InstruWriteOK_flag;


/* 20170916 Ê¹ÓÃÍ¬Ò»¸öCPUÖ®ºó£¬·¢ËÍ¸ø±¨¾¯Æ÷Ïß³ÌµÄÊý¾Ý */
extern char g_chrp8266ToAlarm[100];
extern char g_chrpESP8266TEMP[8];
extern char g_chrpESP8266VALVE[8];

/* 0x10功能码，最后两个字节为CRC校验码，CRC16H，CRC16L */
static uint8_t g_chr10FuncInstru[100] = {0x01, 0x10,
								(WRITE_REGISTER_START_ADDR >> 8), (WRITE_REGISTER_START_ADDR & 0xFF),
								(WRITE_REGISTER_NUM >> 8), (WRITE_REGISTER_NUM & 0xFF),							/* 写入的寄存器数量 */
								WRITE_REGISTER_NUM,																/* 写入的字节数量（一个寄存器一个字节） */
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,							/* 时间 */
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,						/* 链路故障点 */
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,						/* 当前温度 */
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,						/* 是否连接到服务器 */
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,						/* 模块唯一标识码 */
								0x00,																			/* 阀门关断 */
								0x00, 0x00};																	/* CRC校验码 */

unsigned char guchrEsp8266Model;								// ESP8266工作模式标志位

char gchrEsp8266IsConnectModule = ESP8266_CONNECT_NULL;			/* 表示MCU与ESP8266模块之间的通讯连接情况 */
char gchrEsp8266IsConnectRouter = ESP8266_CONNECT_NULL;			/* 表示模块与路由器之间的通讯连接情况 */
char gchrEsp8266IsConnectServer = ESP8266_CONNECT_NULL;			/* 表示模块与远端服务器之间的通讯连接情况 */

char *pOk = NULL;	/* 接收strstr函数的返回值，判断返回值中是否有“OK”，以此来判断AT指令发送是否成功 */
char *pBusy = NULL;
char *pError = NULL;		/* 接收strstr函数的返回值，判断返回值中是否有“ERROR”，以此来判断AT指令发送是否成功 */
char *pNo = NULL;	/* 接收strstr函数的返回值，判断返回值中是否有“No”，以此来判断AT指令发送是否成功 */
char *pSSID = NULL;
char *pPassword = NULL;
char *pConnect = NULL;
char *pClosed = NULL;
char *pALREADY = NULL;
char *pAPData[5] = {NULL};
char APDataStr[5][8] = {"SSID=\"", "\"PW=\"", "\"SN=\"", "\"DT=\"", "\"\r\n"};		/* 接收到的数据中的特征字符串 */
char *pServerData[3] = {NULL};
char ServerDataStr[3][15] = {"SET_TIME=[", "SET_SN=[", "CLOSE_VALVE=["};

/*
 *
 * 如将数字10（或0x0A）转化成"10";
 */
int ESP8266_NumToStr(char* p_chrpDesStr, int p_uchrSecNum)
{
	if (0 >= p_uchrSecNum)
	{
		return -1;
	}
	else
	{
		sprintf(p_chrpDesStr, "%ld", (long)p_uchrSecNum);
		return 0;
	}
}

static void ESP8266_Delay(int t)
{
	char i;
	while(t--)
	{
		for(i=0; i<100; i++)
		{
			
		}
	}
}

/*
 *
 */
int ESP8266_Send(char* p_uchrpData, unsigned char p_uchrLen,char p_chrChannel)
{
	char p_chrNumStr[10], p_chrICPSendBuffer[55];
	if(0 == p_chrChannel)
	{
		return -1;
	}

	ESP8266_NumToStr(p_chrNumStr, p_uchrLen);

	sprintf(p_chrICPSendBuffer,"AT+CIPSEND=%c,%s\r\n",p_chrChannel, p_chrNumStr);

	USART1_SendString(p_chrICPSendBuffer);

	ESP8266_Delay(900);				/* 经过测试，此处的延时也是必须的 */

	USART1_SendString(p_uchrpData);

	ESP8266_Delay(900);				/* 经过测试，此处的延时也是必须的 */
		
	return 0;
}

/*
 * 相当于USART1_SendString();
 */
void ESP8266_USARTSend(char *ch)
{
	USART1_SendString(ch);
}

/*
 * 将SSID和PW存入flash中
 */
void ESP8266_SSIDandPW_SaveToFlash(uint32_t DesAddress, char* SSID, char* RouterPW, char * SN)
{
	uint32_t srcData[122];
	unsigned char i = 0;
	/* 存储第一个标志字段 */
	srcData[0] = 0x11111111;
	/* 存储SSID长度 */
	srcData[1] = strlen(SSID);
	/* 存储SSID */
	for(i=0; i<strlen(SSID); i++)
	{
		srcData[i+2] = SSID[i];
	}
	/* 存储PW长度 */
	srcData[2+strlen(SSID)] = strlen(RouterPW);
	/* 存储PW */
	for(i=0; i<strlen(RouterPW); i++)
	{
		srcData[i+3+strlen(SSID)] = RouterPW[i];
	}
	/* 存储SN长度 */
	srcData[3+strlen(SSID)+strlen(RouterPW)] = strlen(SN);
	/* 存储SN */
	for(i=0; i<strlen(SN); i++)
	{
		srcData[i+4+strlen(SSID)+strlen(RouterPW)] = SN[i];
	}
	
	
	Flash_WriteDataToFlash(BANK1_WRITE_START_ADDR,srcData,4+strlen(SSID)+strlen(RouterPW)+strlen(SN));
}

/*
 * 将SSID和PW从FLASH中读取出来，
 * 其中第一个desData[0]表示是否有写入数据，若为0xFFFFFFFF或0x00000000表示从未写入数据，则直接退出
 */
int ESP8266_SSIDandPW_ReadFromFlash(uint32_t srcAddress, char* desSSID, char* desRouterPW, char* desSN)
{
	uint32_t desData[65];
	unsigned char i = 0;
	Flash_ReadDataFromFlash(srcAddress,desData,65);
	if((0xFFFFFFFF == desData[0]) || (0x00000000 == desData[0]))
	{
		return ESP8266_RE_FAIL;
	}
	
	for(i=0; i<desData[1]; i++)
	{
		desSSID[i] = (char)desData[2+i];
	}
	
	for(i=0; i<desData[2+desData[1]]; i++)
	{
		desRouterPW[i] = (char)desData[3+desData[1]+i];
	}
	
	for(i=0; i<desData[2+desData[1]+1+desData[2+desData[1]]]; i++)
	{
		desSN[i] = (char)desData[4+desData[1]+desData[2+desData[1]]+i];
	}
	return ESP8266_RE_OK;
}

/* ----------------------------新整理的函数 2017-06-17 --------------------------------- */
int ESP8266_CheckModuleLink(void)
{
	ESP8266_USARTSend("AT\r\n");									/* 向ESP8266发送"AT\r\n"，判断与模块是否连接正常 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);											/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)													/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);									/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");						/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);										/* 将glb_msg1清零 */
	RxCounter1 = 0;													/* 将glb_msg1的数据计数清零 */
	if(NULL == pOk)													/* 如果没有"OK" */
	{
		gchrEsp8266IsConnectModule = ESP8266_CONNECT_FAIL;			/* 标志模块连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Module Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
		gchrEsp8266IsConnectModule = ESP8266_CONNECT_OK;			/* 标志模块连接成功 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Module OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
}

void ESP8266_SetWorkMode(unsigned char p_uchrWorkMode)
{
	if(ESP8266_MODEL_STA == p_uchrWorkMode)
	{
		ESP8266_USARTSend("AT+CWMODE=1\r\n");								/* 设置工作模式为STA */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Send AT+CWMODE=1\r\n");
			#endif
		OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
			#ifdef DEBUG
			sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWMODE=1\", then rec \"%s\"",glb_msg1);
			DEBUG_SEND_STR(tempBufferForDebug);
			#endif
	}
	else if(ESP8266_MODEL_AP == p_uchrWorkMode)
	{
		ESP8266_USARTSend("AT+CWMODE=2\r\n");								/* 设置工作模式为STA */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Send AT+CWMODE=2\r\n");
			#endif
		OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
			#ifdef DEBUG
			sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWMODE=2\", then rec \"%s\"",glb_msg1);
			DEBUG_SEND_STR(tempBufferForDebug);
			#endif
	}
	else if(ESP8266_MODEL_STA_AP == p_uchrWorkMode)
	{
		ESP8266_USARTSend("AT+CWMODE=3\r\n");									/* 设置工作模式为AP+STA */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Send AT+CWMODE=3\r\n");
			#endif
		OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
			#ifdef DEBUG
			sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWMODE=3\", then rec \"%s\"\r\n",glb_msg1);
			DEBUG_SEND_STR(tempBufferForDebug);
			#endif
	}
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
}

int ESP8266_LinkToRouter(char* p_chrpSSID, char* p_chrpRouterPW)
{
	char p_chrpTempBuffer[50];
	sprintf(p_chrpTempBuffer,"AT+CWJAP=\"%s\",\"%s\"\r\n", p_chrpSSID, p_chrpRouterPW);
	ESP8266_USARTSend(p_chrpTempBuffer);
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send AT+CWJAP=\"%s\",\"%s\"\r\n",p_chrpSSID,p_chrpRouterPW);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	OSTimeDlyHMSM(0,0,25,0);															/* 延时30s，为数据接收提供时间，连入路由器所需时间较长 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWJAP=...\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif		
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
		memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
		OSTimeDlyHMSM(0,0,50,0);														/* 延时30s，等待busy结束 */
	}
	pOk = strstr((const char*)glb_msg1, "OK");						/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
	if(NULL == pOk)
	{
		gchrEsp8266IsConnectRouter = ESP8266_CONNECT_FAIL;	/* 标志路由器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Router Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
		gchrEsp8266IsConnectRouter = ESP8266_CONNECT_OK;		/* 标志路由器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Router OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
}

int ESP8266_LinkToServer(char* p_chrpRemoteServerIP, char* p_chrpRemoteServerPort)
{
	char p_chrpTempBuffer[50];
	ESP8266_USARTSend("AT+CIPMUX=1\r\n");									/* 设置设置多连接 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+CIPMUX=1\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);																/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPMUX=1\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;	
	sprintf(p_chrpTempBuffer,"AT+CIPSTART=4,\"TCP\",\"%s\",%s\r\n",p_chrpRemoteServerIP,p_chrpRemoteServerPort);
	ESP8266_USARTSend(p_chrpTempBuffer);												/* 向ESP8266发送AT+CIPSTART...指令，连接服务器 */
	
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send %s\r\n",p_chrpTempBuffer);
		DEBUG_SEND_STR(tempBufferForDebug);
//		sprintf(tempBufferForDebug,"[DEBUG]:Send AT+CIPSTART=..., Port is %s\r\n",p_chrpRemoteServerPort);
//		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	OSTimeDlyHMSM(0,0,10,0);															/* 延时10s，为数据接收提供时间，连入服务器 */			
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPSTART=...\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");						/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
	if(NULL == pOk)																				/* 如果没有"OK" */
	{
		gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;	/* 标志服务器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Server Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
		gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成功 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Connect Server OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
}

int ESP8266_CheckRouterLink(void)
{
	ESP8266_USARTSend("AT+CWJAP?\r\n");									/* 检测是否连上路由器 */
	OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWJAP?\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");				/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)												/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);								/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	pError = strstr((const char*)glb_msg1, "ERROR");
	pNo = strstr((const char*)glb_msg1, "No AP");
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);										/* 将glb_msg1清零 */
	RxCounter1 = 0;																			/* 将glb_msg1的数据计数清零 */
	if((NULL == pOk) || (NULL != pError) || (NULL != pNo))	/* 如果没有"OK" */
	{
		gchrEsp8266IsConnectRouter = ESP8266_CONNECT_FAIL;/* 标志路由器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Check for Connect Router Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
		gchrEsp8266IsConnectRouter = ESP8266_CONNECT_OK;	/* 标志路由器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Check for Connect Router OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
}

int ESP8266_CheckServerLink(char* p_chrpRemoteServerIP, char* p_chrpRemoteServerPort)
{
	char p_chrpTempBuffer[50];
	ESP8266_USARTSend("AT+CIPMUX=1\r\n");									/* 设置设置多连接 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+CIPMUX=1\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);																/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPMUX=1\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,25,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;	
	sprintf(p_chrpTempBuffer,"AT+CIPSTART=4,\"TCP\",\"%s\",%s\r\n",p_chrpRemoteServerIP,p_chrpRemoteServerPort);
	ESP8266_USARTSend(p_chrpTempBuffer);												/* 向ESP8266发送AT+CIPSTART...指令，连接服务器 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Before delay\r\n");
		#endif
	ESP8266_Delay(500);															/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPSTART=... for check\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");				/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																			/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,25,0);													/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	pALREADY = strstr((const char*)glb_msg1, "ALREADY");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);										/* 将glb_msg1清零 */
	RxCounter1 = 0;																			/* 将glb_msg1的数据计数清零 */
	if((NULL != pALREADY) || (NULL != pOk))																			/* 如果没有"OK" */
	{
		gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Check for Connect Server OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
	else
	{
		gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Check for Connect Server Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
}

int ESP8266_SetAP(char* p_chrpAPSSID, char* p_chrpAPPW, char* p_chrpAPIP, char* p_chrpAPPort)
{
	char p_chrpTempBuffer[50];
	sprintf(p_chrpTempBuffer,"AT+CWSAP=\"%s\",\"%s\",5,3\r\n",p_chrpAPSSID,p_chrpAPPW);
	ESP8266_USARTSend(p_chrpTempBuffer);
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+CWSAP=...\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CWSAP=...\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */

	sprintf(p_chrpTempBuffer,"AT+CIPAP=\"%s\"\r\n",p_chrpAPIP);
	ESP8266_USARTSend(p_chrpTempBuffer);
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+CIPAP=...\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPAP=...\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */

	ESP8266_USARTSend("AT+CIPMUX=1\r\n");									/* 设置设置多连接 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+CIPMUX=1\r\n");
		#endif
	OSTimeDlyHMSM(0,0,5,0);																/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPMUX=1\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;	
//
	sprintf(p_chrpTempBuffer,"AT+CIPSERVER=1,%s\r\n",p_chrpAPPort);
	ESP8266_USARTSend(p_chrpTempBuffer);
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send AT+CIPSERVER=1,%s\r\n",p_chrpAPPort);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	OSTimeDlyHMSM(0,0,5,0);															/* 延时5s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+CIPSERVER=...\", then rec \"%s\"\r\n",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)																				/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);														/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");						/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
	RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
	if(NULL == pOk)																				/* 如果没有"OK" */
	{
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Set AP+STA Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Set AP+STA OK\r\n");
			#endif
		ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
		memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
		RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
			#ifdef DEBUG
			sprintf(tempBufferForDebug,"[DEBUG]:The length of glb_msg1 is %d\r\n",strlen((const char*)glb_msg1));
			DEBUG_SEND_STR(tempBufferForDebug);
			#endif
		return ESP8266_RE_OK;
	}
}

int ESP8266_RstModule(void)
{
	ESP8266_USARTSend("AT+RST\r\n");									/* 向ESP8266发送"AT+RST\r\n"，是模块重启 */
		#ifdef DEBUG
		DEBUG_SEND_STR("[DEBUG]:Send AT+RST\r\n");
		#endif
	OSTimeDlyHMSM(0,0,10,0);											/* 延时10s，为数据接收提供时间 */
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]:Send \"AT+RST\", then rec \"%s\"",glb_msg1);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	/* 检测接收到的数据是否有busy */
	pBusy = strstr((const char*)glb_msg1, "busy");					/* 判断USART1接收数据缓存区glb_msg1中是否含有"busy" */
	if(NULL != pBusy)													/* 如果有"busy" */
	{
		OSTimeDlyHMSM(0,0,30,0);									/* 延时30s，等待busy结束 */
	}
	/* 检测接收到的数据是否有OK */
	pOk = strstr((const char*)glb_msg1, "OK");						/* 判断USART1接收数据缓存区glb_msg1中是否含有"OK" */
	ESP8266_CheckDataFromServer(glb_msg1, "SET_SN=[", "]SET_TIME=[", "]CLOSE_VALVE=[");
	memset(glb_msg1, 0, MSG1_LEN);										/* 将glb_msg1清零 */
	RxCounter1 = 0;													/* 将glb_msg1的数据计数清零 */
	if(NULL == pOk)													/* 如果没有"OK" */
	{
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Module RST Fail\r\n");
			#endif
		return ESP8266_RE_FAIL;
	}
	else
	{
			#ifdef DEBUG
			DEBUG_SEND_STR("[DEBUG]:Module RST OK\r\n");
			#endif
		return ESP8266_RE_OK;
	}
}

void ESP8266_MakeInstru_ToServer(char* p_chrpDesInstru,
								char* p_chrpSN,
								char* p_chrpCO,
								char* p_chrpTEMP,
								char* p_chrpVALVE,
								char* p_chrpSensorStatus)
{
    char p_chrpTempInstru[100];
	sprintf(p_chrpTempInstru, "SN=[%s]CO=[%s]TEMP=[%s]VALVE=[%s]STATE=[%s]", p_chrpSN, p_chrpCO, p_chrpTEMP, p_chrpVALVE, p_chrpSensorStatus);
	strcpy(p_chrpDesInstru, p_chrpTempInstru);
}

void ESP8266_Hex2Str(char* p_chrpDesStr, unsigned char p_chrSrcByte)
{
	if((p_chrSrcByte & 0x0F) <= 9)
	{
		p_chrpDesStr[1] = (p_chrSrcByte & 0x0F)+'0';
	}
	else
	{
		p_chrpDesStr[1] = ((p_chrSrcByte & 0x0F)-0x0A)+'A';
	}

	if((p_chrSrcByte>>4) <= 9)
	{
		p_chrpDesStr[0] = (p_chrSrcByte>>4)+'0';
	}
	else
	{
		p_chrpDesStr[0] = (((p_chrSrcByte & 0xF0)>>4)-0x0A)+'A';
	}
}

void ESP8266_Hex2BinStr(char* p_chrpDesStr, unsigned char p_chrSrcByte)
{
	char i;
	for(i=0; i<8; i++)
	{
		p_chrpDesStr[i] = ((p_chrSrcByte>>(7-i))&0x01)+0x30;
	}
}

void ESP8266_Str2Hex(unsigned char* p_chrDesByte, char* p_chrpSrcStr)
{
	*p_chrDesByte = (p_chrpSrcStr[1]-0x30)+(p_chrpSrcStr[0]-0x30)*10;
}

/* 比较CO浓度之差 */
char ESP8266_CompareValueOfCO(char* p_chrpStr1, char* p_chrpStr2, char p_chrErrorValue)
{
	int v1 = 0, v2 = 0;
	if((p_chrpStr1[2]>='0') && (p_chrpStr1[2]<='9'))
	{
		v1 = v1+(p_chrpStr1[2]-'0')*16;
	}
	else if((p_chrpStr1[2]>='a') && (p_chrpStr1[2]<='f'))
	{
		v1 = v1+(p_chrpStr1[2]-'a'+10)*16;
	}
	else if((p_chrpStr1[2]>='A') && (p_chrpStr1[2]<='F'))
	{
		v1 = v1+(p_chrpStr1[2]-'A'+10)*16;
	}

	if((p_chrpStr1[3]>='0') && (p_chrpStr1[3]<='9'))
	{
		v1 = v1+(p_chrpStr1[3]-'0');
	}
	else if((p_chrpStr1[3]>='a') && (p_chrpStr1[3]<='f'))
	{
		v1 = v1+(p_chrpStr1[3]-'a'+10);
	}
	else if((p_chrpStr1[3]>='A') && (p_chrpStr1[3]<='F'))
	{
		v1 = v1+(p_chrpStr1[3]-'A'+10);
	}

	if((p_chrpStr2[2]>='0') && (p_chrpStr2[2]<='9'))
	{
		v2 = v2+(p_chrpStr2[2]-'0')*16;
	}
	else if((p_chrpStr2[2]>='a') && (p_chrpStr2[2]<='f'))
	{
		v2 = v2+(p_chrpStr2[2]-'a'+10)*16;
	}
	else if((p_chrpStr2[2]>='A') && (p_chrpStr2[2]<='F'))
	{
		v2 = v2+(p_chrpStr2[2]-'A'+10)*16;
	}

	if((p_chrpStr2[3]>='0') && (p_chrpStr2[3]<='9'))
	{
		v2 = v2+(p_chrpStr2[3]-'0');
	}
	else if((p_chrpStr2[3]>='a') && (p_chrpStr2[3]<='f'))
	{
		v2 = v2+(p_chrpStr2[3]-'a'+10);
	}
	else if((p_chrpStr2[3]>='A') && (p_chrpStr2[3]<='F'))
	{
		v2 = v2+(p_chrpStr2[3]-'A'+10);
	}

	if(((v1-v2)>p_chrErrorValue) || ((v2-v1)>p_chrErrorValue))
	{
		return 1;			/* 返回1表示超出范围 */
	}
	else
	{
		return 0;			/* 返回0表示还没超出范围 */
	}
}

void ESP8266_StrCpy(char* p_chrpDesStr, char* p_chrpSrcStr,unsigned char p_uchrOffsetLen, unsigned char p_uchrLen)
{
	unsigned char i;
	for(i=0; i<p_uchrLen; i++)
	{
		p_chrpDesStr[i] = p_chrpSrcStr[p_uchrOffsetLen+i];
	}
	p_chrpDesStr[p_uchrLen] = '\0';
}

/* SERVER->ALARMER设置: SET_SN=[00:22:33:44:55:66]SET_TIME=[170702100123]CLOSE_VALVE=[00] */
void ESP8266_CheckDataFromServer(unsigned char* p_chrpSrcStr, char* p_chrpDesStr1, char* p_chrpDesStr2, char* p_chrpDesStr3)
{
	char p_LinkStatus[5] = "000";
	char p_tmpSN[30];
//	char tmp;
	
		#ifdef DEBUG
		sprintf(tempBufferForDebug,"[DEBUG]: Get From Server %s\r\n", p_chrpSrcStr);
		DEBUG_SEND_STR(tempBufferForDebug);
		#endif
	pServerData[0] = strstr((const char*)p_chrpSrcStr, p_chrpDesStr1);	/* 检测接收到的数据中是否有"SET_TIME=[" */
	pServerData[1] = strstr((const char*)p_chrpSrcStr, p_chrpDesStr2);	/* 检测接收到的数据中是否有"SET_SN=[" */
	pServerData[2] = strstr((const char*)p_chrpSrcStr, p_chrpDesStr3);	/* 检测接收到的数据中是否有"SET_VALVE=[" */
	if((NULL != pServerData[0]) || (NULL != pServerData[1]) || (NULL != pServerData[2]))
	{
		if(NULL != pServerData[0])											/* 若有"SET_TIME=[" */
		{
			ESP8266_StrCpy(p_tmpSN, pServerData[0], strlen(p_chrpDesStr1), 17);
//			strcpy(g_chrpESP8266SN, pServerData[0]+strlen(p_chrpDesStr1));	/* 将时间数据存在g_chrpESP8266SNe中 */
//			g_chrpESP8266SN[pServerData[1]-pServerData[0]-strlen(p_chrpDesStr1)] = '\0';
			if(0 != strcmp(g_chrpESP8266SN, p_tmpSN))
			{
					#ifdef DEBUG
					sprintf(tempBufferForDebug,"[DEBUG]:OriSN \"%s\", RecSN \"%s\"\r\n", g_chrpESP8266SN, p_tmpSN);
					DEBUG_SEND_STR(tempBufferForDebug);
					#endif
				return;
			}

		}
		/* END 20170916 由于实际应用中，不需要修改设备识别号，因此注释掉该段代码 END */
//		ESP8266_SSIDandPW_SaveToFlash(BANK1_WRITE_START_ADDR, g_chrpSSID, g_chrpRouterPW, g_chrpESP8266SN);
			
		
		if(NULL != pServerData[1])
		{
			ESP8266_StrCpy(g_chrpESP8266SetTime, pServerData[1], strlen(p_chrpDesStr2), 12);
//			strcpy(g_chrpESP8266SetTime, pServerData[1]+strlen(p_chrpDesStr2));	/* 将设备唯一标识符存在g_chrpESP8266SetTime中 */
//			g_chrpESP8266SetTime[pServerData[2]-pServerData[1]-strlen(p_chrpDesStr2)] = '\0';
				#ifdef DEBUG
				sprintf(tempBufferForDebug,"[DEBUG]:Get from Server ******* \"%s\"\r\n",g_chrpESP8266SetTime);
				DEBUG_SEND_STR(tempBufferForDebug);
				#endif
			/* ------------------ 在g_chr10FuncInstru中添加时间数据 ------------------------- */
			
			/* ------------------ END 在g_chr10FuncInstru中添加时间数据 END ------------------------- */
		}
		
		if(NULL != pServerData[2])
		{

			if('0' == (*(pServerData[2]+strlen(ServerDataStr[2])+2)))				/* 阀门关闭 */
			{
				g_chrSetValve = 0;
				sprintf(g_chrpESP8266VALVE, "00");
			}
			else if('1' == (*(pServerData[2]+strlen(ServerDataStr[2])+2)))			/* 阀门接通 */
			{
				g_chrSetValve = 1;
				sprintf(g_chrpESP8266VALVE, "01");
			}
		}
		
		/* 20170916 设置连接状态字符串 */
		if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectModule)
		{
			p_LinkStatus[0] = '0';			/* 连接失败置为0 */
		}
		else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectModule)
		{
			p_LinkStatus[0] = '1';			/* 连接成功置为1 */
		}
		if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectRouter)
		{
			p_LinkStatus[1] = '0';			/* 连接失败置为0 */
		}
		else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectRouter)
		{
			p_LinkStatus[1] = '1';			/* 连接成功置为1 */
		}
		if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectServer)
		{
			p_LinkStatus[2] = '0';			/* 连接失败置为0 */
		}
		else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectServer)
		{
			p_LinkStatus[2] = '1';			/* 连接成功置为1 */
		}
		
		/* 此处原先将数据通过串口写到报警器CPU，现在共用同一个CPU，应该讲数据保存，后面统一将数据通过消息邮箱发送到报警器线程 */
//		ESP8266_Send10Instru();
		
		/* SN=[xxxxxxxxxxxx]TIME=[170702100123]LINK_STATUS=[010]TEMP=[0247]VALVE=[01] */
		sprintf(g_chrp8266ToAlarm, "SN=[%s]TIME=[%s]LINK_STATUS=[%s]TEMP=[%s]VALVE=[%s]", g_chrpESP8266SN, g_chrpESP8266SetTime, p_LinkStatus, g_chrpESP8266TEMP, g_chrpESP8266VALVE);
		
		OSTimeDlyHMSM(0,0,2,0);	

//		/* 此处原先判断向报警器写入数据是否成功，将是否成功返回，现在公用同一个CPU，该代码就不应在这里 */
//		if((0x01 == glb_msg3[0]) && (0x10 == glb_msg3[1]))
//		{
//			g_chr10InstruWriteOK_flag = 1;

//		}
//		else
//		{
//			g_chr10InstruWriteOK_flag = 2;

//		}
	}
}

void ESP8266_Send10Instru(void)
{
	unsigned short int p_uint16CRCData;
	OSSchedLock();
	memset(glb_msg3, 0, MSG3_LEN);											/* 将glb_msg1清零 */
	RxCounter3 = 0;														/* 出具处理完之后，需要将数据清零 */		
	memset(&g_chr10FuncInstru[7], 0x00, WRITE_REGISTER_NUM);				/* 将原数据清空，写入新的数据 */
	/* 写入时间 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7]), g_chrpESP8266SetTime);		/* 写入年 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[8]), g_chrpESP8266SetTime+2);		/* 写入月 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[9]), g_chrpESP8266SetTime+4);		/* 写入日 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[10]), g_chrpESP8266SetTime+6);		/* 写入时 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[11]), g_chrpESP8266SetTime+8);		/* 写入分 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[12]), g_chrpESP8266SetTime+10);		/* 写入秒 */
	/* 链路故障点 */
	if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectModule)
	{
		g_chr10FuncInstru[7+9] = 0x01;										/* 写入模块连接成功 */
	}
	else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectModule)
	{
		g_chr10FuncInstru[7+9] = 0x00;										/* 写入模块连接失败 */
	}
	if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectRouter)
	{
		g_chr10FuncInstru[7+9+1] = 0x01;									/* 写入路由器连接成功 */
	}
	else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectRouter)
	{
		g_chr10FuncInstru[7+9+1] = 0x00;									/* 写入路由器连接失败 */
	}
	/* 写入当前温度 */	
	Read_Temperature(1);
	OSTimeDlyHMSM(0,0,2,0);														/* 延时1s，是数据接收完 */
	Read_Temperature(5);
	Deal_Temperature();
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10]), (char*)theTemperature);		/* 写入温度，未解析 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+1]), (char*)(theTemperature+2));		/* 写入温度，未解析 */
	/* 写入是否连接到服务器 */
	if(ESP8266_CONNECT_FAIL == gchrEsp8266IsConnectServer)
	{
		g_chr10FuncInstru[7+9+10+10] = 0x00;								/* 连入服务器失败 */
	}
	else if(ESP8266_CONNECT_OK == gchrEsp8266IsConnectServer)
	{
		g_chr10FuncInstru[7+9+10+10] = 0x01;								/* 连入服务器成功 */
	}
	/* 模块唯一识别码SN */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10]), g_chrpESP8266SN);		/* 写入设备号，即MAC地址 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10+1]), g_chrpESP8266SN+3);		/* 写入设备号，即MAC地址 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10+2]), g_chrpESP8266SN+6);		/* 写入设备号，即MAC地址 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10+3]), g_chrpESP8266SN+9);		/* 写入设备号，即MAC地址 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10+4]), g_chrpESP8266SN+12);		/* 写入设备号，即MAC地址 */
	ESP8266_Str2Hex((&g_chr10FuncInstru[7+9+10+10+10+5]), g_chrpESP8266SN+15);		/* 写入设备号，即MAC地址 */
	/* 阀门关断 */
	if(0 == g_chrSetValve)
	{
		g_chr10FuncInstru[7+9+10+10+10+10] = 0x00;								/* 0x00表示关断 */
	}
	
	g_chrSetValve = 1;		/* 每次处理完之后，需要将g_chrSetValve设为1 */
	/* ------------- END 向g_chr10FuncInstru中填入相关数据 END ------------------- */
	p_uint16CRCData = crc16(g_chr10FuncInstru, 7+9+10+10+10+10+1);
	g_chr10FuncInstru[7+9+10+10+10+10+1] = p_uint16CRCData >> 8;
	g_chr10FuncInstru[7+9+10+10+10+10+2] = p_uint16CRCData & 0xFF;
	UART3_SendBuffer(g_chr10FuncInstru, 7+9+10+10+10+10+1+2);
	OSSchedUnlock();

}


