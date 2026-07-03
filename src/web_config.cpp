#include "web_config.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Web 服务器实例
static WebServer s_server(80);
static Preferences s_prefs;

// 传感器通道映射关系，存储的是全局 12 通道物理索引（0-11）
static int s_channel_map[4] = {0, 4, 8, 1};

// 缓存 12 个物理通道的实时状态
struct SensorDataCache {
    float raw_val;
    uint16_t filtered;
    uint16_t baseline;
    uint16_t threshold;
    bool detected;
};

static SensorDataCache s_sensor_cache[12] = {0};

// HTML 页面内容（支持现代暗色调、玻璃悬浮卡片式交互）
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>水传感器节点配置与监控</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --card-bg: rgba(30, 41, 59, 0.7);
            --border-color: rgba(255, 255, 255, 0.1);
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --accent-blue: #38bdf8;
            --accent-cyan: #06b6d4;
            --accent-green: #10b981;
            --accent-orange: #f97316;
            --water-alert: #22d3ee;
            --dry-normal: #64748b;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-main);
            min-height: 100vh;
            padding: 2rem 1rem;
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .container {
            width: 100%;
            max-width: 900px;
            display: flex;
            flex-direction: column;
            gap: 2rem;
        }

        header {
            text-align: center;
            margin-bottom: 1rem;
        }

        header h1 {
            font-size: 2rem;
            font-weight: 700;
            background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 0.5rem;
        }

        header p {
            color: var(--text-muted);
            font-size: 0.95rem;
        }

        .card {
            background: var(--card-bg);
            backdrop-filter: blur(12px);
            border: 1px solid var(--border-color);
            border-radius: 16px;
            padding: 1.5rem;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        }

        .card:hover {
            box-shadow: 0 12px 40px 0 rgba(56, 189, 248, 0.1);
        }

        .card-title {
            font-size: 1.25rem;
            font-weight: 600;
            margin-bottom: 1.25rem;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 0.5rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .grid-12 {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
            gap: 1rem;
        }

        .sensor-item {
            background: rgba(15, 23, 42, 0.6);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1rem;
            display: flex;
            flex-direction: column;
            gap: 0.4rem;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .sensor-item.water-detected {
            border-color: var(--accent-cyan);
            box-shadow: inset 0 0 12px rgba(6, 182, 212, 0.15);
            animation: pulse-border 2s infinite;
        }

        @keyframes pulse-border {
            0% { border-color: rgba(6, 182, 212, 0.6); }
            50% { border-color: rgba(6, 182, 212, 1); }
            100% { border-color: rgba(6, 182, 212, 0.6); }
        }

        .sensor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .sensor-id {
            font-weight: 600;
            color: var(--accent-blue);
        }

        .status-badge {
            font-size: 0.75rem;
            padding: 0.15rem 0.5rem;
            border-radius: 9999px;
            font-weight: 700;
            text-transform: uppercase;
        }

        .status-badge.dry {
            background: rgba(100, 116, 139, 0.2);
            color: var(--dry-normal);
        }

        .status-badge.water {
            background: rgba(6, 182, 212, 0.2);
            color: var(--water-alert);
        }

        .sensor-meta {
            font-size: 0.8rem;
            color: var(--text-muted);
            line-height: 1.4;
        }

        .sensor-meta span {
            color: var(--text-main);
        }

        .config-form {
            display: flex;
            flex-direction: column;
            gap: 1.25rem;
        }

        .form-row {
            display: flex;
            flex-wrap: wrap;
            gap: 1rem;
            justify-content: space-between;
        }

        .form-group {
            flex: 1 1 200px;
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
        }

        label {
            font-size: 0.9rem;
            font-weight: 500;
            color: var(--text-muted);
        }

        select {
            background: rgba(15, 23, 42, 0.8);
            border: 1px solid var(--border-color);
            color: var(--text-main);
            padding: 0.75rem;
            border-radius: 8px;
            font-size: 0.95rem;
            outline: none;
            cursor: pointer;
            transition: border-color 0.2s;
        }

        select:focus {
            border-color: var(--accent-blue);
        }

        .btn {
            background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
            color: #fff;
            border: none;
            padding: 0.85rem 1.75rem;
            border-radius: 8px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: opacity 0.2s, transform 0.1s;
            align-self: flex-end;
            margin-top: 1rem;
            box-shadow: 0 4px 14px rgba(56, 189, 248, 0.3);
        }

        .btn:hover {
            opacity: 0.9;
        }

        .btn:active {
            transform: scale(0.98);
        }

        #toast {
            position: fixed;
            bottom: 2rem;
            left: 50%;
            transform: translateX(-50%) translateY(100px);
            background: rgba(16, 185, 129, 0.9);
            color: white;
            padding: 0.85rem 2rem;
            border-radius: 50px;
            font-weight: 600;
            box-shadow: 0 8px 24px rgba(16, 185, 129, 0.3);
            transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
            pointer-events: none;
            z-index: 1000;
        }

        #toast.show {
            transform: translateX(-50%) translateY(0);
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>水传感器节点配置</h1>
            <p>通过热点实时监控 12 路物理通道电容并配置 4 路输出映射</p>
        </header>

        <!-- 监控卡片 -->
        <div class="card">
            <div class="card-title">
                <span>实时状态监控 (12 物理通道)</span>
                <span style="font-size: 0.8rem; font-weight: normal; color: var(--text-muted);">自动刷新 (1Hz)</span>
            </div>
            <div class="grid-12" id="sensor-grid">
                <!-- 12个通道的卡片将通过 JS 渲染 -->
            </div>
        </div>

        <!-- 配置卡片 -->
        <div class="card">
            <div class="card-title">映射配置输出通道</div>
            <form class="config-form" id="config-form" onsubmit="saveConfig(event)">
                <div class="form-row">
                    <div class="form-group">
                        <label for="sensor0">输出通道 1 (Sensor 1)</label>
                        <select id="sensor0" name="sensor0"></select>
                    </div>
                    <div class="form-group">
                        <label for="sensor1">输出通道 2 (Sensor 2)</label>
                        <select id="sensor1" name="sensor1"></select>
                    </div>
                    <div class="form-group">
                        <label for="sensor2">输出通道 3 (Sensor 3)</label>
                        <select id="sensor2" name="sensor2"></select>
                    </div>
                    <div class="form-group">
                        <label for="sensor3">输出通道 4 (Sensor 4)</label>
                        <select id="sensor3" name="sensor3"></select>
                    </div>
                </div>
                <button type="submit" class="btn">保存配置</button>
            </form>
        </div>
    </div>

    <div id="toast">保存成功，已即时生效！</div>

    <script>
        const CHANNELS = [
            "Chip 1 Chan 1 (通道 0)", "Chip 1 Chan 2 (通道 1)", "Chip 1 Chan 3 (通道 2)", "Chip 1 Chan 4 (通道 3)",
            "Chip 2 Chan 1 (通道 4)", "Chip 2 Chan 2 (通道 5)", "Chip 2 Chan 3 (通道 6)", "Chip 2 Chan 4 (通道 7)",
            "Chip 3 Chan 1 (通道 8)", "Chip 3 Chan 2 (通道 9)", "Chip 3 Chan 3 (通道 10)", "Chip 3 Chan 4 (通道 11)"
        ];

        // 渲染下拉框
        function renderOptions(mapping) {
            for (let i = 0; i < 4; i++) {
                const select = document.getElementById(`sensor${i}`);
                select.innerHTML = '';
                CHANNELS.forEach((name, idx) => {
                    const option = document.createElement('option');
                    option.value = idx;
                    option.textContent = name;
                    if (mapping && mapping[i] == idx) {
                        option.selected = true;
                    }
                    select.appendChild(option);
                });
            }
        }

        // 获取当前映射与数据
        async function fetchConfig() {
            try {
                const res = await fetch('/api/config');
                const data = await res.json();
                renderOptions(data.mapping);
            } catch (err) {
                console.error("Fetch config failed:", err);
            }
        }

        async function updateData() {
            try {
                const res = await fetch('/api/data');
                const data = await res.json();
                const grid = document.getElementById('sensor-grid');
                grid.innerHTML = '';

                data.sensors.forEach((s, idx) => {
                    const isWater = s.detected;
                    const item = document.createElement('div');
                    item.className = `sensor-item ${isWater ? 'water-detected' : ''}`;
                    item.innerHTML = `
                        <div class="sensor-header">
                            <span class="sensor-id">通道 ${idx}</span>
                            <span class="status-badge ${isWater ? 'water' : 'dry'}">${isWater ? '有水' : '干燥'}</span>
                        </div>
                        <div class="sensor-meta">
                            <div>原始值: <span>${(s.raw_val).toFixed(2)} pF</span></div>
                            <div>滤波值: <span>${s.filtered}</span></div>
                            <div>基准值: <span>${s.baseline}</span></div>
                            <div>阈值: <span>${s.threshold}</span></div>
                        </div>
                    `;
                    grid.appendChild(item);
                });
            } catch (err) {
                console.error("Fetch live data failed:", err);
            }
        }

        async function saveConfig(e) {
            e.preventDefault();
            const form = document.getElementById('config-form');
            const formData = new FormData(form);
            const params = new URLSearchParams();
            for (const pair of formData) {
                params.append(pair[0], pair[1]);
            }

            try {
                const res = await fetch('/api/config', {
                    method: 'POST',
                    body: params
                });
                if (res.ok) {
                    showToast();
                } else {
                    alert("保存失败");
                }
            } catch (err) {
                alert("请求出错: " + err);
            }
        }

        function showToast() {
            const toast = document.getElementById('toast');
            toast.className = 'show';
            setTimeout(() => {
                toast.className = '';
            }, 2500);
        }

        // 初始化
        fetchConfig();
        updateData();
        setInterval(updateData, 1000);
    </script>
</body>
</html>
)rawhtml";

