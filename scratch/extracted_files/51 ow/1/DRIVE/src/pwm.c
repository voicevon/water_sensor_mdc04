#ifndef _PWM_C_
#define _PWM_C_


#include "pwm.h"


#define PWMDIV_V				(24000000/12000000)		//当PWM时钟为其他时钟频率时，需相应修改参数
#define PWMDUT_V				(PWMDIV_V/2)			//占空比为50%


/*********************************************************************************************************************/
void PWM_CH0_P00(void)
{
	INDEX = PWM_CH0;												//设置INDEX值对应PWM0
	PWMCON = TIE(0) | ZIE(0) | PIE(0) | NIE(0) | MS(0) | CKS_XH ;		  //设置PWM时钟源为  

	PWMCFG = TOG(0) | 0;				
	PWMPS = PWM_P00_INDEX;				//设置P00为PWM0输出引脚
	P00F  = PWM_SETTING;	
	
	//设置PWMDIV、PWMDUT
	PWMDIVH	= (unsigned char)(PWMDIV_V>>8);			
	PWMDIVL	= (unsigned char)(PWMDIV_V);			
	PWMDUTH	=	(unsigned char)(PWMDUT_V>>8);		
	PWMDUTL	= (unsigned char)(PWMDUT_V);	

 	PWMUPD = 1 << PWM_CH0;		//PWMDIV、PWMDUT更新使能
	while(PWMUPD);		//等待更新完成
 	PWMEN = 1 << PWM_CH0;	//PWM0、PWM1使能
}

/*********************************************************************************************************************/
#endif
