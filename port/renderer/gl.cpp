// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/gl.h"

namespace openrac::gl {

#define OPENRAC_GL_DEFINE(ret, name, params) PFN_##name name = nullptr;
OPENRAC_GL_FUNCTIONS(OPENRAC_GL_DEFINE)
#undef OPENRAC_GL_DEFINE

namespace {
bool g_loaded = false;
}  // namespace

bool load(GetProcAddress get, std::string& missing) {
    missing.clear();
    // A function pointer of type void (*)() converts to any other function
    // pointer type and back; that is what every GL loader relies on.
#define OPENRAC_GL_LOAD(ret, name, params)                                                         \
    name = reinterpret_cast<PFN_##name>(get(#name));                                               \
    if (name == nullptr) {                                                                         \
        missing += missing.empty() ? #name : " " #name;                                            \
    }
    OPENRAC_GL_FUNCTIONS(OPENRAC_GL_LOAD)
#undef OPENRAC_GL_LOAD
    g_loaded = missing.empty();
    return g_loaded;
}

bool loaded() {
    return g_loaded;
}

}  // namespace openrac::gl
