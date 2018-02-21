#include "includes.h"
#include "stm32f10x_gpio.h"


#define	 DQ0 GPIO_ResetBits(DS18B20_PORT , DS18B20_PIN)	  //DS18B20数据串口
#define	 DQ1 GPIO_SetBits(DS18B20_PORT , DS18B20_PIN)

static unsigned char  presence;

static unsigned char temp_data[2] = {0x00,0x00};  //存温度高低位

static unsigned char display[5] =   {0x00,0x00,0x00,0x00,0x00};//存温度转换值

static unsigned char ditab[16] =    {0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x04,	 //温度值较准
                      0x05,0x06,0x06,0x07,0x08,0x08,0x09,0x09};

unsigned char theTemperature[4] = "xxxx";	 //温度值缓存

#define DS18B20_CLK     RCC_APB2Periph_GPIOA
#define DS18B20_PIN     GPIO_Pin_12                 
#define DS18B20_PORT		GPIOA 

/* 不精确，但可以接受 */
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
 * 函数名：DS18B20_GPIO_Config
 * 描述  ：配置DS18B20用到的I/O口
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_GPIO_Config(void)
{		
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;

	/*开启DS18B20_PORT的外设时钟*/
	RCC_APB2PeriphClockCmd(DS18B20_CLK, ENABLE); 

	/*选择要控制的DS18B20_PORT引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;	

	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*调用库函数，初始化DS18B20_PORT*/
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

/*************** 初始化DS18B20  ******************************************/

static int Init_DS18B20(void)
{
	unsigned char temp;
	
	DS18B20_GPIO_Config();
	
    DS18B20_GPIO_Out();     // DQ为输出 
	DQ1;                   //DQ复位
    delayus(50);            //稍做延时70us    

    DQ0;      //将DQ拉低  
    delayus(480);           //精确延时780us 大于 480us
    DQ1;       //拉高总线
    
    DS18B20_GPIO_In();       //置DQ为输入
	temp=GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN); //读DQ值
    delayus(50);            //稍做延时
   
    if(temp==0)      //如果=0则初始化成功 =1则初始化失败
     presence = 1;
    else  
	 presence = 0;
          
    delayus(430);           //  延时470us
    DS18B20_GPIO_Out();      //置DQ为输出 
    DQ1;      //释放总线 
     
    return(presence);    //返回信号，0=presence,1= no presence
}


/****************写一个字节 ******************************************/

static void WriteOneChar(unsigned char dat)
{
  	unsigned char i = 0;

  	DS18B20_GPIO_Out();      //置PD0为输出
	for (i = 8; i > 0; i--)	 //写一字节
  	{
		DQ0;
    	if(dat&0x01)
     		DQ1;      //写"1" 
		else     
	 		DQ0;     //写"0"

    	delayus(60);
    	DQ1;        
    	dat>>=1;
  }
}


/*****************读一个字节 ****************************************/

static int ReadOneChar(void)
{
	unsigned char i = 0;
    unsigned char dat = 0;

    for (i = 8; i > 0; i--)
    {
      	DQ0;      //总线为低电平
      	dat >>= 1;
    
      	DQ1;       //总线为高电平(释放总线)   
      	DS18B20_GPIO_In();       //置DQ为输入
        
      	if(GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN))	//读DQ
      		dat |= 0x80;

      	delayus(70);
		DS18B20_GPIO_Out();	   //置DQ为输入
      	DQ1;  
    }
    return (dat);
}
 
 
/*************读取温度  ********************************************/

void Read_Temperature(unsigned char t)
{
	unsigned i;
	uint16_t temp_data16 = 0;
	for(i=0; i<t; i++)
	{
		Init_DS18B20();
	   if(presence==0)  
	   {
		 WriteOneChar(0xCC);  // 跳过读序号列号的操作
		 WriteOneChar(0x44);  // 启动温度转换

		 Init_DS18B20();
		 WriteOneChar(0xCC);  //跳过读序号列号的操作
		 WriteOneChar(0xBE);  //读取温度寄存器
		 
		 temp_data[0] = ReadOneChar();   //温度低8位
		 temp_data[1] = ReadOneChar();   //温度高8位
		   
		  temp_data16 += (temp_data[1]<<8)+temp_data[0];
		}
		OSTimeDlyHMSM(0, 0, 0,200);				/* 经过测试，此处的延时也是必须的 */
	}
	temp_data16 = temp_data16/t;
	temp_data[0] = temp_data16 & 0xFF;
	temp_data[1] = temp_data16 >> 8;
}

/********************数据转换与温度显示  ****************************/

void Deal_Temperature(void)
 {
	char flag = 0;
	 uint16_t temp_data16;
	 temp_data16 = (temp_data[1]<<8)+temp_data[0];
	 
	 if(temp_data16 > 6348)						/* 温度正负值判断 */
	 {
		 temp_data16 = 65536-temp_data16;
		 temp_data[0] = temp_data16 & 0xFF;
		temp_data[1] = temp_data16 >> 8;
		 flag = 1;
	 }
	 
	 
    display[4]=temp_data[0]&0x0f;
    display[0]=ditab[display[4]];     //查表得小数位的值
  
    display[4]=(((temp_data[0]&0xf0)>>4)|((temp_data[1]&0x0f)<<4))-1;	    //取得温度值与实际值较准
    display[3]=display[4]/100;
    display[1]=display[4]%100;
    display[2]=display[1]/10;
    display[1]=display[1]%10;

    if(display[3]==0x30)        //高位为0，不显示
     { 
       display[3]=0x20;              
       if(display[2]==0x30)      //次高位为0，不显示
       display[2]=0x20;
     }

	theTemperature[0]=display[3]+0x30;	 //百位数显示 
	theTemperature[1]=display[2]+0x30;	  //十位数显示 
	theTemperature[2]=display[1]+0x30;	 //个位数显示 
	theTemperature[3]=display[0]+0x30;	  //小数位数显示  
	 
	 if(1 == flag)						/* 如果温度为负 */
	 {
		 theTemperature[0] = 1+0x30;
	 }
	 else								/* 如果温度为正 */
	 {
		 theTemperature[0] = 0x30;		
	 }

//	LCD_PutString(0,200,temp,Black,Magenta); //显示温度
      
}


	 


