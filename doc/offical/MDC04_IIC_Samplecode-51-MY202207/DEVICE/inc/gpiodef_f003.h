#ifndef _GPIODEF_H_
#define _GPIODEF_H_
enum
{
	HIGH_Z		= 0,					
	INPUT		= 1,          
	OUTPUT		= 2,          
                     
	PWM_SETTING		= 5,
                        
	P12_ADC0_SETTING			= 3,
	P11_ADC1_SETTING			= 3,
	P01_ADC2_SETTING			= 3,
	P03_ADC3_SETTING			= 3,
	P04_ADC4_SETTING			= 3,
	P05_ADC5_SETTING			= 3,
	P06_ADC6_SETTING			= 3,
	P07_ADC7_SETTING			= 3,
	P30_ADC8_SETTING			= 3,
	P17_ADC9_SETTING			= 3,
	P16_ADC10_SETTING			= 3,
	P15_ADC11_SETTING			= 3,
	P12_ADC_VREF_SETTING		= 4,
                       
	P06_UART1_TX_SETTING		= 4,      
	P07_UART1_RX_SETTING		= 4,      
	P16_UART2_TX_SETTING		= 4,      
	P02_UART2_RX_SETTING		= 4,      
                         
	P14_I2C_SDA_SETTING			= 6,      
	P13_I2C_SCL_SETTING			= 6,      
                      
	P16_I2C_SDA_SETTING			= 6,      
	P02_I2C_SCL_SETTING			= 6,      
   
	
	P10_SPI_SCK_SETTING			= 6,      
	P01_SPI_MISO_SETTING		= 6,      
	P00_SPI_MOSI_SETTING		= 6,      
	P15_SPI_SSB_SETTING			= 6,      
	
	P05_OPAINP_SETTING			= 6,
	P06_OPAINN_SETTING			= 6,
	P07_OPAOUT_SETTING			= 6,

	P03_OPBINP_SETTING			= 6,
	P04_OPBINN_SETTING			= 6,		
                        
	P05_T0_SETTING				= 1,        
	P00_T1_SETTING				= 1,        
	P10_T2_SETTING				= 1,        
                        
	P20_RESET_SETTING			= 3,			  
	P30_XOSCH_IN_SETTING		= 4,
	P17_XOSCH_OUT_SETTING		= 4,  
	

	T2CP_SETTING				= 7,  
	P11_T2EX_SETTING			= 1,  

	P02_SWIM_SETTING			= 3,
	P05_BEEP_SETTING			= 4,
	P14_FB_SETTING				= 4,
	P11_CLO_SETTING				= 6,
	P30_CLKI_SETTING			= 6,
	P12_T2CPO_SETTING			= 7,

	PU_EN				= 0x80,
	PD_EN				= 0x40,
	OP_EN				= 0x20,
};	
#define  GPIO_Init(reg,val)	reg = val


/******************************************************************************/

#endif
