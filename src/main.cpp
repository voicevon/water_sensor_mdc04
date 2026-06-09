#include <Arduino.h>
#include "MDC04.h"

/* I2C 引脚定义（可根据实际板级连接修改） */
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

/* 自动计算 Modbus CRC-16 校验值 */
uint16_t calculate_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void setup() {
    /* 初始化串口，波特率 115200 */
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    /* 初始化 I2C 总线并设置速率 */
    if (!MDC04_Init(I2C_SDA_PIN, I2C_SCL_PIN)) {
        // I2C 初始化失败，程序在此处循环（或者可以做其他异常处理）
        while (1) {
            delay(1000);
        }
    }

    /* WDC04/MDC04 芯片初始化参数配置 */
    float Co = 15.0f; // 偏置电容 Co = 15.0 pF
    float Cr = 15.5f; // 反馈电容 Cr = 15.5 pF (满量程范围 [Co-Cr, Co+Cr] 即 [-0.5 pF, 30.5 pF])

    MDC04_SysCfg(MDC04_REPEATABILITY_HIGH);          // 设置高测量精度
    MDC04_Set_Cap_Offset(Co);                        // 写入偏置配置
    MDC04_Set_Cap_FullScale(Cr);                     // 写入量程范围配置
    MDC04_Set_Cap_Channel(CAP_CH1CH2CH3CH4_SEL);     // 开启全部四路通道
}

void loop() {
    float fcap1 = 0.0f;
    float fcap2 = 0.0f;
    float fcap3 = 0.0f;
    float fcap4 = 0.0f;

    /* 1. 温度测量并转换（配合芯片内部校准） */
    MDC04_StartTempConvert();

    /* 2. 触发四通道电容测量转换 */
    if (MDC04_ConvertCap()) {
        /* 高精度精度单通道转换约10.5ms，四通道等待 44 ms */
        delay(44);

        /* 读取电容数值 */
        MDC04_ReadCap(&fcap1, &fcap2, &fcap3, &fcap4);

        /* 3. 封装并发送协议帧 */
        uint8_t frame[21];
        frame[0] = 0xAA; // 帧头高字节
        frame[1] = 0x55; // 帧头低字节
        frame[2] = 0x10; // 数据负载长度 (16 字节)

        // 填充四路浮点数电容值 (ESP32为小端格式)
        memcpy(&frame[3], &fcap1, 4);
        memcpy(&frame[7], &fcap2, 4);
        memcpy(&frame[11], &fcap3, 4);
        memcpy(&frame[15], &fcap4, 4);

        // 计算 CRC-16 Modbus (校验范围为 Length + Payload，即偏移量 2 至 18 共 17 字节)
        uint16_t crc = calculate_crc16(&frame[2], 17);
        frame[19] = (uint8_t)(crc & 0xFF);        // CRC 低字节
        frame[20] = (uint8_t)((crc >> 8) & 0xFF); // CRC 高字节

        // 串口二进制输出
        Serial.write(frame, sizeof(frame));
    }

    /* 采样周期为 1 秒 */
    delay(1000);
}
