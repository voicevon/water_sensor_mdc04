/****************************************************************************************/
/*
 *
 * Copyright (C) 2020. Mysentech Inc, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Minyuan Sensing Technology Inc.(Mysentech)  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 *  Please contact <sales@mysentech.com> or contributors for further questions.
*/
/****************************************************************************************/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdio.h>
#include <stdint.h>
//#include <assert.h>
#include "MDC04.h"
#include "MY_stdtype.h"

/*
 * GLOBAL DEFINITIONS
 ****************************************************************************************
 */
 
/****全局变量：保存和电容配置寄存器对应的偏置电容和量程电容数值****/
static MY_FLT32 CapCfg_offset = 15.0f, CapCfg_range = 15.5f;

/****偏置电容和反馈电容阵列权系数****/
static const MY_FLT32 COS_Factor[8] = { 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 40.0f };
/*Cos= (40.0*q[7]+32.0*q[6]+16.0*q[5]+8.0*q[4]+4.0*q[3]+2.0*q[2]+1.0*q[1]+0.5*q[0])*/ //对应手册内Cos公式计算

static const struct {
    MY_FLT32 Cfb0; 
    MY_FLT32 Factor[6];
} CFB = { 2.0f, {2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 46.0f} };
/*Cfb =(46*p[5]+32*p[4]+16*p[3]+8*p[2]+4*p[1]+2*p[0]+2)*/ //对应手册内Cfeedback公式计算

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**-----------------------------------------------------------------------
  * @brief  温度测量函数
  * @param  无
  * @retval 以摄氏度为单位的浮点温度
-------------------------------------------------------------------------*/
MY_FLT32 MDC04_StartTempConvert(void)
{ 
	MY_FLT32 fTemp; 
	MY_U16 iTemp; 
	
		MDC04_ConvertTemp();
//*注：此处延时时间与芯片重复性配置有关，默认高重复性即对应10.5ms，考虑不同单片机的电压、温度、时钟等不理想因素存在固预留15ms
//*注：如果实际配置为低重复性，或环境较为理想，可以将此处转换时间缩短	
	MY_DELAY_MS(11);  
		
//	MDC04_ReadTempWaiting(&iTemp);
//	printf("\n iTemp= %4x ",iTemp);
	if (MDC04_ReadTempWaiting(&iTemp)) 
//		return MY_ERR_INVLID_POINTER;

	fTemp=MDC04_OutputtoTemp((int16_t)iTemp); 
	printf("\n Temp= %.3f ",fTemp);
	
	return fTemp;
}
/**-----------------------------------------------------------------------
  * @brief  启动（多个通道）电容测量
  * @param  无
  * @retval I2C发送状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ConvertCap(void)
{	 
	MY_U8 cmd[2]={(MY_U8)(CONVERT_C >>8), (MY_U8)(CONVERT_C & 0xFF)};

	/*主发送16位命令，从以ACK响应命令的最后字节*/
	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK)
	{
		return FALSE;
	}
	
  return TRUE;	 
}
/**-----------------------------------------------------------------------
  * @brief  电容读取函数
  * @param  无
  * @retval 无
-------------------------------------------------------------------------*/
void MDC04_ReadCap(MY_FLT32 *fcap1,MY_FLT32 *fcap2,MY_FLT32 *fcap3,MY_FLT32 *fcap4)
{   
	uint16_t icap[5];  
	MY_U8 status, cfg;

	MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
	MDC04_ReadStatusConfig((MY_U8 *)&status, (uint8_t *)&cfg);
	MDC04_ReadCapC1C2C3C4(icap);

	*fcap1 = MDC04_OutputtoCap(icap[0], CapCfg_offset, CapCfg_range);
	*fcap2 = MDC04_OutputtoCap(icap[1], CapCfg_offset, CapCfg_range);
	*fcap3 = MDC04_OutputtoCap(icap[2], CapCfg_offset, CapCfg_range);
	*fcap4=  MDC04_OutputtoCap(icap[3], CapCfg_offset, CapCfg_range);
}


