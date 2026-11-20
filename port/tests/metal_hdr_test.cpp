// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output on Metal (renderer/metal/mtlgl_hdr.mm). The layer's set-up for each output; the
// encode pass, run without a window into a stand-in of the drawable (mtlgl::test_hdr_encode),
// against common/hdr.h's arithmetic and against the Vulkan backend's pass on the same picture
// (where Vulkan opens); then a hidden window's real CAMetalLayer turned to EDR and back, with a
// present each way under Metal's API validation.
//
// Skipped (77) where there is no Metal device.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "check.h"
#include "common/hdr.h"
#include "platform/window.h"
#include "renderer/gl/gl.h"
#include "renderer/metal/mtlgl.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/renderer_gl_context.h"

namespace {

using namespace openrac::gl;
namespace hdr = openrac::hdr;
namespace mtlgl = openrac::mtlgl;

// Four half floats a pixel, as both backends' scRGB stand-ins store them.
constexpr std::size_t kTexel = 8;

// Paints the default framebuffer as vulkan_hdr_test does: four quarters of different colours and
// a ramp along the bottom rows, so that both the orientation and the curve show.
void paint(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glEnable(GL_SCISSOR_TEST);
    constexpr float kQuarters[4][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
    };

    for (int n = 0; n < 4; ++n) {
        glScissor((n % 2) * width / 2, (n / 2) * height / 2, width / 2, height / 2);
        glClearColor(kQuarters[n][0], kQuarters[n][1], kQuarters[n][2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // The ramp's rows (4) along the bottom.
    for (int x = 0; x < width; ++x) {
        const float v = static_cast<float>(x) / static_cast<float>(width - 1);
        glScissor(x, 0, 1, 4);
        glClearColor(v, v * 0.5f, 1.0f - v, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glDisable(GL_SCISSOR_TEST);
}

// The default framebuffer as floats 0..1, row 0 at the bottom.
std::vector<float> read_picture(int width, int height) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(width * height * 4));
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bytes.data());
    std::vector<float> picture(bytes.size());

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        picture[i] = static_cast<float>(bytes[i]) / 255.0f;
    }

    return picture;
}

float half_to_float(std::uint16_t h) {
    const int exponent = (h >> 10) & 0x1F;
    const int mantissa = h & 0x3FF;
    const float sign = (h & 0x8000) != 0 ? -1.0f : 1.0f;

    // Subnormal halves.
    if (exponent == 0) {
        return sign * std::ldexp(static_cast<float>(mantissa), -24);
    }

    // Infinity (the pass never writes NaN).
    if (exponent == 31) {
        return sign * INFINITY;
    }

    return sign * std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, exponent - 15);
}

float channel(const std::vector<std::uint8_t>& screen, std::size_t pixel, int c) {
    std::uint16_t h = 0;
    std::memcpy(&h, &screen[pixel * kTexel + 2 * static_cast<std::size_t>(c)], 2);
    return half_to_float(h);
}

// A half float's precision, as vulkan_hdr_test allows it.
bool near(float got, float want) {
    return std::fabs(got - want) <= std::max(2e-3f * want, 1e-4f);
}

// Each channel within a half float's precision.
bool near_rgb(const hdr::Rgb& got, const hdr::Rgb& want) {
    return near(got[0], want[0]) && near(got[1], want[1]) && near(got[2], want[2]);
}

// The layer's set-up and the output for each request: EDR's scRGB for any HDR where the display
// offers EDR, SDR as before otherwise.
void test_layer_choice() {
    CHECK(mtlgl::shown_output(hdr::Output::Sdr, true) == hdr::Output::Sdr);
    CHECK(mtlgl::shown_output(hdr::Output::ScRgb, true) == hdr::Output::ScRgb);
    CHECK(mtlgl::shown_output(hdr::Output::Hdr10, true) == hdr::Output::ScRgb);
    CHECK(mtlgl::shown_output(hdr::Output::ScRgb, false) == hdr::Output::Sdr);
    CHECK(mtlgl::layer_setup(hdr::Output::Sdr) == mtlgl::LayerSetup{});
    const mtlgl::LayerSetup edr = mtlgl::layer_setup(hdr::Output::ScRgb);
    CHECK(edr.extended_range && edr.half_float && edr.linear_srgb && edr.shader_writes);
    // macOS's display reading (platform/window.cpp's hdr_display) leads to scRGB's 1.0 at the
    // system's white, so EDR's headroom is the peak over paper white.
    hdr::Display display;
    display.known = true;
    display.hdr_enabled = true;
    display.relative = true;
    display.headroom = 4.0f;
    display.sdr_white_nits = hdr::kScRgbUnitNits;
    display.peak_nits = display.sdr_white_nits * display.headroom;
    CHECK(hdr::wanted_output(hdr::Mode::On, display, true) == hdr::Output::ScRgb);
    const hdr::Levels levels = hdr::resolve_levels(0, 0, display);
    CHECK(levels.paper_white == hdr::kScRgbUnitNits);
    CHECK(hdr::headroom(levels) == display.headroom - 1.0f);
}

// The pass's pixels against hdr::encode's of `picture` (row 0 at the bottom), top row first.
void check_against_cpu(
    const std::vector<std::uint8_t>& screen,
    const std::vector<float>& picture,
    int width,
    int height,
    const hdr::Levels& levels
) {
    CHECK(screen.size() == static_cast<std::size_t>(width * height) * kTexel);

    if (screen.size() != static_cast<std::size_t>(width * height) * kTexel) {
        return;
    }

    int bad = 0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float* in = &picture[(static_cast<std::size_t>(height - 1 - y) * width + x) * 4];
            const hdr::Rgb want = hdr::encode({in[0], in[1], in[2]}, hdr::Output::ScRgb, levels);
            const std::size_t pixel = static_cast<std::size_t>(y) * width + x;
            const hdr::Rgb got{
                channel(screen, pixel, 0), channel(screen, pixel, 1), channel(screen, pixel, 2)
            };

            // The first few differences only, so a broken pass does not flood the log.
            if (!near_rgb(got, want) && bad++ < 8) {
                std::fprintf(
                    stderr,
                    "pixel %d,%d: %f %f %f, want %f %f %f\n",
                    x,
                    y,
                    got[0],
                    got[1],
                    got[2],
                    want[0],
                    want[1],
                    want[2]
                );
            }
        }
    }

    CHECK(bad == 0);
}

