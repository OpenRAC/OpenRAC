// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exercises AMD temporal dispatch on synthetic uniform colour, reversed depth and zero motion.
#include <array>
#include <iostream>
#include <vector>

#include "enhanced/fsr3/sdk_context.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;

// Exercise both the fixed-capacity import and its larger dynamic fallback.
void test_native_import_setup() {
    std::array<unsigned, 9> textures{};
    glGenTextures(int(textures.size()), textures.data());
    std::array<vkgl::NativeImageUse, 9> uses{};
    const std::array<unsigned char, 4> pixel{17, 38, 91, 255};
    for (std::size_t i = 0; i < textures.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        uses[i] = {textures[i], false};
    }
    bool called = false;
    const auto record = [&](VkCommandBuffer command, std::span<const vkgl::NativeImage> images) {
        called = true;
        CHECK(command != VK_NULL_HANDLE);
        for (const auto& image : images) {
            CHECK(image.image != VK_NULL_HANDLE);
            CHECK(image.width == 1 && image.height == 1);
        }
        return true;
    };
    // Establish read layouts and flush initial uploads before counting repeated imports.
    CHECK(vkgl::record_native_compute(uses, record));
    glFinish();
    for (const bool fast : {false, true}) {
        renderer::render_opt::set(renderer::render_opt::Opt::NativeComputeSetup, fast);
        for (const std::size_t count : {8u, 9u}) {
            const auto before = vkgl::submission_counters().barriers;
            called = false;
            CHECK(vkgl::record_native_compute(std::span(uses).first(count), record));
            CHECK(called);
            CHECK(vkgl::submission_counters().barriers - before == (fast ? 0 : count));
        }
        // An invalid last input must leave all earlier inputs and recording untouched.
        auto invalid = uses;
        invalid.back().texture = 0;
        const auto before = vkgl::submission_counters().barriers;
        called = false;
        CHECK(!vkgl::record_native_compute(invalid, record));
        CHECK(!called);
        CHECK(vkgl::submission_counters().barriers == before);
    }
    CHECK(vkgl::allocate_storage_texture(textures[0], GL_RGBA8, 1, 1));
    const std::array<vkgl::NativeImageUse, 1> writable{{{textures[0], true}}};
    for (unsigned repeat = 0; repeat < 2; ++repeat) {
        const auto before = vkgl::submission_counters().barriers;
        CHECK(vkgl::record_native_compute(writable, record));
        CHECK(vkgl::submission_counters().barriers == before + 1);
    }
    glFinish();
    glDeleteTextures(int(textures.size()), textures.data());
    renderer::render_opt::reset();
    CHECK(vkgl::submission_counters().validation_errors == 0);
}

// A small synthetic scene with explicitly initialised temporal inputs and a storage-capable output.
struct Frame {
    Frame() = default;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;
    renderer::FrameBuffer world;         // Render-resolution colour and sampleable depth.
    renderer::FrameBuffer output;        // Display-resolution colour read back after dispatch.
    std::array<unsigned, 3> textures{};  // Motion, reactive mask and float depth input.

    // Allocates independent input images; no game data or external files participate.
    bool create(std::string& error) {
        // A 1.5x display-to-render ratio is the FSR Quality preset.
        if (!world.create(64, 48, error, 1, true) || !output.create(96, 72, error)) {
            return false;
        }

        glGenTextures(3, textures.data());
        std::vector<float> zero(std::size_t{64} * 48 * 4);

        // Motion is float RGBA; the SDK consumes its first two channels in render-pixel units.
        for (const auto texture : textures) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, 64, 48, 0, GL_RGBA, GL_FLOAT, zero.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        std::vector<float> depth(std::size_t{64} * 48 * 4, .5f);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 48, GL_RGBA, GL_FLOAT, depth.data());
        return vkgl::allocate_storage_texture(output.colour_texture(), GL_RGBA8, 96, 72);
    }

    // Clears a finite reversed-depth surface with a known, uniform LDR colour.
    void clear(std::array<float, 3> colour) {
        glBindFramebuffer(GL_FRAMEBUFFER, world.id());
        glViewport(0, 0, 64, 48);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glClearColor(colour[0], colour[1], colour[2], 1);
        glClearDepth(0.5);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    // Releases owned images while the test's context is still current.
    ~Frame() {
        glDeleteTextures(3, textures.data());
        world.release();
        output.release();
    }
};

