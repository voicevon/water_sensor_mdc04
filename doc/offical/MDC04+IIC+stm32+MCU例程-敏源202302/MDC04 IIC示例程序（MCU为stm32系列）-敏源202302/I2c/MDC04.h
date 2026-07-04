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

#ifndef _MDC04_H_
#define _MDC04_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "MY_stdtype.h"

#define MDC04_I2C_ADDR 		0x44  /* Addr 引脚接低电平44/高电平45*/

//***********需要根据用户的延时函数进行相应替换*********//
#include "bsp_SysTick.h"
#define MY_DELAY_MS Delay_ms
#define MY_DELAY_US Delay_us
#ifndef MY_DELAY_MS
#error "please redefine MY_DELAY_MS as you own platform delay function!"
#endif

//#include "customer.h"
#include "MY_IIC_DRV.h"
#define MY_TRANSPORT_WRITE      GPIOI2C_Cmd_Write
#define MY_TRANSPORT_READ       GPIOI2C_Cmd_Read
#define MY_TRANSPORT_DATA_WRITE GPIOI2C_Transmit
#define MY_TRANSPORT_DATA_READ  GPIOI2C_Receive

//******************************************************//

/* MDC04/01 Registers definition----------------------------------------------*/
/*Bit definition of config register*/
#define CFG_CLKSTRETCH_Mask   	(0x20)
#define CFG_MPS_Mask   		  	(0x1C)
#define CFG_Repeatbility_Mask 	(0x03)

#define CFG_MPS_Single  	(0x00)
#define CFG_MPS_Half  		(0x04)
#define CFG_MPS_1  			(0x08)
#define CFG_MPS_2 			(0x0C)
#define CFG_MPS_4 			(0x10)
#define CFG_MPS_10 			(0x14)

#define CFG_Rep_Low   	(0x00)
#define CFG_Rep_Medium  (0x01)
#define CFG_Rep_High 	(0x02)

#define CFG_ClkStreatch_Disable (0x00 << 5)
#define CFG_ClkStreatch_Enable 	(0x01 << 5)
/*Bit definition of status register*/
#define Status_Meature_Mask   	    (0x81)
#define Status_WriteCrc_Mask   	    (0x20)
#define Status_CMD_Mask   			(0x10)
#define Status_POR_Mask   			(0x08)

/*Bit definition of CFB register*/
#define CFB_COSRANGE_Mask   		(0xC0)
#define CFB_CFBSEL_Mask   			(0x3F)

#define CFB_COS_BITRANGE_5  		(0x1F)
#define CFB_COS_BITRANGE_6  		(0x3F)
#define CFB_COS_BITRANGE_7  		(0x7F)
#define CFB_COS_BITRANGE_8  		(0xFF)

#define COS_RANGE_5BIT				  		(0x00)
#define COS_RANGE_6BIT				  		(0x40)
#define COS_RANGE_7BIT				  		(0x80)
#define COS_RANGE_8BIT				  		(0xC0)
/*Bit definition of Ch_Sel register*/
#define CCS_CHANNEL_Mask   					(0x07)

#define CCS_CapChannel_Cap1					(0x01) 		
#define CCS_CapChannel_Cap2					(0x02)
#define CCS_CapChannel_Cap3					(0x03)
#define CCS_CapChannel_Cap4					(0x04)
#define CCS_CapChannel_Cap1_2				(0x05)
#define CCS_CapChannel_Cap1_2_3			    (0x06)
#define CCS_CapChannel_Cap1_2_3_4		    (0x07)

#define CAP_CH1_SEL  								(0x01)
#define CAP_CH2_SEL  								(0x02)
#define CAP_CH3_SEL  								(0x03)
#define CAP_CH4_SEL  								(0x04)
#define CAP_CH1CH2_SEL  						(0x05)
#define CAP_CH1CH2CH3_SEL  					(0x06)
#define CAP_CH1CH2CH3CH4_SEL  			(0x07)

/******************  Bit definition for MDC04 configuration register  ******************/
#define MDC04_CFG_REPEATABILITY_MASK          (0x03)
#define MDC04_CFG_MPS_MASK					  (0x1C)
#define MDC04_CFG_I2CCLKSTRETCH_MASK          (0x20)
/******************  Bit definition for MDC04 temperature register  *******/
#define MDC04_REPEATABILITY_LOW               (0x00 << 0)
#define MDC04_REPEATABILITY_MEDIUM            (0x01 << 0)
#define MDC04_REPEATABILITY_HIGH              (0x02 << 0)
/******************  Bit definition for TTrim in parameters  *******/
#define MDC04_MPS_SINGLE					            (0x00 << 2)
#define MDC04_MPS_0P5Hz					            	(0x01 << 2)
#define MDC04_MPS_1Hz					            		(0x02 << 2)
#define MDC04_MPS_2Hz					            		(0x03 << 2)
#define MDC04_MPS_4Hz					            		(0x04 << 2)
#define MDC04_MPS_10Hz					            	(0x05 << 2)

