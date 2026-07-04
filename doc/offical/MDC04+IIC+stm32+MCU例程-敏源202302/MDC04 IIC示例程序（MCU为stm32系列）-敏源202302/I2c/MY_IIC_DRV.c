/* Includes ------------------------------------------------------------------*/
//#include <assert.h>
#include "MY_IIC_DRV.h"
#include "math.h"
#include "stm32f10x_rcc.h"
//#include "HAL_device.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Definition of GPIOI2C constants */
#define GPIOI2C_ACK  				((uint8_t)1)
#define GPIOI2C_NACK 				((uint8_t)0)
#define I2CWRITE_MASK  			    ((uint8_t)0xFE)
#define I2CREAD_MASK   			    ((uint8_t)0x01)
/* Private macro -------------------------------------------------------------*/
/* Definition of Macros for bit operations */
#define i2c_set_scl()   { GPIOI2C_SCL_GPIO_PORT->BSRR = GPIOI2C_SCL_PIN; } 
#define i2c_clear_scl() { GPIOI2C_SCL_GPIO_PORT->BRR = GPIOI2C_SCL_PIN; }
#define i2c_get_scl()   ( GPIOI2C_SCL_GPIO_PORT->IDR & GPIOI2C_SCL_PIN )
#define i2c_set_sda()   { GPIOI2C_SDA_GPIO_PORT->BSRR = GPIOI2C_SDA_PIN; }
#define i2c_clear_sda() { GPIOI2C_SDA_GPIO_PORT->BRR = GPIOI2C_SDA_PIN; } 
#define i2c_get_sda()   ( GPIOI2C_SDA_GPIO_PORT->IDR & GPIOI2C_SDA_PIN )

#define sda_wr_control(b) 		{ if(b & mask) i2c_set_sda() else i2c_clear_sda(); }
#define sda_rd_control(b) 		{ if(i2c_get_sda() != 0) b |= mask; }

/* Private variables ---------------------------------------------------------*/
#define SCL_STRETCH_TIMEOUT						1000000


/* Exported functions ---------------------------------------------------------*/

GPIOI2C_BusStateDef GPIOI2C_Bus_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;

		RCC_APB2PeriphClockCmd(GPIOI2C_SCL_BUSCLK, ENABLE);
		RCC_APB2PeriphClockCmd(GPIOI2C_SDA_BUSCLK, ENABLE);
	
//		GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIOI2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	
    GPIO_Init(GPIOI2C_SCL_GPIO_PORT, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIOI2C_SDA_PIN;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(GPIOI2C_SDA_GPIO_PORT, &GPIO_InitStructure);
	
    SDA_H;
    SCL_H;
	return GPIOI2C_BUS_IDLE;
}

/* Support size=0 for dummy write */
/**-----------------------------------------------------------------------
  * @brief  ÎÞ²ÎÖ¸Áî´«µÝº¯Êý£¨Õë¶Ô½ö·¢ËÍÖ¸Áî£¬Èç²âÎÂ¡¢µçÈÝ×ª»»£©
  * @param  DeviceAddr£ºµØÖ·  *Buff£ºÊý¾Ý´æ´¢Êý×é  Size£º·¢ËÍÊý¾Ý×Ü³¤¶È
  * @retval ÊÇ·ñ´«Êä³É¹¦
-------------------------------------------------------------------------*/
uint8_t GPIOI2C_Transmit(uint8_t DeviceAddr, uint8_t *Buff, uint8_t Size)
{uint8_t i,ret=GPIOI2C_XFER_LASTACK;

	if(I2C_Start()==GPIOI2C_BUS_ERROR)
  {
		I2C_Stop();
		return GPIOI2C_XFER_BUSERR; 
	}		
	
  if(!I2C_master_write((DeviceAddr<<1) & I2CWRITE_MASK))
	{
		
		I2C_Stop();
		return GPIOI2C_XFER_ADDRNACK;
		
			
	}
	
	for(i=0; i<Size; i++) 
	{
		if(I2C_master_write((*Buff++))==GPIOI2C_NACK)
		{
			if(i<(Size-1)) {ret=GPIOI2C_XFER_ABORTNACK;break;}
            else {ret=GPIOI2C_XFER_LASTNACK;}
		}
	}
	I2C_Stop();
	return ret;
}

