// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks SDR colour treatment against analytical curve values, preserving alpha across resizes.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

#include "common/stats.h"
#include "enhanced/renderer/agx.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/passes/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#endif
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

#if defined(OPENRAC_VULKAN)
// The same curve keeps identity/alpha exact; shader-stage UNORM8 rounding differs by at most one.
void test_direct_colour() {
    using namespace openrac;
    using namespace openrac::gl;
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return;
    }
    std::string error;
    renderer::Shader pattern;
    CHECK(pattern.build(
        "AgX spatial pattern",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
layout(location=0) out vec4 result;
void main(){uvec2 p=uvec2(gl_FragCoord.xy);result=p.y==0u?vec4(64,128,191,77)/255.:vec4(uvec4(p.x*17u+p.y*3u,p.y*31u,p.x*7u+p.y*19u,p.x+p.y*23u)&255u)/255.;})",
        error
    ));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    enhanced::AgxPreview agx;
    renderer::FrameBuffer target;
    stats::set_enabled(stats::Client::Dump, true);
    unsigned comparisons = 0;
    for (const auto size : {std::array{67, 39}, std::array{15, 9}, std::array{8, 8}}) {
        CHECK(target.create(size[0], size[1], error, 1, true));
        CHECK(vkgl::allocate_storage_texture(target.colour_texture(), GL_RGBA8, size[0], size[1]));
        const auto texture = target.colour_texture(), depth = target.depth_texture(),
                   framebuffer = target.id();
        for (const float exposure : {-4.8f, -1.f, 0.f, .25f, 1.f, 4.8f}) {
            std::vector<std::uint8_t> reference;
            for (bool direct : {false, true}) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glViewport(0, 0, size[0], size[1]);
                glDisable(GL_SCISSOR_TEST);
                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
                glClearDepth(.5);
                glClear(GL_DEPTH_BUFFER_BIT);
                pattern.use();
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                renderer::render_opt::set(renderer::render_opt::Opt::AgxDirectOutput, direct);
                stats::begin_frame(stats::FrameKind::Tick, comparisons);
                agx.apply(target, exposure);
                stats::begin_frame(stats::FrameKind::Tick, comparisons + 1);
                const auto* measured = stats::history(0);
                CHECK(measured != nullptr);
                if (measured) {
                    auto found = std::find_if(
                        measured->gauges.begin(), measured->gauges.end(), [](const auto& item) {
                            return item.first == "AgX image copies";
                        }
                    );
                    CHECK(found != measured->gauges.end() && found->second == (direct ? 0 : 1));
                }
                auto pixels = target.read_rgba();
                if (!direct) {
                    reference = std::move(pixels);
                } else {
                    for (std::size_t i = 0; i < pixels.size(); ++i) {
                        CHECK(std::abs(int(pixels[i]) - int(reference[i])) <= (i % 4 == 3 ? 0 : 1));
                    }
                    ++comparisons;
                }
                CHECK(
                    target.colour_texture() == texture && target.depth_texture() == depth
                    && target.id() == framebuffer
                );
                CHECK(vkgl::storage_texture(texture));
            }
        }
    }
    renderer::render_opt::reset();
    stats::set_enabled(stats::Client::Dump, false);
    agx.release();
    target.release();
    pattern.release();
    glDeleteVertexArrays(1, &vao);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    std::printf(
        "Compared %u spatial/exposure AgX frames (RGB within one UNORM8 step, alpha exact)\n",
        comparisons
    );
}

