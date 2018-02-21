#ifndef __RS485_H_
#define __RS485_H_
#include "stm32f10x.h"

#define RS485_REC_BUFFER_FINISH_NO				0			//��ʾRS485�������ݰ�δ���
#define RS485_REC_BUFFER_FINISH_YES				1			//��ʾRS485�������ݰ����

#define RS485_REC_BUFFER_LENGTH_MAX				14		//��ʾRS485�����Э����һ�����ݰ�����Ϊ10

void RS485_TraBuffer(char* Buffer, uint16_t length);
void RS485_PinDirConfiguration(void);
void RS485_TraReqBuffer(void);
uint8_t RS485_BufferCmp(uint8_t* buffer1, uint8_t* buffer2, uint8_t CmpLen);

#endif
