// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <iostream>
#include <string>

#include "renderer/shader_precision.h"
#include "renderer/vulkan/shader_compiler.h"
#include "renderer/vulkan/spirv_precision.h"
#include "tests/check.h"

using namespace openrac;
constexpr const char* vertex = R"glsl(#version 410 core
void main() { gl_Position = vec4(0,0,0,1); }
)glsl";
constexpr const char* fragment = R"glsl(#version 410 core
uniform vec4 colour;
uniform float weight;
layout(std140) uniform Lighting { vec4 light; };
layout(location=0) out vec4 result;
void main() {
    mp_vec3 c = mp_vec3(colour.rgb) * mp_float(0.75);
    c = c + mp_vec3(light.rgb) * mp_float(weight);
    c = mix(clamp(c, mp_vec3(0), mp_vec3(1)), pow(c, mp_vec3(2)), mp_float(0.5));
    result = vec4(vec3(c), colour.a);
}
)glsl";

int main() {
    const auto version_only =
        renderer::shader_precision_source("#version 410 core // mp_float", true);
    CHECK(version_only.starts_with("#version 450 core\n#define OPENRAC_PRECISION_DECLARED"));
    vkgl::CompiledProgram full, half;
    std::string error;
    const auto fp32 = renderer::shader_precision_source(fragment, false);
    const auto fp16 = renderer::shader_precision_source(fragment, true);
    CHECK(
        fp32.find("#extension GL_EXT_shader_explicit_arithmetic_types_float16") == std::string::npos
    );
    CHECK(fp16.find("#define OPENRAC_USE_FP16 1") != std::string::npos);
    CHECK(renderer::shader_precision_source(fp16, true) == fp16);
    CHECK(vkgl::compile_program(vertex, fp32, full, error));
    if (!error.empty()) {
        std::cerr << error;
    }
    error.clear();
    CHECK(vkgl::compile_program(vertex, fp16, half, error));
    if (!error.empty()) {
        std::cerr << error;
    }
    const auto a = vkgl::spirv_precision(full.fragment), b = vkgl::spirv_precision(half.fragment);
    CHECK(a.fp16 == 0 && a.fp32 > 0);
    CHECK(b.fp16 >= 3);
    CHECK(!a.storage16 && !b.storage16);
    CHECK(!a.float64 && !b.float64);
    CHECK(full.default_size == half.default_size);
    CHECK(full.default_binding == half.default_binding);
    CHECK(full.uniforms.size() == half.uniforms.size());
    for (std::size_t i = 0; i < full.uniforms.size() && i < half.uniforms.size(); ++i) {
        CHECK(full.uniforms[i].name == half.uniforms[i].name);
        CHECK(full.uniforms[i].gl_type == half.uniforms[i].gl_type);
        CHECK(full.uniforms[i].offset == half.uniforms[i].offset);
    }
    CHECK(full.blocks.size() == half.blocks.size());
    if (!full.blocks.empty() && !half.blocks.empty()) {
        CHECK(full.blocks[0].size == half.blocks[0].size);
    }
    vkgl::CompiledProgram storage;
    error.clear();
    CHECK(
        vkgl::compile_program(
            vertex,
            R"glsl(#version 410 core
uniform sampler2D half_float_texture;
layout(location=0) out vec4 result;
void main() { result = texture(half_float_texture, vec2(0.5)) * 0.5; }
)glsl",
            storage,
            error
        )
    );
    CHECK(vkgl::spirv_precision(storage.fragment).fp16 == 0);
    std::cout << "FP16 arithmetic instructions: " << b.fp16 << "; FP32 fallback: " << a.fp32
              << '\n';
    return test::result();
}
