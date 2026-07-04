
  
#include "bsp_SysTick.h"

static __IO u32 TimingDelay;
 
/**
  * @brief  launch SysTick
  * @param  N/A
  * @retval N/A
  */
void SysTick_Init(void)
{
	/* SystemFrequency / 1000    1ms interrupt
	 * SystemFrequency / 100000	 10us interrupt
	 * SystemFrequency / 1000000 1us interrupt
	 */
//	if (SysTick_Config(SystemFrequency / 100000))	// ST3.0.0 lib
	if (SysTick_Config(SystemCoreClock / 1000000))	// ST3.5.0 lib
	{ 
		/* Capture error */ 
		while (1);
	}
		// shut down
	SysTick->CTRL &= ~ SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief   us delay,1us unit
  * @param  
  *		@arg 
  * @retval  N/A
  */
void Delay_us(__IO u32 nTime)
{ 
	TimingDelay = nTime;	

	// enable sysTick 
	SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;

	while(TimingDelay != 0); 
}

/**
  * @brief  get tick
  * @param  N/A
  * @retval N/A
  * @attention  SysTick interrupt function SysTick_Handler() call
  */
void TimingDelay_Decrement(void)
{
	if (TimingDelay != 0x00)
	{ 
		TimingDelay--;
	}
}
/*********************************************END OF FILE**********************/
