// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Production reflection shader: atlas addressing, two-box blending, BRDF lookup and fallback.
#include <bit>
#include <cmath>

#include "enhanced/lighting/environment_filter.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/tests/water_gpu_fixture.h"
#include "enhanced/viewer/environment_selection.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "scene_shaders.h"
#include "tests/check.h"

namespace openrac::test::water {
void check_environment(Context& fixture) {
    using namespace gl;
    lighting::EnvironmentAsset asset;
    asset.resolution = 8;
    asset.probes = {{{-1, 0, 0}, {-5, -5, -5}, {5, 5, 5}}, {{1, 0, 0}, {-5, -5, -5}, {5, 5, 5}}};
    asset.pixels.resize(std::size_t(asset.width()) * asset.height());

    for (unsigned i = 0; i < 2; ++i) {
        const std::vector<lighting::Float3> faces(
            6 * 8 * 8, i == 0 ? lighting::Float3{1, 0, 0} : lighting::Float3{0, 0, 1}
        );
        lighting::filter_environment(asset, i, faces, 32);
    }

    lighting::integrate_environment_brdf(asset, 128);
    const std::string source =
        std::string("#version 410 core\n") + std::string(lighting::shaders::environment_glsl) + R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%5;float value;
    if(test==0) value=environment_specular(vec3(0),vec3(0,0,1),vec3(0,0,1),0.,.04,vec3(.1,.2,.3)).r;
    if(test==1) value=environment_specular(vec3(9,0,0),vec3(0,0,1),vec3(0,0,1),0.,.04,vec3(.1,.2,.3)).g;
    if(test==2) value=environment_project(0,vec3(2,1,0),vec3(1,0,0)).x;
    if(test==3) value=environment_project(0,vec3(2,1,0),vec3(1,0,0)).y;
    if(test==4) value=environment_specular(vec3(0),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.1,.2,.3)).r;
    uint bits=floatBitsToUint(value);
    result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";
    renderer::Shader shader;
    CHECK(shader.build("environment box blending", fixture.vertex, source, fixture.error));
    shader.use();
    lighting::EnvironmentSelection reference_selection;
    reference_selection.bind(shader);
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R11F_G11F_B10F,
        int(asset.width()),
        int(asset.height()),
        0,
        GL_RGB,
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        asset.pixels.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    const auto metadata = lighting::environment_metadata(asset.probes);

    glUniform1i(shader.uniform("environment_count"), 2);
    glUniform1i(shader.uniform("environment_resolution"), 8);
    glUniform1i(shader.uniform("environment_map"), 20);
    CHECK(reference_selection.prepare(metadata, fixture.error));
    reference_selection.bind(shader);
    glUniform1i(shader.uniform("environment_columns"), static_cast<int>(asset.columns()));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
    const auto brdf = lighting::unpack_environment_colour(
        asset.pixels[std::size_t(asset.brdf_row() + 63) * asset.width() + 63]
    );
    const std::array<float, 5> expected{.02f, .2f, 6, 1, (brdf[0] * .04f + brdf[1]) * .5f};

    for (unsigned x = 0; x < kWidth; ++x) {
        unsigned bits = 0;

        for (unsigned c = 0; c < 4; ++c) {
            bits |= unsigned(fixture.pixels[x * 4 + c]) << (c * 8);
        }

        const auto value = std::bit_cast<float>(bits);
        CHECK(std::isfinite(value));
        CHECK(std::abs(value - expected[x % 5]) < .001f);
    }