/**-----------------------------------------------------------------------
  * @brief  配置偏置电容offset 
  * @param  设置期望偏置电容值（中心值：0~103.5pf之间）
  * @retval 标志位
-------------------------------------------------------------------------*/
MY_BOOL MDC04_Set_Cap_Offset(MY_FLT32 Co)
{	
	if(!((Co >= 0.0f) && (Co <= 103.5f))) 
	{
       return MY_ERR_INVLID_RANGE;
	}
	MDC04_CapConfigureOffset(Co);
	MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
	
       return MY_ERR_INVLID_POINTER;
}

/**-----------------------------------------------------------------------
  * @brief  配置量程电容
  * @param  设置期望量程值（0~15.5pf之间）
  * @retval 标志位
-------------------------------------------------------------------------*/
MY_BOOL MDC04_Set_Cap_FullScale(MY_FLT32 Cr)
{ 					
	if(!((Cr >=0.0f) && (Cr <= 15.5))) 		
	{
	   return FALSE;
	}		
	MDC04_CapConfigureFs(Cr);			
	MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);	
		
	   return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置电容转换通道
  * @param  期望工作通道
  * @param  Ch=0/1 通道1 Ch=2 通道2 Ch=3 通道3 Ch=4 通道4 Ch=5 通道1&2 Ch=6 通道1&2&3 Ch=7 通道1&2&3&4，
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
MY_BOOL MDC04_Set_Cap_Channel(int Ch)
{   
	MY_U8 Cap_Cfg; 
	
	MDC04_SetCapChannel(Ch & 0x07);				
	MDC04_GetCapChannel(&Cap_Cfg);
				
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置测量重复性
  * @param  Re:重复性级别
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
MY_BOOL MDC04_SysCfg(unsigned char Re)
{ 
	MY_U8 repeat, syscfg_Read, status;
	
	if(Re == 0x00) {repeat=CFG_Rep_Low;}
	if(Re == 0x01) {repeat=CFG_Rep_Medium;}	
	if(Re == 0x02) {repeat=CFG_Rep_High;}
	MDC04_SetConfig(CFG_MPS_Single, repeat);
	MDC04_ReadStatusConfig(&status, &syscfg_Read);
	
    return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置自动量程
  * @param  COS:待测真实容值（偏置电容值）
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
MY_BOOL MDC04_Set_CapRange_Auto(MY_FLT32 *COS)
{ 
MY_U8 cmd[2]={(MY_U8)(AUTO_CALIBRATION >>8), (MY_U8)(AUTO_CALIBRATION & 0xFF)};

	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) !=GPIOI2C_XFER_LASTNACK)
		return FALSE;
	
	MY_DELAY_MS(500);
	
	MDC04_GetCfg_CapOffset(COS);	

	return TRUE;
}


/**-----------------------------------------------------------------------
  * @brief  保存设置到EEPROM
  * @param  COS:待测真实容值（偏置电容值）
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
MY_BOOL MDC04_Copy_EEPROM(void)
{	
	MY_U8 cmd[2]={(MY_U8)(COPY_PAGE0 >>8), (MY_U8)(COPY_PAGE0 & 0xFF)};	
	bool ret= TRUE;
	
	/*发送写E2PROM命令*/		
	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK)
		ret = FALSE;
	
	/*等待擦除和编程完成*/		
	MY_DELAY_MS(50);	

    return ret;	
}

/*******************Exported Functions*************************************************************/
/**-----------------------------------------------------------------------
  * @brief  把16位二进制补码表示的温度输出转换为以摄氏度为单位的温度读数
  * @param  out：有符号的16位二进制温度输出
  * @retval 以摄氏度为单位的浮点温度
-------------------------------------------------------------------------*/
MY_FLT32 MDC04_OutputtoTemp(int16_t out)
{
	return ((MY_FLT32)out/256.0 + 40.0);	
}

/**-----------------------------------------------------------------------
  * @brief  把以摄氏度为单位的浮点温度值转换为16位二进制补码表示的温度值
  * @param  以摄氏度为单位的浮点温度值
  * @retval 有符号的16位二进制温度值
-------------------------------------------------------------------------*/
MY_INT16 MDC04_TemptoOutput(MY_FLT32 Temp)
{
	return (int16_t)((Temp-40.0)*256.0);	
}

