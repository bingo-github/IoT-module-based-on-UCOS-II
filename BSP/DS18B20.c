#include "includes.h"
#include "stm32f10x_gpio.h"


#define	 DQ0 GPIO_ResetBits(DS18B20_PORT , DS18B20_PIN)	  //DS18B20���ݴ���
#define	 DQ1 GPIO_SetBits(DS18B20_PORT , DS18B20_PIN)

static unsigned char  presence;

static unsigned char temp_data[2] = {0x00,0x00};  //���¶ȸߵ�λ

static unsigned char display[5] =   {0x00,0x00,0x00,0x00,0x00};//���¶�ת��ֵ

static unsigned char ditab[16] =    {0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x04,	 //�¶�ֵ��׼
                      0x05,0x06,0x06,0x07,0x08,0x08,0x09,0x09};

unsigned char theTemperature[4] = "xxxx";	 //�¶�ֵ����

#define DS18B20_CLK     RCC_APB2Periph_GPIOA
#define DS18B20_PIN     GPIO_Pin_12                 
#define DS18B20_PORT		GPIOA 

/* ����ȷ�������Խ��� */
static void delayus(int time)
{    
   u16 i=0;  
   while(time--)
   {
      i=6;  
      while(i--) ;    
   }
}

/*
 * ��������DS18B20_GPIO_Config
 * ����  ������DS18B20�õ���I/O��
 * ����  ����
 * ���  ����
 */
static void DS18B20_GPIO_Config(void)
{		
	/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	GPIO_InitTypeDef GPIO_InitStructure;

	/*����DS18B20_PORT������ʱ��*/
	RCC_APB2PeriphClockCmd(DS18B20_CLK, ENABLE); 

	/*ѡ��Ҫ���Ƶ�DS18B20_PORT����*/															   
  	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;	

	/*��������ģʽΪͨ���������*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*������������Ϊ50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*���ÿ⺯������ʼ��DS18B20_PORT*/
  	GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
	
		GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);	 
}

static void DS18B20_GPIO_In(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	  
	RCC_APB2PeriphClockCmd( DS18B20_CLK , ENABLE); 

	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;  
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
  	GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);				 
}

static void DS18B20_GPIO_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	  
	RCC_APB2PeriphClockCmd( DS18B20_CLK , ENABLE); 

	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN; 
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  	GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);				 
}

/*************** ��ʼ��DS18B20  ******************************************/

static int Init_DS18B20(void)
{
	unsigned char temp;
	
	DS18B20_GPIO_Config();
	
    DS18B20_GPIO_Out();     // DQΪ��� 
	DQ1;                   //DQ��λ
    delayus(50);            //������ʱ70us    

    DQ0;      //��DQ����  
    delayus(480);           //��ȷ��ʱ780us ���� 480us
    DQ1;       //��������
    
    DS18B20_GPIO_In();       //��DQΪ����
	temp=GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN); //��DQֵ
    delayus(50);            //������ʱ
   
    if(temp==0)      //���=0���ʼ���ɹ� =1���ʼ��ʧ��
     presence = 1;
    else  
	 presence = 0;
          
    delayus(430);           //  ��ʱ470us
    DS18B20_GPIO_Out();      //��DQΪ��� 
    DQ1;      //�ͷ����� 
     
    return(presence);    //�����źţ�0=presence,1= no presence
}


/****************дһ���ֽ� ******************************************/

static void WriteOneChar(unsigned char dat)
{
  	unsigned char i = 0;

  	DS18B20_GPIO_Out();      //��PD0Ϊ���
	for (i = 8; i > 0; i--)	 //дһ�ֽ�
  	{
		DQ0;
    	if(dat&0x01)
     		DQ1;      //д"1" 
		else     
	 		DQ0;     //д"0"

    	delayus(60);
    	DQ1;        
    	dat>>=1;
  }
}


/*****************��һ���ֽ� ****************************************/

static int ReadOneChar(void)
{
	unsigned char i = 0;
    unsigned char dat = 0;

    for (i = 8; i > 0; i--)
    {
      	DQ0;      //����Ϊ�͵�ƽ
      	dat >>= 1;
    
      	DQ1;       //����Ϊ�ߵ�ƽ(�ͷ�����)   
      	DS18B20_GPIO_In();       //��DQΪ����
        
      	if(GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN))	//��DQ
      		dat |= 0x80;

      	delayus(70);
		DS18B20_GPIO_Out();	   //��DQΪ����
      	DQ1;  
    }
    return (dat);
}
 
 
/*************��ȡ�¶�  ********************************************/

void Read_Temperature(unsigned char t)
{
	unsigned i;
	uint16_t temp_data16 = 0;
	for(i=0; i<t; i++)
	{
		Init_DS18B20();
	   if(presence==0)  
	   {
		 WriteOneChar(0xCC);  // ����������кŵĲ���
		 WriteOneChar(0x44);  // �����¶�ת��

		 Init_DS18B20();
		 WriteOneChar(0xCC);  //����������кŵĲ���
		 WriteOneChar(0xBE);  //��ȡ�¶ȼĴ���
		 
		 temp_data[0] = ReadOneChar();   //�¶ȵ�8λ
		 temp_data[1] = ReadOneChar();   //�¶ȸ�8λ
		   
		  temp_data16 += (temp_data[1]<<8)+temp_data[0];
		}
		OSTimeDlyHMSM(0, 0, 0,200);				/* �������ԣ��˴�����ʱҲ�Ǳ���� */
	}
	temp_data16 = temp_data16/t;
	temp_data[0] = temp_data16 & 0xFF;
	temp_data[1] = temp_data16 >> 8;
}

/********************����ת�����¶���ʾ  ****************************/

void Deal_Temperature(void)
 {
	char flag = 0;
	 uint16_t temp_data16;
	 temp_data16 = (temp_data[1]<<8)+temp_data[0];
	 
	 if(temp_data16 > 6348)						/* �¶�����ֵ�ж� */
	 {
		 temp_data16 = 65536-temp_data16;
		 temp_data[0] = temp_data16 & 0xFF;
		temp_data[1] = temp_data16 >> 8;
		 flag = 1;
	 }
	 
	 
    display[4]=temp_data[0]&0x0f;
    display[0]=ditab[display[4]];     //����С��λ��ֵ
  
    display[4]=(((temp_data[0]&0xf0)>>4)|((temp_data[1]&0x0f)<<4))-1;	    //ȡ���¶�ֵ��ʵ��ֵ��׼
    display[3]=display[4]/100;
    display[1]=display[4]%100;
    display[2]=display[1]/10;
    display[1]=display[1]%10;

    if(display[3]==0x30)        //��λΪ0������ʾ
     { 
       display[3]=0x20;              
       if(display[2]==0x30)      //�θ�λΪ0������ʾ
       display[2]=0x20;
     }

	theTemperature[0]=display[3]+0x30;	 //��λ����ʾ 
	theTemperature[1]=display[2]+0x30;	  //ʮλ����ʾ 
	theTemperature[2]=display[1]+0x30;	 //��λ����ʾ 
	theTemperature[3]=display[0]+0x30;	  //С��λ����ʾ  
	 
	 if(1 == flag)						/* ����¶�Ϊ�� */
	 {
		 theTemperature[0] = 1+0x30;
	 }
	 else								/* ����¶�Ϊ�� */
	 {
		 theTemperature[0] = 0x30;		
	 }

//	LCD_PutString(0,200,temp,Black,Magenta); //��ʾ�¶�
      
}


	 


