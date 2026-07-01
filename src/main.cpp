#include <Arduino.h>

#include "config.h"
#include "MDC04.h"
#include "protocol.h"
#include "ble_adv.h"
#include "wifi_mqtt.h"
#include "esp_log.h"

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
    Serial.printf("I2C Pins: SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("====================================");

    // 初始化 WiFi 与 MQTT
    wifi_mqtt_init();

    // 初始化 BLE 广播
    ble_init();

    // 初始化 LED 引脚
    pinMode(LED_PIN_A, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    digitalWrite(LED_PIN_A, LOW);
    digitalWrite(LED_PIN_B, LOW);



    // 初始化 MDC04 传感器及其 I2C 接口
    if (!MDC04_Init(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("[ERROR] MDC04 I2C Init Failed! System halted.");
        while (1) {
            delay(1000);
        }
    }

    // 扫描 I2C 总线，输出已连接的设备地址
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

    /* WDC04/MDC04 芯片初始化参数配置 */
    float Co = 15.0f; // 偏置电容 Co = 15.0 pF
    float Cr = 15.5f; // 反馈电容 Cr = 15.5 pF (满量程范围 [Co-Cr, Co+Cr] 即 [-0.5 pF, 30.5 pF])

    MDC04_SysCfg(MDC04_REPEATABILITY_HIGH);          // 设置高测量精度
    MDC04_Set_Cap_Offset(Co);                        // 写入偏置配置
    MDC04_Set_Cap_FullScale(Cr);                     // 写入量程范围配置
    MDC04_Set_Cap_Channel(CAP_CH1CH2CH3CH4_SEL);     // 开启全部四路通道
    Serial.println("[MDC04] Configured successfully.");
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

        float fcap1 = 0.0f;
        float fcap2 = 0.0f;
        float fcap3 = 0.0f;
        float fcap4 = 0.0f;

        // 1. 触发并读取 MDC04 的四通道电容值
        MDC04_StartTempConvert(); // 温度测量转换配合芯片校准
        if (MDC04_ConvertCap()) {
            /* 高精度单通道转换约 10.5ms，四通道等待 44 ms */
            delay(44);

            /* 读取电容数值 */
            MDC04_ReadCap(&fcap1, &fcap2, &fcap3, &fcap4);
        }

        // 2. 将 float 电容值转换为 uint16_t (保留两位小数，乘以 100)
        SensorFrame frame;
        frame.ch[0] = convert_to_capacitance(fcap1);
        frame.ch[1] = convert_to_capacitance(fcap2);
        frame.ch[2] = convert_to_capacitance(fcap3);
        frame.ch[3] = convert_to_capacitance(fcap4);

        // 3. 并行输出
        ble_update(frame);           // → BLE 广播
        if (mqtt_publish(frame)) {   // → MQTT JSON (成功发布时触发 LED A 点亮 100ms)
            s_led_a_off_time = now + 100;
            digitalWrite(LED_PIN_A, HIGH);
        }

        // 4. USB 串口诊断日志已开启，方便本地串口调试
        Serial.printf("[MDC04] Ch1:%.2f pF (%u) | Ch2:%.2f pF (%u) | Ch3:%.2f pF (%u) | Ch4:%.2f pF (%u)\n",
                      fcap1, frame.ch[0], fcap2, frame.ch[1], fcap3, frame.ch[2], fcap4, frame.ch[3]);
    }
}
