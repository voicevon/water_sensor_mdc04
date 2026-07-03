#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 Web Server 及其 AP 模式，加载 NVS 中的通道映射
 */
void web_config_init();

/**
 * @brief 在主循环中驱动 Web Server 客户端请求
 */
void web_config_loop();

/**
 * @brief 获取输出通道（0-3）映射到的 12 路物理通道索引（0-11）
 */
int get_mapped_channel(int output_idx);

/**
 * @brief 更新指定物理通道的实时传感器数据，供网页 API 查询
 */
void web_config_update_sensor(int idx, float raw_val, uint16_t filtered, uint16_t baseline, uint16_t threshold, bool detected);

/**
 * @brief 打印当前 Wi-Fi 的工作模式和 IP 地址信息
 */
void print_wifi_status(const char* label);
