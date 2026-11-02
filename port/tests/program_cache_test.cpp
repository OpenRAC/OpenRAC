// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL's program binaries (renderer/program_cache.h): a program built again from the same
// sources is loaded from the binary kept in memory, a later run's from the binary's file, and it
// draws the pixels the compiled program draws; a damaged file is compiled again and kept over.
// The cache is a folder of the test's own. Skipped (exit 77) where no context can be made, or the
// context gives no program binaries.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/program_cache.h"
#include "renderer/shader.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac::gl;
using openrac::renderer::Shader;
namespace program_cache = openrac::renderer::program_cache;

// A triangle over the whole frame, coloured by the pixel's place and a uniform.
constexpr const char* kVertex = R"(#version 410 core
void main() {
    vec2 corner = vec2(float((gl_VertexID & 1) * 4) - 1.0, float((gl_VertexID & 2) * 2) - 1.0);
    gl_Position = vec4(corner, 0.0, 1.0);
}
)";
constexpr const char* kFragment = R"(#version 410 core
uniform vec4 tint;
uniform sampler2D tex_T0;
out vec4 colour;
void main() {
    colour = vec4(fract(gl_FragCoord.xy / 7.0), tint.z, 1.0) * tint.w;
}
)";

std::vector<std::uint8_t> draw(const Shader& shader) {
    openrac::renderer::FrameBuffer frame;
    std::string error;
    if (!frame.create(16, 16, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return {};
    }
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glViewport(0, 0, 16, 16);
    shader.use();
    glUniform4f(shader.uniform("tint"), 0.0f, 0.0f, 0.25f, 0.75f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::vector<std::uint8_t> pixels = frame.read_rgba();
    glDeleteVertexArrays(1, &vao);
    frame.release();
    return pixels;
}

// Builds the test's program and draws with it.
std::vector<std::uint8_t> build_and_draw(const char* fragment = kFragment) {
    Shader shader;
    std::string error;
    const bool ok = shader.build("cache test", kVertex, fragment, error);
    if (!ok) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(ok);
    std::vector<std::uint8_t> pixels = ok ? draw(shader) : std::vector<std::uint8_t>{};
    shader.release();
    return pixels;
}

void set_cache_folder(const std::filesystem::path& folder) {
#if defined(_WIN32)
    _putenv_s("OPENRAC_GL_PROGRAM_CACHE", folder.string().c_str());
#else
    setenv("OPENRAC_GL_PROGRAM_CACHE", folder.string().c_str(), 1);
#endif
}

}  // namespace

int main() {
    // Before anything reads the environment (it is read once).
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path()
        / ("openrac_program_cache_test_"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    set_cache_folder(folder);

    openrac::test::GlContext context;
    std::string why;
    if (!context.open(why)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return openrac::test::kSkip;
    }
    if (!program_cache::enabled()) {
        std::printf("SKIP: this context keeps no program binaries\n");
        return openrac::test::kSkip;
    }

    // Compiled, and kept.
    const std::vector<std::uint8_t> compiled = build_and_draw();
    CHECK(compiled.size() == 16u * 16u * 4u);
    CHECK(program_cache::counts().compiled == 1 && program_cache::counts().loaded == 0);

    // Built again in this run: from the binary in memory, drawing the same pixels.
    CHECK(build_and_draw() == compiled);
    CHECK(program_cache::counts().compiled == 1 && program_cache::counts().loaded == 1);

    // A later run (nothing in memory): from the binary's file.
    program_cache::reset();
    CHECK(build_and_draw() == compiled);
    CHECK(program_cache::counts().loaded == 1 && program_cache::counts().compiled == 0);

    // Other sources are another program: compiled.
    std::string other(kFragment);
    other.replace(other.find("7.0"), 3, "5.0");
    const std::vector<std::uint8_t> different = build_and_draw(other.c_str());
    CHECK(different.size() == compiled.size() && different != compiled);
    CHECK(program_cache::counts().compiled == 1);

    // A damaged file is not taken: compiled again, and the file kept over it is whole.
    int damaged = 0;
    for (const auto& file : std::filesystem::recursive_directory_iterator(folder)) {
        if (file.is_regular_file()) {
            std::fstream f(file.path(), std::ios::in | std::ios::out | std::ios::binary);
            f.seekp(-1, std::ios::end);
            f.put('\x5A');
            ++damaged;
        }
    }
    CHECK(damaged == 2);
    program_cache::reset();
    CHECK(build_and_draw() == compiled);
    CHECK(program_cache::counts().compiled == 1 && program_cache::counts().loaded == 0);
    program_cache::reset();
    CHECK(build_and_draw() == compiled);
    CHECK(program_cache::counts().loaded == 1);
    CHECK(glGetError() == GL_NO_ERROR);

    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    return openrac::test::result();
}
