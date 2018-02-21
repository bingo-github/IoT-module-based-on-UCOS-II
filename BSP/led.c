/******************** ��ʢ���ӹ����� ********************
 * �ļ���  ��led.c
 * ����    ��led Ӧ�ú�����
 *          
 * ʵ��ƽ̨��MINI STM32������ ����STM32F103C8T6
 * Ӳ�����ӣ�-----------------
 *          |   PB15 - LED1   |
 *          |   PB14 - LED2   |
 *          |                 |
 *           ----------------- 
 * ��汾  ��ST3.0.0  																										  
 * �Ա��꣺http://shop66177872.taobao.com
*********************************************************/
#include "led.h"


 /***************  ����LED�õ���I/O�� *******************/
void LED_GPIO_Config(void)	
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC, ENABLE); // ʹ��PB�˿�ʱ��  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  //��ʼ��PB�˿�
  GPIO_SetBits(GPIOC, GPIO_Pin_13);	 // �ر�����LED
}
