// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/world/gl_world.h"

namespace openrac::renderer::world::gl {

#define OPENRAC_WORLD_GL_DEFINE(ret, name, params) PFN_##name name = nullptr;
OPENRAC_WORLD_GL_FUNCTIONS(OPENRAC_WORLD_GL_DEFINE)
#undef OPENRAC_WORLD_GL_DEFINE

namespace {
bool g_loaded = false;
}  // namespace

bool load(GetProcAddress get, std::string& missing) {
    missing.clear();
    // As renderer/gl.cpp: a void (*)() converts to any other function pointer.
#define OPENRAC_WORLD_GL_LOAD(ret, name, params)                                                   \
    name = reinterpret_cast<PFN_##name>(get(#name));                                               \
    if (name == nullptr) {                                                                         \
        missing += missing.empty() ? #name : " " #name;                                            \
    }
    OPENRAC_WORLD_GL_FUNCTIONS(OPENRAC_WORLD_GL_LOAD)
#undef OPENRAC_WORLD_GL_LOAD
    g_loaded = missing.empty() && openrac::gl::loaded();
    return g_loaded;
}

bool loaded() {
    return g_loaded;
}

}  // namespace openrac::renderer::world::gl
