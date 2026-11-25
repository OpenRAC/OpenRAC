// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Package misses render embedded diagnostics, without compiling GLSL even in developer builds.
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "renderer/frame/colour_graph_backend.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"
#include "renderer/ui/backdrop_blur_sources.h"
#include "renderer/vulkan/pipelines/error_shader.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer_shaders.h"
#include "tests/check.h"
#include "tests/graph_fixture.h"
#include "tests/renderer_gl_context.h"

using namespace openrac;
using namespace openrac::gl;

namespace {
// Readback is top row first; samples below use GL's lower-left coordinates.
void pixel(
    const std::vector<std::uint8_t>& pixels, int x, int y, std::array<std::uint8_t, 4> expected
) {
    CHECK(pixels.size() == std::size_t{32} * 32 * 4);

    if (pixels.size() != std::size_t{32} * 32 * 4) {
        return;
    }

    const std::size_t offset = ((std::size_t{31} - y) * 32 + x) * 4;

    for (int c = 0; c < 4; ++c) {
        CHECK(pixels[offset + c] == expected[c]);
    }
}

void test_removed_fragment_resources(
    const renderer::FrameBuffer& target, const std::string& fragment
) {
    const std::string vertex = shader_sources::renderer_ui_backdrop_blur::kVertex;
    vkgl::CompiledProgram fallback;
    CHECK(vkgl::load_error_shader(vertex, fallback));
    CHECK(fallback.fallback == vkgl::GraphicsFallback::Vertex);
    // This vertex declares no resources; the packaged blur fragment owns all of them.
    CHECK(fallback.default_binding == -1);
    CHECK(fallback.blocks.empty() && fallback.storage.empty());

    for (const auto& uniform : fallback.uniforms) {
        CHECK(!uniform.sampler || uniform.binding < 0);
    }

    renderer::Shader shader;
    std::string error;
    CHECK(shader.build("removed fragment resources", vertex, fragment, error));
    shader.use();
    glActiveTexture(GL_TEXTURE0);
    // A stale fragment sampler must not turn this into an illegal attachment feedback read.
    glBindTexture(GL_TEXTURE_2D, target.colour_texture());
    test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
    const auto pixels = target.read_rgba();
    pixel(pixels, 4, 4, {255, 0, 255, 255});
    pixel(pixels, 12, 4, {0, 0, 0, 255});
    glBindTexture(GL_TEXTURE_2D, 0);
}

void test_depth_prepass(const renderer::FrameBuffer& target) {
    const auto colour = target.read_rgba();
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    // The combined framebuffer has colour, but the production prepass declares only depth.
    const auto result = renderer::backend::record_surface_prepass(target.id(), false, false, [] {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });
    CHECK(result.has_value());

    if (!result) {
        std::fprintf(stderr, "%s\n", result.error().message.c_str());
        return;
    }

    CHECK(target.read_rgba() == colour);
}

// Column-major camera for the translated triangle in the geometry and mask checks.
constexpr std::array<float, 16> kTranslated{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1};

// Owns the synthetic triangle's GL objects; the test context outlives the owner.
class Triangle {
public:
    // Uploads the three clip-space positions used by the known-vertex fallback checks.
    Triangle() {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
        const std::array<float, 9> triangle{-1, -1, 0, 0, -1, 0, -1, 0, 0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Releases both names while the fixture's graphics context is still open.
    ~Triangle() {
        glDeleteBuffers(1, &m_buffer);
        glDeleteVertexArrays(1, &m_vao);
    }

    Triangle(const Triangle&) = delete;
    Triangle& operator=(const Triangle&) = delete;
    Triangle(Triangle&&) = delete;
    Triangle& operator=(Triangle&&) = delete;

private:
    unsigned m_vao = 0;     // Vertex array owned by this fixture.
    unsigned m_buffer = 0;  // Position buffer owned by this fixture.
};

// Preserves the packaged vertex's bytecode and complete uniform offsets.
void test_vertex_interface() {
    // A known vertex keeps its module and uniform offsets.
    const std::string vertex = renderer::shaders::debug_lines_vert;
    const std::string fragment = renderer::shaders::debug_lines_frag;
    vkgl::CompiledProgram original, fallback;
    CHECK(
        vkgl::decode_program(
            vkgl::builtin_shader_package().find(
                vkgl::shader_key("program", {vertex, fragment, {}})
            ),
            original
        )
    );
    CHECK(vkgl::load_error_shader(vertex, fallback));
    CHECK(fallback.fallback == vkgl::GraphicsFallback::Vertex);
    CHECK(fallback.vertex == original.vertex);
    fallback.fragment = original.fragment;
    CHECK(vkgl::encode_program(fallback) == vkgl::encode_program(original));
}

// Renders the checkerboard only inside the translated triangle, using its real camera uniform.
void test_geometry(const renderer::FrameBuffer& target, const std::string& unknown) {
    const std::string vertex = renderer::shaders::debug_lines_vert;
    std::string error;
    renderer::Shader geometry;
    CHECK(geometry.build("missing fragment variant", vertex, unknown, error));
    geometry.use();
    const auto camera = geometry.uniform("view_projection");
    CHECK(camera >= 0);
    glUniformMatrix4fv(camera, 1, GL_FALSE, kTranslated.data());
    test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
    auto pixels = target.read_rgba();
    pixel(pixels, 4, 4, {0, 255, 0, 255});
    pixel(pixels, 20, 4, {255, 0, 255, 255});
    pixel(pixels, 28, 1, {0, 0, 0, 255});
}

// Replaces incompatible topology and rejection state for a writable colour pass.
void test_viewport_fallback(const renderer::FrameBuffer& target) {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ZERO, GL_ZERO, GL_ZERO, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_NEVER);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT_AND_BACK);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NEVER, 0, ~0u);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    // No element buffer exists. A fallback must not try to read these indices.
    test::graph_draw(glDrawElements, GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, nullptr);
    const auto pixels = target.read_rgba();
    pixel(pixels, 4, 4, {255, 0, 255, 255});
    pixel(pixels, 12, 4, {0, 0, 0, 255});
    pixel(pixels, 28, 28, {255, 0, 255, 255});
}