/**-----------------------------------------------------------------------
  * @brief  把16位二进制电容输出转换为以pF为单位的电容读数
  * @param  out：无符号的16位二进制电容输出
  * @param  Co：配置的偏置电容数值
  * @param  Cr：配置的范围电容（量程）数值
  * @retval 以pF为单位的浮点电容数值
-------------------------------------------------------------------------*/
MY_FLT32 MDC04_OutputtoCap(uint16_t out, MY_FLT32 Co, MY_FLT32 Cr)
{
	return (2.0*(out/65535.0-0.5)*Cr+Co);	
}

/**-----------------------------------------------------------------------
  * @brief  计算多个字节序列的校验和
  * @param  data：字节数组指针
  * @param  nbrOfBytes：字节数组的长度
  * @retval 校验和（CRC）
-------------------------------------------------------------------------*/
#define POLYNOMIAL 	0x131 //100110001
MY_U8 MY_I2C_CRC8(MY_U8 data[], MY_U8 nbrOfBytes)
{
  MY_U8 bit;        // bit mask
  MY_U8 crc = 0xFF; // calculated checksum
  MY_U8 byteCtr;    // byte counter
  
  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)
  {
    crc ^= (data[byteCtr]);
    for(bit = 8; bit > 0; --bit)
    {
      if(crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
      else           crc = (crc << 1);
    }
  }
  
  return crc;
}

/**-----------------------------------------------------------------------
  * @brief  向芯片内部寄存器的单个字节写入数值
  * @param  Reg：寄存器地址
  * @param  Value：要写入的数值
  * @retval 写入成功，返回真，否则，返回假。
-------------------------------------------------------------------------*/
MY_BOOL MDC04_SingleByteWrite(MY_U8 reg, MY_U8 value)
{ 
	MY_U8 scr_wr[3];
	
	scr_wr[0] = value;	
	scr_wr[1]	=	0xFF; 
	scr_wr[2]	=	MY_I2C_CRC8(scr_wr,2);
	
	//GPIOI2C_Cmd_Write(MDC04_I2C_ADDR , WRITE_ONE_BYTE | reg, scr_wr, 3); 
  MY_TRANSPORT_WRITE(MDC04_I2C_ADDR , WRITE_ONE_BYTE | reg, scr_wr, 3);
	
	return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  读出芯片内部寄存器的单个字节
  * @param  Reg：寄存器地址
  * @param  Value：读出字节数值指针
  * @retval 读出成功，返回真，否则，返回假。
-------------------------------------------------------------------------*/
MY_BOOL MDC04_SingleByteRead(MY_U8 reg, MY_U8 *value)
{ 
	MY_U8 scr_rd[3];
	
	MY_TRANSPORT_READ(MDC04_I2C_ADDR, READ_ONE_BYTE | reg, scr_rd, 3);
	
	if(scr_rd[2] !=	MY_I2C_CRC8(scr_rd,2))  /*检查校验和是否匹配*/
		return FALSE;
	
	*value= scr_rd[0];
	
	return TRUE;
}


/**-----------------------------------------------------------------------
  * @brief  启动温度测量
  * @param  无
  * @retval I2C发送状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ConvertTemp(void)
{	 
	MY_U8 cmd[2]={(MY_U8)(CONVERT_T >>8), (MY_U8)(CONVERT_T & 0xFF)};
	/*主发送16位命令，从以ACK响应命令的最后字节*/
//	MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2);
	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2)!= GPIOI2C_XFER_LASTACK)
	{


		return FALSE;
	}	

  return TRUE;	 
}



/**-----------------------------------------------------------------------
  * @brief  等待转换结束后读测量结果。和@ConvertTemp联合使用
  * @param  iTemp：返回的16位温度测量结果
  * @retval I2C接收状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ReadTempWaiting(uint16_t *iTemp)
{
	MY_U8 data[3];

	/*读3个字节。前两个是温度转换结果，最后字节是前两个的校验和--CRC。*/	
	if(MY_TRANSPORT_DATA_READ(MDC04_I2C_ADDR, data, 3) != GPIOI2C_XFER_LASTNACK)
	{	
		return FALSE;  /*I2C地址头应答为NACK*/
	}

	/*计算接收的前两个字节的校验和，并与接收的第3个CRC字节比较。*/	
  if(data[2] != MY_I2C_CRC8(data, 2))
  {	
		return FALSE;  /*CRC验证失败*/
  }
			
	*iTemp=(uint16_t)data[0]<<8 | data[1];

  return TRUE;		
}

