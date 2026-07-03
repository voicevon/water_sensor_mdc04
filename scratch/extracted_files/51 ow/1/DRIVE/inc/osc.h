#ifndef _OSC_H_
#define _OSC_H_
/*********************************************************************************************************************/
#include "ca51f003_config.h"
#include "ca51f003sfr.h"
#include "ca51f003xsfr.h"



#define			OSC_POWER_STEADY_TIME		20		//振荡电路电源稳定时间 ms
#define			TEMP_MEASURE_TIME			0		//温度测量最小时间 ms
#define			CAP_MEASURE_TIME			5		//电容测量时间 ms



#define			OSC_POWER_ON
#define			OSC_POWER_OFF



//float CapFromFreq(float frequency)

void OSC_Freq_GPIO_Init(void);
void OSC_POWER_GPIO_Init(void);
void Freq_RefTimer_OPM_Init(void);
void Freq_CountTimer_OPM_Init(void);
void VNTDC_Start_OPM(void);
unsigned int OSC_Measure(void);

/*********************************************************************************************************************/
#endif