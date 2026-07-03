#pragma once

// ============================================================
//  全局配置文件 — 所有硬件参数、协议常量、网络配置的唯一来源
//  修改引脚、波特率、WiFi 凭据等只需改此文件
// ============================================================

// -------- MDC04 通信模式与引脚定义 --------
#define MDC04_MODE_I2C       0
#define MDC04_MODE_ONEWIRE   1

// 模式选择：MDC04_MODE_I2C 或 MDC04_MODE_ONEWIRE
#define MDC04_COMM_MODE      MDC04_MODE_ONEWIRE

// I2C 模式下的引脚定义
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 16

// 3颗芯片对应的独立单总线（One-Wire）DQ引脚
#define ONEWIRE_DQ1_PIN 13 // 对应 Chip 1 (ch1-ch3)
#define ONEWIRE_DQ2_PIN 14 // 对应 Chip 2 (ch4-ch6)
#define ONEWIRE_DQ3_PIN 27 // 对应 Chip 3 (ch7-ch9)

// -------- LED 状态指示灯引脚 --------
#define LED_PIN_B        2
#define LED_PIN_A       15


// -------- 采样与发送定时周期 (1 Hz) --------
#define SEND_INTERVAL_MS  1000UL

// -------- BLE 广播参数 --------
#define BLE_DEVICE_NAME    "FengBLE"
// Company ID: 0xFFFF（厂商测试标识），BLE 小端序：LSB 在前
#define BLE_COMPANY_ID_LSB 0xFF
#define BLE_COMPANY_ID_MSB 0xFF

// -------- WiFi 网络配置 --------
#define WIFI_SSID       "Perfect"
#define WIFI_PASSWORD   "12344321"

// -------- MQTT Broker & 设备命名配置 --------
#define DEVICE_NAME     "dongzhan"
#define MQTT_BROKER     "voicevon.vicp.io"
#define MQTT_PORT       1883
#define MQTT_CONTROL_TOPIC "water/sensor/start"
#define MQTT_STATUS_TOPIC  "water/sensor/status"
// MQTT 非阻塞重连最小间隔（毫秒）
#define MQTT_RECONNECT_INTERVAL_MS  5000UL
