#ifndef _PWM_H_
#define _PWM_H_
/*********************************************************************************************************************/

#include "ca51f003_config.h"
#include "ca51f003sfr.h"
#include "ca51f003xsfr.h"
#include "gpiodef_f003.h"


//PWMCON
#define TIE(N)			(N<<7)
#define ZIE(N)			(N<<6)
#define PIE(N)			(N<<5)
#define NIE(N)			(N<<4)
#define MS(N)			(N<<3)		//中心对齐模式
#define MOD(N)			(N<<0)	    //互补模式

#define CKS_SYS			(0<<0)
#define CKS_IH			(1<<0)
#define CKS_IL			(2<<0)
#define CKS_XH			(3<<0)

//PWMPS

#define PWM_P00_INDEX   0 
#define PWM_P01_INDEX   1 
#define PWM_P02_INDEX   2 
#define PWM_P03_INDEX   3 
#define PWM_P04_INDEX   4 
#define PWM_P05_INDEX   5 
#define PWM_P06_INDEX   6 
#define PWM_P07_INDEX   7 
#define PWM_P10_INDEX   8
#define PWM_P11_INDEX   9
#define PWM_P12_INDEX   10
#define PWM_P13_INDEX   11
#define PWM_P14_INDEX   12
#define PWM_P15_INDEX   13
#define PWM_P16_INDEX   14
#define PWM_P17_INDEX   15
#define PWM_P20_INDEX   16
#define PWM_P30_INDEX   17


//PWMCFG
#define TOG(N)			(N<<7)


//PWMFBC
#define PWMFBIF				(1<<7)
#define PWMFBIE(N)			(N<<5)
#define PWMFBL(N)			(N<<1)
#define PWMFBE(N)			(N<<0)


//PWMFBS
#define PWMFBS5(N)			(N<<5)
#define PWMFBS4(N)			(N<<4)
#define PWMFBS3(N)			(N<<3)
#define PWMFBS2(N)			(N<<2)
#define PWMFBS1(N)			(N<<1)
#define PWMFBS0(N)			(N<<0)


//PWMBD
#define PWMFBD5(N)			(N<<5)
#define PWMFBD4(N)			(N<<4)
#define PWMFBD3(N)			(N<<3)
#define PWMFBD2(N)			(N<<2)
#define PWMFBD1(N)			(N<<1)
#define PWMFBD0(N)			(N<<0)



//PWMAIF   PWMBIF	PWMCIF 	PWMDIF
#define  TIF1	(1<<7)
#define  ZIF1	(1<<6)
#define  PIF1	(1<<5)
#define  NIF1	(1<<4)
#define  TIF0	(1<<3)
#define  ZIF0	(1<<2)
#define  PIF0	(1<<1)
#define  NIF0	(1<<0)	

#define PWM_CH0		0
#define PWM_CH1		1
#define PWM_CH2		2
#define PWM_CH3		3
#define PWM_CH4		4
#define PWM_CH5		5
#define PWM_CH6		6
#define PWM_CH7		7



void PWM_CH0_P00(void);

/*********************************************************************************************************************/
#endif
