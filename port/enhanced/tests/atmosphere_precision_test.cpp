// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/shader.h"
#include "renderer/shader_precision.h"
#include "renderer_shaders.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/spirv_precision.h"
#include "renderer/vulkan/vkgl.h"
#endif
using namespace openrac;
using namespace openrac::gl;

int main() {
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::kSkip;
    }
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    const std::string vertex = renderer::shaders::gtao_vert;
    const std::string fragment =
        std::string("#version 430 core\n") + lighting::shaders::atmosphere_lighting_glsl + R"(
uniform vec3 sun;
uniform vec4 parameters;
uniform int channel,point_light;
layout(location=0) out vec4 result;
void main() {
    float phase=atmosphere_phase(parameters.x,parameters.y);
    vec3 lit=point_light!=0 ? atmosphere_point_colour(sun,parameters.z)
        : atmosphere_illumination(sun,phase,parameters.z,parameters.w);
    vec4 value=vec4(lit,phase);
    uint bits=floatBitsToUint(value[channel]);
    result=vec4(float(bits&255u),float((bits>>8)&255u),float((bits>>16)&255u),float(bits>>24))/255.;
}
)";
#ifdef OPENRAC_VULKAN
    vkgl::CompiledProgram full, half;
    const bool full_ok = vkgl::compile_program(
        vertex, renderer::shader_precision_source(fragment, false), full, error
    );
    if (!full_ok) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(full_ok);
    const bool half_ok = vkgl::compile_program(
        vertex, renderer::shader_precision_source(fragment, true), half, error
    );
    if (!half_ok) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(half_ok);
    const auto full_precision = vkgl::spirv_precision(full.fragment);
    const auto half_precision = vkgl::spirv_precision(half.fragment);
    CHECK(full_precision.fp16 == 0 && full_precision.fp32 > 0);
    CHECK(half_precision.fp16 >= 4 && half_precision.fp32 > 0);
    CHECK(!full_precision.storage16 && !half_precision.storage16);
    CHECK(full.default_size == half.default_size && full.uniforms.size() == half.uniforms.size());
    for (std::size_t i = 0; i < full.uniforms.size() && i < half.uniforms.size(); ++i) {
        CHECK(full.uniforms[i].name == half.uniforms[i].name);
        CHECK(full.uniforms[i].offset == half.uniforms[i].offset);
        CHECK(full.uniforms[i].gl_type == half.uniforms[i].gl_type);
    }
    std::printf(
        "Atmosphere lighting SPIR-V: %llu FP16 arithmetic instructions; fallback %llu\n",
        static_cast<unsigned long long>(half_precision.fp16),
        static_cast<unsigned long long>(full_precision.fp16)
    );
#endif
    renderer::Shader shader;
    CHECK(shader.build("atmosphere lighting precision", vertex, fragment, error));
    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    renderer::FrameBuffer target;
    CHECK(target.create(4, 4, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 4, 4);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    const std::array<std::array<float, 3>, 3> colours{
        {{.8f, .6f, .4f}, {100000, 200000, 50000}, {1e-8f, 2e-8f, 0}}
    };
    for (std::size_t colour = 0; colour < colours.size(); ++colour) {
        for (int point = 0; point < 2; ++point) {
            for (float g : {-.8f, 0.f, .3f, .8f}) {
                // Extreme colours use exact neutral phase to verify the FP32 range guard.
                const float anisotropy = colour == 0 ? g : 0;
                const float cosine = .2f, visibility = colour == 0 ? .65f : 1.f,
                            strength = colour == 0 ? .7f : 1.f;
                const double phase = std::min(
                    4.,
                    (1. - double(anisotropy) * anisotropy)
                        / std::pow(
                            std::max(
                                .01, 1. + double(anisotropy) * anisotropy - 2. * anisotropy * cosine
                            ),
                            1.5
                        )
                );
                shader.use();
                glUniform3f(
                    shader.uniform("sun"),
                    colours[colour][0],
                    colours[colour][1],
                    colours[colour][2]
                );
                glUniform4f(shader.uniform("parameters"), cosine, anisotropy, visibility, strength);
                glUniform1i(shader.uniform("point_light"), point);
                for (int c = 0; c < 4; ++c) {
                    glUniform1i(shader.uniform("channel"), c);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    std::array<std::uint8_t, 4> pixel{};
                    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
                    const auto bits = std::uint32_t(pixel[0]) | (std::uint32_t(pixel[1]) << 8)
                                      | (std::uint32_t(pixel[2]) << 16)
                                      | (std::uint32_t(pixel[3]) << 24);
                    const float actual = std::bit_cast<float>(bits);
                    const double expected =
                        c == 3  ? phase
                        : point ? double(colours[colour][c]) * visibility
                                : 1. - strength
                                      + double(colours[colour][c]) * phase * visibility * strength;
                    CHECK(std::isfinite(actual));
                    const double tolerance = colour == 0 ? .004 : 1e-6;
                    CHECK(
                        std::abs(actual - expected)
                        <= tolerance * std::max(std::abs(expected), 1e-12)
                    );
                }
            }
        }
    }
    glDeleteVertexArrays(1, &vao);
    CHECK(glGetError() == GL_NO_ERROR);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