/* Application assure size>=1. */
/**-----------------------------------------------------------------------
  * @brief  Êý¾Ý½ÓÊÕº¯Êý
  * @param  DeviceAddr£ºµØÖ·  *Buff£ºÊý¾Ý´æ´¢Êý×é  Size£º·¢ËÍÊý¾Ý×Ü³¤¶È
  * @retval ÊÇ·ñ´«Êä³É¹¦
-------------------------------------------------------------------------*/
uint8_t GPIOI2C_Receive(uint8_t DeviceAddr, uint8_t *Buff, uint8_t Size)
{ uint8_t i;	
	
	if(I2C_Start()==GPIOI2C_BUS_ERROR)
  {
		I2C_Stop();
		return GPIOI2C_XFER_BUSERR; 
	}		
	
  if(I2C_master_write((DeviceAddr<<1) | I2CREAD_MASK) == GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ADDRNACK;
	}
	
	for(i=0; i<(Size-1); i++) {
		(*Buff++)=I2C_master_read_Streching(GPIOI2C_ACK);
	}
	
	(*Buff)=I2C_master_read_Streching(GPIOI2C_NACK);
	
	I2C_Stop();
	
	return GPIOI2C_XFER_LASTNACK;
}


/**-----------------------------------------------------------------------
  * @brief  ÓÐ²ÎÖ¸Áî´«µÝº¯Êý£¨Õë¶ÔÄ³Ð©Ö¸ÁîºóÐèÒªÐ´ÈëÅäÖÃ²ÎÊýÀàÐÍ£¬ÈçÅäÖÃÁ¿³Ì£© 
  * @param  DeviceAddr:µØÖ· cmd:´«´ïµÄÖ¸Áî *pdata:ËæÖ¸Áî´«µÝµÄ²ÎÊý Size:·¢ËÍÊý¾Ý×Ü³¤¶È
  * @retval ÊÇ·ñ´«Êä³É¹¦
-------------------------------------------------------------------------*/
uint8_t GPIOI2C_Cmd_Write(uint8_t DeviceAddr, uint16_t cmd, uint8_t *pData, uint8_t Size)
{uint8_t i, ret=GPIOI2C_XFER_LASTACK;
	
	if(I2C_Start()==GPIOI2C_BUS_ERROR)
  {
		I2C_Stop();
		return GPIOI2C_XFER_BUSERR; 
	}		
	
  if(I2C_master_write((DeviceAddr<<1) & I2CWRITE_MASK)==GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ADDRNACK;
	}

	if(I2C_master_write((uint8_t)(cmd>>8))==GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ABORTNACK;	
	}
	if(I2C_master_write((uint8_t)(cmd))==GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ABORTNACK;	
	}	
		
	for(i=0; i<Size; i++) 
	{
		if(I2C_master_write((*pData++))==GPIOI2C_NACK)
		{
			if(i<(Size-1)) {ret=GPIOI2C_XFER_ABORTNACK; break;}
      else {ret=GPIOI2C_XFER_LASTNACK;}
		}
	}
	
	I2C_Stop();
	return ret;	
}

/**-----------------------------------------------------------------------
  * @brief  Êý¾Ý¶ÁÈ¡º¯Êý£¨Õë¶ÔÄ³Ð©Ö¸ÁîºóÐèÒªÐ´ÈëÅäÖÃ²ÎÊýÀàÐÍ£¬ÈçÅäÖÃÁ¿³Ì£© 
  * @param  DeviceAddr£ºµØÖ·  Cmd£º´ý·¢ËÍ¶ÁÈ¡Ö¸Áî *pdata:¶ÁÈ¡µ½µÄÊý¾Ý Size£º¶ÁÈ¡µÄÊý¾Ý³¤¶È
  * @retval ÊÇ·ñ´«Êä³É¹¦
-------------------------------------------------------------------------*/
uint8_t GPIOI2C_Cmd_Read(uint8_t DeviceAddr, uint16_t Cmd, uint8_t *pData, uint8_t Size)
{uint8_t i;
	
	if(I2C_Start()==GPIOI2C_BUS_ERROR)
  {
		I2C_Stop();
		return GPIOI2C_XFER_BUSERR; 
	}		
	
  if(I2C_master_write((DeviceAddr<<1) & I2CWRITE_MASK)==GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ADDRNACK;
	}

	if(I2C_master_write((uint8_t)(Cmd>>8)) == GPIOI2C_NACK)
	{
		I2C_Stop();

		return GPIOI2C_XFER_ABORTNACK;
	}
	
	if(I2C_master_write((uint8_t)(Cmd)) == GPIOI2C_NACK)
	{
		I2C_Stop();

		return GPIOI2C_XFER_ABORTNACK;
	}	
	
	I2C_reStart();
	
  if(I2C_master_write((DeviceAddr<<1) | I2CREAD_MASK)== GPIOI2C_NACK)
	{
		I2C_Stop();

		return GPIOI2C_XFER_ABORTNACK;
	}	
	
	for(i=0; i<(Size-1); i++) {
		(*pData++)=I2C_master_read_Streching(GPIOI2C_ACK);
	}
	
	(*pData)=I2C_master_read_Streching(GPIOI2C_NACK);
	
	I2C_Stop();

	return GPIOI2C_XFER_LASTNACK;
}

