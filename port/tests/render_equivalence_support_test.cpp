// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks program retention (a test's, and the game's kept programs) and exact atlas capture against
// fresh synchronous draws.

#include <algorithm>
#include <string>
#include <vector>

#include "common/stats.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/render_frame_batch.h"
#include "tests/renderer_gl_context.h"

namespace {

namespace gl = openrac::gl;
namespace vk = openrac::vkgl;
namespace test = openrac::test;
namespace renderer = openrac::renderer;

// Full-frame geometry avoids texture or scene dependencies in the lifetime regression.
constexpr auto kVertex = R"(#version 410 core
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
})";
// An inset triangle creates fractional coverage inside the target rather than only clear pixels.
constexpr auto kMsaaVertex = R"(#version 410 core
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 0.7 - 0.8, 0.0, 1.0);
})";
// Vertical detail exposes incorrectly flipped atlas frames as well as stale uniforms.
constexpr auto kFragment = R"(#version 410 core
uniform vec4 tint;
out vec4 colour;
void main() { colour = vec4(tint.rgb + vec3(fract(gl_FragCoord.y / 5.0)), tint.a); }
)";

// Draws a newly linked GL program, optionally leaving its uniforms at GL's required zero defaults.
// `own_stripes` gives the kept-programs test shaders no other test has left in the pool.
std::vector<std::uint8_t> fresh_draw(
    bool set_tint, bool different_source = false, bool own_stripes = false
) {
    renderer::FrameBuffer frame;
    renderer::Shader shader;
    std::string error;
    CHECK(frame.create(16, 12, error));
    std::string source(kFragment);
    if (different_source) {
        source.replace(source.find("tint.rgb"), 8, "(tint.rgb * 0.5)");
    }
    if (own_stripes) {
        source.replace(source.find("5.0"), 3, "7.0");
    }
    CHECK(shader.build("retained program regression", kVertex, source, error));
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, frame.id());
    gl::glViewport(0, 0, 16, 12);
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glDisable(gl::GL_CULL_FACE);
    gl::glDisable(gl::GL_BLEND);
    gl::glDisable(gl::GL_SCISSOR_TEST);
    shader.use();
    if (set_tint) {
        gl::glUniform4f(shader.uniform("tint"), .1f, .2f, .3f, 1);
    }
    gl::GLuint vao = 0;
    gl::glGenVertexArrays(1, &vao);
    gl::glBindVertexArray(vao);
    gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
    const auto pixels = frame.read_rgba();
    gl::glDeleteVertexArrays(1, &vao);
    shader.release();
    frame.release();
    return pixels;
}

// Reused driver objects preserve pixels but never the old GL program's uniform values.
void test_fresh_uniforms_and_pipeline_reuse() {
    vk::test_reuse_programs(false);
    const auto tinted = fresh_draw(true);
    const auto zero = fresh_draw(false);
    const auto changed = fresh_draw(true, true);
    CHECK(changed != tinted);
    CHECK(tinted != zero);
    vk::test_reuse_programs(true);
    CHECK(fresh_draw(true) == tinted);
    vk::test_reuse_programs(true, false);
    const auto oracle = vk::submission_counters();
    CHECK(fresh_draw(true) == tinted);
    CHECK(vk::submission_counters().test_program_reuses == oracle.test_program_reuses);
    CHECK(vk::submission_counters().pipeline_creation_calls > oracle.pipeline_creation_calls);
    vk::test_reuse_programs(true);
    const auto before = vk::submission_counters();
    CHECK(fresh_draw(false) == zero);
    const auto after = vk::submission_counters();
    CHECK(after.test_program_reuses == before.test_program_reuses + 1);
    CHECK(after.pipeline_creation_calls == before.pipeline_creation_calls);
    const auto other = vk::submission_counters();
    CHECK(fresh_draw(true, true) == changed);
    CHECK(vk::submission_counters().test_program_reuses == other.test_program_reuses);
    CHECK(vk::submission_counters().pipeline_creation_calls > other.pipeline_creation_calls);
    vk::test_reuse_programs(false);
    const auto cold = vk::submission_counters();
    CHECK(fresh_draw(false) == zero);
    CHECK(vk::submission_counters().test_program_reuses == cold.test_program_reuses);
    CHECK(vk::submission_counters().pipeline_creation_calls > cold.pipeline_creation_calls);
}