#define MDC04_CLKSTRETCH_EN					          (0x01 << 5)
/******************  Bit definition for status register  *******/
#define MDC04_STATUS_CONVERTMODE_MASK          (0x81)
#define MDC04_STATUS_I2CDATACRC_MASK           (0x20)
#define MDC04_STATUS_I2CCMDCRC_MASK            (0x10)
#define MDC04_STATUS_SYSRESETFLAG_MASK         (0x08)

#define MDC04_CONVERTMODE_IDLE             		   (0x00)
#define MDC04_CONVERTMODE_T             		   (0x01)
#define MDC04_CONVERTMODE_C             		   (0x02)
#define MDC04_CONVERTMODE_TC1            		   (0x03)
/******************  Bit definition for channel select register  *******/
#define MDC04_CHANNEl_SELECT_MASK           	       (0x07)
#define MDC04_CHANNEl_C1           					   (0x01)
#define MDC04_CHANNEl_C2           					   (0x02)
#define MDC04_CHANNEl_C3           					   (0x03)
#define MDC04_CHANNEl_C4           					   (0x04)
#define MDC04_CHANNEl_C1C2           				   (0x05)
#define MDC04_CHANNEl_C1C2C3           		   	 (0x06)
#define MDC04_CHANNEl_C1C2C3C4           	   	 (0x07)
/******************  Bit definition for feeadback capacitor register  *******/
#define MDC04_CFEED_OSR_MASK           	   		 (0xC0)
#define MDC04_CFEED_CFB_MASK           	   		 (0x3F)
/*Definition of conversion time corresponding to repeatability setting*/
#define tCapConvert_LowRep   			(4)		/* ms. per channel*/
#define tCapConvert_MediumRep   	    (6)		/* ms. per channel*/
#define tCapConvert_HighRep   		    (11)	/* ms. per channel*/
#define tTempConvert_LowRep  		    (4)		/* ms. */
#define tTempConvert_MediumRep      	(6)		/* ms. */
#define tTempConvert_HighRep   		    (11)	/* ms. */
#define tAutoCal_LowRep  			    (20)	/* ms. */
#define tAutoCal_MediumRep   			(6)	    /* ms. */
#define tAutoCal_HighRep  			    (65)	/* ms. */

/*Temperature conversion time for different resolutions */
#define MDC04_tConv_T_LOWRep		  (3.5)	    /*ms*/
#define MDC04_tConv_T_MEDIUMRep	      (93.75)	/*ms*/
#define MDC04_tConv_T_HIGHRep		  (10.5)	/*ms*/
#define MDC04_tConv_C_LOWRep		  (3.5)		/*ms*/
#define MDC04_tConv_C_MEDIUMRep	      (93.75)	/*ms*/
#define MDC04_tConv_C_HIGHRep		  (10.5)	/*ms*/

#define MDC04_tAutoCal_HighRep		  (62.8)	/*ms*/
#define MDC04_tAutoCal_LowRep		  (19.0)	/*ms*/

/* MDC04/01 i2c Commands-------------------------------------------------------*/
typedef enum {
    CONVERT_T            	= 0xCC44,   //温度测量函数，只发送温度转换指令
    CONVERT_C            	= 0xCC66,	//电容测量函数，只发送电容转换指令，全部通道测量
    READ_ONE_BYTE        	= 0xd200,   //读取某一寄存器地址指令，如读取1E地址即发送d21e指令
    WRITE_ONE_BYTE       	= 0x5200,   //向某一寄存器地址写入指令，如写入地址1F即发送521f指令
    WRITE_CONFIG         	= 0x5206,   //写配置寄存器指令，用来设定周期测量频率以及测温转换重复性
    READ_STATUSCONFIG       = 0xf32d,   //读取状态+配置寄存器，返回高字节状态寄存器，低字节配置寄存器
    CLEAR_STATUS            = 0x3041,   //清除状态寄存器
    BREAK                	= 0x3093,   //停止连续测量功能
    AUTO_CALIBRATION        = 0xa187,   //自动校准电容量程，建议在某一稳定被测电容状态下执行该条指令，会自动适配电容量程
	SOFT_RESET              = 0x30a2,	//软件复位，执行该指令可以使芯片自动复位初始化	
    COPY_PAGE0           	= 0xcc48,   //将暂存器内数据存储至内部EEPROM中,如状态/配置寄存器数据，报警值，用户自定义数据
} MDC04_I2C_CMD;

