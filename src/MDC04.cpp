#include "MDC04.h"
#include "config.h"

#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
#include <OneWire.h>
static OneWire* s_ow_buses[3] = {nullptr, nullptr, nullptr};
static int s_ow_device_count = 0;

static bool OW_SetCapChannel(int dev, uint8_t ch) {
    if (dev < 0 || dev >= 3 || !s_ow_buses[dev]) return false;
    
    // 读取 15 字节参数
    uint8_t params[15];
    s_ow_buses[dev]->reset();
    s_ow_buses[dev]->write(0xCC); // Skip ROM
    s_ow_buses[dev]->write(0x8B); // Read Parameters
    for (int i = 0; i < 15; i++) {
        params[i] = s_ow_buses[dev]->read();
    }
    
    // 修改 Ch_Sel (byte 3)
    params[3] = (params[3] & ~0x07) | (ch & 0x07);
    
    // 重新计算 Maxim CRC-8 并写回
    params[14] = OneWire::crc8(params, 14);
    
    s_ow_buses[dev]->reset();
    s_ow_buses[dev]->write(0xCC); // Skip ROM
    s_ow_buses[dev]->write(0xAB); // Write Parameters
    for (int i = 0; i < 15; i++) {
        s_ow_buses[dev]->write(params[i]);
    }
    
    return true;
}
#endif

/* GLOBAL DEFINITIONS */
static float CapCfg_offset = 15.0f, CapCfg_range = 15.5f;

/* 偏移电容和反馈电容系数 */
static const float COS_Factor[8] = { 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 40.0f };
static const struct {
    float Cfb0; 
    float Factor[6];
} CFB = { 2.0f, {2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 46.0f} };

/* Helper Functions Prototypes (Internal) */
static uint8_t Wire_Transmit(uint8_t DeviceAddr, const uint8_t *pData, uint8_t Size);
static uint8_t Wire_Receive(uint8_t DeviceAddr, uint8_t *pData, uint8_t Size);
static uint8_t Wire_Cmd_Write(uint8_t DeviceAddr, uint16_t cmd, const uint8_t *pData, uint8_t Size);
static uint8_t Wire_Cmd_Read(uint8_t DeviceAddr, uint16_t cmd, uint8_t *pData, uint8_t Size);
static uint8_t MY_I2C_CRC8(uint8_t data[], uint8_t nbrOfBytes);
static bool MDC04_SingleByteWrite(uint8_t reg, uint8_t value);
static bool MDC04_SingleByteRead(uint8_t reg, uint8_t *value);
static bool MDC04_ConvertTemp(void);
static bool MDC04_ReadTempWaiting(uint16_t *iTemp);
static float MDC04_OutputtoTemp(int16_t out);
static float MDC04_OutputtoCap(uint16_t out, float Co, float Cr);
static bool ReadCapRaw(uint8_t ch, uint16_t *icap);
static bool MDC04_ReadCapC1C2C3C4(uint16_t *icap);
static bool ReadCosConfig(uint8_t *Coscfg);
static bool WriteCosConfig(uint8_t Coffset, uint8_t Cosbits);
static bool ReadCfbConfig(uint8_t *cfb);
static bool WriteCfbConfig(uint8_t value);
static bool MDC04_SetConfig(uint8_t mps, uint8_t repeatability);
static bool MDC04_ReadStatusConfig(uint8_t *status, uint8_t *cfg);
static uint8_t CaptoCoscfg(float osCap);
static float CoscfgtoCapOffset(uint8_t osCfg);
static uint8_t CapRangetocfbCfg(float fsCap);
static float CfbcfgtoCapRange(uint8_t fbCfg);
static bool MDC04_GetCfg_CapOffset(float *Coffset);
static bool MDC04_GetCfg_CapRange(float *Crange);
static bool MDC04_CapConfigureOffset(float Coffset);
static bool MDC04_CapConfigureFs(float Cfs);
static bool MDC04_CapConfigureRange(float Cmin, float Cmax);
static bool MDC04_ReadCapConfigure(float *Coffset, float *Crange);
static bool MDC04_GetCapChannel(uint8_t *chann);
static bool MDC04_SetCapChannel(uint8_t chann);

