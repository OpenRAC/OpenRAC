// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string_view>

#include "enhanced/renderer/fur.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

using namespace openrac;
using namespace openrac::gl;

// Makes a synthetic body with separate fur and equipment islands in one material.
scene::LevelData fixture(enhanced::FurProfile& profile) {
    scene::LevelData level;
    level.game = profile.game;
    level.vertices.resize(9);
    for (auto& vertex : level.vertices) {
        vertex.uv[0] = .1f;
        vertex.uv[1] = .1f;
    }
    level.vertices[3].uv[0] = level.vertices[3].uv[1] = 0;
    level.vertices[4].uv[0] = 1;
    level.vertices[4].uv[1] = 0;
    level.vertices[5].uv[0] = 0;
    level.vertices[5].uv[1] = 1;
    level.indices = {3, 4, 5, 6, 7, 8, 0, 1, 2};
    level.models.resize(1);
    level.models[0].primitives = {{0, 6, 0}, {6, 3, 1}};
    level.materials.resize(2);
    level.materials[0].name = profile.material;
    level.materials[0].image = 0;
    level.images.emplace_back(profile.width, profile.height);
    level.moby_classes[profile.class_id] = {0, false};
    profile.index_count = profile.vertex_count = 6;
    profile.patches = {enhanced::FurPatch{{{0, 0}}, {0, 1}}};
    profile.uv_signature = enhanced::fur_uv_signature(level, level.models[0].primitives[0]);
    return level;
}

// Checks authored exclusions and rejects models whose identity or topology differs.
void test_maps_and_identity() {
    auto profile = enhanced::rac1_fur_profile();
    CHECK(profile.game == "rac1" && profile.class_id == 0 && profile.patches.size() == 9);
    auto level = fixture(profile);
    const auto map = enhanced::make_fur_map(level, level.models[0].primitives[0], profile);
    CHECK(map.width == 256 && map.height == 256);
    const auto mask = [&](int x, int y) {
        return map.texel(x, y) & 255u;
    };
    CHECK(mask(30, 30) == 255);   // Arm stripes remain fur.
    CHECK(mask(90, 40) == 255);   // Eyelid, not the eyeball.
    CHECK(mask(65, 92) == 255);   // Cheek.
    CHECK(mask(30, 225) == 255);  // Foot.
    CHECK(mask(240, 240) == 0);   // Outside the selected triangle.
    CHECK(mask(109, 69) == 0);    // Nose.
    CHECK(mask(72, 205) == 0);    // Claw.
    for (std::size_t at = 0; at < map.pixels.size(); at += 4) {
        CHECK(map.pixels[at + 3] == 255);
        if (map.pixels[at] > 250) {
            const float u = float(map.pixels[at + 1]) * 2 / 255 - 1;
            const float v = float(map.pixels[at + 2]) * 2 / 255 - 1;
            CHECK(std::abs(std::hypot(u, v) - 1) < .015f);
        }
    }
    CHECK(((map.texel(30, 30) >> 16) & 255u) == 255);  // Arms flow down UV V.
    const auto selected = enhanced::identify_fur_surface(level, profile);
    CHECK(selected && selected->model == 0 && selected->first == 3 && selected->count == 6);
    CHECK(selected && selected->vertices == std::vector<unsigned>({1, 1, 1, 0, 0, 0}));
    level.vertices[4].uv[0] += .1f;
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.indices[1] = 999;
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.indices[6] = 3;  // Shared with a non-fur material: a vertex-range gate would leak.
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.overrides = "mod";
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.game = "another-game";
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.moby_classes[0].second = true;
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.materials[0].name = "different-atlas";
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.images[0].width = 512;
    CHECK(!enhanced::identify_fur_surface(level, profile));
    level = fixture(profile);
    level.vertices[4].uv[0] = std::numeric_limits<float>::quiet_NaN();
    CHECK(!enhanced::identify_fur_surface(level, profile));
    profile.width = 0;
    CHECK(enhanced::make_fur_map(level, level.models[0].primitives[0], profile).pixels.empty());
}

// Checks production map sampling and vertex eligibility on the GPU.
int test_gpu() {
    auto settings = enhanced::fur_material_settings();
    settings.enabled = false;
    settings.lighting_enabled = false;
    enhanced::set_fur_material_settings(settings);
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        std::printf("SKIP %s\n", error.c_str());
        return test::kSkip;
    }
    auto profile = enhanced::rac1_fur_profile();
    auto level = fixture(profile);
    enhanced::HeroFur fur;
    fur.load(level, profile);
    scene::Instance hero;
    hero.palette = 7;
    fur.prepare(hero);
    renderer::Shader shader;
    const std::string vertex =
        std::string("#version 430 core\n") + lighting::shaders::fur_vertex_glsl + R"(
