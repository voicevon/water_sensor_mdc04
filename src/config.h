#pragma once

// ============================================================
//  全局配置文件 — 所有硬件参数、协议常量、网络配置的唯一来源
//  修改引脚、波特率、WiFi 凭据等只需改此文件
// ============================================================

#include <Arduino.h>

#define SENSOR_COUNT    4

/**
 * @brief 将 MDC04 的物理电容值转换为 uint16_t 的大端序数值（保留两位小数，乘以 100）
 */
inline uint16_t convert_to_capacitance(float pf_val) {
    float val = pf_val * 100.0f;
    if (val < 0.0f) val = 0.0f;
    if (val > 65535.0f) val = 65535.0f;
    return (uint16_t)val;
}

// -------- MDC04 通信引脚定义 --------
// 3颗芯片对应的独立单总线（One-Wire）DQ引脚
#define ONEWIRE_DQ1_PIN 13 // 对应 Chip 1 (ch1-ch3)
#define ONEWIRE_DQ2_PIN 14 // 对应 Chip 2 (ch4-ch6)
#define ONEWIRE_DQ3_PIN 27 // 对应 Chip 3 (ch7-ch9)

// -------- LED 状态指示灯引脚 --------
#define LED_PIN_B        2
#define LED_PIN_A       15


// -------- 采样与发送定时周期 (1 Hz) --------
#define SEND_INTERVAL_MS  1000UL

// -------- 传感器映射配置 --------
// 将 4 个对外输出通道（Sensor 1-4）映射到全局 12 通道数据中
// 格式为：对应的芯片索引（0-2）和芯片内通道索引（0-3）
#define SENSOR1_CHIP  0
#define SENSOR1_CHAN  0  // 映射到 Chip 1 Channel 1

#define SENSOR2_CHIP  1
#define SENSOR2_CHAN  0  // 映射到 Chip 2 Channel 1

#define SENSOR3_CHIP  2
#define SENSOR3_CHAN  0  // 映射到 Chip 3 Channel 1

#define SENSOR4_CHIP  0
#define SENSOR4_CHAN  1  // 映射到 Chip 1 Channel 2 (作为默认第4路占位)

// -------- BLE 广播参数 --------
#define BLE_DEVICE_NAME    "FengBLE"
// Company ID: 0xFFFF（厂商测试标识），BLE 小端序：LSB 在前
#define BLE_COMPANY_ID_LSB 0xFF
#define BLE_COMPANY_ID_MSB 0xFF

// -------- WiFi 网络配置 --------
#define WIFI_SSID       "Perfect"
#define WIFI_PASSWORD   "12344321"

#define WIFI_AP_SSID    "WaterSensor_AP"
#define WIFI_AP_PASSWORD "12344321"

// -------- MQTT Broker & 设备命名配置 --------
#define DEVICE_NAME     "dongzhan"
#define MQTT_BROKER     "voicevon.vicp.io"
#define MQTT_PORT       1883
#define MQTT_CONTROL_TOPIC "water/sensor/start"
#define MQTT_STATUS_TOPIC  "water/sensor/status"
// MQTT 非阻塞重连最小间隔（毫秒）
#define MQTT_RECONNECT_INTERVAL_MS  5000UL
