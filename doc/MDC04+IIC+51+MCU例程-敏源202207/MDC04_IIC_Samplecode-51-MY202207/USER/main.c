
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
#include "MDC04_IIC.h"

#include "IIC.h"

#include <intrins.h>
#include <stdio.h>
/*********************************************************************************************************************/
extern float CapCfg_offset, CapCfg_range;
extern	float fcap1,fcap2,fcap3,fcap4;
unsigned char a= 0xef;
//unsigned short b =0xef56;
void main(void)
{
	unsigned char cos =0x28,cfb =0xff,Coscfg;

#ifdef LVD_RST_ENABLE
	LVDCON = 0xE0;		//设置LVD复位电压为2V
#endif
	
	Sys_Clk_Set_XOSCH();	//设置系统时钟为XOSCH
	Uart2_Initial(UART2_BAUTRATE);
  
  I2C_Init();
 WriteCosConfig(cos,cfb);

	EA = 1;		//开全局中断

	
	while(1)
	{
		  
     
//     ReadCosConfig(Coscfg);
		uart_printf("ok - ");
		MDC04_ConvertCap();
		Delay_ms(100);
		MDC04_ReadCap(&fcap1,&fcap2,&fcap3,&fcap4);
	uart_printf("cap1=%.3f,cap2=%.3f,",fcap1,fcap2);
	uart_printf("cap3=%.3f,cap4=%.3f\n",fcap3,fcap4);
		Delay_ms(1000);

	}
}
#endif
