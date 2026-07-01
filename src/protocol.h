#pragma once

// ============================================================
//  串口帧协议层
//  职责：CRC-8-Maxim 校验、SensorFrame 结构体、帧组装与发送
// ============================================================

#include <Arduino.h>

// 帧格式常量
#define FRAME_LEN        11   // 总帧长（字节）：2 帧头 + 8 数据 + 1 CRC
#define FRAME_HEADER_HI  0xAA
#define FRAME_HEADER_LO  0x55
#define CHANNEL_COUNT    4

/**
 * @brief 传感器一帧 of logic data
 *        ch[0]~ch[3] 存储各通道已转换的伪电容值（uint16_t，大端序发送）
 */
struct SensorFrame {
    uint16_t ch[CHANNEL_COUNT];
};

/**
 * @brief 将 MDC04 的物理电容值转换为 uint16_t 的大端序数值（保留两位小数，乘以 100）
 *
 * @param pf_val MDC04 读取的物理电容值 (pF)
 * @return uint16_t 转换后的电容特征值
 */
uint16_t convert_to_capacitance(float pf_val);
