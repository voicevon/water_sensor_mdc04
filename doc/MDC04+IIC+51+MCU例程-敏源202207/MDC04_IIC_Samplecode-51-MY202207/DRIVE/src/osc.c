#ifndef _OSC_C_
#define _OSC_C_


#include "osc.h"
#include "delay.h"


static unsigned char cap_ready = 0;


////float CapFromFreq(float frequency)
////{
////	float omg, Cy, Csnsr;
////	omg = 2.0*pi*frequency ;
////	if(frequency > 0.0) 
////	{
////		Cy = (1.0E21/omg/omg/L2) - V7_Cn;  //pF
////		Csnsr=Cy*C12/(C12-Cy)-19;
////		return Csnsr;
////	}

////	else return -1.0E12; //Negative infinite.
////}

void OSC_Freq_GPIO_Init(void)
{
	P05F = (P05F&0x18) | (0x02 << 5) | 0x01;	//P0.5 上拉、复用为T0
	P05C = P05C & (~0x20);		//弱上拉
}

void OSC_POWER_GPIO_Init(void)
{
	//振荡电路供电IO初始化	未实现
	
	OSC_POWER_OFF;		//关闭振荡电路电源	未实现
}

void Freq_RefTimer_OPM_Init(void)
{
	TR1 = 0;	//定时器1失能
	ET1 = 1;	//定时器1中断使能
	
	TMOD = (TMOD&0x0F) | (1 << 4);	//模式选择: 定时器1，模式1。
	
	TH1 = 0;	//Timer1 高8位
	TL1 = 0;	//Timer1 低8位
}

void Freq_CountTimer_OPM_Init(void)
{
	TR0 = 0;	//定时器0失能
	ET0 = 0;	//定时器0中断失能
	
	TMOD = (TMOD&0xF0) | (1 << 2) | (1 << 0);	//模式选择: 计数器0，模式1。
	
	TH0 = 0;	//Timer0 高8位
	TL0 = 0;	//Timer0 低8位
}

void VNTDC_Start_OPM(void)
{
	unsigned int ref_tim_cnt;
	
	ref_tim_cnt = 65536 - (FOSC / (12000/CAP_MEASURE_TIME));
	
	//配置 计数TIM Counter 初始值
	TH0 = 0;
	TL0 = 0;
	
	//配置 定时TIM Counter 初始值
	TH1 = (unsigned char)((ref_tim_cnt & 0xff00) >> 8);		//Timer1 高8位
	TL1 = (unsigned char)(ref_tim_cnt & 0x00ff);	//Timer1 低8位
	
	cap_ready = 0;
	
	//定时器使能
	TR0 = 1;
	TR1 = 1;
}

unsigned int OSC_Measure(void)
{
	unsigned long Tick_TimeOut = 100000;
	unsigned int FreqCounts;
	
	/*****************************开始电容测量*****************************/
	
	OSC_POWER_ON;	//开启振荡电路电源	未实现
	
	//测温	未实现
	
	Delay_ms(OSC_POWER_STEADY_TIME - TEMP_MEASURE_TIME);
	
	VNTDC_Start_OPM();
	
	while(!cap_ready)//等待TM1溢出
	{
		if(!Tick_TimeOut--) 
		{
			break;
		}
	}
	
	FreqCounts = (TH0*256) | TL0;	//读取振荡计数
	
	
	OSC_POWER_OFF;	//关闭振荡电路电源	未实现
	/**********************************************************************/

	return FreqCounts;
}

void TIMER1_ISR (void) interrupt 3
{
	//关闭TIM
	TR0 = 0;	//TIM0失能
	TR1 = 0;	//TIM1失能
	
	cap_ready = 1;
}

#endif
