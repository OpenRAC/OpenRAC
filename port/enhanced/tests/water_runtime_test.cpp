// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <cstdio>
#include <fstream>
#include <string_view>

#include "common/stats.h"
#include "enhanced/renderer/water.h"
#include "enhanced/tests/water_test_bake.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
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

#ifdef OPENRAC_VULKAN
// Compare every float bit of the production 128x128 wave map after multi-step updates.
void check_barrier_reuse(const test::WaterTestBake& fixture, scene::LevelScene& scene) {
    renderer::Shader read;
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(512, 128, error));
    CHECK(read.build(
        "water barrier equivalence",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
uniform sampler2D waves;
layout(location=0) out vec4 result;
void main(){
    ivec2 p=ivec2(gl_FragCoord.xy);
    uint bits=floatBitsToUint(texelFetch(waves,ivec2(p.x/4,p.y),0)[p.x%4]);
    result=vec4(float(bits&255u),float((bits>>8)&255u),float((bits>>16)&255u),float(bits>>24))/255.;
})",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    std::vector<std::vector<std::uint8_t>> reference;
    std::array<std::uint64_t, 6> reference_barriers{}, reference_dispatches{};
    constexpr std::array<std::uint64_t, 6> frames{10, 11, 15, 16, 17, 18};
    // First frame/reset and single ticks use four substeps; the four-tick catch-up uses sixteen.
    constexpr std::array<std::uint64_t, 6> saved_barriers{3, 3, 15, 3, 0, 3};
    for (bool compact : {false, true}) {
        renderer::render_opt::set(renderer::render_opt::Opt::WaterStepBarriers, compact);
        renderer::render_opt::set(renderer::render_opt::Opt::ShaderVariants, compact);
        auto water = enhanced::make_water(scene, fixture.folder);
        CHECK(bool(water));
        if (!water || !water->init(error)) {
            CHECK(false);
            return;
        }
        renderer::FrameInput input;
        input.enhanced_water = true;
        for (std::size_t i = 0; i < frames.size(); ++i) {
            auto options = enhanced::WaterDebugSettings{};
            options.paused = i == 4;
            enhanced::set_water_debug_settings(options);
            input.frame = frames[i];
            input.water_interactor = renderer::Vec3{.5f + float(i) * .05f, .5f, 1};
            input.water_hands[0] = renderer::Vec3{.4f + float(i) * .04f, .5f, 1};
            if (i == 3) {
                enhanced::reset_water_simulation();
            }
            const auto before = vkgl::submission_counters();
            water->prepare(input);
            const auto after = vkgl::submission_counters();
            const auto barriers = after.commands[std::size_t(vkgl::Command::Barrier)]
                                  - before.commands[std::size_t(vkgl::Command::Barrier)];
            const auto dispatches = after.commands[std::size_t(vkgl::Command::Dispatch)]
                                    - before.commands[std::size_t(vkgl::Command::Dispatch)];
            if (!compact) {
                reference_barriers[i] = barriers;
                reference_dispatches[i] = dispatches;
            } else {
                CHECK(reference_barriers[i] == barriers + saved_barriers[i]);
                CHECK(reference_dispatches[i] == dispatches);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, target.id());
            glViewport(0, 0, 512, 128);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_CULL_FACE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            read.use();
            glUniform1i(read.uniform("waves"), 20);
            CHECK(enhanced::bind_water_deformation(read));
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            const auto pixels = target.read_rgba();
            if (!compact) {
                reference.push_back(pixels);
            } else {
                CHECK(pixels == reference[i]);
            }
        }
        water->release();
    }
    CHECK(reference[0] != reference[1]);  // the moving interactor must actually create/change waves
    CHECK(reference[3] == reference[4]);  // pausing leaves the wave map unchanged
    std::printf(
        "Water barriers removed: 3 per tick, 15 per four-tick catch-up; six wave maps match "
        "exactly\n"
    );
    renderer::render_opt::reset();
    enhanced::set_water_debug_settings({});
    glDeleteVertexArrays(1, &vao);
}
#endif

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
            // Attachments prepare waves before depth; liquid lighting remains deferred.
            enhanced::prepare_water_deformation(input);
            CHECK(enhanced::water_available());
            const auto waves = dispatches();
            enhanced::prepare_water_deformation(input);
            CHECK(dispatches() == waves);
            runtime->prepare(input);
            CHECK(dispatches() == waves + 1);  // lighting only, never a second simulation step
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
            CHECK(dispatches() > previous);  // still settling after three seconds
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
    check_barrier_reuse(fixture, fixture_scene);
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
