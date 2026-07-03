#ifndef _MDC04_H_
#define _MDC04_H_

#include <Arduino.h>
#include <Wire.h>

#define MDC04_I2C_ADDR      0x44  /* Addr 引脚接低电平44/高电平45 */

/* I2C 传输结果定义 */
#define GPIOI2C_XFER_LASTNACK         ((uint8_t)0x00)   /* 无错误/正常读结束NACK */
#define GPIOI2C_XFER_ADDRNACK         ((uint8_t)0x01)   /* 无设备/地址无应答 */
#define GPIOI2C_XFER_ABORTNACK        ((uint8_t)0x02)   /* 传输被阻断NACK */
#define GPIOI2C_XFER_LASTACK          ((uint8_t)0x04)   /* 正常写结束ACK */
#define GPIOI2C_XFER_BUSERR           ((uint8_t)0x10)   /* 总线错误 */
#define GPIOI2C_XFER_BUSBUSY          ((uint8_t)0x20)   /* 总线忙 */

/* MDC04/01 Registers definition----------------------------------------------*/
#define CFG_CLKSTRETCH_Mask     (0x20)
#define CFG_MPS_Mask            (0x1C)
#define CFG_Repeatbility_Mask   (0x03)

#define CFG_MPS_Single  (0x00)
#define CFG_MPS_Half    (0x04)
#define CFG_MPS_1       (0x08)
#define CFG_MPS_2       (0x0C)
#define CFG_MPS_4       (0x10)
#define CFG_MPS_10      (0x14)

#define CFG_Rep_Low     (0x00)
#define CFG_Rep_Medium  (0x01)
#define CFG_Rep_High    (0x02)

#define CFG_ClkStreatch_Disable (0x00 << 5)
#define CFG_ClkStreatch_Enable  (0x01 << 5)

/* Bit definition of status register */
#define Status_Meature_Mask     (0x81)
#define Status_WriteCrc_Mask    (0x20)
#define Status_CMD_Mask         (0x10)
#define Status_POR_Mask         (0x08)

/* Bit definition of CFB register */
#define CFB_COSRANGE_Mask       (0xC0)
#define CFB_CFBSEL_Mask         (0x3F)

#define CFB_COS_BITRANGE_5      (0x1F)
#define CFB_COS_BITRANGE_6      (0x3F)
#define CFB_COS_BITRANGE_7      (0x7F)
#define CFB_COS_BITRANGE_8      (0xFF)

#define COS_RANGE_5BIT          (0x00)
#define COS_RANGE_6BIT          (0x40)
#define COS_RANGE_7BIT          (0x80)
#define COS_RANGE_8BIT          (0xC0)

/* Bit definition of Ch_Sel register */
#define CCS_CHANNEL_Mask                    (0x07)

#define CCS_CapChannel_Cap1                 (0x01)      
#define CCS_CapChannel_Cap2                 (0x02)
#define CCS_CapChannel_Cap3                 (0x03)
#define CCS_CapChannel_Cap4                 (0x04)
#define CCS_CapChannel_Cap1_2               (0x05)
#define CCS_CapChannel_Cap1_2_3             (0x06)
#define CCS_CapChannel_Cap1_2_3_4           (0x07)

#define CAP_CH1_SEL                         (0x01)
#define CAP_CH2_SEL                         (0x02)
#define CAP_CH3_SEL                         (0x03)
#define CAP_CH4_SEL                         (0x04)
#define CAP_CH1CH2_SEL                      (0x05)
#define CAP_CH1CH2CH3_SEL                   (0x06)
#define CAP_CH1CH2CH3CH4_SEL                (0x07)

/******************  Bit definition for MDC04 configuration register  ******************/
#define MDC04_CFG_REPEATABILITY_MASK          (0x03)
#define MDC04_CFG_MPS_MASK                    (0x1C)
#define MDC04_CFG_I2CCLKSTRETCH_MASK          (0x20)

/******************  Bit definition for MDC04 temperature register  *******/
#define MDC04_REPEATABILITY_LOW               (0x00 << 0)
#define MDC04_REPEATABILITY_MEDIUM            (0x01 << 0)
#define MDC04_REPEATABILITY_HIGH              (0x02 << 0)

/******************  Bit definition for TTrim in parameters  *******/
#define MDC04_MPS_SINGLE                      (0x00 << 2)
#define MDC04_MPS_0P5Hz                       (0x01 << 2)
#define MDC04_MPS_1Hz                         (0x02 << 2)
#define MDC04_MPS_2Hz                         (0x03 << 2)
#define MDC04_MPS_4Hz                         (0x04 << 2)
#define MDC04_MPS_10Hz                        (0x05 << 2)

