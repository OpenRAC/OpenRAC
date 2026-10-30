// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exercises Arm ASR dispatch on synthetic inputs, and compares it with FSR 3.1 on moving detail.
#include <array>
#include <iostream>
#include <vector>

#include "enhanced/asr/asr_context.h"
#include "enhanced/fsr3/sdk_context.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;

// Render-resolution inputs and two display-resolution outputs, one per upscaler.
struct Frame {
    Frame() = default;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    renderer::FrameBuffer world;                   // Render-resolution colour and depth.
    std::array<renderer::FrameBuffer, 2> outputs;  // ASR's, then FSR 3.1's.
    std::array<unsigned, 3> textures{};            // Motion, reactive mask and float depth.

    bool create(std::string& error) {
        if (!world.create(64, 48, error, 1, true)) {
            return false;
        }
        for (auto& output : outputs) {
            if (!output.create(96, 72, error)
                || !vkgl::allocate_storage_texture(output.colour_texture(), GL_RGBA8, 96, 72)) {
                return false;
            }
        }

        glGenTextures(3, textures.data());
        std::vector<float> zero(std::size_t{64} * 48 * 4);
        for (const auto texture : textures) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, 64, 48, 0, GL_RGBA, GL_FLOAT, zero.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        std::vector<float> depth(std::size_t{64} * 48 * 4, .5f);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 48, GL_RGBA, GL_FLOAT, depth.data());
        return true;
    }

    std::array<unsigned, 5> inputs(unsigned output) const {
        return {
            world.colour_texture(),
            textures[2],
            textures[0],
            textures[1],
            outputs[output].colour_texture()
        };
    }

    ~Frame() {
        glDeleteTextures(3, textures.data());
        world.release();
        for (auto& output : outputs) {
            output.release();
        }
    }
};

// Uploads a pattern moving one render pixel to the right per tick, with matching motion.
void upload_pattern(Frame& frame, unsigned tick) {
    std::vector<unsigned char> colour(64 * 48 * 4);
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
}

// A uniform surface keeps its colour through ASR, including a reset after a colour change.
void test_constant_colour(Frame& frame, enhanced::asr_detail::AsrContext& asr) {
    std::string error;
    for (unsigned iteration = 0; iteration < 5; ++iteration) {
        const std::array<float, 3> colour = iteration == 4 ? std::array<float, 3>{.8f, .1f, .4f}
                                                           : std::array<float, 3>{.25f, .5f, .75f};
        glBindFramebuffer(GL_FRAMEBUFFER, frame.world.id());
        glViewport(0, 0, 64, 48);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(colour[0], colour[1], colour[2], 1);
        glClear(GL_COLOR_BUFFER_BIT);
        enhanced::fsr3_detail::Dispatch dispatch;
        dispatch.reset = iteration == 0 || iteration == 4;
        dispatch.sharpness = 0;
        const bool rendered = asr.dispatch(frame.inputs(0), dispatch, error);
        CHECK(rendered);
        if (!rendered) {
            std::cerr << error << '\n';
            return;
        }
        const auto pixels = frame.outputs[0].read_rgba();
        for (unsigned c = 0; c < 3; ++c) {
            const auto at = (36 * 96 + 48) * 4 + c;
            CHECK(std::abs(static_cast<float>(pixels[at]) - colour[c] * 255) < 3);
        }
    }
}

// On moving high-contrast detail ASR stays near FSR 3.1 (an FSR 2 derivative differs in detail,
// a mean of 6 to 16 steps here); a broken input binding or history reads tens of steps off.
void test_close_to_fsr(Frame& frame, enhanced::asr_detail::AsrContext& asr) {
    std::string error;
    enhanced::fsr3_detail::SdkContext fsr;
    CHECK(fsr.configure({64, 48}, {96, 72}, error));
    std::vector<float> motion(64 * 48 * 4);
    for (unsigned i = 0; i < 64 * 48; ++i) {
        motion[i * 4] = -1.f;
    }
    glBindTexture(GL_TEXTURE_2D, frame.textures[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 48, GL_RGBA, GL_FLOAT, motion.data());
    for (unsigned tick = 0; tick < 12; ++tick) {
        upload_pattern(frame, tick);
        enhanced::fsr3_detail::Dispatch dispatch;
        dispatch.reset = tick == 0 || tick == 8;
        dispatch.sharpness = tick < 6 ? 0.f : .4f;
        dispatch.jitter = {float(int(tick % 3) - 1) * .2f, float(int(tick % 2)) * .25f};
        CHECK(asr.dispatch(frame.inputs(0), dispatch, error));
        CHECK(fsr.dispatch(frame.inputs(1), dispatch, error));
        const auto ours = frame.outputs[0].read_rgba();
        const auto reference = frame.outputs[1].read_rgba();
        double sum = 0;
        for (std::size_t at = 0; at < ours.size(); ++at) {
            if (at % 4 != 3) {
                sum += std::abs(int(reference[at]) - int(ours[at]));
            }
        }
        const double average = sum / (96 * 72 * 3);
        std::cout << "ASR against FSR 3.1, frame " << tick << ": mean " << average << '\n';
        CHECK(average < 24);
    }
    fsr.release();
}
}  // namespace

int main() {
    try {
        openrac::test::GlContext context;
        std::string error;
        if (!context.open(error)) {
            std::cerr << error << '\n';
            return openrac::test::kSkip;
        }
        vkgl::NativeDevice device;
        // ASR's shaders need these features; a device without them keeps FSR 3.1.
        if (!vkgl::native_device(device) || !device.float16 || !device.int16
            || !device.subgroup_extended_types || !device.fragment_stores) {
            std::cerr << "no 16-bit shader arithmetic\n";
            return openrac::test::kSkip;
        }
        Frame frame;
        CHECK(frame.create(error));
        enhanced::asr_detail::AsrContext asr;
        const bool configured = asr.configure({64, 48}, {96, 72}, error);
        CHECK(configured);
        if (!configured) {
            std::cerr << error << '\n';
            return openrac::test::result();
        }
        CHECK(asr.float16_active());
        test_constant_colour(frame, asr);
        test_close_to_fsr(frame, asr);
        asr.release();
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return openrac::test::result();
    } catch (...) {
        return 1;
    }
}
