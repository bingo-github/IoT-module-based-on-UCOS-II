#include "includes.h"
#include "globals.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "flash.h"
#include "stm32f10x_flash.h"
#include <string.h>
#include "esp8266.h"
#include "stm32f10x_it.h"
#include "Iwdg.h"
#include "crc16.h"
#include "DS18B20.h"

#define LED_MODE_AP					1
#define LED_MODE_STA				2
#define LED_MODE_STA_LINKED			3

#define CODE_SEMENT_CHECK_MODULE_LINK			1
#define CODE_SEMENT_LINK_ROUTER						2
#define CODE_SEMENT_LINK_SERVER						3
#define CODE_SEMENT_SEND_WHILE						4
#define CODE_SEMENT_AP										5

static unsigned int g_uchrAPTimeCounter = 0;						/* ���ڼ�¼����APģʽ֮���ʱ�䣬������ָ��ʱ��û���յ�APP���ݣ����Զ��ص�STAģʽ */

/* 03�����룬��������ֽ�ΪCRCУ���룬CRC16H��CRC16L */
//uint8_t g_chr03FuncInstru[] = {0x01, 0x03, 
//								(READ_REGISTER_START_ADDR >> 8), (READ_REGISTER_START_ADDR & 0xFF), 
//								(READ_REGISTER_NUM >> 8), (READ_REGISTER_NUM & 0xFF), 
//								0x00, 0x00};
uint8_t g_chr03FuncInstru[] = {0x01, 0x03, 
								(READ_REGISTER_START_ADDR >> 8), (READ_REGISTER_START_ADDR & 0xFF), 
								(READ_REGISTER_NUM >> 8), (READ_REGISTER_NUM & 0xFF), 
								0x00, 0x00};


#ifdef DEBUG
char tempBufferForDebug[250];										/* ���ڴ���2���Ե��ַ��������� */
#endif
char tempBufferForMBox[100];

char g_chrpSSID[40] = "bingo";								/* �˴���Ҫ�����û����������õ�·����SSID���ܳ���40���ֽ� */
char g_chrpRouterPW[40] = "hellobing";						/* �˴���Ҫ�����û����������õ�·�������벻�ܳ���40���ֽ� */

/* 20170701: ���ִ˴����ַ�������Ҫ��ֱ���ú궨�����滻���� */
//static char g_chrpRemoteServerIP[] = REMOTE_SERVER_IP_ADDR;		/* Զ�˷�����IP��ַ */
//static char g_chrpRemoteServerPort[] = REMOTE_SERVER_PORT;		/* Զ�˷������˿ں� */
//static char g_chrpESP8266ApSSID[] = ESP8266_AP_SSID;				/* ESP8266����APģʽ�ų��ȵ�ʱ��SSID */
//static char g_chrpESP8266ApPW[]	= ESP8266_AP_PW;				/* ESP8266����APģʽ�ų��ȵ�ʱ��WiFi���� */
//static char g_chrpESP8266ApServerIP[] = ESP8266_AP_SERVER_IP;		/* ESP8266����APģʽ�ų��ȵ�ʱ�ķ�����IP��ַ */
//static char g_chrpESP8266ApServerPort[] = ESP8266_AP_SERVER_PORT;	/* ESP8266����APģʽ�ų��ȵ�ʱ�ķ������˿ں� */

static char g_chrESP8266_ConnectNum = 0;							/* ���ڼ�¼����8266AP���豸������ͨ���� */

char g_chrLedMode = LED_MODE_STA;
char g_chr10InstruWriteOK_flag = 0;		

