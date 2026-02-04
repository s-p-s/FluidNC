// Copyright (c) 2026
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Sensors/TCS3472.h"

#include "Machine/MachineConfig.h"
#include "string_util.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
    constexpr uint8_t TCS_CMD_BIT      = 0x80;
    constexpr uint8_t TCS_CMD_AUTO_INC = 0x20;

    constexpr uint8_t TCS_REG_ENABLE = 0x00;
    constexpr uint8_t TCS_REG_ATIME  = 0x01;
    constexpr uint8_t TCS_REG_WTIME  = 0x03;
    constexpr uint8_t TCS_REG_CONTROL = 0x0F;
    constexpr uint8_t TCS_REG_ID      = 0x12;
    constexpr uint8_t TCS_REG_CDATAL  = 0x14;

    constexpr uint8_t TCS_ENABLE_PON = 0x01;
    constexpr uint8_t TCS_ENABLE_AEN = 0x02;

    uint8_t gain_to_reg(int32_t gain) {
        switch (gain) {
            case 1:
                return 0;
            case 4:
                return 1;
            case 16:
                return 2;
            case 60:
                return 3;
            default:
                return 1;
        }
    }

    bool is_valid_gain(int32_t gain) {
        return gain == 1 || gain == 4 || gain == 16 || gain == 60;
    }

    uint8_t integration_ms_to_atime(int32_t integration_ms) {
        // ATIME = 256 - cycles; cycle = 2.4ms
        // Clamp to 1..256 cycles.
        if (integration_ms < 3) {
            integration_ms = 3;
        }
        if (integration_ms > 614) {
            integration_ms = 614;
        }

        // Round to nearest cycle.
        int32_t cycles = (integration_ms * 10 + 12) / 24;  // integration_ms/2.4
        if (cycles < 1) {
            cycles = 1;
        }
        if (cycles > 256) {
            cycles = 256;
        }
        return static_cast<uint8_t>(256 - cycles);
    }
}

std::vector<TCS3472*>& TCS3472::_sensors() {
    static std::vector<TCS3472*> list;
    return list;
}

const std::vector<TCS3472*>& TCS3472::sensors() {
    return _sensors();
}

TCS3472::TCS3472(const char* name) : ConfigurableModule(name) {
    _sensors().push_back(this);
}

void TCS3472::group(Configuration::HandlerBase& handler) {
    handler.item("id", _id, 0, 255);
    handler.item("i2c_num", _i2c_num);
    handler.item("i2c_address", _address);
    handler.item("i2c_frequency", _frequency, 10000, 400000);

    handler.item("integration_ms", _integration_ms, 3, 614);
    handler.item("gain", _gain, 1, 60);

    handler.item("illumination", _illumination);
    handler.item("illumination_pin", _illumination_pin);
}

void TCS3472::validate() {
    if (!is_valid_gain(_gain)) {
        log_config_error("tcs3472 gain must be 1, 4, 16, or 60");
    }
}

void TCS3472::afterParse() {
    _error = false;
    _ready = false;

#if !MAX_N_I2C
    log_error("tcs3472 configured but MAX_N_I2C=0");
    _error = true;
    return;
#else
    if (_i2c_num >= MAX_N_I2C) {
        log_error("tcs3472 i2c_num out of range: " << int(_i2c_num));
        _error = true;
        return;
    }
    if (!config || !config->_i2c[_i2c_num]) {
        log_error("i2c" << int(_i2c_num) << " section must be defined for tcs3472");
        _error = true;
        return;
    }
    _i2c = config->_i2c[_i2c_num];
#endif
}

