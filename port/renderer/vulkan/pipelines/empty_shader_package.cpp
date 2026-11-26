// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Empty built-in package for builds with OPENRAC_SHADER_PACKAGE disabled.
#include <cstddef>

extern "C" const char* openrac_shader_package(std::size_t* size) {
    *size = 0;
    return nullptr;
}
