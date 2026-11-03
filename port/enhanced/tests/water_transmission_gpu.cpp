// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Analytic scene depths and colours exercise production refraction and composition on the GPU.
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
#include "renderer/math.h"
#include "renderer/world/gl_world.h"
#include "tests/check.h"

namespace openrac::test::water {
using namespace gl;

namespace {
// Independent sRGB decoding for the captured (already lit) scene colour.
double linear(double value) {
    return value <= .04045 ? value / 12.92 : std::pow((value + .055) / 1.055, 2.4);
}
}  // namespace

// Exercise reversed perspective/orthographic depths, contact fading and full-footprint rejection.
void check_transmission(Context& fixture) {
    const std::string probes = lighting::shaders::probes_glsl;
    const auto start = probes.find("vec3 probe_linear(");
    const auto end = probes.find("\n}", probes.find("vec3 probe_display(", start)) + 2;
    CHECK(start != std::string::npos && end > start);
    std::string source = "#version 410 core\n" + probes.substr(start, end - start)
                         + std::string(lighting::shaders::water_transmission_glsl);
    source += R"(
uniform vec4 test_surface;
uniform vec2 test_offset;
uniform vec2 test_uv;
uniform int test_composite;
uniform int test_contacts;
layout(location=0) out vec4 result;
void main() {
    vec3 background;float thickness;
    bool valid=water_scene_sample(test_uv,test_surface.w,test_surface.xyz,test_offset,
                                  test_contacts==11?.1:.5,.5,test_contacts!=0,background,thickness);
    int channel=int(gl_FragCoord.x)%5;
    float value=channel==0?float(valid):channel==1?thickness:background[channel-2];
    if(test_composite!=0) {
        water_background=vec3(.2,.3,.4);water_background_weight=vec3(.8,.7,.6);
        water_radiance=vec3(.1,.2,.3);water_composite_active=true;
        vec4 composite=effect_composite(vec3(.6),vec3(.1));
        value=channel<4?composite[channel]:float(effect_has_composite());
    }
    if(test_contacts!=0) {
        water_background=background;water_background_weight=vec3(.5);
        water_radiance=vec3(.01,.02,.03);
        vec4 composite=effect_composite(vec3(1),vec3(0));
        value=channel==0?float(valid):channel==1?water_contact_coverage:composite[channel-2];
    }
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader shader;
    CHECK(shader.build("water scene transmission", fixture.vertex, source, fixture.error));
    shader.use();
    std::array<unsigned, 2> textures{};
    glGenTextures(2, textures.data());
    std::vector<std::array<float, 4>> colours(kCount);
    for (unsigned y = 0; y < kWidth; ++y) {
        for (unsigned x = 0; x < kWidth; ++x) {
            colours[y * kWidth + x] = {.15f + .025f * float(x), .2f + .02f * float(y), .3f, 1};
        }
    }
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        renderer::world::gl::GL_RGBA32F,
        kWidth,
        kWidth,
        0,
        GL_RGBA,
        GL_FLOAT,
        colours.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glUniform1i(shader.uniform("water_scene_colour"), 3);
    glUniform1i(shader.uniform("water_scene_depth"), 4);
    glUniform2f(shader.uniform("water_scene_extent"), kWidth, kWidth);
    const auto perspective = renderer::perspective_reversed(1, 1, .1f, 100);
    for (unsigned test = 0; test < 27; ++test) {
        const bool ortho = test == 11 || test == 26;
        const std::array<float, 4> p =
            ortho ? std::array<float, 4>{.02f, 1, 0, 1}
                  : std::array<float, 4>{perspective[10], perspective[14], -1, 0};
        const auto depth = [&](float distance) {
            return .5f * ((-p[0] * distance + p[1]) / (-p[2] * distance + p[3]) + 1);
        };
        const float distance = test == 14 || test == 15  ? 3.f
                               : test == 3               ? 3.001f
                               : test == 4               ? 3.05f
                               : test == 5 || test == 19 ? 8.f
                               : test == 16              ? 3.04f
                               : test == 18              ? 3.24f
                               : test == 20              ? 1.f
                               : test >= 17              ? 3.12f
                                                         : 4.f;
        std::vector<std::array<float, 4>> depths(kCount, {depth(distance), 0, 0, 1});
        if (test == 1 || test == 2) {
            std::fill(
                depths.begin(),
                depths.end(),
                std::array<float, 4>{test == 1 ? 0.f : depth(1), 0, 0, 1}
            );
        }
        if (test == 21) {
            std::fill(depths.begin(), depths.end(), std::array<float, 4>{0, 0, 0, 1});
        }
        if (test == 22 || test == 23) {
            for (unsigned y = 7; y <= 8; ++y) {
                for (unsigned x = 9; x <= 11; ++x) {
                    depths[y * kWidth + x][0] = depth(test == 22 ? 8.f : 1.f);
                }
            }
        }
        if (test == 7 || test == 8) {
            depths[7 * kWidth + 9][0] = test == 7 ? depth(1) : 0;
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
            depths.data()
        );
        glUniform1i(shader.uniform("water_scene_ready"), test != 0);
        glUniform4fv(shader.uniform("water_scene_projection"), 1, p.data());
        glUniform4f(
            shader.uniform("test_surface"),
            test == 12 || test == 24 || test == 26 ? 3.f : 0.f,
            0,
            -3,
            depth(3)
        );
        glUniform2f(shader.uniform("test_uv"), (test == 9 ? .5f : 7.5f) / kWidth, 7.5f / kWidth);
        glUniform2f(
            shader.uniform("test_offset"),
            test == 22 || test == 23  ? 100.f
            : test >= 6 && test <= 10 ? (test == 9    ? -1.f
                                         : test == 10 ? 100.f
                                                      : 1.f)
                                      : 0.f,
            0
        );
        glUniform1i(shader.uniform("test_composite"), test == 13);
        glUniform1i(shader.uniform("test_contacts"), test >= 15 ? int(test - 14) : 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
        const bool valid = test > 2 && test != 20 && test != 21;
        const unsigned sample_x = test == 6 ? 9 : test == 9 ? 0 : test == 10 ? 13 : 7;
        std::array<double, 5> expected{
            double(valid),
            valid ? (distance - 3.) * .5 * (test == 12 ? std::sqrt(2.) : 1.) : 0,
            valid ? linear(colours[7 * kWidth + sample_x][0]) : 0,
            valid ? linear(colours[7 * kWidth + sample_x][1]) : 0,
            valid ? linear(.3) : 0
        };
        if (test == 13) {
            for (unsigned c = 0; c < 3; ++c) {
                const double weight = .8 - .1 * c;
                const double value =
                    (.1 + .1 * c) * .6 + linear(.1) * (1 - weight) + (.2 + .1 * c) * weight;
                expected[c] = 1.055 * std::pow(value, 1. / 2.4) - .055;
            }
            expected[3] = expected[4] = 1;
        }
        if (test >= 15) {
            // Coverage follows the original receiver/floor separation, even when refraction
            // encounters a much deeper floor or is rejected at an above-water silhouette.
            const double path = (distance - 3.) * (test == 24 ? std::sqrt(2.) : 1.);
            const double t = std::clamp(path * .5 / .12, 0., 1.);
            const double coverage = valid ? t * t * (3 - 2 * t) : 1.;
            expected[0] = double(valid);
            expected[1] = coverage;
            const auto& straight = colours[7 * kWidth + 7];
            const auto& refracted = colours[7 * kWidth + (test == 22 ? 10 : 7)];
            for (unsigned c = 0; c < 3; ++c) {
                const double original = valid ? linear(straight[c]) : 0;
                const double background = valid ? linear(refracted[c]) : 0;
                const double value =
                    original * (1 - coverage) + (.01 + .01 * c + .5 * background) * coverage;
                expected[c + 2] =
                    value <= .0031308 ? value * 12.92 : 1.055 * std::pow(value, 1. / 2.4) - .055;
            }
        }
        for (unsigned x = 0; x < kWidth; ++x) {
            const auto at = x * 4;
            const auto bits = std::uint32_t(fixture.pixels[at])
                              | (std::uint32_t(fixture.pixels[at + 1]) << 8)
                              | (std::uint32_t(fixture.pixels[at + 2]) << 16)
                              | (std::uint32_t(fixture.pixels[at + 3]) << 24);
            const auto actual = std::bit_cast<float>(bits);
            if (!std::isfinite(actual) || std::abs(actual - expected[x % 5]) >= 2e-4) {
                std::fprintf(
                    stderr,
                    "Transmission case %u channel %u: %.9g expected %.9g\n",
                    test,
                    x % 5,
                    actual,
                    expected[x % 5]
                );
                CHECK(false);
            }
        }
    }
    shader.release();
    glDeleteTextures(2, textures.data());
    glActiveTexture(GL_TEXTURE0);
    CHECK(glGetError() == GL_NO_ERROR);
    std::printf(
        "Compared 27 scene transmission/refraction/contact/composition cases with CPU values\n"
    );
}
}  // namespace openrac::test::water
