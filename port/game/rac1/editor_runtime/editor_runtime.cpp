// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// The built-in editor runtime's entry (editor_runtime.h): it loads through the mod interface, as a
// mod with its own manifest, and starts the cutscene player and the behaviours.

#include "editor_runtime.h"

#include "openrac/mod.hpp"

namespace editor_runtime {
void cutscenes_load(openrac::Mod& mod);
void behaviours_load(openrac::Mod& mod);
}  // namespace editor_runtime

namespace {

void start(openrac::Mod& mod) {
    editor_runtime::cutscenes_load(mod);
    editor_runtime::behaviours_load(mod);
}

}  // namespace

namespace openrac::editor_runtime {

void query(openrac_mod_info* info) {
    info->api_major = OPENRAC_MOD_API_MAJOR;
    info->api_minor = OPENRAC_MOD_API_MINOR;
    info->id = "editor-runtime";
    info->version = "1.0.0";
}

int load(const openrac_api* api, openrac_mod* mod) {
    return openrac::detail::load(api, mod, start);
}

}  // namespace openrac::editor_runtime
