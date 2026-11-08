// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window's Metal renderer (platform/window.h's MetalBackend): mtlgl, checked whole on open.

#include <string>

#include "platform/window.h"
#include "renderer/gl.h"
#include "renderer/metal/mtlgl.h"

namespace openrac::platform {
namespace {

bool open(SDL_Window* window, int width, int height, bool vsync, std::string& error) {
    if (!mtlgl::open({window, width, height, vsync}, error)) {
        return false;
    }
    // The caller's gl::load fails on one missing function, after the window is open and cannot
    // fall back; a Metal that is not whole yet is found here.
    std::string missing;
#define OPENRAC_METAL_CHECK(ret, name, params)                                                     \
    if (mtlgl::get_proc(#name) == nullptr) {                                                       \
        missing += missing.empty() ? #name : " " #name;                                            \
    }
    OPENRAC_GL_FUNCTIONS(OPENRAC_METAL_CHECK)
#undef OPENRAC_METAL_CHECK
    if (!missing.empty()) {
        error = "the Metal renderer lacks GL functions: " + missing;
        mtlgl::close();
        return false;
    }
    return true;
}

}  // namespace

const MetalBackend& metal_backend() {
    static const MetalBackend backend{
        open,
        mtlgl::close,
        mtlgl::active,
        mtlgl::get_proc,
        mtlgl::present,
        mtlgl::set_swap_interval,
        mtlgl::device_name,
    };
    return backend;
}

}  // namespace openrac::platform
