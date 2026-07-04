/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MY_IIC_DRV_H
#define MY_IIC_DRV_H

/* Includes ------------------------------------------------------------------*/
#include "ca51f003xsfr.h"
#include "gpiodef_f003.h"
#include "ca51f003sfr.h"

sbit SDA = P1^1 ;
sbit SCL = P1^0 ;



/* Imported variables ------------------------------------------------------------*/
extern unsigned long SystemCoreClock;
#define uchar unsigned char
/* Exported types ------------------------------------------------------------*/
#define i2c_set_scl()   { P10=1;}
#define i2c_clear_scl() { P10=0;}
#define i2c_get_scl()   ( P10F == INPUT)
#define i2c_set_sda()   { P11=1;}
#define i2c_clear_sda() { P11=0;}
#define i2c_get_sda()   ( P11F == INPUT)

#define sda_wr_control(b) 		{ if(b & mask) i2c_set_sda() else i2c_clear_sda(); }
#define sda_rd_control(b) 		{ i2c_get_sda();if( SDA != 0) b |= mask; }
typedef struct
{
  unsigned long CLKSpeed;       /*!< Specifies the clock frequency.
                                  This parameter must be set to a value lower than 400kHz */
  unsigned long DutyCycle;        /*!< Specifies the I2C fast mode duty cycle.
                                  This parameter can be a value of @ref I2C_duty_cycle_in_fast_mode */
  unsigned long tLOW;      /*!< Specifies the first device own address.
                                  This parameter can be a 7-bit or 10-bit address. */
  unsigned long tHigh;   /*!< Specifies if 7-bit or 10-bit addressing mode is selected.
                                  This parameter can be a value of @ref I2C_addressing_mode */
  unsigned long tHOLD;  /*!< Specifies if dual addressing mode is selected.
                                  This parameter can be a value of @ref I2C_dual_addressing_mode */
  unsigned long tSETUP;      /*!< Specifies the second device own address if dual addressing mode is selected
                                  This parameter can be a 7-bit address. */
  unsigned long Mode;  /*!< Specifies if general call mode is selected.
                                  This parameter can be a value of @ref I2C_general_call_addressing_mode */
  unsigned long NoStretchMode;    /*!< Specifies if nostretch mode is selected.
                                  This parameter can be a value of @ref I2C_nostretch_mode */
}GPIOI2C_ConfDef;

/** @defgroup GPIOI2C_state_structure_definition GPIO state structure definition
  * @brief  GPIO State structure definition  
  * @{
  */ 
typedef enum
{
  GPIOI2C_BUS_RESET         	= 0x10,   /*!< Peripheral is not yet Initialized         */
  GPIOI2C_BUS_IDLE              = 0x00,   /*!< Peripheral Initialized and ready for use  */
  GPIOI2C_BUS_BUSY              = 0x01,   /*!< An transaction process is ongoing         */   
  GPIOI2C_BUS_ERROR             = 0x20    /*!< Error                                     */ 
}GPIOI2C_BusStateDef;

/** 
  * @}
  */
typedef enum
{
  GPIOI2C_MODE_MASTER              = 0x00,   /*!< I2C communication is in Slave Mode       */
  GPIOI2C_MODE_SLAVE               = 0x10,   /*!< I2C communication is in Master Mode      */
}GPIOI2C_ModeDef;

/** @defgroup I2C_handle_Structure_definition I2C handle Structure definition 
  * @brief  I2C handle Structure definition  
  * @{
  */

/* Exported constants --------------------------------------------------------*/	
/** @defgroup  
  * I2C transaction result
  */ 
#define GPIOI2C_XFER_LASTNACK         ((unsigned char)0x00)   	 	/* !< No error              */
#define GPIOI2C_XFER_ADDRNACK   			((unsigned char)0x01)    	  /* !< No Device             */
#define GPIOI2C_XFER_ABORTNACK  			((unsigned char)0x02)    	  /* !< NACK before last byte */
#define GPIOI2C_XFER_LASTACK    			((unsigned char)0x04)     	/* !< ACK last byte         */
#define GPIOI2C_XFER_BUSERR     			((unsigned char)0x10)     	/* !< Bus error             */
#define GPIOI2C_XFER_BUSBUSY     			((unsigned char)0x20)     	/* !< Bus busy              */

///* Definition of GPIOI2C Pins and Macro*/

#define GPIOI2C_SCL_GPIO_PORT          GPIOD
#define GPIOI2C_SCL_PIN                GPIO_Pin_1
#define GPIOI2C_SCL_BUSCLK             RCC_AHBPeriph_GPIOD
#define GPIOI2C_SCL_GPIO_CLK_ENABLE()  RCC_AHBPeriphClockCmd(GPIOI2C_SCL_BUSCLK, ENABLE);

#define GPIOI2C_SDA_GPIO_PORT          GPIOD
#define GPIOI2C_SDA_PIN                GPIO_Pin_0
#define GPIOI2C_SDA_BUSCLK             RCC_AHBPeriph_GPIOD
#define GPIOI2C_SDA_GPIO_CLK_ENABLE()  RCC_AHBPeriphClockCmd(GPIOI2C_SDA_BUSCLK, ENABLE);

#define HAL_GPIO_Init				GPIO_Init
#define HAL_GPIO_WritePin		    GPIO_WriteBit
#define HAL_GPIO_ReadPin		    GPIO_ReadOutputDataBit
/****ESK Simulated I2C definition*****/
#define I2C1_SCL_PORT               GPIOD
#define I2C1_SCL_PIN                GPIO_Pin_1
#define I2C1_SCL_BUSCLK             RCC_AHBPeriph_GPIOD

#define I2C1_SDA_PORT               GPIOD
#define I2C1_SDA_PIN                GPIO_Pin_0
#define I2C1_SDA_BUSCLK             RCC_AHBPeriph_GPIOD

#define SCL_H                       I2C1_SCL_PORT->BSRR = I2C1_SCL_PIN
#define SCL_L                       I2C1_SCL_PORT->BRR  = I2C1_SCL_PIN
#define SCL_read                    (I2C1_SCL_PORT-IDR  & I2C1_SCL_PIN)

#define	SDA_H	                      I2C1_SDA_PORT->BSRR	= I2C1_SDA_PIN
#define SDA_L	                      I2C1_SDA_PORT->BRR	= I2C1_SDA_PIN
#define SDA_read                     (I2C1_SDA_PORT->IDR & I2C1_SDA_PIN)
/* Exported functions --------------------------------------------------------*/
void I2C_Init(void);
uchar GPIOI2C_Receive(unsigned char DeviceAddr, unsigned char *pData, unsigned char Size);
uchar GPIOI2C_Transmit(unsigned char DeviceAddr, unsigned char *pData, unsigned char Size);
uchar GPIOI2C_Cmd_Write(unsigned char DeviceAddr, unsigned short cmd, unsigned char *pData, unsigned char Size);
uchar GPIOI2C_Cmd_Read(unsigned char DeviceAddr, unsigned short Cmd, unsigned char *pData, unsigned char Size);
int I2C_Start(void);
int I2C_reStart(void);
int I2C_Stop(void);
int I2C_master_write(unsigned char b);
uchar I2C_master_read_Streching(unsigned char ack);


#endif /* SI2C_H */