/**-----------------------------------------------------------------------
  * @brief  查询是否转换结束，然后读测量结果。和@ConvertTemp联合使用
  * @param  iTemp：返回的16温度测量结果
  * @retval 读结果状态
-------------------------------------------------------------------------*/
MY_BOOL ReadTempPolling(uint16_t *iTemp)
{ 
	int timeout=0; MY_U8 data[3];

	MY_DELAY_MS(1);   /*minimum waiting time of 1ms. after convert*/
	
	/*尝试读3个字节。如果转换还没结束，芯片对地址头的应答为NACK。如果转换结束，应答为ACK。
	前两个字节是温度转换结果，最后字节是前两个的校验和CRC。*/		
	while (MY_TRANSPORT_DATA_READ(MDC04_I2C_ADDR, data, 3) == GPIOI2C_XFER_ADDRNACK)
	{	
		MY_DELAY_MS(1);	
    timeout++; 
		if(timeout > 50) 
		{				
			return FALSE;				/*超时错误*/
		}
	}
	/*计算接收的前两个字节的校验和，并与接收的第3个字节比较。*/		
  if(data[2] != MY_I2C_CRC8(data,2))
  {	
		return FALSE;
  }
			
	*iTemp=(uint16_t)data[0]<<8 | data[1];

  return TRUE;		
}


/**-----------------------------------------------------------------------
  * @brief  查询是否转换结束，然后读测量结果。和 @ConvertTC1联合使用
  * @param  iTemp：返回的16温度测量结果
  * @param  iCap1：返回的16电容1测量结果
  * @retval 读结果状态
-------------------------------------------------------------------------*/
MY_BOOL ReadTempCap1Polling(uint16_t *iTemp, uint16_t *iCap1)
{ int timeout=0; MY_U8 data[6];

	MY_DELAY_MS(1);   /*minimum waiting time of 1ms. after convert*/
	
	/*尝试读3个字节。如果转换还没结束，芯片对地址头的应答为NACK。如果转换结束，应答为ACK。
	前两个字节是温度转换结果，最后字节是前两个的校验和CRC。*/		
	while (MY_TRANSPORT_DATA_READ(MDC04_I2C_ADDR, data, 6) == GPIOI2C_XFER_ADDRNACK)
	{	
		MY_DELAY_MS(1);	
    timeout++; 
		if(timeout > 50) 
		{				
			return FALSE;				/*超时错误*/
		}
	}

	/*计算接收的温度字节的校验和，并与接收的第3个字节比较。*/	
  if(data[2] != MY_I2C_CRC8(data,2))
  {	
		return FALSE;
  }
	
	/*计算接收的电容1字节的校验和，并与接收的第6个字节比较。*/			
	*iTemp=(uint16_t)data[0]<<8 | data[1];
	
  if(data[5] != MY_I2C_CRC8(&data[3],2))
  {	
		return FALSE;
  }
			
	*iCap1=(uint16_t)data[3]<<8 | data[4];	

  return TRUE;		
}

