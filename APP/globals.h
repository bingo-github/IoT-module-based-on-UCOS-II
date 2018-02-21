
#ifdef GLOBALS 
#define EXT
#else
#define EXT extern 
#endif

#define DEBUG							/* 调试时需要定义该变量，不调试则不需要 */
#ifdef DEBUG
#define DEBUG_SEND_STR		USART3_SendString
#endif

#define REMOTE_SERVER_IP_ADDR		"www.meternet.cn"		/* 远端服务器ip地址 */
#define REMOTE_SERVER_PORT			"1617"				/* 远端服务器的端口号 */

//#define REMOTE_SERVER_IP_ADDR		"125.211.78.131"		/* 远端服务器ip地址 */
//#define REMOTE_SERVER_PORT			"1617"				/* 远端服务器的端口号 */

//#define REMOTE_SERVER_IP_ADDR		"192.168.1.102"		/* 远端服务器ip地址 */
//#define REMOTE_SERVER_PORT			"6666"				/* 远端服务器的端口号 */

#define ESP8266_AP_SSID				"JYKJ-BJQ"			/* 8266模块处于AP模式时放出热点的SSID */
#define ESP8266_AP_PW				"12345678"			/* 8266模块处于AP模式时放出热点的WiFi密码 */
#define ESP8266_AP_SERVER_IP		"192.168.5.254"		/* 8266模块处于AP模式时作为服务器的IP地址 */
#define ESP8266_AP_SERVER_PORT		"8888"				/* 8266模块处于AP模式时作为服务器的端口号 */

#define	READ_REGISTER_START_ADDR	0x03E8				/* 报警器中被读取数据的寄存器首地址 */
#define READ_REGISTER_NUM			2					/* 报警器中被读取的数据的个数N，实际需要读取N*2个字节 */

#define WRITE_REGISTER_START_ADDR	2001				/* 写入报警器的数据寄存器地址 */
#define WRITE_REGISTER_NUM			50					/* 写寄存器个数，从2001到2060（不包括2060） */
