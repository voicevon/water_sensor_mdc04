#pragma once

#include <Arduino.h>

// SPA 暗色玻璃拟态多 Tab 前端 HTML（存储于 Flash PROGMEM，不占用 RAM）
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
            padding: 1.5rem 1rem;
            display: flex;
            flex-direction: column;
            gap: 0.75rem;
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
            font-size: 1rem;
            font-weight: 500;
            transition: color 0.2s;
            cursor: pointer;
            padding: 0.4rem 0;
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
        <div class="logo">传感器节点</div>
        <button class="menu-btn" onclick="toggleDrawer(true)">☰</button>
    </header>

    <!-- 侧边抽屉菜单 -->
    <div class="overlay" id="overlay" onclick="toggleDrawer(false)"></div>
    <div class="drawer" id="drawer">
        <div class="nav-link active" data-tab="tab-monitor" onclick="switchTab(this)">实时监控</div>
        <div class="nav-link" data-tab="tab-algo" onclick="switchTab(this)">传感器设置</div>
        <div class="nav-link" data-tab="tab-mapping" onclick="switchTab(this)">芯片通道配置</div>
        <div class="nav-link" data-tab="tab-wifi" onclick="switchTab(this)">网络配置</div>
        <div class="nav-link" data-tab="tab-about" onclick="switchTab(this)">关于</div>
    </div>

    <div class="container">
        <!-- TAB 1: 实时监控 -->
        <div id="tab-monitor" class="tab-content active">
            <div class="card">
                <div class="card-title">
                    <span>3 物理通道数据列表</span>
                    <span style="font-size: 0.8rem; font-weight: normal; color: var(--text-muted);">自动刷新(1Hz)</span>
                </div>
                <div class="grid-12" id="sensor-grid">
                    <!-- JS 渲染 -->
                </div>
            </div>
        </div>

        <!-- TAB ALGO: 传感器设置 -->
        <div id="tab-algo" class="tab-content">
            <div id="algo-cards-container">
                <!-- 由 JS 渲染 3 个通道的设置卡片 -->
            </div>
        </div>

        <!-- TAB 2: 芯片通道配置 -->
        <div id="tab-mapping" class="tab-content">
            <div class="card">
                <div class="card-title">MDC04 芯片有效通道配置</div>
                <form id="mapping-form" onsubmit="saveMapping(event)">
                    <div class="form-group">
                        <label for="chip0">芯片 1 有效通道</label>
                        <select id="chip0" name="chip0"></select>
                    </div>
                    <div class="form-group">
                        <label for="chip1">芯片 2 有效通道</label>
                        <select id="chip1" name="chip1"></select>
                    </div>
                    <div class="form-group">
                        <label for="chip2">芯片 3 有效通道</label>
                        <select id="chip2" name="chip2"></select>
                    </div>
                    <button type="submit" class="btn" style="width: 100%;">保存配置</button>
                </form>
            </div>
        </div>

        <!-- TAB 3: 网络配置 -->
        <div id="tab-wifi" class="tab-content">
            <div class="card" style="padding: 1rem 1.5rem; margin-bottom: 1.25rem; display: flex; flex-direction: column; gap: 0.75rem; align-items: flex-start; font-size: 0.95rem;">
                <div style="display: flex; align-items: center; gap: 0.5rem;">
                    <span style="color: var(--text-muted);">WiFi:</span>
                    <span id="wifi-status" class="status-badge dry">检测中...</span>
                </div>
                <div style="display: flex; align-items: center; gap: 0.5rem;">
                    <span style="color: var(--text-muted);">MQTT:</span>
                    <span id="mqtt-status" class="status-badge dry">检测中...</span>
                </div>
            </div>
            <div class="card">
                <div class="card-title">网络参数配置</div>
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
                        <label for="name">节点名称 (DEVICE_NAME)</label>
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
                    <p>节点名称：sensor</p>
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

    <div id="toast">保存成功，已即时生效！</div>

    <script>
        const CHANNELS = [
            "次氯酸钠", "碳源", "铁盐"
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
            } else if (targetId === 'tab-algo') {
                initAlgoCards();
            }
        }

        function onAlgoTypeChange(idx) {
            const type = parseInt(document.getElementById(`algo-type-${idx}`).value);
            document.getElementById(`algo-dynamic-params-${idx}`).style.display = (type === 0) ? 'block' : 'none';
            document.getElementById(`algo-discrete-params-${idx}`).style.display = (type === 1) ? 'block' : 'none';
            document.getElementById(`algo-envelope-params-${idx}`).style.display = (type === 2) ? 'block' : 'none';
        }

        let algoInitialized = false;

        function initAlgoCards() {
            if(algoInitialized) return;
            const container = document.getElementById('algo-cards-container');
            container.innerHTML = '';
            
            for(let i=0; i<3; i++) {
                const card = document.createElement('div');
                card.className = 'card';
                card.innerHTML = `
                    <div class="card-title">传感器设置 - ${CHANNELS[i]}</div>
                    <form id="algo-form-${i}" onsubmit="saveSensorConfig(event, ${i})" oninput="this.dataset.edited='true'">
                        <div class="form-group">
                            <label>检测算法</label>
                            <select id="algo-type-${i}" name="type" onchange="onAlgoTypeChange(${i})">
                                <option value="0">动态阈值（施密特迟滞）</option>
                                <option value="1">离散方差</option>
                                <option value="2">包络范围</option>
                            </select>
                        </div>
                        
                        <div id="algo-dynamic-params-${i}" style="display:block;">
                            <div class="form-group">
                                <label>阈值偏移量 (Threshold Offset)</label>
                                <input type="number" name="offset" min="-500" max="500" placeholder="默认 50">
                                <span style="font-size:0.72rem;color:var(--text-muted);margin-top:0.25rem;">此参数为滤波线偏离慢速基准线以触发水警报的电容增量</span>
                            </div>
                        </div>

                        <div id="algo-discrete-params-${i}" style="display:none;">
                            <div class="form-group">
                                <label>方差触发阈值 (var_threshold)</label>
                                <input type="number" name="var_thr" min="0" max="100000" placeholder="默认 5000">
                                <span style="font-size:0.72rem;color:var(--text-muted);margin-top:0.25rem;">平滑方差超过此值时判定为有水</span>
                            </div>
                        </div>

                        <div id="algo-envelope-params-${i}" style="display:none;">
                            <div class="form-group">
                                <label>包络窗口大小 (env_window, 1~120)</label>
                                <input type="number" name="env_win" min="1" max="120" placeholder="默认 30">
                            </div>
                            <div class="form-group">
                                <label>上触发偏置 (env_upper_offset)</label>
                                <input type="number" name="env_up" min="0" max="5000" placeholder="默认 500">
                            </div>
                            <div class="form-group">
                                <label>下恢复偏置 (env_lower_offset)</label>
                                <input type="number" name="env_lo" min="0" max="5000" placeholder="默认 300">
                            </div>
                        </div>

                        <button type="submit" class="btn" style="width:100%;margin-top:1rem;">保存该传感器配置</button>
                    </form>
                `;
                container.appendChild(card);
            }
            algoInitialized = true;
        }

        const CHIP_CHANNELS = ["通道 1", "通道 2", "通道 3", "通道 4"];

        // 渲染下拉框
        function renderMappingOptions(channels) {
            for (let i = 0; i < 3; i++) {
                const select = document.getElementById(`chip${i}`);
                if (!select) continue;
                select.innerHTML = '';
                CHIP_CHANNELS.forEach((name, idx) => {
                    const option = document.createElement('option');
                    option.value = idx;
                    option.textContent = name;
                    if (channels && channels[i] == idx) {
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
                renderMappingOptions(data.chip_channels);
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

                // 更新 Wi-Fi 和 MQTT 状态显示
                const wifiBadge = document.getElementById('wifi-status');
                const mqttBadge = document.getElementById('mqtt-status');
                if (wifiBadge) {
                    wifiBadge.textContent = data.wifi_connected ? '已连接' : '未连接';
                    wifiBadge.className = data.wifi_connected ? 'status-badge water' : 'status-badge dry';
                }
                if (mqttBadge) {
                    mqttBadge.textContent = data.mqtt_connected ? '已连接' : '未连接';
                    mqttBadge.className = data.mqtt_connected ? 'status-badge water' : 'status-badge dry';
                }

                const grid = document.getElementById('sensor-grid');
                grid.innerHTML = '';

                data.sensors.forEach((s, idx) => {
                    const isWater = s.detected;
                    const algoNames = ['动态阈值', '离散方差', '包络范围'];
                    const algoName = algoNames[s.algo_type] || '未知';
                    const algoColors = ['var(--accent-blue)', '#a78bfa', '#f59e0b'];
                    const algoColor = algoColors[s.algo_type] || 'var(--text-muted)';
                    const item = document.createElement('div');
                    item.className = `sensor-item ${isWater ? 'water-detected' : ''}`;
                    item.innerHTML = `
                        <div class="sensor-header">
                            <span class="sensor-id">${CHANNELS[idx]}</span>
                            <span class="status-badge ${isWater ? 'water' : 'dry'}">${isWater ? '有水' : '无水'}</span>
                        </div>
                        <div class="sensor-meta">
                            <div>原始值: <span>${(s.raw_val).toFixed(2)} pF</span></div>
                            <div>滤波值: <span>${s.filtered}</span></div>
                            <div>基准值: <span>${s.baseline}</span></div>
                            <div>阈值: <span>${s.threshold}</span></div>
                            <div style="margin-top:4px">算法: <span style="color:${algoColor};font-size:0.82rem;font-weight:600;">${algoName}</span></div>
                        </div>
                    `;
                    grid.appendChild(item);

                    // Update config form if not currently being edited
                    const form = document.getElementById(`algo-form-${idx}`);
                    if (form && form.dataset.edited !== 'true') {
                        form.elements['offset'].value = s.offset;
                        form.elements['type'].value = s.algo_type;
                        form.elements['var_thr'].value = s.var_thr;
                        form.elements['env_win'].value = s.env_win;
                        form.elements['env_up'].value = s.env_up;
                        form.elements['env_lo'].value = s.env_lo;
                        onAlgoTypeChange(idx);
                    }
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

        async function saveSensorConfig(e, idx) {
            e.preventDefault();
            const form = document.getElementById(`algo-form-${idx}`);
            const params = new URLSearchParams(new FormData(form));
            params.append('ch', idx);

            // Fetch current algo type to know which fields to keep
            const type = parseInt(form.elements['type'].value);
            if (type !== 0) params.delete('offset');
            if (type !== 1) params.delete('var_thr');
            if (type !== 2) { params.delete('env_win'); params.delete('env_up'); params.delete('env_lo'); }

            try {
                // Submit offset logic
                if (type === 0 && params.has('offset')) {
                    const tParams = new URLSearchParams();
                    tParams.append('ch', idx);
                    tParams.append('offset', params.get('offset'));
                    await fetch('/api/threshold', { method: 'POST', body: tParams });
                }

                // Submit algo logic
                const res = await fetch('/api/algo', { method: 'POST', body: params });
                
                if (res.ok) {
                    showToast("配置已保存，即时生效！");
                    form.dataset.edited = 'false';
                    updateData();
                } else {
                    alert("保存失败");
                }
            } catch (err) {
                alert("出错: " + err);
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
        setInterval(updateData, 1000);
    </script>
</body>
</html>
)rawhtml";
