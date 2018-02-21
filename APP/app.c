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

static unsigned int g_uchrAPTimeCounter = 0;						/* 用于记录进入AP模式之后的时间，当超过指定时间没有收到APP数据，则自动回到STA模式 */

/* 03功能码，最后两个字节为CRC校验码，CRC16H，CRC16L */
//uint8_t g_chr03FuncInstru[] = {0x01, 0x03, 
//								(READ_REGISTER_START_ADDR >> 8), (READ_REGISTER_START_ADDR & 0xFF), 
//								(READ_REGISTER_NUM >> 8), (READ_REGISTER_NUM & 0xFF), 
//								0x00, 0x00};
uint8_t g_chr03FuncInstru[] = {0x01, 0x03, 
								(READ_REGISTER_START_ADDR >> 8), (READ_REGISTER_START_ADDR & 0xFF), 
								(READ_REGISTER_NUM >> 8), (READ_REGISTER_NUM & 0xFF), 
								0x00, 0x00};


#ifdef DEBUG
char tempBufferForDebug[250];										/* 用于串口2调试的字符串缓冲区 */
#endif
char tempBufferForMBox[100];

char g_chrpSSID[40] = "bingo";								/* 此处需要提醒用户，其所设置的路由器SSID不能超过40个字节 */
char g_chrpRouterPW[40] = "hellobing";						/* 此处需要提醒用户，其所设置的路由器密码不能超过40个字节 */

/* 20170701: 发现此处的字符串不需要，直接用宏定义来替换就行 */
//static char g_chrpRemoteServerIP[] = REMOTE_SERVER_IP_ADDR;		/* 远端服务器IP地址 */
//static char g_chrpRemoteServerPort[] = REMOTE_SERVER_PORT;		/* 远端服务器端口号 */
//static char g_chrpESP8266ApSSID[] = ESP8266_AP_SSID;				/* ESP8266处于AP模式放出热点时的SSID */
//static char g_chrpESP8266ApPW[]	= ESP8266_AP_PW;				/* ESP8266处于AP模式放出热点时的WiFi密码 */
//static char g_chrpESP8266ApServerIP[] = ESP8266_AP_SERVER_IP;		/* ESP8266处于AP模式放出热点时的服务器IP地址 */
//static char g_chrpESP8266ApServerPort[] = ESP8266_AP_SERVER_PORT;	/* ESP8266处于AP模式放出热点时的服务器端口号 */

static char g_chrESP8266_ConnectNum = 0;							/* 用于记录连入8266AP的设备所处的通道号 */

char g_chrLedMode = LED_MODE_STA;
char g_chr10InstruWriteOK_flag = 0;		

/* 向服务器发送数据相关的字符串变量 */
#define ESP8266InstruToServer_Len	100
char g_chrpESP8266InstruToServer[ESP8266InstruToServer_Len];						/* 发送给服务器的字符串缓冲区，注意，此处设置该缓冲区大小为80，若数据量较大，需要将该缓冲区相应增大 */
char g_chrpESP8266SN[35] = "aa:bb:cc:dd:ee:ff";							
char g_chrpESP8266CO[8] = "000A";							/* 当前CO浓度，以16进制的字符显示 */
static char g_chrpESP8266CO_PRE[8] = "xxxx";						/* 上一次CO浓度，以16进制的字符显示，用于前后两次的比较 */
char g_chrpESP8266TEMP[8] = "0257";							/* 当前温度，以10进制的字符显示 */
char g_chrpESP8266TEMP_PRE[8] = "xxxx";						/* 上一次温度，以10进制的字符显示，用于前后两次的比较 */
static char g_chrpESP8266SensorStatus[12] = "01010101";					/* 当前传感器状态 */
static char g_chrpESP8266SensorStatus_PRE[12] = "01010101";				/* 上一次传感器状态，用于前后两次的比较 */
char g_chrpESP8266VALVE[8] = "01";							/* 当前阀门状态 */
static char g_chrpESP8266VALVE_PRE[8] = "01";						/* 上一次阀门状态 */
char g_chrpESP8266SetTime[20] = "170702100123";				/* 从服务器获取的时间 */
char g_chrSetValve = 1;												/* 从服务器获取的想要设置阀门状态 */