// Compares the centre RGB, outside the reconstruction filter's boundary footprint, in UNORM bytes.
void check_centre_colour(const renderer::FrameBuffer& output, std::array<float, 3> colour) {
    const auto pixels = output.read_rgba();

    // Three UNORM steps allow filter and quantisation rounding without accepting stale colour.
    for (unsigned c = 0; c < 3; ++c) {
        const auto at = (36 * 96 + 48) * 4 + c;
        CHECK(std::abs(static_cast<float>(pixels[at]) - colour[c] * 255) < 3);
    }
}

// Constant inputs must retain their RGB after upscaling, including a reset after a colour change.
void test_temporal_dispatch_preserves_constant_colour() {
    Frame frame;
    std::string error;
    CHECK(frame.create(error));
    enhanced::fsr3_detail::SdkContext upscaler;
    const bool configured = upscaler.configure({64, 48}, {96, 72}, error);
    CHECK(configured);

    // SDK failures include their numeric error or the originating shader diagnostic.
    if (!configured) {
        std::cerr << error << '\n';
        return;
    }

    const std::array<unsigned, 5> textures{
        frame.world.colour_texture(),
        frame.textures[2],
        frame.textures[0],
        frame.textures[1],
        frame.output.colour_texture()
    };

    // The final iteration changes colour and invalidates history rather than accepting stale radiance.
    for (unsigned iteration = 0; iteration < 5; ++iteration) {
        const std::array<float, 3> colour = iteration == 4 ? std::array<float, 3>{.8f, .1f, .4f}
                                                           : std::array<float, 3>{.25f, .5f, .75f};
        frame.clear(colour);
        enhanced::fsr3_detail::Dispatch dispatch;
        dispatch.reset = iteration == 0 || iteration == 4;
        dispatch.sharpness = 0;
        const bool rendered = upscaler.dispatch(textures, dispatch, error);
        CHECK(rendered);

        // Do not inspect an output image the SDK failed to produce.
        if (!rendered) {
            std::cerr << error << '\n';
            break;
        }

        check_centre_colour(frame.output, colour);
    }

    upscaler.release();
    CHECK(vkgl::submission_counters().validation_errors == 0);
}

