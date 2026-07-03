#ifndef _MDC04_C_
#define _MDC04_C_

#include "ca51f003_config.h"
#include "owmy.h"
#include "MDC04.h"
#include "delay.h"
#include "uart.h"


//#define u16 unsigned short 


/****全局变量：保存和电容配置寄存器对应的偏置电容和量程电容数值****/
float CapCfg_offset, CapCfg_range;

/****偏置电容和反馈电容阵列权系数****/
static const float COS_Factor[8] = {0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 40.0};
/*Cos= (40.0*q[7]+32.0*q[6]+16.0*q[5]+8.0*q[4]+4.0*q[3]+2.0*q[2]+1.0*q[1]+0.5*q[0])*/
static const struct  {float Cfb0; float Factor[6];} CFB = { 2.0, 2.0, 4.0, 8.0, 16.0, 32.0, 46.0};
/*Cfb =(46*p[5]+32*p[4]+16*p[3]+8*p[2]+4*p[1]+2*p[0]+2)*/


void  ConvertTemp(void)
{	 
	int i;
	unsigned char  scr[8];
//	 uchar c[2];
	unsigned short  Temp_u16;
	float temp;
	OW_ResetPresence();	
	
		

  OW_WriteByte(0xcc);
  OW_WriteByte(0x44);
	Delay_ms(15);
	
	OW_ResetPresence();	
   OW_WriteByte(0xcc);
    OW_WriteByte(0xbe);
	

		for(i=0; i < 8; i++)
    {
			scr[i] = OW_ReadByte();
//		    uart_printf("%bx ",scr);
	}

	Temp_u16 = scr[1]<<8 | scr[0];

//	uart_printf(" Temp_u16:%x  ",Temp_u16);
	temp = ((short)Temp_u16)/256.0+40.0;
	
	uart_printf("temp:%.2f \n",temp);
	
 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  * @brief  读芯片寄存器的参数组
  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRPARAMETERS）
  * @retval 读状态
**/
int MDC04_ReadParameters_SkipRom(unsigned char *scr)
{
    int i;

    if(OW_ResetPresence() == 1)						
			return 0;
		
    OW_WriteByte(0xcc);
    OW_WriteByte(0x8b);//READ_PARAMETERS
		
		for(i=0; i < sizeof(MDC04_SCRPARAMETERS); i++)
    {
		*scr++ = OW_ReadByte();
	}

    return 1;
}
/**
  * @brief  写芯片寄存器的参数组
  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRPARAMETERS）
  * @retval 写状态
**/
int MDC04_WriteParameters_SkipRom(unsigned char *scr)
{
    int i;

    if(OW_ResetPresence() == 1)					
			return 0;
		
    OW_WriteByte(0xcc);
    OW_WriteByte(0xab);//WRITE_PARAMETERS
		
		for(i=0; i < sizeof(MDC04_SCRPARAMETERS); i++)
    {
		OW_WriteByte(*scr++);
	}

    return 1;
}

/**
  * @brief  读芯片寄存器的暂存器组
  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRATCHPAD_READ）
  * @retval 读状态
*/
int MDC04_ReadScratchpad_SkipRom(unsigned char *scr)
{
    int i;

	/*size < sizeof(MDC04_SCRATCHPAD_READ)*/
    if(OW_ResetPresence() == 1)					
			return 0;
		
    OW_WriteByte(0xcc);
    OW_WriteByte(0xbe);//READ_SCRATCHPAD
		
		for(i=0; i < sizeof(MDC04_SCRATCHPAD_READ); i++)
    {
		*scr++ = OW_ReadByte();
	}

    return 1;
}
/**
  * @brief  写芯片寄存器的暂存器组
  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRATCHPAD_WRITE）
  * @retval 写状态
**/
int MDC04_WriteScratchpad_SkipRom(unsigned char *scr)
{
    int i;

    if(OW_ResetPresence() == 1)						
			return 0;
		
    OW_WriteByte(0xcc);
    OW_WriteByte(0x4e);//WRITE_SCRATCHPAD
		
		for(i=0; i < sizeof(MDC04_SCRATCHPAD_WRITE); i++)
    {
			OW_WriteByte(*scr++);
	}

    return 1;
}


/**
  * @brief  读量程电容配置寄存器内容
  * @param  Cfb：量程配置寄存器低6位的内容
  * @retval 状态
*/
int ReadCfbConfig(unsigned char *Cfb)
{ 
	unsigned char scrb[sizeof(MDC04_SCRPARAMETERS)];
	MDC04_SCRPARAMETERS *scr = (MDC04_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/	
	if(MDC04_ReadParameters_SkipRom(scrb) == 0)
	{		
		return 0;  /*读寄存器失败*/
	}
	
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/	
//  if(scrb[sizeof(MDC04_SCRPARAMETERS)-1] != MY_OW_CRC8(scrb, sizeof(MDC04_SCRPARAMETERS)-1))
//  {	
//		return FALSE;   /*CRC验证未通过*/
//  }	
		
	*Cfb = scr->Cfb & 0x3F;
	
  return 1;
}

/**
  * @brief  读偏置电容配置寄存器内容
  * @param  Coffset：偏置配置寄存器有效位的内容
  * @retval 无
*/
int ReadCosConfig(unsigned char *Coscfg)
{ 
	unsigned char scrb[sizeof(MDC04_SCRPARAMETERS)];
	MDC04_SCRPARAMETERS *scr = (MDC04_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/	
	if(MDC04_ReadParameters_SkipRom(scrb) == 0)
	{		
		return 0;  /*读寄存器失败*/
	}
	
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/	
//  if(scrb[sizeof(MDC04_SCRPARAMETERS)-1] != MY_OW_CRC8(scrb, sizeof(MDC04_SCRPARAMETERS)-1))
//  {	
//		return FALSE;  /*CRC验证未通过*/
//  }	

	*Coscfg = scr->Cos & (0xFF >> (3 - (scr->Cfb >> 6))); //屏蔽掉无效位，根据CFB寄存器的高2位
	
  return 1;		
}


/**
  * @brief  将偏置电容配置转换为对应的偏置电容数值（pF）
  * @param  osCfg：偏置电容配置
  * @retval 对应偏置电容的数值
*/
float CoscfgtoCapOffset(unsigned char  osCfg)
{
	int i; 
	float Coffset = 0.0; 

	for(i = 0; i < 8; i++) 
	{
		if(osCfg & 0x01) Coffset += COS_Factor[i];
		osCfg >>= 1;
	}
	
	return Coffset;
}
/**
  * @brief  将量程电容配置转换为对应的量程电容数值（pF）
  * @param  fbCfg：量程电容配置
  * @retval 对应量程电容的数值
*/
float CfbcfgtoCapRange(unsigned char  fbCfg)
{
	int i;	
	float Crange = CFB.Cfb0;
	
	for(i = 0; i <= 5; i++)
	{
		if(fbCfg & 0x01) Crange += CFB.Factor[i];
		fbCfg >>= 1;
	}

	return (0.507/3.6) * Crange;	
}

/**
  * @brief  启动温度和电容通道1同时测量
  * @param  无
  * @retval 单总线发送状态
*/
void ConvertTC1(void)
{	 
			
  OW_ResetPresence();

  OW_WriteByte(0xcc);
  OW_WriteByte(0x10);
		
}

/**
  * @brief  获取配置的偏置电容数值（pF）
  * @param  Coffset：偏置电容配置
  * @retval 无
*/
void GetCfg_CapOffset(float *Coffset)
{
	unsigned char  Cos_cfg;
	
	ReadCosConfig(&Cos_cfg);
	*Coffset = CoscfgtoCapOffset(Cos_cfg);	
}

/**
  * @brief  获取配置的量程电容数值（pF）
  * @param  Crange：返回量程电容数值
  * @retval 无
*/
void GetCfg_CapRange(float *Crange)
{
	unsigned char  Cfb_cfg;
	
	ReadCfbConfig(&Cfb_cfg);
	*Crange = CfbcfgtoCapRange(Cfb_cfg);
}


/**
  * @brief  读电容配置
  * @param  Coffset：配置的偏置电容。
  * @param  Crange：配置的量程电容。
  * @retval 无
*/
void ReadCapConfigure(float *Coffset, float *Crange)
{ 
	GetCfg_CapOffset(Coffset);
	GetCfg_CapRange(Crange);

}
/**
  * @brief  把16位二进制补码表示的温度输出转换为以摄氏度为单位的温度读数
  * @param  out：有符号的16位二进制温度输出
  * @retval 以摄氏度为单位的浮点温度
*/
float MDC04_OutputtoTemp(unsigned short out)
{
	return (((short)out)/256.0 + 40.0);	
}

/**
  * @brief  把16位二进制电容输出转换为以pF为单位的电容读数
  * @param  out：无符号的16位二进制电容输出
  * @param  Co：配置的偏置电容数值
  * @param  Cr：配置的范围电容（量程）数值
  * @retval 以pF为单位的浮点电容数值
*/
float MDC04_OutputtoCap(unsigned short out, float Co, float Cr)
{
	return (2.0*(out/65535.0-0.5)*Cr+Co);	
}


 int ReadTempCap1(unsigned short *iTemp, unsigned short *iCap1)
{	
	unsigned char scrb[sizeof(MDC04_SCRATCHPAD_READ)];
	MDC04_SCRATCHPAD_READ *scr = (MDC04_SCRATCHPAD_READ *) scrb;

	/*读9个字节。前两个是温度转换结果，最后字节是前8个的校验和--CRC。*/	
	if(MDC04_ReadScratchpad_SkipRom(scrb) == 0)
	{		
		return 0;  /*读寄存器失败*/
	}

//	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/	
//  if(scrb[8] != MY_OW_CRC8(scrb, 8))
//  {	
//		return FALSE;  /*CRC验证未通过*/
//  }
			
	*iTemp=scr->T_msb<<8 | scr->T_lsb;			
	*iCap1=scr->C1_msb<<8 | scr->C1_lsb;	
//	uart_printf("iCap1=%x  ",iCap1);

  return 1;		
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////


int SetCapChannel(unsigned char chann)
{ 
	unsigned char scrb[sizeof(MDC04_SCRPARAMETERS)];
	MDC04_SCRPARAMETERS *scr = (MDC04_SCRPARAMETERS *) scrb;

	/*读15个字节。第4字节是通道选择寄存器，最后字节是前14个的校验和--CRC。*/	
	if(MDC04_ReadParameters_SkipRom(scrb) == 0)
	{		
		return 0;  /*读寄存器失败*/
	}
	
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/	
//  if(scrb[sizeof(MDC04_SCRPARAMETERS)-1] != MY_OW_CRC8(scrb, sizeof(MDC04_SCRPARAMETERS)-1))
//  {	
//		return FALSE;  /*CRC验证未通过*/
//  }	
	
	scr->Ch_Sel = (scr->Ch_Sel & ~0x07) | (chann & 0x07);
	
	MDC04_WriteParameters_SkipRom(scrb);

	return 1;
}

void Convert_TC1(void)
{
	unsigned short iTemp, iCap1;
	float fTemp, fCap1,sum_cap,sum_temp;
	int k1,AVERAGE_NUM =20,count = 0;
	
	SetCapChannel(0x01);//CAP_CH1_SEL
	

	
	
	for( k1 =0;k1<AVERAGE_NUM;k1++)
	{
		ConvertTC1();
		Delay_ms(15);
		
		ReadTempCap1(&iTemp, &iCap1) ;
//		uart_printf("iCap1=%x  ",iCap1);
		fTemp=MDC04_OutputtoTemp(iTemp);
		//f = 15.000 ra = 15.492
//		uart_printf("of = %.3f ra = %.3f  ",CapCfg_offset,CapCfg_range);
		fCap1=MDC04_OutputtoCap(iCap1, CapCfg_offset, CapCfg_range);
//		uart_printf(" %.3f    ",fCap1);
		sum_temp += fTemp;
		sum_cap += fCap1;
		count++;
	    Delay_ms(200);
	//						Delay_ms(500);

	}

		
	if(count >= AVERAGE_NUM)
	{
		fCap1 = sum_cap/count;
		fTemp = sum_temp/count;

		sum_cap = 0;
		sum_temp = 0;
		count=0;
	}
	
	
	uart_printf("Temp: %.2f Cap1: %.3f\r\n",fTemp,fCap1);

}

void readparameter(void)
{
	int i;
	unsigned char para[15];

   OW_ResetPresence();
		
    OW_WriteByte(0xcc);
    OW_WriteByte(0x8b);//READ_PARAMETERS
		
		for(i=0; i < 15; i++)
    {
		para[i] = OW_ReadByte();
//		uart_printf(" %bx ",para[i]);
	}
			for(i=0; i < 15; i++)
    {
//		para[i] = OW_ReadByte();
		uart_printf(" %bx ",para[i]);
	}

}

int writeCosCfb(unsigned char *Cos,unsigned char *Cfb)
{ 
	unsigned char scrb[sizeof(MDC04_SCRPARAMETERS)];
	MDC04_SCRPARAMETERS *scr = (MDC04_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/	
	if(MDC04_ReadParameters_SkipRom(scrb) == 0)
	{		
		return 0;  /*读寄存器失败*/
	}
	
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/	
//  if(scrb[sizeof(MDC04_SCRPARAMETERS)-1] != MY_OW_CRC8(scrb, sizeof(MDC04_SCRPARAMETERS)-1))
//  {	
//		return FALSE;  /*CRC验证未通过*/
//  }	
//	uart_printf(": %bx  %bx :  ",Cos,Cfb);
	scr->Cos = *Cos ;
	scr->Cfb = *Cfb ;
	

	
	MDC04_WriteParameters_SkipRom(scrb);
	
  return 1;		
}



#endif
