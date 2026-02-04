// Copyright (c) 2026
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Sensors/TCS3472.h"

#include "Parameters.h"
#include "Settings.h"  // get_param(), paramIsJSON()
#include "string_util.h"

#include <cstdlib>
#include <string>
#include <string_view>

namespace {
    const TCS3472* find_sensor_by_id(uint8_t id) {
        for (auto* s : TCS3472::sensors()) {
            if (s && s->id() == id) {
                return s;
            }
        }
        return nullptr;
    }

    TCS3472* find_sensor_by_id_mut(uint8_t id) {
        for (auto* s : TCS3472::sensors()) {
            if (s && s->id() == id) {
                return s;
            }
        }
        return nullptr;
    }

    std::string first_token(std::string_view s) {
        while (!s.empty() && s.front() == ' ') {
            s.remove_prefix(1);
        }
        size_t end = 0;
        while (end < s.size() && s[end] != ' ') {
            end++;
        }
        return std::string(s.substr(0, end));
    }

    bool parse_u8(std::string_view s, uint8_t& out) {
        if (s.empty()) {
            return false;
        }
        char* end = nullptr;
        long  v   = std::strtol(std::string(s).c_str(), &end, 10);
        if (!end || *end != '\0') {
            return false;
        }
        if (v < 0 || v > 255) {
            return false;
        }
        out = static_cast<uint8_t>(v);
        return true;
    }

    void set_reading_params(const std::string& prefix, const TCS3472::Reading& r) {
        if (prefix.empty()) {
            return;
        }

        auto setf = [&](const char* suffix, float value) {
            set_named_param((prefix + suffix).c_str(), value);
        };

        setf("_c", float(r.clear));
        setf("_r", float(r.red));
        setf("_g", float(r.green));
        setf("_b", float(r.blue));

        if (r.clear > 0) {
            setf("_rn", float(r.red) / float(r.clear));
            setf("_gn", float(r.green) / float(r.clear));
            setf("_bn", float(r.blue) / float(r.clear));
        } else {
            setf("_rn", 0.0f);
            setf("_gn", 0.0f);
            setf("_bn", 0.0f);
        }
    }

    void report_list(Channel& out) {
        if (TCS3472::sensors().empty()) {
            log_string(out, "No tcs3472 sensors configured");
            return;
        }

        log_string(out, "tcs3472 sensors:");
        for (auto* s : TCS3472::sensors()) {
            if (!s) {
                continue;
            }
            std::string line = "  id=" + std::to_string(int(s->id())) +
                               " ready=" + (s->ready() ? std::string("yes") : std::string("no")) +
                               " integration_ms=" + std::to_string(s->integration_ms()) +
                               " gain=" + std::to_string(s->gain());
            log_string(out, line.c_str());
        }
    }
}

Error tcs3472_command(const char* value, AuthenticationLevel auth_level, Channel& out) {
    (void)auth_level;

    if (!value || value[0] == '\0') {
        log_string(out, "Usage:");
        log_string(out, "  $TCS=LIST");
        log_string(out, "  $TCS=READ [id=n] [fresh=yes] [p=prefix] [json=yes]");
        log_string(out, "  $TCS=LAST [id=n] [p=prefix] [json=yes]");
        report_list(out);
        return Error::Ok;
    }

    std::string_view sv(value);
    std::string      cmd = first_token(sv);

    if (string_util::equal_ignore_case(cmd, "LIST")) {
        report_list(out);
        return Error::Ok;
    }

    bool want_json = paramIsJSON(value);

    uint8_t id = 0;
    {
        std::string s;
        if (get_param(value, "id=", s) || get_param(value, "ID=", s)) {
            uint8_t tmp = 0;
            if (parse_u8(s, tmp)) {
                id = tmp;
            }
        }
    }

    std::string prefix;
    if (get_param(value, "p=", prefix) || get_param(value, "P=", prefix) || get_param(value, "prefix=", prefix)) {
        // ok
    } else {
        prefix = "tcs";
    }

    if (string_util::equal_ignore_case(cmd, "LAST")) {
        auto* sensor = find_sensor_by_id(id);
        if (!sensor) {
            log_string(out, "tcs3472: sensor id not found");
            return Error::InvalidValue;
        }

        TCS3472::Reading r;
        if (!sensor->last(r)) {
            log_string(out, "tcs3472: no previous reading");
            return Error::Ok;
        }

        set_reading_params(prefix, r);

        if (want_json) {
            std::string j = "{\"id\":" + std::to_string(int(id)) +
                            ",\"c\":" + std::to_string(r.clear) +
                            ",\"r\":" + std::to_string(r.red) +
                            ",\"g\":" + std::to_string(r.green) +
                            ",\"b\":" + std::to_string(r.blue) + "}";
            log_string(out, j.c_str());
        } else {
            std::string line = "tcs3472 id=" + std::to_string(int(id)) +
                               " C=" + std::to_string(r.clear) +
                               " R=" + std::to_string(r.red) +
                               " G=" + std::to_string(r.green) +
                               " B=" + std::to_string(r.blue);
            log_string(out, line.c_str());
        }

        return Error::Ok;
    }

    if (string_util::equal_ignore_case(cmd, "READ")) {
        auto* sensor = find_sensor_by_id_mut(id);
        if (!sensor) {
            log_string(out, "tcs3472: sensor id not found");
            return Error::InvalidValue;
        }

        bool fresh = false;
        {
            std::string s;
            if (get_param(value, "fresh=", s) || get_param(value, "FRESH=", s)) {
                fresh = string_util::equal_ignore_case(s, "yes") || string_util::equal_ignore_case(s, "true") || s == "1";
            }
        }

        TCS3472::Reading r;
        if (!sensor->read(r, fresh)) {
            log_string(out, "tcs3472: read failed");
            return Error::SettingReadFail;
        }

        set_reading_params(prefix, r);

        if (want_json) {
            std::string j = "{\"id\":" + std::to_string(int(id)) +
                            ",\"c\":" + std::to_string(r.clear) +
                            ",\"r\":" + std::to_string(r.red) +
                            ",\"g\":" + std::to_string(r.green) +
                            ",\"b\":" + std::to_string(r.blue) + "}";
            log_string(out, j.c_str());
        } else {
            std::string line = "tcs3472 id=" + std::to_string(int(id)) +
                               " C=" + std::to_string(r.clear) +
                               " R=" + std::to_string(r.red) +
                               " G=" + std::to_string(r.green) +
                               " B=" + std::to_string(r.blue);
            log_string(out, line.c_str());
        }

        return Error::Ok;
    }

    log_string(out, "tcs3472: unknown command, use LIST/READ/LAST");
    return Error::InvalidValue;
}
