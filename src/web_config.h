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
 * @brief 获取指定芯片的有效通道索引 (chip_idx: 0-2, 返回 0-3)
 */
int get_chip_active_channel(int chip_idx);

/**
 * @brief 更新指定物理通道的实时传感器数据，供网页 API 查询
 */
void web_config_update_sensor(int idx, float raw_val, uint16_t filtered, uint16_t baseline, uint16_t threshold, bool detected);

/**
 * @brief 从 NVS 获取配置的 STA Wi-Fi SSID
 */
String get_sta_ssid();

/**
 * @brief 从 NVS 获取配置的 STA Wi-Fi 密码
 */
String get_sta_password();

/**
 * @brief 获取指定物理通道在内存缓存/NVS中设定的阈值偏移量 (0-11)
 */
int get_channel_threshold(int ch_idx);



/**
 * @brief 从 NVS 获取配置的设备名称 (DEVICE_NAME)
 */
String get_device_name();

/**
 * @brief 从 NVS 获取配置的 MQTT Broker 地址
 */
String get_mqtt_broker();

/**
 * @brief 从 NVS 获取配置的 MQTT Broker 端口
 */
int get_mqtt_port();

/**
 * @brief 获取指定通道的算法类型 (0=DYNAMIC, 1=DISCRETE, 2=ENVELOPE)
 */
int get_algo_type(int ch);

/**
 * @brief 获取离散方差算法的方差触发阈值
 */
int get_var_threshold(int ch);

/**
 * @brief 获取包络算法参数
 */
int get_env_window(int ch);
int get_env_upper_offset(int ch);
int get_env_lower_offset(int ch);