void TCS3472::init() {
    if (_error) {
        return;
    }
#if MAX_N_I2C
    if (!_i2c) {
        _error = true;
        return;
    }

    if (_illumination_pin.defined()) {
        _illumination_pin.setAttr(Pin::Attr::Output);
        if (_illumination) {
            _illumination_pin.on();
        } else {
            _illumination_pin.off();
        }
    }

    log_info("tcs3472 id=" << int(_id) << " i2c=" << int(_i2c_num) << " addr=" << to_hex(_address)
                           << " integration_ms=" << _integration_ms << " gain=" << _gain);

    _ready = configure_sensor();
    if (!_ready) {
        log_error("tcs3472 init failed (id=" << int(_id) << ")");
        _error = true;
    }
#endif
}

bool TCS3472::write_u8(uint8_t reg, uint8_t value) {
#if !MAX_N_I2C
    return false;
#else
    uint8_t buf[2] = { static_cast<uint8_t>(TCS_CMD_BIT | reg), value };
    return _i2c && (_i2c->write(_address, buf, sizeof(buf)) == int(sizeof(buf)));
#endif
}

bool TCS3472::read_u8(uint8_t reg, uint8_t& value) {
#if !MAX_N_I2C
    (void)reg;
    (void)value;
    return false;
#else
    uint8_t cmd = static_cast<uint8_t>(TCS_CMD_BIT | reg);
    if (!_i2c || _i2c->write(_address, &cmd, 1) != 1) {
        return false;
    }
    return _i2c->read(_address, &value, 1) == 1;
#endif
}

bool TCS3472::configure_sensor() {
#if !MAX_N_I2C
    return false;
#else
    // Disable first
    if (!write_u8(TCS_REG_ENABLE, 0x00)) {
        return false;
    }

    uint8_t id_reg = 0;
    if (!read_u8(TCS_REG_ID, id_reg)) {
        return false;
    }

    // Program integration time + gain
    if (!write_u8(TCS_REG_ATIME, integration_ms_to_atime(_integration_ms))) {
        return false;
    }
    if (!write_u8(TCS_REG_CONTROL, gain_to_reg(_gain))) {
        return false;
    }

    // Default wait time.
    (void)write_u8(TCS_REG_WTIME, 0xFF);

    // Power on
    if (!write_u8(TCS_REG_ENABLE, TCS_ENABLE_PON)) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(3));

    // Enable RGBC
    if (!write_u8(TCS_REG_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN)) {
        return false;
    }

    log_info("tcs3472 chip id register: " << to_hex(id_reg));
    return true;
#endif
}

bool TCS3472::read_rgcb(Reading& out) {
#if !MAX_N_I2C
    (void)out;
    return false;
#else
    uint8_t cmd = static_cast<uint8_t>(TCS_CMD_BIT | TCS_CMD_AUTO_INC | TCS_REG_CDATAL);
    if (!_i2c || _i2c->write(_address, &cmd, 1) != 1) {
        return false;
    }
    uint8_t buf[8] = {};
    if (_i2c->read(_address, buf, sizeof(buf)) != int(sizeof(buf))) {
        return false;
    }

    auto u16 = [&](int idx) -> uint16_t { return uint16_t(buf[idx]) | (uint16_t(buf[idx + 1]) << 8); };
    out.clear = u16(0);
    out.red   = u16(2);
    out.green = u16(4);
    out.blue  = u16(6);
    out.valid = true;
    return true;
#endif
}

bool TCS3472::read(Reading& out, bool fresh) {
#if !MAX_N_I2C
    (void)out;
    (void)fresh;
    return false;
#else
    if (!_ready || _error) {
        return false;
    }

    if (fresh) {
        // Wait for a full integration cycle to accumulate new data.
        vTaskDelay(pdMS_TO_TICKS(_integration_ms));
    }

    if (!read_rgcb(out)) {
        return false;
    }

    _last = out;
    return true;
#endif
}

bool TCS3472::last(Reading& out) const {
    out = _last;
    return _last.valid;
}

namespace {
    ConfigurableModuleFactory::InstanceBuilder<TCS3472> tcs_module __attribute__((init_priority(104))) ("tcs3472");
}
