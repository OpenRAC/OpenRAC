// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output's encode pass (renderer/vulkan/presentation/hdr_output.cpp, shaders/hdr_encode.comp) against its
// CPU arithmetic (common/hdr.h). A swapchain cannot be read back and the test machine has no HDR
// screen, so vkgl::test_hdr_encode runs the pass on the default framebuffer into a stand-in of the
// screen's image, HDR10 and scRGB, and the test compares every pixel, turned the right way up,
// with hdr::encode of the bytes the window reads back.
//
// Skipped (77) where Vulkan cannot open.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "common/hdr.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {

using namespace openrac::gl;
namespace hdr = openrac::hdr;

int g_width = 64;  // the drawable's, once the window is open
int g_height = 48;

// Paints the window: four quarters of different colours and a grey ramp along the bottom rows, so
// that both the orientation and the curve show.
void paint_window() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_width, g_height);
    glEnable(GL_SCISSOR_TEST);
    constexpr std::array<std::array<float, 3>, 4> kQuarters = {{
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
    }};
    for (int n = 0; n < 4; ++n) {
        glScissor((n % 2) * g_width / 2, (n / 2) * g_height / 2, g_width / 2, g_height / 2);
        glClearColor(kQuarters[n][0], kQuarters[n][1], kQuarters[n][2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    for (int x = 0; x < g_width; ++x) {
        const float v = static_cast<float>(x) / (g_width - 1);
        glScissor(x, 0, 1, 4);
        glClearColor(v, v * 0.5f, 1.0f - v, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);
}

float half_to_float(std::uint16_t h) {
    const int exponent = (h >> 10) & 0x1F;
    const int mantissa = h & 0x3FF;
    const float sign = (h & 0x8000) != 0 ? -1.0f : 1.0f;
    if (exponent == 0) {
        return sign * std::ldexp(static_cast<float>(mantissa), -24);
    }
    if (exponent == 31) {
        return sign * INFINITY;
    }
    return sign * std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, exponent - 15);
}

// The pass's pixels against the CPU's, for `output` at `levels`: a 10-bit step for HDR10 (the
// GPU's pow and the rounding), a half float's precision for scRGB. `picture` is the pass's input
// as RGBA floats, row 0 at the bottom: the window's, or framebuffer `source`'s.
void test_encode(
    const std::vector<float>& picture,
    hdr::Output output,
    const hdr::Levels& levels,
    unsigned source = 0
) {
    std::vector<std::uint8_t> screen;
    CHECK(openrac::vkgl::test_hdr_encode(output, levels, g_width, g_height, screen, false, source));
    std::vector<std::uint8_t> staged;
    CHECK(openrac::vkgl::test_hdr_encode(output, levels, g_width, g_height, staged, true, source));
    CHECK(screen == staged);
    const std::size_t texel = output == hdr::Output::Hdr10 ? 4 : 8;
    CHECK(screen.size() == static_cast<std::size_t>(g_width * g_height) * texel);
    if (screen.size() != static_cast<std::size_t>(g_width * g_height) * texel) {
        return;
    }
    int bad = 0;
    for (int y = 0; y < g_height; ++y) {
        for (int x = 0; x < g_width; ++x) {
            // The screen's row 0 is its top, the window's its bottom.
            const float* in =
                &picture[(static_cast<std::size_t>(g_height - 1 - y) * g_width + x) * 4];
            const hdr::Rgb want = hdr::encode({in[0], in[1], in[2]}, output, levels);
            const std::uint8_t* got = &screen[(static_cast<std::size_t>(y) * g_width + x) * texel];
            for (int c = 0; c < 3; ++c) {
                float value = 0.0f;
                float tolerance = 0.0f;
                if (output == hdr::Output::Hdr10) {
                    std::uint32_t word = 0;
                    std::memcpy(&word, got, 4);
                    value = static_cast<float>((word >> (10 * c)) & 0x3FF) / 1023.0f;
                    tolerance = 1.5f / 1023.0f;
                } else {
                    std::uint16_t h = 0;
                    std::memcpy(&h, got + 2 * c, 2);
                    value = half_to_float(h);
                    tolerance = std::max(2e-3f * want[c], 1e-4f);
                }
                if (std::fabs(value - want[c]) > tolerance && bad++ < 8) {
                    std::fprintf(
                        stderr,
                        "%s pixel %d,%d channel %d: %f, want %f\n",
                        hdr::name(output),
                        x,
                        y,
                        c,
                        value,
                        want[c]
                    );
                }
            }
        }
    }
    CHECK(bad == 0);
}

// A screen of another size than the window: the pass scales as the blit does; every pixel is
// inside PQ's range and the top left quarter (the window's top left, white) is paper white.
void test_scaled() {
    std::vector<std::uint8_t> screen;
    const hdr::Levels levels{203.0f, 1000.0f, {}};
    CHECK(openrac::vkgl::test_hdr_encode(hdr::Output::Hdr10, levels, 96, 80, screen));
    if (screen.size() != 96u * 80u * 4u) {
        CHECK(false);
        return;
    }
    std::uint32_t word = 0;
    std::memcpy(&word, &screen[(10 * 96 + 70) * 4], 4);
    const float red = static_cast<float>(word & 0x3FF) / 1023.0f;
    CHECK(std::fabs(hdr::pq_decode(red) - 203.0f) < 2.0f);
}

// One full-screen triangle of `colour` into `target`, blended ONE, ONE when `add`.
void fill(
    const openrac::renderer::FrameBuffer& target,
    const openrac::renderer::Shader& shader,
    const std::array<float, 4>& colour,
    bool add
) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, target.width(), target.height());
    shader.use();
    glUniform4f(shader.uniform("colour"), colour[0], colour[1], colour[2], colour[3]);
    if (add) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
}

