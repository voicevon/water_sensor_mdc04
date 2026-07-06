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

// 12 个物理通道的阈值偏移量
static int s_threshold_offset[12] = {50};

// 轮询各个通道测量之间的软件延时间隔 (ms)
static int s_poll_delay = 50;

// STA Wi-Fi 及系统网络缓存
static String s_sta_ssid = "";
static String s_sta_password = "";
static String s_device_name = "";
static String s_mqtt_broker = "";
static int s_mqtt_port = 1883;

// 缓存 12 个物理通道的实时状态
struct SensorDataCache {
    float raw_val;
    uint16_t filtered;
    uint16_t baseline;
    uint16_t threshold;
    bool detected;
};

static SensorDataCache s_sensor_cache[12] = {0};

// ---------------- HTML 页面内容 ----------------
// 全新设计的 SPA 暗色玻璃拟态多 Tab 页面，整合右上角菜单与阈值微调 Modal
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>传感器监控与配置中心</title>
    <style>
        :root {
            --bg-color: #0b0f19;
            --card-bg: rgba(22, 30, 49, 0.7);
            --border-color: rgba(255, 255, 255, 0.08);
            --text-main: #f1f5f9;
            --text-muted: #64748b;
            --accent-blue: #38bdf8;
            --accent-cyan: #06b6d4;
            --accent-green: #10b981;
            --water-alert: #22d3ee;
            --dry-normal: #64748b;
            --nav-bg: rgba(17, 24, 39, 0.95);
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-main);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        /* 顶部导航栏，右上角放置菜单按钮 */
        header {
            width: 100%;
            background: var(--nav-bg);
            border-bottom: 1px solid var(--border-color);
            padding: 1rem 1.5rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            position: sticky;
            top: 0;
            z-index: 100;
            backdrop-filter: blur(8px);
        }

        .logo {
            font-size: 1.15rem;
            font-weight: 700;
            letter-spacing: 0.5px;
            background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .menu-btn {
            background: transparent;
            border: none;
            color: var(--text-main);
            font-size: 1.5rem;
            cursor: pointer;
            outline: none;
            width: 40px;
            height: 40px;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 8px;
            transition: background 0.2s;
        }

        .menu-btn:hover {
            background: rgba(255, 255, 255, 0.05);
        }

        /* 抽屉导航链接 */
        .drawer {
            position: fixed;
            top: 0;
            right: 0;
            width: 250px;
            height: 100%;
            background: #111827;
            border-left: 1px solid var(--border-color);
            box-shadow: -10px 0 30px rgba(0,0,0,0.5);
            transform: translateX(100%);
            transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            z-index: 200;
            padding: 2rem 1.5rem;
            display: flex;
            flex-direction: column;
            gap: 1.5rem;
        }

        .drawer.open {
            transform: translateX(0);
        }

        .drawer-close {
            align-self: flex-end;
            background: transparent;
            border: none;
            color: var(--text-muted);
            font-size: 1.5rem;
            cursor: pointer;
        }

        .drawer-close:hover {
            color: var(--text-main);
        }

        .nav-link {
            color: var(--text-muted);
            text-decoration: none;
            font-size: 1.1rem;
            font-weight: 500;
            transition: color 0.2s;
            cursor: pointer;
            padding: 0.5rem 0;
            border-bottom: 1px solid rgba(255,255,255,0.02);
        }

        .nav-link:hover, .nav-link.active {
            color: var(--accent-blue);
        }

        .overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.5);
            z-index: 150;
            display: none;
        }

        .overlay.show {
            display: block;
        }

        /* 主容器 */
        .container {
            width: 100%;
            max-width: 900px;
            padding: 1.5rem 1rem;
            display: flex;
            flex-direction: column;
            gap: 1.5rem;
        }

        /* TAB 页面控制 */
        .tab-content {
            display: none;
        }

        .tab-content.active {
            display: block;
        }

        .card {
            background: var(--card-bg);
            backdrop-filter: blur(16px);
            border: 1px solid var(--border-color);
            border-radius: 16px;
            padding: 1.5rem;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
            margin-bottom: 1.5rem;
        }

        .card-title {
            font-size: 1.15rem;
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
            background: rgba(10, 15, 28, 0.6);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1rem;
            display: flex;
            flex-direction: column;
            gap: 0.4rem;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            position: relative;
        }

        .sensor-item.water-detected {
            border-color: var(--accent-cyan);
            box-shadow: inset 0 0 12px rgba(6, 182, 212, 0.15);
            animation: pulse-border 2s infinite;
        }

        @keyframes pulse-border {
            0% { border-color: rgba(6, 182, 212, 0.5); }
            50% { border-color: rgba(6, 182, 212, 1); }
            100% { border-color: rgba(6, 182, 212, 0.5); }
        }

        .sensor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 0.25rem;
        }

        .sensor-id {
            font-weight: 600;
            color: var(--accent-blue);
        }

        .status-badge {
            font-size: 0.72rem;
            padding: 0.15rem 0.45rem;
            border-radius: 9999px;
            font-weight: 700;
        }

        .status-badge.dry {
            background: rgba(100, 116, 139, 0.15);
            color: var(--dry-normal);
        }

        .status-badge.water {
            background: rgba(6, 182, 212, 0.15);
            color: var(--water-alert);
        }

        .sensor-meta {
            font-size: 0.78rem;
            color: var(--text-muted);
            line-height: 1.4;
        }

        .sensor-meta span {
            color: var(--text-main);
        }

        /* 齿轮配置按钮 */
        .btn-config {
            position: absolute;
            bottom: 0.75rem;
            right: 0.75rem;
            background: rgba(255,255,255,0.03);
            border: 1px solid var(--border-color);
            color: var(--text-muted);
            width: 28px;
            height: 28px;
            border-radius: 6px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.2s;
        }

        .btn-config:hover {
            color: var(--accent-blue);
            background: rgba(56, 189, 248, 0.1);
            border-color: var(--accent-blue);
        }

        /* 表单及按钮元素 */
        .form-group {
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
            margin-bottom: 1.25rem;
        }

        label {
            font-size: 0.88rem;
            font-weight: 500;
            color: var(--text-muted);
        }

        select, input {
            background: rgba(10, 15, 28, 0.8);
            border: 1px solid var(--border-color);
            color: var(--text-main);
            padding: 0.75rem;
            border-radius: 8px;
            font-size: 0.95rem;
            outline: none;
            width: 100%;
        }

        select:focus, input:focus {
            border-color: var(--accent-blue);
        }

        .btn {
            background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
            color: #fff;
            border: none;
            padding: 0.8rem 1.5rem;
            border-radius: 8px;
            font-size: 0.95rem;
            font-weight: 600;
            cursor: pointer;
            transition: opacity 0.2s;
            box-shadow: 0 4px 12px rgba(56, 189, 248, 0.25);
            display: inline-flex;
            justify-content: center;
            align-items: center;
        }

        .btn:hover {
            opacity: 0.9;
        }

        /* MODAL 阈值微调弹窗 */
        .modal {
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%) scale(0.9);
            width: 90%;
            max-width: 400px;
            background: #111827;
            border: 1px solid var(--border-color);
            border-radius: 16px;
            padding: 1.5rem;
            box-shadow: 0 20px 50px rgba(0,0,0,0.6);
            z-index: 300;
            opacity: 0;
            pointer-events: none;
            transition: all 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
        }

        .modal.show {
            transform: translate(-50%, -50%) scale(1);
            opacity: 1;
            pointer-events: auto;
        }

        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1rem;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 0.5rem;
        }

        .modal-title {
            font-size: 1.1rem;
            font-weight: 600;
            color: var(--accent-blue);
        }

        .modal-footer {
            display: flex;
            justify-content: flex-end;
            gap: 0.75rem;
            margin-top: 1.25rem;
        }

        .btn-secondary {
            background: rgba(255,255,255,0.05);
            border: 1px solid var(--border-color);
            color: var(--text-main);
        }

        .btn-secondary:hover {
            background: rgba(255,255,255,0.1);
        }

        #toast {
            position: fixed;
            bottom: 2rem;
            left: 50%;
            transform: translateX(-50%) translateY(200px);
            opacity: 0;
            visibility: hidden;
            background: rgba(16, 185, 129, 0.95);
            color: white;
            padding: 0.75rem 1.75rem;
            border-radius: 50px;
            font-weight: 600;
            box-shadow: 0 8px 24px rgba(16, 185, 129, 0.3);
            transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275), opacity 0.2s, visibility 0.2s;
            pointer-events: none;
            z-index: 1000;
            font-size: 0.9rem;
        }

        #toast.show {
            transform: translateX(-50%) translateY(0);
            opacity: 1;
            visibility: visible;
        }

        .wifi-list {
            margin-top: 0.5rem;
            max-height: 150px;
            overflow-y: auto;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            background: rgba(0, 0, 0, 0.2);
            display: none;
        }
        .wifi-list.show {
            display: block;
        }
        .wifi-item {
            padding: 0.6rem 1rem;
            border-bottom: 1px solid var(--border-color);
            cursor: pointer;
            display: flex;
            justify-content: space-between;
            font-size: 0.88rem;
            transition: background 0.2s;
        }
        .wifi-item:last-child {
            border-bottom: none;
        }
        .wifi-item:hover {
            background: rgba(255, 255, 255, 0.05);
        }
        .wifi-signal {
            color: var(--accent-blue);
        }
    </style>
