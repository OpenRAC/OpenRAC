// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>

#include "enhanced/lighting/environment.h"
#include "enhanced/renderer/water.h"
#include "enhanced/renderer/wetness.h"
#include "enhanced/tests/gpu_probe_fixture.h"
#include "enhanced/tests/water_test_bake.h"
#include "enhanced/viewer/environment_selection.h"
#include "enhanced/viewer/scene_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"
#include "scene_shaders.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/vkgl.h"
#endif
using namespace openrac;
using namespace openrac::gl;
#ifdef OPENRAC_VULKAN
void ggx_coat_reference() {
    // Compile the production wet-coat function, not a second shader implementation.
    std::string mesh(scene::shaders::mesh_frag);
    const auto begin = mesh.find("vec3 wet_coat(");
    const auto end = mesh.find("\n#else", begin);
    CHECK(begin != std::string::npos && end != std::string::npos);
    std::string fragment = "#version 430 core\n";
    fragment += lighting::shaders::shading_brdf_glsl;
    fragment += lighting::shaders::environment_glsl;
    fragment += R"(
vec4 hero_wetness_material=vec4(.34,1.07,.51,.35);
float hero_wetness_f0=.0633;
float v_hero_wetness;
vec3 v_world_position=vec3(0);
uniform int reflection_outside;
vec4 v_probe_selector;
vec4 probe_sunlight_direction=vec4(0);
vec3 probe_sunlight_scale=vec3(1);
vec4 light_sets[64];
vec3 probe_unit(vec3 v){return length(v)>0.?normalize(v):vec3(0);}
)";
    fragment += mesh.substr(begin, end - begin);
    fragment += R"(
