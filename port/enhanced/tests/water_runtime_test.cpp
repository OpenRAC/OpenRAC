// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cstdio>
#include <fstream>
#include <string_view>

#include "common/stats.h"
#include "enhanced/renderer/water.h"
#include "enhanced/tests/water_test_bake.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif
using namespace openrac;
using namespace openrac::gl;
namespace wgl = renderer::world::gl;

int main() {
#ifndef OPENRAC_VULKAN
    return test::kSkip;
#else
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::printf("SKIP %s\n", error.c_str());
        return test::kSkip;
    }
    if (!gl::is_vulkan() || !gl::enhanced_rendering_available()) {
        return test::kSkip;
    }
    // Exercise the actual controller with synthetic data, not a game process.
    // Duplicate frames, pause, original-mode fallback and re-enable must agree.
    test::WaterTestBake fixture;
    // An empty scene has no probe selector. Supply its unsigned empty mask view
    // explicitly, matching the sampler type even when probe selection is disabled.
    GLuint mask_buffer = 0, mask_texture = 0;
    const std::array<unsigned, 4> empty_mask{};
    glGenBuffers(1, &mask_buffer);
    glBindBuffer(wgl::GL_TEXTURE_BUFFER, mask_buffer);
    glBufferData(wgl::GL_TEXTURE_BUFFER, sizeof(empty_mask), empty_mask.data(), GL_STATIC_DRAW);
    glGenTextures(1, &mask_texture);
    glActiveTexture(GL_TEXTURE0 + 15);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, mask_texture);
    glTexBuffer(wgl::GL_TEXTURE_BUFFER, wgl::GL_RGBA32UI, mask_buffer);
    glActiveTexture(GL_TEXTURE0);
    scene::LevelScene fixture_scene;
    auto runtime = enhanced::make_water(fixture_scene, fixture.folder);
    CHECK(bool(runtime));
    if (runtime) {
        const auto upload_counter = std::size_t(stats::Counter::TextureUploadBytes);
        const auto before_init = stats::g_counters[upload_counter];
        const bool initialized = runtime->init(error);
        if (!initialized) {
            std::printf("water runtime fixture: %s\n", error.c_str());
        }
        CHECK(initialized);
        const auto initial_upload_bytes = stats::g_counters[upload_counter] - before_init;
        if (initialized) {
            const auto dispatches = []() {
                return vkgl::submission_counters().commands[std::size_t(vkgl::Command::Dispatch)];
            };
            renderer::FrameInput input;
            input.enhanced_water = true;
            input.frame = 10;
            input.water_interactor = renderer::Vec3{.5f, .5f, 1};
            enhanced::set_water_debug_settings(enhanced::WaterDebugSettings{});
            runtime->prepare(input);
            CHECK(enhanced::water_available());
            auto previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() == previous);
            auto paused = enhanced::WaterDebugSettings{};
            paused.paused = true;
            enhanced::set_water_debug_settings(paused);
            input.frame = 11;
            input.water_interactor = renderer::Vec3{.75f, .5f, 1};
            previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() == previous + 1);  // lighting only
            input.enhanced_water = false;
            runtime->prepare(input);
            CHECK(!enhanced::water_available());
            enhanced::set_water_debug_settings(enhanced::WaterDebugSettings{});
            input.enhanced_water = true;
            previous = dispatches();
            runtime->prepare(input);
            CHECK(enhanced::water_available());
            CHECK(dispatches() > previous);  // same-tick re-enable
            stats::Probe* off = nullptr;
            for (auto* probe : stats::probes()) {
                if (std::string_view(probe->variable) == "OPENRAC_WATER_OFF") {
                    off = probe;
                }
            }
            CHECK(off != nullptr);
            if (off) {
                off->on = true;
                runtime->prepare(input);
                CHECK(!enhanced::water_available());
                off->on = false;
                runtime->prepare(input);
                CHECK(enhanced::water_available());
            }
            std::array<renderer::EffectVertex, 3> triangle{
                {{0, 0, 1, 0, 0, 0x80808080},
                 {1, 0, 1, 1, 0, 0x80808080},
                 {0, 1, 1, 0, 1, 0x80808080}}
            };
            renderer::EffectStrip strip;
            strip.surface = 1;
            strip.count = 3;
            input.strips = std::span(&strip, 1);
            input.strip_vertices = triangle;
            input.frame = 12;
            input.water_interactor = renderer::Vec3{100, 100, 100};
            runtime->prepare(input);
            enhanced::reset_water_simulation();
            runtime->prepare(input);
            previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() == previous);
            // Leaving contact keeps the old window alive while its wake fades.
            input.strips = {};
            input.strip_vertices = {};
            input.frame = 13;
            previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() > previous);
            for (input.frame = 14; input.frame <= 190; ++input.frame) {
                runtime->prepare(input);
            }
            previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() > previous); // still settling after three seconds
            for (input.frame = 192; input.frame <= 330; ++input.frame) {
                runtime->prepare(input);
            }
            previous = dispatches();
            runtime->prepare(input);
            CHECK(dispatches() == previous);  // fully faded, no visible water
            input.water_interactor = renderer::Vec3{.5f, .5f, 1};
            ++input.frame;
            runtime->prepare(input);
            CHECK(dispatches() > previous);  // re-entry starts the simulation again
        }
        runtime->release();
        CHECK(!enhanced::water_available());
        const auto before_reinit = stats::g_counters[upload_counter];
        CHECK(runtime->init(error));
        CHECK(stats::g_counters[upload_counter] - before_reinit == initial_upload_bytes);
        runtime->release();
        CHECK(!enhanced::water_available());
    }
    // Malformed metadata must fall back before it can reach XY indexing or GPU allocation.
    for (const auto& [offset, value] : std::array<std::pair<std::streamoff, float>, 4>{
             {{64, 1.5f}, {68, 2.f}, {52, 1e30f}, {144, 1.f}}
         }) {
        fixture.write_bake();
        std::fstream file(
            fixture.folder / "water.bin", std::ios::in | std::ios::out | std::ios::binary
        );
        file.seekp(offset);
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
        file.close();
        CHECK(!enhanced::make_water(fixture_scene, fixture.folder));
    }

    glDeleteTextures(1, &mask_texture);
    glDeleteBuffers(1, &mask_buffer);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    return test::result();
#endif
}
