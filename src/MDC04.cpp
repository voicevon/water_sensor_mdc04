#include "MDC04.h"
#include "config.h"
#include "web_config.h"
#include <OneWire.h>

/* ============================================================
 *  OneWire 总线实例（最多 3 颗芯片，每颗一条独立总线）
 * ============================================================ */
// 静态分配 OneWire 实例，避免 dynamic new 造成内存泄漏
static OneWire s_ow_bus0(ONEWIRE_DQ1_PIN);
static OneWire s_ow_bus1(ONEWIRE_DQ2_PIN);
static OneWire s_ow_bus2(ONEWIRE_DQ3_PIN);
static OneWire* s_ow_buses[3] = {&s_ow_bus0, &s_ow_bus1, &s_ow_bus2};
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

        // 配置该芯片开启指定的单通道转换（Ch_sel = active_channel + 1）
        int active_chan = get_chip_active_channel(dev);
        OW_SetCapChannel(dev, active_chan + 1);
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

    // 1. 对所有在线芯片重新设置 Ch_Sel（保证故障恢复自愈）
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev]) continue;
        int active_chan = get_chip_active_channel(dev);
        OW_SetCapChannel(dev, active_chan + 1);
    }

    // 2. 触发所有在线芯片同时进行 Convert C（0x66）
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev] || !s_ow_buses[dev]) continue;
        s_ow_buses[dev]->reset();
        s_ow_buses[dev]->write(0xCC); // Skip ROM
        s_ow_buses[dev]->write(0x66); // Convert C
    }

    // 3. 等待转换完成（单通道高重复性转换时长为 10.5ms，等待 20ms 确保安全完成）
    delay(20);

    // 4. 逐芯片读取配置通道的转换结果
    for (int dev = 0; dev < 3; dev++) {
        if (!s_ow_online[dev] || !s_ow_buses[dev]) continue;

        int active_chan = get_chip_active_channel(dev);

        if (active_chan == 0) {
            // 读取标准暂存器（Read Scratchpad = 0xBE，9字节）以获取通道 1 的值
            s_ow_buses[dev]->reset();
            s_ow_buses[dev]->write(0xCC); // Skip ROM
            s_ow_buses[dev]->write(0xBE); // Read Scratchpad

            uint8_t data[9];
            for (int i = 0; i < 9; i++) {
                data[i] = s_ow_buses[dev]->read();
            }

            if (OneWire::crc8(data, 8) != data[8]) {
                Serial.printf("[MDC04] CRC error (standard scratchpad): chip %d\n", dev + 1);
                continue;
            }

            uint16_t raw1 = ((uint16_t)data[3] << 8) | data[2];
            out_caps[dev * 4 + 0] = MDC04_OutputtoCap(raw1, CapCfg_offset, CapCfg_range);
        } else {
            // 读取扩展暂存器（Read C2-4 = 0xDC，7字节）以获取通道 2, 3, 4 的值
            s_ow_buses[dev]->reset();
            s_ow_buses[dev]->write(0xCC); // Skip ROM
            s_ow_buses[dev]->write(0xDC); // Read C2-4

            uint8_t data_ext[7];
            for (int i = 0; i < 7; i++) {
                data_ext[i] = s_ow_buses[dev]->read();
            }

            if (OneWire::crc8(data_ext, 6) != data_ext[6]) {
                Serial.printf("[MDC04] CRC error (extended scratchpad): chip %d\n", dev + 1);
                continue;
            }

            if (active_chan == 1) {
                uint16_t raw2 = ((uint16_t)data_ext[1] << 8) | data_ext[0];
                out_caps[dev * 4 + 1] = MDC04_OutputtoCap(raw2, CapCfg_offset, CapCfg_range);
            } else if (active_chan == 2) {
                uint16_t raw3 = ((uint16_t)data_ext[3] << 8) | data_ext[2];
                out_caps[dev * 4 + 2] = MDC04_OutputtoCap(raw3, CapCfg_offset, CapCfg_range);
            } else if (active_chan == 3) {
                uint16_t raw4 = ((uint16_t)data_ext[5] << 8) | data_ext[4];
                out_caps[dev * 4 + 3] = MDC04_OutputtoCap(raw4, CapCfg_offset, CapCfg_range);
            }
        }
    }
    return true;
}