layout(location=0) out vec4 result;
void main() {
    int test=int(gl_FragCoord.x);
    v_world_position=reflection_outside!=0?vec3(8):vec3(0);
    v_hero_wetness=test==0?0.:(test==2?.5:1.);
    v_probe_selector=vec4(0,1,test==7?.75:.25,1);
    light_sets[0]=vec4(1); light_sets[4]=vec4(3);
    light_sets[1]=light_sets[5]=vec4(0,0,test==4?1.:-1.,0);
    vec3 eye=test==5?vec3(1,0,0):(test==6?vec3(0,0,-1):vec3(0,0,1));
    float value=wet_coat(vec3(.25),vec3(0,0,1),eye,test==3?0.:1.).r;
    uint b=floatBitsToUint(value);
    result=vec4(b&255u,(b>>8)&255u,(b>>16)&255u,b>>24)/255.;
})";
    renderer::Shader shader;
    std::string error;
    // The analytical CPU oracle is FP32; exercise mixed precision in rendered_surface instead.
    const bool fp16 = renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderFloat16);
    renderer::render_opt::set(renderer::render_opt::Opt::ShaderFloat16, false);
    const bool built = shader.build(
        "wet coat GGX reference",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        fragment,
        error
    );
    renderer::render_opt::set(renderer::render_opt::Opt::ShaderFloat16, fp16);
    if (!built) {
        std::printf("%s\n", error.c_str());
    }
    CHECK(built);
    if (!built) {
        return;
    }
    renderer::FrameBuffer target;
    CHECK(target.create(8, 1, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    shader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 8, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // A known atlas separates reflection strength and roughness from direct-light GGX.
    constexpr unsigned kAtlasSide = 64, kCaptureRows = 13, kAtlasHeight = kCaptureRows + kAtlasSide;
    std::vector<float> atlas(kAtlasSide * kAtlasHeight * 4, 0);
    for (unsigned y = 0; y < kAtlasHeight; ++y) {
        for (unsigned x = 0; x < kAtlasSide; ++x) {
            const float radiance = y < 6 ? 2.f : (y < 10 ? 4.f : 6.f);
            atlas[(y * kAtlasSide + x) * 4] = y < kCaptureRows ? radiance : 1.f;
        }
    }
    unsigned environment = 0;
    glGenTextures(1, &environment);
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, environment);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        renderer::world::gl::GL_RGBA32F,
        kAtlasSide,
        kAtlasHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        atlas.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glActiveTexture(GL_TEXTURE0);
    lighting::EnvironmentSelection selection;
    const std::array<lighting::EnvironmentProbe, 1> probes{{{{0, 0, 0}, {-4, -4, -4}, {4, 4, 4}}}};
    CHECK(selection.prepare(lighting::environment_metadata(probes), error));
    selection.bind(shader);
    glUniform1i(shader.uniform("environment_map"), 20);
    glUniform1i(shader.uniform("environment_resolution"), 8);
    glUniform1i(shader.uniform("environment_columns"), 1);
    for (int capture = 0; capture <= 2; ++capture) {
        glUniform1i(shader.uniform("environment_count"), capture != 0 ? 1 : 0);
        glUniform1i(shader.uniform("reflection_outside"), capture == 2 ? 1 : 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::array<unsigned char, 32> pixels{};
        glReadPixels(0, 0, 8, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        for (unsigned i = 0; i < 8; ++i) {
            unsigned at = i * 4;
            auto bits = std::uint32_t(pixels[at]) | (std::uint32_t(pixels[at + 1]) << 8)
                        | (std::uint32_t(pixels[at + 2]) << 16)
                        | (std::uint32_t(pixels[at + 3]) << 24);
            double actual = std::bit_cast<float>(bits);
            double wet = i == 0 ? 0. : (i == 2 ? .5 : 1.);
            double roughness = .51 + (.35 - .51) * wet;
            // At normal incidence GGX*Smith*F*NoL reduces to F0/(4 pi roughness^4).
            double specular =
                (i >= 3 && i <= 6) ? 0. : .0633 / (4 * 3.141592653589793 * std::pow(roughness, 4));
            const double lod = std::max(roughness, 1. / 3.) * 3 - 1;
            const double reflected = capture == 1 && i != 5 && i != 6 ? (2 + 2 * lod) * .0633 : 0;
            double expected =
                .25 * (1 - .66 * wet) + wet * 1.07 * ((i == 7 ? 2.5 : 1.5) * specular + reflected);
            const double tolerance = 3e-5 * std::max(1., std::abs(expected));
            if (std::abs(actual - expected) >= tolerance) {
                std::printf(
                    "Wet coat capture %d case %u: GPU %.9g reference %.9g\n",
                    capture,
                    i,
                    actual,
                    expected
                );
            }
            CHECK(std::isfinite(actual) && std::abs(actual - expected) < tolerance);
        }
    }
    selection.release();
    glDeleteTextures(1, &environment);
    glDeleteVertexArrays(1, &vao);
}

void rendered_surface() {
    scene::LevelData data;
    data.vertices.resize(3);  // The hero starts at a nonzero global vertex ID.
    for (int model = 0; model < 2; ++model) {
        for (auto p : std::array<renderer::Vec3, 3>{{{1, 1, .7f}, {3, 1, .7f}, {2, 3, .7f}}}) {
            scene::Vertex vertex{};
            std::copy(p.begin(), p.end(), vertex.position);
            vertex.normal[2] = 1;
            std::fill_n(vertex.colour, 3, .5f);
            vertex.colour[3] = 1;
            vertex.weights[0] = 1;
            vertex.light_slot = -1;
            data.indices.push_back(unsigned(data.vertices.size()));
            data.vertices.push_back(vertex);
        }
        data.models.emplace_back();
        data.models.back().primitives.push_back({unsigned(model * 3), 3});
    }
    data.materials.resize(1);
    data.light_sets.resize(256);
    data.moby_classes[0] = {0, false};
    renderer::Renderer renderer;
    scene::LevelScene scene;
    lighting::use_scene_lighting(scene);
    std::string error;
    const bool uploaded = scene.upload(data, renderer.textures(), error);
    if (!uploaded) {
        std::printf("rendered wetness: %s\n", error.c_str());
    }
    CHECK(uploaded);
    if (!uploaded) {
        return;
    }
    lighting::ProbeGrid grid;
    grid.origin = {0, 0, -2};
    grid.spacing = {8, 8, 8};
    auto volume = lighting::ProbeVolume::allocate(grid);
    for (auto& p : volume.probes) {
        p.valid = true;
    }
    for (auto& e : volume.irradiance) {
        e = {3.14159265f, 3.14159265f, 3.14159265f};
    }
    CHECK(lighting::scene_lighting(scene)->set_probe_volume(test::gpu_probes(volume), error));
    scene.set_probe_lighting(true, 1);
    CHECK(renderer.add(std::make_unique<scene::LayerRenderer>(scene, scene::Layer::Mobys), error));
    CHECK(renderer.init(error));
    renderer::FrameBuffer target;
    CHECK(target.create(32, 32, error));
    renderer::FrameInput input;
    input.enhanced_water = true;
    input.frame = 1;
    input.camera.view = renderer::translation({-2, -2, -.7f});
    input.camera.projection = renderer::identity();
    scene::Instance hero;
    hero.palette = 0;
    hero.temporal_id = 77;
    hero.hero_shadow_anchor = true;
    hero.moby_light = {0, 0, 0, 1};
    hero.moby_ambient = {0, 0, 0, 0};
    scene.set_instances(scene::Layer::Mobys, {hero});
    scene.set_palette({renderer::identity()});
    auto draw = [&] {
        renderer.render(input, {target.id(), 32, 32});
        return target.read_rgba();
    };
    constexpr unsigned at = (16 * 32 + 16) * 4;
    enhanced::set_wetness_debug_view(true);
    auto debug = draw();
    CHECK(debug[at] < 3 && std::abs(int(debug[at + 1]) - 166) <= 2 && debug[at + 2] > 252);
    scene.set_probe_lighting(false, 1);
    CHECK(draw() == debug);  // The contact diagnostic also works without active probe lighting.
    scene.set_probe_lighting(true, 1);
    enhanced::set_wetness_debug_view(false);
    auto wet = draw();
    input.enhanced_water = false;
    auto dry = draw();
    CHECK(wet[at] > 0 && wet[at] < dry[at]);
    CHECK(wet[at + 3] == dry[at + 3]);  // RGB-only coating; alpha is unchanged.
    auto material = enhanced::WetnessMaterialSettings{};
    material.diffuse_scale = 1;
    material.specular_strength = 0;
    enhanced::set_wetness_material_settings(material);
    input.enhanced_water = true;
    CHECK(draw() == dry);
    enhanced::set_wetness_material_settings({});
    // A different model with the same palette must never inherit the blue debug mask.
    input.enhanced_water = true;
    ++input.frame;
    hero.model = 1;
    hero.hero_shadow_anchor = false;
    scene.set_instances(scene::Layer::Mobys, {hero});
    enhanced::set_wetness_debug_view(true);
    auto other = draw();
    CHECK(other[at] == other[at + 1] && other[at + 1] == other[at + 2]);
    enhanced::set_wetness_debug_view(false);
}
#endif
int main() {
#ifndef OPENRAC_VULKAN
    return test::kSkip;
#else
    test::GlContext context;
    std::string error;
    if (!context.open(error)) {
        std::printf("SKIP %s\n", error.c_str());
        return test::kSkip;
    }
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }
    const auto defaults = enhanced::wetness_material_settings();
    CHECK(defaults.diffuse_scale == .34f && defaults.specular_strength == 1.07f);
    CHECK(defaults.dry_roughness == .51f && defaults.wet_roughness == .35f);
    CHECK(defaults.reflectivity == .0633f);
    auto invalid = enhanced::WetnessMaterialSettings{};
    invalid.diffuse_scale = -1;
    invalid.specular_strength = 100;
    invalid.dry_roughness = 0;
    invalid.wet_roughness = 2;
    invalid.reflectivity = std::numeric_limits<float>::quiet_NaN();
    enhanced::set_wetness_material_settings(invalid);
    auto bounded = enhanced::wetness_material_settings();
    CHECK(bounded.diffuse_scale == 0 && bounded.specular_strength == 4);
    CHECK(bounded.dry_roughness == .08f && bounded.wet_roughness == 1);
    CHECK(bounded.reflectivity == .0633f);
    enhanced::set_wetness_material_settings({});
    ggx_coat_reference();
    // Actual runtime controller and production compute shader, with synthetic assets only.
    test::WaterTestBake bake;
    scene::LevelScene scene;
    auto water = enhanced::make_water(scene, bake.folder);
    CHECK(bool(water));
    if (!water || !water->init(error)) {
        std::printf("%s\n", error.c_str());
        return 1;
    }
    scene::LevelData level;
    level.vertices.resize(8);
    const std::array<std::array<float, 3>, 5> positions{
        {{.5f, .5f, .7f}, {.5f, .5f, 1.5f}, {9, .5f, .7f}, {.5f, .5f, -1}, {.5f, .5f, 1.5f}}
    };
    for (unsigned i = 0; i < 5; ++i) {
        auto& v = level.vertices[i + 3];
        std::copy(positions[i].begin(), positions[i].end(), v.position);
        v.weights[0] = 1;
        v.joints[0] = i == 4 ? 1 : 0;
        level.indices.push_back(i + 3);
    }
    level.models.resize(1);
    level.models[0].primitives.push_back({0, 5});
    level.moby_classes[0] = {0, false};
    level.materials.resize(1);
    lighting::use_scene_lighting(scene);
    renderer::TexturePool textures;
    if (!scene.upload(level, textures, error)) {
        std::printf("wetness scene: %s\n", error.c_str());
        return 1;
    }
    enhanced::HeroWetness wet;
    wet.load(level);
    unsigned palette = 0, vao = 0;
    glGenTextures(1, &palette);
    glGenVertexArrays(1, &vao);
    std::array<float, 4096> matrices{};
    auto identity = renderer::identity();
    std::copy(identity.begin(), identity.end(), matrices.begin());
    std::copy(identity.begin(), identity.end(), matrices.begin() + 16);
    matrices[16 + 14] = -1;  // A submerged hand, although its bind-pose vertex is above water.
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, palette);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        renderer::world::gl::GL_RGBA32F,
        1024,
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        matrices.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    renderer::Shader read;
    CHECK(read.build(
        "wetness test readback",
        R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
        R"(#version 430 core
layout(std430,binding=9) readonly buffer HeroWetness {float values[];};
layout(location=0) out vec4 result;
void main(){uint b=floatBitsToUint(values[int(gl_FragCoord.x)]);
result=vec4(b&255u,(b>>8)&255u,(b>>16)&255u,b>>24)/255.;})",
        error
    ));
    renderer::FrameBuffer target;
    CHECK(target.create(5, 1, error));
    renderer::FrameInput input;
    input.enhanced_water = true;
    input.frame = 1;
    scene::Instance hero;
    hero.palette = 0;
    hero.temporal_id = 123;
    const auto values = [&] {
        read.use();
        wet.bind(read);
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glViewport(0, 0, 5, 1);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::array<unsigned char, 20> pixels{};
        glReadPixels(0, 0, 5, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        std::array<float, 5> result{};
        for (unsigned i = 0; i < 5; ++i) {
            unsigned p = i * 4;
            auto bits = std::uint32_t(pixels[p]) | (std::uint32_t(pixels[p + 1]) << 8)
                        | (std::uint32_t(pixels[p + 2]) << 16)
                        | (std::uint32_t(pixels[p + 3]) << 24);
            result[i] = std::bit_cast<float>(bits);
        }
        return result;
    };
    const auto expect = [&](std::array<float, 5> expected) {
        auto result = values();
        for (unsigned i = 0; i < 5; ++i) {
            if (std::abs(result[i] - expected[i]) > 2e-5f || !std::isfinite(result[i])) {
                std::printf(
                    "sample %u got %.8g expected %.8g\n", i, double(result[i]), double(expected[i])
                );
            }
            CHECK(std::isfinite(result[i]) && std::abs(result[i] - expected[i]) < 2e-5f);
        }
    };
    wet.prepare(input, palette, hero);
    expect({1, 0, 0, 0, 1});
    // Animation/world motion carries the original wet patch; it does not wet the whole body.
    hero.matrix[14] = 4;
    input.frame = 601;
    wet.prepare(input, palette, hero);
    expect({.5f, 0, 0, 0, .5f});
    wet.prepare(input, palette, hero);
    expect({.5f, 0, 0, 0, .5f});  // duplicate render: no extra drying
    input.frame = 1201;
    wet.prepare(input, palette, hero);
    expect({0, 0, 0, 0, 0});
    hero.matrix = renderer::identity();
    ++input.frame;
    wet.prepare(input, palette, hero);
    expect({1, 0, 0, 0, 1});
    // A new hero identity cannot inherit the old hero's history.
    hero.matrix[14] = 4;
    ++hero.temporal_id;  // A same-tick identity change must reset too.
    wet.prepare(input, palette, hero);
    expect({0, 0, 0, 0, 0});
    hero.matrix = renderer::identity();
    ++input.frame;
    wet.prepare(input, palette, hero);
    expect({1, 0, 0, 0, 1});
    input.enhanced_water = false;
    wet.prepare(input, palette, hero);
    input.enhanced_water = true;
    hero.matrix[14] = 4;
    wet.prepare(input, palette, hero);
    expect({0, 0, 0, 0, 0});
    rendered_surface();
    // Losing the water runtime must still allow the saved patch to dry safely.
    hero.matrix = renderer::identity();
    ++input.frame;
    wet.prepare(input, palette, hero);
    expect({1, 0, 0, 0, 1});
    water->release();
    input.frame += 600;
    wet.prepare(input, palette, hero);
    expect({.5f, 0, 0, 0, .5f});
    enhanced::set_wetness_debug_view(true);
    CHECK(enhanced::wetness_debug_view());
    enhanced::set_wetness_debug_view(false);
    wet.release();
    wet.load(scene::LevelData{});
    wet.prepare(input, palette, hero);
    wet.release();
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &palette);
    CHECK(vkgl::submission_counters().validation_errors == 0);
    std::printf(
        "GPU wetness: submersion, skinning, persistence, 20-second drying and resets checked\n"
    );
    return test::result();
#endif
}
