
#ifndef _MAIN_C_
#define _MAIN_C_
/*********************************************************************************************************************/
#include "ca51f003_config.h"
#include "ca51f003sfr.h"
#include "ca51f003xsfr.h"
#include "gpiodef_f003.h"
#include "system_clock.h"

#include "uart.h"
#include "delay.h"
#include "MDC04.h"

#include "owmy.h"

#include <intrins.h>
#include <stdio.h>
/*********************************************************************************************************************/
extern float CapCfg_offset, CapCfg_range;
unsigned char a= 0xef;
//unsigned short b =0xef56;
void main(void)
{
	unsigned char cos =0x28,cfb =0xff;
	unsigned int FreqCounts=80;
#ifdef LVD_RST_ENABLE
	LVDCON = 0xE0;		//设置LVD复位电压为2V
#endif
	
	Sys_Clk_Set_XOSCH();	//设置系统时钟为XOSCH
	Uart2_Initial(UART2_BAUTRATE);
  
  OW_Init();
 

	EA = 1;		//开全局中断
	
 
	while(1)
	{
		    uart_printf("W2- ");
		writeCosCfb(&cos,&cfb);
	 ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
			Delay_ms(2);
				Convert_TC1();
	
//			  readparameter();
		Delay_ms(1000);
//		uart_printf("hex = %bx  u16=%x \n",a,b);
//		 OW_Reset();
//		ConvertTemp();

//		uart_printf("Cap cnt = %u\n",FreqCounts);
	}
}
#endif
