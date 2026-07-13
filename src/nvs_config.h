#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 NVS 存储并加载所有配置项到内存缓存
 *        由 web_config_init() 调用
 */
void nvs_config_init();

/**
 * @brief 配置写入接口 — 供 REST POST handler 调用
 *        返回 true = 值已更新并写入 NVS；false = 无变化或非法值
 */
// 基础网络配置
bool nvs_set_sta_ssid(const String& val);
bool nvs_set_sta_password(const String& val);
bool nvs_set_device_name(const String& val);
bool nvs_set_mqtt_broker(const String& val);
bool nvs_set_mqtt_port(int val);

// 芯片有效通道配置（chip_idx: 0-2, channel_idx: 0-3）
bool nvs_set_chip_active_channel(int chip_idx, int channel_idx);
int  get_chip_active_channel(int chip_idx);

// 阈值偏移量配置（ch: 0-11, offset: -500 到 500）
bool nvs_set_threshold_offset(int ch, int offset);

// 轮询延时配置（0-1000ms）
bool nvs_set_poll_delay(int delay_ms);