// Compare both live SDK paths on moving high-contrast detail, including history and sharpening.
void test_half_history_matches_reference(bool setup_only = false, bool fp16 = true) {
    Frame frame;
    std::string error;
    CHECK(frame.create(error));
    renderer::FrameBuffer half_output;
    CHECK(half_output.create(96, 72, error));
    CHECK(vkgl::allocate_storage_texture(half_output.colour_texture(), GL_RGBA8, 96, 72));
    enhanced::fsr3_detail::SdkContext full, half;
    renderer::render_opt::set(renderer::render_opt::Opt::FsrFloat16, setup_only && fp16);
    CHECK(full.configure({64, 48}, {96, 72}, error));
    if (!setup_only) {
        CHECK(!full.float16_active());
    }
    renderer::render_opt::set(renderer::render_opt::Opt::FsrFloat16, !setup_only || fp16);
    CHECK(half.configure({64, 48}, {96, 72}, error));
    vkgl::NativeDevice device;
    CHECK(vkgl::native_device(device));
    CHECK(half.float16_active() == ((!setup_only || fp16) && device.float16 && device.int16));
    std::vector<unsigned char> colour(64 * 48 * 4);
    std::vector<float> motion(64 * 48 * 4);
    for (unsigned i = 0; i < 64 * 48; ++i) {
        motion[i * 4] = -1.f;
    }
    glBindTexture(GL_TEXTURE_2D, frame.textures[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 48, GL_RGBA, GL_FLOAT, motion.data());
    for (unsigned tick = 0; tick < 12; ++tick) {
        frame.clear({0, 0, 0});
        for (unsigned y = 0; y < 48; ++y) {
            for (unsigned x = 0; x < 64; ++x) {
                const unsigned at = (y * 64 + x) * 4, moving = (x + 64 - tick) % 64;
                colour[at] = static_cast<unsigned char>((moving * 4 + y * 3) % 256);
                colour[at + 1] = moving < 32 ? 3 : 250;
                colour[at + 2] = ((moving / 3 + y / 3) % 2) ? 230 : 12;
                colour[at + 3] = 255;
            }
        }
        glBindTexture(GL_TEXTURE_2D, frame.world.colour_texture());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 48, GL_RGBA, GL_UNSIGNED_BYTE, colour.data());
        enhanced::fsr3_detail::Dispatch dispatch;
        dispatch.reset = tick == 0 || tick == 8;
        dispatch.sharpness = tick < 6 ? 0.f : .4f;
        dispatch.jitter = {float(int(tick % 3) - 1) * .2f, float(int(tick % 2)) * .25f};
        std::array<unsigned, 5> inputs{
            frame.world.colour_texture(),
            frame.textures[2],
            frame.textures[0],
            frame.textures[1],
            frame.output.colour_texture()
        };
        if (setup_only) {
            renderer::render_opt::set(renderer::render_opt::Opt::NativeComputeSetup, false);
        }
        CHECK(full.dispatch(inputs, dispatch, error));
        const auto reference = frame.output.read_rgba();
        inputs[4] = half_output.colour_texture();
        if (setup_only) {
            renderer::render_opt::set(renderer::render_opt::Opt::NativeComputeSetup, true);
        }
        CHECK(half.dispatch(inputs, dispatch, error));
        const auto result = half_output.read_rgba();
        CHECK(reference.size() == result.size());
        if (setup_only) {
            CHECK(reference == result);
        }
        double sum = 0;
        unsigned worst = 0;
        for (std::size_t at = 0; at < result.size(); ++at) {
            if (at % 4 != 3) {
                const auto delta = unsigned(std::abs(int(reference[at]) - int(result[at])));
                sum += delta;
                worst = std::max(worst, delta);
            }
        }
        const double average = sum / (96 * 72 * 3);
        std::cout << "Half history frame " << tick << ": mean " << average << ", max " << worst
                  << '\n';
        CHECK(average < .125 && worst <= 4);
    }
    // A precision change at the same size must recreate pipelines and invalidate history.
    renderer::render_opt::set(renderer::render_opt::Opt::FsrFloat16, false);
    CHECK(half.configure({64, 48}, {96, 72}, error));
    CHECK(!half.float16_active());
    renderer::render_opt::set(renderer::render_opt::Opt::FsrFloat16, true);
    CHECK(half.configure({64, 48}, {96, 72}, error));
    CHECK(half.float16_active() == (device.float16 && device.int16));
    renderer::render_opt::reset();
    CHECK(vkgl::submission_counters().validation_errors == 0);
}
}  // namespace

int main() {
    try {
        openrac::test::GlContext context;
        std::string error;

        // Native GPU checks may skip only when their graphics context cannot be opened.
        if (!context.open(error)) {
            std::cerr << error << '\n';
            return openrac::test::kSkip;
        }

        CHECK(openrac::gl::enhanced_rendering_available());
        test_native_import_setup();
        test_temporal_dispatch_preserves_constant_colour();
        test_half_history_matches_reference();
        test_half_history_matches_reference(true);
        test_half_history_matches_reference(true, false);
        return openrac::test::result();
    } catch (...) {
        return 1;
    }
}
