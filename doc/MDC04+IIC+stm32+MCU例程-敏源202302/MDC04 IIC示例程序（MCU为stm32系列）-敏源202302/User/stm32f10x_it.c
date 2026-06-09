/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI
  
  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "bsp_usart1.h"
int FUNC = -1;

int ii=0;
uint8_t buffer[3] = {0};

extern void TimingDelay_Decrement(void);

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
   if (CoreDebug->DHCSR & 1) {  //check C_DEBUGEN == 1 -> Debugger Connected  
      __breakpoint(0);  // halt program execution here         
  }
	
	while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	TimingDelay_Decrement();	
}

void USART1_IRQHandler(void)
{ 
	uint8_t i=0;
	uint8_t Clear = Clear;
  
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {	
    /* Read one byte from the receive data register */
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
		buffer[ii++] = USART1 -> DR;	
		//printf("i'm here");
		//printf("\r\nii: %d",ii);
  }
	
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
		Clear = USART1 -> SR;
		Clear = USART1 -> DR;
	}	

	if(ii == 3) {
			FUNC = Package_Received;
      ii = 0;
  }
	
	
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
  {	
    /* Read one byte from the receive data register */
    i = USART_ReceiveData(USART1); 
		
		if(i == Read_Temperature_t) {FUNC = 0x06;}
		else if(i == Read_Rom_Message_t) {FUNC = 0x01;}
		else if(i == Temperature_Get_7_t) {FUNC = 0x07;}
		else if(i == Temperature_Get_77_t) {FUNC = 0x08;}
		else if(i == Check_Temperature_t) {FUNC = 0x09;}
		else if(i == AB_Calibre_t) {FUNC = 0x0a;}
		else if(i == Check_Parameters_t) {FUNC = 0x05;}
		else if(i == Set_Pages) {FUNC = 0x02;}
		else if(i == Set_Pages_Test) {FUNC = 0x22;}
//		else if(i == Set_Page1) {FUNC = 0x04;}
		else if(i == Read_7051_t) {FUNC = 0x0b;}
		else if(i == Read_Scratchpad_t) {FUNC = 0x03;}
		else if(i == Read_Scratchpad_Ext_t) {FUNC = 0x0c;}
		else if(i == Copy_Page0_t) {FUNC = 0x0d;}
	  else if(i == Copy_Page1_t) {FUNC = 0x0e;}
		else if(i == Search_ROM_t) {FUNC = 0x0f;}
		else if(i == Write_ROM_ID_t) {FUNC = 0x10;}
		else if(i == Write_Scratchpad_t) {FUNC = 0x11;}
		else if(i == Check_3wire_2wire_RW) {FUNC = 0x13;}
		//else if(i == Unit_Test) {FUNC = 0x11;}
		else {FUNC = 0x00;}
		
  }
  
  
}
/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
