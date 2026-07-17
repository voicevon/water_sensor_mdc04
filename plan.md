# 将 water_sensor_logger 算法移植到 water_sensor_mdc04

## 背景

`water_sensor_logger` 是一个 Python 离线分析平台，其 `sensor_logic.py` 实现了三套水位检测算法，经过充分调试和 Fix 迭代，已被证明是稳定的"黄金版本"。

`water_sensor_mdc04` 是 ESP32 嵌入式固件，其 `Sensor.cpp` 中目前只有一套算法（动态施密特阈值 + 滑动均值滤波），且存在若干与 Python 版本的算法行为差异。

**移植目标**：将 Python 端的三套算法（含所有 Fix）同步到 C++ 固件，使硬件端算法行为与分析平台完全一致。

---

## 差异分析：Python vs C++ 现有算法

### 算法一：`SensorAlgorithm` ↔ `Sensor`（施密特双向迟滞 + 滑动均值）

| 特性 | Python (`sensor_logic.py`) | C++ (`Sensor.cpp`) | 状态 |
|------|---|---|---|
| 看门狗触发后立即 early return | ✅ Fix #5：触发后跳过状态机 | ❌ 无 early return，复位后立刻重跑状态机，看门狗在信号仍高于阈值时会立即失效 | **需修复** |
| `thresholdOffset < 0` 反向触发 | ❌ 不支持（state==0 时 filtered > threshold 即触发有水） | ✅ 支持反向偏移（offset < 0 时逻辑反转） | 双方设计差异，需讨论 |
| WDT 计时器与状态机的双写问题 | ✅ 避免了同帧双写 | ❌ 状态机帧结束后，`_hasWaterStartTime` 在状态翻转时被第二次写入，导致计时起点漂移 | **需修复** |

### 算法二：`DiscreteVarianceAlgorithm`（离散方差算法）

| 特性 | Python | C++ |
|------|---|---|
| 该算法是否存在 | ✅ 完整实现 | ❌ **不存在** |

### 算法三：`EnvelopeRangeAlgorithm`（包络范围算法）

| 特性 | Python | C++ |
|------|---|---|
| 该算法是否存在 | ✅ 完整实现（含 Fix #6：`dry_baseline` 用 `None` 初始化） | ❌ **不存在** |

---

## Open Questions

> [!IMPORTANT]
> **Q1：算法选择机制设计**
> 
> Python 端通过 HTTP Query 参数实时切换算法，C++ 固件端应如何选择使用哪套算法？建议：
> - 方案 A（推荐）：NVS 持久化配置 + Web 配置页面切换，重启后保持。
> - 方案 B：编译期 `#define` 选择，通过 platformio.ini 的 `build_flags` 控制。
> - 方案 C：只移植施密特算法的 Bug Fix，暂不移植另外两套算法。

> [!IMPORTANT]
> **Q2：`thresholdOffset < 0` 反向偏移逻辑**
>
> C++ 端支持负偏移（信号下降时触发有水），Python 端不支持。这两端是否需要对齐？
> - 方案 A：Python 端增加支持（但会改变 logger 行为）。
> - 方案 B：C++ 端保留此特性，移植时维持现有行为（两端存在合理差异）。

> [!NOTE]
> **Q3：包络算法的 `max(buf)` / `min(buf)` 在 C++ 端的性能**
>
> Python 用 `deque` 存包络窗口，每次调用 `max(buf)` 是 O(N)。C++ 嵌入式环境中窗口默认为 30 个点，O(30) 完全可接受。移植时保持 O(N) 实现即可，无需单调队列优化。

---

## 核心移植工作：Sensor.cpp Bug Fix

**无论上方哪种方案被选中，以下两个 Fix 都必须执行：**

### Fix A：WDT 看门狗 early return（对应 Python Fix #5）

**问题**：`Sensor::pushRaw()` 中，WDT 超时触发 `_lastState = NO_WATER` 后，代码继续往下执行状态机逻辑；若此时信号仍高于阈值，状态机会在同一帧内立即切回 `HAS_WATER`，导致 WDT 完全失效。

**修复**：WDT 触发后立即 `return`，跳过下方的状态机评估代码。

```cpp
// Sensor.cpp pushRaw() 中看门狗触发块
if (millis() - _hasWaterStartTime >= WATER_WATCHDOG_TIMEOUT_MS) {
    _lastState = SensorState::NO_WATER;
    _hasWaterStartTime = 0;
    if (_stateChangeCb) _stateChangeCb(_id, SensorState::NO_WATER);
    return;  // ← 新增 early return，阻止状态机重评估
}
```

### Fix B：状态翻转时避免 `_hasWaterStartTime` 双写

