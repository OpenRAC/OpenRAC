// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <bit>
#include <cmath>
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

// Real GPU controller: impacts must wake an idle pond, without requiring Ratchet in the water.
void check_impact_waves(const test::WaterTestBake& fixture, scene::LevelScene& scene) {
    renderer::Shader read;
    renderer::FrameBuffer target;
    std::string error;
    CHECK(target.create(512, 128, error));
    CHECK(read.build(
        "impact wave bits",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
uniform sampler2D waves;
layout(location=0) out vec4 result;
void main(){ivec2 p=ivec2(gl_FragCoord.xy);
uint b=floatBitsToUint(texelFetch(waves,ivec2(p.x/4,p.y),0)[p.x%4]);
result=vec4(float(b&255u),float((b>>8)&255u),float((b>>16)&255u),float(b>>24))/255.;})",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    const auto snapshot = [&] {
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, 512, 128);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        read.use();
        glUniform1i(read.uniform("waves"), 20);
        CHECK(enhanced::bind_water_deformation(read));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return target.read_rgba();
    };
    const renderer::WaterImpulse blast{{4, 4, 1}, {4, 4, 1}, 1.5f, 1.2f};
    const renderer::WaterImpulse projectile{{4, 4, 4}, {4, 4, -2}, .3f, .6f};
    for (const auto impulse : {blast, projectile}) {
        auto water = enhanced::make_water(scene, fixture.folder);
        const bool ready = water && water->init(error);
        CHECK(ready);
        if (!ready) {
            std::printf("impact fixture: %s\n", error.c_str());
            continue;
        }
        renderer::FrameInput input;
        input.enhanced_water = true;
        input.frame = 1;
        input.water_interactor =
            renderer::Vec3{30, 30, 20};  // Shore/air, no body/hand contribution.
        input.water_impulses = std::span(&impulse, 1);
        water->prepare(
            input
        );  // Includes reset: the first real impact must survive initialization.
        const auto first = snapshot();
        float height_peak = 0, velocity_peak = 0;
        double mass = 0;
        for (std::size_t texel = 0; texel < 128 * 128; ++texel) {
            for (unsigned c = 0; c < 4; ++c) {
                const auto at = (texel * 4 + c) * 4;
                const auto bits = std::uint32_t(first[at]) | (std::uint32_t(first[at + 1]) << 8)
                                  | (std::uint32_t(first[at + 2]) << 16)
                                  | (std::uint32_t(first[at + 3]) << 24);
                const float value = std::bit_cast<float>(bits);
                CHECK(std::isfinite(value));
                if (c == 0) {
                    height_peak = std::max(height_peak, std::abs(value));
                    mass += value;
                }
                if (c == 1 || c == 2) {
                    velocity_peak = std::max(velocity_peak, std::abs(value));
                }
            }
        }
        CHECK(height_peak > 1e-6f && velocity_peak > .001f);
        CHECK(
            height_peak <= enhanced::WaterDebugSettings{}.displacement_limit + 1e-6f
            && velocity_peak <= 2.501f
        );
        CHECK(std::abs(mass) < .002);  // The pressure kick changes momentum, not water mass.
        water->prepare(input);
        CHECK(snapshot() == first);  // Colour/depth/shadow preparations never apply an event twice.
        auto options = enhanced::water_debug_settings();
        options.paused = true;
        enhanced::set_water_debug_settings(options);
        ++input.frame;
        water->prepare(input);
        CHECK(snapshot() == first);
        enhanced::set_water_debug_settings({});
        // A sustained burst at the per-tick limit must stay finite and bounded.
        std::array<renderer::WaterImpulse, renderer::kWaterImpulseLimit> burst;
        burst.fill(impulse);
        input.water_impulses = burst;
        for (unsigned tick = 0; tick < 30; ++tick) {
            ++input.frame;
            water->prepare(input);
        }
        const auto stressed = snapshot();
        std::array<float, 4> maxima{};
        for (std::size_t at = 0; at < stressed.size(); at += 4) {
            const auto bits = std::uint32_t(stressed[at]) | (std::uint32_t(stressed[at + 1]) << 8)
                              | (std::uint32_t(stressed[at + 2]) << 16)
                              | (std::uint32_t(stressed[at + 3]) << 24);
            const float value = std::bit_cast<float>(bits);
            CHECK(std::isfinite(value));
            const unsigned channel = unsigned(at / 4) % 4;
            maxima[channel] = std::max(maxima[channel], std::abs(value));
        }
        std::printf(
            "Impact burst: peak height %.6f, velocity %.6f/%.6f, marker %.6f\n",
            maxima[0],
            maxima[1],
            maxima[2],
            maxima[3]
        );
        CHECK(maxima[0] <= enhanced::WaterDebugSettings{}.displacement_limit + 1e-6f);
        CHECK(maxima[1] <= 2.501f && maxima[2] <= 2.501f && maxima[3] <= 1.f);
        water->release();
    }
    // Exercise the actual live controls, including source independence and idle activation.
    const auto response = [&](const renderer::WaterImpulse& impulse,
                              enhanced::WaterDebugSettings tuning) {
        enhanced::set_water_debug_settings(tuning);
        auto water = enhanced::make_water(scene, fixture.folder);
        const bool ready = water && water->init(error);
        CHECK(ready);
        std::array<double, 2> result{};  // Kinetic response and its spatial second moment.
        if (!ready) {
            return result;
        }
        renderer::FrameInput input;
        input.frame = 1;
        input.enhanced_water = true;
        input.water_impulses = std::span(&impulse, 1);
        const auto before =
            vkgl::submission_counters().commands[std::size_t(vkgl::Command::Dispatch)];
        water->prepare(input);
        const bool explosion = impulse.from == impulse.to;
        if ((explosion ? tuning.explosion_strength : tuning.projectile_strength) == 0) {
            CHECK(
                vkgl::submission_counters().commands[std::size_t(vkgl::Command::Dispatch)] == before
            );
            return result;
        }
        const auto pixels = snapshot();
        for (std::size_t texel = 0; texel < 128 * 128; ++texel) {
            double energy = 0;
            for (unsigned channel = 1; channel <= 2; ++channel) {
                const auto at = (texel * 4 + channel) * 4;
                const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                                  | (std::uint32_t(pixels[at + 2]) << 16)
                                  | (std::uint32_t(pixels[at + 3]) << 24);
                const float velocity = std::bit_cast<float>(bits);
                CHECK(std::isfinite(velocity));
                energy += double(velocity) * velocity;
            }
            const double dx = std::remainder((double(texel % 128) + .5) * .25 - 4, 32);
            // read_rgba returns top-to-bottom rows; the simulation texture is bottom-to-top.
            const double dy = std::remainder((double(127 - texel / 128) + .5) * .25 - 4, 32);
            result[0] += energy;
            result[1] += energy * (dx * dx + dy * dy);
        }
        if (result[0] > 0) {
            result[1] /= result[0];
        }
        return result;
    };
    for (const auto& impulse : {blast, projectile}) {
        const bool explosion = impulse.from == impulse.to;
        auto tuning = enhanced::WaterDebugSettings{};
        const auto normal = response(impulse, tuning);
        CHECK(normal[0] > 0);
        (explosion ? tuning.explosion_strength : tuning.projectile_strength) = 0;
        CHECK(response(impulse, tuning)[0] == 0);
        (explosion ? tuning.explosion_strength : tuning.projectile_strength) = .5f;
        CHECK(response(impulse, tuning)[0] < normal[0] * .8);
        (explosion ? tuning.explosion_strength : tuning.projectile_strength) = 2;
        CHECK(response(impulse, tuning)[0] > normal[0] * 2);
        tuning = {};
        // Disabling the other source leaves this one byte-for-byte equivalent in its response.
        (explosion ? tuning.projectile_strength : tuning.explosion_strength) = 0;
        CHECK(response(impulse, tuning) == normal);
        tuning = {};
        (explosion ? tuning.explosion_radius : tuning.projectile_radius) = 2;
        CHECK(response(impulse, tuning)[1] > normal[1] * 1.5);
        (explosion ? tuning.explosion_radius : tuning.projectile_radius) = .5f;
        const auto smaller = response(impulse, tuning);
        CHECK(smaller[0] < normal[0] * .8);
        // Tiny projectile radii approach the grid spacing: footprint size is quantised.
        CHECK(smaller[1] < normal[1]);
    }
    enhanced::set_water_debug_settings({});
    // A projectile entirely above the surface must not activate the local simulation.
    auto water = enhanced::make_water(scene, fixture.folder);
    const bool ready = water && water->init(error);
    CHECK(ready);
    if (ready) {
        renderer::WaterImpulse miss{{4, 4, 10}, {5, 4, 10}, .3f, .6f};
        renderer::FrameInput input;
        input.frame = 1;
        input.enhanced_water = true;
        input.water_impulses = std::span(&miss, 1);
        const auto before =
            vkgl::submission_counters().commands[std::size_t(vkgl::Command::Dispatch)];
        water->prepare(input);
        CHECK(vkgl::submission_counters().commands[std::size_t(vkgl::Command::Dispatch)] == before);
    }
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
    check_impact_waves(fixture, fixture_scene);
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
