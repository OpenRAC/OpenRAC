// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Synthetic coat geometry must extend the outline, flatten when wet and respect exclusions/fade.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "enhanced_viewer_shaders.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace openrac::gl;
constexpr int kSize = 96;  // Enough pixels to distinguish dry and flattened coat extents.

// GL names are released while the test's context is current, including on early failure.
struct Resources {
    unsigned vao = 0, map = 0;
    Resources() = default;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(Resources&&) = delete;

    ~Resources() {
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &map);
    }
};

// Finds actual shaded silhouette coverage, rather than counting submitted vertices.
int right_edge(const std::vector<std::uint8_t>& image) {
    int edge = -1;
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            if (image[(std::size_t(y) * kSize + std::size_t(x)) * 4] != 0) {
                edge = std::max(edge, x);
            }
        }
    }
    return edge;
}

// Uses the same shared functions in both programmable stages.
bool build_shader(renderer::Shader& shader, std::string& error) {
    const auto common = std::string(lighting::shaders::fur_shell_glsl);
    const auto vertex = std::string("#version 430 core\n") + common + R"(
uniform float wetness;
out vec2 sample_uv;
void main() {
    vec2 points[3]=vec2[3](vec2(-.35,-.6),vec2(.35,-.6),vec2(0,.6));
    vec2 p=points[gl_VertexID];
    sample_uv=p*.5+.5;
    p.x+=fur_shell_extent(wetness);
    gl_Position=vec4(p,0,1);
})";
    const auto fragment =
        std::string("#version 430 core\n#define OPENRAC_FUR_FRAGMENT 1\n") + common + R"(
uniform sampler2D groom;
uniform int eligible;
in vec2 sample_uv;
layout(location=0) out vec4 result;
void main() {
    if(fur_shell_reject(groom,sample_uv,eligible)) discard;
    result=vec4(1);
})";
    return shader.build("fur shell silhouette", vertex, fragment, error);
}

// Exercises the exact shared colour/motion coverage and bind-space displacement functions.
int run() {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan()) {
        std::printf("SKIP: %s\n", error.c_str());
        return test::kSkip;
    }
    Resources resources;
    renderer::Shader shader;
    renderer::FrameBuffer target;
    CHECK(build_shader(shader, error));
    CHECK(target.create(kSize, kSize, error));
    if (!shader.id() || !target.id()) {
        std::printf("%s\n", error.c_str());
        return test::result();
    }
    glGenVertexArrays(1, &resources.vao);
    glBindVertexArray(resources.vao);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &resources.map);
    glBindTexture(GL_TEXTURE_2D, resources.map);
    const unsigned white_groom = 0xff8080ffu;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_groom);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, kSize, kSize);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    shader.use();
    glUniform1i(shader.uniform("groom"), 0);
    glUniform1i(shader.uniform("hero_fur_shell_enabled"), 1);
    glUniform4f(shader.uniform("hero_fur_shell_parameters"), .35f, 32, 12, 24);
    const auto draw = [&](int layers, float wetness, float distance, int eligible) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(shader.uniform("wetness"), wetness);
        glUniform1f(shader.uniform("hero_fur_shell_distance"), distance);
        glUniform1i(shader.uniform("eligible"), eligible);
        for (int layer = 0; layer <= layers; ++layer) {
            glUniform1f(shader.uniform("hero_fur_shell"), float(layer) / 11.f);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        return target.read_rgba();
    };
    const auto base = draw(0, 0, 0, 1);
    const auto dry = draw(10, 0, 0, 1);
    const auto wet = draw(10, 1, 0, 1);
    CHECK(right_edge(dry) > right_edge(base));
    CHECK(right_edge(wet) < right_edge(dry));
    CHECK(draw(10, 0, 24, 1) == base);
    CHECK(draw(10, 0, 0, 0) == base);
    CHECK(draw(10, 0, 0, 1) == dry);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    std::printf(
        "Coat outline: base %d, wet %d, dry %d\n",
        right_edge(base),
        right_edge(wet),
        right_edge(dry)
    );
    return test::result();
}
}  // namespace

// No retail model or persistent reference image is required.
int main() {
    return run();
}