/**-----------------------------------------------------------------------
  * @brief  读指定电容通道测量结果。和 @ConvertCap联合使用
  * @param  ch：电容通道
  * @param  icap：16位电容转换结果
  * @retval 读结果状态
-------------------------------------------------------------------------*/
MY_BOOL ReadCap(MY_U8 ch, uint16_t *icap)
{
    MY_U8 Cap_lsb, Cap_msb, scr_cap_lsb, scr_cap_msb;	
	
	switch(ch)
	{
		case 2:
			scr_cap_lsb = SCR_cap2_lsb; 
			scr_cap_msb = SCR_cap2_msb;
			break;		
		case 3:
			scr_cap_lsb = SCR_cap3_lsb; 
			scr_cap_msb = SCR_cap3_msb; 
			break;			
		case 4:
			scr_cap_lsb = SCR_cap4_lsb; 
			scr_cap_msb = SCR_cap4_msb; 
			break;
		case 1:				
		default:
			scr_cap_lsb = SCR_cap1_lsb; 
			scr_cap_msb = SCR_cap1_msb; 				
			break;
	}

	MDC04_SingleByteRead(scr_cap_lsb, &Cap_lsb);	
	MDC04_SingleByteRead(scr_cap_msb, &Cap_msb);	

	*icap = (Cap_msb <<8) | Cap_lsb;
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  读电容转换结果
  * @param  icap：四通道电容值
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ReadCapC1C2C3C4(MY_U16 *icap)
{
//	  MY_ASSERT(icap != NULL);
	
	  ReadCap(1, icap++); 
	  ReadCap(2, icap++); 
	  ReadCap(3, icap++); 
	  ReadCap(4, icap);
	
	  return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读偏置电容配置寄存器内容和有效位宽设置
  * @param  Coffset：偏置配置寄存器有效位的内容
  * @param  Cosbits：偏置配置寄存器有效位宽
  * @retval 无
-------------------------------------------------------------------------*/
static bool ReadCosConfig(MY_U8 *Coscfg)
{ 
	MY_U8 reg_cos, reg_cfb;
		
	MDC04_SingleByteRead(SCR_REGADDR_COS, &reg_cos);	
	MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb);
	
	*Coscfg = reg_cos & (0xFF >> (3 - (reg_cfb >> 6)));
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  写偏置电容配置寄存器和有效位宽设置
  * @param  Coffset：偏置配置寄存器的数值
  * @param  Cosbits：偏置配置寄存器有效位宽，可能为：
	*		@COS_RANGE_5BIT				  	
	*		@COS_RANGE_6BIT				  	
	*		@COS_RANGE_7BIT				  		
	*		@COS_RANGE_8BIT				  	
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL WriteCosConfig(MY_U8 Coffset, MY_U8 Cosbits)
{ 
	MY_U8 reg_cfb;
	
	MDC04_SingleByteWrite(SCR_REGADDR_COS, Coffset);			

	MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb);	
	reg_cfb = (reg_cfb & ~CFB_COSRANGE_Mask) | Cosbits;	
	MDC04_SingleByteWrite(SCR_REGADDR_CFB, reg_cfb);
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  读量程电容配置寄存器内容
  * @param  Cfb：量程配置寄存器低6位的内容
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL ReadCfbConfig(MY_U8 *cfb)
{ 
	MY_U8 reg_cfb;
			
	MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb);	
	*cfb = (reg_cfb & CFB_CFBSEL_Mask);
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  写量程电容配置寄存器
  * @param  Cfb：量程配置寄存器低6位的内容
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL WriteCfbConfig(MY_U8 value)
{ 
	MY_U8 reg_cfb;

	MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb);
	
	reg_cfb = (reg_cfb & ~CFB_CFBSEL_Mask) | (value & CFB_CFBSEL_Mask);
	
	MDC04_SingleByteWrite(SCR_REGADDR_CFB, reg_cfb);	
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  读电容转换通道选择
  * @param  chann：通道选择寄存器Ch_Sel低3位的内容，可能为：
		CCS_CapChannel_Cap1							
		CCS_CapChannel_Cap2					
		CCS_CapChannel_Cap3				
		CCS_CapChannel_Cap4					
		CCS_CapChannel_Cap1_2				
		CCS_CapChannel_Cap1_2_3			
		CCS_CapChannel_Cap1_2_3_4		
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_GetCapChannel(MY_U8 *chann)
{ 
	MY_U8 reg_ChSel;

	MDC04_SingleByteRead(SCR_REGADDR_ChSel, &reg_ChSel);
	
	*chann = reg_ChSel & CCS_CHANNEL_Mask;	
	
	return TRUE;
}
/**-----------------------------------------------------------------------
  * @brief  写电容转换通道选择
  * @param  chann：通道选择寄存器Ch_Sel低3位的内容，可能为：
		CCS_CapChannel_Cap1							
		CCS_CapChannel_Cap2					
		CCS_CapChannel_Cap3				
		CCS_CapChannel_Cap4					
		CCS_CapChannel_Cap1_2				
		CCS_CapChannel_Cap1_2_3			
		CCS_CapChannel_Cap1_2_3_4		
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_SetCapChannel(MY_U8 chann)
{ 
	MY_U8 reg_ChSel;

	MDC04_SingleByteRead(SCR_REGADDR_ChSel, &reg_ChSel);
	
	reg_ChSel = (reg_ChSel & ~CCS_CHANNEL_Mask) | (chann & CCS_CHANNEL_Mask);
	
	MDC04_SingleByteWrite(SCR_REGADDR_ChSel, reg_ChSel);	
	
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置周期测量频率和重复性
  * @param  mps 要设置的周期测量频率（每秒测量次数），可能为下列其一
	*				@arg CFG_MPS_Single		：每执行ConvertTemp一次，启动一次温度测量
	*				@arg CFG_MPS_Half			：每执行ConvertTemp一次，启动每秒0.5次重复测量
	*				@arg CFG_MPS_1				：每执行ConvertTemp一次，启动每秒1次重复测量
	*				@arg CFG_MPS_2				：每执行ConvertTemp一次，启动每秒2次重复测量
	*				@arg CFG_MPS_4				：每执行ConvertTemp一次，启动每秒4次重复测量
	*				@arg CFG_MPS_10				：每执行ConvertTemp一次，启动每秒10次重复测量
  * @param  repeatability：要设置的重复性值，可能为下列其一
	*				@arg CFG_Repeatbility_Low				：设置低重复性
	*				@arg CFG_Repeatbility_Medium		：设置中重复性
	*				@arg CFG_Repeatbility_High			：设置高重复性
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_SetConfig(MY_U8 mps, MY_U8 repeatability)
{ 
	MY_U8 scr_rd[3], scr_wr[3];
	
	MY_TRANSPORT_READ(MDC04_I2C_ADDR, READ_STATUSCONFIG, scr_rd, 3);
		
	scr_wr[0] = repeatability;	
	scr_wr[1] = 0xFF; 
	scr_wr[2]=MY_I2C_CRC8(scr_wr, 2);
	
	MY_TRANSPORT_WRITE(MDC04_I2C_ADDR, WRITE_CONFIG, scr_wr, 3);	
	
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读状态和配置
  * @param  status 返回的状态寄存器值
  * @param  cfg 返回的配置寄存器值
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ReadStatusConfig(MY_U8 *status, MY_U8 *cfg)
{ 
	MY_U8 scr_rd[3];
	
	/*读3个字节，第一字节是状态寄存器的值，第二字节是配置寄存器的值，后跟这两个字节的校验和。*/	
	MY_TRANSPORT_READ(MDC04_I2C_ADDR, READ_STATUSCONFIG, scr_rd, 3);		
	*status = scr_rd[0];
	*cfg = scr_rd[1];	
	
  return TRUE;		
}

/**-----------------------------------------------------------------------
  * @brief  将偏置电容数值（pF）转换为对应的偏置电容配置
  * @param  osCap：偏置电容的数值
  * @retval 对应偏置配置寄存器的数值
-------------------------------------------------------------------------*/
MY_U8 CaptoCoscfg(MY_FLT32 osCap)
{
	int i; MY_U8 CosCfg = 0x00;
	
	for(i = 7; i >= 0; i--)
	{
		if(osCap >= COS_Factor[i])
		{
			CosCfg |= (0x01 << i);
			osCap -= COS_Factor[i];
		}
	}
	
	return CosCfg;	
}

/**-----------------------------------------------------------------------
  * @brief  将偏置电容配置转换为对应的偏置电容数值（pF）
  * @param  osCfg：偏置电容配置
  * @retval 对应偏置电容的数值
-------------------------------------------------------------------------*/
MY_FLT32 CoscfgtoCapOffset(MY_U8 osCfg)
{
	MY_U8 i; 
	MY_FLT32 Coffset = 0.0; 

	for(i = 0; i < 8; i++) 
	{
		if(osCfg & 0x01) Coffset += COS_Factor[i];
		osCfg >>= 1;
	}
	
	return Coffset;
}

/**-----------------------------------------------------------------------
  * @brief  将量程电容数值（pF）转换为对应的量程电容配置
  * @param  fsCap：量程电容的数值
  * @retval 对应量程配置的数值
-------------------------------------------------------------------------*/
MY_U8 CapRangetocfbCfg(MY_FLT32 fsCap)
{
	int8_t i; MY_U8 CfbCfg = 0x00;
	
	fsCap = fsCap * (3.6/0.507);
	
	fsCap -= CFB.Cfb0;
	
	for(i = 5; i >= 0; i--)
	{
		if(fsCap >= CFB.Factor[i])
		{
			fsCap -= CFB.Factor[i];
			CfbCfg |= (0x01 << i);
		}			
	}
	
	return CfbCfg;	
}

/**-----------------------------------------------------------------------
  * @brief  将量程电容配置转换为对应的量程电容数值（pF）
  * @param  fbCfg：量程电容配置
  * @retval 对应量程电容的数值
-------------------------------------------------------------------------*/
MY_FLT32 CfbcfgtoCapRange(MY_U8 fbCfg)
{
	MY_U8 i;	
	MY_FLT32 Crange = CFB.Cfb0;
	
	for(i = 0; i <= 5; i++)
	{
		if(fbCfg & 0x01) Crange += CFB.Factor[i];
		fbCfg >>= 1;
	}

	return (0.507/3.6) * Crange;	
}

/**-----------------------------------------------------------------------
  * @brief  获取配置的偏置电容数值（pF）
  * @param  Coffset：偏置电容配置
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_GetCfg_CapOffset(MY_FLT32 *Coffset)
{    
	MY_U8 Cos_cfg;	
	ReadCosConfig(&Cos_cfg);
	*Coffset = CoscfgtoCapOffset(Cos_cfg);	
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  获取配置的量程电容数值（pF）
  * @param  Crange：返回量程电容数值
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_GetCfg_CapRange(MY_FLT32 *Crange)
{
	MY_U8 Cfb_cfg;
	
	ReadCfbConfig(&Cfb_cfg);
	*Crange = CfbcfgtoCapRange(Cfb_cfg);
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  配置偏置电容
  * @param  Coffset：要配置的偏置电容数值。范围0~103.5 pF。
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_CapConfigureOffset(MY_FLT32 Coffset)
{ 
	MY_U8 CosCfg, Cosbits;
		
	CosCfg = CaptoCoscfg(Coffset + 0.25);	
	
	if(!(CosCfg & ~0x1F)) Cosbits = COS_RANGE_5BIT;
	else if(!(CosCfg & ~0x3F)) Cosbits = COS_RANGE_6BIT;
			 else if(!(CosCfg & ~0x7F)) Cosbits = COS_RANGE_7BIT;
						else Cosbits = COS_RANGE_8BIT;

	WriteCosConfig(CosCfg, Cosbits); 
		
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  配置量程电容
  * @param  Cfs：要配置的量程电容数值。范围+/-（0.281~15.49） pF。
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_CapConfigureFs(MY_FLT32 Cfs)
{ 
	MY_U8 Cfbcfg;
	
	Cfs = (Cfs + 0.1408);		
	Cfbcfg = CapRangetocfbCfg(Cfs);	
	
	WriteCfbConfig(Cfbcfg);

	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  配置电容测量范围
  * @param  Cmin：要配置测量范围的低端。
  * @param  Cmax：要配置测量范围的高端。
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_CapConfigureRange(MY_FLT32 Cmin, MY_FLT32 Cmax)
{ 
	MY_FLT32 Cfs, Cos;
		
	if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && ((Cmax-Cmin) <= 31.0)))  
	return FALSE;	//The input value is out of range.
	
	Cos = (Cmin + Cmax)/2.0;
	Cfs = (Cmax - Cmin)/2.0;	
	
	MDC04_CapConfigureOffset(Cos);	
	MDC04_CapConfigureFs(Cfs);

	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  自动配合偏置电容
  * @param  Coffset，自动配置完成后返回偏置电容数值
  * @retval 状态
-------------------------------------------------------------------------*/
MY_BOOL MDC04_CapConfigureAuto(MY_FLT32 *Coffset)
{
	MY_U8 cmd[2]={(MY_U8)(AUTO_CALIBRATION >>8), (MY_U8)(AUTO_CALIBRATION & 0xFF)};
	MY_U8 Cos_cfg;	

	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) !=GPIOI2C_XFER_LASTNACK)
	
	return FALSE;
	
	MY_DELAY_MS(500);
	
	ReadCosConfig(&Cos_cfg);
	*Coffset = CoscfgtoCapOffset(Cos_cfg);	
	
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读电容配置
  * @param  Coffset：配置的偏置电容。
  * @param  Crange：配置的量程电容。
  * @retval 无
-------------------------------------------------------------------------*/
MY_BOOL MDC04_ReadCapConfigure(MY_FLT32 *Coffset, MY_FLT32 *Crange)
{ 
	MDC04_GetCfg_CapOffset(Coffset);
	MDC04_GetCfg_CapRange(Crange);
	
	return TRUE;
}