void Quick_wakeup(void)
   { 
	  i2c_set_scl();
		
	  i2c_clear_sda();
	   
      MY_DELAY_US(50);   
	  i2c_set_sda();
	   
      MY_DELAY_US(10);
	  i2c_clear_scl();
	   
      MY_DELAY_US(20);
 	  i2c_clear_sda();	
	   
      MY_DELAY_US(25);
	  i2c_set_scl();
	   
      MY_DELAY_US(10);
	  i2c_set_sda();
      MY_DELAY_US(15);
   }

int I2C_Start(void)
{
	

	Quick_wakeup();
	Quick_wakeup();
	Quick_wakeup();
	Quick_wakeup();
	Quick_wakeup();
	/*Bus free time(between Stop to Start time) tBUF>1.3us met by implement in stop() function*/
	i2c_clear_sda();
	MY_DELAY_US(200); 
	i2c_clear_scl();
	return GPIOI2C_BUS_BUSY;
}


int I2C_reStart(void)
{
  /*ReStart condition */
	i2c_set_sda();
	MY_DELAY_US(1);  
	i2c_set_scl();
	MY_DELAY_US(1);  
	i2c_clear_sda();
	MY_DELAY_US(1); 
	i2c_clear_scl();
	MY_DELAY_US(1); 
	return GPIOI2C_BUS_BUSY;
}


int I2C_Stop(void)
{
  /*Stop condition */
	i2c_clear_sda();
	MY_DELAY_US(1);  /*SU;STA*/
  i2c_set_scl();
  MY_DELAY_US(1);   /*tSU;STO*/
  i2c_set_sda();	
  MY_DELAY_US(1);    /*tBUF>1.3us for Bus free time*/
	
	return GPIOI2C_BUS_IDLE;
}


int I2C_master_write(uint8_t b)
{
  uint8_t mask = 0x80, ack;
	do
  {	
		MY_DELAY_US(1);  /*tHD;DAT*/				
		sda_wr_control(b);	
		MY_DELAY_US(1);  /*tSU_DAT*/	
		i2c_set_scl();
		MY_DELAY_US(1);  /*nHigh*/
		i2c_clear_scl();			
  }while ((mask>>=1) != 0);

	/* ACK slot checking */
	MY_DELAY_US(1); /*tHD;DAT*/		
	i2c_set_sda();/* Release SDA, waiting for tVD_ACK */
	MY_DELAY_US(1);  /*tSU_DAT*/
	i2c_set_scl();
	MY_DELAY_US(1); /*tHD;DAT*/

	ack = (i2c_get_sda()? 0:1);	
	MY_DELAY_US(1);  /*nHigh*/
	i2c_clear_scl();
  return (ack);
}

/**-----------------------------------------------------------------------
  * @brief  µÈ´ýÊ±ÖÓÀ­Éì»Ö¸´
  * @param  µÈ´ýÊ±¼ä
  * @retval µÈ´ý±êÖ¾Î»
-------------------------------------------------------------------------*/
int waitSclStretching(int ms_time_out)
{	
	while(!i2c_get_scl())
	{
		if(ms_time_out-- <= 0) return 1;
		MY_DELAY_US(1);		
	}
	
	return 0;
}

/**
* @brief I2C master read 8-bit bit-bang
* @param unsigned char ack – acknowledgement control
* @retval unsigned char b –  data received
*/
uint8_t I2C_master_read_Streching(uint8_t ack)
{ uint8_t mask = 0x80, b = 0;

	i2c_set_sda();									/*Release SDA*/

	do
	{
	MY_DELAY_US(1);	
	i2c_set_scl();
	waitSclStretching(SCL_STRETCH_TIMEOUT);
	MY_DELAY_US(1);
	sda_rd_control(b);
	i2c_clear_scl();
	MY_DELAY_US(1);
	}while ((mask>>=1) != 0);

	/* ACK slot control */
	if(ack != 0) i2c_clear_sda();		/* SDA=0 for ACK*/
	MY_DELAY_US(1);
	i2c_set_scl();
	MY_DELAY_US(1);
	i2c_clear_scl();
	MY_DELAY_US(1);

	return (b);
}

