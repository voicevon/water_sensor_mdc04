#ifndef __USART1_H
#define	__USART1_H

#include "stm32f10x.h"
#include <stdio.h>
#define  Address 0x11  

typedef enum   
{  
    Package_Received           =   0x00,
		Read_Rom_Message_t    =   0x01,
			Set_Pages      =   0x02,
	 Set_Pages_Test      =   0x22,
   Read_Scratchpad_t     =   0x03,   
	//	Set_Page1    =   0x04,
	Check_Parameters_t    =   0x05,

 Read_Temperature_t    =   0x06,
	
    Temperature_Get_7_t   =   0x07,
		Temperature_Get_77_t  =   0x08,
	  Check_Temperature_t   =   0x09,
		AB_Calibre_t          =   0x0a,

	
		Read_7051_t           =   0x0b,   
  
    Read_Scratchpad_Ext_t =   0x0c,
		Copy_Page0_t    	    =   0x0d,
	  Copy_Page1_t    	    =   0x0e,
		Search_ROM_t    	    =   0x0f,
		
		Write_ROM_ID_t     	    =   0x10,
    Write_Scratchpad_t     =   0x11,
    
    Check_3wire_2wire_RW        =  0x13,		
		Select_Single                = 0x14,
	
		//Instant_Test        =   0x10,
		//Unit_Test           =   0x11,
} DS18B20_TESTCASE;


void USART1_Config(void);


//int fputc(int ch, FILE *f);
//void USART1_printf(USART_TypeDef* USARTx, uint8_t *Data,...);

#endif /* __USART1_H */
