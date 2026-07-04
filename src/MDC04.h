#ifndef _MDC04_H_
#define _MDC04_H_

#include <Arduino.h>
#include <OneWire.h>

/* ============================================================
 *  MDC04 OneWire 驱动接口
 *  通信方式：3 颗芯片，每颗一条独立 OneWire 总线
 * ============================================================ */

/**
 * @brief 初始化 3 条独立 OneWire 总线，检测各芯片在线状态
 * @return true：全部芯片检测到 presence pulse；false：有芯片未响应
 */
bool MDC04_Init_All(void);

/**
 * @brief 读取 12 通道电容值（3 芯片 × 4 通道轮询）
 * @param out_caps  输出浮点数组（长度 >= 12），按 [chip*4 + ch_idx] 排列
 *                  例：out_caps[0]~[3] = Chip1 Ch1~Ch4
 *                      out_caps[4]~[7] = Chip2 Ch1~Ch4
 *                      out_caps[8]~[11]= Chip3 Ch1~Ch4
 * @return true：读取成功；false：无芯片在线
 */
bool MDC04_Read_All_12Channels(float* out_caps);

#endif /* _MDC04_H_ */
