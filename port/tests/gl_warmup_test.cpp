// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL's states warmed up at a level's load (renderer/gl/gl_warmup.h): a program drawn in two blend
// states notes two states, drawing them again notes none, and warm() draws both once without
// touching the game's target or leaving its state changed; the states' file is saved. The cache
// folder is the test's own. Skipped (exit 77) where no OpenGL context can be made, or on the
// Vulkan backend, which makes its pipelines ahead on its own.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "check.h"
#include "common/cache_folder.h"
#include "platform/window.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/gl_warmup.h"
#include "renderer/gl/shader.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac::gl;
namespace gl_warmup = openrac::renderer::gl_warmup;

constexpr GLenum kCurrentProgram = 0x8B8D;
constexpr GLenum kDrawFramebufferBinding = 0x8CA6;

// A triangle over the whole frame, in one colour.
constexpr const char* kVertex = R"(#version 410 core
void main() {
    vec2 corner = vec2(float((gl_VertexID & 1) * 4) - 1.0, float((gl_VertexID & 2) * 2) - 1.0);
    gl_Position = vec4(corner, 0.0, 1.0);
}
)";
constexpr const char* kFragment = R"(#version 410 core
out vec4 colour;
void main() {
    colour = vec4(0.25, 0.5, 0.75, 0.5);
}
)";

void set_variable(const char* name, const std::string& value) {
#if defined(_WIN32)
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

// Draws the frame in the two states: blend off, then on.
void draw_states(
    const openrac::renderer::Shader& shader, const openrac::renderer::FrameBuffer& frame
) {
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glViewport(0, 0, 8, 8);
    shader.use();
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
}

}  // namespace

int main() {
    // Before anything reads the environment: the cache folder is the test's own.
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path()
        / ("openrac_gl_warmup_test_"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    set_variable("XDG_CACHE_HOME", folder.string());
    set_variable("HOME", folder.string());  // macOS keeps caches under HOME, not XDG_CACHE_HOME
    set_variable("OPENRAC_GL_PROGRAM_CACHE", (folder / "programs").string());

    openrac::test::GlContext context;
    std::string why;
    if (!context.open(why) || is_vulkan()) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return openrac::test::kSkip;
    }

    gl_warmup::install(openrac::platform::Window::gl_loader());
    openrac::renderer::Shader shader;
    std::string error;
    CHECK(shader.build("warm-up test", kVertex, kFragment, error));
    openrac::renderer::FrameBuffer frame;
    CHECK(frame.create(8, 8, error));
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Two states drawn: two noted; drawn again: none more.
    draw_states(shader, frame);
    CHECK(gl_warmup::counts().noted == 2);
    draw_states(shader, frame);
    CHECK(gl_warmup::counts().noted == 2);

    // The warm-up draws both, and leaves the frame and the bound state as they were.
    const std::vector<std::uint8_t> before = frame.read_rgba();
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    shader.use();
    gl_warmup::warm();
    CHECK(gl_warmup::counts().warmed == 2);
    GLint program = 0, framebuffer = 0;
    glGetIntegerv(kCurrentProgram, &program);
    glGetIntegerv(kDrawFramebufferBinding, &framebuffer);
    CHECK(static_cast<GLuint>(program) == shader.id());
    CHECK(static_cast<GLuint>(framebuffer) == frame.id());
    CHECK(frame.read_rgba() == before);

    // A warm-up draw is not noted as a state of its own.
    draw_states(shader, frame);
    CHECK(gl_warmup::counts().noted == 2);

    // The file is saved on a thread of its own: there within a few seconds.
    const std::filesystem::path states = openrac::cache_folder() / "gl_states";
    bool saved = false;
    for (int n = 0; n < 50 && !saved; ++n) {
        std::error_code ec;
        saved = std::filesystem::exists(states, ec) && !std::filesystem::is_empty(states, ec);
        if (!saved) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    CHECK(saved);

    glDeleteVertexArrays(1, &vao);
    frame.release();
    shader.release();
    std::error_code ec;
    std::filesystem::remove_all(folder, ec);
    return openrac::test::result();
}