// Identical bytecode cannot reuse a layout with different dynamic-uniform descriptor types.
void test_descriptor_mode_isolation() {
    auto& probes = openrac::stats::probes();
    const auto found = std::find_if(probes.begin(), probes.end(), [](const auto* probe) {
        return std::string_view(probe->variable) == "OPENRAC_VK_UNIFORM_REUSE_OFF";
    });
    CHECK(found != probes.end());
    if (found == probes.end()) {
        return;
    }
    auto& off = (*found)->on;
    const bool previous = off;
    off = false;
    vk::test_reuse_programs(true);
    const auto expected = fresh_draw(true);
    off = true;
    const auto before = vk::submission_counters();
    CHECK(fresh_draw(true) == expected);
    const auto changed = vk::submission_counters();
    CHECK(changed.test_program_reuses == before.test_program_reuses);
    CHECK(changed.pipeline_creation_calls > before.pipeline_creation_calls);
    off = false;
    CHECK(fresh_draw(true) == expected);
    CHECK(vk::submission_counters().test_program_reuses == changed.test_program_reuses + 1);
    off = previous;
    vk::test_reuse_programs(false);
}

// The game's kept programs (vk-program-keep): a deleted program's pipelines serve the next link of
// the same shaders, with the same pixels and none of its uniform values.
void test_kept_programs() {
    using renderer::render_opt::Opt;
    vk::test_reuse_programs(false);
    // Whole pipelines first: each is made by its draw, so the driver's calls can be counted.
    renderer::render_opt::set(Opt::VkPipelineLibrary, false);
    const auto tinted = fresh_draw(true, false, true);
    const auto zero = fresh_draw(false, false, true);
    const auto changed = fresh_draw(true, true, true);
    vk::keep_programs(true);
    // Nothing is kept yet: this program is the first one deleted while keeping is on.
    const auto first = vk::submission_counters();
    CHECK(fresh_draw(true, false, true) == tinted);
    CHECK(vk::submission_counters().program_reuses == first.program_reuses);
    const auto before = vk::submission_counters();
    CHECK(fresh_draw(false, false, true) == zero);
    const auto after = vk::submission_counters();
    CHECK(after.program_reuses == before.program_reuses + 1);
    CHECK(after.pipeline_creation_calls == before.pipeline_creation_calls);
    CHECK(after.test_program_reuses == before.test_program_reuses);
    // Other shaders take nothing.
    CHECK(fresh_draw(true, true, true) == changed);
    CHECK(vk::submission_counters().program_reuses == after.program_reuses);
    // Switched off, a link is fresh and what is kept waits.
    renderer::render_opt::set(Opt::VkProgramKeep, false);
    const auto off = vk::submission_counters();
    CHECK(fresh_draw(true, false, true) == tinted);
    CHECK(vk::submission_counters().program_reuses == off.program_reuses);
    CHECK(vk::submission_counters().pipeline_creation_calls > off.pipeline_creation_calls);
    renderer::render_opt::set(Opt::VkProgramKeep, true);
    CHECK(fresh_draw(true, false, true) == tinted);
    CHECK(vk::submission_counters().program_reuses == off.program_reuses + 1);
    // Linked from parts (where the device offers it): the parts and the linked pipeline are kept.
    renderer::render_opt::set(Opt::VkPipelineLibrary, true);
    CHECK(fresh_draw(true, true, true) == changed);
    const auto linked = vk::submission_counters();
    CHECK(fresh_draw(true, true, true) == changed);
    CHECK(vk::submission_counters().program_reuses == linked.program_reuses + 1);
    CHECK(vk::submission_counters().pipelines_linked == linked.pipelines_linked);
    vk::wait_for_pipeline_prewarm();
    CHECK(fresh_draw(true, true, true) == changed);
    vk::keep_programs(false);
    const auto cold = vk::submission_counters();
    CHECK(fresh_draw(false, false, true) == zero);
    CHECK(vk::submission_counters().program_reuses == cold.program_reuses);
    renderer::render_opt::reset();
}