/* I2C 通信封装层 */
static uint8_t Wire_Transmit(uint8_t DeviceAddr, const uint8_t *pData, uint8_t Size) {
    Wire.beginTransmission(DeviceAddr);
    for (uint8_t i = 0; i < Size; i++) {
        Wire.write(pData[i]);
    }
    uint8_t error = Wire.endTransmission();
    if (error == 0) return GPIOI2C_XFER_LASTACK;
    if (error == 2) return GPIOI2C_XFER_ADDRNACK;
    if (error == 3) return GPIOI2C_XFER_ABORTNACK;
    return GPIOI2C_XFER_BUSERR;
}

static uint8_t Wire_Receive(uint8_t DeviceAddr, uint8_t *pData, uint8_t Size) {
    Wire.beginTransmission(DeviceAddr);
    uint8_t error = Wire.endTransmission();
    if (error == 2) {
        return GPIOI2C_XFER_ADDRNACK;
    } else if (error != 0) {
        return GPIOI2C_XFER_BUSERR;
    }
    
    uint8_t readBytes = Wire.requestFrom(DeviceAddr, Size);
    if (readBytes < Size) {
        return GPIOI2C_XFER_ABORTNACK;
    }
    for (uint8_t i = 0; i < Size; i++) {
        pData[i] = Wire.read();
    }
    return GPIOI2C_XFER_LASTNACK;
}

static uint8_t Wire_Cmd_Write(uint8_t DeviceAddr, uint16_t cmd, const uint8_t *pData, uint8_t Size) {
    Wire.beginTransmission(DeviceAddr);
    Wire.write((uint8_t)(cmd >> 8));
    Wire.write((uint8_t)(cmd & 0xFF));
    for (uint8_t i = 0; i < Size; i++) {
        Wire.write(pData[i]);
    }
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("[MDC04 I2C Error] Cmd Write endTransmission failed. Cmd=0x%04X, Error=%d\n", cmd, error);
        if (error == 2) return GPIOI2C_XFER_ADDRNACK;
        if (error == 3) return GPIOI2C_XFER_ABORTNACK;
        return GPIOI2C_XFER_BUSERR;
    }
    return GPIOI2C_XFER_LASTACK;
}

static uint8_t Wire_Cmd_Read(uint8_t DeviceAddr, uint16_t cmd, uint8_t *pData, uint8_t Size) {
    Wire.beginTransmission(DeviceAddr);
    Wire.write((uint8_t)(cmd >> 8));
    Wire.write((uint8_t)(cmd & 0xFF));
    uint8_t error = Wire.endTransmission(false); // 发送 Repeated Start
    if (error != 0) {
        Serial.printf("[MDC04 I2C Error] Cmd Read endTransmission failed. Cmd=0x%04X, Error=%d\n", cmd, error);
        if (error == 2) {
            return GPIOI2C_XFER_ADDRNACK;
        } else {
            return GPIOI2C_XFER_BUSERR;
        }
    }
    
    uint8_t readBytes = Wire.requestFrom(DeviceAddr, Size);
    if (readBytes < Size) {
        Serial.printf("[MDC04 I2C Error] Cmd Read requestFrom failed. Cmd=0x%04X, Expected %d bytes, got %d\n", cmd, Size, readBytes);
        return GPIOI2C_XFER_ABORTNACK;
    }
    for (uint8_t i = 0; i < Size; i++) {
        pData[i] = Wire.read();
    }
    return GPIOI2C_XFER_LASTNACK;
}

