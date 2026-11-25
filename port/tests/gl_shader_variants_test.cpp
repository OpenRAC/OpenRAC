// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL and ES exact feature variants: constants disappear from driver uniform reflection,
// inactive runtime uniforms and blocks survive switches, and observed combinations warm again.
// Expected pixels follow the fixture's arithmetic, with no game assets or reference captures.

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>

#include "common/cache_folder.h"
#include "platform/window.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/gl_warmup.h"
#include "renderer/gl/program_cache.h"
#include "renderer/gl/shader.h"
#include "renderer/gl/shader_variants.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {

using namespace openrac::gl;
namespace renderer = openrac::renderer;
namespace variants = renderer::shader_variants;

// Zero, six ordinary feature values and both signed extremes exercise the exact variant key.
constexpr std::array<int, 9> kFeatureValues =
    {0, 1, 2, 3, 4, 5, 6, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
using renderer::world::gl::GL_UNIFORM_BUFFER;

// A fullscreen triangle; the matrix is inactive until a nonzero side variant needs it.
constexpr const char* kVertex = R"(#version 410 core
// openrac_mode: side
uniform int side;
uniform mat4 transform[2];
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
    if (side != 0) {
        gl_Position = transform[1] * gl_Position;
    }
}
)";

// Runtime colour, gain and the block are only used by nonzero feature combinations.
constexpr const char* kFragment = R"(#version 410 core
// openrac_mode: tint
uniform int tint;
uniform vec4 colours[3];
uniform float gain;
layout(std140) uniform Tuning { vec4 tuning; };
layout(location=0) out vec4 result;
void main() {
    result = tint == 0 ? vec4(0, 0, 1, 1)
                      : vec4(colours[1].r * colours[2].r * gain, tuning.g, float(tint & 7) / 7.0, 1);
}
)";

// Sets an isolated cache before the environment snapshot is read.
void set_cache(const std::filesystem::path& path) {
#if defined(_WIN32)
    _putenv_s("OPENRAC_GL_PROGRAM_CACHE", path.string().c_str());
    _putenv_s("XDG_CACHE_HOME", path.string().c_str());
#else
    setenv("OPENRAC_GL_PROGRAM_CACHE", path.string().c_str(), 1);
    setenv("XDG_CACHE_HOME", path.string().c_str(), 1);
#if defined(__APPLE__)
    setenv("HOME", path.string().c_str(), 1);
#endif
#endif
}

// Writes overlapping vector and matrix arrays before the exact variant that reads their tails.
void set_runtime_values(const renderer::Shader& shader) {
    constexpr std::array<float, 32> kTransforms = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    };
    constexpr std::array<float, 16> kZeroMatrix{};
    constexpr std::array<float, 12> kColours = {0, 0, 0, 1, 0.25f, 0, 0, 1, 1, 0, 0, 1};
    const int transform = shader.uniform("transform");
    const int colours = shader.uniform("colours");
    glUniformMatrix4fv(transform, 2, GL_FALSE, kTransforms.data());
    glUniformMatrix4fv(transform, 1, GL_FALSE, kZeroMatrix.data());
    glUniform4fv(colours, 3, kColours.data());
    glUniform4f(shader.uniform("colours[1]"), 1, 0, 0, 1);
    glUniform4f(colours, 0, 1, 0, 1);
    glUniform1f(shader.uniform("gain"), 128.0f / 255.0f);
}

// Tests exact combinations, including values beyond four choices and both signed extremes.
void test_exact_variants(renderer::Shader& shader, renderer::FrameBuffer& target) {
    constexpr GLenum kCurrentProgram = 0x8B8D;
    const auto native_location = reinterpret_cast<PFN_glGetUniformLocation>(
        openrac::platform::Window::gl_loader()("glGetUniformLocation")
    );
    const auto& values = kFeatureValues;
    const auto before = variants::counts();
    const int side = shader.uniform("side");
    const int tint = shader.uniform("tint");
    CHECK(side >= 0 && tint >= 0);
    CHECK(shader.uniform("missing_uniform") == -1);
    CHECK(native_location(shader.id(), "side") == -1);

    // Repeated passes must reuse exact executables and preserve runtime values without rewriting them.
    for (unsigned pass = 0; pass < 2; ++pass) {
        for (const int value : values) {
            glUniform1i(side, value);
            glUniform1i(tint, value);
            renderer::world::gl::glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 1);
            const auto pixels = target.read_rgba();
            // A readback failure must report a failed check rather than index an empty result.
            if (pixels.size() != 4) {
                CHECK(pixels.size() == 4);
                return;
            }
            CHECK(pixels[0] == (value == 0 ? 0 : 128));
            CHECK(pixels[1] == (value == 0 ? 0 : 64));
            CHECK(pixels[2] == (value == 0 ? 255 : ((value & 7) * 255 + 3) / 7));
            CHECK(pixels[3] == 255);
            GLint native = 0;
            glGetIntegerv(kCurrentProgram, &native);
            CHECK(native_location(static_cast<GLuint>(native), "tint") == -1);
            CHECK(shader.uniform("side") == side && shader.uniform("tint") == tint);
        }

        CHECK(variants::counts().selected - before.selected == values.size() - 1);
    }

    CHECK(variants::counts().failed == 0);
}