// The channels in which two scRGB read-backs of the same size differ by more than a half float's
// precision.
int differing_channels(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    int differ = 0;

    for (std::size_t i = 0; i < a.size() / kTexel; ++i) {
        for (int c = 0; c < 3; ++c) {
            differ += near(channel(a, i, c), channel(b, i, c)) ? 0 : 1;
        }
    }

    return differ;
}

// The levels the test encodes at: BT.2408's white, a dim one, and EDR's (white at scRGB's 1.0).
const hdr::Levels kLevels[] = {
    {203.0f, 1000.0f, {}},
    {80.0f, 400.0f, {}},
    {hdr::kScRgbUnitNits, hdr::kScRgbUnitNits * 4.0f, {0.5f, 1.0f, 1.0f, 1.0f}},
};

// The Vulkan backend's pass on the same picture, one read-back for each of kLevels; nothing where
// Vulkan cannot open. The window's drawable size is the picture's (`width`, `height`).
std::optional<std::vector<std::vector<std::uint8_t>>> vulkan_reference(int& width, int& height) {
    openrac::test::GlContext context;
    std::string error;

    if (!context.open(error, width, height) || !openrac::vkgl::active()) {
        std::printf("no Vulkan to compare with: %s\n", error.c_str());
        return std::nullopt;
    }

    context.window().drawable_size(width, height);
    paint(width, height);
    std::vector<std::vector<std::uint8_t>> out;

    for (const hdr::Levels& levels : kLevels) {
        std::vector<std::uint8_t> screen;
        CHECK(openrac::vkgl::test_hdr_encode(hdr::Output::ScRgb, levels, width, height, screen));
        out.push_back(std::move(screen));
    }

    return out;
}