/* 芯片初始化 */
bool MDC04_Init(int sdaPin, int sclPin) {
    // 显式配置引脚为输入上拉，启用 ESP32 内部上拉电阻
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, INPUT_PULLUP);
    bool ok = Wire.begin(sdaPin, sclPin);
    // 无外部上拉电阻时，400kHz 速率过高会导致信号上升沿时间不够，因此将速率降至 50kHz
    Wire.setClock(50000); 
    return ok;
}

/* 启动温度转换 */
float MDC04_StartTempConvert(void) { 
    float fTemp = -999.0f; 
    uint16_t iTemp; 
    
    if (MDC04_ConvertTemp()) {
        delay(11); // 高分辨率下温度转换时间约 10.5ms，延迟 11ms
        if (MDC04_ReadTempWaiting(&iTemp)) {
            fTemp = MDC04_OutputtoTemp((int16_t)iTemp);
        }
    }
    return fTemp;
}

/* 启动电容转换 */
bool MDC04_ConvertCap(void) {     
    uint8_t cmd[2] = {(uint8_t)(CONVERT_C >> 8), (uint8_t)(CONVERT_C & 0xFF)};
    if (Wire_Transmit(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK) {
        return false;
    }
    return true;     
}

/* 读取四路通道计算出的电容值 */
bool MDC04_ReadCap(float *fcap1, float *fcap2, float *fcap3, float *fcap4) {   
    uint16_t icap[4];  
    uint8_t status, cfg;

    if (!MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range)) return false;
    if (!MDC04_ReadStatusConfig(&status, &cfg)) return false;
    if (!MDC04_ReadCapC1C2C3C4(icap)) return false;

    *fcap1 = MDC04_OutputtoCap(icap[0], CapCfg_offset, CapCfg_range);
    *fcap2 = MDC04_OutputtoCap(icap[1], CapCfg_offset, CapCfg_range);
    *fcap3 = MDC04_OutputtoCap(icap[2], CapCfg_offset, CapCfg_range);
    *fcap4 = MDC04_OutputtoCap(icap[3], CapCfg_offset, CapCfg_range);
    return true;
}

/* 设置偏置电容 Co (0 ~ 103.5 pF) */
bool MDC04_Set_Cap_Offset(float Co) {    
    if (!((Co >= 0.0f) && (Co <= 103.5f))) {
        return false;
    }
    MDC04_CapConfigureOffset(Co);
    MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);
    return true;
}

/* 设置满量程范围 Cr (0 ~ 15.5 pF) */
bool MDC04_Set_Cap_FullScale(float Cr) {                     
    if (!((Cr >= 0.0f) && (Cr <= 15.5f))) {
        return false;
    }        
    MDC04_CapConfigureFs(Cr);            
    MDC04_ReadCapConfigure(&CapCfg_offset, &CapCfg_range);    
    return true;
}

/* 设置测量电容通道选择 */
bool MDC04_Set_Cap_Channel(int Ch) {   
    MDC04_SetCapChannel(Ch & 0x07);                
    return true;
}

/* 配置系统参数（重复性配置） */
bool MDC04_SysCfg(unsigned char Re) { 
    uint8_t repeat = CFG_Rep_High;
    if (Re == 0x00) { repeat = CFG_Rep_Low; }
    if (Re == 0x01) { repeat = CFG_Rep_Medium; } 
    if (Re == 0x02) { repeat = CFG_Rep_High; }
    MDC04_SetConfig(CFG_MPS_Single, repeat);
    return true;
}

