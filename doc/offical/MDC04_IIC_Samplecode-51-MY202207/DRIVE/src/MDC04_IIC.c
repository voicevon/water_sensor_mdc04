#ifndef _MDC04_C_
#define _MDC04_C_

//#include "reg51.h"
#include "ca51f003_config.h"
#include "IIC.h"
#include "MDC04_IIC.h"
#include "delay.h"
#include "uart.h"

//#define u16 unsigned short
  
/****全局变量：保存和电容配置寄存器对应的偏置电容和量程电容数值****/
float CapCfg_offset, CapCfg_range;
float fcap1,fcap2,fcap3,fcap4;
/****偏置电容和反馈电容阵列权系数****/
static const float COS_Factor[8] = {0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 40.0};
/*Cos= (40.0*q[7]+32.0*q[6]+16.0*q[5]+8.0*q[4]+4.0*q[3]+2.0*q[2]+1.0*q[1]+0.5*q[0])*/
static const struct  {float Cfb0; float Factor[6];} CFB = { 2.0, 2.0, 4.0, 8.0, 16.0, 32.0, 46.0};
/*Cfb =(46*p[5]+32*p[4]+16*p[3]+8*p[2]+4*p[1]+2*p[0]+2)*/
typedef enum {
    FALSE = 0,
    TRUE = !FALSE
} bool;
float MDC04_OutputtoCap(unsigned short out, float Co, float Cr)
{
	return (2.0*(out/65535.0-0.5)*Cr+Co);	
}


/**-----------------------------------------------------------------------
  * @brief  启动（多个通道）电容测量
  * @param  无
  * @retval I2C发送状态
-------------------------------------------------------------------------*/
int MDC04_ConvertCap(void)
{	 
	unsigned char cmd[2]={(unsigned char)(CONVERT_C >>8), (unsigned char)(CONVERT_C & 0xFF)};

	/*主发送16位命令，从以ACK响应命令的最后字节*/
	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK)
	{
		return FALSE;
	}
	
  return TRUE;	 
}