**问题**：在 WDT 块（行 78-93）中，若处于 HAS_WATER 状态且 WDT 未超时，`_hasWaterStartTime = millis()` 会被重新赋值。随后状态机（行 130-140）在状态不变时又不执行写入，但在状态翻转到 HAS_WATER 时会第二次赋值，此时 `millis()` 已经变化，计时起点偏移一帧。修复：WDT 块中不应主动刷新 `_hasWaterStartTime`，仅在初始化时赋值。

---

## 方案 A（推荐）详细实施计划

若用户选择方案 A（NVS + Web 可切换算法），完整变更如下：

### 层 1：算法类扩展（Sensor.h / Sensor.cpp）

#### [MODIFY] [Sensor.h](file:///d:/Software/antigravity/water_sensor_mdc04/src/Sensor.h)
- 新增 `enum class AlgoType { DYNAMIC, DISCRETE, ENVELOPE }` 类型定义。
- `Sensor` 类新增 `_algoType` 成员及 `setAlgoType()` / `getAlgoType()` 方法。
- 新增离散方差算法所需的私有成员：`_baseBuf2`, `_varBuf`, `_baseSum2`, `_varSum`, `_varSmoothed`。
- 新增包络算法所需的私有成员：`_envBuf`, `_dryBaseline`, `_envUpper`, `_envLower`，以及两个窗口参数。
- 新增各算法的私有 `_runDiscrete()` / `_runEnvelope()` 方法。

#### [MODIFY] [Sensor.cpp](file:///d:/Software/antigravity/water_sensor_mdc04/src/Sensor.cpp)
- **Fix A**：WDT 触发后新增 `return` early return。
- **Fix B**：修正 WDT 块中 `_hasWaterStartTime` 的赋值逻辑。
- 实现 `_runDiscrete()` — 直接翻译 Python `DiscreteVarianceAlgorithm.process_point()`。
- 实现 `_runEnvelope()` — 直接翻译 Python `EnvelopeRangeAlgorithm.process_point()`（含 Fix #6 的 None→optional 等价处理）。
- `pushRaw()` 中根据 `_algoType` 分发调用。

### 层 2：NVS 配置扩展（nvs_config.h / nvs_config.cpp）

#### [MODIFY] [nvs_config.h](file:///d:/Software/antigravity/water_sensor_mdc04/src/nvs_config.h)
- 新增 `nvs_set_algo_type(int ch, int type)` 与 `get_algo_type(int ch)` 声明。
- 新增离散方差算法参数配置：`nvs_set_var_threshold(int ch, int val)` 等。

#### [MODIFY] [nvs_config.cpp](file:///d:/Software/antigravity/water_sensor_mdc04/src/nvs_config.cpp)
- 实现以上接口，NVS Key 采用 `al0`~`al11` 存储算法类型，`vt0`~`vt11` 存储方差阈值。

### 层 3：Web 配置接口（web_config.h / web_config.cpp）

#### [MODIFY] [web_config.cpp](file:///d:/Software/antigravity/water_sensor_mdc04/src/web_config.cpp)
- 新增 `POST /api/algo` 接口：接受 `ch`, `type`, `var_threshold` 等参数，写入 NVS 并实时应用。
- `handle_get_data()` 响应中新增 `algo_type` 字段。

### 层 4：主循环（main.cpp）

#### [MODIFY] [main.cpp](file:///d:/Software/antigravity/water_sensor_mdc04/src/main.cpp)
- `setup()` 中新增从 NVS 恢复各通道算法类型的初始化代码。

### 层 5：Web 配置页面（index_html.h）

#### [MODIFY] [index_html.h](file:///d:/Software/antigravity/water_sensor_mdc04/src/index_html.h)
- 每个通道的配置卡片中增加算法下拉选择框（动态阈值 / 离散方差 / 包络范围）。
- 离散方差算法增加方差阈值输入框。

---

## 方案 C（最小改动）仅 Fix Bug

若只执行 Bug Fix，只需修改：
- **[MODIFY] [Sensor.cpp](file:///d:/Software/antigravity/water_sensor_mdc04/src/Sensor.cpp)**：Fix A + Fix B，约 5 行代码改动。

---

## 验证计划

### 自动化测试
- 编译固件：`pio run`，确保零报错。
- 上传固件并监控串口，验证 WDT 日志在持续有水 5 小时后正确触发且不反弹。

### 人工验证
- 通过 Web 配置页面切换算法类型，观察各通道的 `filtered` 和 `state` 数值变化是否符合各算法特征。
- 对照 water_sensor_logger 的历史 CSV 数据，用 Python 端和 C++ 端分别计算，验证输出一致性。