/* 20170916 使用同一个CPU之后，发送给报警器线程的数据 */
char g_chrp8266ToAlarm[100];

OS_STK task_ALARM_stk[TASK_ALARM_STK_SIZE];		  					/* 定义栈 */
OS_STK task_ESP8266_stk[TASK_ESP8266_STK_SIZE];		  				/* 定义栈 */
OS_STK task_TEMP_stk[TASK_TEMP_STK_SIZE];

OS_EVENT *Str_Box_toALARM;
OS_EVENT *Str_Box_toESP8266;



void Task_Start(void *p_arg)
{
    (void)p_arg;                									// 'p_arg' 并没有用到，防止编译器提示警告
	SysTick_init();
	
	Str_Box_toALARM = OSMboxCreate ((void*)0);  //创建消息邮箱
	Str_Box_toESP8266 = OSMboxCreate((void*)0); //创建消息邮箱
	
	OSTaskCreateExt(Task_ALARM,
									(void *)0,		  				//创建任务2
									&task_ALARM_stk[TASK_ALARM_STK_SIZE-1],
									TASK_ALARM_PRIO,
									TASK_ALARM_PRIO,
									&task_ALARM_stk[0],
									TASK_ALARM_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	
	
	OSTaskCreateExt(Task_ESP8266,
									(void *)0,		  				//创建任务3
									&task_ESP8266_stk[TASK_ESP8266_STK_SIZE-1],
									TASK_ESP8266_PRIO,
									TASK_ESP8266_PRIO,
									&task_ESP8266_stk[0],
									TASK_ESP8266_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	
									
	OSTaskCreateExt(Task_TEMP,
									(void *)0,		  				//创建任务3
									&task_TEMP_stk[TASK_TEMP_STK_SIZE-1],
									TASK_TEMP_PRIO,
									TASK_TEMP_PRIO,
									&task_TEMP_stk[0],
									TASK_TEMP_STK_SIZE,
									(void *)0,
									OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR); 	

    while (1)
    {
		IWDG_Feed();												/* 对看门狗进行定期喂狗 */
		
		/* 指示灯闪烁 */
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
		
		//OSTimeDlyHMSM(0, 0,5,0);					/* 至少需要有一个delay，这样才能使优先级更低的进程能够运行 */
		// RS485_TraBuffer("I am live!\r\n", strlen("I am live!\r\n"));
		
		if(g_uchrAPTimeCounter > 0)
		{
			g_uchrAPTimeCounter++;
		}
    }
}

//任务2
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
		/* 每个一定时间向报警器发送03功能码，用以读取报警器数据 */
//		OSTimeDlyHMSM(0, 0, 15,0);				/* 心跳包发送完之后，隔一定时间来判断所收取到的数据 */
		
		/* --------------------------- 发送03功能码 ------------------------- */
		/* 0x03读取功能码应该是基本固定不变的 */
			mboxRec = OSMboxPend(Str_Box_toESP8266,mboxTimes,&err);
			if('a' == mboxRec[0])
			{

			}
			
//		p_uint16CRCData = crc16(g_chr03FuncInstru, 6);
//		g_chr03FuncInstru[6] = p_uint16CRCData >> 8;
//		g_chr03FuncInstru[7] = p_uint16CRCData & 0xFF;
//		UART3_SendBuffer(g_chr03FuncInstru, 8);
		/* --------------------------- END 发送03功能码 END ------------------------- */
		
		OSTimeDlyHMSM(0, 0, 6,250);	
		/* 这里目前是按照一个寄存器一个字节来算的 */
////		if((0x01 == glb_msg3[0]) && (0x03 == glb_msg3[1]) /*&& (9 == RxCounter3)*/)
////		{
////			/* ----------------- 此处应该进行0x03功能返回码处理 ---------- */
////			ESP8266_Hex2Str(g_chrpESP8266CO, glb_msg3[3]);
////			ESP8266_Hex2Str(g_chrpESP8266CO+2, glb_msg3[4]);
//////			ESP8266_Hex2BinStr(g_chrpESP8266SensorStatus, glb_msg3[5]);
//////			ESP8266_Hex2Str(g_chrpESP8266VALVE, glb_msg3[6]);
////			/* ----------------- END 此处应该进行0x03功能返回码处理 END ---------- */
////		}
////		memset(glb_msg3, 0, MSG3_LEN);											/* 将glb_msg1清零 */
////		RxCounter3 = 0;														/* 出具处理完之后，需要将数据清零 */		
			mboxSend = g_chrp8266ToAlarm;
			//sprintf(mboxSend,"bcd");
			OSMboxPost(Str_Box_toALARM,mboxSend);
		OSTimeDlyHMSM(0, 0, 6,250);	
		
    }
}