#define MDC04_CLKSTRETCH_EN                   (0x01 << 5)

/******************  Bit definition for status register  *******/
#define MDC04_STATUS_CONVERTMODE_MASK          (0x81)
#define MDC04_STATUS_I2CDATACRC_MASK           (0x20)
#define MDC04_STATUS_I2CCMDCRC_MASK            (0x10)
#define MDC04_STATUS_SYSRESETFLAG_MASK         (0x08)

#define MDC04_CONVERTMODE_IDLE                 (0x00)
#define MDC04_CONVERTMODE_T                    (0x01)
#define MDC04_CONVERTMODE_C                    (0x02)
#define MDC04_CONVERTMODE_TC1                  (0x03)

/******************  Bit definition for channel select register  *******/
#define MDC04_CHANNEl_SELECT_MASK              (0x07)
#define MDC04_CHANNEl_C1                       (0x01)
#define MDC04_CHANNEl_C2                       (0x02)
#define MDC04_CHANNEl_C3                       (0x03)
#define MDC04_CHANNEl_C4                       (0x04)
#define MDC04_CHANNEl_C1C2                     (0x05)
#define MDC04_CHANNEl_C1C2C3                   (0x06)
#define MDC04_CHANNEl_C1C2C3C4                 (0x07)

/******************  Bit definition for feedback capacitor register  *******/
#define MDC04_CFEED_OSR_MASK                   (0xC0)
#define MDC04_CFEED_CFB_MASK                   (0x3F)

/* MDC04/01 i2c Commands-------------------------------------------------------*/
typedef enum {
    CONVERT_T               = 0xCC44,   // 测量温度
    CONVERT_C               = 0xCC66,   // 测量电容
    READ_ONE_BYTE           = 0xd200,   // 读取单个字节寄存器
    WRITE_ONE_BYTE          = 0x5200,   // 写入单个字节寄存器
    WRITE_CONFIG            = 0x5206,   // 写入配置寄存器
    READ_STATUSCONFIG       = 0xf32d,   // 读取状态和配置寄存器
    CLEAR_STATUS            = 0x3041,   // 清除状态寄存器
    BREAK                   = 0x3093,   // 停止周期测量
    AUTO_CALIBRATION        = 0xa187,   // 自动校准
    SOFT_RESET              = 0x30a2,   // 软件复位
    COPY_PAGE0              = 0xcc48,   // 保存 Page0 到 EEPROM
} MDC04_I2C_CMD;

/* I2C access Register address */
#define SCR_temp_lsb                                (0)
#define SCR_temp_msb                                (1)
#define SCR_cap1_lsb                                (2)
#define SCR_cap1_msb                                (3)
#define SCR_cap2_lsb                                (14)
#define SCR_cap2_msb                                (15)
#define SCR_cap3_lsb                                (16)
#define SCR_cap3_msb                                (17)
#define SCR_cap4_lsb                                (18)
#define SCR_cap4_msb                                (19)

#define SCR__CFG                                    (6)
#define SCR_REGADDR_STATUS                          (7)
#define SCR_REGADDR_ChSel                           (28)
#define SCR_REGADDR_COS                             (29)
#define SCR_REGADDR_CFB                             (34)

#define SCR_REGADDR_TCOEFF_A                        (31)
#define SCR_REGADDR_TCOEFF_AB                       (32)
#define SCR_REGADDR_TCOEFF_B                        (33)

//*********** error code ********************//
#define MY_ERR_INVLID_POINTER    (0x80)
#define MY_ERR_INVLID_RANGE      (0x81)

/* Exported functions */
bool MDC04_Init(int sdaPin, int sclPin);
float MDC04_StartTempConvert(void);
bool MDC04_ReadCap(float *fcap1, float *fcap2, float *fcap3, float *fcap4);
bool MDC04_Set_Cap_Offset(float Co);
bool MDC04_Set_Cap_FullScale(float Cr);
bool MDC04_Set_Cap_Range(float Cmin, float Cmax);
bool MDC04_Set_Cap_Channel(int Ch);
bool MDC04_SysCfg(unsigned char Re);
bool MDC04_Set_CapRange_Auto(float *COS);
bool MDC04_Copy_EEPROM(void);
bool MDC04_ConvertCap(void);

/* 双模式统一接口 */
bool MDC04_Init_All(void);
bool MDC04_Read_All(float* out_caps);
bool MDC04_Read_All_12Channels(float* out_caps);

#endif /*_MDC04_H_*/
