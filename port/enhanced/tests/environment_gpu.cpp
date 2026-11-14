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
#include "renderer/gl/gl.h"
#include "scene_shaders.h"
#include "tests/check.h"

namespace openrac::test::water {
// Check packed captures, interface-side selection and unchanged opaque material reflection.
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
uniform int interface_side;
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x)%11;float value;
    if(test==0) value=environment_specular(vec3(0),vec3(0,0,1),vec3(0,0,1),0.,.04,vec3(.1,.2,.3)).r;
    if(test==1) value=environment_specular(vec3(9,0,0),vec3(0,0,1),vec3(0,0,1),0.,.04,vec3(.1,.2,.3)).g;
    if(test==2) value=environment_project(0,vec3(2,1,0),vec3(1,0,0)).x;
    if(test==3) value=environment_project(0,vec3(2,1,0),vec3(1,0,0)).y;
    if(test==4) value=environment_specular(vec3(0),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.1,.2,.3)).r;
    if(test==5) value=environment_specular(vec3(0,0,-4.75),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.8)).r;
    if(test==6) value=environment_specular_surface(vec3(0,0,-4.75),vec3(0,0,-4.25),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.8)).r;
    if(test==7) value=environment_specular_surface(vec3(0),vec3(0,0,.5),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.8)).r;
    if(test==8) value=environment_specular_surface(vec3(9,0,0),vec3(9,0,.5),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.1,.2,.3)).g;
    if(test==9) value=environment_specular_surface(vec3(0,0,4.75),vec3(0,0,4.25),vec3(0,0,-1),vec3(0,0,-1),1.,.04,vec3(.8)).r;
    if(test==10) value=environment_specular_surface(vec3(0,0,-4.75),vec3(0,0,100),vec3(0,0,1),vec3(0,0,1),1.,.04,vec3(.8)).r;
    if(interface_side!=0) {
        vec3 side=vec3(0,0,float(interface_side));
        vec3 colour=environment_specular_surface(vec3(0),side*.5,side,side,1.,.04,vec3(.8));
        value=colour[int(gl_FragCoord.x)%3];
    }
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
    unsigned brdf_texture = 0;
    glGenTextures(1, &brdf_texture);
    glActiveTexture(GL_TEXTURE0 + 24);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
    std::vector<std::uint32_t> lookup(64 * 64);
    for (unsigned y = 0; y < 64; ++y) {
        std::copy_n(
            asset.pixels.begin() + std::size_t(asset.brdf_row() + y) * asset.width(),
            64,
            lookup.begin() + y * 64
        );
    }
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R11F_G11F_B10F,
        64,
        64,
        0,
        GL_RGB,
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        lookup.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(shader.uniform("environment_brdf"), 24);
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
    const float captured = (brdf[0] * .04f + brdf[1]) * .5f;
    const float edge_coverage = (1.f / 3) * (1.f / 3) * (3 - 2.f / 3);
    const std::array<float, 11> expected{
        .02f,
        .2f,
        6,
        1,
        captured,
        .8f * (1 - edge_coverage) + captured * edge_coverage,
        captured,
        captured,
        .2f,
        captured,
        captured
    };

    for (unsigned x = 0; x < kWidth; ++x) {
        unsigned bits = 0;

        for (unsigned c = 0; c < 4; ++c) {
            bits |= unsigned(fixture.pixels[x * 4 + c]) << (c * 8);
        }

        const auto value = std::bit_cast<float>(bits);
        CHECK(std::isfinite(value));
        CHECK(std::abs(value - expected[x % 11]) < .001f);
    }

    // An interface overlaps the floor fade bands of two differently coloured reflection media.
    // Select the viewing side without moving the true surface/parallax origin.
    auto media = asset.probes;
    media[0].minimum[2] = -.25f;
    media[1].maximum[2] = .25f;
    CHECK(reference_selection.prepare(lighting::environment_metadata(media), fixture.error));
    reference_selection.bind(shader);
    for (const int side : {1, -1}) {
        glUniform1i(shader.uniform("interface_side"), side);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
        for (unsigned x = 0; x < kWidth; ++x) {
            unsigned bits = 0;
            for (unsigned c = 0; c < 4; ++c) {
                bits |= unsigned(fixture.pixels[x * 4 + c]) << (c * 8);
            }
            const float value = std::bit_cast<float>(bits);
            const float expected_medium = x % 3 == (side == 1 ? 0u : 2u) ? captured * 2 : 0;
            CHECK(std::isfinite(value) && std::abs(value - expected_medium) < .001f);
        }
    }
    glUniform1i(shader.uniform("interface_side"), 0);
    CHECK(reference_selection.prepare(metadata, fixture.error));
    reference_selection.bind(shader);

    // Metal packets use standard shading over their geometry when enhanced rendering is active.
    const char* metal_vertex = R"(#version 410 core
