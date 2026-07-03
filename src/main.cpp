#include <Arduino.h>

#include "config.h"
#include "MDC04.h"
#include "ble_adv.h"
#include "wifi_mqtt.h"
#include "esp_log.h"
#include "Sensor.h"

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
#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
    Serial.printf("One-Wire Pins: DQ1=%d, DQ2=%d, DQ3=%d\n", ONEWIRE_DQ1_PIN, ONEWIRE_DQ2_PIN, ONEWIRE_DQ3_PIN);
#else
    Serial.printf("I2C Pins: SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
#endif
    Serial.println("====================================");

    // 1. 初始化 LED 引脚
    pinMode(LED_PIN_A, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    digitalWrite(LED_PIN_A, LOW);
    digitalWrite(LED_PIN_B, LOW);

#if MDC04_COMM_MODE == MDC04_MODE_ONEWIRE
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
        ESP.restart();
    }
    Serial.println("[MDC04] Configured successfully in One-Wire mode.");
#else
    // 2. 初始化 MDC04 传感器及其 I2C 接口
    bool mdc04_ok = MDC04_Init_All();

    // 3. 扫描 I2C 总线并打印结果，辅助排查物理连接
    Serial.println("[I2C] Scanning I2C bus...");
    uint8_t count = 0;
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[I2C] Device found at address 0x%02X\n", address);
            count++;
        }
    }
    if (count == 0) {
        Serial.println("[I2C] No devices found. Please check wiring and pull-ups!");
    }

    // 4. 判断总线和驱动初始化状态，若失败则直接挂起系统，不启动网络功能
    if (!mdc04_ok || count == 0) {
        Serial.println("[ERROR] MDC04 I2C Init Failed or no device detected! Flashing LED B and restarting...");
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN_B, HIGH);
            delay(100);
            digitalWrite(LED_PIN_B, LOW);
            delay(100);
        }
        ESP.restart();
    }

    /* WDC04/MDC04 芯片初始化参数配置 */
    float Co = 15.0f; // 偏置电容 Co = 15.0 pF
    float Cr = 15.5f; // 反馈电容 Cr = 15.5 pF (满量程范围 [Co-Cr, Co+Cr] 即 [-0.5 pF, 30.5 pF])

    MDC04_SysCfg(MDC04_REPEATABILITY_HIGH);          // 设置高测量精度
    MDC04_Set_Cap_Offset(Co);                        // 写入偏置配置
    MDC04_Set_Cap_FullScale(Cr);                     // 写入量程范围配置
    MDC04_Set_Cap_Channel(CAP_CH1CH2CH3CH4_SEL);     // 开启全部四路通道
    Serial.println("[MDC04] Configured successfully.");
#endif

    // 5. MDC04 初始化成功后，再启动网络通信与广播
    wifi_mqtt_init();
    ble_init();
}

// ============================================================

void loop() {
    unsigned long now = millis();

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
            // 2. 将数据喂入 12 路 Sensor 状态机
            for (int i = 0; i < 12; i++) {
                uint16_t raw_val = convert_to_capacitance(all_caps[i]);
                s_sensors[i].pushRaw(raw_val);
            }
        }

        // 3. 根据映射表，选择 4 路数据进行 BLE 与 MQTT 输出
        uint16_t mapped_sensors[SENSOR_COUNT] = {0};
        if (read_ok) {
            // 获取映射索引
            int mapped_idx1 = SENSOR1_CHIP * 4 + SENSOR1_CHAN;
            int mapped_idx2 = SENSOR2_CHIP * 4 + SENSOR2_CHAN;
            int mapped_idx3 = SENSOR3_CHIP * 4 + SENSOR3_CHAN;
            int mapped_idx4 = SENSOR4_CHIP * 4 + SENSOR4_CHAN;

            mapped_sensors[0] = convert_to_capacitance(all_caps[mapped_idx1]);
            mapped_sensors[1] = convert_to_capacitance(all_caps[mapped_idx2]);
            mapped_sensors[2] = convert_to_capacitance(all_caps[mapped_idx3]);
            mapped_sensors[3] = convert_to_capacitance(all_caps[mapped_idx4]);

            // 4. 并行输出至 BLE 与 MQTT
            ble_update(mapped_sensors);           // → BLE 广播
            if (mqtt_publish(mapped_sensors)) {   // → MQTT JSON (成功发布时触发 LED A 点亮 100ms)
                s_led_a_off_time = now + 100;
                digitalWrite(LED_PIN_A, HIGH);
            }
        }

        // 5. 本地 USB 串口 12 路诊断日志与状态打印
        if (read_ok) {
            Serial.println("------------------------------------------------------------------------------------------------");
            Serial.println("CH_IDX  CHIP_ID  CHAN_ID  RAW_VAL  FILTERED  BASELINE  THRESHOLD  STATE");
            for (int i = 0; i < 12; i++) {
                // 如果是 I2C 模式，仅有前4路有效，其余跳过打印或显示为0
#if MDC04_COMM_MODE != MDC04_MODE_ONEWIRE
                if (i >= 4) break;
#endif
                int chip_id = i / 4 + 1;
                int chan_id = i % 4 + 1;
                Serial.printf("  %-4d    Chip %d   Chan %d    %-7.2f  %-8u  %-8u  %-9u  %s\n",
                              i, chip_id, chan_id, all_caps[i],
                              s_sensors[i].getFiltered(), s_sensors[i].getBaseline(),
                              s_sensors[i].getThreshold(), s_sensors[i].isDetected() ? "WATER" : "DRY");
            }
            Serial.println("------------------------------------------------------------------------------------------------");
        }
    }
}
