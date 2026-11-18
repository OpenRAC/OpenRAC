// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The Controls tab's Keyboard and mouse panel: the built-in keyboard and mouse's settings and keys
// (game/rac1/keyboard_mouse), changed in the game. Each change is saved to the settings file and
// used from the next tick (mods::set_built_in_setting). A key is changed by pressing the new one.

#include <algorithm>
#include <cmath>
#include <format>
#include <imgui.h>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "debug/menu_ui.h"
#include "debug/overlay.h"
#include "key_labels.h"
#include "mods.h"
#include "openrac/keys.h"

namespace openrac::frontend::debug_overlay {
namespace {

constexpr const char* kId = "keyboard-mouse";
// A key capture gives up after this long without a press, in seconds.
constexpr double kCaptureTimeout = 6.0;
// The camera's turn a count at sensitivity 1, in degrees (kbm.hpp kRadiansPerPixel), and
// Counter-Strike's at its sensitivity 1 (m_yaw).
constexpr double kDegreesPerCount = 0.0015 * 180.0 / 3.14159265358979;
constexpr double kCsDegreesPerCount = 0.022;

struct Panel {
    std::string open;     // the row showing its own rows
    std::string capture;  // the keybind waiting for a press
    bool add = false;     // the press adds a bind, else it replaces them
    double since = 0.0;
    std::string message;
};

Panel g;

std::string number(double v) {
    std::string t = std::format("{:.2f}", v);
    while (t.back() == '0') {
        t.pop_back();
    }
    if (t.back() == '.') {
        t.pop_back();
    }
    return t;
}

// How far one step of a number moves it: a hundredth of its range, rounded to 1, 2 or 5 times a
// power of ten.
double step_of(const mods::Setting& s) {
    const double range = s.max && s.min ? *s.max - *s.min : 10.0;
    double step = std::pow(10.0, std::floor(std::log10(std::max(range / 100.0, 1e-3))));
    for (const double m : {1.0, 2.0, 5.0}) {
        if (step * m >= range / 100.0) {
            step *= m;
            break;
        }
    }
    return s.type == mods::SettingType::Int ? std::max(1.0, std::round(step)) : step;
}

double value_of(const mods::SettingValue& v) {
    return v.type == mods::SettingType::Int ? static_cast<double>(v.i) : v.f;
}

std::string shown(const mods::Setting& s, const mods::SettingValue& v) {
    switch (s.type) {
        case mods::SettingType::Bool:
            return v.b ? "On" : "Off";
        case mods::SettingType::Int:
        case mods::SettingType::Float:
            return number(value_of(v));
        case mods::SettingType::Choice:
            for (std::size_t i = 0; i < s.choices.size(); ++i) {
                if (s.choices[i] == v.s && i < s.choice_names.size()) {
                    return s.choice_names[i];
                }
            }
            return v.s;
        default:
            return v.s;
    }
}

// The binds of a keybind's text, "KeyW,MouseLeft", as names.
std::vector<std::string> binds_of(const std::string& text) {
    std::vector<std::string> out;
    std::size_t from = 0;
    while (from <= text.size()) {
        const std::size_t comma = std::min(text.find(',', from), text.size());
        if (comma > from) {
            out.push_back(text.substr(from, comma - from));
        }
        from = comma + 1;
    }
    return out;
}

std::string joined(const std::vector<std::string>& binds) {
    std::string out;
    for (const std::string& b : binds) {
        out += (out.empty() ? "" : ",") + b;
    }
    return out;
}

// The binds as a player reads them: "Space / Left click".
std::string shown_binds(const std::vector<std::string>& binds) {
    std::string out;
    for (const std::string& b : binds) {
        out += (out.empty() ? "" : " / ") + keys::menu_label(b);
    }
    return out;
}

bool set(const std::string& key, const std::string& literal) {
    if (!mods::set_built_in_setting(kId, key, literal)) {
        g.message = "That value is refused";
        return false;
    }
    g.message.clear();
    return true;
}

void set_number(const mods::Setting& s, double v) {
    v = std::clamp(v, s.min.value_or(v), s.max.value_or(v));
    if (s.type == mods::SettingType::Int) {
        set(s.key, std::to_string(static_cast<long long>(std::llround(v))));
    } else {
        set(s.key, number(v));
    }
}

void set_binds(const std::string& key, const std::vector<std::string>& binds) {
    set(key, "\"" + joined(binds) + "\"");
}

// The sensitivity in other words: Counter-Strike's, and cm a full turn at the DPI set. Only where
// the mouse turns the camera by an angle (Ratchet & Clank, whose settings have the DPI).
std::string sensitivity_hint() {
    const mods::BuiltInSetting sens = mods::built_in_setting(kId, "mouse_sensitivity");
    const mods::BuiltInSetting dpi = mods::built_in_setting(kId, "mouse_dpi");
    if (sens.spec == nullptr || dpi.spec == nullptr) {
        return "";
    }
    const double degrees = kDegreesPerCount * sens.value.f;
    std::string hint = std::format("As CS sensitivity {:.2f}", degrees / kCsDegreesPerCount);
    if (dpi.value.i > 0 && degrees > 0.0) {
        hint += std::format(
            ", {:.1f} cm a full turn at {} DPI",
            360.0 / degrees / static_cast<double>(dpi.value.i) * 2.54,
            dpi.value.i
        );
    }
    return hint;
}

// Another play key with one of these binds (menu keys are only read in menus).
bool clashes(const mods::Setting& s, const std::vector<std::string>& binds) {
    if (s.key.starts_with("menu_")) {
        return false;
    }
    for (const mods::Setting* other : mods::built_in_settings(kId)) {
        if (other->key == s.key || other->type != mods::SettingType::Keybind
            || other->key.starts_with("menu_")) {
            continue;
        }
        for (const std::string& b : binds_of(mods::built_in_setting(kId, other->key).value.s)) {
            if (std::find(binds.begin(), binds.end(), b) != binds.end()) {
                return true;
            }
        }
    }
    return false;
}

bool row(const std::string& label, const std::string& value, ImU32 ink = 0) {
    const ImU32 shown_ink = ink != 0 ? ink : ui().gold;
    return menu_row(label.c_str(), ui().text, [&](ImDrawList* list, ImVec2 right) {
        const ImVec2 size = ImGui::CalcTextSize(value.c_str());
        list->AddText(ImVec2(right.x - size.x, right.y - size.y * 0.5f), shown_ink, value.c_str());
    });
}

bool sub(const std::string& label) {
    return menu_row(("      " + label).c_str(), ui().soft);
}

void setting_rows(const mods::Setting& s, const mods::SettingValue& v) {
    const bool open = g.open == s.key;
    ImGui::PushID(s.key.c_str());
    if (s.type == mods::SettingType::Keybind) {
        const std::vector<std::string> binds = binds_of(v.s);
        const bool waiting = g.capture == s.key;
        const std::string text = waiting         ? "press a key, button or the wheel (Esc: cancel)"
                                 : binds.empty() ? "nothing"
                                                 : shown_binds(binds);
        if (row(s.name, text, waiting ? ui().amber : clashes(s, binds) ? ui().amber : 0)) {
            g.open = open ? "" : s.key;
        }
        note_hover(static_cast<int>(menu::Id::KeyboardMouse));
        if (open && !waiting) {
            if (sub("Replace")) {
                g.capture = s.key;
                g.add = false;
                g.since = ImGui::GetTime();
            }
            if (binds.size() < static_cast<std::size_t>(s.binds) && sub("Add a key")) {
                g.capture = s.key;
                g.add = true;
                g.since = ImGui::GetTime();
            }
            for (const std::string& b : binds) {
                if (sub("Remove " + keys::menu_label(b))) {
                    std::vector<std::string> rest = binds;
                    std::erase(rest, b);
                    set_binds(s.key, rest);
                }
            }
            if (v.s != s.fallback.s
                && sub("Default (" + shown_binds(binds_of(s.fallback.s)) + ")")) {
                set(s.key, "\"" + s.fallback.s + "\"");
            }
        }
        ImGui::PopID();
        return;
    }
    if (row(s.name, shown(s, v))) {
        switch (s.type) {
            case mods::SettingType::Bool:
                set(s.key, v.b ? "false" : "true");
                break;
            case mods::SettingType::Choice: {
                const auto it = std::find(s.choices.begin(), s.choices.end(), v.s);
                const std::size_t next =
                    it == s.choices.end() ? 0 : (it - s.choices.begin() + 1) % s.choices.size();
                if (!s.choices.empty()) {
                    set(s.key, "\"" + s.choices[next] + "\"");
                }
                break;
            }
            default:
                g.open = open ? "" : s.key;
                break;
        }
    }
    note_hover(static_cast<int>(menu::Id::KeyboardMouse));
    if (s.key == "mouse_sensitivity") {
        menu_note(sensitivity_hint().c_str(), ui().dim);
    }
    if (open && (s.type == mods::SettingType::Int || s.type == mods::SettingType::Float)) {
        const double now = value_of(v), step = step_of(s);
        if (sub("Lower (-" + number(step) + ")")) {
            set_number(s, now - step);
        }
        if (sub("Raise (+" + number(step) + ")")) {
            set_number(s, now + step);
        }
        if (value_of(s.fallback) != now && sub("Default (" + number(value_of(s.fallback)) + ")")) {
            set_number(s, value_of(s.fallback));
        }
    }
    ImGui::PopID();
}

// The bind an SDL event presses, or none.
int bind_of(const SDL_Event& e) {
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
            return e.key.repeat ? OPENRAC_BIND_NONE : static_cast<int>(e.key.scancode);
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch (e.button.button) {
                case SDL_BUTTON_LEFT:
                    return OPENRAC_BIND_MOUSE_LEFT;
                case SDL_BUTTON_MIDDLE:
                    return OPENRAC_BIND_MOUSE_MIDDLE;
                case SDL_BUTTON_RIGHT:
                    return OPENRAC_BIND_MOUSE_RIGHT;
                case SDL_BUTTON_X1:
                    return OPENRAC_BIND_MOUSE_BACK;
                case SDL_BUTTON_X2:
                    return OPENRAC_BIND_MOUSE_FORWARD;
                default:
                    return OPENRAC_BIND_NONE;
            }
        case SDL_EVENT_MOUSE_WHEEL:
            return e.wheel.y > 0.0f   ? OPENRAC_BIND_WHEEL_UP
                   : e.wheel.y < 0.0f ? OPENRAC_BIND_WHEEL_DOWN
                   : e.wheel.x > 0.0f ? OPENRAC_BIND_WHEEL_RIGHT
                   : e.wheel.x < 0.0f ? OPENRAC_BIND_WHEEL_LEFT
                                      : OPENRAC_BIND_NONE;
        default:
            return OPENRAC_BIND_NONE;
    }
}

}  // namespace