/* 自动配置电容范围 */
bool MDC04_Set_CapRange_Auto(float *COS) { 
    uint8_t cmd[2] = {(uint8_t)(AUTO_CALIBRATION >> 8), (uint8_t)(AUTO_CALIBRATION & 0xFF)};
    if (Wire_Transmit(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK) {
        return false;
    }
    delay(500);
    MDC04_GetCfg_CapOffset(COS);    
    return true;
}

/* 将当前 Page0 配置写入 EEPROM 备份 */
bool MDC04_Copy_EEPROM(void) {    
    uint8_t cmd[2] = {(uint8_t)(COPY_PAGE0 >> 8), (uint8_t)(COPY_PAGE0 & 0xFF)};    
    bool ret = true;
    if (Wire_Transmit(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK) {
        ret = false;
    }
    delay(50);    
    return ret; 
}

/* 设置电容范围最小/最大值 */
bool MDC04_Set_Cap_Range(float Cmin, float Cmax) {
    if (!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && ((Cmax-Cmin) <= 31.0))) {
        return false;
    }
    return MDC04_CapConfigureRange(Cmin, Cmax);
}

/******************* Internal Helpers Implementation ***********************************/

#define POLYNOMIAL  0x131 // 100110001 (CRC8多项式)
static uint8_t MY_I2C_CRC8(uint8_t data[], uint8_t nbrOfBytes) {
    uint8_t bit;
    uint8_t crc = 0xFF;
    uint8_t byteCtr;
    for (byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
        crc ^= (data[byteCtr]);
        for (bit = 8; bit > 0; --bit) {
            if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else crc = (crc << 1);
        }
    }
    return crc;
}

static bool MDC04_SingleByteWrite(uint8_t reg, uint8_t value) { 
    uint8_t scr_wr[3];
    scr_wr[0] = value;  
    scr_wr[1] = 0xFF; 
    scr_wr[2] = MY_I2C_CRC8(scr_wr, 2);
    uint8_t err = Wire_Cmd_Write(MDC04_I2C_ADDR, WRITE_ONE_BYTE | reg, scr_wr, 3);
    if (err != GPIOI2C_XFER_LASTACK) {
        Serial.printf("[MDC04 Error] MDC04_SingleByteWrite failed. Reg=0x%02X, Val=0x%02X, ErrorCode=%d\n", reg, value, err);
        return false;
    }
    return true;    
}

static bool MDC04_SingleByteRead(uint8_t reg, uint8_t *value) { 
    uint8_t scr_rd[3];
    uint8_t err = Wire_Cmd_Read(MDC04_I2C_ADDR, READ_ONE_BYTE | reg, scr_rd, 3);
    if (err != GPIOI2C_XFER_LASTNACK) {
        Serial.printf("[MDC04 Error] MDC04_SingleByteRead cmd read failed. Reg=0x%02X, ErrorCode=%d\n", reg, err);
        return false;
    }
    uint8_t calc_crc = MY_I2C_CRC8(scr_rd, 2);
    if (scr_rd[2] != calc_crc) {
        Serial.printf("[MDC04 Error] MDC04_SingleByteRead CRC check failed. Reg=0x%02X, ReadCRC=0x%02X, CalcCRC=0x%02X, DataBytes: 0x%02X 0x%02X\n",
                      reg, scr_rd[2], calc_crc, scr_rd[0], scr_rd[1]);
        return false;
    }
    *value = scr_rd[0];
    return true;
}

static bool MDC04_ConvertTemp(void) {    
    uint8_t cmd[2] = {(uint8_t)(CONVERT_T >> 8), (uint8_t)(CONVERT_T & 0xFF)};
    if (Wire_Transmit(MDC04_I2C_ADDR, cmd, 2) != GPIOI2C_XFER_LASTACK) {
        return false;
    }   
    return true;     
}

static bool MDC04_ReadTempWaiting(uint16_t *iTemp) {
    uint8_t data[3];
    if (Wire_Receive(MDC04_I2C_ADDR, data, 3) != GPIOI2C_XFER_LASTNACK) {   
        return false;
    }
    if (data[2] != MY_I2C_CRC8(data, 2)) {   
        return false;
    }
    *iTemp = ((uint16_t)data[0] << 8) | data[1];
    return true;        
}

static float MDC04_OutputtoTemp(int16_t out) {
    return ((float)out/256.0f + 40.0f);    
}

static float MDC04_OutputtoCap(uint16_t out, float Co, float Cr) {
    return (2.0f * (out/65535.0f - 0.5f) * Cr + Co);    
}

static bool ReadCapRaw(uint8_t ch, uint16_t *icap) {
    uint8_t Cap_lsb, Cap_msb, scr_cap_lsb, scr_cap_msb;    
    switch (ch) {
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

    if (!MDC04_SingleByteRead(scr_cap_lsb, &Cap_lsb)) return false;    
    if (!MDC04_SingleByteRead(scr_cap_msb, &Cap_msb)) return false;    

    *icap = (Cap_msb << 8) | Cap_lsb;
    return true;    
}

static bool MDC04_ReadCapC1C2C3C4(uint16_t *icap) {
    if (!ReadCapRaw(1, &icap[0])) return false; 
    if (!ReadCapRaw(2, &icap[1])) return false; 
    if (!ReadCapRaw(3, &icap[2])) return false; 
    if (!ReadCapRaw(4, &icap[3])) return false;
    return true;
}

static bool ReadCosConfig(uint8_t *Coscfg) { 
    uint8_t reg_cos, reg_cfb;
    if (!MDC04_SingleByteRead(SCR_REGADDR_COS, &reg_cos)) return false;    
    if (!MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb)) return false;
    *Coscfg = reg_cos & (0xFF >> (3 - (reg_cfb >> 6)));
    return true;    
}

static bool WriteCosConfig(uint8_t Coffset, uint8_t Cosbits) { 
    uint8_t reg_cfb;
    MDC04_SingleByteWrite(SCR_REGADDR_COS, Coffset);            
    if (!MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb)) return false;    
    reg_cfb = (reg_cfb & ~CFB_COSRANGE_Mask) | Cosbits; 
    MDC04_SingleByteWrite(SCR_REGADDR_CFB, reg_cfb);
    return true;    
}

