// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "touch": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_touch_define(openrac_mod* mod, const char* json) {
    std::string error;
    if (mod == nullptr || json == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!touch::define(mod_id(mod), json, error)) {
        log::warn("mods: {}", error);
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

int api_touch_icon(openrac_mod* mod, const char* name, const char* svg) {
    std::string error;
    if (mod == nullptr || name == nullptr || svg == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!touch::icon(mod_id(mod), name, svg, error)) {
        log::warn("mods: {}", error);
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

void api_touch_push(openrac_mod* mod, const char* context) {
    if (mod != nullptr) {
        touch::push(mod_id(mod), context != nullptr ? context : "");
    }
}

void api_touch_hint(openrac_mod* mod, const char* hint, int on) {
    if (mod != nullptr && hint != nullptr) {
        touch::hint(mod_id(mod), hint, on != 0);
    }
}

}  // namespace openrac::mods::detail