/**-----------------------------------------------------------------------
  * @brief  向芯片内部寄存器的单个字节写入数值
  * @param  Reg：寄存器地址
  * @param  Value：要写入的数值
  * @retval 写入成功，返回真，否则，返回假。
-------------------------------------------------------------------------*/
int MDC04_SingleByteWrite(unsigned char reg, unsigned char value)
{ 
	unsigned char scr_wr[3];
	
	scr_wr[0] = value;	
	scr_wr[1]	=	0xFF; 
// 	scr_wr[2]	=	MY_I2C_CRC8(scr_wr,2);

  MY_TRANSPORT_WRITE(MDC04_I2C_ADDR , WRITE_ONE_BYTE | reg, scr_wr, 3);
	
	return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  读出芯片内部寄存器的单个字节
  * @param  Reg：寄存器地址
  * @param  Value：读出字节数值指针
  * @retval 读出成功，返回真，否则，返回假。
-------------------------------------------------------------------------*/
int MDC04_SingleByteRead(unsigned char reg, unsigned char *value)
{ 
	unsigned char scr_rd[3];
	
	MY_TRANSPORT_READ(MDC04_I2C_ADDR, READ_ONE_BYTE | reg, scr_rd, 3);
	
// 	if(scr_rd[2] !=	MY_I2C_CRC8(scr_rd,2))  /*检查校验和是否匹配*/
// 		return FALSE;
	
	*value= scr_rd[0];
	
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  电容测量函数
  * @param  无
  * @retval 无
-------------------------------------------------------------------------*/
void MDC04_ReadCap(float *fcap1,float *fcap2,float *fcap3,float *fcap4)
{   
	unsigned short icap_u16[4]={0};  
	unsigned char status, cfg;

	MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
	MDC04_ReadStatusConfig((unsigned char *)&status, (unsigned char *)&cfg);
	MDC04_ReadCapC1C2C3C4(icap_u16);

	
	*fcap1 = MDC04_OutputtoCap(icap_u16[0], CapCfg_offset, CapCfg_range);
	*fcap2 = MDC04_OutputtoCap(icap_u16[1], CapCfg_offset, CapCfg_range);
	*fcap3 = MDC04_OutputtoCap(icap_u16[2], CapCfg_offset, CapCfg_range);
	*fcap4 = MDC04_OutputtoCap(icap_u16[3], CapCfg_offset, CapCfg_range);
//	uart_printf("CapCfg_offset = %.3f,CapCfg_range = %.3f \r\n",CapCfg_offset,CapCfg_range);
//	uart_printf("icap_u1=%x,icap_u2=%x,icap_u3=%x,icap_u4=%x\n",icap_u16[0],icap_u16[1],icap_u16[2],icap_u16[3]);
//	uart_printf("fcap4 = %.3f   ",*fcap4);
//	uart_printf("cap1=%.3f,cap2=%.3f,cap3=%.3f,cap4=%.3f\n",*fcap1,*fcap2,*fcap3,*fcap4);
//	uart_printf("cap1=%.3f,cap2=%.3f,",*fcap1,*fcap2);
//	uart_printf("cap3=%.3f,cap4=%.3f\n",*fcap3,*fcap4);
}

/**-----------------------------------------------------------------------
  * @brief  读电容转换结果
  * @param  icap：四通道电容值
  * @retval 无
-------------------------------------------------------------------------*/
int MDC04_ReadCapC1C2C3C4(unsigned short *icap)
{
	
	  ReadCap(1, icap++); 
	  ReadCap(2, icap++); 
	  ReadCap(3, icap++); 
	  ReadCap(4, icap);
	
	  return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读指定电容通道测量结果。和 @ConvertCap联合使用
  * @param  ch：电容通道
  * @param  icap：16位电容转换结果
  * @retval 读结果状态
-------------------------------------------------------------------------*/
int ReadCap(unsigned char ch, unsigned short *icap)
{
   unsigned char Cap_lsb, Cap_msb, scr_cap_lsb, scr_cap_msb;	
	
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
  * @brief  配置偏置电容offset 
  * @param  设置期望偏置电容值（中心值：0~103.5pf之间）
  * @retval 标志位
-------------------------------------------------------------------------*/
uchar MDC04_Set_Cap_Offset(float Co)
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
int MDC04_Set_Cap_FullScale(float Cr)
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
  * @brief  配置电容测量范围
  * @param  Cmin:可测量最小值
  * @param  Cmax:可测量最大值
  * @retval 标志位
-------------------------------------------------------------------------*/
int MDC04_Set_Cap_Range(float Cmin,float Cmax)
{ 					
	if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && ((Cmax-Cmin) <= 31.0)))  
		{
		return FALSE;
		}	
		MDC04_CapConfigureRange(Cmin, Cmax);			
		MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
		
		return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置电容转换通道
  * @param  期望工作通道
  * @param  Ch=0/1 通道1 Ch=2 通道2 Ch=3 通道3 Ch=4 通道4 Ch=5 通道1&2 Ch=6 通道1&2&3 Ch=7 通道1&2&3&4，
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
int MDC04_Set_Cap_Channel(int Ch)
{   
	unsigned char Cap_Cfg; 
	
	MDC04_SetCapChannel(Ch & 0x07);				
	MDC04_GetCapChannel(&Cap_Cfg);
				
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置测量重复性
  * @param  Re:重复性级别
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
int MDC04_SysCfg(unsigned char Re)
{ 
	unsigned char repeat, syscfg_Read, status;
	
	if(Re == 1) {repeat=CFG_Rep_Low;}
	if(Re == 2) {repeat=CFG_Rep_Medium;}	
	if(Re == 3) {repeat=CFG_Rep_High;}
	MDC04_SetConfig(CFG_MPS_Single, repeat);
	MDC04_ReadStatusConfig(&status, &syscfg_Read);
	
    return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  设置自动量程
  * @param  COS:待测真实容值（偏置电容值）
  * @retval 标志位(1:执行完毕)
-------------------------------------------------------------------------*/
int MDC04_Set_CapRange_Auto(float *COS)
{ 
  unsigned char cmd[2]={(unsigned char)(AUTO_CALIBRATION >>8), (unsigned char)(AUTO_CALIBRATION & 0xFF)};

	if(MY_TRANSPORT_DATA_WRITE(MDC04_I2C_ADDR, cmd, 2) !=GPIOI2C_XFER_LASTNACK)
		return FALSE;
	
	MY_DELAY_MS(500);
	
	MDC04_GetCfg_CapOffset(COS);	

	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读偏置电容配置寄存器内容和有效位宽设置
  * @param  Coffset：偏置配置寄存器有效位的内容
  * @param  Cosbits：偏置配置寄存器有效位宽
  * @retval 无
-------------------------------------------------------------------------*/
int ReadCosConfig(unsigned char *Coscfg)
{ 
	unsigned char reg_cos, reg_cfb;
		
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
int WriteCosConfig(unsigned char Coffset, unsigned char Cosbits)
{ 
	unsigned char reg_cfb;
	
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
int ReadCfbConfig(unsigned char *cfb)
{ unsigned char reg_cfb;
			
	MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb);	
	*cfb = (reg_cfb & CFB_CFBSEL_Mask);
	
  return TRUE;	
}
/**-----------------------------------------------------------------------
  * @brief  写量程电容配置寄存器
  * @param  Cfb：量程配置寄存器低6位的内容
  * @retval 状态
-------------------------------------------------------------------------*/
int WriteCfbConfig(unsigned char value)
{ 
	unsigned char reg_cfb;

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
int MDC04_GetCapChannel(unsigned char *chann)
{ 
	unsigned char reg_ChSel;

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
int MDC04_SetCapChannel(unsigned char chann)
{ 
	unsigned char reg_ChSel;

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
int MDC04_SetConfig(unsigned char mps, unsigned char repeatability)
{ 
	unsigned char scr_rd[3], scr_wr[3];
	
	MY_TRANSPORT_READ(MDC04_I2C_ADDR, READ_STATUSCONFIG, scr_rd, 3);
		
	scr_wr[0] = repeatability;	
	scr_wr[1] = 0xFF; 
// 	scr_wr[2]=MY_I2C_CRC8(scr_wr, 2);
	
	MY_TRANSPORT_WRITE(MDC04_I2C_ADDR, WRITE_CONFIG, scr_wr, 3);	
	
	return TRUE;
}

/**-----------------------------------------------------------------------
  * @brief  读状态和配置
  * @param  status 返回的状态寄存器值
  * @param  cfg 返回的配置寄存器值
  * @retval 状态
-------------------------------------------------------------------------*/
int MDC04_ReadStatusConfig(unsigned char *status, unsigned char *cfg)
{ 
	unsigned char scr_rd[3];
	
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
unsigned char CaptoCoscfg(float osCap)
{
	int i; unsigned char CosCfg = 0x00;
	
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
float CoscfgtoCapOffset(unsigned char osCfg)
{
	unsigned char i; 
	float Coffset = 0.0; 

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
unsigned char CapRangetocfbCfg(float fsCap)
{
	int i; unsigned char CfbCfg = 0x00;
	
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
float CfbcfgtoCapRange(unsigned char fbCfg)
{
	unsigned char i;	
	float Crange = CFB.Cfb0;
	
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
int MDC04_GetCfg_CapOffset(float *Coffset)
{    
	unsigned char Cos_cfg;	
	ReadCosConfig(&Cos_cfg);
	*Coffset = CoscfgtoCapOffset(Cos_cfg);	
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  获取配置的量程电容数值（pF）
  * @param  Crange：返回量程电容数值
  * @retval 无
-------------------------------------------------------------------------*/
int MDC04_GetCfg_CapRange(float *Crange)
{
	unsigned char Cfb_cfg;
	
	ReadCfbConfig(&Cfb_cfg);
	*Crange = CfbcfgtoCapRange(Cfb_cfg);
	
  return TRUE;	
}

/**-----------------------------------------------------------------------
  * @brief  配置偏置电容
  * @param  Coffset：要配置的偏置电容数值。范围0~103.5 pF。
  * @retval 状态
-------------------------------------------------------------------------*/
int MDC04_CapConfigureOffset(float Coffset)
{ 
	unsigned char CosCfg, Cosbits;
		
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
int MDC04_CapConfigureFs(float Cfs)
{ 
	unsigned char Cfbcfg;
	
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
int MDC04_CapConfigureRange(float Cmin, float Cmax)
{ 
	float Cfs, Cos;
		
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
int MDC04_CapConfigureAuto(float *Coffset)
{
	unsigned char cmd[2]={(unsigned char)(AUTO_CALIBRATION >>8), (unsigned char)(AUTO_CALIBRATION & 0xFF)};
	unsigned char Cos_cfg;	

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
int MDC04_ReadCapConfigure(float *Coffset, float *Crange)
{ 
	MDC04_GetCfg_CapOffset(Coffset);
	MDC04_GetCfg_CapRange(Crange);
	
	return TRUE;
}