/* �����������������ص��ַ������� */
#define ESP8266InstruToServer_Len	100
char g_chrpESP8266InstruToServer[ESP8266InstruToServer_Len];						/* ���͸����������ַ�����������ע�⣬�˴����øû�������СΪ80�����������ϴ���Ҫ���û�������Ӧ���� */
char g_chrpESP8266SN[35] = "aa:bb:cc:dd:ee:ff";							
char g_chrpESP8266CO[8] = "000A";							/* ��ǰCOŨ�ȣ���16���Ƶ��ַ���ʾ */
static char g_chrpESP8266CO_PRE[8] = "xxxx";						/* ��һ��COŨ�ȣ���16���Ƶ��ַ���ʾ������ǰ�����εıȽ� */
char g_chrpESP8266TEMP[8] = "0257";							/* ��ǰ�¶ȣ���10���Ƶ��ַ���ʾ */
char g_chrpESP8266TEMP_PRE[8] = "xxxx";						/* ��һ���¶ȣ���10���Ƶ��ַ���ʾ������ǰ�����εıȽ� */
static char g_chrpESP8266SensorStatus[12] = "01010101";					/* ��ǰ������״̬ */
static char g_chrpESP8266SensorStatus_PRE[12] = "01010101";				/* ��һ�δ�����״̬������ǰ�����εıȽ� */
char g_chrpESP8266VALVE[8] = "01";							/* ��ǰ����״̬ */
static char g_chrpESP8266VALVE_PRE[8] = "01";						/* ��һ�η���״̬ */
char g_chrpESP8266SetTime[20] = "170702100123";				/* �ӷ�������ȡ��ʱ�� */
char g_chrSetValve = 1;												/* �ӷ�������ȡ����Ҫ���÷���״̬ */

/* 20170916 ʹ��ͬһ��CPU֮�󣬷��͸��������̵߳����� */
char g_chrp8266ToAlarm[100];

OS_STK task_ALARM_stk[TASK_ALARM_STK_SIZE];		  					/* ����ջ */
OS_STK task_ESP8266_stk[TASK_ESP8266_STK_SIZE];		  				/* ����ջ */
OS_STK task_TEMP_stk[TASK_TEMP_STK_SIZE];

OS_EVENT *Str_Box_toALARM;
OS_EVENT *Str_Box_toESP8266;