// The painted `width` x `height` picture scaled up 1.5 times, as to a larger drawable: the red
// quarter's middle (GL's bottom left, so the screen's bottom left too) is red at paper white.
void test_scaled(int width, int height) {
    const int wide = width * 3 / 2;
    const int tall = height * 3 / 2;
    std::vector<std::uint8_t> scaled;
    CHECK(mtlgl::test_hdr_encode(kLevels[0], wide, tall, scaled));
    CHECK(scaled.size() == static_cast<std::size_t>(wide * tall) * kTexel);

    if (scaled.size() != static_cast<std::size_t>(wide * tall) * kTexel) {
        return;
    }

    const std::size_t middle = static_cast<std::size_t>(tall * 3 / 4) * wide + wide / 4;
    CHECK(near(channel(scaled, middle, 0), kLevels[0].paper_white / hdr::kScRgbUnitNits));
    CHECK(channel(scaled, middle, 1) == 0.0f);
}

// The pass without a window: against the CPU at each of kLevels, against Vulkan's where it ran,
// and scaled to a larger screen.
void test_encode(
    int width, int height, const std::optional<std::vector<std::vector<std::uint8_t>>>& vulkan
) {
    std::string error;
    CHECK(mtlgl::open({nullptr, width, height, false}, error));
    std::string missing;
    CHECK(load(mtlgl::get_proc, missing));
    paint(width, height);
    const std::vector<float> picture = read_picture(width, height);

    for (std::size_t n = 0; n < std::size(kLevels); ++n) {
        std::vector<std::uint8_t> screen;
        CHECK(mtlgl::test_hdr_encode(kLevels[n], width, height, screen));
        check_against_cpu(screen, picture, width, height, kLevels[n]);
        // Both backends run the same shader: the same half floats, to a half float's precision.
        CHECK(!vulkan || (*vulkan)[n].size() == screen.size());

        if (vulkan && (*vulkan)[n].size() == screen.size()) {
            CHECK(differing_channels(screen, (*vulkan)[n]) == 0);
        }
    }

    test_scaled(width, height);
    mtlgl::close();
}

// A hidden window's own layer: EDR and back, each followed by a present, which Metal's validation
// (MTL_DEBUG_LAYER) checks, writing the drawable with the encode pass in HDR.
void test_window_layer() {
    openrac::platform::Platform sdl(openrac::platform::kVideo);
    SDL_Window* window =
        sdl.ok() ? SDL_CreateWindow("OpenRAC test", 64, 48, SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN)
                 : nullptr;

    // No window system here (a headless session): the layer cannot be checked.
    if (window == nullptr) {
        std::printf("no window for the layer check: %s\n", SDL_GetError());
        return;
    }

    std::string error;
    CHECK(mtlgl::open({window, 0, 0, false}, error));
    CHECK(mtlgl::current_layer() == mtlgl::layer_setup(hdr::Output::Sdr));
    CHECK(mtlgl::hdr_output() == hdr::Output::Sdr);
    // A display without EDR: SDR, and the layer as it was.
    mtlgl::set_hdr(hdr::Output::ScRgb, kLevels[2], false);
    CHECK(mtlgl::hdr_output() == hdr::Output::Sdr && !mtlgl::hdr_offered());
    CHECK(mtlgl::current_layer() == mtlgl::layer_setup(hdr::Output::Sdr));
    // With EDR: the layer turns at once, and turns back.
    mtlgl::set_hdr(hdr::Output::ScRgb, kLevels[2], true);
    CHECK(mtlgl::hdr_output() == hdr::Output::ScRgb && mtlgl::hdr_offered());
    CHECK(mtlgl::current_layer() == mtlgl::layer_setup(hdr::Output::ScRgb));
    mtlgl::present();
    mtlgl::set_hdr(hdr::Output::Sdr, kLevels[2], true);
    CHECK(mtlgl::hdr_output() == hdr::Output::Sdr);
    CHECK(mtlgl::current_layer() == mtlgl::layer_setup(hdr::Output::Sdr));
    mtlgl::present();
    mtlgl::close();
    SDL_DestroyWindow(window);
}

}  // namespace

int main() {
    int width = 64;
    int height = 48;
    std::string error;

    // Metal at all: skipped where there is none.
    if (!mtlgl::open({nullptr, 1, 1, false}, error)) {
        std::printf("SKIP: %s\n", error.c_str());
        return openrac::test::kSkip;
    }

    mtlgl::close();
    test_layer_choice();
    const auto vulkan = vulkan_reference(width, height);
    test_encode(width, height, vulkan);
    test_window_layer();
    return openrac::test::result();
}
