#ifndef CA51F003_CONFIG_H
#define CA51F003_CONFIG_H
/**************************************************************************************************************/
#define IRCH				0
#define IRCL				1
#define XOSCH				2
#define XCLK_IN				3

/**************************************************************************************************************/
#define SYSCLK_SRC			XOSCH			//芯片系统时钟选择
/**************************************************************************************************************/


/************系统时钟频率定义**********************************************************/
#if (SYSCLK_SRC == IRCH)
	#define FOSC		(16000000)
#elif (SYSCLK_SRC == IRCL)
	#define FOSC		(131000)
#else
	#define FOSC		(24000000)
#endif
/***************************************************************************************************************/

#define LVD_RST_ENABLE

/*************************UART功能开关宏定义********************************************************************/
//#define UART1_EN			//如果使用UART1，打开此宏定义
#define UART2_EN			//如果使用UART2，打开此宏定义

#define PRINT_EN				//使用uart_printf函数打印使能

#ifdef PRINT_EN
// 	#define UART1_PRINT		//如果使用UART1打印，打开此宏定义
	#define UART2_PRINT		//如果使用UART2打印，打开此宏定义
			
	#ifdef UART1_PRINT
		#define UART1_EN
	#elif defined  UART2_PRINT
		#define UART2_EN
	#endif
#endif
#ifdef UART1_EN
	#define UART1_BAUTRATE		115200
#endif
#ifdef UART2_EN
	#define UART2_BAUTRATE		9600
#endif
/*********************************************************************************************************************/

#endif										