void Task_Start(void *p_arg)
{
    (void)p_arg;                									// 'p_arg' ��û���õ�����ֹ��������ʾ����
	SysTick_init();
	
	Str_Box_toALARM = OSMboxCreate ((void*)0);  //������Ϣ����
	Str_Box_toESP8266 = OSMboxCreate((void*)0); //������Ϣ����
	
	OSTaskCreateExt(Task_ALARM,
									(void *)0,		  				//��������2
									&task_ALARM_stk[TASK_ALARM_STK_SIZE-1],
									TASK_ALARM_PRIO,
									TASK_ALARM_PRIO,
									&task_ALARM_stk[0],
									TASK_ALARM_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	
	
	OSTaskCreateExt(Task_ESP8266,
									(void *)0,		  				//��������3
									&task_ESP8266_stk[TASK_ESP8266_STK_SIZE-1],
									TASK_ESP8266_PRIO,
									TASK_ESP8266_PRIO,
									&task_ESP8266_stk[0],
									TASK_ESP8266_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	
									
	OSTaskCreateExt(Task_TEMP,
									(void *)0,		  				//��������3
									&task_TEMP_stk[TASK_TEMP_STK_SIZE-1],
									TASK_TEMP_PRIO,
									TASK_TEMP_PRIO,
									&task_TEMP_stk[0],
									TASK_TEMP_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	

    while (1)
    {
		IWDG_Feed();												/* �Կ��Ź����ж���ι�� */
		
		/* ָʾ����˸ */
		if(LED_MODE_AP == g_chrLedMode)
		{
			LED1( ON );
			OSTimeDlyHMSM(0, 0, 0,530);
			LED1( OFF);   
			OSTimeDlyHMSM(0, 0,0,530);
		}
		else if(LED_MODE_STA == g_chrLedMode)
		{
			LED1( ON );
			OSTimeDlyHMSM(0, 0, 1,30);
			LED1( OFF);   
			OSTimeDlyHMSM(0, 0,1,30);
		}
		else if(LED_MODE_STA_LINKED == g_chrLedMode)
		{
			LED1( ON );
			OSTimeDlyHMSM(0, 0, 2,30);
			LED1( OFF);   
			OSTimeDlyHMSM(0, 0, 2,30);
		}
		
		//OSTimeDlyHMSM(0, 0,5,0);					/* ������Ҫ��һ��delay����������ʹ���ȼ����͵Ľ����ܹ����� */
		// RS485_TraBuffer("I am live!\r\n", strlen("I am live!\r\n"));
		
		if(g_uchrAPTimeCounter > 0)
		{
			g_uchrAPTimeCounter++;
		}
    }
}

//����2
void Task_ALARM(void *p_arg)
{
//	uint16_t p_uint16CRCData;
	char *mboxRec;
	char *mboxSend;
	INT8U  err, mboxTimes=2;
//	char str[100];
    (void)p_arg;                	
    while (1)
    {
		/* ÿ��һ��ʱ���򱨾�������03�����룬���Զ�ȡ���������� */
//		OSTimeDlyHMSM(0, 0, 15,0);				/* ������������֮�󣬸�һ��ʱ�����ж�����ȡ�������� */
		
		/* --------------------------- ����03������ ------------------------- */
		/* 0x03��ȡ������Ӧ���ǻ����̶������ */
			mboxRec = OSMboxPend(Str_Box_toESP8266,mboxTimes,&err);
			if('a' == mboxRec[0])
			{

			}
			
//		p_uint16CRCData = crc16(g_chr03FuncInstru, 6);
//		g_chr03FuncInstru[6] = p_uint16CRCData >> 8;
//		g_chr03FuncInstru[7] = p_uint16CRCData & 0xFF;
//		UART3_SendBuffer(g_chr03FuncInstru, 8);
		/* --------------------------- END ����03������ END ------------------------- */
		
		OSTimeDlyHMSM(0, 0, 6,250);	
		/* ����Ŀǰ�ǰ���һ���Ĵ���һ���ֽ������ */
////		if((0x01 == glb_msg3[0]) && (0x03 == glb_msg3[1]) /*&& (9 == RxCounter3)*/)
////		{
////			/* ----------------- �˴�Ӧ�ý���0x03���ܷ����봦�� ---------- */
////			ESP8266_Hex2Str(g_chrpESP8266CO, glb_msg3[3]);
////			ESP8266_Hex2Str(g_chrpESP8266CO+2, glb_msg3[4]);
//////			ESP8266_Hex2BinStr(g_chrpESP8266SensorStatus, glb_msg3[5]);
//////			ESP8266_Hex2Str(g_chrpESP8266VALVE, glb_msg3[6]);
////			/* ----------------- END �˴�Ӧ�ý���0x03���ܷ����봦�� END ---------- */
////		}
////		memset(glb_msg3, 0, MSG3_LEN);											/* ��glb_msg1���� */
////		RxCounter3 = 0;														/* ���ߴ�����֮����Ҫ���������� */		
			mboxSend = g_chrp8266ToAlarm;
			//sprintf(mboxSend,"bcd");
			OSMboxPost(Str_Box_toALARM,mboxSend);
		OSTimeDlyHMSM(0, 0, 6,250);	
		
    }
}

/* ����3 */
void Task_ESP8266(void *p_arg)
{
//#define CODE_SEMENT_CHECK_MODULE_LINK			1
//#define CODE_SEMENT_LINK_ROUTER						2
//#define CODE_SEMENT_LINK_SERVER						3
//#define CODE_SEMENT_SEND_WHILE						4
//#define CODE_SEMENT_AP										5
	int codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
	int p_intRe;
	int i;
	(void)p_arg;       
	
	/* 20170919 �ֶ����д��SSID������ĳ���� */
//	#ifdef DEBUG
//	DEBUG_SEND_STR("[DEBUG]:HERE 1\r\n");
//	#endif
//	OSTimeDlyHMSM(0, 0, 3,0);	
//	ESP8266_SSIDandPW_SaveToFlash(BANK1_WRITE_START_ADDR,"Q-me", "anze2009", "12345678901234530");
//	OSTimeDlyHMSM(0, 0, 3,0);	
//	#ifdef DEBUG
//	DEBUG_SEND_STR("[DEBUG]:HERE 2\r\n");
//	#endif
	/* END 20170919 �ֶ����д��SSID������ĳ���� END */
	
	ESP8266_RstModule();						/* �������������ӷ�����ʧ�ܣ�������ģ�飨��ʵ�����Գ��Ի�����ͨ���ŵķ����� */
		while(1)
		{
//			goto Loop_GetSSIDandPW;
//				#ifdef DEBUG
//				OSTimeDlyHMSM(0, 0,5,0);														/* DEBUG ����ʱʹ�� */
//				#endif
Loop_OutOfAll:
				#ifdef DEBUG
				DEBUG_SEND_STR("[DEBUG]:Into Loop_OutOfAll\r\n");
				#endif
			if(CODE_SEMENT_CHECK_MODULE_LINK == codeSement)
			{
				while(ESP8266_RE_FAIL == ESP8266_CheckModuleLink())
				{
					OSTimeDlyHMSM(0,0,10,0);											/* ??5s,????????? */
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:goto Loop_CheckModuleLink\r\n");
						#endif
				}
				codeSement = CODE_SEMENT_LINK_ROUTER;
			}
			
			if(CODE_SEMENT_LINK_ROUTER == codeSement)
			{
					#ifdef DEBUG
					DEBUG_SEND_STR("[DEBUG]:Into Loop_LinkToRouter\r\n");
					#endif
				/* ------------------------- ����ģʽ1���� ------------------------------- */			
				ESP8266_SetWorkMode(ESP8266_MODEL_STA);
				g_chrLedMode = LED_MODE_STA;
				/* ------------------------- ����ģʽ1���� END ------------------------------- */			
				
				/* ------------------------- ����·���� ------------------------------- */			
				memset(g_chrpSSID, 0, strlen(g_chrpSSID));												/* ��SSID���� */
				memset(g_chrpRouterPW, 0, strlen(g_chrpRouterPW));								/* ��RouterPW���� */
				memset(g_chrpESP8266SN, 0, strlen(g_chrpESP8266SN));								/* ��RouterPW���� */
				p_intRe = ESP8266_SSIDandPW_ReadFromFlash(BANK1_WRITE_START_ADDR, g_chrpSSID, g_chrpRouterPW, g_chrpESP8266SN);			
				if(ESP8266_RE_FAIL == p_intRe)
				{
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:Read flash fail, then goto Loop_GetSSIDandPW\r\n");
						#endif
					codeSement = CODE_SEMENT_AP;
					goto Loop_OutOfAll;
				}
				p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
				if(ESP8266_RE_FAIL == p_intRe)
				{
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:Try to link router agian\r\n");
						#endif
					p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
					if(ESP8266_RE_FAIL == p_intRe)
					{
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:goto Loop_GetSSIDandPW\r\n");
							#endif
						codeSement = CODE_SEMENT_AP;
						goto Loop_OutOfAll;
					}
				}
				codeSement = CODE_SEMENT_LINK_SERVER;
			}
			
			if(CODE_SEMENT_LINK_SERVER == codeSement)
			{
					#ifdef DEBUG
					DEBUG_SEND_STR("[DEBUG]:Into Loop_LinkToServer\r\n");
					#endif
				p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
				if(ESP8266_RE_FAIL == p_intRe)
				{
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* ��־����������ʧ�� */
					p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
					if(ESP8266_RE_FAIL == p_intRe)
					{
						gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* ��־����������ʧ�� */
						ESP8266_RstModule();						/* �������������ӷ�����ʧ�ܣ�������ģ�飨��ʵ�����Գ��Ի�����ͨ���ŵķ����� */
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:goto Loop_CheckModuleLink\r\n");
							#endif
						codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
						goto Loop_OutOfAll;
					}
					else
					{
						gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* ��־���������ӳ� */
					}
				}
				else
				{
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* ��־���������ӳ� */
				}
				codeSement = CODE_SEMENT_SEND_WHILE;
			}
			
			if(CODE_SEMENT_SEND_WHILE == codeSement)
			{
				while(1)
				{
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:Into while(1) to send data\r\n");
						#endif
					g_chrLedMode = LED_MODE_STA_LINKED;
//					memset(glb_msg1, 0, MSG1_LEN);											/* ��glb_msg1���� */
//					RxCounter1 = 0;																				/* ��glb_msg1�����ݼ������� */
					OSTimeDlyHMSM(0,0,3,0);		
					
					while(ESP8266_RE_FAIL == ESP8266_CheckModuleLink())
					{
						OSTimeDlyHMSM(0,0,10,0);											/* ??5s,????????? */
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:Module Link error\r\n");
							#endif
					}
					
					/* ------------------------- ��������ǰ��ǰ����ATģ�����Ӽ�� END ------------------------------- */
					
					/* -------------------------��������ǰ������·�������Ӽ�� --------------------------- */	
	//				p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
	//				if(ESP8266_RE_FAIL == p_intRe)
	//				{
	//						#ifdef DEBUG
	//						DEBUG_SEND_STR("[DEBUG]:Try to link router agian\r\n");
	//						#endif
	//					p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
	//					if(ESP8266_RE_FAIL == p_intRe)
	//					{
	//							#ifdef DEBUG
	//							DEBUG_SEND_STR("[DEBUG]:goto Loop_GetSSIDandPW\r\n");
	//							#endif
	//						codeSement = CODE_SEMENT_AP;
	//						goto Loop_OutOfAll;
	//					}
	//				}
					
					p_intRe = ESP8266_CheckRouterLink();
					if(ESP8266_RE_FAIL == p_intRe)
					{
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:Check for Connect Router Fail\r\n");
							#endif
						p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
						if(ESP8266_RE_FAIL == p_intRe)
						{
								#ifdef DEBUG
								DEBUG_SEND_STR("[DEBUG]:Try to link router agian\r\n");
								#endif
							p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
							if(ESP8266_RE_FAIL == p_intRe)
							{
									#ifdef DEBUG
									DEBUG_SEND_STR("[DEBUG]:goto Loop_GetSSIDandPW\r\n");
									#endif
								codeSement = CODE_SEMENT_AP;
								goto Loop_OutOfAll;
							}
						}
					}
					
					
//					if(1 == g_chr10InstruWriteOK_flag)
//					{
//						sprintf(g_chrpESP8266InstruToServer,"SN=[%s]CO=[%s]TEMP=[%s]STATE=[%s]SET_OK",g_chrpESP8266SN, g_chrpESP8266CO, theTemperature, g_chrpESP8266SensorStatus);
//						ESP8266_Send(g_chrpESP8266InstruToServer, strlen(g_chrpESP8266InstruToServer),'4');
//						g_chr10InstruWriteOK_flag = 0;
//					}
//					else if(2 == g_chr10InstruWriteOK_flag)
//					{
//						sprintf(g_chrpESP8266InstruToServer,"SN=[%s]CO=[%s]TEMP=[%s]STATE=[%s]SET_ERR",g_chrpESP8266SN, g_chrpESP8266CO, theTemperature, g_chrpESP8266SensorStatus);
//						ESP8266_Send(g_chrpESP8266InstruToServer, strlen(g_chrpESP8266InstruToServer),'4');
//						g_chr10InstruWriteOK_flag = 0;
//					}
					/* -------------------------��������ǰ������·�������Ӽ�� END --------------------------- */
					
					/* -------------------------��������ǰ�����з����������Ӽ�� ------------------------------ */
//					p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
//					if(ESP8266_RE_FAIL == p_intRe)
//					{
//						gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* ��־����������ʧ�� */
//						p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
//						if(ESP8266_RE_FAIL == p_intRe)
//						{
//							gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* ��־����������ʧ�� */
//							ESP8266_RstModule();						/* �������������ӷ�����ʧ�ܣ�������ģ�飨��ʵ�����Գ��Ի�����ͨ���ŵķ����� */
//								#ifdef DEBUG
//								DEBUG_SEND_STR("[DEBUG]:goto Loop_CheckModuleLink\r\n");
//								#endif
//							codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
//							goto Loop_OutOfAll;
//						}
//						else
//						{
//							gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* ��־���������ӳ� */
//						}
//					}
//					else
//					{
//						gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* ��־���������ӳ� */
//					}
					
				p_intRe = ESP8266_CheckServerLink(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
				if(ESP8266_RE_FAIL == p_intRe)
				{
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* ��־����������ʧ�� */
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:goto Loop_LinkToServer\r\n");
						#endif
					codeSement = CODE_SEMENT_LINK_SERVER;
					goto Loop_OutOfAll;														/* ������������ʧ�ܣ�ֻ�������������ص�Loop_LinkToServer */
				}

//				if(1 == g_chr10InstruWriteOK_flag)
//				{
//					sprintf(g_chrpESP8266InstruToServer,"SN=[%s]CO=[%s]TEMP=[%s]STATE=[%s]SET_OK",g_chrpESP8266SN, g_chrpESP8266CO, theTemperature, g_chrpESP8266SensorStatus);
//					ESP8266_Send(g_chrpESP8266InstruToServer, strlen(g_chrpESP8266InstruToServer),'4');
//					g_chr10InstruWriteOK_flag = 0;
//				}
//				else if(2 == g_chr10InstruWriteOK_flag)
//				{
//					sprintf(g_chrpESP8266InstruToServer,"SN=[%s]CO=[%s]TEMP=[%s]STATE=[%s]SET_ERR",g_chrpESP8266SN, g_chrpESP8266CO, theTemperature, g_chrpESP8266SensorStatus);
//					ESP8266_Send(g_chrpESP8266InstruToServer, strlen(g_chrpESP8266InstruToServer),'4');
//					g_chr10InstruWriteOK_flag = 0;
//				}
					/* -------------------------��������ǰ�����з����������Ӽ�� END ------------------------------ */
					
					/* �ڴ˴����������ݴ�������͸������� */
					Read_Temperature(1);
					OSTimeDlyHMSM(0,0,2,0);														/* ��ʱ1s�������ݽ����� */
					Read_Temperature(5);
					Deal_Temperature();
					strcpy(g_chrpESP8266TEMP, (const char*)theTemperature);
	//				OSSchedLock();
					memset(g_chrpESP8266InstruToServer, 0, ESP8266InstruToServer_Len);
					p_intRe = ESP8266_CompareValueOfCO(g_chrpESP8266CO, g_chrpESP8266CO_PRE, 5);
					if((1 == p_intRe) ||
						(0 != strcmp(g_chrpESP8266TEMP, g_chrpESP8266TEMP_PRE)) ||
						(0 != strcmp(g_chrpESP8266SensorStatus, g_chrpESP8266SensorStatus_PRE)) ||
						(0 != strcmp(g_chrpESP8266VALVE, g_chrpESP8266VALVE_PRE)))
					{
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:Data different!\r\n");
							#endif
						ESP8266_MakeInstru_ToServer(g_chrpESP8266InstruToServer,
													g_chrpESP8266SN,
													g_chrpESP8266CO,
													g_chrpESP8266TEMP,
													g_chrpESP8266VALVE,
													g_chrpESP8266SensorStatus);

						strcpy(g_chrpESP8266CO_PRE, g_chrpESP8266CO);
						strcpy(g_chrpESP8266TEMP_PRE, g_chrpESP8266TEMP);
						strcpy(g_chrpESP8266SensorStatus_PRE, g_chrpESP8266SensorStatus);
						strcpy(g_chrpESP8266VALVE_PRE, g_chrpESP8266VALVE);
					}
					else
					{
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:Data Same!\r\n");
							#endif
						sprintf(g_chrpESP8266InstruToServer,"SN=[%s]",g_chrpESP8266SN);
					}
					
	//				OSSchedUnlock();
					ESP8266_Send(g_chrpESP8266InstruToServer, strlen(g_chrpESP8266InstruToServer),'4');
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:Send Data to Server\r\n");
						#endif
					OSTimeDlyHMSM(0,0,5,0);															/* ??30s,?????????,??????????? */
				}
				codeSement = CODE_SEMENT_AP;
			}
			
			if(CODE_SEMENT_AP == codeSement)
			{
					#ifdef DEBUG
					DEBUG_SEND_STR("[DEBUG]:Into Loop_GetSSIDandPW\r\n");
					#endif
				g_chrLedMode = LED_MODE_AP;
				/* ------------------------- ���й���ģʽ���� --------------------------------- */
				ESP8266_SetWorkMode(ESP8266_MODEL_STA_AP);
				/* ------------------------- ���й���ģʽ���� END --------------------------------- */
				p_intRe = ESP8266_SetAP(ESP8266_AP_SSID, ESP8266_AP_PW, ESP8266_AP_SERVER_IP, ESP8266_AP_SERVER_PORT);
				if(ESP8266_RE_FAIL == p_intRe)
				{
					codeSement = CODE_SEMENT_AP;
					goto Loop_OutOfAll;
				}
				else if(ESP8266_RE_OK == p_intRe)
				{
					g_uchrAPTimeCounter = 1;
					/* AP���óɹ��󣬽��н���APP���ݵ����� */
					while(1)
					{
Loop_GetSSIDandPW_WHILE:
						if(500 < g_uchrAPTimeCounter)			/* ���APʱ�䳬ʱ */
						{
							g_uchrAPTimeCounter = 0;
							codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
							goto Loop_OutOfAll;
						}
						if(0 < strlen((const char*)glb_msg1))
						{
							OSTimeDlyHMSM(0,0,2,0);														/* ��ʱ1s�������ݽ����� */
								#ifdef DEBUG
								sprintf(tempBufferForDebug,"[DEBUG]:In AP MODE, Rec \"%s\"\r\n",glb_msg1);
								DEBUG_SEND_STR(tempBufferForDebug);
								#endif
							pConnect = strstr((const char*)glb_msg1, "CONNECT");				/* �����յ����������Ƿ���"CONNECT" */
							if(NULL != pConnect)																/* �����CONNECT */
							{
								g_chrESP8266_ConnectNum = glb_msg1[0];									/* ��ESP8266_ConnectNum�б�������AP���豸ͨ����'0','1','2','3','4' */
									#ifdef DEBUG
									sprintf(tempBufferForDebug,"[DEBUG]:The CONNECT Channel is \"%c\"\r\n",g_chrESP8266_ConnectNum);
									DEBUG_SEND_STR(tempBufferForDebug);
									#endif
								memset(glb_msg1, 0, MSG1_LEN);									/* ��glb_msg1���� */
								RxCounter1 = 0;																		/* ��glb_msg1�����ݼ������� */
								goto Loop_GetSSIDandPW_WHILE;											/* ��ת��Loop_GetSSIDandPW_WHILE������ѭ�� */
							}
							/* ����Ƿ�Ϊ�����豸�Ͽ��¼� */
							pClosed = strstr((const char*)glb_msg1, "CLOSED");	/* �����յ����������Ƿ���"CLOSED" */
							if(NULL != pClosed)																	/* �����CLOSED */
							{
								g_chrESP8266_ConnectNum = 0;
									#ifdef DEBUG
									DEBUG_SEND_STR("[DEBUG]:Clear the ESP8266_ConnectNum\r\n");
									#endif
								memset(glb_msg1, 0, MSG1_LEN);									/* ��glb_msg1���� */
								RxCounter1 = 0;																		/* ��glb_msg1�����ݼ������� */
								goto Loop_GetSSIDandPW_WHILE;											/* ��ת��Loop_GetSSIDandPW_WHILE������ѭ�� */
							}
							/* ����Ƿ�Ϊ����APP���������¼� */		/* char APDataStr[5][8] = {"SSID=\"", "\"PW=\"", "\"SN=\"", "\"DT=\"", "\"\r\n"}; */
							pAPData[0] = strstr((const char*)glb_msg1, APDataStr[0]);	/* �����յ����������Ƿ���"SSID=\"" */
							pAPData[1] = strstr((const char*)glb_msg1, APDataStr[1]);	/* �����յ����������Ƿ���"\"PW=\"" */
							pAPData[2] = strstr((const char*)glb_msg1, APDataStr[2]);	/* �����յ����������Ƿ���"\"SN=\"" */
							pAPData[3] = strstr((const char*)glb_msg1, APDataStr[3]);	/* �����յ����������Ƿ���"\"DT=\"" */
							pAPData[4] = strstr((const char*)glb_msg1, APDataStr[4]);	/* �����յ����������Ƿ���"\"\r\n" */
							if((NULL != pAPData[0]) && (NULL != pAPData[1])  && (NULL != pAPData[2]) &&
							(NULL != pAPData[3]) && (NULL != pAPData[4]))	/* �������5�������ַ������� */
							{
								g_uchrAPTimeCounter = 1;
								for(i=0; i<4; i++)
								{
									ESP8266_Send("JOIN_OK\r\n", strlen("JOIN_OK\r\n"),g_chrESP8266_ConnectNum);
									OSTimeDlyHMSM(0,0,10,0);														/* ��ʱ30s���ȴ�busy���� */
										#ifdef DEBUG
										sprintf(tempBufferForDebug,"[DEBUG]:Send \"JOIN_OK\", then rec \"%s\"\r\n",glb_msg1);
										DEBUG_SEND_STR(tempBufferForDebug);
										#endif
//									pOk = strstr((const char*)glb_msg1, "OK");					/* ??USART1???????glb_msg1?????"busy" */
//									//memset(glb_msg1, 0, MSG1_LEN);											/* ��glb_msg1���� */
//									if(NULL == pOk)													/* ???"OK" */
//									{
//										goto Loop_SendOK;
//									}
								}
								/* ��ȡSSID */
								ESP8266_StrCpy(g_chrpSSID, pAPData[0], strlen(APDataStr[0]), pAPData[1]-pAPData[0]-strlen(APDataStr[0]));
								/* ��ȡ·�������� */							
								ESP8266_StrCpy(g_chrpRouterPW, pAPData[1], strlen(APDataStr[1]), pAPData[2]-pAPData[1]-strlen(APDataStr[1]));
								/* ��ȡSN */
								ESP8266_StrCpy(g_chrpESP8266SN, pAPData[2], strlen(APDataStr[2]),pAPData[3]-pAPData[2]-strlen(APDataStr[2]));				/* SN����Ϊ17 */
								/* ��ȡʱ�� */
								ESP8266_StrCpy(g_chrpESP8266SetTime, pAPData[3], strlen(APDataStr[3]), pAPData[4]-pAPData[3]-strlen(APDataStr[3]));			/* ʱ�䳤��Ϊ12 */
									#ifdef DEBUG
									sprintf(tempBufferForDebug,"[DEBUG]:New SSID is \"%s\"\r\n",g_chrpSSID);
									DEBUG_SEND_STR(tempBufferForDebug);
									sprintf(tempBufferForDebug,"[DEBUG]:New RouterPW is \"%s\"\r\n",g_chrpRouterPW);
									DEBUG_SEND_STR(tempBufferForDebug);
									sprintf(tempBufferForDebug,"[DEBUG]:New SN is \"%s\"\r\n",g_chrpESP8266SN);
									DEBUG_SEND_STR(tempBufferForDebug);
									sprintf(tempBufferForDebug,"[DEBUG]:New DT is \"%s\"\r\n",g_chrpESP8266SetTime);
									DEBUG_SEND_STR(tempBufferForDebug);
									#endif
								memset(glb_msg1, 0, MSG1_LEN);									/* ��glb_msg1���� */
								RxCounter1 = 0;																		/* ��glb_msg1�����ݼ������� */
								ESP8266_Send10Instru();							
								/* ���µ�SSID����������·���� */
								/* ------------------------- ����·���� ------------------------------- */			
								p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
								OSTimeDlyHMSM(0,0,5,0);														/* ��ʱ30s���ȴ�busy���� */
								ESP8266_SSIDandPW_SaveToFlash(BANK1_WRITE_START_ADDR,g_chrpSSID, g_chrpRouterPW, g_chrpESP8266SN);
									#ifdef DEBUG
									DEBUG_SEND_STR("[DEBUG]:Save SSID and RouterPW Finish\r\n");
									#endif
								OSTimeDlyHMSM(0,0,5,0);														/* ��ʱ30s���ȴ�busy���� */
								memset(glb_msg1, 0, MSG1_LEN);											/* ��glb_msg1���� */
								RxCounter1 = 0;																				/* ��glb_msg1�����ݼ������� */
								codeSement = CODE_SEMENT_LINK_ROUTER;
								goto Loop_OutOfAll;
								
							}
							else
							{
								memset(glb_msg1, 0, MSG1_LEN);											/* ��glb_msg1���� */
								RxCounter1 = 0;																				/* ��glb_msg1�����ݼ������� */
							}
						}
					}
				}
			}
		}
}

void Task_TEMP(void *p_arg)
{
	               	
	char *mboxRec_tmp;
	char *mboxSend;
	INT8U  err, mboxTimes=2;
	char str[20] = "abc";
	(void)p_arg; 
	while(1)
	{
		mboxRec_tmp = OSMboxPend(Str_Box_toALARM,mboxTimes,&err);
		if('b' == mboxRec_tmp[0])
		{
//			sprintf(tempBufferForMBox,"[DEBUG-ALARM-MBOX]:%s\r\n",mboxRec_tmp);
//			DEBUG_SEND_STR(tempBufferForMBox);
		}
		OSTimeDlyHMSM(0, 0, 5,250);	
		
		mboxSend = str;
		//sprintf(mboxSend,"abc");
			OSMboxPost(Str_Box_toESP8266,mboxSend);
		OSTimeDlyHMSM(0, 0, 5,250);	
	}
}