static bool ReadCfbConfig(uint8_t *cfb) { 
    uint8_t reg_cfb;
    if (!MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb)) return false;    
    *cfb = (reg_cfb & CFB_CFBSEL_Mask);
    return true;    
}

static bool WriteCfbConfig(uint8_t value) { 
    uint8_t reg_cfb;
    if (!MDC04_SingleByteRead(SCR_REGADDR_CFB, &reg_cfb)) return false;
    reg_cfb = (reg_cfb & ~CFB_CFBSEL_Mask) | (value & CFB_CFBSEL_Mask);
    MDC04_SingleByteWrite(SCR_REGADDR_CFB, reg_cfb);    
    return true;    
}

static bool MDC04_SetCapChannel(uint8_t chann) { 
    uint8_t reg_ChSel;
    if (!MDC04_SingleByteRead(SCR_REGADDR_ChSel, &reg_ChSel)) return false;
    reg_ChSel = (reg_ChSel & ~CCS_CHANNEL_Mask) | (chann & CCS_CHANNEL_Mask);
    MDC04_SingleByteWrite(SCR_REGADDR_ChSel, reg_ChSel);    
    return true;
}

static bool MDC04_GetCapChannel(uint8_t *chann) {
    uint8_t reg_ChSel;
    if (!MDC04_SingleByteRead(SCR_REGADDR_ChSel, &reg_ChSel)) return false;
    *chann = reg_ChSel & CCS_CHANNEL_Mask;
    return true;
}

static bool MDC04_SetConfig(uint8_t mps, uint8_t repeatability) { 
    uint8_t scr_rd[3], scr_wr[3];
    if (Wire_Cmd_Read(MDC04_I2C_ADDR, READ_STATUSCONFIG, scr_rd, 3) != GPIOI2C_XFER_LASTNACK) return false;
    scr_wr[0] = repeatability;  
    scr_wr[1] = 0xFF; 
    scr_wr[2] = MY_I2C_CRC8(scr_wr, 2);
    Wire_Cmd_Write(MDC04_I2C_ADDR, WRITE_CONFIG, scr_wr, 3);    
    return true;
}

