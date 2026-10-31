// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

#include "common/profiler.h"
#include "common/stats.h"
#include "enhanced/renderer/atmosphere.h"
#include "enhanced/viewer/scene_lighting.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer_shaders.h"
#include "scene/level_renderer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif
using namespace openrac;
using namespace openrac::gl;

namespace {
PFN_glGetIntegerv actual_get_integer = nullptr;

void OPENRAC_GLAPI small_texel_buffer_limit(GLenum name, GLint* value) {
    if (name == GL_MAX_TEXTURE_BUFFER_SIZE) {
        *value = 128;
    } else {
        actual_get_integer(name, value);
    }
}
}  // namespace

int main(int argc, char** argv) {
    // Empty/repeated cleanup must be safe even when context creation has not happened.
    {
        enhanced::Atmosphere unused;
        unused.release();
        unused.release();
    }
    const bool profile = argc > 1 && std::string_view(argv[1]) == "--profile";
    if (profile && !profiling::wait_for_connection(std::chrono::seconds(15))) {
        return 1;
    }
    const auto low = renderer::volumetric_grid(renderer::VolumetricQuality::Low, 1920, 1080);
    const auto medium = renderer::volumetric_grid(renderer::VolumetricQuality::Medium, 1920, 1080);
    const auto high = renderer::volumetric_grid(renderer::VolumetricQuality::High, 1920, 1080);
    CHECK(low.width == 30 && low.height == 17 && low.slices == 24);
    CHECK(medium.width == 60 && medium.height == 34 && medium.slices == 48);
    CHECK(high.width == 120 && high.height == 68 && high.slices == 96);
    const auto cap = renderer::volumetric_grid(renderer::VolumetricQuality::High, 16384, 16384);
    CHECK(cap.width == 160 && cap.height == 90 && cap.slices == 96);
    CHECK(renderer::volumetric_grid(renderer::VolumetricQuality::Off, 1920, 1080).slices == 0);
    CHECK(renderer::volumetric_grid(renderer::VolumetricQuality::High, 0, 1080).slices == 0);
    enhanced::AtmosphereSettings options;
    options.height_density = std::numeric_limits<float>::quiet_NaN();
    options.anisotropy = 9;
    options.volumes[0].centre_radius[3] = -1;
    enhanced::set_atmosphere_settings(options);
    const auto safe = enhanced::atmosphere_settings();
    CHECK(
        safe.height_density == 0 && safe.anisotropy == .8f
        && safe.volumes[0].centre_radius[3] == .01f
    );
    enhanced::set_atmosphere_settings({});
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return test::kSkip;
    }
    if (!is_vulkan() || !enhanced_rendering_available()) {
        const std::string original = "#version 410 core\nvoid main(){}";
        CHECK(enhanced::Atmosphere::fragment_source(original) == original);
        return test::result();
    }
    enhanced::Atmosphere atmosphere;
    atmosphere.load();
    scene::LevelScene scene;
    renderer::FrameInput input;
    input.camera.view = renderer::identity();
    input.camera.projection = renderer::perspective_reversed(1.f, 1.f, .05f, 2000.f);
    // Nearly parallel fixture rays isolate integration from angular grid interpolation.
    input.camera.projection[0] = input.camera.projection[5] = 10000;
    renderer::FrameBuffer frame;
    CHECK(frame.create(64, 64, error));
    renderer::RenderTarget target{frame.id(), 64, 64};
    renderer::Shader read;
    const std::string fragment = enhanced::Atmosphere::fragment_source(R"(#version 430 core
uniform float test_depth;
uniform int test_channel,test_sky;
layout(location=0) out vec4 result;
void main(){
    vec4 value;
    if(test_sky==2) value=vec4(atmosphere_apply(vec3(.2),vec3(0,0,-test_depth),true),1);
    else if(test_sky==3) value=atmosphere_sky_sample();
    else value=atmosphere_sample(test_depth,test_sky!=0);
    uint bits=floatBitsToUint(value[test_channel]);
    result=vec4(float(bits&255u),float((bits>>8)&255u),float((bits>>16)&255u),float(bits>>24))/255.;
})");
    if (!read.build("atmosphere readback", renderer::shaders::gtao_vert, fragment, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        CHECK(false);
        return test::result();
    }
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    const auto sample = [&](float depth, int sky) {
        std::array<float, 4> result{};
        glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
        glViewport(0, 0, target.width, target.height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        read.use();
        atmosphere.bind(read);
        glUniform1f(read.uniform("test_depth"), depth);
        glUniform1i(read.uniform("test_sky"), sky);
        glBindVertexArray(vao);
        for (int c = 0; c < 4; ++c) {
            glUniform1i(read.uniform("test_channel"), c);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            std::array<std::uint8_t, 4> pixel{};
            glReadPixels(
                target.width / 2, target.height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data()
            );
            const auto bits = std::uint32_t(pixel[0]) | (std::uint32_t(pixel[1]) << 8)
                              | (std::uint32_t(pixel[2]) << 16) | (std::uint32_t(pixel[3]) << 24);
            result[c] = std::bit_cast<float>(bits);
            CHECK(std::isfinite(result[c]));
        }
        return result;
    };
    options = {};
    options.sun_strength = 0;
    options.distance = 100;
    const std::array<float, 4> colour{.4f, .5f, .6f, 1};
    const std::array<float, 4> fog{-255.f / (100.f * 1024.f), 255, 0, 255};
    // Authored haze alone follows the original transmission curve, without 8-bit F stepping.
    enhanced::set_atmosphere_settings(options);
    // On a small device, High must retain original fog instead of sampling a truncated view.
    actual_get_integer = glGetIntegerv;
    input.volumetric_quality = renderer::VolumetricQuality::High;
    glGetIntegerv = small_texel_buffer_limit;
    atmosphere.prepare(scene, input, target, colour, fog, true);
    glGetIntegerv = actual_get_integer;
    CHECK(sample(37.f, 2)[0] == .2f);
    // Lowering the tier must recover immediately; the failed request is not sticky.
    input.volumetric_quality = renderer::VolumetricQuality::Low;
    glGetIntegerv = small_texel_buffer_limit;
    atmosphere.prepare(scene, input, target, colour, fog, true);
    glGetIntegerv = actual_get_integer;
    CHECK(sample(37.f, 2)[0] != .2f);
    input.volumetric_quality = renderer::VolumetricQuality::Medium;
    atmosphere.prepare(scene, input, target, colour, fog, true);
    for (float depth : {0.f, .01f, 3.3f, 37.f, 80.f}) {
        const auto actual = sample(depth, false);
        renderer::render_opt::set(renderer::render_opt::Opt::ShaderVariants, false);
        atmosphere.prepare(scene, input, target, colour, fog, true);
        CHECK(sample(depth, false) == actual);
        renderer::render_opt::set(renderer::render_opt::Opt::ShaderVariants, true);
        atmosphere.prepare(scene, input, target, colour, fog, true);
        CHECK(sample(depth, false) == actual);
        const float expected = 1 - depth / 100;
        CHECK(std::abs(actual[3] - expected) < .012f);
        for (int c = 0; c < 3; ++c) {
            CHECK(std::abs(actual[c] - colour[c] * (1 - actual[3])) < 2e-5f);
        }
        const auto sky = sample(depth, true);
        CHECK(sky[3] == 1 && sky[0] == 0 && sky[1] == 0 && sky[2] == 0);
    }
    // Homogeneous height fog + authored haze multiply transmission; their source is integrated once.
    options.height_density = .02f;
    options.height_falloff = 0;
    enhanced::set_atmosphere_settings(options);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    for (float depth : {.001f, 1.2f, 17.3f, 59.f}) {
        const auto combined = sample(depth, false), local = sample(depth, true);
        const float expected = std::exp(-.02f * depth);
        CHECK(std::abs(local[3] - expected) < 2e-5f);
        CHECK(std::abs(combined[3] - (1 - depth / 100) * expected) < .012f);
        for (int c = 0; c < 3; ++c) {
            CHECK(std::abs(combined[c] - colour[c] * (1 - combined[3])) < 3e-5f);
        }
    }
    // Sky composition applies fog-only transport once and preserves destination alpha.
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glViewport(0, 0, target.width, target.height);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(.8f, .8f, .8f, .6f);
    glClear(GL_COLOR_BUFFER_BIT);
    atmosphere.draw_sky(target);
    const auto sky_pixels = frame.read_rgba();
    for (int c = 0; c < 3; ++c) {
        const float expected = colour[c] + (.8f - colour[c]) * std::exp(-2.f);
        CHECK(
            std::abs(int(sky_pixels[(32 * 64 + 32) * 4 + c]) - int(std::lround(expected * 255)))
            <= 2
        );
    }
    CHECK(sky_pixels[(32 * 64 + 32) * 4 + 3] == 153);
    const auto endpoint = sample(options.distance, 1), sky_endpoint = sample(options.distance, 3);
    for (int c = 0; c < 4; ++c) {
        CHECK(std::abs(endpoint[c] - sky_endpoint[c]) < 2e-6f);
    }
    // Column light culling must retain a light crossed later along the ray, while
    // rejecting distant/behind-camera lights without altering transport.
    options.haze = 0;
    enhanced::set_atmosphere_settings(options);
    std::array<float, 64> point_lights{};
    point_lights[0] = point_lights[1] = point_lights[2] = 1;
    point_lights[6] = -40;
    point_lights[7] = 10;
    scene.set_point_lights(point_lights);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    const auto lit = sample(60, false);
    double light_integral = 0;
    for (int i = 0; i < 20000; ++i) {
        const double distance = (i + .5) * 60. / 20000;
        const double attenuation = std::max(0., 1. - std::abs(distance - 40.) / 10.);
        light_integral += .02 * std::exp(-.02 * distance) * attenuation * 60. / 20000;
    }
    for (int c = 0; c < 3; ++c) {
        const double expected = colour[c] * (1. - std::exp(-1.2) + light_integral);
        CHECK(std::abs(lit[c] - expected) < .003);
    }
    point_lights[8] = point_lights[9] = point_lights[10] = 1000;
    point_lights[12] = 1000;
    point_lights[15] = 10;
    point_lights[16] = point_lights[17] = point_lights[18] = 1000;
    point_lights[22] = 40;
    point_lights[23] = 10;
    scene.set_point_lights(point_lights);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    const auto culled = sample(60, false);
    for (int c = 0; c < 4; ++c) {
        CHECK(std::abs(lit[c] - culled[c]) < 1e-6f);
    }
    scene.set_point_lights({});
    // A small sphere between coarse slice centres still contributes its optical thickness.
    options.height_density = 0;
    options.haze = 0;
    options.volumes[0].centre_radius = {0, 0, -40, .2f};
    options.volumes[0].colour_density = {.4f, .5f, .6f, .1f};
    enhanced::set_atmosphere_settings(options);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    const auto sphere = sample(60, false);
    CHECK(sphere[3] < .99f && sphere[3] > .95f);
    CHECK(sample(30, false)[3] == 1);
    // Empty columns stay exactly transparent even with active lights.
    options.volumes[0].colour_density[3] = 0;
    enhanced::set_atmosphere_settings(options);
    scene.set_point_lights(point_lights);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    const auto empty = sample(60, false);
    CHECK(empty[0] == 0 && empty[1] == 0 && empty[2] == 0 && empty[3] == 1);
    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
    glClearColor(.2f, .3f, .4f, .6f);
    glClear(GL_COLOR_BUFFER_BIT);
    const auto clear_sky = frame.read_rgba();
    atmosphere.draw_sky(target);
    CHECK(frame.read_rgba() == clear_sky);
    // Packing active media must retain a volume after disabled array slots.
    options.volumes[7] = options.volumes[0];
    options.volumes[7].colour_density[3] = .1f;
    enhanced::set_atmosphere_settings(options);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    CHECK(sample(60, false)[3] < .99f);
    scene.set_point_lights({});
    // Disabled providers and disabled feature controls cannot retain active transport.
    atmosphere.prepare(scene, input, target, colour, fog, false);
    read.use();
    atmosphere.bind(read);
    const auto disabled_provider = sample(60, 2);
    CHECK(std::abs(disabled_provider[0] - .2f) < 1e-6f);
    options.enabled = false;
    enhanced::set_atmosphere_settings(options);
    atmosphere.prepare(scene, input, target, colour, fog, true);
    const auto disabled = sample(60, 2);
    CHECK(
        std::abs(disabled[0] - .2f) < 1e-6f && std::abs(disabled[1] - .2f) < 1e-6f
        && std::abs(disabled[2] - .2f) < 1e-6f
    );
    // Resizing and moving the camera rebuild the transport rather than retaining a stale grid.
    options = {};
    options.sun_strength = 0;
    options.height_density = .02f;
    options.height_falloff = 0;
    enhanced::set_atmosphere_settings(options);
    CHECK(frame.create(113, 79, error));
    target = {frame.id(), 113, 79};
    input.camera.position = {4, 2, 7};
    input.camera.view = renderer::look_along(input.camera.position, {0, 1, 0}, {0, 0, 1});
    atmosphere.prepare(scene, input, target, colour, fog, true);
    CHECK(std::abs(sample(17.3f, true)[3] - std::exp(-.02f * 17.3f)) < 2e-5f);
    // Switching quality updates both allocation size and the consumer's slice mapping.
    for (auto quality :
         {renderer::VolumetricQuality::High,
          renderer::VolumetricQuality::Low,
          renderer::VolumetricQuality::Off,
          renderer::VolumetricQuality::Medium}) {
        input.volumetric_quality = quality;
        atmosphere.prepare(scene, input, target, colour, fog, true);
        if (quality == renderer::VolumetricQuality::Off) {
            CHECK(std::abs(sample(17.3f, 2)[0] - .2f) < 1e-6f);
        } else {
            CHECK(std::abs(sample(17.3f, true)[3] - std::exp(-.02f * 17.3f)) < 2e-5f);
        }
    }
    // Compile the actual effect adapter with the core particle shader.
    lighting::use_scene_lighting(scene);
    renderer::Renderer renderer;
    renderer::RenderState state{renderer.textures(), target, {}};
    renderer::EffectRenderer effects(
        "atmospheric particles",
        renderer::EffectPart::All,
        renderer::Bucket::Particles,
        enhanced::atmosphere_material(scene)
    );
    const bool effect_ready = effects.init(state, error);
    if (!effect_ready) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }
    CHECK(effect_ready);
    effects.release();
    // Optional synthetic GPU workload: production compute, surface lookup and sky composition.
    // No scene shadows are available here; this is not a substitute for an in-game capture.
    if (profile) {
        CHECK(frame.create(1920, 1080, error));
        target = {frame.id(), 1920, 1080};
        input.camera.position = {0, 0, 0};
        input.camera.view = renderer::identity();
        input.camera.projection = renderer::perspective_reversed(1.f, 1920.f / 1080, .05f, 2000.f);
        options = {};
        options.height_density = .002f;
        options.height_falloff = 0;
        options.volumes[0].centre_radius = {0, 0, -40, 12};
        options.volumes[0].colour_density = {.4f, .5f, .6f, .02f};
        enhanced::set_atmosphere_settings(options);
        std::array<float, 64> points{};
        for (int i = 0; i < 8; ++i) {
            points[i * 8] = .3f;
            points[i * 8 + 1] = .2f;
            points[i * 8 + 2] = .1f;
            points[i * 8 + 4] = float(i - 4) * 30;
            points[i * 8 + 6] = -50;
            points[i * 8 + 7] = 20;
        }
        scene.set_point_lights(points);
        renderer::Shader surface;
        CHECK(surface.build(
            "atmosphere benchmark surface",
            renderer::shaders::gtao_vert,
            enhanced::Atmosphere::fragment_source(R"(#version 430 core
layout(location=0) out vec4 result;
void main(){
    float depth=1.+99.*gl_FragCoord.x/1920.;
    vec4 transport=atmosphere_sample(depth,false);
    result=vec4(vec3(.2)*transport.a+transport.rgb,1);
})"),
            error
        ));
        renderer::Shader reference_sky;
        CHECK(reference_sky.build(
            "atmosphere reference sky",
            renderer::shaders::gtao_vert,
            enhanced::Atmosphere::fragment_source(R"(#version 430 core
layout(location=0) out vec4 result;
void main(){
    vec4 fog=atmosphere_sample(atmosphere_far,true);
    result=vec4(fog.rgb,1.-fog.a);
})"),
            error
        ));
        const auto draw_reference_sky = [&] {
            const stats::Scope scope("atmosphere reference sky", stats::kGpu);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
            reference_sky.use();
            atmosphere.bind(reference_sky);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        };
        const std::array qualities{
            renderer::VolumetricQuality::Low,
            renderer::VolumetricQuality::Medium,
            renderer::VolumetricQuality::High
        };
        const std::array names{
            "atmosphere benchmark low", "atmosphere benchmark medium", "atmosphere benchmark high"
        };
        for (std::size_t tier = 0; tier < qualities.size(); ++tier) {
            input.volumetric_quality = qualities[tier];
            for (int iteration = 0; iteration < 600; ++iteration) {
                {
                    const stats::Scope scope(names[tier], stats::kGpu);
                    atmosphere.prepare(scene, input, target, colour, fog, true);
                    glBindFramebuffer(GL_FRAMEBUFFER, frame.id());
                    glViewport(0, 0, target.width, target.height);
                    surface.use();
                    atmosphere.bind(surface);
                    glBindVertexArray(vao);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_BLEND);
                    {
                        const stats::Scope lookup("atmosphere surface lookup", stats::kGpu);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }
                    // Alternate order to avoid giving either path a consistent cache advantage.
                    if (iteration % 2 == 0) {
                        draw_reference_sky();
                        atmosphere.draw_sky(target);
                    } else {
                        atmosphere.draw_sky(target);
                        draw_reference_sky();
                    }
                }
                glFinish();
                context.window().swap();
                profiling::frame();
            }
        }
    }
    enhanced::set_atmosphere_settings({});
    glDeleteVertexArrays(1, &vao);
    CHECK(glGetError() == GL_NO_ERROR);
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