// Web API - 实时 12 通道数据获取
static void handle_get_data() {
    String json = "{\"sensors\":[";
    for (int i = 0; i < 12; i++) {
        json += "{";
        json += "\"raw_val\":" + String(s_sensor_cache[i].raw_val) + ",";
        json += "\"filtered\":" + String(s_sensor_cache[i].filtered) + ",";
        json += "\"baseline\":" + String(s_sensor_cache[i].baseline) + ",";
        json += "\"threshold\":" + String(s_sensor_cache[i].threshold) + ",";
        json += "\"detected\":" + String(s_sensor_cache[i].detected ? "true" : "false");
        json += "}";
        if (i < 11) json += ",";
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// Web API - 配置映射关系获取
static void handle_get_config() {
    String json = "{\"mapping\":[";
    for (int i = 0; i < 4; i++) {
        json += String(s_channel_map[i]);
        if (i < 3) json += ",";
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// Web API - 保存配置映射关系
static void handle_post_config() {
    bool changed = false;
    for (int i = 0; i < 4; i++) {
        String arg_name = "sensor" + String(i);
        if (s_server.hasArg(arg_name)) {
            int new_val = s_server.arg(arg_name).toInt();
            if (new_val >= 0 && new_val < 12) {
                if (s_channel_map[i] != new_val) {
                    s_channel_map[i] = new_val;
                    changed = true;
                    String key = "map" + String(i);
                    s_prefs.putInt(key.c_str(), new_val);
                }
            }
        }
    }
    if (changed) {
        Serial.printf("[WebConfig] Mapping updated: %d, %d, %d, %d\n",
                      s_channel_map[0], s_channel_map[1], s_channel_map[2], s_channel_map[3]);
    }
    s_server.send(200, "text/plain", "OK");
}

void web_config_init() {
    // 1. 初始化 Preferences 非易失性存储
    s_prefs.begin("sensor_map", false);

    // 如果还没有保存的值，使用原 config.h 定义的默认偏移映射做缺省
    s_channel_map[0] = s_prefs.getInt("map0", SENSOR1_CHIP * 4 + SENSOR1_CHAN);
    s_channel_map[1] = s_prefs.getInt("map1", SENSOR2_CHIP * 4 + SENSOR2_CHAN);
    s_channel_map[2] = s_prefs.getInt("map2", SENSOR3_CHIP * 4 + SENSOR3_CHAN);
    s_channel_map[3] = s_prefs.getInt("map3", SENSOR4_CHIP * 4 + SENSOR4_CHAN);

    Serial.printf("[WebConfig] Loaded Mapping: %d, %d, %d, %d\n",
                  s_channel_map[0], s_channel_map[1], s_channel_map[2], s_channel_map[3]);

    // 2. 启动 WiFi AP 模式
    // 首先设置模式为 AP+STA 共存模式，这样既能连外部局域网和MQTT，也能开启配置热点
    print_wifi_status("WebConfig BEFORE softAP");
    WiFi.mode(WIFI_AP_STA);
    
    // 启动软 AP 热点，密码使用 "12344321" (和局域网一致)
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    print_wifi_status("WebConfig AFTER softAP");

    // 3. 挂载 Web 路由
    s_server.on("/", HTTP_GET, []() {
        s_server.send_P(200, "text/html", INDEX_HTML);
    });

    s_server.on("/api/data", HTTP_GET, handle_get_data);
    s_server.on("/api/config", HTTP_GET, handle_get_config);
    s_server.on("/api/config", HTTP_POST, handle_post_config);

    s_server.begin();
    Serial.println("[WebConfig] Built-in Web Server started on port 80");
}

void print_wifi_status(const char* label) {
    wifi_mode_t mode = WiFi.getMode();
    const char* mode_str = "UNKNOWN";
    if (mode == WIFI_OFF) mode_str = "OFF";
    else if (mode == WIFI_STA) mode_str = "STA";
    else if (mode == WIFI_AP) mode_str = "AP";
    else if (mode == WIFI_AP_STA) mode_str = "AP_STA";
    
    Serial.printf("[%s] Current WiFi Mode: %s, AP IP: %s, Station IP: %s, Station Status: %d\n",
                  label, mode_str, 
                  WiFi.softAPIP().toString().c_str(), 
                  WiFi.localIP().toString().c_str(),
                  WiFi.status());
}

void web_config_loop() {
    // 强制检测防回退：如果模式被外部库强制修改回了 STA，在此迅速恢复为 AP_STA 并重新开启 softAP
    if (WiFi.getMode() == WIFI_STA) {
        Serial.println("[WebConfig] WiFi mode was reverted to STA. Restoring AP_STA and restarting softAP...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        print_wifi_status("WebConfig RESTORED AP_STA");
    }
    s_server.handleClient();
}

int get_mapped_channel(int output_idx) {
    if (output_idx < 0 || output_idx >= 4) return 0;
    return s_channel_map[output_idx];
}

void web_config_update_sensor(int idx, float raw_val, uint16_t filtered, uint16_t baseline, uint16_t threshold, bool detected) {
    if (idx < 0 || idx >= 12) return;
    s_sensor_cache[idx].raw_val = raw_val;
    s_sensor_cache[idx].filtered = filtered;
    s_sensor_cache[idx].baseline = baseline;
    s_sensor_cache[idx].threshold = threshold;
    s_sensor_cache[idx].detected = detected;
}