static bool MDC04_ReadStatusConfig(uint8_t *status, uint8_t *cfg) { 
    uint8_t scr_rd[3];
    if (Wire_Cmd_Read(MDC04_I2C_ADDR, READ_STATUSCONFIG, scr_rd, 3) != GPIOI2C_XFER_LASTNACK) return false;      
    *status = scr_rd[0];
    *cfg = scr_rd[1];    
    return true;        
}

static uint8_t CaptoCoscfg(float osCap) {
    int i; 
    uint8_t CosCfg = 0x00;
    for (i = 7; i >= 0; i--) {
        if (osCap >= COS_Factor[i]) {
            CosCfg |= (0x01 << i);
            osCap -= COS_Factor[i];
        }
    }
    return CosCfg;  
}

static float CoscfgtoCapOffset(uint8_t osCfg) {
    uint8_t i; 
    float Coffset = 0.0f; 
    for (i = 0; i < 8; i++) {
        if (osCfg & 0x01) Coffset += COS_Factor[i];
        osCfg >>= 1;
    }
    return Coffset;
}

static uint8_t CapRangetocfbCfg(float fsCap) {
    int8_t i; 
    uint8_t CfbCfg = 0x00;
    fsCap = fsCap * (3.6f / 0.507f);
    fsCap -= CFB.Cfb0;
    for (i = 5; i >= 0; i--) {
        if (fsCap >= CFB.Factor[i]) {
            fsCap -= CFB.Factor[i];
            CfbCfg |= (0x01 << i);
        }           
    }
    return CfbCfg;  
}

static float CfbcfgtoCapRange(uint8_t fbCfg) {
    uint8_t i;  
    float Crange = CFB.Cfb0;
    for (i = 0; i <= 5; i++) {
        if (fbCfg & 0x01) Crange += CFB.Factor[i];
        fbCfg >>= 1;
    }
    return (0.507f / 3.6f) * Crange;    
}

static bool MDC04_GetCfg_CapOffset(float *Coffset) {    
    uint8_t Cos_cfg;    
    if (!ReadCosConfig(&Cos_cfg)) return false;
    *Coffset = CoscfgtoCapOffset(Cos_cfg);    
    return true;    
}

static bool MDC04_GetCfg_CapRange(float *Crange) {
    uint8_t Cfb_cfg;
    if (!ReadCfbConfig(&Cfb_cfg)) return false;
    *Crange = CfbcfgtoCapRange(Cfb_cfg);
    return true;    
}

static bool MDC04_CapConfigureOffset(float Coffset) { 
    uint8_t CosCfg, Cosbits;
    CosCfg = CaptoCoscfg(Coffset + 0.25f);    
    if (!(CosCfg & ~0x1F)) Cosbits = COS_RANGE_5BIT;
    else if (!(CosCfg & ~0x3F)) Cosbits = COS_RANGE_6BIT;
    else if (!(CosCfg & ~0x7F)) Cosbits = COS_RANGE_7BIT;
    else Cosbits = COS_RANGE_8BIT;
    return WriteCosConfig(CosCfg, Cosbits); 
}

static bool MDC04_CapConfigureFs(float Cfs) { 
    uint8_t Cfbcfg;
    Cfs = (Cfs + 0.1408f);       
    Cfbcfg = CapRangetocfbCfg(Cfs); 
    return WriteCfbConfig(Cfbcfg);
}

static bool MDC04_CapConfigureRange(float Cmin, float Cmax) { 
    float Cfs, Cos;
    Cos = (Cmin + Cmax) / 2.0f;
    Cfs = (Cmax - Cmin) / 2.0f;    
    if (!MDC04_CapConfigureOffset(Cos)) return false;    
    if (!MDC04_CapConfigureFs(Cfs)) return false;
    return true;
}