bool keyboard_mouse_shown() {
    return !mods::built_in_settings(kId).empty();
}

void keyboard_mouse_panel() {
    if (!g.message.empty()) {
        menu_note(g.message.c_str(), ui().err);
    } else {
        menu_note("Changes are saved and used at once.", ui().soft);
    }
    if (!g.capture.empty() && ImGui::GetTime() - g.since > kCaptureTimeout) {
        g.capture.clear();
        g.message = "No press came: nothing was changed";
    }
    if (!s.menu_open) {
        g.capture.clear();
    }
    for (const mods::Setting* setting : mods::built_in_settings(kId)) {
        setting_rows(*setting, mods::built_in_setting(kId, setting->key).value);
    }
    if (sub("Every keyboard and mouse setting to the defaults")) {
        for (const mods::Setting* p : mods::built_in_settings(kId)) {
            const mods::Setting& setting = *p;
            const mods::SettingValue& d = setting.fallback;
            switch (setting.type) {
                case mods::SettingType::Bool:
                    set(setting.key, d.b ? "true" : "false");
                    break;
                case mods::SettingType::Int:
                case mods::SettingType::Float:
                    set_number(setting, value_of(d));
                    break;
                default:
                    set(setting.key, "\"" + d.s + "\"");
                    break;
            }
        }
    }
    note_hover(static_cast<int>(menu::Id::KeyboardMouse));
}

bool keyboard_mouse_capturing() {
    return !g.capture.empty();
}

bool keyboard_mouse_capture_event(const SDL_Event& e) {
    if (g.capture.empty()) {
        return false;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE) {
        g.capture.clear();
        return true;
    }
    const int bind = bind_of(e);
    const char* name = bind != OPENRAC_BIND_NONE ? openrac_bind_to_name(bind) : nullptr;
    if (name == nullptr) {
        // Another event (the release of the press that started it, a motion): not the answer, and
        // a key with no name (a media key) is not one either.
        return e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN
               || e.type == SDL_EVENT_MOUSE_WHEEL;
    }
    const std::string key = g.capture;
    g.capture.clear();
    std::vector<std::string> binds =
        g.add ? binds_of(mods::built_in_setting(kId, key).value.s) : std::vector<std::string>{};
    if (std::find(binds.begin(), binds.end(), name) == binds.end()) {
        binds.emplace_back(name);
    }
    set_binds(key, binds);
    return true;
}

}  // namespace openrac::frontend::debug_overlay
