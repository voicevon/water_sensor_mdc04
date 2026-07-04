#include "MDC04.h"
#include "config.h"
#include "web_config.h"
#include <OneWire.h>

/* ============================================================
 *  OneWire 总线实例（最多 3 颗芯片，每颗一条独立总线）
 * ============================================================ */
static OneWire* s_ow_buses[3] = {nullptr, nullptr, nullptr};
static bool     s_ow_online[3] = {false, false, false}; // 记录每颗芯片是否在线
static int      s_ow_device_count = 0;

/* 默认电容量程参数（用于 OutputtoCap 物理换算） */
static float CapCfg_offset = 15.0f;
static float CapCfg_range  = 15.5f;

/* ============================================================
 *  内部辅助：设置指定芯片的通道选择寄存器（Ch_Sel）
 *  ch: 1=Cap1, 2=Cap2, 3=Cap3, 4=Cap4
 * ============================================================ */
static bool OW_SetCapChannel(int dev, uint8_t ch) {
    if (dev < 0 || dev >= 3 || !s_ow_buses[dev]) return false;

    // 使用专用 Write Ch_Sel 命令（0xAA），比全量参数写入更快更可靠
    s_ow_buses[dev]->reset();
    s_ow_buses[dev]->write(0xCC); // Skip ROM
    s_ow_buses[dev]->write(0xAA); // Write Ch_Sel
    s_ow_buses[dev]->write(ch & 0x07); // 仅写 Ch_Sel 低 3 位

    // 回读验证：使用 Read Ch_Sel（0x8A）确认写入成功
    s_ow_buses[dev]->reset();
    s_ow_buses[dev]->write(0xCC); // Skip ROM
    s_ow_buses[dev]->write(0x8A); // Read Ch_Sel
    uint8_t got = s_ow_buses[dev]->read();
    if ((got & 0x07) != (ch & 0x07)) {
        Serial.printf("[MDC04] WARN: Ch_Sel write failed chip %d! Expected %d, got 0x%02X\n",
                      dev + 1, ch, got);
        return false;
    }
    return true;
}

/* ============================================================
 *  内部辅助：将 ADC 原始值转换为物理电容（pF）
 *  公式：C = 2 * (out/65535 - 0.5) * Cr + Co
 * ============================================================ */
static float MDC04_OutputtoCap(uint16_t out, float Co, float Cr) {
    return (2.0f * ((float)out / 65535.0f - 0.5f) * Cr + Co);
}

/* ============================================================
 *  公共接口实现
 * ============================================================ */

/**
 * @brief 初始化最多 3 条独立 OneWire 总线
 *        检测各芯片在线状态，未接入的芯片跳过（不影响其他芯片）
 * @return true：至少一颗芯片在线；false：无任何芯片响应
 */
bool MDC04_Init_All(void) {
    Serial.printf("[MDC04] Initializing One-Wire mode (Pins: %d, %d, %d)...\n",
                  ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN);

    s_ow_buses[0] = new OneWire(ONEWIRE_DQ1_PIN);
    s_ow_buses[1] = new OneWire(ONEWIRE_DQ2_PIN);
    s_ow_buses[2] = new OneWire(ONEWIRE_DQ3_PIN);
    s_ow_device_count = 0;

    const int pins[3] = { ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN };

    for (int dev = 0; dev < 3; dev++) {
        s_ow_online[dev] = false;

        // 检测 presence pulse，未在线则跳过
        bool present = s_ow_buses[dev]->reset();
        if (!present) {
            Serial.printf("[MDC04] Chip %d not detected on Pin %d, skipping.\n",
                          dev + 1, pins[dev]);
            continue;
        }

        // ConvertTC1(0x10)：同时转换温度和通道1电容，等待 15ms
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0x10); // Convert TC1
        delay(15);

        // Read TC1 (0xBD): 读 9 字节 scratchpad
        // 格式: T_lsb[0], T_msb[1], C1_lsb[2], C1_msb[3], Tha[4], Tla[5], Cfg[6], Status[7], CRC[8]
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC);
        s_ow_buses[dev]->write(0xBE); // Read Scratchpad (standard Dallas 1-Wire cmd)
        uint8_t dummy[9];
        for (int i = 0; i < 9; i++) {
            dummy[i] = s_ow_buses[dev]->read();
        }

        if (OneWire::crc8(dummy, 8) != dummy[8]) {
            Serial.printf("[MDC04] Warning: Chip %d on Pin %d online but CRC failed.\n",
                          dev + 1, pins[dev]);
        } else {
            Serial.printf("[MDC04] Chip %d online on Pin %d.\n", dev + 1, pins[dev]);
        }

        s_ow_online[dev] = true;
        s_ow_device_count++;

        // 配置该芯片开启 4 个通道同时转换（Ch_sel = 0x07）
        OW_SetCapChannel(dev, 0x07);
    }

    Serial.printf("[MDC04] Init complete. %d/%d chips online.\n", s_ow_device_count, 3);
    return (s_ow_device_count > 0);
}