// Restores disabled colour writes before an ordinary packaged shader draws again.
void test_restored_mask(const renderer::FrameBuffer& target) {
    const auto pixels = target.read_rgba();
    const std::string vertex = renderer::shaders::debug_lines_vert;
    const std::string fragment = renderer::shaders::debug_lines_frag;
    std::string error;
    // Ordinary rendering must still respect the caller's disabled colour writes after substitution.
    renderer::Shader ordinary;
    CHECK(ordinary.build("packaged debug lines", vertex, fragment, error));
    ordinary.use();
    glUniformMatrix4fv(ordinary.uniform("view_projection"), 1, GL_FALSE, kTranslated.data());
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
    CHECK(target.read_rgba() == pixels);
}

// Exercises both substitutions, removed fragment reads and a real depth-only graph declaration.
void test_fallback_rendering(renderer::Shader& fullscreen, const std::string& unknown) {
    CHECK(vkgl::error_shader_available());
    test_vertex_interface();
    std::string error;
    renderer::FrameBuffer target;
    CHECK(target.create(32, 32, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 32, 32);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    const Triangle triangle;
    test_geometry(target, unknown);
    test_removed_fragment_resources(target, unknown);
    fullscreen.use();
    test_depth_prepass(target);
    test_viewport_fallback(target);
    test_restored_mask(target);
}

// Runs the invocation while the executable boundary retains exception reporting.
int run_tests(int argc) {
    test::GlContext context;
    context.require_packaged_shaders();
    std::string error;

    if (!context.open(error) || !gl::is_vulkan()) {
        std::printf("SKIP: Vulkan context unavailable: %s\n", error.c_str());
        return test::kSkip;
    }

    const auto before = vkgl::submission_counters();
    // Deliberately invalid sources must reach substitution without invoking a compiler.
    const std::string unknown = "#version 410 core\nmissing_shader_for_error_test";
    CHECK(vkgl::check_stage(true, unknown, error) == (argc == 1));
    CHECK(vkgl::check_stage(false, unknown, error) == (argc == 1));
    renderer::Shader fullscreen;
    const bool linked = fullscreen.build("missing graphics", unknown, unknown, error);

    if (argc > 1) {
        CHECK(!linked);
        CHECK(error.find("missing packaged Vulkan shader") != std::string::npos);
        CHECK(vkgl::submission_counters().shader_compiles == before.shader_compiles);
        return test::result();
    }

    CHECK(linked);

    if (!linked) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::result();
    }

    test_fallback_rendering(fullscreen, unknown);
    CHECK(vkgl::submission_counters().shader_compiles == before.shader_compiles);
    CHECK(vkgl::submission_counters().shader_package_misses > before.shader_package_misses);
    CHECK(vkgl::submission_counters().validation_errors == before.validation_errors);
    return test::result();
}
}  // namespace

int main(int argc, char**) {
    try {
        return run_tests(argc);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "unexpected shader test exception: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unexpected shader test exception\n");
        return 1;
    }
}
