#pragma once

// ============================================================
//  网络通信层（WiFi + MQTT）
//  职责：WiFi 初始化、MQTT 非阻塞重连、JSON Payload 发布
// ============================================================

#include "protocol.h" // SensorFrame

/**
 * @brief 初始化 WiFi 连接与 MQTT Broker 配置
 *        WiFi 连接采用最多 20 次 × 500ms 的阻塞等待，超时后继续启动并在后台重试。
 *        网络凭据与 Broker 地址由 config.h 中的宏定义决定。
 */
void wifi_mqtt_init();

/**
 * @brief 在主循环中维持 WiFi / MQTT 心跳（非阻塞）
 *        - 若 WiFi 已连接且 MQTT 断开，则尝试非阻塞重连（5s 节流）
 *        - 若 MQTT 已连接，则调用 mqttClient.loop() 处理订阅消息与保活包
 *
 * @param current_time  millis() 当前时间戳，避免重复调用
 */
void wifi_mqtt_loop(unsigned long current_time);

/**
 * @brief 将传感器数据组装为 JSON 字符串并发布到 MQTT Topic
 *
 * JSON 格式（物理排线倒序映射，ch0_val→ch1，ch3_val→ch4）：
 *   {"ch4":<ch3_val>, "ch3":<ch2_val>, "ch2":<ch1_val>, "ch1":<ch0_val>}
 *
 * 仅在 MQTT 已连接时发布，断线时静默跳过。
 *
 * @param frame 传感器数据帧
 * @return bool 是否成功发布了数据
 */
bool mqtt_publish(const SensorFrame &frame);

/**
 * @brief 获取 MQTT 客户端当前的连接状态
 * 
 * @return bool 如果已连接返回 true，否则返回 false
 */
bool mqtt_is_connected();

/**
 * @brief 获取 WiFi 当前的连接状态
 * 
 * @return bool 如果已连接返回 true，否则返回 false
 */
bool wifi_is_connected();