/* 任务3 */
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
	
	/* 20170919 手动添加写入SSID和密码的程序段 */
//	#ifdef DEBUG
//	DEBUG_SEND_STR("[DEBUG]:HERE 1\r\n");
//	#endif
//	OSTimeDlyHMSM(0, 0, 3,0);	
//	ESP8266_SSIDandPW_SaveToFlash(BANK1_WRITE_START_ADDR,"Q-me", "anze2009", "12345678901234530");
//	OSTimeDlyHMSM(0, 0, 3,0);	
//	#ifdef DEBUG
//	DEBUG_SEND_STR("[DEBUG]:HERE 2\r\n");
//	#endif
	/* END 20170919 手动添加写入SSID和密码的程序段 END */
	
	ESP8266_RstModule();						/* 若连续两次连接服务器失败，则重启模块（其实还可以尝试换连入通道号的方法） */
		while(1)
		{
//			goto Loop_GetSSIDandPW;
//				#ifdef DEBUG
//				OSTimeDlyHMSM(0, 0,5,0);														/* DEBUG 调试时使用 */
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
				/* ------------------------- 工作模式1设置 ------------------------------- */			
				ESP8266_SetWorkMode(ESP8266_MODEL_STA);
				g_chrLedMode = LED_MODE_STA;
				/* ------------------------- 工作模式1设置 END ------------------------------- */			
				
				/* ------------------------- 连接路由器 ------------------------------- */			
				memset(g_chrpSSID, 0, strlen(g_chrpSSID));												/* 将SSID清零 */
				memset(g_chrpRouterPW, 0, strlen(g_chrpRouterPW));								/* 将RouterPW清零 */
				memset(g_chrpESP8266SN, 0, strlen(g_chrpESP8266SN));								/* 将RouterPW清零 */
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
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
					p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
					if(ESP8266_RE_FAIL == p_intRe)
					{
						gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
						ESP8266_RstModule();						/* 若连续两次连接服务器失败，则重启模块（其实还可以尝试换连入通道号的方法） */
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:goto Loop_CheckModuleLink\r\n");
							#endif
						codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
						goto Loop_OutOfAll;
					}
					else
					{
						gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成 */
					}
				}
				else
				{
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成 */
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
//					memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
//					RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
					OSTimeDlyHMSM(0,0,3,0);		
					
					while(ESP8266_RE_FAIL == ESP8266_CheckModuleLink())
					{
						OSTimeDlyHMSM(0,0,10,0);											/* ??5s,????????? */
							#ifdef DEBUG
							DEBUG_SEND_STR("[DEBUG]:Module Link error\r\n");
							#endif
					}
					
					/* ------------------------- 发送数据前，前进行AT模块连接检测 END ------------------------------- */
					
					/* -------------------------发送数据前，进行路由器连接检测 --------------------------- */	
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
					/* -------------------------发送数据前，进行路由器连接检测 END --------------------------- */
					
					/* -------------------------发送数据前，进行服务器的连接检测 ------------------------------ */
//					p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
//					if(ESP8266_RE_FAIL == p_intRe)
//					{
//						gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
//						p_intRe = ESP8266_LinkToServer(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
//						if(ESP8266_RE_FAIL == p_intRe)
//						{
//							gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
//							ESP8266_RstModule();						/* 若连续两次连接服务器失败，则重启模块（其实还可以尝试换连入通道号的方法） */
//								#ifdef DEBUG
//								DEBUG_SEND_STR("[DEBUG]:goto Loop_CheckModuleLink\r\n");
//								#endif
//							codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
//							goto Loop_OutOfAll;
//						}
//						else
//						{
//							gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成 */
//						}
//					}
//					else
//					{
//						gchrEsp8266IsConnectServer = ESP8266_CONNECT_OK;	/* 标志服务器连接成 */
//					}
					
				p_intRe = ESP8266_CheckServerLink(REMOTE_SERVER_IP_ADDR, REMOTE_SERVER_PORT);
				if(ESP8266_RE_FAIL == p_intRe)
				{
					gchrEsp8266IsConnectServer = ESP8266_CONNECT_FAIL;/* 标志服务器连接失败 */
						#ifdef DEBUG
						DEBUG_SEND_STR("[DEBUG]:goto Loop_LinkToServer\r\n");
						#endif
					codeSement = CODE_SEMENT_LINK_SERVER;
					goto Loop_OutOfAll;														/* 若服务器连接失败，只能重新连，调回到Loop_LinkToServer */
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
					/* -------------------------发送数据前，进行服务器的连接检测 END ------------------------------ */
					
					/* 在此处处理，将数据打包，发送给服务器 */
					Read_Temperature(1);
					OSTimeDlyHMSM(0,0,2,0);														/* 延时1s，是数据接收完 */
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
				/* ------------------------- 进行工作模式设置 --------------------------------- */
				ESP8266_SetWorkMode(ESP8266_MODEL_STA_AP);
				/* ------------------------- 进行工作模式设置 END --------------------------------- */
				p_intRe = ESP8266_SetAP(ESP8266_AP_SSID, ESP8266_AP_PW, ESP8266_AP_SERVER_IP, ESP8266_AP_SERVER_PORT);
				if(ESP8266_RE_FAIL == p_intRe)
				{
					codeSement = CODE_SEMENT_AP;
					goto Loop_OutOfAll;
				}
				else if(ESP8266_RE_OK == p_intRe)
				{
					g_uchrAPTimeCounter = 1;
					/* AP设置成功后，进行接收APP数据的任务 */
					while(1)
					{
Loop_GetSSIDandPW_WHILE:
						if(500 < g_uchrAPTimeCounter)			/* 如果AP时间超时 */
						{
							g_uchrAPTimeCounter = 0;
							codeSement = CODE_SEMENT_CHECK_MODULE_LINK;
							goto Loop_OutOfAll;
						}
						if(0 < strlen((const char*)glb_msg1))
						{
							OSTimeDlyHMSM(0,0,2,0);														/* 延时1s，是数据接收完 */
								#ifdef DEBUG
								sprintf(tempBufferForDebug,"[DEBUG]:In AP MODE, Rec \"%s\"\r\n",glb_msg1);
								DEBUG_SEND_STR(tempBufferForDebug);
								#endif
							pConnect = strstr((const char*)glb_msg1, "CONNECT");				/* 检测接收到的数据中是否有"CONNECT" */
							if(NULL != pConnect)																/* 如果有CONNECT */
							{
								g_chrESP8266_ConnectNum = glb_msg1[0];									/* 在ESP8266_ConnectNum中保存连入AP的设备通道号'0','1','2','3','4' */
									#ifdef DEBUG
									sprintf(tempBufferForDebug,"[DEBUG]:The CONNECT Channel is \"%c\"\r\n",g_chrESP8266_ConnectNum);
									DEBUG_SEND_STR(tempBufferForDebug);
									#endif
								memset(glb_msg1, 0, MSG1_LEN);									/* 将glb_msg1清零 */
								RxCounter1 = 0;																		/* 将glb_msg1的数据计数清零 */
								goto Loop_GetSSIDandPW_WHILE;											/* 跳转到Loop_GetSSIDandPW_WHILE，继续循环 */
							}
							/* 检测是否为连入设备断开事件 */
							pClosed = strstr((const char*)glb_msg1, "CLOSED");	/* 检测接收到的数据中是否有"CLOSED" */
							if(NULL != pClosed)																	/* 如果有CLOSED */
							{
								g_chrESP8266_ConnectNum = 0;
									#ifdef DEBUG
									DEBUG_SEND_STR("[DEBUG]:Clear the ESP8266_ConnectNum\r\n");
									#endif
								memset(glb_msg1, 0, MSG1_LEN);									/* 将glb_msg1清零 */
								RxCounter1 = 0;																		/* 将glb_msg1的数据计数清零 */
								goto Loop_GetSSIDandPW_WHILE;											/* 跳转到Loop_GetSSIDandPW_WHILE，继续循环 */
							}
							/* 检测是否为接收APP发来数据事件 */		/* char APDataStr[5][8] = {"SSID=\"", "\"PW=\"", "\"SN=\"", "\"DT=\"", "\"\r\n"}; */
							pAPData[0] = strstr((const char*)glb_msg1, APDataStr[0]);	/* 检测接收到的数据中是否有"SSID=\"" */
							pAPData[1] = strstr((const char*)glb_msg1, APDataStr[1]);	/* 检测接收到的数据中是否有"\"PW=\"" */
							pAPData[2] = strstr((const char*)glb_msg1, APDataStr[2]);	/* 检测接收到的数据中是否有"\"SN=\"" */
							pAPData[3] = strstr((const char*)glb_msg1, APDataStr[3]);	/* 检测接收到的数据中是否有"\"DT=\"" */
							pAPData[4] = strstr((const char*)glb_msg1, APDataStr[4]);	/* 检测接收到的数据中是否有"\"\r\n" */
							if((NULL != pAPData[0]) && (NULL != pAPData[1])  && (NULL != pAPData[2]) &&
							(NULL != pAPData[3]) && (NULL != pAPData[4]))	/* 如果上述5个特征字符串都有 */
							{
								g_uchrAPTimeCounter = 1;
								for(i=0; i<4; i++)
								{
									ESP8266_Send("JOIN_OK\r\n", strlen("JOIN_OK\r\n"),g_chrESP8266_ConnectNum);
									OSTimeDlyHMSM(0,0,10,0);														/* 延时30s，等待busy结束 */
										#ifdef DEBUG
										sprintf(tempBufferForDebug,"[DEBUG]:Send \"JOIN_OK\", then rec \"%s\"\r\n",glb_msg1);
										DEBUG_SEND_STR(tempBufferForDebug);
										#endif
//									pOk = strstr((const char*)glb_msg1, "OK");					/* ??USART1???????glb_msg1?????"busy" */
//									//memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
//									if(NULL == pOk)													/* ???"OK" */
//									{
//										goto Loop_SendOK;
//									}
								}
								/* 获取SSID */
								ESP8266_StrCpy(g_chrpSSID, pAPData[0], strlen(APDataStr[0]), pAPData[1]-pAPData[0]-strlen(APDataStr[0]));
								/* 获取路由器密码 */							
								ESP8266_StrCpy(g_chrpRouterPW, pAPData[1], strlen(APDataStr[1]), pAPData[2]-pAPData[1]-strlen(APDataStr[1]));
								/* 获取SN */
								ESP8266_StrCpy(g_chrpESP8266SN, pAPData[2], strlen(APDataStr[2]),pAPData[3]-pAPData[2]-strlen(APDataStr[2]));				/* SN长度为17 */
								/* 获取时间 */
								ESP8266_StrCpy(g_chrpESP8266SetTime, pAPData[3], strlen(APDataStr[3]), pAPData[4]-pAPData[3]-strlen(APDataStr[3]));			/* 时间长度为12 */
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
								memset(glb_msg1, 0, MSG1_LEN);									/* 将glb_msg1清零 */
								RxCounter1 = 0;																		/* 将glb_msg1的数据计数清零 */
								ESP8266_Send10Instru();							
								/* 用新的SSID和密码连接路由器 */
								/* ------------------------- 连接路由器 ------------------------------- */			
								p_intRe = ESP8266_LinkToRouter(g_chrpSSID, g_chrpRouterPW);
								OSTimeDlyHMSM(0,0,5,0);														/* 延时30s，等待busy结束 */
								ESP8266_SSIDandPW_SaveToFlash(BANK1_WRITE_START_ADDR,g_chrpSSID, g_chrpRouterPW, g_chrpESP8266SN);
									#ifdef DEBUG
									DEBUG_SEND_STR("[DEBUG]:Save SSID and RouterPW Finish\r\n");
									#endif
								OSTimeDlyHMSM(0,0,5,0);														/* 延时30s，等待busy结束 */
								memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
								RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
								codeSement = CODE_SEMENT_LINK_ROUTER;
								goto Loop_OutOfAll;
								
							}
							else
							{
								memset(glb_msg1, 0, MSG1_LEN);											/* 将glb_msg1清零 */
								RxCounter1 = 0;																				/* 将glb_msg1的数据计数清零 */
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