// The first pixel of `target`, as floats.
std::array<float, 4> first_pixel(const openrac::renderer::FrameBuffer& target) {
    std::vector<float> pixels(static_cast<std::size_t>(target.width() * target.height() * 4));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.id());
    glReadPixels(0, 0, target.width(), target.height(), GL_RGBA, GL_FLOAT, pixels.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return {pixels[0], pixels[1], pixels[2], pixels[3]};
}

bool near4(const std::array<float, 4>& got, const std::array<float, 4>& want) {
    for (int c = 0; c < 4; ++c) {
        if (std::fabs(got[c] - want[c]) > 2e-3f) {
            std::fprintf(
                stderr,
                "got %f %f %f %f, want %f %f %f %f\n",
                got[0],
                got[1],
                got[2],
                got[3],
                want[0],
                want[1],
                want[2],
                want[3]
            );
            return false;
        }
    }
    return true;
}

// The HDR frame (vkgl.h, hdr_frame): in half floats with clamp_colour_writes, a draw's own colour
// is clamped to 0..1 as an 8-bit frame clamps it, and only blending's sums go above 1. Without the
// clamp a float target keeps what it is given; an 8-bit one stops at 1.
void test_float_frame() {
    std::string error;
    openrac::renderer::Shader shader;
    CHECK(shader.build(
        "hdr frame fixture",
        "#version 410 core\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);\n"
        "}\n",
        "#version 410 core\n"
        "uniform vec4 colour;\n"
        "layout(location = 0) out vec4 result;\n"
        "void main() {\n"
        "    if (colour.a < 0.0) {\n"
        "        discard;\n"
        "    }\n"
        "    result = colour;\n"
        "}\n",
        error
    ));
    // A shader whose one output has no location: clamped too.
    openrac::renderer::Shader lone;
    CHECK(lone.build(
        "hdr frame fixture without a location",
        "#version 410 core\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);\n"
        "}\n",
        "#version 410 core\n"
        "uniform vec4 colour;\n"
        "out vec4 result;\n"
        "void main() { result = colour; }\n",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    openrac::renderer::FrameBuffer frame;
    CHECK(frame.create(4, 4, error, 1, false, true) && frame.float_colour());
    openrac::vkgl::clamp_colour_writes(frame.id());
    // A colour past both ends, written as it is: clamped.
    fill(frame, shader, {2.0f, 0.5f, -1.0f, 3.0f}, false);
    CHECK(near4(first_pixel(frame), {1.0f, 0.5f, 0.0f, 1.0f}));
    fill(frame, lone, {3.0f, 0.25f, -2.0f, 2.0f}, false);
    CHECK(near4(first_pixel(frame), {1.0f, 0.25f, 0.0f, 1.0f}));
    // Two adds of 0.75: 1.5, above white.
    fill(frame, shader, {0.0f, 0.0f, 0.0f, 0.0f}, false);
    fill(frame, shader, {0.75f, 0.75f, 0.25f, 0.5f}, true);
    fill(frame, shader, {0.75f, 2.0f, 0.25f, 0.5f}, true);
    CHECK(near4(first_pixel(frame), {1.5f, 1.75f, 0.5f, 1.0f}));
    // Its bytes stop at white, as an 8-bit frame's do.
    const std::vector<std::uint8_t> bytes = frame.read_rgba();
    CHECK(bytes.size() == 64 && bytes[0] == 255 && bytes[1] == 255 && bytes[2] == 128);

    // A float target without the clamp: what it is given.
    openrac::renderer::FrameBuffer plain;
    CHECK(plain.create(4, 4, error, 1, false, true));
    fill(plain, shader, {2.0f, 0.5f, 0.25f, 1.0f}, false);
    CHECK(near4(first_pixel(plain), {2.0f, 0.5f, 0.25f, 1.0f}));

    // An 8-bit target: the adds stop at 1.
    openrac::renderer::FrameBuffer sdr;
    CHECK(sdr.create(4, 4, error));
    fill(sdr, shader, {0.0f, 0.0f, 0.0f, 0.0f}, false);
    fill(sdr, shader, {0.75f, 0.75f, 0.25f, 0.5f}, true);
    fill(sdr, shader, {0.75f, 0.75f, 0.25f, 0.5f}, true);
    const std::vector<std::uint8_t> sdr_bytes = sdr.read_rgba();
    CHECK(sdr_bytes.size() == 64 && sdr_bytes[0] == 255 && sdr_bytes[2] == 128);
    glDeleteVertexArrays(1, &vao);
}

// Light above white with the look's sliders: a float picture of colours added up past white
// (fire-like reds, a cyan, grey, a dim pixel) through the pass, against the CPU's to_light.
void test_look() {
    std::string error;
    openrac::renderer::Shader shader;
    CHECK(shader.build(
        "hdr look fixture",
        "#version 410 core\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);\n"
        "}\n",
        "#version 410 core\n"
        "uniform vec4 colour;\n"
        "layout(location = 0) out vec4 result;\n"
        "void main() { result = colour; }\n",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    openrac::renderer::FrameBuffer frame;
    CHECK(frame.create(g_width, g_height, error, 1, false, true) && frame.float_colour());
    constexpr std::array<std::array<float, 4>, 8> kColumns = {{
        {3.0f, 1.35f, 0.3f, 1.0f},
        {2.0f, 0.9f, 0.2f, 1.0f},
        {1.5f, 0.2f, 0.05f, 1.0f},
        {0.2f, 1.8f, 2.4f, 1.0f},
        {2.0f, 2.0f, 2.0f, 1.0f},
        {4.0f, 0.0f, 3.0f, 1.0f},
        {0.5f, 0.25f, 0.75f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
    }};
    glEnable(GL_SCISSOR_TEST);
    for (int n = 0; n < 8; ++n) {
        glScissor(n * g_width / 8, 0, g_width / 8, g_height);
        fill(frame, shader, kColumns[n], false);
    }
    glDisable(GL_SCISSOR_TEST);
    std::vector<float> picture(static_cast<std::size_t>(g_width * g_height * 4));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.id());
    glReadPixels(0, 0, g_width, g_height, GL_RGBA, GL_FLOAT, picture.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    CHECK(picture[0] > 2.9f);  // above white, as drawn

    const hdr::Look looks[] = {
        {},
        {1.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.5f, 1.0f, 0.0f},
        {0.5f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f, 1.0f},
    };
    for (const hdr::Look& look : looks) {
        test_encode(picture, hdr::Output::Hdr10, {203.0f, 1000.0f, look}, frame.id());
        test_encode(picture, hdr::Output::ScRgb, {203.0f, 2000.0f, look}, frame.id());
    }
    glDeleteVertexArrays(1, &vao);
}

}  // namespace

int main() {
    openrac::test::GlContext context;
    std::string error;

    // No Vulkan here (or it fell back to OpenGL): nothing of the backend to test.
    if (!context.open(error, g_width, g_height) || !openrac::vkgl::active()) {
        std::printf("SKIP: Vulkan unavailable: %s\n", error.c_str());
        return openrac::test::kSkip;
    }
    // A high-density display gives the window more pixels than asked for, and the pass reads all.
    context.window().drawable_size(g_width, g_height);

    paint_window();
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(g_width * g_height * 4));
    glReadPixels(0, 0, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, bytes.data());
    std::vector<float> window(bytes.size());
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        window[i] = bytes[i] / 255.0f;
    }
    test_encode(window, hdr::Output::Hdr10, {203.0f, 1000.0f, {}});
    test_encode(window, hdr::Output::Hdr10, {480.0f, 1500.0f, {}});
    test_encode(window, hdr::Output::ScRgb, {203.0f, 1000.0f, {}});
    test_encode(window, hdr::Output::ScRgb, {80.0f, 400.0f, {}});
    // A look changes nothing at or below white.
    test_encode(window, hdr::Output::Hdr10, {203.0f, 1000.0f, {0.5f, 1.0f, 1.0f, 1.0f}});
    test_scaled();
    test_float_frame();
    test_look();
    // Without an HDR request the screen is sent SDR, and a hidden window has no HDR to offer.
    CHECK(openrac::vkgl::hdr_output() == hdr::Output::Sdr);
    CHECK(openrac::vkgl::submission_counters().validation_errors == 0);
    return openrac::test::result();
}