uniform int test_id;
uniform float test_palette, test_lighting;
flat out int v_hero_fur;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1);
    v_hero_fur=hero_fur_receiver(test_id,test_palette,test_lighting);
})";
    const std::string fragment =
        std::string("#version 430 core\n") + lighting::shaders::fur_glsl + R"(
uniform vec2 test_uv;
layout(location=0) out vec4 result;
void main() {result=vec4(v_hero_fur!=0 ? fur_diagnostic(test_uv) : vec3(.25),1);}
)";
    const bool built = shader.build("fur coverage and direction", vertex, fragment, error);
    if (!built) {
        std::printf("%s\n", error.c_str());
    }
    CHECK(built);
    if (!built) {
        return test::result();
    }
    renderer::FrameBuffer target;
    CHECK(target.create(8, 8, error));
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, 8, 8);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    shader.use();
    glUniform1i(shader.uniform("test_id"), 3);
    glUniform1f(shader.uniform("test_palette"), 7);
    glUniform1f(shader.uniform("test_lighting"), 1);
    glUniform2f(shader.uniform("test_uv"), 30.5f / 256, 30.5f / 256);
    const auto draw = [&] {
        fur.bind(shader);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return target.read_rgba();
    };
    enhanced::set_fur_debug_view(enhanced::FurDebug::Off);
    const auto unchanged = draw();
    enhanced::set_fur_debug_view(enhanced::FurDebug::Mask);
    const auto coverage = draw();
    CHECK(coverage[0] > 30 && coverage[0] < 45 && coverage[1] > 250 && coverage[2] > 70);
    glUniform2f(shader.uniform("test_uv"), 170.5f / 256, 120.5f / 256);
    const auto eye = draw();
    CHECK(eye[0] < 20 && eye[1] < 20 && eye[2] < 20);
    glUniform2f(shader.uniform("test_uv"), 30.5f / 256, 30.5f / 256);
    enhanced::set_fur_debug_view(enhanced::FurDebug::Direction);
    CHECK(draw() != coverage);
    for (int id : {2, 6, 9000}) {
        glUniform1i(shader.uniform("test_id"), id);
        CHECK(draw() == unchanged);
    }
    glUniform1i(shader.uniform("test_id"), 3);
    for (float palette : {-1.f, 6.f, 8.f}) {
        glUniform1f(shader.uniform("test_palette"), palette);
        CHECK(draw() == unchanged);
    }
    glUniform1f(shader.uniform("test_palette"), 7);
    glUniform1f(shader.uniform("test_lighting"), 2);  // Inventory/UI model.
    CHECK(draw() == unchanged);
    glUniform1f(shader.uniform("test_lighting"), 1);
    hero.model = 1;
    fur.prepare(hero);
    CHECK(draw() == unchanged);
    fur.prepare(std::nullopt);
    CHECK(draw() == unchanged);
    fur.load({}, profile);  // Valid fallback descriptor and no stale surface after a level change.
    CHECK(draw() == unchanged);
    enhanced::set_fur_debug_view(enhanced::FurDebug::Off);
    glDeleteVertexArrays(1, &vao);
    enhanced::set_fur_material_settings({});
    return test::result();
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view(argv[1]) == "--gpu") {
        return test_gpu();
    }
    if (argc >= 3 && std::string_view(argv[1]) == "--level") {
        scene::LevelData level;
        std::string error;
        CHECK(scene::load_level(argv[2], level, error));
        const auto& profile = enhanced::rac1_fur_profile();
        const auto surface = enhanced::identify_fur_surface(level, profile);
        if (!surface) {
            std::printf("No supported fur surface: %s\n", error.c_str());
            CHECK(surface);
            return test::result();
        }
        const auto map = enhanced::make_fur_map(
            level, level.models[surface->model].primitives[surface->primitive], profile
        );
        if (argc == 4) {
            std::ofstream output(argv[3], std::ios::binary);
            output.write(
                reinterpret_cast<const char*>(map.pixels.data()), std::streamsize(map.pixels.size())
            );
            CHECK(output.good());
        }
        for (const auto point : std::array<std::array<int, 2>, 6>{
                 {{30, 30}, {90, 40}, {65, 92}, {210, 140}, {150, 249}, {30, 225}}
             }) {
            CHECK((map.texel(point[0], point[1]) & 255u) == 255);
        }
        for (const auto point : std::array<std::array<int, 2>, 7>{
                 {{170, 120}, {109, 69}, {150, 70}, {190, 180}, {60, 160}, {175, 230}, {72, 205}}
             }) {
            CHECK((map.texel(point[0], point[1]) & 255u) == 0);
        }
        std::printf("Verified fur UV layout, coverage and exclusions on %s\n", argv[2]);
        return test::result();
    }
    test_maps_and_identity();
    return test::result();
}
