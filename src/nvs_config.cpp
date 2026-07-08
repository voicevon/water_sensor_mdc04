#include "nvs_config.h"
#include "config.h"
#include <Preferences.h>

// ============================================================
//  NVS 存储实例（内部私有）
// ============================================================
static Preferences s_prefs;

// NVS 命名空间与键名常量，集中管理避免拼写错误
static const char NVS_NAMESPACE[]    = "sensor_map";
static const char NVS_KEY_SSID[]     = "sta_ssid";
static const char NVS_KEY_PASS[]     = "sta_pass";
static const char NVS_KEY_NAME[]     = "sta_name"; // 统一使用 "sta_name"
static const char NVS_KEY_BROKER[]   = "mqtt_broker";
static const char NVS_KEY_PORT[]     = "mqtt_port";
static const char NVS_KEY_POLL_DELAY[]= "poll_delay";

// ============================================================
//  配置项内存缓存（内部私有）
// ============================================================
static String s_sta_ssid     = "";
static String s_sta_password = "";
static String s_device_name  = "";
static String s_mqtt_broker  = "";
static int    s_mqtt_port    = 1883;

// 4路输出通道到12路物理通道的映射索引
static int s_channel_map[4] = {0, 4, 8, 1};

// 12个物理通道的阈值偏移量（全部默认为 50，避免 C++ 聚合初始化仅赋首元素的陷阱）
static int s_threshold_offset[12] = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};

// 轮询各通道测量之间的软件延时（ms）
static int s_poll_delay = 50;

// ============================================================
//  NVS 初始化
// ============================================================
void nvs_config_init() {
    s_prefs.begin(NVS_NAMESPACE, false);

    // 加载 4 路映射配置（如果 NVS 无值，使用 config.h 定义的缺省位置）
    s_channel_map[0] = s_prefs.getInt("map0", SENSOR1_CHIP * 4 + SENSOR1_CHAN);
    s_channel_map[1] = s_prefs.getInt("map1", SENSOR2_CHIP * 4 + SENSOR2_CHAN);
    s_channel_map[2] = s_prefs.getInt("map2", SENSOR3_CHIP * 4 + SENSOR3_CHAN);
    s_channel_map[3] = s_prefs.getInt("map3", SENSOR4_CHIP * 4 + SENSOR4_CHAN);

    // 加载 12 个通道阈值偏移（NVS 无值时默认 50）
    for (int i = 0; i < 12; i++) {
        String key = "thr" + String(i);
        s_threshold_offset[i] = s_prefs.getInt(key.c_str(), 50);
    }

    // 加载 STA WiFi 与 MQTT 配置
    s_sta_ssid    = s_prefs.getString(NVS_KEY_SSID,       FACTORY_WIFI_SSID);
    s_sta_password= s_prefs.getString(NVS_KEY_PASS,       FACTORY_WIFI_PASSWORD);
    s_device_name = s_prefs.getString(NVS_KEY_NAME,       FACTORY_DEVICE_NAME);
    s_mqtt_broker = s_prefs.getString(NVS_KEY_BROKER,     FACTORY_MQTT_BROKER);
    s_mqtt_port   = s_prefs.getInt(NVS_KEY_PORT,          FACTORY_MQTT_PORT);
    s_poll_delay  = s_prefs.getInt(NVS_KEY_POLL_DELAY,    50);

    Serial.printf("[NvsConfig] Loaded Mapping: %d, %d, %d, %d\n",
                  s_channel_map[0], s_channel_map[1], s_channel_map[2], s_channel_map[3]);
    Serial.printf("[NvsConfig] Loaded WiFi STA SSID: %s, Device: %s, MQTT Broker: %s:%d\n",
                  s_sta_ssid.c_str(), s_device_name.c_str(),
                  s_mqtt_broker.c_str(), s_mqtt_port);
}

// ============================================================
//  Getter 实现（函数声明在 web_config.h）
// ============================================================
String get_sta_ssid()                  { return s_sta_ssid; }
String get_sta_password()              { return s_sta_password; }
String get_device_name()               { return s_device_name; }
String get_mqtt_broker()               { return s_mqtt_broker; }
int    get_mqtt_port()                 { return s_mqtt_port; }
int    get_mapped_channel(int idx)     { return (idx >= 0 && idx < 4) ? s_channel_map[idx] : 0; }
int    get_channel_threshold(int ch)   { return (ch >= 0 && ch < 12) ? s_threshold_offset[ch] : 50; }
int    get_poll_delay()                { return s_poll_delay; }

// ============================================================
//  Setter 实现（含变化检测 + NVS 写入）
// ============================================================
bool nvs_set_sta_ssid(const String& val) {
    if (val.length() == 0 || val == s_sta_ssid) return false;
    s_sta_ssid = val;
    s_prefs.putString(NVS_KEY_SSID, val);
    return true;
}

bool nvs_set_sta_password(const String& val) {
    if (val == s_sta_password) return false;
    s_sta_password = val;
    s_prefs.putString(NVS_KEY_PASS, val);
    return true;
}

bool nvs_set_device_name(const String& val) {
    if (val.length() == 0 || val == s_device_name) return false;
    s_device_name = val;
    s_prefs.putString(NVS_KEY_NAME, val);
    return true;
}

bool nvs_set_mqtt_broker(const String& val) {
    if (val.length() == 0 || val == s_mqtt_broker) return false;
    s_mqtt_broker = val;
    s_prefs.putString(NVS_KEY_BROKER, val);
    return true;
}

bool nvs_set_mqtt_port(int val) {
    if (val <= 0 || val == s_mqtt_port) return false;
    s_mqtt_port = val;
    s_prefs.putInt(NVS_KEY_PORT, val);
    return true;
}

bool nvs_set_channel_map(int output_idx, int physical_idx) {
    if (output_idx < 0 || output_idx >= 4) return false;
    if (physical_idx < 0 || physical_idx >= 12) return false;
    if (s_channel_map[output_idx] == physical_idx) return false;
    s_channel_map[output_idx] = physical_idx;
    String key = "map" + String(output_idx);
    s_prefs.putInt(key.c_str(), physical_idx);
    return true;
}

bool nvs_set_threshold_offset(int ch, int offset) {
    if (ch < 0 || ch >= 12) return false;
    if (offset < 1 || offset > 500) return false;
    if (s_threshold_offset[ch] == offset) return false;
    s_threshold_offset[ch] = offset;
    String key = "thr" + String(ch);
    s_prefs.putInt(key.c_str(), offset);
    return true;
}

bool nvs_set_poll_delay(int delay_ms) {
    if (delay_ms < 0 || delay_ms > 1000) return false;
    if (s_poll_delay == delay_ms) return false;
    s_poll_delay = delay_ms;
    s_prefs.putInt(NVS_KEY_POLL_DELAY, delay_ms);
    return true;
}