/*I2C access Register address*/
#define SCR_temp_lsb 								(0)
#define SCR_temp_msb 								(1)
#define SCR_cap1_lsb 								(2)
#define SCR_cap1_msb 								(3)
#define SCR_cap2_lsb 								(14)
#define SCR_cap2_msb 								(15)
#define SCR_cap3_lsb 								(16)
#define SCR_cap3_msb 								(17)
#define SCR_cap4_lsb 								(18)
#define SCR_cap4_msb 								(19)

#define SCR__CFG							(6)
#define SCR_REGADDR_STATUS					(7)
#define SCR_REGADDR_ChSel						(28)
#define SCR_REGADDR_COS							(29)
#define SCR_REGADDR_CFB 						(34)

#define SCR_REGADDR_TCOEFF_A				(31)
#define SCR_REGADDR_TCOEFF_AB				(32)
#define SCR_REGADDR_TCOEFF_B				(33)

//***********error code********************//
#define MY_ERR_INVLID_POINTER    (0x80)
#define MY_ERR_INVLID_RANGE      (0x81)

/*Exported functions*/
MY_FLT32 MDC04_StartTempConvert(void);
void MDC04_ReadCap(MY_FLT32 *fcap1,MY_FLT32 *fcap2,MY_FLT32 *fcap3,MY_FLT32 *fcap4);
MY_BOOL MDC04_Set_Cap_Offset(MY_FLT32 Co);
MY_BOOL MDC04_Set_Cap_FullScale(MY_FLT32 Cr);
MY_BOOL MDC04_Set_Cap_Range(MY_FLT32 Cmin,MY_FLT32 Cmax);
MY_BOOL MDC04_Set_Cap_Channel(int Ch);
MY_BOOL MDC04_SysCfg(unsigned char Re);
MY_BOOL MDC04_Set_CapRange_Auto(MY_FLT32 *COS);
MY_BOOL MDC04_Copy_EEPROM(void);

#include "MDC04.h"  
/*Internal functions*/
static MY_INT16 MDC04_TemptoOutput(MY_FLT32 Temp);
static MY_FLT32 MDC04_OutputtoTemp(MY_INT16 out);
static MY_FLT32 MDC04_OutputtoCap(MY_U16 out, MY_FLT32 Co, MY_FLT32 Cr);
static MY_U8   MY_I2C_CRC8(MY_U8 data[], MY_U8 nbrOfBytes);
static MY_BOOL MDC04_SingleByteWrite(MY_U8 reg, MY_U8 value);
static MY_BOOL MDC04_SingleByteRead(MY_U8 reg, MY_U8 *value);
static MY_BOOL MDC04_ConvertTemp(void);
MY_BOOL MDC04_ConvertCap(void);
static MY_BOOL MDC04_ReadTempPolling(MY_U16 *iTemp);  //针对周期测量稳定模式使用
static MY_BOOL MDC04_ReadTempCap1Polling(MY_U16 *iTemp, MY_U16 *iCap1);  //针对周期测量温度+电容通道1模式使用
static void MDC04_Read_Chx_Cap(MY_U8 ch, MY_U16 *icap);        //读取指定通道电容测量结果
static MY_BOOL MDC04_GetCapChannel(MY_U8 *chann);
static MY_BOOL MDC04_SetCapChannel(MY_U8 chann);
static MY_BOOL MDC04_ReadCapConfigure(MY_FLT32 *Coffset, MY_FLT32 *Crange);
static MY_BOOL MDC04_CapConfigureFs(MY_FLT32 Cfs);
static MY_BOOL MDC04_CapConfigureOffset(MY_FLT32 Coffset);
static MY_BOOL MDC04_ReadCapConfigure(MY_FLT32 *Coffset, MY_FLT32 *Crange);
static MY_BOOL MDC04_CapConfigureRange(MY_FLT32 Cmin, MY_FLT32 Cmax);
static MY_BOOL MDC04_GetCfg_CapOffset(MY_FLT32 *Coffset);
static MY_BOOL MDC04_GetCfg_CapRange(MY_FLT32 *Crange);

static MY_BOOL MDC04_ReadTempWaiting(MY_U16 *iTemp);  //针对周期测量温度模式使用
static MY_BOOL MDC04_ReadCapC1C2C3C4(MY_U16 *icap);
static MY_BOOL MDC04_SetConfig(MY_U8 mps, MY_U8 repeatability);
static MY_BOOL MDC04_ReadStatusConfig(MY_U8 *status, MY_U8 *cfg);


#endif /*_MDC04_H_*/
