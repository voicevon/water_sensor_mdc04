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


// ---- 算法类型配置（ch: 0-11, type: 0=DYNAMIC, 1=DISCRETE, 2=ENVELOPE）----
bool nvs_set_algo_type(int ch, int type);
int  get_algo_type(int ch);

// ---- 离散方差算法：方差触发阈值配置（ch: 0-11, threshold: 0 ~ 100000）----
bool nvs_set_var_threshold(int ch, int threshold);
int  get_var_threshold(int ch);

// ---- 包络算法参数配置 ----
bool nvs_set_env_window(int ch, int window);        // 包络窗口（1~120）
bool nvs_set_env_upper_offset(int ch, int offset);  // 上触发偏置（0~5000）
bool nvs_set_env_lower_offset(int ch, int offset);  // 下恢复偏置（0~5000）

int  get_env_window(int ch);
int  get_env_upper_offset(int ch);
int  get_env_lower_offset(int ch);