/**
 * @brief 读取 12 通道电容值（最多 3 芯片 × 4 通道）
 *        流程：SetCapChannel(ch) → ConvertTC1(0x10) → Read TC1(0xBD, 9字节)
 *        未在线的芯片对应通道保留 0.0f
 * @param out_caps  输出数组，长度 >= 12，按 [chip*4 + ch_idx] 排列
 */
bool MDC04_Read_All_12Channels(float* out_caps) {
    if (s_ow_device_count == 0) return false;

    // 初始化输出数组
    for (int i = 0; i < 12; i++) {
        out_caps[i] = 0.0f;
    }

    // 1. 对所有在线芯片设置 Ch_Sel = 0x07（使能 Ch1~Ch4 4个通道同时转换）
    //    虽然初始化配置过，但在循环读取中重新配置可保证芯片复位后能够自愈
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev]) continue;
        OW_SetCapChannel(dev, 0x07);
    }

    // 2. 触发所有在线芯片同时进行 Convert C（0x66）
    //    Convert C 指令会同时转换 Ch_Sel 中选中的所有电容通道
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev] || !s_ow_buses[dev]) continue;
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0x66); // Convert C
    }

    // 3. 等待转换完成（4通道转换时间叠加，高重复性下 10.5ms * 4 = 42ms，等待 50ms 确保安全完成）
    delay(50);

    // 4. 逐芯片读取转换结果
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev] || !s_ow_buses[dev]) continue;

        // 4a. 读取标准暂存器（Read Scratchpad = 0xBE，9字节）以获取通道 1 的值
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0xBE); // Read Scratchpad

        uint8_t data[9];
        for (int i = 0; i < 9; i++) {
            data[i] = s_ow_buses[dev]->read();
        }

        // CRC-8 (Maxim/Dallas) 校验前 8 字节
        if (OneWire::crc8(data, 8) != data[8]) {
            Serial.printf("[MDC04] CRC error (standard scratchpad): chip %d\n", dev + 1);
            continue;
        }

        // 电容 1 原始值：C1_lsb = byte[2], C1_msb = byte[3]
        uint16_t raw1 = ((uint16_t)data[3] << 8) | data[2];
        float cap1 = MDC04_OutputtoCap(raw1, CapCfg_offset, CapCfg_range);
        out_caps[dev * 4 + 0] = cap1;

        // 4b. 读取扩展暂存器（Read C2-4 = 0xDC，7字节）以获取通道 2, 3, 4 的值
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0xDC); // Read C2-4

        uint8_t data_ext[7];
        for (int i = 0; i < 7; i++) {
            data_ext[i] = s_ow_buses[dev]->read();
        }

        // CRC-8 (Maxim/Dallas) 校验前 6 字节
        if (OneWire::crc8(data_ext, 6) != data_ext[6]) {
            Serial.printf("[MDC04] CRC error (extended scratchpad): chip %d\n", dev + 1);
            continue;
        }

        // 电容 2, 3, 4 原始值
        uint16_t raw2 = ((uint16_t)data_ext[1] << 8) | data_ext[0];
        uint16_t raw3 = ((uint16_t)data_ext[3] << 8) | data_ext[2];
        uint16_t raw4 = ((uint16_t)data_ext[5] << 8) | data_ext[4];

        float cap2 = MDC04_OutputtoCap(raw2, CapCfg_offset, CapCfg_range);
        float cap3 = MDC04_OutputtoCap(raw3, CapCfg_offset, CapCfg_range);
        float cap4 = MDC04_OutputtoCap(raw4, CapCfg_offset, CapCfg_range);

        out_caps[dev * 4 + 1] = cap2;
        out_caps[dev * 4 + 2] = cap3;
        out_caps[dev * 4 + 3] = cap4;

        // 打印调试日志（保持与原格式一致）
        Serial.printf("[MDC04] chip%d ch1 raw=0x%04X cap=%.3f\n", dev + 1, raw1, cap1);
        Serial.printf("[MDC04] chip%d ch2 raw=0x%04X cap=%.3f\n", dev + 1, raw2, cap2);
        Serial.printf("[MDC04] chip%d ch3 raw=0x%04X cap=%.3f\n", dev + 1, raw3, cap3);
        Serial.printf("[MDC04] chip%d ch4 raw=0x%04X cap=%.3f\n", dev + 1, raw4, cap4);
    }
    return true;
}