</head>
<body>

    <header>
        <div class="logo">水传感器配置节点</div>
        <button class="menu-btn" onclick="toggleDrawer(true)">☰</button>
    </header>

    <!-- 侧边抽屉菜单 -->
    <div class="overlay" id="overlay" onclick="toggleDrawer(false)"></div>
    <div class="drawer" id="drawer">
        <button class="drawer-close" onclick="toggleDrawer(false)">✕</button>
        <div style="height: 1rem;"></div>
        <div class="nav-link active" data-tab="tab-monitor" onclick="switchTab(this)">实时监控 (Monitor)</div>
        <div class="nav-link" data-tab="tab-mapping" onclick="switchTab(this)">输出映射 (Mapping)</div>
        <div class="nav-link" data-tab="tab-wifi" onclick="switchTab(this)">外部 Wi-Fi 配置 (STA)</div>
        <div class="nav-link" data-tab="tab-about" onclick="switchTab(this)">关于 (About)</div>
    </div>

    <div class="container">
        <!-- TAB 1: 实时监控 -->
        <div id="tab-monitor" class="tab-content active">
            <div class="card" style="padding: 1rem 1.5rem; display: flex; flex-wrap: wrap; justify-content: space-between; align-items: center; gap: 1rem; margin-bottom: 1.25rem;">
                <div style="font-weight: 500; font-size: 0.95rem; color: var(--text-main);">通道轮询防串扰时间间隔 (Anti-Crosstalk):</div>
                <form id="delay-form" onsubmit="saveDelay(event)" style="display: flex; gap: 0.5rem; align-items: center; flex: 1 1 auto; max-width: 300px; justify-content: flex-end;">
                    <input type="number" id="poll-delay" name="delay" min="0" max="1000" style="padding: 0.45rem 0.75rem; font-size: 0.9rem; background: rgba(255,255,255,0.05); border: 1px solid var(--border-color); border-radius: 8px; color: var(--text-main); width: 80px; text-align: center;" required>
                    <span style="font-size: 0.9rem; color: var(--text-muted);">ms</span>
                    <button type="submit" class="btn" style="padding: 0.45rem 1rem; font-size: 0.90rem; border-radius: 8px;">保存</button>
                </form>
            </div>
            <div class="card">
                <div class="card-title">
                    <span>12 物理通道数据列表</span>
                    <span style="font-size: 0.8rem; font-weight: normal; color: var(--text-muted);">自动刷新(1Hz)</span>
                </div>
                <div class="grid-12" id="sensor-grid">
                    <!-- JS 渲染 -->
                </div>
            </div>
        </div>

        <!-- TAB 2: 输出映射 -->
        <div id="tab-mapping" class="tab-content">
            <div class="card">
                <div class="card-title">4 路输出映射配置</div>
                <form id="mapping-form" onsubmit="saveMapping(event)">
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
                    <button type="submit" class="btn" style="width: 100%;">保存映射</button>
                </form>
            </div>
        </div>

        <!-- TAB 3: 系统与网络配置 -->
        <div id="tab-wifi" class="tab-content">
            <div class="card">
                <div class="card-title">无线局域网 (Wi-Fi STA)</div>
                <form id="wifi-form" onsubmit="saveWifi(event)">
                    <div class="form-group">
                        <label for="ssid">Wi-Fi 网络名称 (SSID)</label>
                        <div style="display: flex; gap: 0.5rem;">
                            <input type="text" id="ssid" name="ssid" placeholder="输入外部路由器名称" style="flex: 1;" required>
                            <button type="button" class="btn" style="width: auto; padding: 0.5rem 1rem; font-size: 0.85rem;" onclick="scanWifi(this)">扫描</button>
                        </div>
                        <div id="wifi-list" class="wifi-list"></div>
                    </div>
                    <div class="form-group">
                        <label for="password">Wi-Fi 网络密码 (Password)</label>
                        <input type="password" id="password" name="password" placeholder="输入外部 Wi-Fi 密码">
                    </div>

                    <div style="margin: 1.5rem 0 1rem 0; border-top: 1px solid var(--border-color); padding-top: 1rem;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: var(--accent-blue); margin-bottom: 0.75rem;">设备与 MQTT 配置</h4>
                    </div>

                    <div class="form-group">
                        <label for="name">设备名称 (DEVICE_NAME)</label>
                        <input type="text" id="name" name="name" placeholder="例如: home" required>
                    </div>
                    <div class="form-group">
                        <label for="broker">MQTT Broker 地址 (域名或 IP)</label>
                        <input type="text" id="broker" name="broker" placeholder="例如: voicevon.vicp.io" required>
                    </div>
                    <div class="form-group">
                        <label for="port">MQTT 端口 (Port)</label>
                        <input type="number" id="port" name="port" min="1" max="65535" placeholder="默认: 1883" required>
                    </div>

                    <button type="submit" class="btn" style="width: 100%;">保存并应用配置</button>
                </form>
            </div>
        </div>

        <!-- TAB 4: 关于 (About) -->
        <div id="tab-about" class="tab-content">
            <div class="card">
                <div class="card-title">关于系统</div>
                <div style="line-height: 2; color: var(--text-main); font-size: 0.95rem;">
                    <p>设备名称：liquid sensor</p>
                    <p>版本信息：Version 1.0</p>
                    <p>发布时间：2026年7月</p>
                    <p></p>
                    <p style="margin-top: 1.5rem; color: var(--text-muted); font-size: 0.85rem; border-top: 1px solid var(--border-color); padding-top: 0.75rem; text-align: center;">
                        版权所有 © 山东卷积分公司
                    </p>
                </div>
            </div>
        </div>
    </div>

    <!-- 阈值微调 Modal 弹窗 -->
    <div class="modal" id="threshold-modal">
        <div class="modal-header">
            <div class="modal-title" id="modal-ch-title">配置通道 0</div>
            <button class="drawer-close" style="font-size: 1.15rem;" onclick="closeModal()">✕</button>
        </div>
        <form id="threshold-form" onsubmit="saveThreshold(event)">
            <input type="hidden" id="modal-ch-id" name="ch">
            <div class="form-group">
                <label for="modal-offset">阈值偏移量 (Threshold Offset)</label>
                <input type="number" id="modal-offset" name="offset" min="1" max="500" required>
                <span style="font-size: 0.72rem; color: var(--text-muted); margin-top: 0.25rem;">此参数为滤波线高于慢速基准线以触发水警报的电容增量</span>
            </div>
            <div class="modal-footer">
                <button type="button" class="btn btn-secondary" onclick="closeModal()">取消</button>
                <button type="submit" class="btn">确定</button>
            </div>
        </form>
    </div>

    <div id="toast">保存成功，已即时生效！</div>

    <script>
        const CHANNELS = [
            "Chip 1 Chan 1 (通道 0)", "Chip 1 Chan 2 (通道 1)", "Chip 1 Chan 3 (通道 2)", "Chip 1 Chan 4 (通道 3)",
            "Chip 2 Chan 1 (通道 4)", "Chip 2 Chan 2 (通道 5)", "Chip 2 Chan 3 (通道 6)", "Chip 2 Chan 4 (通道 7)",
            "Chip 3 Chan 1 (通道 8)", "Chip 3 Chan 2 (通道 9)", "Chip 3 Chan 3 (通道 10)", "Chip 3 Chan 4 (通道 11)"
        ];

        // 切换抽屉菜单
        function toggleDrawer(open) {
            document.getElementById('drawer').classList.toggle('open', open);
            document.getElementById('overlay').classList.toggle('show', open);
        }

        // SPA 页面切卡
        function switchTab(el) {
            document.querySelectorAll('.nav-link').forEach(link => link.classList.remove('active'));
            el.classList.add('active');
            
            const targetId = el.getAttribute('data-tab');
            document.querySelectorAll('.tab-content').forEach(tab => tab.classList.remove('active'));
            document.getElementById(targetId).classList.add('active');
            
            toggleDrawer(false);

            if (targetId === 'tab-mapping') {
                fetchMapping();
            } else if (targetId === 'tab-wifi') {
                fetchWifi();
            }
        }

        // 打开阈值配置Modal
        function openModal(chIdx, currentOffset) {
            document.getElementById('modal-ch-id').value = chIdx;
            document.getElementById('modal-ch-title').textContent = `配置物理通道 ${chIdx} (${CHANNELS[chIdx].split(' ')[0]} ${CHANNELS[chIdx].split(' ')[1]})`;
            document.getElementById('modal-offset').value = currentOffset;
            document.getElementById('threshold-modal').classList.add('show');
            document.getElementById('overlay').classList.add('show');
        }

        function closeModal() {
            document.getElementById('threshold-modal').classList.remove('show');
            if (!document.getElementById('drawer').classList.contains('open')) {
                document.getElementById('overlay').classList.remove('show');
            }
        }

        // 渲染下拉框
        function renderMappingOptions(mapping) {
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

        // APIs 请求
        async function fetchMapping() {
            try {
                const res = await fetch('/api/config');
                const data = await res.json();
                renderMappingOptions(data.mapping);
            } catch (err) {
                console.error("Fetch mapping failed:", err);
            }
        }

        async function fetchWifi() {
            try {
                const res = await fetch('/api/sysconfig');
                const data = await res.json();
                document.getElementById('ssid').value = data.ssid || '';
                document.getElementById('password').value = data.pass || '';
                document.getElementById('name').value = data.name || '';
                document.getElementById('broker').value = data.broker || '';
                document.getElementById('port').value = data.port || 1883;
            } catch (err) {
                console.error("Fetch WiFi failed:", err);
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
                        <button class="btn-config" onclick="openModal(${idx}, ${s.offset})" title="配置阈值">⚙</button>
                    `;
                    grid.appendChild(item);
                });
            } catch (err) {
                console.error("Fetch data failed:", err);
            }
        }

        async function saveMapping(e) {
            e.preventDefault();
            const form = document.getElementById('mapping-form');
            const params = new URLSearchParams(new FormData(form));
            try {
                const res = await fetch('/api/config', { method: 'POST', body: params });
                if (res.ok) showToast("映射配置保存成功！");
            } catch (err) {
                alert("出错: " + err);
            }
        }

        async function saveWifi(e) {
            e.preventDefault();
            const form = document.getElementById('wifi-form');
            const params = new URLSearchParams(new FormData(form));
            try {
                const res = await fetch('/api/sysconfig', { method: 'POST', body: params });
                if (res.ok) showToast("系统与网络配置已保存，设备将在后台重试连接！");
            } catch (err) {
                alert("出错: " + err);
            }
        }

        async function saveThreshold(e) {
            e.preventDefault();
            const form = document.getElementById('threshold-form');
            const params = new URLSearchParams(new FormData(form));
            try {
                const res = await fetch('/api/threshold', { method: 'POST', body: params });
                if (res.ok) {
                    closeModal();
                    showToast("阈值偏移已更新，立即生效！");
                    updateData();
                }
            } catch (err) {
                alert("出错: " + err);
            }
        }

        async function saveDelay(e) {
            e.preventDefault();
            const form = document.getElementById('delay-form');
            const params = new URLSearchParams(new FormData(form));
            try {
                const res = await fetch('/api/polldelay', { method: 'POST', body: params });
                if (res.ok) showToast("通道轮询延时已更新，立即生效！");
            } catch (err) {
                alert("出错: " + err);
            }
        }

        async function fetchDelay() {
            try {
                const res = await fetch('/api/polldelay');
                const data = await res.json();
                document.getElementById('poll-delay').value = data.delay !== undefined ? data.delay : 50;
            } catch (err) {
                console.error("Fetch delay failed:", err);
            }
        }

        async function scanWifi(btn) {
            const origText = btn.textContent;
            btn.textContent = "扫描中...";
            btn.disabled = true;
            const list = document.getElementById('wifi-list');
            list.innerHTML = '<div style="padding: 0.75rem 1rem; font-size: 0.85rem; color: var(--text-muted); text-align: center;">正在搜索附近 WiFi 热点...</div>';
            list.classList.add('show');
            
            try {
                const res = await fetch('/api/scan');
                const data = await res.json();
                list.innerHTML = '';
                if (data.networks && data.networks.length > 0) {
                    data.networks.forEach(net => {
                        if (!net.ssid) return;
                        const item = document.createElement('div');
                        item.className = 'wifi-item';
                        item.innerHTML = `<span>${net.ssid}</span><span class="wifi-signal">${net.rssi} dBm</span>`;
                        item.onclick = () => {
                            document.getElementById('ssid').value = net.ssid;
                            list.classList.remove('show');
                        };
                        list.appendChild(item);
                    });
                } else {
                    list.innerHTML = '<div style="padding: 0.75rem 1rem; font-size: 0.85rem; color: var(--text-muted); text-align: center;">未找到可用的 WiFi 网络</div>';
                }
            } catch (err) {
                list.innerHTML = '<div style="padding: 0.75rem 1rem; font-size: 0.85rem; color: #EF4444; text-align: center;">扫描失败</div>';
            } finally {
                btn.textContent = origText;
                btn.disabled = false;
            }
        }

        function showToast(msg) {
            const toast = document.getElementById('toast');
            toast.textContent = msg;
            toast.className = 'show';
            setTimeout(() => { toast.className = ''; }, 2500);
        }

        // 初始化定时任务
        updateData();
        fetchDelay();
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
        json += "\"detected\":" + String(s_sensor_cache[i].detected ? "true" : "false") + ",";
        json += "\"offset\":" + String(s_threshold_offset[i]);
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

// Web API - 获取 Wi-Fi 配置
static void handle_get_wifi() {
    String json = "{";
    json += "\"ssid\":\"" + s_sta_ssid + "\",";
    json += "\"pass\":\"" + s_sta_password + "\"";
    json += "}";
    s_server.send(200, "application/json", json);
}

// Web API - 修改 Wi-Fi 配置
static void handle_post_wifi() {
    bool changed = false;
    if (s_server.hasArg("ssid")) {
        String new_ssid = s_server.arg("ssid");
        if (new_ssid.length() > 0 && new_ssid != s_sta_ssid) {
            s_sta_ssid = new_ssid;
            s_prefs.putString("sta_ssid", new_ssid);
            changed = true;
        }
    }
    if (s_server.hasArg("password")) {
        String new_pass = s_server.arg("password");
        if (new_pass != s_sta_password) {
            s_sta_password = new_pass;
            s_prefs.putString("sta_pass", new_pass);
            changed = true;
        }
    }
    if (changed) {
        Serial.printf("[WebConfig] WiFi STA credentials updated. SSID: %s\n", s_sta_ssid.c_str());
    }
    s_server.send(200, "text/plain", "OK");
}

// Web API - 单个通道的阈值偏移量配置
static void handle_post_threshold() {
    if (s_server.hasArg("ch") && s_server.hasArg("offset")) {
        int ch = s_server.arg("ch").toInt();
        int offset = s_server.arg("offset").toInt();
        if (ch >= 0 && ch < 12 && offset >= 1 && offset <= 500) {
            s_threshold_offset[ch] = offset;
            String key = "thr" + String(ch);
            s_prefs.putInt(key.c_str(), offset);
            Serial.printf("[WebConfig] Channel %d threshold offset set to %d\n", ch, offset);
            s_server.send(200, "text/plain", "OK");
            return;
        }
    }
    s_server.send(400, "text/plain", "Bad Request");
}

// Web API - 获取轮询间隔延时
static void handle_get_polldelay() {
    String json = "{\"delay\":" + String(s_poll_delay) + "}";
    s_server.send(200, "application/json", json);
}

// Web API - 更新轮询间隔延时
static void handle_post_polldelay() {
    if (s_server.hasArg("delay")) {
        int delay_val = s_server.arg("delay").toInt();
        if (delay_val >= 0 && delay_val <= 1000) {
            s_poll_delay = delay_val;
            s_prefs.putInt("poll_delay", delay_val);
            Serial.printf("[WebConfig] Poll delay updated to %d ms\n", delay_val);
            s_server.send(200, "text/plain", "OK");
            return;
        }
    }
    s_server.send(400, "text/plain", "Bad Request");
}

// Web API - 扫描附近可用 Wi-Fi 网络
static void handle_wifi_scan() {
    int n = WiFi.scanNetworks(false, false);
    String json = "{\"networks\":[";
    if (n > 0) {
        // 对信号强度（RSSI）进行排序（降序）
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
            json += "\"rssi\":" + String(WiFi.RSSI(idx));
            json += "}";
            if (i < n - 1) json += ",";
        }
    }
    json += "]}";
    WiFi.scanDelete();
    s_server.send(200, "application/json", json);
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

// Web API - 获取系统与网络配置
static void handle_get_sysconfig() {
    String json = "{";
    json += "\"ssid\":\"" + s_sta_ssid + "\",";
    json += "\"pass\":\"" + s_sta_password + "\",";
    json += "\"name\":\"" + s_device_name + "\",";
    json += "\"broker\":\"" + s_mqtt_broker + "\",";
    json += "\"port\":" + String(s_mqtt_port);
    json += "}";
    s_server.send(200, "application/json", json);
}

// Web API - 修改系统与网络配置
static void handle_post_sysconfig() {
    bool changed = false;
    if (s_server.hasArg("ssid")) {
        String val = s_server.arg("ssid");
        if (val.length() > 0 && val != s_sta_ssid) {
            s_sta_ssid = val;
            s_prefs.putString("sta_ssid", val);
            changed = true;
        }
    }
    if (s_server.hasArg("password")) {
        String val = s_server.arg("password");
        if (val != s_sta_password) {
            s_sta_password = val;
            s_prefs.putString("sta_pass", val);
            changed = true;
        }
    }
    if (s_server.hasArg("name")) {
        String val = s_server.arg("name");
        if (val.length() > 0 && val != s_device_name) {
            s_device_name = val;
            s_prefs.putString("dev_name", val);
            changed = true;
        }
    }
    if (s_server.hasArg("broker")) {
        String val = s_server.arg("broker");
        if (val.length() > 0 && val != s_mqtt_broker) {
            s_mqtt_broker = val;
            s_prefs.putString("mqtt_broker", val);
            changed = true;
        }
    }
    if (s_server.hasArg("port")) {
        int val = s_server.arg("port").toInt();
        if (val > 0 && val != s_mqtt_port) {
            s_mqtt_port = val;
            s_prefs.putInt("mqtt_port", val);
            changed = true;
        }
    }

    if (changed) {
        Serial.println("[WebConfig] System configurations updated in NVS.");
    }
    s_server.send(200, "text/plain", "OK");
}

void web_config_init() {
    // 1. 初始化 Preferences 非易失性存储
    s_prefs.begin("sensor_map", false);

    // 加载 4 路映射配置（如果无，使用 config.h 定义的缺省位置）
    s_channel_map[0] = s_prefs.getInt("map0", SENSOR1_CHIP * 4 + SENSOR1_CHAN);
    s_channel_map[1] = s_prefs.getInt("map1", SENSOR2_CHIP * 4 + SENSOR2_CHAN);
    s_channel_map[2] = s_prefs.getInt("map2", SENSOR3_CHIP * 4 + SENSOR3_CHAN);
    s_channel_map[3] = s_prefs.getInt("map3", SENSOR4_CHIP * 4 + SENSOR4_CHAN);

    // 加载 12 个通道的阈值偏移（默认为 50）
    for (int i = 0; i < 12; i++) {
        String key = "thr" + String(i);
        s_threshold_offset[i] = s_prefs.getInt(key.c_str(), 50);
    }

    // 加载 STA Wi-Fi 配置与系统参数
    s_sta_ssid = s_prefs.getString("sta_ssid", FACTORY_WIFI_SSID);
    s_sta_password = s_prefs.getString("sta_pass", FACTORY_WIFI_PASSWORD);
    s_device_name = s_prefs.getString("dev_name", FACTORY_DEVICE_NAME);
    s_mqtt_broker = s_prefs.getString("mqtt_broker", FACTORY_MQTT_BROKER);
    s_mqtt_port = s_prefs.getInt("mqtt_port", FACTORY_MQTT_PORT);
    
    // 加载轮询延时参数
    s_poll_delay = s_prefs.getInt("poll_delay", 50);

    Serial.printf("[WebConfig] Loaded Mapping: %d, %d, %d, %d\n",
                  s_channel_map[0], s_channel_map[1], s_channel_map[2], s_channel_map[3]);
    Serial.printf("[WebConfig] Loaded WiFi STA SSID: %s, Device Name: %s, MQTT Broker: %s:%d\n",
                  s_sta_ssid.c_str(), s_device_name.c_str(), s_mqtt_broker.c_str(), s_mqtt_port);

    // 2. 启动 WiFi AP 模式
    print_wifi_status("WebConfig BEFORE softAP");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
    print_wifi_status("WebConfig AFTER softAP");

    // 3. 挂载 Web 路由
    s_server.on("/", HTTP_GET, []() {
        s_server.send_P(200, "text/html", INDEX_HTML);
    });

    s_server.on("/api/data", HTTP_GET, handle_get_data);
    s_server.on("/api/config", HTTP_GET, handle_get_config);
    s_server.on("/api/config", HTTP_POST, handle_post_config);
    
    s_server.on("/api/sysconfig", HTTP_GET, handle_get_sysconfig);
    s_server.on("/api/sysconfig", HTTP_POST, handle_post_sysconfig);
    s_server.on("/api/wifi", HTTP_GET, handle_get_sysconfig);
    s_server.on("/api/wifi", HTTP_POST, handle_post_sysconfig);

    s_server.on("/api/threshold", HTTP_POST, handle_post_threshold);
    s_server.on("/api/polldelay", HTTP_GET, handle_get_polldelay);
    s_server.on("/api/polldelay", HTTP_POST, handle_post_polldelay);
    s_server.on("/api/scan", HTTP_GET, handle_wifi_scan);

    s_server.begin();
    Serial.println("[WebConfig] Built-in Web Server started on port 80");
}

void web_config_loop() {
    // 强制检测防回退：如果模式被外部库强制修改回了 STA，在此迅速恢复为 AP_STA 并重新开启 softAP
    if (WiFi.getMode() == WIFI_STA) {
        Serial.println("[WebConfig] WiFi mode was reverted to STA. Restoring AP_STA and restarting softAP...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
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

String get_sta_ssid() {
    return s_sta_ssid;
}

String get_sta_password() {
    return s_sta_password;
}

int get_channel_threshold(int ch_idx) {
    if (ch_idx < 0 || ch_idx >= 12) return 50;
    return s_threshold_offset[ch_idx];
}

int get_poll_delay() {
    return s_poll_delay;
}

String get_device_name() {
    return s_device_name;
}

String get_mqtt_broker() {
    return s_mqtt_broker;
}

int get_mqtt_port() {
    return s_mqtt_port;
}
