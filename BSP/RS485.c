#include "stm32f10x.h"
#include "RS485.h"
#include "USART3.h"
#include "crc16.h"

char Buffer_RS485Rec[2*RS485_REC_BUFFER_LENGTH_MAX] = {0x01,0x03,0x02,0x00,0x00,0x00,0x00};
uint16_t Len_RS485RecBuffer = 0;
uint8_t RS485_AlreadyRecData_flag = 0;
uint8_t Rs485_RecBuffer_flag = RS485_REC_BUFFER_FINISH_NO;
vu8 RS485_TraBufferTime_counter = 0;			//RS485向报警器发送请求数据的计时器，一上电就每秒一个请求
char Buffer_RS485Tra[] = {0x01,0x03,0x00,0x00,0x00,0x01,0x78,0x8E};			//请求数据包

uint8_t Bufffer_RS485Rec_Temp[2*RS485_REC_BUFFER_LENGTH_MAX];							//RS485收到的数据的临时缓冲区
uint8_t Bufffer_RS485Rec_PreTemp[2*RS485_REC_BUFFER_LENGTH_MAX];						//上一次RS485收到的数据的临时缓冲区

/*
 *输入参数：
 *返回参数：
 *函数说明：RS485方向控制管脚配置
 */
void RS485_PinDirConfiguration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;                   //rs-485DIR方向控制脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);  
}

/*
 *输入参数：
 *返回参数：
 *函数说明：设置RS485的方向电平为高电平，高电平发送
 */
void RS485_SetDirPinTra(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_2);                     //485方向控制，高电平发送
}

/*
 *输入参数：
 *输出参数：
 *函数说明：设置RS485的方向电平为低电平，低电平接收
 */
void RS485_SetDirPinRec(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_2);                    //485方向控制，低电平接收
}

/*
 *输入参数：Time
 *返回参数：
 *函数说明：延时函数
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
 *输入参数：Buffer——要发送的数据的地址
 *					length——要发送的数据的长度
 *返回参数：
 *函数说明：通过RS485发送Buffer地址中length个字节数据
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
 *输入参数：
 *返回参数：
 *函数说明：通过RS485向报警器发送数据请求包
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
 *输入参数：buffer1——用于比较的数组1
 *					buffer2——用于比较的数组2
 *					CmpLen——两个的前CmpLen字节进行比较
 *返回参数：0——两个数组相同
 *					1——两个数组不同
 *函数说明：比较2个数组的前CmpLen个字节
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
