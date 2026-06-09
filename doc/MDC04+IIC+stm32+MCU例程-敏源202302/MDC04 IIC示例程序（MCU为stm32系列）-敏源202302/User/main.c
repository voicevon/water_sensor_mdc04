#include <stdio.h>
#include <stdlib.h>

#include "stm32f10x.h"

#include "bsp_SysTick.h"
#include "bsp_usart1.h"


#include "MY_IIC_DRV.h"
#include "MDC04.h"


int main(void)
{ 
	MY_FLT32 fcap1,fcap2,fcap3,fcap4;
	float Co=15.0f, Cr=15.5f;
	USART1_Config();
	GPIOI2C_Bus_Init();
	SysTick_Init();
	
	/****************开始电容测量前，需配置重复性、中心值及测量范围，并选择电容测量通道**************************/
	
	MDC04_SysCfg(MDC04_REPEATABILITY_HIGH);     		//重复性配置
	MDC04_Set_Cap_Offset(Co);												//中心值选择
	MDC04_Set_Cap_FullScale(Cr);										//测量范围，实际量程为[Co-Cr,Co+Cr]
	MDC04_Set_Cap_Channel(CAP_CH1CH2CH3CH4_SEL);		//选择电容测量通道
	
	while(1)
	{
	/*************************测温，函数内部已添加电容转换时间****************************/
		MDC04_StartTempConvert(); 
	/***********************************测4通道电容**************************************/
		MDC04_ConvertCap();
		MY_DELAY_MS(44);		//高重复性下每个通道需要的转换时间为10.5ms，4通道全部转换时间需*4
		MDC04_ReadCap(&fcap1,&fcap2,&fcap3,&fcap4);
		printf(" C1= %.3f C2= %.3f C3= %.3f C4= %.3f",fcap1,fcap2,fcap3,fcap4 );
		MY_DELAY_MS(1000);
	}
	
}
