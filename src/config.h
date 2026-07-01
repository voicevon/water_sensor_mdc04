#pragma once

// ============================================================
//  全局配置文件 — 所有硬件参数、协议常量、网络配置的唯一来源
//  修改引脚、波特率、WiFi 凭据等只需改此文件
// ============================================================

// -------- MDC04 I2C 引脚定义 --------
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 16

// -------- LED 状态指示灯引脚 --------
#define LED_PIN_A        2
#define LED_PIN_B       15


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
