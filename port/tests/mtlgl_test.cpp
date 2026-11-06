// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// renderer/metal/mtlgl.h without a window: the context says what it is, clears the default
// framebuffer and reads it back with GL's pack alignment, and refuses the clears it cannot do yet.
// Skipped (exit 77) where there is no Metal device.

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "check.h"
#include "renderer/gl.h"
#include "renderer/metal/mtlgl.h"

namespace {

using namespace openrac::gl;

template <typename Fn>
Fn proc(const char* name) {
    const auto p = openrac::mtlgl::get_proc(name);
    CHECK(p != nullptr);
    return reinterpret_cast<Fn>(p);
}

}  // namespace

int main() {
    std::string error;
    if (!openrac::mtlgl::open({.width = 5, .height = 3}, error)) {
        std::printf("%s\n", error.c_str());
        return error == "no Metal device" ? 77 : 1;
    }
    const auto get_string = proc<PFN_glGetString>("glGetString");
    const auto get_error = proc<PFN_glGetError>("glGetError");
    const auto clear_colour = proc<PFN_glClearColor>("glClearColor");
    const auto clear = proc<PFN_glClear>("glClear");
    const auto enable = proc<PFN_glEnable>("glEnable");
    const auto disable = proc<PFN_glDisable>("glDisable");
    const auto pixel_store = proc<PFN_glPixelStorei>("glPixelStorei");
    const auto read_pixels = proc<PFN_glReadPixels>("glReadPixels");
    CHECK(openrac::mtlgl::get_proc("glNotAFunction") == nullptr);

    const std::string_view version = reinterpret_cast<const char*>(get_string(GL_VERSION));
    CHECK(version.starts_with("4.1 OpenRAC on Metal"));

    clear_colour(1.0f, 0.5f, 0.0f, 1.0f);
    clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK(get_error() == GL_NO_ERROR);

    // Rows of 5 pixels padded to 8 bytes' multiples: 20 bytes become 24.
    pixel_store(GL_PACK_ALIGNMENT, 8);
    std::vector<std::uint8_t> pixels(24 * 3, 0xAA);
    read_pixels(0, 0, 5, 3, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (int row = 0; row < 3; ++row) {
        for (int x = 0; x < 5; ++x) {
            const std::uint8_t* p = &pixels[static_cast<std::size_t>(row * 24 + x * 4)];
            CHECK(p[0] == 255 && p[1] >= 127 && p[1] <= 128 && p[2] == 0 && p[3] == 255);
        }
        CHECK(pixels[static_cast<std::size_t>(row * 24 + 20)] == 0xAA);
    }

    // Partly outside: only the pixel inside is written.
    std::uint8_t corner[8] = {};
    pixel_store(GL_PACK_ALIGNMENT, 4);
    read_pixels(4, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, corner);
    CHECK(corner[0] == 255 && corner[4] == 0);

    enable(GL_SCISSOR_TEST);
    clear(GL_COLOR_BUFFER_BIT);
    CHECK(get_error() == 0x0502);  // GL_INVALID_OPERATION
    disable(GL_SCISSOR_TEST);

    read_pixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, pixels.data());
    CHECK(get_error() == 0x0500);  // GL_INVALID_ENUM

    openrac::mtlgl::present();
    openrac::mtlgl::close();
    CHECK(!openrac::mtlgl::active());
    return openrac::test::result();
}
