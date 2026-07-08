#include "web_config.h"
#include "nvs_config.h"
#include "data_cache.h"
#include "index_html.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
//  Web 服务器实例（服务层私有）
// ============================================================
static WebServer s_server(80);

// ============================================================
//  辅助函数：打印当前 WiFi 模式状态
// ============================================================
static void print_wifi_status(const char* label) {
    wifi_mode_t mode = WiFi.getMode();
    const char* mode_str = "UNKNOWN";
    if (mode == WIFI_OFF)    mode_str = "OFF";
    else if (mode == WIFI_STA)    mode_str = "STA";
    else if (mode == WIFI_AP)     mode_str = "AP";
    else if (mode == WIFI_AP_STA) mode_str = "AP_STA";

    Serial.printf("[%s] Mode: %s, AP IP: %s, STA IP: %s, Status: %d\n",
                  label, mode_str,
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.status());
}

// ============================================================
//  REST API 处理函数
// ============================================================

// GET /api/data — 返回 12 路实时传感数据
static void handle_get_data() {
    String json = "{\"sensors\":[";
    for (int i = 0; i < 12; i++) {
        const SensorDataCache& s = data_cache_get_sensor(i);
        json += "{";
        json += "\"raw_val\":"  + String(s.raw_val) + ",";
        json += "\"filtered\":" + String(s.filtered) + ",";
        json += "\"baseline\":" + String(s.baseline) + ",";
        json += "\"threshold\":" + String(s.threshold) + ",";
        json += "\"detected\":" + String(s.detected ? "true" : "false") + ",";
        json += "\"offset\":"  + String(get_channel_threshold(i));
        json += "}";
        if (i < 11) json += ",";
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// GET /api/config — 返回通道映射配置
static void handle_get_config() {
    String json = "{\"mapping\":[";
    for (int i = 0; i < 4; i++) {
        json += String(get_mapped_channel(i));
        if (i < 3) json += ",";
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// POST /api/config — 保存通道映射到 NVS
static void handle_post_config() {
    bool changed = false;
    for (int i = 0; i < 4; i++) {
        String arg_name = "sensor" + String(i);
        if (s_server.hasArg(arg_name)) {
            int new_val = s_server.arg(arg_name).toInt();
            if (nvs_set_channel_map(i, new_val)) {
                changed = true;
            }
        }
    }
    if (changed) {
        Serial.printf("[WebConfig] Mapping updated: %d, %d, %d, %d\n",
                      get_mapped_channel(0), get_mapped_channel(1),
                      get_mapped_channel(2), get_mapped_channel(3));
    }
    s_server.send(200, "text/plain", "OK");
}

// GET /api/sysconfig — 返回网络与系统配置
static void handle_get_sysconfig() {
    String json = "{";
    json += "\"ssid\":\""   + get_sta_ssid() + "\",";
    json += "\"pass\":\""   + get_sta_password() + "\",";
    json += "\"name\":\""   + get_device_name() + "\",";
    json += "\"broker\":\"" + get_mqtt_broker() + "\",";
    json += "\"port\":"     + String(get_mqtt_port());
    json += "}";
    s_server.send(200, "application/json", json);
}

// POST /api/sysconfig — 保存网络配置到 NVS
static void handle_post_sysconfig() {
    bool changed = false;
    if (s_server.hasArg("ssid"))     changed |= nvs_set_sta_ssid(s_server.arg("ssid"));
    if (s_server.hasArg("password")) changed |= nvs_set_sta_password(s_server.arg("password"));
    if (s_server.hasArg("name"))     changed |= nvs_set_device_name(s_server.arg("name"));
    if (s_server.hasArg("broker"))   changed |= nvs_set_mqtt_broker(s_server.arg("broker"));
    if (s_server.hasArg("port"))     changed |= nvs_set_mqtt_port(s_server.arg("port").toInt());

    if (changed) {
        Serial.println("[WebConfig] System configurations updated in NVS.");
    }
    s_server.send(200, "text/plain", "OK");
}

// GET /api/scan — 扫描附近 WiFi
static void handle_wifi_scan() {
    int n = WiFi.scanNetworks(false, false);
    String json = "{\"networks\":[";
    if (n > 0) {
        // 按 RSSI 降序排列
        int indices[n];
        for (int i = 0; i < n; i++) indices[i] = i;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                    int temp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = temp;
                }
            }
        }
        for (int i = 0; i < n; i++) {
            int idx = indices[i];
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(idx) + "\",";
            json += "\"rssi\":"   + String(WiFi.RSSI(idx));
            json += "}";
            if (i < n - 1) json += ",";
        }
    }
    json += "]}";
    WiFi.scanDelete();
    s_server.send(200, "application/json", json);
}

// POST /api/threshold — 配置单通道阈值偏移量
static void handle_post_threshold() {
    if (s_server.hasArg("ch") && s_server.hasArg("offset")) {
        int ch     = s_server.arg("ch").toInt();
        int offset = s_server.arg("offset").toInt();
        if (nvs_set_threshold_offset(ch, offset)) {
            Serial.printf("[WebConfig] Channel %d threshold offset set to %d\n", ch, offset);
            s_server.send(200, "text/plain", "OK");
            return;
        }
    }
    s_server.send(400, "text/plain", "Bad Request");
}

// GET /api/polldelay — 返回轮询间隔
static void handle_get_polldelay() {
    String json = "{\"delay\":" + String(get_poll_delay()) + "}";
    s_server.send(200, "application/json", json);
}

// POST /api/polldelay — 保存轮询间隔到 NVS
static void handle_post_polldelay() {
    if (s_server.hasArg("delay")) {
        int delay_val = s_server.arg("delay").toInt();
        if (nvs_set_poll_delay(delay_val)) {
            Serial.printf("[WebConfig] Poll delay updated to %d ms\n", delay_val);
            s_server.send(200, "text/plain", "OK");
            return;
        }
    }
    s_server.send(400, "text/plain", "Bad Request");
}

// ============================================================
//  公共接口实现
// ============================================================

void web_config_init() {
    // 1. 初始化 NVS 配置层
    nvs_config_init();

    // 2. 启动 WiFi AP 模式
    print_wifi_status("WebConfig BEFORE softAP");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
    print_wifi_status("WebConfig AFTER softAP");

    // 3. 挂载 Web 路由
    s_server.on("/", HTTP_GET, []() {
        s_server.send_P(200, "text/html", INDEX_HTML);
    });
    s_server.on("/api/data",       HTTP_GET,  handle_get_data);
    s_server.on("/api/config",     HTTP_GET,  handle_get_config);
    s_server.on("/api/config",     HTTP_POST, handle_post_config);
    s_server.on("/api/sysconfig",  HTTP_GET,  handle_get_sysconfig);
    s_server.on("/api/sysconfig",  HTTP_POST, handle_post_sysconfig);
    s_server.on("/api/wifi",       HTTP_GET,  handle_get_sysconfig);   // 兼容旧接口
    s_server.on("/api/wifi",       HTTP_POST, handle_post_sysconfig);  // 兼容旧接口
    s_server.on("/api/scan",       HTTP_GET,  handle_wifi_scan);
    s_server.on("/api/threshold",  HTTP_POST, handle_post_threshold);
    s_server.on("/api/polldelay",  HTTP_GET,  handle_get_polldelay);
    s_server.on("/api/polldelay",  HTTP_POST, handle_post_polldelay);

    s_server.begin();
    Serial.println("[WebConfig] Embedded Web Server started on port 80");
}

void web_config_loop() {
    // 防回退机制：检测 WiFi 模式是否被外部库强制切回 STA
    if (WiFi.getMode() == WIFI_STA) {
        Serial.println("[WebConfig] WiFi mode was reverted to STA. Restoring AP_STA and restarting softAP...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
        print_wifi_status("WebConfig RESTORED AP_STA");
    }
    s_server.handleClient();
}

// ============================================================
//  web_config.h 中声明的缓存更新接口（转发至 data_cache）
// ============================================================
void web_config_update_sensor(int idx, float raw_val, uint16_t filtered,
                               uint16_t baseline, uint16_t threshold, bool detected) {
    data_cache_update_sensor(idx, raw_val, filtered, baseline, threshold, detected);
}
