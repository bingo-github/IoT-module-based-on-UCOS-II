#ifndef __FLASH_H_
#define __FLASH_H_
#include "stm32f10x.h"

#define BANK1_WRITE_START_ADDR  		((uint32_t)0x0800E800)				//写入数据的起始地址
#define BANK1_WRITE_END_ADDR    		((uint32_t)0x0800EC00)				//写入数据的末尾地址

#define RM04_GET_FLASH_DATA_TYPE_WRITE_FLAG					1			//获取写入标志位
#define RM04_GET_FLASH_DATA_TYPE_REMOTE_IP_ADDR			2			//获取远端IP地址
#define RM04_GET_FLASH_DATA_TYPE_REMOTE_PORT				3			//获取远端端口号
#define RM04_GET_FLASH_DATA_TYPE_WIFI_SSID					4			//获取WiFi的SSID
#define RM04_GET_FLASH_DATA_TYPE_WIFI_PASSWORD			5			//获取WiFi密码

#define RM04_MAKE_INSTRU_TYPE_WIFI_CONF							1			//生成WiFi配置的AT指令
#define RM04_MAKE_INSTRU_TYPE_REMOTEIP							2			//生成配置远端IP的AT指令
#define RM04_MAKE_INSTRU_TYPE_REMOTEPORT						3			//生成配置远端端口号的AT指令

#define FLASH_PAGE_SIZE    ((uint16_t)0x400)//定义页的大小，对于大容量器件页的大小为0X400字节

void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint16_t Length);
void Flash_ReadDataFromFlash(uint32_t SrcAddress, uint32_t* Data, uint16_t Length);

#endif
