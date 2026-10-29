// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_window.h"
#include "enhanced/tests/water_gpu_fixture.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/shader.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#endif
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace openrac::test::water {
using namespace openrac::gl;
namespace wgl = renderer::world::gl;

void check_optics(Context& fixture) {
    bool float16 = false;
#ifdef OPENRAC_VULKAN
    vkgl::NativeDevice device;
    float16 = vkgl::native_device(device) && device.float16;
#endif
    // Optical approximations have a separate explicit budget; displacement/simulation retain theirs.
    const double optical_error = float16 ? 1e-3 : 2e-5;
    const double unit_error = 1e-5;
    const auto* vertex = fixture.vertex;
    auto& error = fixture.error;
    auto& pixels = fixture.pixels;
    const auto texture = fixture.texture;
    // Shared Gerstner displacement/tangents against independent CPU positions
    // and finite differences, including a spatially varying shoreline taper.
    std::string swell_source = "#version 410 core\n";
    swell_source += lighting::shaders::water_swell_glsl;
    const std::string water_source = lighting::shaders::water_glsl;
    const auto curvature_start = water_source.find("float water_shore_curvature");
    const auto curvature_end = water_source.find("float water_shore_weight", curvature_start);
    CHECK(curvature_start != std::string::npos && curvature_end > curvature_start);
    swell_source += water_source.substr(curvature_start, curvature_end - curvature_start);
    swell_source += R"(
layout(location=0) out vec4 result;
void main(){
    int test=int(gl_FragCoord.x)%36;
    vec3 dx,dy;
    vec3 d=water_swell(vec2(.37,.91),.43,.8,vec2(.1,-.08),dx,dy);
    float value=0.;
    if(test<3) value=d[test];
    else if(test<6) value=dx[test-3];
    else if(test<9) value=dy[test-6];
    else if(test==9) value=length(water_swell(vec2(.37,.91),.43,0.,vec2(0),dx,dy));
    else if(test==10) value=water_art_tint(vec3(.025,.19,.30),vec3(.2)).b;
    else if(test==11) value=water_art_tint(vec3(.025,.19,.30),vec3(0)).b;
    else if(test==12) value=water_boundary_swell(vec3(.3,1,0),.43,.8).z;
    else if(test==13) value=water_boundary_swell(vec3(.3,1,0),.43,.8).x;
    else if(test==14) value=length(water_boundary_swell(vec3(.3,1,0),.43,0.));
    else if(test==15) value=length(water_boundary_swell(vec3(.3,0,0),.43,.8));
    else if(test<32) {
        // Vary bank direction, taper and distance together, including the fade.
        vec3 boundary=vec3(.3,.2+.05*float(test-16),.12);
        vec3 change=vec3(.7,.13,-.09);
        float epsilon=.001;
        vec3 numeric=(water_boundary_swell(boundary+epsilon*change,.43,.8+epsilon*.1)
                     -water_boundary_swell(boundary-epsilon*change,.43,.8-epsilon*.1))/(2.*epsilon);
        value=length(water_boundary_tangent(boundary,change,.43,.8,.1)-numeric);
    }
    if(test==32) value=water_shore_curvature(.25,0.,1.,.5,1.);
    if(test==33) value=water_shore_curvature(-.5,0.,1.,.5,1.);
    if(test==34) value=water_shore_curvature(0.,0.,1.,0.,1.);
    if(test==35) value=water_shore_curvature(0.,0.,1.,1e-8,1.);
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader swell_test;
    CHECK(
        swell_test.build("Gerstner water displacement and tangents", vertex, swell_source, error)
    );
    swell_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const auto swell_position = [](double x, double y) {
        const double norm = std::hypot(.87, .49), dx = .87 / norm, dy = .49 / norm;
        const double phase = (6.28318531 / 2.75) * (dx * x + dy * y) - .43 * 1.4;
        const double weight = .8 + .1 * (x - .37) - .08 * (y - .91);
        return std::array<double, 3>{
            weight * .045 * .65 * dx * std::cos(phase),
            weight * .045 * .65 * dy * std::cos(phase),
            weight * .045 * std::sin(phase)
        };
    };
    const auto centre = swell_position(.37, .91);
    const double epsilon = 1e-4;
    const auto left = swell_position(.37 - epsilon, .91),
               right = swell_position(.37 + epsilon, .91);
    const auto bottom = swell_position(.37, .91 - epsilon),
               top = swell_position(.37, .91 + epsilon);
    for (unsigned x = 0; x < kWidth; ++x) {
        unsigned at = x * 4, test = x % 36;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        double expected = 0.;
        if (test < 3) {
            expected = centre[test];
        } else if (test < 6) {
            expected = (right[test - 3] - left[test - 3]) / (2 * epsilon);
        } else if (test < 9) {
            expected = (top[test - 6] - bottom[test - 6]) / (2 * epsilon);
        } else if (test == 10) {
            expected = .30;
        } else if (test == 11) {
            expected = .30 * (.68 + .32 * .4);
        } else if (test == 12) {
            expected =
                .042 * .8 * std::exp(-.3 * .12) * std::sin((6.28318531 / 2.2) * .3 - .43 * 2.);
        } else if (test == 13) {
            expected = .014 * .8 * std::exp(-.3 * .12) * .65
                       * std::cos((6.28318531 / 2.2) * .3 - .43 * 2.);
        }
        if (test == 32) {
            expected = 2.;
        }
        CHECK(std::isfinite(actual));
        CHECK(std::abs(actual - expected) < 2e-5);
    }
    // Exercise the production BRDF/absorption helpers on the GPU against analytic values.
    const std::string material = lighting::shaders::water_glsl;
    const auto helper_start = material.find("// Air/water interface");
    const auto helper_end = material.find("vec3 water_direct_specular");
    CHECK(helper_start != std::string::npos && helper_end > helper_start);
    std::string physical = "#version 410 core\n" + std::string(lighting::shaders::shading_brdf_glsl)
                           + material.substr(helper_start, helper_end - helper_start);
    physical += R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%8;
    vec3 n=vec3(0,0,1);
    float value=0.;
    if(test==0) value=water_fresnel(1.);
    if(test==1) value=water_fresnel(0.);
    if(test==2) value=water_specular(n,n,n,.5);
    if(test==3) value=water_specular(n,n,-n,.5);
    if(test==4) value=water_transmittance(0.,1.).r;
    if(test==5) value=water_transmittance(2.,1.).r;
    if(test==6) value=water_transmittance(2.,1.).b;
    if(test==7) value=water_transmittance(2.,0.).g;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader physical_test;
    CHECK(physical_test.build("water physical lighting", vertex, physical.c_str(), error));
    physical_test.use();
    glUniform3f(physical_test.uniform("water_material"), .22f, .02037f, 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const double refracted_cosine = std::sqrt(1. - 1. / (1.333 * 1.333));
    const std::array<double, 8> expected_light{
        .02037,
        1.,
        .02037 / (4. * 3.141592653589793 * .0625),
        0.,
        1.,
        std::exp(-.85 * 2),
        std::exp(-.12 * 2),
        std::exp(-.24 * 2 / refracted_cosine)
    };
    for (unsigned x = 0; x < kWidth; ++x) {
        const auto at = x * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        CHECK(std::isfinite(actual));
        CHECK(std::abs(actual - expected_light[x % 8]) < optical_error);
    }
    // The shared fallback preserves GI colour, applies Fresnel once and keeps black GI black.
    const std::string fallback_source =
        std::string("#version 410 core\n") + std::string(lighting::shaders::shading_brdf_glsl) + R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%4;
    vec3 colour=gi_specular_fallback(test==3 ? vec3(0) : vec3(3.14159265,0,0),test==1 ? 0. : 1.,.04);
    float value=test==2 ? colour.g : colour.r;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader fallback_test;
    CHECK(fallback_test.build("local GI reflection fallback", vertex, fallback_source, error));
    fallback_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const std::array<float, 4> expected_fallback{.04f, 1, 0, 0};
    for (unsigned x = 0; x < kWidth; ++x) {
        const auto at = x * 4;
        const auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                          | (std::uint32_t(pixels[at + 2]) << 16)
                          | (std::uint32_t(pixels[at + 3]) << 24);
        CHECK(std::abs(std::bit_cast<float>(bits) - expected_fallback[x % 4]) < 1e-5f);
    }
    // Smooth slopes must preserve a planar ramp at subcell positions and atlas borders.
    // Poison outside the selected 8x8 body catches cross-body filtering.
    std::vector<Cell> ramp(kCount, Cell{1000, 0, 0, 0});
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            ramp[(y + 4) * kWidth + x + 4][0] = .1f * float(x) - .05f * float(y);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, wgl::GL_RGBA32F, kWidth, kWidth, 0, GL_RGBA, GL_FLOAT, ramp.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const auto smooth_start = material.find("// Smooth simulation slopes");
    CHECK(smooth_start != std::string::npos);
    std::string smooth =
        "#version 410 core\nuniform sampler2D water_wave_map;\nuniform vec2 water_atlas_extent;\n"
        + material.substr(smooth_start, helper_start - smooth_start);
    smooth += R"(
layout(location=0) out vec4 result;
uniform int component;
void main() {
    vec2 cell=vec2(.5)+vec2(gl_FragCoord.xy-vec2(.5))*7./15.;
    vec2 slope=water_height_gradient(cell,vec2(4),vec2(8),.5);
    vec3 n=water_smooth_normal(normalize(vec3(-.3,.1,1)),slope);
    uint bits=floatBitsToUint(n[component]);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader smooth_test;
    CHECK(smooth_test.build("water smooth normals", vertex, smooth.c_str(), error));
    smooth_test.use();
    glUniform1i(smooth_test.uniform("water_wave_map"), 0);
    glUniform2f(smooth_test.uniform("water_atlas_extent"), kWidth, kWidth);
    // Authored slope (.3,-.1) plus wave slope (.2,-.1), normalized.
    const std::array<double, 3> expected_normal{
        -.5 / std::sqrt(1.29), .2 / std::sqrt(1.29), 1. / std::sqrt(1.29)
    };
    for (int component = 0; component < 3; ++component) {
        glUniform1i(smooth_test.uniform("component"), component);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (unsigned at = 0; at < pixels.size(); at += 4) {
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            float value = std::bit_cast<float>(bits);
            CHECK(std::isfinite(value));
            // Bilinear filter weights can have only eight fractional bits; edge
            // differences have unequal spans and do not cancel that quantization.
            CHECK(std::abs(value - expected_normal[unsigned(component)]) < 5e-4);
        }
    }
    // The actual wake kernel must produce two oppositely rotating eddies.
    const std::string solver = lighting::shaders::water_comp;
    const auto wake_start = solver.find("vec2 wake_pair");
    const auto wake_end = solver.find("void main()", wake_start);
    CHECK(wake_start != std::string::npos && wake_end > wake_start);
    std::string wake_source =
        "#version 410 core\n" + solver.substr(wake_start, wake_end - wake_start);
    wake_source += R"(
uniform int component;
layout(location=0) out vec4 result;
void main() {
    vec2 p=vec2(1.2,2)+gl_FragCoord.xy*.25;
    vec2 velocity=wake_pair(p,vec2(4),vec2(1,0),1.);
    uint bits=floatBitsToUint(velocity[component]);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader wake_test;
    CHECK(wake_test.build("counter-rotating water wake", vertex, wake_source.c_str(), error));
    wake_test.use();
    std::vector<Cell> wake_velocity(kCount);
    for (int c = 0; c < 2; ++c) {
        glUniform1i(wake_test.uniform("component"), c);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (unsigned i = 0; i < kCount; ++i) {
            unsigned at = i * 4;
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            wake_velocity[i][unsigned(c)] = std::bit_cast<float>(bits);
        }
    }
    const auto curl_at = [&](unsigned x, unsigned y) {
        unsigned i = y * kWidth + x;
        return (wake_velocity[i + 1][1] - wake_velocity[i - 1][1] - wake_velocity[i + kWidth][0]
                + wake_velocity[i - kWidth][0])
               / .5f;
    };
    CHECK(curl_at(8, 10) > .5f);
    CHECK(curl_at(8, 5) < -.5f);
    // Fine detail: analytic slope, disable, minification, seamless flow-phase
    // resets, mirrored-UV orientation, unit normals and time variation.
    const auto fine_start = material.find("// Fine ripples");
    const auto fine_end = material.find("vec3 effect_material");
    CHECK(fine_start != std::string::npos && fine_end > fine_start);
    const auto combine_start = material.find("vec3 water_smooth_normal");
    std::string fine_source = "#version 410 core\n"
                              + material.substr(combine_start, helper_start - combine_start)
                              + material.substr(fine_start, fine_end - fine_start);
    fine_source += R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%11;
    vec2 p=vec2(.13,.27),axis=vec2(1,0),velocity=vec2(.8,-.2);
    vec2 g=water_fine_gradient(p,vec2(0),axis,0.,vec2(0),vec2(0),1.);
    float value=0.;
    if(test==0) value=g.x;
    if(test==1) value=g.y;
    if(test==2) value=length(water_fine_gradient(p,velocity,axis,.4,vec2(0),vec2(0),0.));
    if(test==3) value=length(water_fine_gradient(p,velocity,axis,.4,vec2(1,0),vec2(0,1),1.));
    if(test==4) value=water_ripple_band(vec2(20,0),vec2(.2,0),vec2(0,.2));
    if(test==5) value=length(water_fine_gradient(p,velocity,axis,1.4999,vec2(0),vec2(0),1.)
                          -water_fine_gradient(p,velocity,axis,1.5001,vec2(0),vec2(0),1.));
    if(test==6) value=length(water_fine_gradient(p,velocity,axis,.7499,vec2(0),vec2(0),1.)
                          -water_fine_gradient(p,velocity,axis,.7501,vec2(0),vec2(0),1.));
    if(test==7) value=dot(water_art_direction(vec2(1,0),vec2(0,1),vec2(1,0),vec2(0,-1),axis),axis);
    if(test==8) value=length(water_smooth_normal(vec3(0,0,1),vec2(.2,-.1)+g));
    if(test==9) value=length(g-water_fine_gradient(p,velocity,axis,.3,vec2(0),vec2(0),1.))>.001?1.:0.;
    if(test==10) value=length(g-water_fine_gradient(p,vec2(0),axis,.3,vec2(0),vec2(0),1.))>.001?1.:0.;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader fine_test;
    CHECK(fine_test.build("fine water normals", vertex, fine_source.c_str(), error));
    fine_test.use();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const double tau = 6.28318531;
    const std::array<double, 2> k1{.94 * tau / 1.1, .34 * tau / 1.1},
        k2{-.35 * tau / .67, .94 * tau / .67}, bend_k{.61, .93}, envelope_k{.83, -.47};
    const double bend_phase = .13 * bend_k[0] + .27 * bend_k[1];
    const double envelope_phase = .13 * envelope_k[0] + .27 * envelope_k[1];
    const double envelope = .7 + .3 * std::sin(envelope_phase);
    const double phase1 = .13 * k1[0] + .27 * k1[1] + 1.6 * std::sin(bend_phase);
    const double phase2 = .13 * k2[0] + .27 * k2[1] - 1.1 * std::sin(bend_phase) + .8;
    const double height = .014 * std::sin(phase1) + .006 * std::sin(phase2);
    for (unsigned x = 0; x < kWidth; ++x) {
        unsigned at = x * 4;
        auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                    | (std::uint32_t(pixels[at + 2]) << 16) | (std::uint32_t(pixels[at + 3]) << 24);
        const float actual = std::bit_cast<float>(bits);
        const unsigned test = x % 11;
        CHECK(std::isfinite(actual));
        if (test < 2) {
            CHECK(
                std::abs(
                    actual
                    - (envelope
                           * (.014 * (k1[test] + 1.6 * bend_k[test] * std::cos(bend_phase))
                                  * std::cos(phase1)
                              + .006 * (k2[test] - 1.1 * bend_k[test] * std::cos(bend_phase))
                                    * std::cos(phase2))
                       + .3 * envelope_k[test] * std::cos(envelope_phase) * height)
                )
                < 2e-5
            );
        } else if (test <= 4) {
            CHECK(actual == 0);
        } else if (test <= 6) {
            CHECK(actual < 5e-4);
        } else {
            CHECK(std::abs(actual - 1) < unit_error);
        }
    }
    CHECK(enhanced::water_window_visible(renderer::identity(), {-.5f, -.5f, 0}, {.5f, .5f, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {3, 0, 0}, {4, 1, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {-4, 0, 0}, {-3, 1, 0}));
    CHECK(!enhanced::water_window_visible(renderer::identity(), {0, 3, 0}, {1, 4, 0}));
    CHECK(enhanced::water_window_visible(renderer::identity(), {1.1f, 0, 0}, {1.2f, .1f, 0}));
}
}  // namespace openrac::test::water