static bool MDC04_ReadCapConfigure(float *Coffset, float *Crange) { 
    if (!MDC04_GetCfg_CapOffset(Coffset)) return false;
    if (!MDC04_GetCfg_CapRange(Crange)) return false;
    return true;
}

/* 双模式统一初始化与读取接口实现 */
bool MDC04_Init_All(void) {
#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
    Serial.printf("[MDC04] Initializing One-Wire mode with 3 independent buses (Pins: %d, %d, %d)...\n",
                  ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN);
                  
    s_ow_buses[0] = new OneWire(ONEWIRE_DQ1_PIN);
    s_ow_buses[1] = new OneWire(ONEWIRE_DQ2_PIN);
    s_ow_buses[2] = new OneWire(ONEWIRE_DQ3_PIN);
    
    s_ow_device_count = 0;

    const int pins[3] = { ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN };

    for (int dev = 0; dev < 3; dev++) {
        // 验证芯片是否在线：发送 Reset，检测 presence pulse
        bool present = s_ow_buses[dev]->reset();
        if (!present) {
            Serial.printf("[MDC04] Error: No presence pulse detected for Chip %d on Pin %d\n", dev + 1, pins[dev]);
            return false;
        }
        
        // 选用 Skip ROM (0xCC) 后发送 Read TC1 (0xBD) 命令测试通信
        s_ow_buses[dev]->write(0xCC);
        s_ow_buses[dev]->write(0xBD);
        uint8_t dummy[5];
        for (int i = 0; i < 5; i++) {
            dummy[i] = s_ow_buses[dev]->read();
        }
        
        if (OneWire::crc8(dummy, 4) != dummy[4]) {
            Serial.printf("[MDC04] Warning: Chip %d on Pin %d online but CRC check failed during verify.\n", dev + 1, pins[dev]);
        } else {
            Serial.printf("[MDC04] Chip %d online on Pin %d.\n", dev + 1, pins[dev]);
        }
        s_ow_device_count++;
    }

    Serial.printf("[MDC04] 3 independent buses init complete. %d chips configured.\n", s_ow_device_count);
    return true;
#else
    Serial.printf("[MDC04] Initializing I2C mode on SDA=%d SCL=%d...\n", I2C_SDA_PIN, I2C_SCL_PIN);
    return MDC04_Init(I2C_SDA_PIN, I2C_SCL_PIN);
#endif
}

bool MDC04_Read_All(float* out_caps) {
#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
    if (s_ow_device_count == 0) return false;
    
    // 初始化清空输出数组
    for (int i = 0; i < 4; i++) {
        out_caps[i] = 0.0f;
    }
    
    // 1. 在每个总线上分别通过 Skip ROM 发送 Convert C (0x66) 触发电容转换
    for (int dev = 0; dev < s_ow_device_count; dev++) {
        if (s_ow_buses[dev]) {
            s_ow_buses[dev]->reset();
            s_ow_buses[dev]->write(0xCC); // Skip ROM
            s_ow_buses[dev]->write(0x66); // Convert C
        }
    }
    
    // 2. 延时等待转换完成（单通道转换仅需 10.5ms，这里延时 20ms 确保安全）
    delay(20);
    
    // 3. 逐个总线通过 Skip ROM 读取芯片通道 1 测量值
    for (int dev = 0; dev < s_ow_device_count; dev++) {
        if (!s_ow_buses[dev]) continue;
        
        // 读取温度和通道 1 测量值 (command 0xBD)
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0xBD); // Read TC1
        
        uint8_t data_tc1[5];
        for (int i = 0; i < 5; i++) {
            data_tc1[i] = s_ow_buses[dev]->read();
        }
        
        // 校验 CRC-8 (Maxim)
        if (OneWire::crc8(data_tc1, 4) != data_tc1[4]) {
            Serial.printf("[MDC04] CRC error reading TC1 from chip %d!\n", dev + 1);
            return false;
        }
        
        uint16_t raw_c1 = (data_tc1[3] << 8) | data_tc1[2];
        
        // 物理电容计算
        float c1 = MDC04_OutputtoCap(raw_c1, CapCfg_offset, CapCfg_range);
        
        // 顺次存入输出浮点电容数组的前 3 个元素中
        out_caps[dev] = c1;
    }
    // 第 4 个通道固定留空为 0
    out_caps[3] = 0.0f;
    return true;
