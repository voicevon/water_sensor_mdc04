#include "nvs_config.h"
#include "config.h"
#include <Preferences.h>

// ============================================================
//  NVS 存储实例（内部私有）
// ============================================================
static Preferences s_prefs;

// NVS 命名空间与键名常量，集中管理避免拼写错误
static const char NVS_NAMESPACE[]     = "sensor_map";
static const char NVS_KEY_SSID[]      = "sta_ssid";
static const char NVS_KEY_PASS[]      = "sta_pass";
static const char NVS_KEY_NAME[]      = "sta_name";
static const char NVS_KEY_BROKER[]    = "mqtt_broker";
static const char NVS_KEY_PORT[]      = "mqtt_port";

// ============================================================
//  配置项内存缓存（内部私有）
// ============================================================
static String s_sta_ssid     = "";
static String s_sta_password = "";
static String s_device_name  = "";
static String s_mqtt_broker  = "";
static int    s_mqtt_port    = 1883;

// 3颗芯片对应的有效通道（默认全为通道 1，即索引 0）
static int s_chip_active_channels[3] = {0, 0, 0};

// 12个物理通道的阈值偏移量（全部默认为 50）
static int s_threshold_offset[12] = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};

// ---- 算法类型缓存（0=DYNAMIC, 1=DISCRETE, 2=ENVELOPE）----
static int s_algo_type[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// ---- 离散方差阈值缓存（默认 5000）----
static int s_var_threshold[12] = {5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000};

// ---- 包络算法参数缓存 ----
static int s_env_window[12]        = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
static int s_env_upper_offset[12]  = {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500};
static int s_env_lower_offset[12]  = {300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300};

// ============================================================
//  NVS 初始化
// ============================================================
void nvs_config_init() {
    s_prefs.begin(NVS_NAMESPACE, false);

    // 加载 3 颗芯片的有效通道配置（默认值为 0，代表通道 1）
    s_chip_active_channels[0] = s_prefs.getInt("chip1_ch", 0);
    s_chip_active_channels[1] = s_prefs.getInt("chip2_ch", 0);
    s_chip_active_channels[2] = s_prefs.getInt("chip3_ch", 0);

    // 加载 12 个通道的各项配置
    for (int i = 0; i < 12; i++) {
        // 阈值偏移（键 thr0~thr11）
        s_threshold_offset[i] = s_prefs.getInt(("thr" + String(i)).c_str(), 50);
        // 算法类型（键 al0~al11）
        s_algo_type[i]        = s_prefs.getInt(("al" + String(i)).c_str(), 0);
        // 离散方差阈值（键 vt0~vt11）
        s_var_threshold[i]    = s_prefs.getInt(("vt" + String(i)).c_str(), 5000);
        // 包络参数（键 ew/eu/el + 编号）
        s_env_window[i]       = s_prefs.getInt(("ew" + String(i)).c_str(), 30);
        s_env_upper_offset[i] = s_prefs.getInt(("eu" + String(i)).c_str(), 500);
        s_env_lower_offset[i] = s_prefs.getInt(("el" + String(i)).c_str(), 300);
    }

    // 加载 STA WiFi 与 MQTT 配置
    s_sta_ssid    = s_prefs.getString(NVS_KEY_SSID,       FACTORY_WIFI_SSID);
    s_sta_password= s_prefs.getString(NVS_KEY_PASS,       FACTORY_WIFI_PASSWORD);
    s_device_name = s_prefs.getString(NVS_KEY_NAME,       FACTORY_DEVICE_NAME);
    s_mqtt_broker = s_prefs.getString(NVS_KEY_BROKER,     FACTORY_MQTT_BROKER);
    s_mqtt_port   = s_prefs.getInt(NVS_KEY_PORT,          FACTORY_MQTT_PORT);

    Serial.printf("[NvsConfig] Loaded Chip Active Channels: %d, %d, %d\n",
                  s_chip_active_channels[0], s_chip_active_channels[1], s_chip_active_channels[2]);
    Serial.printf("[NvsConfig] Loaded WiFi STA SSID: %s, Device: %s, MQTT Broker: %s:%d\n",
                  s_sta_ssid.c_str(), s_device_name.c_str(),
                  s_mqtt_broker.c_str(), s_mqtt_port);
}

// ============================================================
//  Getter 实现
// ============================================================
String get_sta_ssid()                       { return s_sta_ssid; }
String get_sta_password()                   { return s_sta_password; }
String get_device_name()                    { return s_device_name; }
String get_mqtt_broker()                    { return s_mqtt_broker; }
int    get_mqtt_port()                      { return s_mqtt_port; }
int    get_chip_active_channel(int chip_idx){ return (chip_idx >= 0 && chip_idx < 3) ? s_chip_active_channels[chip_idx] : 0; }
int    get_channel_threshold(int ch)        { return (ch >= 0 && ch < 12) ? s_threshold_offset[ch] : 50; }
int    get_algo_type(int ch)                { return (ch >= 0 && ch < 12) ? s_algo_type[ch] : 0; }
int    get_var_threshold(int ch)            { return (ch >= 0 && ch < 12) ? s_var_threshold[ch] : 5000; }
int    get_env_window(int ch)               { return (ch >= 0 && ch < 12) ? s_env_window[ch] : 30; }
int    get_env_upper_offset(int ch)         { return (ch >= 0 && ch < 12) ? s_env_upper_offset[ch] : 500; }
int    get_env_lower_offset(int ch)         { return (ch >= 0 && ch < 12) ? s_env_lower_offset[ch] : 300; }

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

bool nvs_set_chip_active_channel(int chip_idx, int channel_idx) {
    if (chip_idx < 0 || chip_idx >= 3) return false;
    if (channel_idx < 0 || channel_idx >= 4) return false;
    if (s_chip_active_channels[chip_idx] == channel_idx) return false;
    s_chip_active_channels[chip_idx] = channel_idx;
    String key = "chip" + String(chip_idx + 1) + "_ch";
    s_prefs.putInt(key.c_str(), channel_idx);
    return true;
}

bool nvs_set_threshold_offset(int ch, int offset) {
    if (ch < 0 || ch >= 12) return false;
    if (offset < -500 || offset > 500) return false;
    if (s_threshold_offset[ch] == offset) return false;
    s_threshold_offset[ch] = offset;
    s_prefs.putInt(("thr" + String(ch)).c_str(), offset);
    return true;
}

bool nvs_set_algo_type(int ch, int type) {
    if (ch < 0 || ch >= 12) return false;
    if (type < 0 || type > 2) return false;
    if (s_algo_type[ch] == type) return false;
    s_algo_type[ch] = type;
    s_prefs.putInt(("al" + String(ch)).c_str(), type);
    return true;
}

bool nvs_set_var_threshold(int ch, int threshold) {
    if (ch < 0 || ch >= 12) return false;
    if (threshold < 0 || threshold > 100000) return false;
    if (s_var_threshold[ch] == threshold) return false;
    s_var_threshold[ch] = threshold;
    s_prefs.putInt(("vt" + String(ch)).c_str(), threshold);
    return true;
}

bool nvs_set_env_window(int ch, int window) {
    if (ch < 0 || ch >= 12) return false;
    if (window < 1 || window > 120) return false;
    if (s_env_window[ch] == window) return false;
    s_env_window[ch] = window;
    s_prefs.putInt(("ew" + String(ch)).c_str(), window);
    return true;
}

bool nvs_set_env_upper_offset(int ch, int offset) {
    if (ch < 0 || ch >= 12) return false;
    if (offset < 0 || offset > 5000) return false;
    if (s_env_upper_offset[ch] == offset) return false;
    s_env_upper_offset[ch] = offset;
    s_prefs.putInt(("eu" + String(ch)).c_str(), offset);
    return true;
}

bool nvs_set_env_lower_offset(int ch, int offset) {
    if (ch < 0 || ch >= 12) return false;
    if (offset < 0 || offset > 5000) return false;
    if (s_env_lower_offset[ch] == offset) return false;
    s_env_lower_offset[ch] = offset;
    s_prefs.putInt(("el" + String(ch)).c_str(), offset);
    return true;
}
