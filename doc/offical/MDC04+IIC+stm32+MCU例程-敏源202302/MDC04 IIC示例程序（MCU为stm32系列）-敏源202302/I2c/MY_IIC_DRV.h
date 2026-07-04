/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MY_IIC_DRV_H
#define MY_IIC_DRV_H

/* Includes ------------------------------------------------------------------*/
//#include "HAL_device.h"
#include <stdint.h>
#include "MDC04.h"
#include "stm32f10x_rcc.h"

/* Imported variables ------------------------------------------------------------*/
extern uint32_t SystemCoreClock;

//***********用户在此处配置SCL/SDA对应IO口***********************************//

#define GPIOI2C_SCL_GPIO_PORT          GPIOC
#define GPIOI2C_SCL_PIN                GPIO_Pin_6
#define GPIOI2C_SCL_BUSCLK             RCC_APB2Periph_GPIOC
#define GPIOI2C_SCL_GPIO_CLK_ENABLE()  RCC_APB2PeriphClockCmd(GPIOI2C_SCL_BUSCLK, ENABLE);

#define GPIOI2C_SDA_GPIO_PORT          GPIOC
#define GPIOI2C_SDA_PIN                GPIO_Pin_7
#define GPIOI2C_SDA_BUSCLK             RCC_APB2Periph_GPIOC
#define GPIOI2C_SDA_GPIO_CLK_ENABLE()  RCC_APB2PeriphClockCmd(GPIOI2C_SDA_BUSCLK, ENABLE);

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint32_t CLKSpeed;       /*!< Specifies the clock frequency.
                                  This parameter must be set to a value lower than 400kHz */
  uint32_t DutyCycle;        /*!< Specifies the I2C fast mode duty cycle.
                                  This parameter can be a value of @ref I2C_duty_cycle_in_fast_mode */
  uint32_t tLOW;      /*!< Specifies the first device own address.
                                  This parameter can be a 7-bit or 10-bit address. */
  uint32_t tHigh;   /*!< Specifies if 7-bit or 10-bit addressing mode is selected.
                                  This parameter can be a value of @ref I2C_addressing_mode */
  uint32_t tHOLD;  /*!< Specifies if dual addressing mode is selected.
                                  This parameter can be a value of @ref I2C_dual_addressing_mode */
  uint32_t tSETUP;      /*!< Specifies the second device own address if dual addressing mode is selected
                                  This parameter can be a 7-bit address. */
  uint32_t Mode;  /*!< Specifies if general call mode is selected.
                                  This parameter can be a value of @ref I2C_general_call_addressing_mode */
  uint32_t NoStretchMode;    /*!< Specifies if nostretch mode is selected.
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
#define GPIOI2C_XFER_LASTNACK         ((uint8_t)0x00)   	 	/* !< No error              */
#define GPIOI2C_XFER_ADDRNACK   			((uint8_t)0x01)    	  /* !< No Device             */
#define GPIOI2C_XFER_ABORTNACK  			((uint8_t)0x02)    	  /* !< NACK before last byte */
#define GPIOI2C_XFER_LASTACK    			((uint8_t)0x04)     	/* !< ACK last byte         */
#define GPIOI2C_XFER_BUSERR     			((uint8_t)0x10)     	/* !< Bus error             */
#define GPIOI2C_XFER_BUSBUSY     			((uint8_t)0x20)     	/* !< Bus busy              */

#define HAL_GPIO_Init				GPIO_Init
#define HAL_GPIO_WritePin		    GPIO_WriteBit
#define HAL_GPIO_ReadPin		    GPIO_ReadOutputDataBit
/****ESK Simulated I2C definition*****/
#define I2C1_SCL_PORT               GPIOC
#define I2C1_SCL_PIN                GPIO_Pin_6
#define I2C1_SCL_BUSCLK             RCC_APB2Periph_GPIOC

#define I2C1_SDA_PORT               GPIOC
#define I2C1_SDA_PIN                GPIO_Pin_7
#define I2C1_SDA_BUSCLK             RCC_APB2Periph_GPIOC

#define SCL_H                       I2C1_SCL_PORT->BSRR = I2C1_SCL_PIN
#define SCL_L                       I2C1_SCL_PORT->BRR  = I2C1_SCL_PIN
#define SCL_read                    (I2C1_SCL_PORT-IDR  & I2C1_SCL_PIN)

#define	SDA_H	                      I2C1_SDA_PORT->BSRR	= I2C1_SDA_PIN
#define SDA_L	                      I2C1_SDA_PORT->BRR	= I2C1_SDA_PIN
#define SDA_read                    (I2C1_SDA_PORT->IDR & I2C1_SDA_PIN)
/* Exported functions --------------------------------------------------------*/
GPIOI2C_BusStateDef GPIOI2C_Bus_Init(void);
uint8_t GPIOI2C_Receive(uint8_t DeviceAddr, uint8_t *pData, uint8_t Size);
uint8_t GPIOI2C_Transmit(uint8_t DeviceAddr, uint8_t *pData, uint8_t Size);
uint8_t GPIOI2C_Cmd_Write(uint8_t DeviceAddr, uint16_t cmd, uint8_t *pData, uint8_t Size);
uint8_t GPIOI2C_Cmd_Read(uint8_t DeviceAddr, uint16_t Cmd, uint8_t *pData, uint8_t Size);
int I2C_Start(void);
int I2C_reStart(void);
int I2C_Stop(void);
int I2C_master_write(uint8_t b);
uint8_t I2C_master_read_Streching(uint8_t ack);


#endif /* SI2C_H */
