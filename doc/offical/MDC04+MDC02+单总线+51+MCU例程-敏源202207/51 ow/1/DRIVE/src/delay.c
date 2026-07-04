#ifndef _Delay_C_
#define _Delay_C_
#include <intrins.h>

#include "delay.h"
void Delay_50us(unsigned int n)	   
{
	unsigned char i;

	while(n--)
	{
		for(i=0;i<90;i++);
	}
}

void Delay_us(unsigned int n)	   
{
	unsigned char i;

	while(n--)
	{
		_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_();  _nop_(); _nop_(); _nop_(); _nop_();
	}
}


void Delay_ms(unsigned int n)
{
	while(n--)
	{
//		Delay_50us(20);
		Delay_us(1000);
	}
}
/*********************************************************************************************************************/
#endif