#pragma once

// ============================================================
//  串口帧协议层
//  职责：CRC-8-Maxim 校验、SensorFrame 结构体、帧组装与发送
// ============================================================

#include "config.h"
#include <Arduino.h>

#define SENSOR_COUNT    4

/**
 * @brief 传感器读数单次快照数据结构
 *        sensors[0]~sensors[3] 存储各通道已转换的伪电容值（uint16_t，大端序发送）
 */
struct SensorSnapshot {
    uint16_t sensors[SENSOR_COUNT];
};

/**
 * @brief 将 MDC04 的物理电容值转换为 uint16_t 的大端序数值（保留两位小数，乘以 100）
 *
 * @param pf_val MDC04 读取的物理电容值 (pF)
 * @return uint16_t 转换后的电容特征值
 */
uint16_t convert_to_capacitance(float pf_val);
