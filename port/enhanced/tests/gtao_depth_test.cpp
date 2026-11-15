// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks every GPU depth bound against CPU min/max reduction, including odd edges, empty pixels and resizing.
#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <vector>

#include "enhanced/renderer/gtao_depth.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
// Two bounds per texel, matching the shader's std430 vec2 layout.
using Bounds = std::array<float, 2>;

// Reduces clamped two-by-two footprints; duplicated odd-edge samples preserve exact extrema.
std::vector<Bounds> reduce(std::span<const Bounds> source, int width, int height) {
    const int next_width = (width + 1) / 2;
    const int next_height = (height + 1) / 2;
    std::vector<Bounds> output(
        static_cast<std::size_t>(next_width) * static_cast<unsigned>(next_height)
    );

    for (int y = 0; y < next_height; ++y) {
        for (int x = 0; x < next_width; ++x) {
            Bounds value{1, 0};

            for (int corner = 0; corner < 4; ++corner) {
                const int sx = std::min(x * 2 + (corner & 1), width - 1);
                const int sy = std::min(y * 2 + (corner >> 1), height - 1);
                const auto& previous = source
                    [static_cast<std::size_t>(sy) * static_cast<unsigned>(width)
                     + static_cast<unsigned>(sx)];
                value = {std::min(value[0], previous[0]), std::max(value[1], previous[1])};
            }

            output
                [static_cast<std::size_t>(y) * static_cast<unsigned>(next_width)
                 + static_cast<unsigned>(x)] = value;
        }
    }

    return output;
}

// Reads both bounds through their texture-buffer view; source depths differ by at least one eighth.
void check_level(
    unsigned buffer, const renderer::GtaoDepthLevel& level, std::span<const Bounds> expected
) {
    using namespace gl;
    constexpr auto kVertex = R"(#version 410 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)";
    constexpr auto kFragment = R"(#version 410 core
uniform samplerBuffer tex_T0;
uniform int first_cell;
uniform int width;
out vec4 colour;
void main(){int i=int(gl_FragCoord.y)*width+int(gl_FragCoord.x);
colour=vec4(texelFetch(tex_T0,first_cell+i).rg,0,1);}
)";
    renderer::Shader shader;
    renderer::FrameBuffer target;
    std::string error;
    CHECK(shader.build("depth bounds readback", kVertex, kFragment, error));
    CHECK(target.create(level.width, level.height, error));
    unsigned texture = 0;
    unsigned vao = 0;
    glGenTextures(1, &texture);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, texture);
    glTexBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, GL_RG32F, buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, level.width, level.height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    shader.use();
    glUniform1i(shader.uniform("first_cell"), static_cast<int>(level.offset / sizeof(Bounds)));
    glUniform1i(shader.uniform("width"), level.width);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::vector<std::uint8_t> pixels(expected.size() * 4);
    glReadPixels(0, 0, level.width, level.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(
            std::abs(
                static_cast<int>(pixels[i * 4])
                - static_cast<int>(std::lround(expected[i][0] * 255))
            )
            <= 1
        );
        CHECK(
            std::abs(
                static_cast<int>(pixels[i * 4 + 1])
                - static_cast<int>(std::lround(expected[i][1] * 255))
            )
            <= 1
        );
    }

    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    target.release();
}

// Validates every stored level and reuses the same owner at different viewport sizes.
void test_depth_bounds(renderer::GtaoDepth& pyramid, int width, int height) {
    using namespace gl;
    std::vector<float> depths(static_cast<std::size_t>(width) * static_cast<unsigned>(height));
    std::vector<Bounds> expected(depths.size());

    for (std::size_t index = 0; index < depths.size(); ++index) {
        depths[index] = static_cast<float>(index % 8) / 8.0f;
        expected[index] = {depths[index], depths[index]};
    }

    unsigned texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x822E, width, height, 0, 0x1903, GL_FLOAT, depths.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK(pyramid.prepare(width, height));

    for (unsigned index = 0; index < pyramid.levels().size(); ++index) {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        pyramid.build_level(texture, index);
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pyramid.buffer());

    for (const auto& level : pyramid.levels()) {
        expected = reduce(expected, width, height);
        width = (width + 1) / 2;
        height = (height + 1) / 2;
        CHECK(width == level.width && height == level.height);
        check_level(pyramid.buffer(), level, expected);
    }

    glDeleteTextures(1, &texture);
}

// Invalid reads fail before callbacks, aliases coalesce, and consumed entry states cannot be replayed.
void test_execution_validation() {
    using namespace gl;
    namespace graph = renderer::graph;
    unsigned buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    constexpr unsigned kBytes = 16;
    glBufferData(GL_SHADER_STORAGE_BUFFER, kBytes, nullptr, GL_DYNAMIC_DRAW);
    vkgl::GraphExecution invalid;
    const auto first = invalid.import_buffer("first view", buffer, vkgl::InitialContents::Discard);
    const auto alias = invalid.import_buffer("second view", buffer, vkgl::InitialContents::Discard);
    CHECK(first && alias && first->index == alias->index);
    invalid.add_pass(
        {"unwritten",
         {},
         {{*first,
           {0, kBytes},
           {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}}}
    );
    bool called = false;
    CHECK(!invalid.run([&](unsigned) { called = true; }));
    CHECK(!called);
    vkgl::GraphExecution empty;
    CHECK(empty.run([&](unsigned) { called = true; }).has_value());
    CHECK(!empty.run([&](unsigned) { called = true; }));
    CHECK(!called);
    glDeleteBuffers(1, &buffer);
}
}  // namespace

// Runs only with Vulkan; all test depths are synthetic binary fractions.
int main() {
    try {
        test::GlContext context;
        std::string error;

        if (!context.open(error) || !vkgl::active()) {
            return test::kSkip;
        }

        renderer::GtaoDepth pyramid;
        test_depth_bounds(pyramid, 1, 1);
        test_depth_bounds(pyramid, 5, 3);
        test_execution_validation();
        test_depth_bounds(pyramid, 129, 65);
        test_depth_bounds(pyramid, 5, 3);
        CHECK(vkgl::submission_counters().validation_errors == 0);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "depth hierarchy test: %s\n", failure.what());
        return 1;
    }
}