#else
    float fcap1 = 0.0f, fcap2 = 0.0f, fcap3 = 0.0f, fcap4 = 0.0f;
    bool ok = MDC04_ReadCap(&fcap1, &fcap2, &fcap3, &fcap4);
    if (ok) {
        out_caps[0] = fcap1;
        out_caps[1] = fcap2;
        out_caps[2] = fcap3;
        out_caps[3] = fcap4;
    }
    return ok;
#endif
}

bool MDC04_Read_All_12Channels(float* out_caps) {
#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
    if (s_ow_device_count == 0) return false;
    
    // 12 个通道初始化为 0.0f
    for (int i = 0; i < 12; i++) {
        out_caps[i] = 0.0f;
    }
    
    // 轮询 4 个通道
    for (uint8_t ch = 1; ch <= 4; ch++) {
        // 配置每一颗芯片的当前通道选择
        for (int dev = 0; dev < s_ow_device_count; dev++) {
            OW_SetCapChannel(dev, ch);
        }
        
        // 触发转换
        for (int dev = 0; dev < s_ow_device_count; dev++) {
            if (s_ow_buses[dev]) {
                s_ow_buses[dev]->reset();
                s_ow_buses[dev]->write(0xCC); // Skip ROM
                s_ow_buses[dev]->write(0x66); // Convert C
            }
        }
        
        // 延时等待转换完成（单通道 10.5ms，延时 20ms 保证安全）
        delay(20);
        
        // 读取每一颗芯片的当前通道电容值
        for (int dev = 0; dev < s_ow_device_count; dev++) {
            if (!s_ow_buses[dev]) continue;
            
            s_ow_buses[dev]->reset();
            s_ow_buses[dev]->write(0xCC); // Skip ROM
            s_ow_buses[dev]->write(0xBD); // Read TC1 (读取温度和当前选中通道的电容值)
            
            uint8_t data_tc1[5];
            for (int i = 0; i < 5; i++) {
                data_tc1[i] = s_ow_buses[dev]->read();
            }
            
            // 校验 CRC-8 (Maxim)
            if (OneWire::crc8(data_tc1, 4) != data_tc1[4]) {
                Serial.printf("[MDC04] CRC error reading TC1 from chip %d (channel %d)!\n", dev + 1, ch);
            } else {
                uint16_t raw_c1 = (data_tc1[3] << 8) | data_tc1[2];
                float c = MDC04_OutputtoCap(raw_c1, CapCfg_offset, CapCfg_range);
                out_caps[dev * 4 + (ch - 1)] = c;
            }
        }
    }
    return true;
#else
    // I2C 模式：只有一个芯片，读 4 个通道，其余 8 个通道填 0
    for (int i = 0; i < 12; i++) {
        out_caps[i] = 0.0f;
    }
    float fcap1 = 0.0f, fcap2 = 0.0f, fcap3 = 0.0f, fcap4 = 0.0f;
    MDC04_StartTempConvert();
    bool ok = false;
    if (MDC04_ConvertCap()) {
        delay(44);
        ok = MDC04_ReadCap(&fcap1, &fcap2, &fcap3, &fcap4);
    }
    if (ok) {
        out_caps[0] = fcap1;
        out_caps[1] = fcap2;
        out_caps[2] = fcap3;
        out_caps[3] = fcap4;
    }
    return ok;
#endif
}
