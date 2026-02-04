// Copyright (c) 2026
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#pragma once

#include "Config.h"

#include "Module.h"

#include "Pin.h"

#if MAX_N_I2C
#    include "Machine/I2CBus.h"
#endif

#include <cstdint>
#include <vector>

class TCS3472 : public ConfigurableModule {
public:
    struct Reading {
        uint16_t clear = 0;
        uint16_t red   = 0;
        uint16_t green = 0;
        uint16_t blue  = 0;

        bool valid = false;
    };

    explicit TCS3472(const char* name);

    void validate() override;
    void afterParse() override;
    void group(Configuration::HandlerBase& handler) override;

    void init() override;

    uint8_t id() const { return _id; }
    bool    ready() const { return _ready; }

    int32_t integration_ms() const { return _integration_ms; }
    int32_t gain() const { return _gain; }

    bool read(Reading& out, bool fresh);
    bool last(Reading& out) const;

    static const std::vector<TCS3472*>& sensors();

private:
    static std::vector<TCS3472*>& _sensors();

    bool write_u8(uint8_t reg, uint8_t value);
    bool read_u8(uint8_t reg, uint8_t& value);
    bool read_rgcb(Reading& out);

    bool configure_sensor();

private:
    uint8_t _i2c_num  = 0;
    uint8_t _address  = 0x29;
    uint8_t _id       = 0;
    int32_t _frequency = 100000;

    int32_t _integration_ms = 50;
    int32_t _gain           = 4;

    bool _illumination = false;
    Pin  _illumination_pin;

    bool _error = false;
    bool _ready = false;

    Reading _last;

#if MAX_N_I2C
    Machine::I2CBus* _i2c = nullptr;
#endif
};
