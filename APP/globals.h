
#ifdef GLOBALS 
#define EXT
#else
#define EXT extern 
#endif

#define DEBUG							/* ����ʱ��Ҫ����ñ���������������Ҫ */
#ifdef DEBUG
#define DEBUG_SEND_STR		USART3_SendString
#endif

#define REMOTE_SERVER_IP_ADDR		"www.meternet.cn"		/* Զ�˷�����ip��ַ */
#define REMOTE_SERVER_PORT			"1617"				/* Զ�˷������Ķ˿ں� */

//#define REMOTE_SERVER_IP_ADDR		"125.211.78.131"		/* Զ�˷�����ip��ַ */
//#define REMOTE_SERVER_PORT			"1617"				/* Զ�˷������Ķ˿ں� */

//#define REMOTE_SERVER_IP_ADDR		"192.168.1.102"		/* Զ�˷�����ip��ַ */
//#define REMOTE_SERVER_PORT			"6666"				/* Զ�˷������Ķ˿ں� */

#define ESP8266_AP_SSID				"JYKJ-BJQ"			/* 8266ģ�鴦��APģʽʱ�ų��ȵ��SSID */
#define ESP8266_AP_PW				"12345678"			/* 8266ģ�鴦��APģʽʱ�ų��ȵ��WiFi���� */
#define ESP8266_AP_SERVER_IP		"192.168.5.254"		/* 8266ģ�鴦��APģʽʱ��Ϊ��������IP��ַ */
#define ESP8266_AP_SERVER_PORT		"8888"				/* 8266ģ�鴦��APģʽʱ��Ϊ�������Ķ˿ں� */

#define	READ_REGISTER_START_ADDR	0x03E8				/* �������б���ȡ���ݵļĴ����׵�ַ */
#define READ_REGISTER_NUM			2					/* �������б���ȡ�����ݵĸ���N��ʵ����Ҫ��ȡN*2���ֽ� */

#define WRITE_REGISTER_START_ADDR	2001				/* д�뱨���������ݼĴ�����ַ */
#define WRITE_REGISTER_NUM			50					/* д�Ĵ�����������2001��2060��������2060�� */