// Preserves every channel and row when snapshots cross presents and framebuffer replacement.
void test_batch_matches_synchronous() {
    renderer::FrameBuffer frame;
    test::RenderFrameBatch batch;
    std::string error;
    constexpr int kFrames = 8;
    CHECK(batch.create(16, 12, kFrames, error));
    CHECK(!batch.create(16, 0, kFrames, error));
    CHECK(!batch.capture(0, -1));
    CHECK(!batch.capture(0, kFrames));
    std::vector<std::vector<std::uint8_t>> reference;
    for (int index = 0; index < kFrames; ++index) {
        CHECK(frame.create(16, 12, error));
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, frame.id());
        gl::glDisable(gl::GL_SCISSOR_TEST);
        gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
        gl::glClearColor(static_cast<float>(index) / static_cast<float>(kFrames), 0, 1, .5f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        gl::glEnable(gl::GL_SCISSOR_TEST);
        gl::glScissor(1, 2, 5, 3);
        gl::glClearColor(1, .25f, 0, 1);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        reference.push_back(frame.read_rgba());
        CHECK(batch.capture(frame.id(), index));
        vk::present();
    }
    CHECK(batch.read() == reference);
    // Reusing storage must replace every previous scenario's pixels.
    CHECK(batch.create(16, 12, kFrames, error));
    reference.clear();
    for (int index = 0; index < kFrames; ++index) {
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, frame.id());
        gl::glDisable(gl::GL_SCISSOR_TEST);
        gl::glClearColor(0, static_cast<float>(index) / static_cast<float>(kFrames), 0, 1);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        reference.push_back(frame.read_rgba());
        CHECK(batch.capture(frame.id(), index));
        vk::present();
    }
    CHECK(batch.read() == reference);
    frame.release();
}

// Compares real multisampled output instead of toggling resolve on a single-sample scene.
void test_multisample_resolve() {
    using renderer::render_opt::Opt;
    std::vector<std::uint8_t> reference;
    for (bool in_pass : {false, true}) {
        renderer::render_opt::set(Opt::MsaaPassResolve, in_pass);
        renderer::FrameBuffer frame;
        renderer::Shader shader;
        std::string error;
        CHECK(shader.build("MSAA equivalence edges", kMsaaVertex, kFragment, error));
        CHECK(frame.create(16, 12, error, 4));
        CHECK(frame.samples() == 4);
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, frame.id());
        gl::glDisable(gl::GL_SCISSOR_TEST);
        gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
        gl::glClearColor(0, 0, 0, 1);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        gl::glViewport(0, 0, 16, 12);
        gl::glDisable(gl::GL_DEPTH_TEST);
        gl::glDisable(gl::GL_STENCIL_TEST);
        gl::glDisable(gl::GL_CULL_FACE);
        gl::glDisable(gl::GL_BLEND);
        gl::GLuint vao = 0;
        gl::glGenVertexArrays(1, &vao);
        gl::glBindVertexArray(vao);
        shader.use();
        gl::glUniform4f(shader.uniform("tint"), 1, 1, 1, 1);
        gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
        const auto pixels = frame.read_rgba();
        // Black/white geometry must contain genuinely resolved fractional coverage.
        CHECK(std::any_of(pixels.begin(), pixels.end(), [](auto value) {
            return value > 0 && value < 255;
        }));
        CHECK(!in_pass || pixels == reference);
        // Keep the independently resolved image for the following in-pass comparison.
        if (!in_pass) {
            reference = pixels;
        }
        gl::glDeleteVertexArrays(1, &vao);
        shader.release();
        frame.release();
    }
    renderer::render_opt::reset();
}

}  // namespace

// Opens only the Vulkan test context; a player's launch keeps programs through keep_programs only.
int main() {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !vk::active()) {
        return test::kSkip;
    }
    test_fresh_uniforms_and_pipeline_reuse();
    test_descriptor_mode_isolation();
    test_kept_programs();
    test_batch_matches_synchronous();
    test_multisample_resolve();
    CHECK(vk::submission_counters().validation_errors == 0);
    CHECK(gl::glGetError() == gl::GL_NO_ERROR);
    return test::result();
}
