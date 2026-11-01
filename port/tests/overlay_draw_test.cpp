// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game, on the GPU (renderer/ui/overlay_draw.h), on each GL (the _es and
// _vulkan tests run this one): into the frame's own kind of target, at its place, right way up,
// blended by alpha, and nothing without vertices. Pixels are read back: a colour in the wrong row
// or a blend left out fails here on the backend that has it.

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include "check.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/ui/overlay_draw.h"
#include "renderer/ui/overlay_list.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac;
using namespace openrac::gl;

constexpr int kWidth = 512;
constexpr int kHeight = 448;

// The pixel at (x, y) of the picture, y down.
std::array<std::uint8_t, 4> pixel(unsigned framebuffer, int x, int y) {
    std::array<std::uint8_t, 4> out{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glReadPixels(x, kHeight - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return out;
}

void clear_to(unsigned framebuffer, float r, float g, float b) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, kWidth, kHeight);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool near(int a, int b) {
    return a >= b - 2 && a <= b + 2;
}

}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why, kWidth, kHeight)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return test::kSkip;
    }
    renderer::FrameBuffer frame;
    CHECK(frame.create(kWidth, kHeight, why));
    renderer::ui::OverlayDraw draw;
    renderer::ui::OverlayList list;

    // No vertices: the frame as it was.
    clear_to(frame.id(), 0.0f, 0.0f, 0.5f);
    list.begin(1.0f);
    draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    CHECK(near(pixel(frame.id(), 100, 100)[2], 128));

    // A red rectangle in the top left: there, and not at the same place from the bottom.
    list.rect(100, 40, 50, 20, 0xFF0000FFu);
    // A half white one lower down, over the blue.
    list.rect(300, 300, 50, 50, 0xFFFFFF80u);
    draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    const auto red = pixel(frame.id(), 120, 50);
    CHECK(red[0] == 255 && red[1] == 0 && red[2] == 0);
    CHECK(pixel(frame.id(), 120, kHeight - 50)[0] == 0);  // the picture is the right way up
    CHECK(pixel(frame.id(), 99, 50)[0] == 0);
    CHECK(pixel(frame.id(), 151, 50)[0] == 0);
    const auto half = pixel(frame.id(), 320, 320);
    CHECK(near(half[0], 128) && near(half[1], 128) && near(half[2], 192));
    CHECK(half[3] == 255);  // the frame's own alpha stays, under a see-through colour too

    // A frame larger than the picture: it stretches, as the game's own picture does.
    renderer::FrameBuffer large;
    CHECK(large.create(kWidth * 2, kHeight * 2, why));
    glBindFramebuffer(GL_FRAMEBUFFER, large.id());
    glViewport(0, 0, kWidth * 2, kHeight * 2);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    list.begin(1.0f);
    list.rect(100, 40, 50, 20, 0x00FF00FFu);
    draw.draw(large.id(), kWidth * 2, kHeight * 2, list.vertices());
    std::array<std::uint8_t, 4> green{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, large.id());
    glReadPixels(240, kHeight * 2 - 1 - 100, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, green.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    CHECK(green[1] == 255);

    // Over and over, as frames do.
    for (int i = 0; i < 8; ++i) {
        draw.draw(frame.id(), kWidth, kHeight, list.vertices());
    }
    CHECK(glGetError() == GL_NO_ERROR);

    draw.release();
    frame.release();
    large.release();
    return test::result();
}