namespace {
// Renders two distinct authored colours and preserved depth before comparing synchronization paths.
void test_graph_frame(
    openrac::enhanced::AgxPreview& agx, openrac::renderer::FrameBuffer& target, float exposure
) {
    using namespace openrac;
    using namespace openrac::gl;
    std::vector<std::uint8_t> reference;
    // Repeated graph recording receives fresh pixels on the same output allocation.
    for (bool repeated : {false, true}) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearDepth(.5);
        glClearColor(.25f, .5f, .75f, 77.f / 255.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, target.width() / 2, target.height());
        glClearColor(.75f, .25f, .5f, 177.f / 255.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        agx.apply(target, exposure);
        auto pixels = target.read_rgba();
        // Repeated graph recording must preserve identical shader output.
        if (!repeated) {
            reference = std::move(pixels);
        } else {
            CHECK(!reference.empty());
            CHECK(reference == pixels);
        }
    }
}

// Checks graph/reference identity on recreated, non-storage and multisample targets at bounded exposures.
void test_graph_colour() {
    using namespace openrac;
    using namespace openrac::gl;
    // Only Vulkan executes graph phases; compatibility colour checks still run in main.
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return;
    }
    enhanced::AgxPreview agx;
    renderer::FrameBuffer target;
    std::string error;
    // Recreate shared names across odd sizes, multisample resolve and non-storage fallback cases.
    for (int variant : {0, 1, 2, 3}) {
        const int width = variant == 0 ? 31 : 17;
        const int samples = variant == 2 ? std::min(4, renderer::FrameBuffer::max_samples()) : 1;
        CHECK(target.create(width, 13, error, samples, true));
        // Only the first two cases support the direct path; both remain immutable through recording.
        if (variant < 2) {
            CHECK(vkgl::allocate_storage_texture(target.colour_texture(), GL_RGBA8, width, 13));
        }
        // Both kernel implementations are independently exact across graph/reference execution.
        for (bool direct : {false, true}) {
            renderer::render_opt::set(renderer::render_opt::Opt::AgxDirectOutput, direct);
            // Out-of-range values also check that both paths retain the exposure clamp.
            for (float exposure : {-10.f, -4.8f, 0.f, .25f, 4.8f, 10.f}) {
                test_graph_frame(agx, target, exposure);
            }
        }
    }
    renderer::render_opt::reset();
    CHECK(vkgl::submission_counters().validation_errors == 0);
}
}  // namespace
#endif

int main(int argc, char**) {
    using namespace openrac;
    using namespace openrac::gl;
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::kSkip;
    }
    if (argc > 1) {
        CHECK(enhanced_rendering_available());
    }
    enhanced::AgxPreview agx;
    renderer::FrameBuffer target;
    // Expected bytes come from evaluating the neutral curve before UNORM rounding.
    const std::array<std::array<int, 3>, 6> input{
        {{0, 0, 0}, {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {64, 128, 192}}
    };
    const std::array<std::array<int, 3>, 6> expected{
        {{0, 0, 0}, {127, 127, 127}, {219, 57, 34}, {110, 197, 81}, {30, 84, 211}, {76, 132, 177}}
    };
    const std::array<std::array<int, 3>, 6> exposed{
        {{0, 0, 0},
         {163, 163, 163},
         {244, 106, 88},
         {151, 219, 131},
         {82, 120, 233},
         {123, 165, 203}}
    };
    for (int variant : {0, 1, 2}) {
        const int size = variant == 0 ? 8 : 13;
        const int samples = variant == 2 ? std::min(4, renderer::FrameBuffer::max_samples()) : 1;
        CHECK(target.create(size, size, error, samples));
#if defined(OPENRAC_VULKAN)
        if (is_vulkan() && enhanced_rendering_available() && samples == 1) {
            CHECK(vkgl::allocate_storage_texture(target.colour_texture(), GL_RGBA8, size, size));
        }
#endif
        for (int ev : {0, 1}) {
            for (std::size_t n = 0; n < input.size(); ++n) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.id());
                glDisable(GL_SCISSOR_TEST);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glClearColor(
                    input[n][0] / 255.f, input[n][1] / 255.f, input[n][2] / 255.f, 77 / 255.f
                );
                glClear(GL_COLOR_BUFFER_BIT);
                agx.apply(target, float(ev));
                const auto pixels = target.read_rgba();
                const auto& want =
                    enhanced_rendering_available() ? (ev ? exposed[n] : expected[n]) : input[n];
                for (std::size_t p = 0; p < pixels.size(); p += 4) {
                    for (std::size_t c = 0; c < 3; ++c) {
                        CHECK(std::abs(int(pixels[p + c]) - want[c]) <= 2);
                    }
                    CHECK(pixels[p + 3] == 77);
                }
            }
        }
    }
#if defined(OPENRAC_VULKAN)
    test_direct_colour();
    test_graph_colour();
#endif
    CHECK(glGetError() == GL_NO_ERROR);
    std::puts("AgX reference colour, exposure, alpha and resize checks passed");
}
