#ifndef __FLASH_H_
#define __FLASH_H_
#include "stm32f10x.h"

#define BANK1_WRITE_START_ADDR  		((uint32_t)0x0800E800)				//д�����ݵ���ʼ��ַ
#define BANK1_WRITE_END_ADDR    		((uint32_t)0x0800EC00)				//д�����ݵ�ĩβ��ַ

#define RM04_GET_FLASH_DATA_TYPE_WRITE_FLAG					1			//��ȡд���־λ
#define RM04_GET_FLASH_DATA_TYPE_REMOTE_IP_ADDR			2			//��ȡԶ��IP��ַ
#define RM04_GET_FLASH_DATA_TYPE_REMOTE_PORT				3			//��ȡԶ�˶˿ں�
#define RM04_GET_FLASH_DATA_TYPE_WIFI_SSID					4			//��ȡWiFi��SSID
#define RM04_GET_FLASH_DATA_TYPE_WIFI_PASSWORD			5			//��ȡWiFi����

#define RM04_MAKE_INSTRU_TYPE_WIFI_CONF							1			//����WiFi���õ�ATָ��
#define RM04_MAKE_INSTRU_TYPE_REMOTEIP							2			//��������Զ��IP��ATָ��
#define RM04_MAKE_INSTRU_TYPE_REMOTEPORT						3			//��������Զ�˶˿ںŵ�ATָ��

#define FLASH_PAGE_SIZE    ((uint16_t)0x400)//����ҳ�Ĵ�С�����ڴ���������ҳ�Ĵ�СΪ0X400�ֽ�

void Flash_WriteDataToFlash(uint32_t DesAddress, uint32_t* Data, uint16_t Length);
void Flash_ReadDataFromFlash(uint32_t SrcAddress, uint32_t* Data, uint16_t Length);

#endif
