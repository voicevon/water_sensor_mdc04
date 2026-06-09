/* Includes ------------------------------------------------------------------*/

#include "IIC.h"
#include "delay.h"

#define unit unsigned int



#define MY_DELAY_US Delay_us
#define GPIOI2C_ACK  				((unsigned char)1)
#define GPIOI2C_NACK 				((unsigned char)0)
#define I2CWRITE_MASK  			    ((unsigned char)0xFE)
#define I2CREAD_MASK   			    ((unsigned char)0x01)
/* Private macro -------------------------------------------------------------*/
/* Definition of Macros for bit operations */


/* Private variables ---------------------------------------------------------*/
#define SCL_STRETCH_TIMEOUT						1000000

#define uchar unsigned char
/* Exported functions ---------------------------------------------------------*/

void I2C_Init(void)
{
  P10F = OUTPUT|PU_EN;				//P11设置为开漏输出 SCL
	P11F = OUTPUT|OP_EN;        //P12设置为开漏输出  SDA
  i2c_clear_scl();
  i2c_clear_sda();
}

/* Support size=0 for dummy write */
/**-----------------------------------------------------------------------
  * @brief  无参指令传递函数（针对仅发送指令，如测温、电容转换）
  * @param  DeviceAddr：地址  *Buff：数据存储数组  Size：发送数据总长度
  * @retval 是否传输成功
-------------------------------------------------------------------------*/
uchar GPIOI2C_Transmit(unsigned char DeviceAddr, unsigned char *Buff, unsigned char Size)
{
	unsigned char i,ret=GPIOI2C_XFER_LASTACK;

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
			if(i<(Size-1)) 
      {
			  ret=GPIOI2C_XFER_ABORTNACK; 
				break;
			}
      else 
      {
			  ret=GPIOI2C_XFER_LASTNACK;
			}
		}
	}
	I2C_Stop();
	return ret;
}

/* Application assure size>=1. */
/**-----------------------------------------------------------------------
  * @brief  数据接收函数
  * @param  DeviceAddr：地址  *Buff：数据存储数组  Size：发送数据总长度
  * @retval 是否传输成功
-------------------------------------------------------------------------*/
uchar GPIOI2C_Receive(unsigned char DeviceAddr, unsigned char *Buff, unsigned char Size)
{ 
	unsigned char i;	
	
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
	
	for(i=0; i<(Size-1); i++) 
	{
		(*Buff++)=I2C_master_read_Streching(GPIOI2C_ACK);
	}
	
	(*Buff)=I2C_master_read_Streching(GPIOI2C_NACK);
	
	I2C_Stop();
	
	return GPIOI2C_XFER_LASTNACK;
}


/**-----------------------------------------------------------------------
  * @brief  有参指令传递函数（针对某些指令后需要写入配置参数类型，如配置量程） 
  * @param  DeviceAddr:地址 cmd:传达的指令 *pdata:随指令传递的参数 Size:发送数据总长度
  * @retval 是否传输成功
-------------------------------------------------------------------------*/
uchar GPIOI2C_Cmd_Write(unsigned char DeviceAddr, unsigned short cmd, unsigned char *pData, unsigned char Size)
{
	unsigned char i, ret=GPIOI2C_XFER_LASTACK;
	
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

	if(I2C_master_write((unsigned char)(cmd>>8))==GPIOI2C_NACK)
	{
		I2C_Stop();
		return GPIOI2C_XFER_ABORTNACK;	
	}
	if(I2C_master_write((unsigned char)(cmd))==GPIOI2C_NACK)
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
  * @brief  数据读取函数（针对某些指令后需要写入配置参数类型，如配置量程） 
  * @param  DeviceAddr：地址  Cmd：待发送读取指令 *pdata:读取到的数据 Size：读取的数据长度
  * @retval 是否传输成功
-------------------------------------------------------------------------*/
uchar GPIOI2C_Cmd_Read(unsigned char DeviceAddr, unsigned short Cmd, unsigned char *pData, unsigned char Size)
{
	unsigned char i;
	
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

	if(I2C_master_write((unsigned char)(Cmd>>8)) == GPIOI2C_NACK)
	{
		I2C_Stop();

		return GPIOI2C_XFER_ABORTNACK;
	}
	
	if(I2C_master_write((unsigned char)(Cmd)) == GPIOI2C_NACK)
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
	/*Bus free time(between Stop to Start time) tBUF>1.3us met by implement in stop() function*/
	i2c_clear_sda();
	MY_DELAY_US(30); 
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
    MY_DELAY_US(1);/*tBUF>1.3us for Bus free time*/
	
	return GPIOI2C_BUS_IDLE;
}


int I2C_master_write(unsigned char b)
{
  unsigned char mask = 0x80,ack;
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
  i2c_get_sda();
	ack=(SDA?0:1);
	MY_DELAY_US(1);  /*nHigh*/
	i2c_clear_scl();
  return (ack);
}

/**-----------------------------------------------------------------------
  * @brief  等待时钟拉伸恢复
  * @param  等待时间
  * @retval 等待标志位
-------------------------------------------------------------------------*/
int waitSclStretching(int ms_time_out)
{	
	i2c_get_scl();
	while(!SCL)
	{
		if(ms_time_out-- <= 0) return 1;
		MY_DELAY_US(1);		
	}
	
	return 0;
}

/**
* @brief I2C master read 8-bit bit-bang
* @param unsigned char ack ?acknowledgement control
* @retval unsigned char b ? data received
*/
uchar I2C_master_read_Streching(unsigned char ack)
{ unsigned char mask= 0x80, b = 0;

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
	}while ((mask>>=1)!=0);

	/* ACK slot control */
	if(ack != 0) i2c_clear_sda();		/* SDA=0 for ACK*/
	MY_DELAY_US(1);
	i2c_set_scl();
	MY_DELAY_US(1);
	i2c_clear_scl();
	MY_DELAY_US(1);

	return (b);
}
