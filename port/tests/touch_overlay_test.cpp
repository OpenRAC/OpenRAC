// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The on-screen pad drawn (renderer/ui/touch_overlay.h) on a real context, desktop GL or ES
// (touch_overlay_es): its shader builds, nothing is drawn before the screen is touched, and a held
// button is drawn brighter than one let go. Skipped (exit 77) where no context can be made.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/ui/touch_icons.h"
#include "renderer/ui/touch_overlay.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac;
using namespace openrac::gl;

constexpr int kWidth = 320;
constexpr int kHeight = 180;

// The window's pixel under window units (x, y), as a sum of its red, green and blue.
int brightness(float x, float y) {
    unsigned char rgba[4] = {};
    glReadPixels(static_cast<GLint>(x * kWidth), static_cast<GLint>((1.0f - y) * kHeight), 1, 1, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba);
    return rgba[0] + rgba[1] + rgba[2];
}

void clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// Recreates the game target as single-sample with an unwritten depth texture still bound.
void test_draw_after_sample_change(platform::TouchPad& touch, float x, float y, int expected) {
    renderer::FrameBuffer frame;
    std::string error;
    constexpr int samples = 4;
    glActiveTexture(GL_TEXTURE0);
    const bool created = frame.create(kWidth, kHeight, error, samples);
    CHECK(created);

    // Present the previous target before replacing it, as when leaving the title screen.
    if (!created) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    frame.blit_to(0, kWidth, kHeight);

    const bool replaced = frame.create(kWidth, kHeight, error, 1, true);
    CHECK(replaced);

    // A transition frame clears colour and leaves its unused depth contents undefined.
    if (!replaced) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glClear(GL_COLOR_BUFFER_BIT);
    clear();
    frame.blit_to(0, kWidth, kHeight);
    renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
    CHECK(brightness(x, y) == expected);

    // An enclosing Vulkan collector must record the overlay while its draw inputs are alive.
    clear();
    const auto recorded = renderer::run_surface_chain([&] {
        renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
    });
    CHECK(recorded);
    CHECK(brightness(x, y) == expected);
}

// Reads the brightest neutral ink inside the menu button, away from its rim.
int logo_brightness(const platform::TouchControl& menu) {
    // The logo spans the centre of the button. The slate and highlights here stay below half white.
    constexpr float sample_radius = 0.6f;
    const int half = static_cast<int>(menu.radius * kHeight * sample_radius);
    const int side = half * 2 + 1;
    const int x = static_cast<int>(menu.x * kWidth) - half;
    const int y = static_cast<int>((1.0f - menu.y) * kHeight) - half;
    constexpr std::size_t rgba_channels = 4;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(side * side) * rgba_channels);
    glReadPixels(x, y, side, side, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    int brightest = 0;

    // Taking the weakest colour channel keeps coloured controls from counting as white ink.
    for (std::size_t i = 0; i < pixels.size(); i += rgba_channels) {
        brightest = std::max<int>(brightest, std::min({pixels[i], pixels[i + 1], pixels[i + 2]}));
    }

    return brightest;
}

}  // namespace

int main() {
    test::GlContext context;
    std::string why;
    if (!context.open(why, kWidth, kHeight)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return test::kSkip;
    }
    platform::TouchPad touch;
    const platform::TouchControl* cross = nullptr;
    for (const platform::TouchControl& c : touch.controls()) {
        if (c.kind == platform::TouchControl::Kind::Button && c.button == platform::Button::Cross) {
            cross = &c;
        }
    }
    CHECK(cross != nullptr);
    if (cross == nullptr) {
        return test::result();
    }
    // The ring's edge, where every button draws whatever its symbol.
    const float edge_x = cross->x + cross->radius * 0.92f * kHeight / kWidth;

    clear();
    renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
    CHECK(brightness(edge_x, cross->y) == 0);  // not touched yet: nothing drawn

    touch.finger_down(1, 0.5f, 0.5f);  // touched away from every control: shown, nothing held
    clear();
    renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
    const int released = brightness(edge_x, cross->y);
    CHECK(released > 0);

    touch.finger_down(2, cross->x, cross->y);
    clear();
    renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
    const int held = brightness(edge_x, cross->y);
    CHECK(held > released);
    CHECK(glGetError() == GL_NO_ERROR);

    // Frame after frame, as the game draws it: the picture blitted into the window, the pad over
    // it, shown. The pad looks the same in each (on Vulkan it flickered on a phone).
    GLuint picture = 0;
    GLuint framebuffer = 0;
    glGenTextures(1, &picture);
    glBindTexture(GL_TEXTURE_2D, picture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kWidth, kHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, picture, 0);
    for (int frame = 0; frame < 4; ++frame) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        clear();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBlitFramebuffer(0, 0, kWidth, kHeight, 0, 0, kWidth, kHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        renderer::ui::touch_overlay::draw(touch, kWidth, kHeight);
        CHECK(brightness(edge_x, cross->y) == held);
        context.window().swap();
    }
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &picture);
    CHECK(glGetError() == GL_NO_ERROR);

    test_draw_after_sample_change(touch, edge_x, cross->y, held);

    // An action button draws its icon: the SVG's #ff8000 in the accent (here pure red).
    CHECK(
        renderer::ui::touch_icons::add(
            "test",
            R"(<svg viewBox="-50 -50 100 100"><rect x="-50" y="-50" width="100" height="100" fill="#ff8000"/></svg>)"
        )
    );
    renderer::ui::touch_overlay::set_accent(1.0f, 0.0f, 0.0f);
    platform::TouchAction action;
    action.id = "go";
    action.icon = "test";
    action.press = static_cast<std::uint16_t>(platform::Button::Cross);
    action.size = platform::TouchSize::Large;
    platform::TouchPad arc;
    arc.set_style(platform::TouchStyle::Arc);
    arc.set_actions({action}, {});
    const platform::TouchControl* go = nullptr;
    const platform::TouchControl* menu = nullptr;
    for (const platform::TouchControl& c : arc.controls()) {
        if (c.action == "go") {
            go = &c;
        }
        if (c.kind == platform::TouchControl::Kind::Menu) {
            menu = &c;
        }
    }
    CHECK(go != nullptr);
    CHECK(menu != nullptr);
    // Draw the logo and the action icon in the same pass.
    if (go != nullptr && menu != nullptr) {
        arc.finger_down(1, 0.5f, 0.5f);
        const float x = go->x * kWidth;
        const float y = (1.0f - go->y) * kHeight;
        arc.set_menu_button(true);
        clear();
        renderer::ui::touch_overlay::draw(arc, kWidth, kHeight);
        // On black, the menu's white logo reaches at least half intensity without a press.
        constexpr int half_white = 128;
        CHECK(logo_brightness(*menu) >= half_white);
        unsigned char rgba[4] = {};
        glReadPixels(
            static_cast<GLint>(x), static_cast<GLint>(y), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba
        );
        CHECK(rgba[0] > 60 && rgba[0] > rgba[1] * 2 && rgba[0] > rgba[2] * 2);
        CHECK(glGetError() == GL_NO_ERROR);
    }

    renderer::ui::touch_overlay::release();
    return test::result();
}