// Tests warm-up's native program replay without changing the logical feature or uniform state.
void test_driver_state_replay(renderer::Shader& shader, renderer::FrameBuffer& target) {
    const auto pixels = target.read_rgba();
    renderer::gl_warmup::warm();
    shader.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    CHECK(target.read_rgba() == pixels);
    CHECK(glGetError() == GL_NO_ERROR);
}

// Tests source-keyed exact combinations loading before the next Shader's first draw.
void test_observed_variants_are_prewarmed(renderer::Shader& shader) {
    const auto before = variants::counts();
    const auto binaries = renderer::program_cache::counts();
    shader.release();
    std::string error;
    CHECK(shader.build("exact feature fixture", kVertex, kFragment, error));

    // Drivers without program binaries still select exact variants synchronously.
    if (renderer::program_cache::enabled()) {
        CHECK(variants::counts().selected - before.selected == kFeatureValues.size() - 1);
        CHECK(
            renderer::program_cache::counts().loaded - binaries.loaded
            >= static_cast<int>(kFeatureValues.size())
        );
    }
}

// Driver queries in the wrapped draw observe the state at execution, independently of replay code.
struct WarmupObservation {
    PFN_glDrawArrays next = nullptr;  // Draw hook installed before the variant layer.
    bool inspect = false;             // Enabled only for the warm-up under test.
    GLuint program = 0;               // Rebuilt native handle observed in the warm-up draw.
    int draws = 0;                    // Draws of the fixture's nonzero variant.
};

// The GL callback has no user pointer; the fixture owns this single-threaded observation.
WarmupObservation& warmup_observation() {
    static WarmupObservation observation;
    return observation;
}

// Reads the fixture's native assignments without going through logical uniform locations.
std::array<GLint, 2> native_warmup_bindings(GLuint program) {
    constexpr GLenum kUniformBlockBinding = 0x8A3F;
    const auto get = openrac::platform::Window::gl_loader();
    using UniformQuery = void(OPENRAC_GLAPI*)(GLuint, GLint, GLint*);
    using BlockQuery = void(OPENRAC_GLAPI*)(GLuint, GLuint, GLenum, GLint*);
    const auto location = reinterpret_cast<PFN_glGetUniformLocation>(get("glGetUniformLocation"));
    const auto index = reinterpret_cast<PFN_glGetUniformBlockIndex>(get("glGetUniformBlockIndex"));
    const auto uniform = reinterpret_cast<UniformQuery>(get("glGetUniformiv"));
    const auto block = reinterpret_cast<BlockQuery>(get("glGetActiveUniformBlockiv"));
    const GLint sampler = location(program, "warmup_sampler");
    std::array<GLint, 2> bindings = {-1, -1};

    // Other warm-up draws in the same context do not declare the fixture's sampler.
    if (sampler >= 0) {
        uniform(program, sampler, &bindings[0]);
        const GLuint native = index(program, "WarmupTuning");
        CHECK(native != renderer::world::gl::GL_INVALID_INDEX);

        if (native != renderer::world::gl::GL_INVALID_INDEX) {
            block(program, native, kUniformBlockBinding, &bindings[1]);
        }
    }

    return bindings;
}

// Checks the native sampler and block assignments at the cached variant's draw.
void OPENRAC_GLAPI observe_warmup_draw(GLenum mode, GLint first, GLsizei count) {
    auto& observation = warmup_observation();

    if (observation.inspect) {
        constexpr GLenum kCurrentProgram = 0x8B8D;
        GLint current = 0;
        glGetIntegerv(kCurrentProgram, &current);
        const auto program = static_cast<GLuint>(current);
        const auto [unit, binding] = native_warmup_bindings(program);

        if (unit >= 0) {
            CHECK(unit == 3);
            CHECK(binding == 7);
            observation.program = program;
            ++observation.draws;
        }
    }

    observation.next(mode, first, count);
}

