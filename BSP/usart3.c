/***************************************
 * 文件名  ：usart3.c
 * 描述    ：配置USART3         
 * 实验平台：MINI STM32开发板 基于STM32F103C8T6
 * 硬件连接：------------------------
 *          | PB10  - USART3(Tx)      |
 *          | PB11 - USART3(Rx)      |
 *           ------------------------
 * 库版本  ：ST3.0.0  

**********************************************************************************/

#include "usart3.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <stdarg.h>


void USART3_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* 使能 USART1 时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); 

	/* USART1 使用IO端口配置 */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
  GPIO_Init(GPIOB, &GPIO_InitStructure);   //初始化GPIOA
	  
	/* USART1 工作模式配置 */
	USART_InitStructure.USART_BaudRate = 9600;	//波特率设置：9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//数据位数设置：8位
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//停止位设置：1位
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //是否奇偶校验：无
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制模式设置：没有使能
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//接收与发送都使能
	USART_Init(USART3, &USART_InitStructure);  //初始化USART1
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART3, ENABLE);// USART1使能
}

 /*发送一个字节数据*/
 void UART3SendByte(unsigned char SendData)
{	   
        USART_SendData(USART3,SendData);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);	    
}  

/*接收一个字节数据*/
unsigned char UART3GetByte(unsigned char* GetData)
{   	   
        if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET)
        {  return 0;//没有收到数据 
		}
        *GetData = USART_ReceiveData(USART3); 
        return 1;//收到数据
}
/*接收一个数据，马上返回接收到的这个数据*/
void UART3Test(void)
{
       unsigned char i = 0;

       while(1)
       {    
		 while(UART3GetByte(&i))
        {
         USART_SendData(USART3,i);
        }      
       }     
}

/* UART3发送数组 */
void UART3_SendBuffer(uint8_t* SendData, int bufferLen)
{
	int i;
	for(i=0; i<bufferLen; i++)
	{
		UART3SendByte(SendData[i]);
	}
}

/* USART3中断优先级配置 */
void USART3_NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure; 
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*********************************************
*函数名称：void USART3_SendString(uint8_t *ch)
*
*入口参数：无
*
*出口参数：无
*
*功能说明：USART3 发送字符串
**********************************************/
void USART3_SendString(char *ch)
{
	while(*ch!=0)
	{		
		while(!USART_GetFlagStatus(USART3, USART_FLAG_TXE));
		USART_SendData(USART3, *ch);
		ch++;
	}   	
}

