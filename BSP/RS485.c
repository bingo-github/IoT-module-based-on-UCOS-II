#include "stm32f10x.h"
#include "RS485.h"
#include "USART3.h"
#include "crc16.h"

char Buffer_RS485Rec[2*RS485_REC_BUFFER_LENGTH_MAX] = {0x01,0x03,0x02,0x00,0x00,0x00,0x00};
uint16_t Len_RS485RecBuffer = 0;
uint8_t RS485_AlreadyRecData_flag = 0;
uint8_t Rs485_RecBuffer_flag = RS485_REC_BUFFER_FINISH_NO;
vu8 RS485_TraBufferTime_counter = 0;			//RS485�򱨾��������������ݵļ�ʱ����һ�ϵ��ÿ��һ������
char Buffer_RS485Tra[] = {0x01,0x03,0x00,0x00,0x00,0x01,0x78,0x8E};			//�������ݰ�

uint8_t Bufffer_RS485Rec_Temp[2*RS485_REC_BUFFER_LENGTH_MAX];							//RS485�յ������ݵ���ʱ������
uint8_t Bufffer_RS485Rec_PreTemp[2*RS485_REC_BUFFER_LENGTH_MAX];						//��һ��RS485�յ������ݵ���ʱ������

/*
 *���������
 *���ز�����
 *����˵����RS485������ƹܽ�����
 */
void RS485_PinDirConfiguration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;                   //rs-485DIR������ƽ�
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);  
}

/*
 *���������
 *���ز�����
 *����˵��������RS485�ķ����ƽΪ�ߵ�ƽ���ߵ�ƽ����
 */
void RS485_SetDirPinTra(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_2);                     //485������ƣ��ߵ�ƽ����
}

/*
 *���������
 *���������
 *����˵��������RS485�ķ����ƽΪ�͵�ƽ���͵�ƽ����
 */
void RS485_SetDirPinRec(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_2);                    //485������ƣ��͵�ƽ����
}

/*
 *���������Time
 *���ز�����
 *����˵������ʱ����
 */
void RS485_Delay(uint32_t Time)
{
    uint32_t i;
    
    i = 0;
    while (Time--) 
		{
       for (i = 0; i < 5000; i++);
    }
}

/*
 *���������Buffer����Ҫ���͵����ݵĵ�ַ
 *					length����Ҫ���͵����ݵĳ���
 *���ز�����
 *����˵����ͨ��RS485����Buffer��ַ��length���ֽ�����
 */
void RS485_TraBuffer(char* Buffer, uint16_t length)
{	
	uint16_t i = 0;
	RS485_SetDirPinTra();
	RS485_Delay(5);
	for(i=0; i<length; i++)
	{
		UART3SendByte(Buffer[i]);
	}
	RS485_Delay(5);
	RS485_SetDirPinRec();
}

/*
 *���������
 *���ز�����
 *����˵����ͨ��RS485�򱨾����������������
 */
void RS485_TraReqBuffer(void)
{
	uint8_t buffer[10];
	int16_t crcBuffer = 0;
	buffer[0] = 0x01;
	buffer[1] = 0x03;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	buffer[4] = 0x00;
	buffer[5] = 0x01;
	crcBuffer = crc16(buffer,6);
	buffer[6] = (crcBuffer >> 8);
	buffer[7] = (crcBuffer & 0xFF);
	RS485_TraBuffer(buffer, 8);
}

/*
 *���������buffer1�������ڱȽϵ�����1
 *					buffer2�������ڱȽϵ�����2
 *					CmpLen����������ǰCmpLen�ֽڽ��бȽ�
 *���ز�����0��������������ͬ
 *					1�����������鲻ͬ
 *����˵�����Ƚ�2�������ǰCmpLen���ֽ�
 */
uint8_t RS485_BufferCmp(uint8_t* buffer1, uint8_t* buffer2, uint8_t CmpLen)
{
	uint8_t i=0;
	for(i=0; i<CmpLen; i++)
	{
		if(buffer1[i] != buffer2[i])
		{
			return 1;
		}
	}
	return 0;
}