// Tests bindings on a freshly rebuilt variant before its first live draw.
void test_cached_variant_warmup_bindings(const renderer::FrameBuffer& target, GLuint vao) {
    constexpr auto kWarmupFragment = R"(#version 410 core
// openrac_mode: warmup_mode
uniform int warmup_mode;
uniform sampler2D warmup_sampler[2];
layout(std140) uniform WarmupTuning { vec4 tuning; };
layout(location=0) out vec4 result;
void main() {
    result = warmup_mode == 0 ? vec4(1) : texture(warmup_sampler[0], vec2(0.5))
                                      + texture(warmup_sampler[1], vec2(0.5)) + tuning;
}
)";
    // Without a binary cache there are no rebuilt variants to warm before the first draw.
    if (!renderer::program_cache::enabled()) {
        return;
    }

    renderer::Shader fixture;
    std::string error;

    // The first build records a nonzero feature combination and the draw state it used.
    for (const bool rebuilt : {false, true}) {
        if (!fixture.build("warm-up bindings", kVertex, kWarmupFragment, error)) {
            CHECK(false);
            return;
        }

        fixture.use();

        // A newly loaded level has not issued its first sampler or block bindings yet.
        if (!rebuilt) {
            // GL aliases the base name and [0]. The newer write uses the name queried first.
            const GLint first_element = fixture.uniform("warmup_sampler[0]");
            glUniform1i(fixture.uniform("warmup_sampler"), 2);
            glUniform1i(first_element, 3);
            const auto block = glGetUniformBlockIndex(fixture.id(), "WarmupTuning");
            glUniformBlockBinding(fixture.id(), block, 7);
            glBindFramebuffer(GL_FRAMEBUFFER, target.id());
            glBindVertexArray(vao);
            glUniform1i(fixture.uniform("warmup_mode"), 1);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    auto& observation = warmup_observation();
    observation.inspect = true;
    renderer::gl_warmup::warm();
    observation.inspect = false;
    CHECK(observation.draws != 0);

    // The fresh native program keeps its link defaults after the temporary replay.
    if (observation.program != 0) {
        const auto [unit, binding] = native_warmup_bindings(observation.program);
        CHECK(unit == 0);
        CHECK(binding == 0);
    }

    CHECK(glGetError() == GL_NO_ERROR);
    fixture.release();
}

// Tests that a refused prewarm leaves the initial shader usable and the exact variant retryable.
void test_rejected_prewarm(const renderer::FrameBuffer& target) {
    // The test needs an observed combination to replay on the next build.
    if (!renderer::program_cache::enabled()) {
        return;
    }

    constexpr auto kSource = "// failure fixture\nuniform int choice;\n";
    constexpr auto kVertexSource = R"(#version 410 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
}
)";
    constexpr auto kRed = "#version 410 core\nout vec4 result;\n"
                          "void main() { result = vec4(1, 0, 0, 1); }\n";
    constexpr auto kGreen = "#version 410 core\nout vec4 result;\n"
                            "void main() { result = vec4(0, 1, 0, 1); }\n";
    const std::vector<std::uint8_t> red = {255, 0, 0, 255};
    const std::vector<std::uint8_t> green = {0, 255, 0, 255};
    const std::vector<std::uint8_t> blue = {0, 0, 255, 255};
    std::string error;
    renderer::Shader zero;
    unsigned attempts = 0;
    GLuint exact = 0;

    // Compiles use already linked test programs so the refusal is independent of driver behaviour.
    const auto compile = [&](const std::vector<int>& values, std::string& why) {
        ++attempts;

        if (values.front() != 1 || attempts == 2) {
            why = "injected exact variant refusal";
            return GLuint{0};
        }

        return exact;
    };

    for (const bool rebuilt : {false, true}) {
        renderer::Shader one;

        if (!one.build("exact green", kVertexSource, kGreen, error)
            || !zero.build("exact red", kVertexSource, kRed, error)) {
            CHECK(false);
            one.release();
            zero.release();
            return;
        }

        // Registration owns the returned exact handle after the callback succeeds.
        exact = one.id();
        variants::register_program(zero.id(), kSource, {"choice"}, compile);
        zero.use();
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glDrawArrays(GL_TRIANGLES, 0, 3);
        CHECK(target.read_rgba() == red);
        glUniform1i(zero.uniform("choice"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        CHECK(target.read_rgba() == green);

        // A refused draw leaves the existing pixel intact instead of drawing any other variant.
        if (rebuilt) {
            glClearColor(0, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(zero.uniform("choice"), 2);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            CHECK(target.read_rgba() == blue);
        }

        zero.release();
    }

    CHECK(attempts == 4);
    CHECK(glGetError() == GL_NO_ERROR);
}

// Tests exact compute feature selection and unsigned runtime values on compute-capable contexts.
void test_compute_variants() {
    constexpr GLenum kMajorVersion = 0x821B;
    constexpr GLenum kMinorVersion = 0x821C;
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(kMajorVersion, &major);
    glGetIntegerv(kMinorVersion, &minor);

    // macOS OpenGL 4.1 has no compute shaders; ES 3.2 and desktop 4.3 do.
    if (!glDispatchCompute || (!is_es() && (major < 4 || (major == 4 && minor < 3)))) {
        return;
    }

    using Map = void*(OPENRAC_GLAPI*)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    using Unmap = GLboolean(OPENRAC_GLAPI*)(GLenum);
    const auto get = openrac::platform::Window::gl_loader();
    const auto map = reinterpret_cast<Map>(get("glMapBufferRange"));
    const auto unmap = reinterpret_cast<Unmap>(get("glUnmapBuffer"));
    CHECK(map && unmap);
    renderer::Shader shader;
    std::string error;
    CHECK(shader.build_compute(
        "compute feature fixture",
        R"(#version 430 core
// openrac_mode: choice
uniform int choice;
uniform uint seed;
layout(local_size_x=1) in;
layout(std430, binding=0) buffer Output { uint result; };
void main() { result = seed + uint(choice); }
)",
        error
    ));
    constexpr GLenum kStorageBuffer = 0x90D2;
    constexpr GLbitfield kCpuReadBarrier = 0x2200;  // Shader storage and buffer-update visibility.
    constexpr GLbitfield kMapRead = 1;
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(kStorageBuffer, buffer);
    glBufferData(kStorageBuffer, sizeof(unsigned), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(kStorageBuffer, 0, buffer);
    shader.use();
    renderer::world::gl::glUniform1ui(shader.uniform("seed"), 41);  // Runtime data, not a feature.

    // Values beyond the former four-value limit must each dispatch the matching exact program.
    for (int choice = 0; choice < 7; ++choice) {
        glUniform1i(shader.uniform("choice"), choice);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(kCpuReadBarrier);
        const auto* value =
            static_cast<const unsigned*>(map(kStorageBuffer, 0, sizeof(unsigned), kMapRead));
        CHECK(value != nullptr);

        if (value) {
            CHECK(*value == 41u + static_cast<unsigned>(choice));
            CHECK(unmap(kStorageBuffer) == GL_TRUE);
        }
    }

    CHECK(glGetError() == GL_NO_ERROR);
    shader.release();
    glDeleteBuffers(1, &buffer);
}

}  // namespace

int main() {
    const auto folder =
        std::filesystem::temp_directory_path()
        / ("openrac_gl_variants_"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    set_cache(folder);
    openrac::test::GlContext context;
    std::string error;

    // These tests exercise the native OpenGL/ES implementation, not Vulkan's separate registry.
    if (!context.open(error) || is_vulkan()) {
        std::printf("SKIP: native OpenGL unavailable: %s\n", error.c_str());
        return openrac::test::kSkip;
    }

    CHECK(renderer::world::gl::load(openrac::platform::Window::gl_loader(), error));
    renderer::gl_warmup::install(openrac::platform::Window::gl_loader());
    warmup_observation().next = glDrawArrays;
    glDrawArrays = observe_warmup_draw;
    renderer::Shader shader;
    CHECK(shader.build("exact feature fixture", kVertex, kFragment, error));
    renderer::FrameBuffer target;
    CHECK(target.create(1, 1, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 1, 1);
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    shader.use();
    set_runtime_values(shader);
    const auto block = glGetUniformBlockIndex(shader.id(), "Tuning");
    CHECK(block != renderer::world::gl::GL_INVALID_INDEX);
    constexpr GLuint kBinding = 7;  // Deliberately differs from the driver's default binding zero.
    glUniformBlockBinding(shader.id(), block, kBinding);
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    constexpr std::array<float, 4> kTuning = {0, 64.0f / 255.0f, 0, 0};
    glBufferData(GL_UNIFORM_BUFFER, sizeof kTuning, kTuning.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kBinding, buffer);
    test_exact_variants(shader, target);
    test_driver_state_replay(shader, target);
    test_observed_variants_are_prewarmed(shader);
    shader.release();
    test_cached_variant_warmup_bindings(target, vao);
    test_rejected_prewarm(target);
    test_compute_variants();
    target.release();
    glDeleteBuffers(1, &buffer);
    glDeleteVertexArrays(1, &vao);
    renderer::gl_warmup::finish_cache_save();
    std::error_code cleanup;
    std::filesystem::remove_all(folder, cleanup);
    return openrac::test::result();
}
