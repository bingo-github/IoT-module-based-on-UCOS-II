#ifndef __ESP8266_H
#define __ESP8266_H
#include "stm32f10x.h"

extern unsigned char guchrEsp8266Model;								// ESP8266工作模式标志位
#define	ESP8266_MODEL_STA				1											// STATIONģʽ
#define ESP8266_MODEL_AP				2											// APģʽ
#define	ESP8266_MODEL_STA_AP		3											// STA+APģʽ

extern char gchrEsp8266IsConnectModule;								/* 表示MCU与ESP8266模块之间的通讯连接情况 */
extern char gchrEsp8266IsConnectRouter;								/* 表示模块与路由器之间的通讯连接情况 */
extern char gchrEsp8266IsConnectServer;								/* 表示模块与远端服务器之间的通讯连接情况 */
#define ESP8266_CONNECT_NULL		0											/* 宏定义，ESP8266连接未定义 */
#define ESP8266_CONNECT_OK			1											/* 宏定义，ESP8266连接成功 */
#define ESP8266_CONNECT_FAIL		2											/* 宏定义，ESP8266连接失败 */

//#define ESP8266_LINK_OK		1					/* 表示（模块、路由器、服务器）连接正常 */
//#define ESP8266_LINK_FAIL	-1					/* 表示（模块、路由器、服务器）连接不正常 */

#define ESP8266_RE_OK		1					/* 表示（模块、路由器、服务器）连接正常 */
#define ESP8266_RE_FAIL	-1					/* 表示（模块、路由器、服务器）连接不正常 */

extern char *pOk;	/* 接收strstr函数的返回值，判断返回值中是否有“OK”，以此来判断AT指令发送是否成功 */
extern char *pBusy;
extern char *pError;		/* 接收strstr函数的返回值，判断返回值中是否有“ERROR”，以此来判断AT指令发送是否成功 */
extern char *pNo;	/* 接收strstr函数的返回值，判断返回值中是否有“No”，以此来判断AT指令发送是否成功 */
extern char *pSSID;
extern char *pPassword;
extern char *pConnect;
extern char *pClosed;
extern char *pAPData[5];
extern char APDataStr[5][8];		/* 接收到的数据中的特征字符串 */
extern char *pServerData[3];
extern char ServerDataStr[3][15];

int ESP8266_Send(char* p_uchrpData, unsigned char p_uchrLen,char p_chrChannel);
char ESP8266_CompareValueOfCO(char* p_chrpStr1, char* p_chrpStr2, char p_chrErrorValue);
void ESP8266_USARTSend(char *ch);
void ESP8266_SSIDandPW_SaveToFlash(uint32_t DesAddress, char* SSID, char* RouterPW, char * SN);
int ESP8266_SSIDandPW_ReadFromFlash(uint32_t srcAddress, char* desSSID, char* desRouterPW, char* desSN);
/* ----------------------------新整理的函数 2017-06-17 --------------------------------- */
int ESP8266_CheckModuleLink(void);
void ESP8266_SetWorkMode(unsigned char p_uchrWorkMode);
int ESP8266_LinkToRouter(char* p_chrpSSID, char* p_chrpRouterPW);
int ESP8266_LinkToServer(char* p_chrpRemoteServerIP, char* p_chrpRemoteServerPort);
int ESP8266_CheckRouterLink(void);
int ESP8266_CheckServerLink(char* p_chrpRemoteServerIP, char* p_chrpRemoteServerPort);
int ESP8266_SetAP(char* p_chrpAPSSID, char* p_chrpAPPW, char* p_chrpAPIP, char* p_chrpAPPort);
int ESP8266_RstModule(void);
void ESP8266_MakeInstru_ToServer(char* p_chrpDesInstru, 
								char* p_chrpSN, 
								char* p_chrpCO, 
								char* p_chrpTEMP, 
								char* p_chrpVALVE,
								char* p_chrpSensorStatus);
void ESP8266_Hex2Str(char* p_chrpDesStr, unsigned char p_chrSrcByte);
void ESP8266_Str2Hex(unsigned char* p_chrDesByte, char* p_chrpSrcStr);
void ESP8266_Hex2BinStr(char* p_chrpDesStr, unsigned char p_chrSrcByte);
void ESP8266_StrCpy(char* p_chrpDesStr, char* p_chrpSrcStr,unsigned char p_uchrOffsetLen, unsigned char p_uchrLen);
void ESP8266_CheckDataFromServer(unsigned char* p_chrpSrcStr, char* p_chrpDesStr1, char* p_chrpDesStr2, char* p_chrpDesStr3);
void ESP8266_Send10Instru(void);

#endif
