#include <Arduino.h>

#include "config.h"
#include "MDC04.h"
#include "ble_adv.h"
#include "wifi_mqtt.h"
#include "esp_log.h"
#include "Sensor.h"
#include "web_config.h"

// 实例化 12 个 Sensor 状态机（ID 为 0-11，阈值偏移采用默认的 50）
static Sensor s_sensors[12] = {
    Sensor(0), Sensor(1), Sensor(2), Sensor(3),
    Sensor(4), Sensor(5), Sensor(6), Sensor(7),
    Sensor(8), Sensor(9), Sensor(10), Sensor(11)
};

// ============================================================
//  顶层调度骨架
//  - setup() : 硬件与通信初始化，依次启动各子系统
//  - loop()  : 维持网络心跳 + 1Hz 定时采样 (MDC04) → BLE与MQTT输出
// ============================================================
uint32_t g_mqtt_publish_interval_ms = 1000UL;
unsigned long s_last_mqtt_publish_time = 0;

static unsigned long s_last_send_time    = 0;
static unsigned long s_led_a_off_time    = 0;     // LED A (GPIO 2) 自动本轮熄灭的时间戳 (0表示常灭)
static bool          s_last_led_b_state  = false; // 记录 LED B 的上一次状态，避免高频写入 GPIO

// ============================================================

void setup() {
    // 彻底关闭 ESP-IDF 的底层日志，避免干扰 WiFi 调试
    esp_log_level_set("*", ESP_LOG_NONE);

    // 初始化本地调试串口
    Serial.begin(115200);
    delay(500); // 等待串口稳定
    Serial.println("\n====================================");
    Serial.println("ESP32 Water MDC04 Sensor Node Start");
    Serial.printf("One-Wire Pins: DQ1=%d, DQ2=%d, DQ3=%d\n", ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN);
    Serial.println("====================================");

    // 1. 初始化 LED 引脚
    pinMode(LED_PIN_A, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    digitalWrite(LED_PIN_A, LOW);
    digitalWrite(LED_PIN_B, LOW);

    // 2. 初始化 MDC04 传感器（单总线模式）
    bool mdc04_ok = MDC04_Init_All();
    if (!mdc04_ok) {
        Serial.println("[ERROR] MDC04 One-Wire Init Failed! Flashing LED B and restarting...");
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN_B, HIGH);
            delay(100);
            digitalWrite(LED_PIN_B, LOW);
            delay(100);
        }
        // ESP.restart();
    }
    Serial.println("[MDC04] Configured successfully in One-Wire mode.");
 
    // 3. 启动 Web 配置服务器与系统初始化
    web_config_init();
 
    // 4. 动态同步从 NVS 中恢复的 12 路阈值偏移和算法类型
    for (int i = 0; i < 12; i++) {
        s_sensors[i].setThresholdOffset(get_channel_threshold(i));
        // 新增：从 NVS 恢复算法类型，并应用对应算法参数
        s_sensors[i].setAlgoType((AlgoType)get_algo_type(i));
        s_sensors[i].setVarThreshold(get_var_threshold(i));
        s_sensors[i].setEnvWindow(get_env_window(i));
        s_sensors[i].setEnvDryWindowUp(get_env_dry_up(i));
        s_sensors[i].setEnvDryWindowDown(get_env_dry_down(i));
        s_sensors[i].setEnvUpperOffset(get_env_upper_offset(i));
        s_sensors[i].setEnvLowerOffset(get_env_lower_offset(i));
    }
    
    // 5. 启动网络通信与 BLE 广播
    wifi_mqtt_init();
    ble_init();
}

// ============================================================

