#include "Sensor.h"
#include "config.h"

Sensor::Sensor(int id, int thresholdOffset)
    : _id(id), _thresholdOffset(thresholdOffset) {
    reset();
}

void Sensor::reset() {
    _rawValue = 0;
    _filteredValue = 0;
    _baselineValue = 0;
    _lastState = SensorState::NO_WATER;
    _hasWaterStartTime = 0;

    _maHead = 0;
    _maCount = 0;
    _maSum = 0;
    for (int i = 0; i < MA_WINDOW; i++) {
        _maBuf[i] = 0;
    }

    _baseHead = 0;
    _baseCount = 0;
    _baseSum = 0;
    for (int i = 0; i < BASELINE_WINDOW; i++) {
        _baseBuf[i] = 0;
    }
}

uint16_t Sensor::pushFilter(uint16_t value) {
    if (_maCount < MA_WINDOW) {
        _maBuf[_maHead] = value;
        _maSum += value;
        _maCount++;
    } else {
        _maSum -= _maBuf[_maHead];
        _maBuf[_maHead] = value;
        _maSum += value;
    }
    _maHead = (_maHead + 1) % MA_WINDOW;
    return (uint16_t)(_maSum / _maCount);
}

uint16_t Sensor::pushBaseline(uint16_t value) {
    if (_baseCount < BASELINE_WINDOW) {
        _baseBuf[_baseHead] = value;
        _baseSum += value;
        _baseCount++;
    } else {
        _baseSum -= _baseBuf[_baseHead];
        _baseBuf[_baseHead] = value;
        _baseSum += value;
    }
    _baseHead = (_baseHead + 1) % BASELINE_WINDOW;
    return (uint16_t)(_baseSum / _baseCount);
}

uint16_t Sensor::getThreshold() const {
    int thresh;
    if (_lastState == SensorState::NO_WATER) {
        thresh = (int)_baselineValue + _thresholdOffset;
    } else {
        thresh = (int)_baselineValue - _thresholdOffset;
    }
    if (thresh < 0) return 0;
    if (thresh > 65535) return 65535;
    return (uint16_t)thresh;
}

void Sensor::pushRaw(uint16_t value) {
    _rawValue = value;
    _filteredValue = pushFilter(value);
    _baselineValue = pushBaseline(_filteredValue);

    // 1. 如果当前处于有水状态，检查看门狗是否超时
    if (_lastState == SensorState::HAS_WATER) {
        if (_hasWaterStartTime == 0) {
            _hasWaterStartTime = millis();
        }
        if (millis() - _hasWaterStartTime >= WATER_WATCHDOG_TIMEOUT_MS) {
            Serial.printf("[WDT] 物理通道 %d 持续有水达到 5 小时，强制复位为 DRY(无水) 状态！当前电容：%u，基准线：%u\n",
                          _id, _filteredValue, _baselineValue);
            _lastState = SensorState::NO_WATER;
            _hasWaterStartTime = 0;
            if (_stateChangeCb) {
                _stateChangeCb(_id, SensorState::NO_WATER);
            }
        }
    } else {
        // 如果处于无水状态，清空有水起始时间
        _hasWaterStartTime = 0;
    }

    // 2. 状态机逻辑评估
    uint16_t curThreshold = getThreshold();
    SensorState currentState = _lastState;
    SensorState nextState = currentState;

    if (currentState == SensorState::NO_WATER) {
        if (_thresholdOffset >= 0) {
            if (_filteredValue > curThreshold) {
                nextState = SensorState::HAS_WATER;
            } else {
                nextState = SensorState::NO_WATER;
            }
        } else { // _thresholdOffset < 0
            if (_filteredValue < curThreshold) {
                nextState = SensorState::HAS_WATER;
            } else {
                nextState = SensorState::NO_WATER;
            }
        }
    } else { // HAS_WATER
        if (_thresholdOffset >= 0) {
            if (_filteredValue < curThreshold) {
                nextState = SensorState::NO_WATER;
            } else {
                nextState = SensorState::HAS_WATER;
            }
        } else { // _thresholdOffset < 0
            if (_filteredValue > curThreshold) {
                nextState = SensorState::NO_WATER;
            } else {
                nextState = SensorState::HAS_WATER;
            }
        }
    }

    if (nextState != currentState) {
        _lastState = nextState;
        if (nextState == SensorState::HAS_WATER) {
            _hasWaterStartTime = millis();
        } else {
            _hasWaterStartTime = 0;
        }
        if (_stateChangeCb) {
            _stateChangeCb(_id, nextState);
        }
    }
}

void Sensor::onStateChange(StateChangeCallback cb) {
    _stateChangeCb = cb;
}
