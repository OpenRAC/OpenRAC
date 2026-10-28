// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Check Tracy timestamps through deferred draws, readback and presentation with F3 off.
#include <chrono>
#include <string>
#include <thread>

#include "common/profiler.h"
#include "common/stats.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
// A full-screen triangle exercises the deferred secondary render-pass path.
constexpr auto kVertex = R"(#version 410 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
})";

// A uniform also exercises descriptor creation while GPU scopes are active.
constexpr auto kFragment = R"(#version 410 core
uniform vec4 colour;
out vec4 result;
void main() { result = colour; }
)";

// The fixture keeps enough draws for repeated submission and query reuse.
constexpr unsigned kFrames = 12;
}  // namespace

int main(int argc, char**) {
    using namespace openrac;
    profiling::thread_name("Tracy test");
    if (argc > 1 && !profiling::wait_for_connection(std::chrono::seconds(15))) {
        return 1;
    }
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !gl::is_vulkan()) {
        return test::kSkip;
    }
    CHECK(!stats::enabled());
    renderer::FrameBuffer target;
    const bool made = target.create(16, 16, error);
    CHECK(made);
    // A failed setup must not lead to empty pixel reads.
    if (!made) {
        return test::result();
    }
    renderer::Shader shader;
    const bool built = shader.build("Tracy draw", kVertex, kFragment, error);
    CHECK(built);
    // Keep shader failures separate from command-buffer lifetime checks.
    if (!built) {
        return test::result();
    }
    unsigned vao = 0;
    gl::glGenVertexArrays(1, &vao);
    gl::glBindVertexArray(vao);
    gl::glViewport(0, 0, 16, 16);
    shader.use();
    gl::glUniform4f(shader.uniform("colour"), .25f, .5f, .75f, 1);
    {
        const stats::Scope outer("Tracy readback span", stats::kGpu);
        for (unsigned i = 0; i < kFrames; ++i) {
            {
                const stats::Scope gpu("Tracy draw", stats::kGpu);
                gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, target.id());
                gl::glClearColor(0, 0, 0, 1);
                gl::glClear(gl::GL_COLOR_BUFFER_BIT);
                gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
                // End a nested zone while a deferred pass is active, then resume drawing.
                {
                    const stats::Scope nested("Tracy nested draw", stats::kGpu);
                    gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
                }
                gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
            }
            const auto pixels = target.read_rgba();
            CHECK(pixels.size() == 16 * 16 * 4);
            // Avoid indexing an empty readback after a failed render.
            if (pixels.size() >= 4) {
                CHECK(pixels[0] >= 63 && pixels[0] <= 64);
                CHECK(pixels[1] >= 127 && pixels[1] <= 128);
                CHECK(pixels[2] >= 191 && pixels[2] <= 192);
            }
            {
                const profiling::Zone wait("Tracy idle");
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            profiling::frame();
        }
        // Rotate to another frame slot before the outer scope ends.
        context.window().swap();
    }
    gl::glFinish();
    CHECK(!stats::enabled());
    CHECK(vkgl::submission_counters().validation_errors == 0);
    CHECK(gl::glGetError() == gl::GL_NO_ERROR);
    gl::glDeleteVertexArrays(1, &vao);
    target.release();
    return test::result();
}