void loop() {
    unsigned long now = millis();

    // 驱动 Web Server
    web_config_loop();

    // 维持 WiFi / MQTT 心跳（非阻塞重连）
    wifi_mqtt_loop(now);

    // LED A: MQTT 发送指示灯。平时常灭。每发送成功一个 MQTT 数据包，点亮 100ms 后关闭。
    if (s_led_a_off_time > 0) {
        if (now >= s_led_a_off_time) {
            s_led_a_off_time = 0;
            digitalWrite(LED_PIN_A, LOW);
        } else {
            digitalWrite(LED_PIN_A, HIGH);
        }
    } else {
        digitalWrite(LED_PIN_A, LOW);
    }

    // LED B 状态指示灯逻辑：
    // - WiFi 未连接：保持常灭 (LOW)
    // - WiFi 已连接，但 MQTT 未连接：2秒周期，50% 占空比闪烁 (1s 亮 / 1s 灭)
    // - WiFi 与 MQTT 均已连接：1秒周期，50% 占空比闪烁 (0.5s 亮 / 0.5s 灭)
    bool led_b_state = false;
    if (wifi_is_connected()) {
        if (mqtt_is_connected()) {
            led_b_state = ((now % 1000) < 500); // 1000ms 周期快闪
        } else {
            led_b_state = ((now % 2000) < 1000); // 2000ms 周期中闪
        }
    } else {
        led_b_state = false; // WiFi 未连接时常灭
    }

    if (led_b_state != s_last_led_b_state) {
        s_last_led_b_state = led_b_state;
        digitalWrite(LED_PIN_B, led_b_state ? HIGH : LOW);
    }

    // 1Hz 非阻塞定时器
    if (now - s_last_send_time >= SEND_INTERVAL_MS) {
        s_last_send_time = now;

        // 1. 读取 MDC04 的全部 12 个通道电容值
        float all_caps[12] = {0.0f};
        bool read_ok = MDC04_Read_All_12Channels(all_caps);

        if (!read_ok) {
            Serial.println("[main] Warning: MDC04 Read 12 Channels failed!");
        } else {
            // 2. 将数据馈入 12 路 Sensor 状态机，并同步给 Web config 缓存
            for (int i = 0; i < 12; i++) {
                // 实时应用网页端配置（阈值偏移 + 算法类型 + 算法参数）
                s_sensors[i].setThresholdOffset(get_channel_threshold(i));
                s_sensors[i].setAlgoType((AlgoType)get_algo_type(i));
                s_sensors[i].setVarThreshold(get_var_threshold(i));
                s_sensors[i].setEnvWindow(get_env_window(i));
                s_sensors[i].setEnvDryWindowUp(get_env_dry_up(i));
                s_sensors[i].setEnvDryWindowDown(get_env_dry_down(i));
                s_sensors[i].setEnvUpperOffset(get_env_upper_offset(i));
                s_sensors[i].setEnvLowerOffset(get_env_lower_offset(i));

                uint16_t raw_val = convert_to_capacitance(all_caps[i]);
                s_sensors[i].pushRaw(raw_val);
                web_config_update_sensor(i, all_caps[i], s_sensors[i].getFiltered(), s_sensors[i].getBaseline(), s_sensors[i].getThreshold(), s_sensors[i].isDetected());
            }
        }

        // 3. 根据芯片有效通道配置，选择 3 路数据进行 BLE 与 MQTT 输出
        uint16_t mapped_sensors[SENSOR_COUNT] = {0};
        if (read_ok) {
            // 获取各芯片的有效通道索引 (0-3)
            int chip1_ch = get_chip_active_channel(0);
            int chip2_ch = get_chip_active_channel(1);
            int chip3_ch = get_chip_active_channel(2);

            // 物理通道索引：芯片 1 为 0+ch, 芯片 2 为 4+ch, 芯片 3 为 8+ch
            int mapped_idx1 = 0 + chip1_ch;
            int mapped_idx2 = 4 + chip2_ch;
            int mapped_idx3 = 8 + chip3_ch;

            mapped_sensors[0] = convert_to_capacitance(all_caps[mapped_idx1]);
            mapped_sensors[1] = convert_to_capacitance(all_caps[mapped_idx2]);
            mapped_sensors[2] = convert_to_capacitance(all_caps[mapped_idx3]);

            bool mapped_states[SENSOR_COUNT] = {
                s_sensors[mapped_idx1].isDetected(),
                s_sensors[mapped_idx2].isDetected(),
                s_sensors[mapped_idx3].isDetected()
            };

            // 计算有水状态字节 (低3位分别对应 sensor1~sensor3 的开关水判定)
            uint8_t state_byte = 0;
            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (mapped_states[i]) {
                    state_byte |= (1 << i);
                }
            }

            // 4. 并行输出至 BLE 与 MQTT
            ble_update(mapped_sensors, mapped_states);           // → BLE 广播 (始终保持 1Hz 广播)
            
            if (now - s_last_mqtt_publish_time >= g_mqtt_publish_interval_ms) {
                s_last_mqtt_publish_time = now;
                if (mqtt_publish(mapped_sensors, state_byte)) {      // → MQTT JSON (成功发布时触发 LED A 点亮 100ms)
                    s_led_a_off_time = now + 100;
                    digitalWrite(LED_PIN_A, HIGH);
                }
            }
        }

        // 5. 本地 USB 串口 12 路诊断日志与状态打印
        // if (read_ok) {
        //     Serial.println("------------------------------------------------------------------------------------------------");
        //     Serial.println("CH_IDX  CHIP_ID  CHAN_ID  RAW_VAL  FILTERED  BASELINE  THRESHOLD  STATE");
        //     for (int i = 0; i < 12; i++) {
        //         int chip_id = i / 4 + 1;
        //         int chan_id = i % 4 + 1;
        //         Serial.printf("  %-4d    Chip %d   Chan %d    %-7.2f  %-8u  %-8u  %-9u  %s\n",
        //                       i, chip_id, chan_id, all_caps[i],
        //                       s_sensors[i].getFiltered(), s_sensors[i].getBaseline(),
        //                       s_sensors[i].getThreshold(), s_sensors[i].isDetected() ? "WATER" : "DRY");
        //     }
        //     Serial.println("------------------------------------------------------------------------------------------------");
        // }
    }
}