out vec2 v_uv; out vec4 v_colour; out vec3 v_normal; out vec3 v_world_position;
out vec4 v_probe_multiplier; flat out vec4 v_probe_selector;
flat out int v_lit; flat out int v_glow; out vec3 v_script_emission; flat out vec3 v_probe_ambient; out vec3 v_emission_colour;
out vec2 v_fog_w;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1); v_uv=vec2(.5); v_colour=vec4(1); v_normal=vec3(0,0,1);
    v_world_position=vec3(0,0,-2); v_probe_multiplier=vec4(1); v_probe_selector=vec4(0);
    v_lit=1; v_glow=0; v_script_emission=vec3(0); v_probe_ambient=vec3(0); v_emission_colour=vec3(1); v_fog_w=vec2(1.);
})";
    renderer::Shader chrome;
    CHECK(chrome.build(
        "environment material reflections",
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
    glUniform1i(chrome.uniform("environment_brdf"), 24);
    glActiveTexture(GL_TEXTURE0 + 24);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
    glActiveTexture(GL_TEXTURE0);
    const auto view = renderer::identity();
    glUniformMatrix4fv(chrome.uniform("view"), 1, GL_FALSE, view.data());
    glUniform1i(chrome.uniform("metal_pass"), 1);
    glUniform4f(chrome.uniform("material_colour"), 1, 1, 1, 1);
    glUniform1f(chrome.uniform("material_diffuse_scale"), 1);
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
    const auto draw = [&](const renderer::Shader& program, int count) {
        glUniform1i(program.uniform("environment_count"), count);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glReadPixels(0, 0, kWidth, kWidth, GL_RGBA, GL_UNSIGNED_BYTE, fixture.pixels.data());
        return std::array{fixture.pixels[0], fixture.pixels[1], fixture.pixels[2]};
    };
    const auto draw_chrome = [&](bool enabled, float strength, int count) {
        glUniform1i(chrome.uniform("object_reflection_enabled"), enabled ? 1 : 0);
        glUniform3f(chrome.uniform("object_reflection_material"), .04f, .65f, strength);
        return draw(chrome, count);
    };
    const auto reflected = draw_chrome(true, 1, 2);
    CHECK(reflected[0] > 80 && reflected[1] > 100 && reflected[2] > 80);
    const auto legacy = draw_chrome(false, 1, 2);
    CHECK(legacy[0] == 0 && legacy[1] == 255 && legacy[2] == 0);
    const auto no_capture = draw_chrome(true, 1, 0);
    CHECK(no_capture[0] == 0 && no_capture[1] > 100 && no_capture[2] == 0);
    CHECK(draw_chrome(true, 0, 2) == no_capture);

    auto solid_source = std::string(scene::shaders::mesh_frag);
    solid_source.insert(
        solid_source.find('\n') + 1,
        "#define OPENRAC_SOLID 1\n#define OPENRAC_COLOUR_SPECIALISED 1\n"
        "#define OPENRAC_STATIC_COLOUR 1\n"
    );
    renderer::Shader solid;
    CHECK(solid.build(
        "solid static dielectric",
        metal_vertex,
        lighting::probe_mesh_fragment(std::move(solid_source)),
        fixture.error
    ));
    solid.use();
    probes.bind(solid, scene::Layer::Terrain, false);
    reference_selection.bind(solid);
    glUniformMatrix4fv(solid.uniform("view"), 1, GL_FALSE, view.data());
    glUniform1i(solid.uniform("environment_brdf"), 24);
    glUniform1i(solid.uniform("environment_map"), 20);
    glUniform1i(solid.uniform("environment_resolution"), 8);
    glUniform1i(solid.uniform("environment_columns"), static_cast<int>(asset.columns()));
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0 + 24);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, original);

    // Both colour variants retain capture and direct highlights without valid diffuse GI.
    chrome.use();
    glUniform1i(chrome.uniform("metal_pass"), 0);
    for (const auto* program : {&chrome, &solid}) {
        program->use();
        glUniform4f(program->uniform("material_colour"), 1, 1, 1, 1);
        glUniform3f(program->uniform("material_dielectric"), .25f, .04f, 1);
        glUniform3f(program->uniform("material_dielectric_tint"), 1, 1, 1);
        // These are the regular draw programs. Develop builds the coverage view separately.
        glUniform1i(program->uniform("probe_mode"), 1);
        const auto capture = draw(*program, 2);
        const auto no_specular = draw(*program, 0);
        CHECK(capture[0] > no_specular[0] && capture[2] > no_specular[2]);
        CHECK(no_specular[0] == 0 && no_specular[1] > 240 && no_specular[2] == 0);
        std::array<float, 256> dielectric_lights{};
        dielectric_lights[0] = .5f;
        dielectric_lights[6] = -1;
        glUniform4fv(program->uniform("light_sets[0]"), 64, dielectric_lights.data());
        CHECK(draw(*program, 0)[0] > no_specular[0]);
        dielectric_lights[0] = 0;
        glUniform4fv(program->uniform("light_sets[0]"), 64, dielectric_lights.data());
    }
    chrome.use();
    glUniform1i(chrome.uniform("metal_pass"), 1);

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
    glUniform1i(chrome.uniform("environment_brdf"), 24);
    glActiveTexture(GL_TEXTURE0 + 24);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
    glActiveTexture(GL_TEXTURE0);
    reference_selection.bind(chrome);
    glUniform1i(chrome.uniform("probe_mode"), 1);
    glUniform1i(chrome.uniform("probe_receiver"), 2);
    glUniform1f(chrome.uniform("probe_gi_multiplier"), 1);
    glUniform4f(chrome.uniform("object_reflection_tint"), 1, 1, 1, 1);
    glUniform1f(chrome.uniform("material_diffuse_scale"), 0);
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
    // The textured colour slot keeps its artwork at full metallic and under direct light.
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
    glUniform1i(chrome.uniform("metal_pass"), 0);
    glUniform3f(chrome.uniform("material_dielectric"), .5f, .04f, 1);
    glUniform3f(chrome.uniform("material_dielectric_tint"), 1, 1, 1);
    glUniform1f(chrome.uniform("material_diffuse_scale"), 1);
    glUniform1f(chrome.uniform("material_metallic"), 2);
    glUniform4f(chrome.uniform("object_reflection_tint"), 1, 1, 1, .3f);
    const auto green_metal = draw(chrome, 2);
    CHECK(green_metal[0] == 0 && green_metal[1] > 0 && green_metal[2] == 0);
    CHECK(fixture.pixels[3] == 255);
    const std::array<std::uint8_t, 4> red{255, 0, 0, 255};
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, original);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, red.data());
    const auto red_metal = draw(chrome, 2);
    CHECK(red_metal[0] > 0 && red_metal[1] == 0 && red_metal[2] == 0);
    // Full metallic retains measured F0 magnitude instead of replacing it with white artwork.
    glUniform3f(chrome.uniform("material_dielectric"), .5f, .8f, 1);
    const auto bright_metal = draw(chrome, 2);
    CHECK(bright_metal[0] > red_metal[0]);
    glUniform3f(chrome.uniform("material_dielectric"), .5f, .4f, 1);
    const auto darker_metal = draw(chrome, 2);
    CHECK(darker_metal[0] > red_metal[0] && darker_metal[0] < bright_metal[0]);
    glUniform3f(chrome.uniform("material_dielectric"), .5f, .04f, 0);
    CHECK((draw(chrome, 2) == std::array<std::uint8_t, 3>{0, 0, 0}));
    glUniform1f(chrome.uniform("material_metallic"), 1);
    CHECK(draw(chrome, 2)[0] > 0);
    probes.release();
    glDeleteTextures(1, &original);
    reference_selection.release();
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &brdf_texture);
    glActiveTexture(GL_TEXTURE0);
}
}  // namespace openrac::test::water
