// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "settings": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_setting_bool(openrac_mod* mod, const char* key) {
    const SettingValue* v = setting(mod, key, SettingType::Bool, SettingType::Bool);
    return v != nullptr && v->b ? 1 : 0;
}

int64_t api_setting_int(openrac_mod* mod, const char* key) {
    const SettingValue* v = setting(mod, key, SettingType::Int, SettingType::Int);
    return v != nullptr ? v->i : 0;
}

double api_setting_float(openrac_mod* mod, const char* key) {
    const SettingValue* v = setting(mod, key, SettingType::Float, SettingType::Int);
    if (v == nullptr) {
        return 0.0;
    }
    return v->type == SettingType::Int ? static_cast<double>(v->i) : v->f;
}

const char* api_setting_string(openrac_mod* mod, const char* key) {
    const SettingValue* v = setting(mod, key, SettingType::String, SettingType::Choice);
    if (v == nullptr) {
        v = setting(mod, key, SettingType::Keybind, SettingType::Keybind);
    }
    return v != nullptr ? v->s.c_str() : "";
}

int api_setting_set_bool(openrac_mod* mod, const char* key, int value) {
    return set_setting(
        mod, key, SettingType::Bool, SettingType::Bool, value != 0 ? "true" : "false"
    );
}

int api_setting_set_int(openrac_mod* mod, const char* key, int64_t value) {
    return set_setting(mod, key, SettingType::Int, SettingType::Int, std::format("{}", value));
}

int api_setting_set_float(openrac_mod* mod, const char* key, double value) {
    if (!std::isfinite(value)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return set_setting(mod, key, SettingType::Float, SettingType::Float, number_text(value));
}

int api_setting_set_string(openrac_mod* mod, const char* key, const char* value) {
    if (value == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // A choice, a string or a keybind: whichever the manifest declares.
    if (setting(mod, key, SettingType::Keybind, SettingType::Keybind) != nullptr) {
        return set_setting(mod, key, SettingType::Keybind, SettingType::Keybind, quoted(value));
    }
    return set_setting(mod, key, SettingType::String, SettingType::Choice, quoted(value));
}

int api_on_settings(openrac_mod* mod, openrac_frame_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    g_settings_callbacks.push_back({fn, user, mod});
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
