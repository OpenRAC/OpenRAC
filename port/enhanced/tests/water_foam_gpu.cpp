// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Foam must follow breaking shape/flow, not absolute wave height or a displacement plateau.
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "enhanced/tests/water_gpu_fixture.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"

namespace openrac::test::water {
namespace {
// Independent smoothstep for the CPU optical reference.
double smooth(double low, double high, double value) {
    const double t = std::clamp((value - low) / (high - low), 0., 1.);
    return t * t * (3 - 2 * t);
}

// Match the physical inputs, independently evaluating the bounded foam response.
double foam(double slope, double curvature, double compression, double marker) {
    return .18 * smooth(.35, 1.5, curvature)
           * std::max(smooth(.12, .35, slope), smooth(.5, 2.5, compression))
           * (.25 + .75 * smooth(.25, .85, marker));
}
}  // namespace

// Compare optical foam and production differential sampling with analytic wave/flow fields.
void check_foam(Context& fixture) {
    using namespace gl;
    const std::string water = lighting::shaders::water_glsl;
    const auto start = water.find("vec4 water_cell(");
    const auto end = water.find("vec3 water_smooth_normal(");
    const auto foam_start = water.find("float water_breaking_foam(");
    const auto foam_end = water.find("vec3 effect_material(");
    CHECK(
        start != std::string::npos && end > start && foam_start != std::string::npos
        && foam_end > foam_start
    );
    std::string source = "#version 410 core\n" + std::string(lighting::shaders::water_window_glsl)
                         + "\nuniform samplerBuffer water_light;\nuniform sampler2D "
                           "water_wave_map;\nuniform vec2 water_atlas_extent;\n"
                         + water.substr(start, end - start)
                         + water.substr(foam_start, foam_end - foam_start);
    source += R"(
uniform float height_shift;
uniform int legacy_field;
layout(location=0) out vec4 result;
void main() {
    int test=(int(gl_FragCoord.x)+16*int(gl_FragCoord.y))%17;
    float rotation,compression,curvature;
    vec4 gradients=legacy_field!=0
        ?water_surface_gradients(vec2(8.5),vec2(0),vec2(16),.25,.2+height_shift,rotation,compression,curvature)
        :water_sim_gradients(water_wave_map,vec2(2.125),1,vec2(0),.2+height_shift,rotation,compression,curvature);
    float value=0.;
    if(test==0) value=water_breaking_foam(vec2(0),0.,0.,.5);
    if(test==1) value=water_breaking_foam(vec2(0),0.,5.,.8);
    if(test==2) value=water_breaking_foam(vec2(.04,0),.1,.2,.8);
    if(test==3) value=water_breaking_foam(vec2(.5,0),-2.,4.,1.);
    if(test==4) value=water_breaking_foam(vec2(.25,0),1.2,0.,.6);
    if(test==5) value=water_breaking_foam(vec2(0),1.2,2.,.6);
    if(test==6) value=water_breaking_foam(vec2(.5,0),0.,0.,1.);
    if(test==7) value=water_breaking_foam(vec2(1,0),10.,10.,1.);
    if(test==8) value=water_breaking_foam(vec2(.35,0),.3501,2.5,1.);
    if(test==9) value=gradients.x;
    if(test==10) value=gradients.y;
    if(test==11) value=rotation;
    if(test==12) value=compression;
    if(test==13) value=curvature;
    if(test==14) value=water_breaking_foam(gradients.xy,curvature,compression,.5);
    if(test==15) value=water_breaking_foam(vec2(.35,0),1.5,2.5,1.);
    if(test==16) {
        water_surface_gradients(vec2(.5),vec2(0),vec2(16),.25,0.,rotation,compression,curvature);
        value=curvature;
    }
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader shader;
    const bool built = shader.build("water breaking foam", fixture.vertex, source, fixture.error);
    CHECK(built);
    if (!built) {
        std::fprintf(stderr, "%s\n", fixture.error.c_str());
        return;
    }
    shader.use();
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glUniform1i(shader.uniform("water_wave_map"), 6);
    glUniform2f(shader.uniform("water_atlas_extent"), 16, 16);
    glUniform1i(shader.uniform("water_sim_local"), 1);
    glUniform1i(shader.uniform("water_sim_body"), 1);
    glUniform1f(shader.uniform("water_sim_weight"), 1);
    glUniform4f(shader.uniform("water_sim_window"), 0, 0, .25f, 16);
    const std::array<double, 17> expected{
        0,
        0,
        0,
        0,
        foam(.25, 1.2, 0, .6),
        foam(0, 1.2, 2, .6),
        0,
        .18,
        foam(.35, .3501, 2.5, 1),
        0,
        0,
        0,
        1.6,
        2,
        foam(0, 2, 1.6, .5),
        .18,
        0
    };
    for (const float shift : {0.f, .25f}) {
        std::vector<Cell> values(kCount);
        for (unsigned y = 0; y < kWidth; ++y) {
            for (unsigned x = 0; x < kWidth; ++x) {
                const float dx = (float(x) - 8) * .25f, dy = (float(y) - 8) * .25f;
                values[y * kWidth + x] =
                    {.2f + shift - .5f * (dx * dx + dy * dy), -.8f * dx, -.8f * dy, .5f};
            }
        }
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            renderer::world::gl::GL_RGBA32F,
            kWidth,
            kWidth,
            0,
            GL_RGBA,
            GL_FLOAT,
            values.data()
        );
        glUniform1f(shader.uniform("height_shift"), shift);
        for (const int legacy : {0, 1}) {
            glUniform1i(shader.uniform("legacy_field"), legacy);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
            for (unsigned i = 0; i < kCount; ++i) {
                const auto at = i * 4;
                const auto bits = std::uint32_t(fixture.pixels[at])
                                  | (std::uint32_t(fixture.pixels[at + 1]) << 8)
                                  | (std::uint32_t(fixture.pixels[at + 2]) << 16)
                                  | (std::uint32_t(fixture.pixels[at + 3]) << 24);
                const float actual = std::bit_cast<float>(bits);
                if (!std::isfinite(actual) || std::abs(actual - expected[i % 17]) >= 2e-5) {
                    std::fprintf(
                        stderr,
                        "Foam case %u legacy %d shift %.2f: %.9g expected %.9g\n",
                        i % 17,
                        legacy,
                        shift,
                        actual,
                        expected[i % 17]
                    );
                    CHECK(false);
                }
            }
        }
    }
    glDeleteTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    shader.release();
    CHECK(glGetError() == GL_NO_ERROR);
    std::printf(
        "Compared 17 foam/differential cases, both sampling paths and two height offsets\n"
    );
}
}  // namespace openrac::test::water