    // The production chrome branch replaces the old material reflection, not its alpha or siblings.
    const char* metal_vertex = R"(#version 410 core
out vec2 v_uv; out vec4 v_colour; out vec3 v_normal; out vec3 v_world_position;
out vec4 v_probe_multiplier; flat out vec4 v_probe_selector;
flat out int v_lit; flat out int v_glow; out vec3 v_script_emission; out vec3 v_emission_colour;
out vec2 v_fog_w;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1); v_uv=vec2(.5); v_colour=vec4(1); v_normal=vec3(0,0,1);
    v_world_position=vec3(0,0,-2); v_probe_multiplier=vec4(1); v_probe_selector=vec4(0);
    v_lit=1; v_glow=0; v_script_emission=vec3(0); v_emission_colour=vec3(1); v_fog_w=vec2(1.);
})";
    renderer::Shader chrome;
    CHECK(chrome.build(
        "environment chrome material",
        metal_vertex,
        lighting::probe_mesh_fragment(scene::shaders::mesh_frag),
        fixture.error
    ));
    lighting::ProbeGrid gi_grid;
    gi_grid.origin = {-8, -8, -8};
    gi_grid.spacing = {16, 16, 16};
    auto volume = lighting::ProbeVolume::allocate(gi_grid);
    lighting::ProbeLighting probes;
    CHECK(probes.upload(gpu_probes(volume), fixture.error));
    chrome.use();
    probes.bind(chrome, scene::Layer::Mobys, false);
    const auto view = renderer::identity();
    glUniformMatrix4fv(chrome.uniform("view"), 1, GL_FALSE, view.data());
    glUniform1i(chrome.uniform("metal_pass"), 1);
    glUniform4f(chrome.uniform("object_reflection_tint"), 1, 1, 1, 1);
    glUniform1i(chrome.uniform("probe_receiver"), 2);
    glUniform1i(chrome.uniform("probe_mode"), 1);
    glUniform1i(chrome.uniform("environment_count"), 2);
    glUniform1i(chrome.uniform("environment_map"), 20);
    glUniform1i(chrome.uniform("environment_resolution"), 8);
    reference_selection.bind(chrome);
    glUniform1i(chrome.uniform("environment_columns"), static_cast<int>(asset.columns()));
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, texture);
    unsigned original = 0;
    glGenTextures(1, &original);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, original);
    const std::array<std::uint8_t, 4> green{0, 255, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, green.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const auto draw_chrome = [&](bool enabled, float strength, int count) {
        glUniform1i(chrome.uniform("object_reflection_enabled"), enabled ? 1 : 0);
        glUniform3f(chrome.uniform("object_reflection_material"), .04f, .65f, strength);
        glUniform1i(chrome.uniform("environment_count"), count);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
        return std::array{fixture.pixels[0], fixture.pixels[1], fixture.pixels[2]};
    };
    const auto reflected = draw_chrome(true, 1, 2);
    CHECK(reflected[0] > 80 && reflected[1] < 5 && reflected[2] > 80);
    const auto legacy = draw_chrome(false, 1, 2);
    CHECK(legacy[0] == 0 && legacy[1] == 255 && legacy[2] == 0);
    CHECK(draw_chrome(true, 1, 0) == legacy);
    CHECK((draw_chrome(true, 0, 2) == std::array<std::uint8_t, 3>{0, 0, 0}));

    // Missing reflection captures use local GI rather than the old green chrome artwork.
    for (auto& probe : volume.probes) {
        probe.valid = true;
    }

    for (auto& pixel : volume.irradiance) {
        pixel = {0, 0, 3.14159265f * .2f};
    }

    for (auto& moment : volume.distance_moments) {
        moment = {100, 10000};
    }

    CHECK(probes.upload(gpu_probes(volume), fixture.error));
    chrome.use();
    probes.bind(chrome, scene::Layer::Mobys, false);
    reference_selection.bind(chrome);
    glUniform1i(chrome.uniform("probe_mode"), 1);
    glUniform1i(chrome.uniform("probe_receiver"), 2);
    glUniform1f(chrome.uniform("probe_gi_multiplier"), 1);
    glUniform4f(chrome.uniform("object_reflection_tint"), 1, 1, 1, 1);
    const auto gi_fallback = draw_chrome(true, 1, 0);
    CHECK(gi_fallback[0] == 0 && gi_fallback[1] == 0 && gi_fallback[2] > 80);
    glUniform1f(chrome.uniform("probe_gi_multiplier"), 2);
    const auto brighter_gi = draw_chrome(true, 1, 0);
    CHECK(brighter_gi[2] > gi_fallback[2] && brighter_gi[0] == 0 && brighter_gi[1] == 0);
    glUniform1f(chrome.uniform("probe_gi_multiplier"), 1);
    auto remote = metadata;

    for (auto& row : remote) {
        row[0] += 50;
    }

    CHECK(reference_selection.prepare(remote, fixture.error));
    reference_selection.bind(chrome);
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, texture);
    CHECK(
        draw_chrome(true, 1, 2) == gi_fallback
    );  // Atlas exists, but its volumes miss this receiver.
    CHECK(reference_selection.prepare(metadata, fixture.error));
    reference_selection.bind(chrome);
    // Isolate direct GGX with a black environment, then verify linear tint and authored opacity.
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, texture);
    std::fill(asset.pixels.begin(), asset.pixels.end(), 0);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R11F_G11F_B10F,
        int(asset.width()),
        int(asset.height()),
        0,
        GL_RGB,
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        asset.pixels.data()
    );
    std::array<float, 256> lights{};
    lights[0] = lights[1] = lights[2] = .5f;
    lights[6] = -1;
    glUniform4fv(chrome.uniform("light_sets[0]"), 64, lights.data());
    glUniform1i(chrome.uniform("object_reflection_enabled"), 1);
    glUniform3f(chrome.uniform("object_reflection_material"), .5f, .04f, 1);
    glUniform4f(chrome.uniform("object_reflection_tint"), .5f, 1, 0, .3f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
    const auto display = [](float value) {
        return 255.f
               * (value <= .0031308f ? 12.92f * value
                                     : 1.055f * std::pow(value, 1.f / 2.4f) - .055f);
    };
    const float direct = .5f * .04f / (4.f * 3.14159265f * .0625f);
    CHECK(std::abs(float(fixture.pixels[0]) - display(direct * .5f)) < 2);
    CHECK(std::abs(float(fixture.pixels[1]) - display(direct)) < 2);
    CHECK(fixture.pixels[2] == 0);
    CHECK(std::abs(int(fixture.pixels[3]) - 77) <= 1);
    probes.release();
    glDeleteTextures(1, &original);
    reference_selection.release();
    glDeleteTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
}
}  // namespace openrac::test::water
