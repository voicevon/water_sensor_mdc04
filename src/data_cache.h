#pragma once

#include <Arduino.h>

// ============================================================
//  12路物理通道实时数据缓存结构
// ============================================================
struct SensorDataCache {
    float    raw_val;    // MDC04 原始电容值（pF）
    uint16_t filtered;   // 滑动平均滤波后值
    uint16_t baseline;   // 慢速基准值
    uint16_t threshold;  // 当前触发阈值
    bool     detected;   // 是否检测到有水
};

/**
 * @brief 更新指定物理通道的实时传感器数据，供网页 /api/data 查询
 * @param idx 物理通道索引（0-11）
 */
void data_cache_update_sensor(int idx, float raw_val, uint16_t filtered,
                               uint16_t baseline, uint16_t threshold, bool detected);

/**
 * @brief 获取指定物理通道的数据缓存（内部服务层使用）
 */
const SensorDataCache& data_cache_get_sensor(int idx);
