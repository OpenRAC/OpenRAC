// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Compare actual GPU-authored UV motion and baked template interpolation with a CPU reference.
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string>

#include "enhanced/renderer/water_mesh.h"
#include "enhanced/tests/water_test_bake.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/shader.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
// Mirror the original layer's phase convention, evaluated only on its coarse vertices.
std::array<float, 2> reference(
    const openrac::renderer::EffectVertex& p, const openrac::renderer::EffectUvMotion& motion
) {
    if (!motion.enabled) {
        return {p.s, p.t};
    }
    const auto seed =
        ((std::bit_cast<std::uint32_t>(p.x) + std::bit_cast<std::uint32_t>(p.y)) >> 16) & 255u;
    const auto phase = (((seed << 6) + (motion.phase & 511u)) << 3) & 4095u;
    const float angle = float(phase) * (6.2831855f / 4096.f) - 3.1415927f;
    return {
        p.s + motion.offset[0] + std::sin(angle) * motion.amplitude,
        p.t + motion.offset[1] + std::cos(angle) * motion.amplitude
    };
}
}  // namespace

int main() {
    using namespace openrac;
    using namespace gl;
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan()) {
        return test::kSkip;
    }
    test::WaterTestBake data;
    enhanced::WaterMesh mesh(data.folder / "water.bin", data.fingerprint);
    const bool initialized = mesh.init(error);
    CHECK(initialized);
    if (!initialized) {
        return test::result();
    }
    renderer::Shader shader;
    std::string vertex = "#version 430 core\n";
    vertex += lighting::shaders::water_uv_glsl;
    vertex += lighting::shaders::water_mesh_glsl;
    vertex += R"(
layout(location=1) in vec2 weights;
out vec2 uv_out;
out float red_out;
void main() {
    vec3 p=vec3(0); vec2 uv=weights; vec4 c=vec4(0);
    water_mesh_vertex(p,uv,c);
    uv_out=uv; red_out=c.r;
    gl_Position=vec4(p.xy*2.0-1.0,0,1);
})";
    constexpr auto fragment = R"(#version 430 core
in vec2 uv_out;
in float red_out;
uniform int component;
out vec4 result;
void main(){
    float value=component==0?uv_out.x:component==1?uv_out.y:red_out;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8u)&255u,(bits>>16u)&255u,bits>>24u)/255.0;
})";
    const bool built = shader.build("baked water UV fixture", vertex, fragment, error);
    CHECK(built);
    if (!built) {
        return test::result();
    }
    renderer::FrameBuffer frame;
    CHECK(frame.create(32, 32, error));
    std::array<renderer::EffectVertex, 3> source{
        {{0, 0, 1.1f, .1f, .2f, 0x80808010},
         {1, 0, 1.1f, .7f, -.1f, 0x80808040},
         {0, 1, 1.1f, .4f, .8f, 0x80808080}}
    };
    renderer::EffectStrip strip;
    strip.surface = 1;
    strip.count = 3;
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    // Signed amplitudes are the two opposed texture layers; wrapped phases and fallback remain valid.
    for (unsigned phase : {0u, 511u, 1025u}) {
        for (float amplitude : {-.07f, 0.f, .07f}) {
            strip.uv_motion = {{.13f, -.27f}, amplitude, phase, amplitude != 0};
            mesh.begin_pass();
            const auto plan = mesh.prepare(strip, source);
            mesh.upload();
            const auto a = reference(source[0], strip.uv_motion);
            const auto b = reference(source[1], strip.uv_motion);
            const auto c = reference(source[2], strip.uv_motion);
            for (int component = 0; component < 3; ++component) {
                glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
                glViewport(0, 0, 32, 32);
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
                shader.use();
                mesh.bind(shader);
                glUniform1i(shader.uniform("component"), component);
                mesh.draw(shader, plan);
                const auto pixels = frame.read_rgba();
                CHECK(pixels.size() == 32 * 32 * 4);
                if (pixels.size() != 32 * 32 * 4) {
                    return test::result();
                }
                // Interior samples cover both baked sub-triangles without relying on edge rasterisation.
                for (const auto point : {std::array<unsigned, 2>{4, 5}, {18, 5}}) {
                    // FrameBuffer returns image rows from the top, while the reference uses GL Y.
                    const auto at = ((31 - point[1]) * 32 + point[0]) * 4;
                    const auto bits = std::uint32_t(pixels[at])
                                      | (std::uint32_t(pixels[at + 1]) << 8)
                                      | (std::uint32_t(pixels[at + 2]) << 16)
                                      | (std::uint32_t(pixels[at + 3]) << 24);
                    const float x = (float(point[0]) + .5f) / 32.f,
                                y = (float(point[1]) + .5f) / 32.f;
                    const float expected =
                        component < 2
                            ? (1 - x - y) * a[component] + x * b[component] + y * c[component]
                            : (1 - x - y) * 16.f + x * 64.f + y * 128.f;
                    CHECK(std::abs(std::bit_cast<float>(bits) - expected) < 3e-5f);
                }
            }
        }
    }
    mesh.release();
    CHECK(vkgl::submission_counters().validation_errors == 0);
    CHECK(glGetError() == GL_NO_ERROR);
    return test::result();
}
