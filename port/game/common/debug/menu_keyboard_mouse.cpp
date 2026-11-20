// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The Controls tab's Keyboard and mouse panel: the built-in keyboard and mouse's settings and keys
// (game/rac1/keyboard_mouse), changed in the game. Each change is saved to the settings file and
// used from the next tick (mods::set_built_in_setting). A key is changed by pressing the new one.

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstring>
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

struct Panel {
    std::string open;     // the row showing its own rows
    std::string capture;  // the keybind waiting for a press
    bool add = false;     // the press adds a bind, else it replaces them
    double since = 0.0;
    std::string message;
    bool reset_asked = false;  // Reset to defaults pressed once
    double reset_since = 0.0;
    std::string typing;            // the number whose value is being typed
    std::array<char, 16> typed{};  // what is typed so far
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

// How far one step of a number moves it: the manifest's step (as the launcher's slider), else a
// hundredth of its range, rounded to 1, 2 or 5 times a power of ten.
double step_of(const mods::Setting& s) {
    if (s.step) {
        return s.type == mods::SettingType::Int ? std::max(1.0, std::round(*s.step)) : *s.step;
    }
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

// Another key of the same group with one of these binds, as the launcher marks it: a play key and
// a menu key may share one (menu keys are only read in menus).
bool clashes(const mods::Setting& s, const std::vector<std::string>& binds) {
    for (const mods::Setting* other : mods::built_in_settings(kId)) {
        if (other->key == s.key || other->type != mods::SettingType::Keybind
            || other->group != s.group) {
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

bool value_row(const std::string& label, const std::string& value, ImU32 ink = 0) {
    const ImU32 shown_ink = ink != 0 ? ink : ui().gold;
    return menu_row(label.c_str(), ui().text, [&](ImDrawList* list, ImVec2 right) {
        const ImVec2 size = ImGui::CalcTextSize(value.c_str());
        list->AddText(ImVec2(right.x - size.x, right.y - size.y * 0.5f), shown_ink, value.c_str());
    });
}

bool sub(const std::string& label) {
    return menu_row(("      " + label).c_str(), ui().soft);
}

// The row just drawn is the setting's: its own help in the help pane.
void hover(const mods::Setting& s) {
    note_hover_row(static_cast<int>(menu::Id::KeyboardMouse), s.section, s.name, s.description);
}

// The decimals a number is shown with: its step's (0.01: two).
int decimals_of(double step) {
    int d = 0;
    while (d < 4
           && std::fabs(step * std::pow(10.0, d) - std::round(step * std::pow(10.0, d))) > 1e-6) {
        ++d;
    }
    return d;
}

// A number with both ends: a slider on its steps (the arrows a step, Shift ten), its label opening
// a row to type the value exactly and one back to the default.
void number_rows(const mods::Setting& s, const mods::SettingValue& v, bool open) {
    const double step = step_of(s), low = *s.min, now = value_of(v);
    const int count = static_cast<int>(std::lround((*s.max - low) / step));
    const int index = static_cast<int>(std::lround((now - low) / step));
    const int places = decimals_of(step);
    bool label = false;
    const int next = menu_number_slider(
        s.name.c_str(), index, count, std::format("{:.{}f}", now, places), &label
    );
    hover(s);
    if (next != index) {
        set_number(s, low + next * step);
    }
    if (label) {
        g.open = open ? "" : s.key;
        g.typing.clear();
    }
    if (!open) {
        return;
    }
    if (g.typing == s.key) {
        // The value typed: Enter keeps it (within the ends, on a step), Escape or leaving drops it.
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive()) {
            ImGui::SetKeyboardFocusHere();
        }
        const bool done = ImGui::InputText(
            "##typed",
            g.typed.data(),
            g.typed.size(),
            ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue
        );
        if (done) {
            double typed = 0.0;
            const char* end = g.typed.data() + std::strlen(g.typed.data());
            if (std::from_chars(g.typed.data(), end, typed).ec == std::errc{}
                && std::isfinite(typed)) {
                set_number(s, low + std::round((typed - low) / step) * step);
            }
            g.typing.clear();
        } else if (ImGui::IsItemDeactivated() || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            g.typing.clear();
        }
    } else if (sub("Type a value")) {
        g.typing = s.key;
        const std::string text = std::format("{:.{}f}", now, places);
        g.typed.fill('\0');
        std::copy_n(text.begin(), std::min(text.size(), g.typed.size() - 1), g.typed.begin());
    }
    if (value_of(s.fallback) != now
        && sub("Default (" + std::format("{:.{}f}", value_of(s.fallback), places) + ")")) {
        set_number(s, value_of(s.fallback));
    }
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
        if (value_row(s.name, text, waiting ? ui().amber : clashes(s, binds) ? ui().amber : 0)) {
            g.open = open ? "" : s.key;
        }
        hover(s);
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
    if ((s.type == mods::SettingType::Int || s.type == mods::SettingType::Float) && s.min
        && s.max) {
        number_rows(s, v, open);
        ImGui::PopID();
        return;
    }
    if (value_row(s.name, shown(s, v))) {
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
    hover(s);
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

// Whether the settings are as a preset (null: the defaults) sets them.
bool is_preset(
    const std::vector<const mods::Setting*>& all,
    const std::vector<mods::Preset>& presets,
    const mods::Preset* preset
) {
    return std::all_of(all.begin(), all.end(), [&](const mods::Setting* s) {
        return !mods::preset_covers(*s, presets)
               || mods::same_value(
                   *s, mods::preset_value(*s, preset), mods::built_in_setting(kId, s->key).value
               );
    });
}

void apply_preset(
    const std::vector<const mods::Setting*>& all,
    const std::vector<mods::Preset>& presets,
    const mods::Preset* preset
) {
    for (const mods::Setting* s : all) {
        if (mods::preset_covers(*s, presets)) {
            set(s->key, mods::setting_literal(*s, mods::preset_value(*s, preset)));
        }
    }
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
    if (!g.capture.empty() && ImGui::GetTime() - g.since > kCaptureTimeout) {
        g.capture.clear();
        g.message = "No press came: nothing was changed";
    }
    if (!s.menu_open) {
        g.capture.clear();
    }
    // At the top, as the launcher's mod page: the line on saving, the presets and the reset.
    if (!g.message.empty()) {
        menu_note(g.message.c_str(), ui().err);
    } else {
        menu_note("Changes save as you make them, and count at once.", ui().soft);
    }
    const std::vector<const mods::Setting*> all = mods::built_in_settings(kId);
    const std::vector<mods::Preset> presets = mods::built_in_presets(kId);
    if (!presets.empty()) {
        // The choices: Default (unless a preset is "default"), then the presets; null is Default.
        std::vector<const mods::Preset*> choices;
        if (std::none_of(presets.begin(), presets.end(), [](const mods::Preset& p) {
                return p.id == "default";
            })) {
            choices.push_back(nullptr);
        }
        for (const mods::Preset& p : presets) {
            choices.push_back(&p);
        }
        std::size_t lit = choices.size();
        for (std::size_t i = 0; i < choices.size() && lit == choices.size(); ++i) {
            if (is_preset(all, presets, choices[i])) {
                lit = i;
            }
        }
        const std::string name = lit == choices.size()     ? "Custom"
                                 : choices[lit] == nullptr ? "Default"
                                                           : choices[lit]->name;
        if (value_row("Presets", name)) {
            apply_preset(
                all, presets, choices[lit == choices.size() ? 0 : (lit + 1) % choices.size()]
            );
        }
        note_hover_row(
            static_cast<int>(menu::Id::KeyboardMouse),
            "",
            "Presets",
            "Sets the keys (and the settings a preset names) at once. Custom: your own."
        );
    }
    // Asked once, done on the second press (the launcher asks too).
    if (g.reset_asked && ImGui::GetTime() - g.reset_since > kCaptureTimeout) {
        g.reset_asked = false;
    }
    if (value_row("Reset to defaults", g.reset_asked ? "Press again to reset" : "", ui().amber)) {
        if (g.reset_asked) {
            for (const mods::Setting* p : all) {
                set(p->key, mods::setting_literal(*p, p->fallback));
            }
        }
        g.reset_asked = !g.reset_asked;
        g.reset_since = ImGui::GetTime();
    }
    note_hover_row(
        static_cast<int>(menu::Id::KeyboardMouse),
        "",
        "Reset to defaults",
        "Every keyboard and mouse setting back to its default. Press it twice."
    );
    // The settings under their sections, the sections in the order each first appears (as the
    // launcher shows them, lib/plugins.ts settingGroups).
    std::vector<std::string> sections;
    for (const mods::Setting* p : all) {
        if (std::find(sections.begin(), sections.end(), p->section) == sections.end()) {
            sections.push_back(p->section);
        }
    }
    for (const std::string& section : sections) {
        if (!section.empty()) {
            card_end();
            menu_heading(section.c_str());
            card_begin();
        }
        for (const mods::Setting* p : all) {
            if (p->section == section) {
                setting_rows(*p, mods::built_in_setting(kId, p->key).value);
            }
        }
    }
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
