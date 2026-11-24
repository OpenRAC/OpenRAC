// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's energy (interface 1.25) as the production mesh.frag draws it: a grey texel lit gold keeps
// its gold with a little of the mod's colour (the instance tint holds only its share,
// OPENRAC_MOBY_MATERIAL_ENERGY_BODY, as frame.cpp's apply_mod_material sets it), and a gold glow
// packet comes out in the mod's colour, brightened. Without the material both are drawn as the
// texel times their colour, as before. A mod's paint (interface 1.34) on the same surface: the
// texel's grey in the mod's colour (the instance tint holds all of it), and its glow packet in it.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.h"
#include "enhanced/viewer/probe_lighting.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"
#include "renderer/textures/texture_pool.h"
#include "renderer_gl_context.h"
#include "scene_shaders.h"
#include "tests/graph_fixture.h"

using namespace openrac;
using namespace openrac::gl;

namespace {

constexpr int kSize = 16;
constexpr int kMaterialNone = 0;
constexpr int kMaterialEnergy = 4;
// scene::kModMaterialPaint, at glow level 0 (no normal here, so no rim either way).
constexpr int kMaterialPaint = 16;
// OPENRAC_MOBY_MATERIAL_ENERGY_BODY: the share of the mod's colour in the tint.
constexpr float kBody = 0.35f;
// The coin texture's grey (level textures moby_0022 and its kind: 87..118 of 255).
constexpr unsigned char kGrey = 110;
// A gold vertex colour, 0x80 = 1.0 as mesh.vert gives it.
constexpr std::array<float, 4> kGold = {1.0f, 0.75f, 0.25f, 1.0f};

// mesh.frag's inputs, a full-screen triangle with the vertex colour, material and glow the test
// gives; no normal, so no highlight (the view is the test's own).
const std::string kVertex = R"(#version 410 core
uniform vec4 test_colour;
uniform int test_material;
uniform int test_glow;
uniform vec3 test_rgb;
out vec2 v_uv;
out vec3 v_stq;
out vec4 v_colour;
out vec3 v_normal;
out vec3 v_world_position;
out vec4 v_probe_multiplier;
flat out vec4 v_probe_selector;
flat out int v_lit;
flat out int v_glow;
flat out int v_mod_material;
flat out vec3 v_mod_rgb;
out vec3 v_script_emission;
flat out vec3 v_probe_ambient;
out vec3 v_emission_colour;
out vec2 v_fog_w;
void main(){
 vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
 gl_Position=vec4(p*2-1,0,1);v_uv=p;v_stq=vec3(p,1);
 v_colour=test_colour*vec4(test_rgb,1);v_normal=vec3(0);v_world_position=vec3(0);
 v_probe_multiplier=vec4(1);v_probe_selector=vec4(0);v_lit=1;v_glow=test_glow;
 v_mod_material=test_material;v_mod_rgb=test_rgb;
 v_script_emission=vec3(0);v_probe_ambient=vec3(0);v_emission_colour=vec3(0);v_fog_w=vec2(0,1);
})";

struct Colour {
    float r = 0, g = 0, b = 0;

    float luminance() const { return 0.299f * r + 0.587f * g + 0.114f * b; }
};

class Fixture {
public:
    Fixture() = default;

    ~Fixture() { glDeleteVertexArrays(1, &m_vao); }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;

    const std::string& error() const { return m_error; }

    // Builds the production mesh.frag and a uniform grey texture.
    bool create() {
        const std::string fragment = lighting::probe_mesh_fragment(scene::shaders::mesh_frag);

        if (!m_shader.build("energy mesh", kVertex, fragment, m_error)) {
            return false;
        }

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        renderer::Rgba8Image image(8, 8);

        for (std::size_t i = 0; i < image.pixels.size(); i += 4) {
            image.pixels[i] = image.pixels[i + 1] = image.pixels[i + 2] = kGrey;
            image.pixels[i + 3] = 255;
        }

        m_source =
            m_pool.add(std::move(image), renderer::AlphaScale::Full, "coin grey", false, true);
        m_texture = m_pool.gl_texture(m_source);
        return m_frame.create(kSize, kSize, m_error);
    }

    // Returns the centre pixel drawn with `material` and the mod's `rgb` (the moby's tint) on a
    // gold surface (`glow` 0) or glow packet.
    Colour draw(int material, int glow, const std::array<float, 3>& rgb = {1, 1, 1}) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_frame.id());
        glViewport(0, 0, kSize, kSize);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        m_shader.use();
        m_pool.bind_upscaled_texture_colours(m_source, m_shader, renderer::TextureFilter::Normal);
        glUniform4f(m_shader.uniform("test_colour"), kGold[0], kGold[1], kGold[2], kGold[3]);
        glUniform1i(m_shader.uniform("test_material"), material);
        glUniform1i(m_shader.uniform("test_glow"), glow);
        glUniform3f(m_shader.uniform("test_rgb"), rgb[0], rgb[1], rgb[2]);
        glUniform1i(m_shader.uniform("glow_pass"), glow);
        glUniform4f(m_shader.uniform("material_colour"), 1, 1, 1, 1);
        glUniform1f(m_shader.uniform("material_diffuse_scale"), 1);
        glUniform1i(m_shader.uniform("textured"), 1);
        glUniform1i(m_shader.uniform("tcc"), 1);
        glUniform1f(m_shader.uniform("tex_alpha_scale"), 0.5f);
        test::graph_draw(glDrawArrays, GL_TRIANGLES, 0, 3);
        const auto out = m_frame.read_rgba();
        const std::size_t at = (std::size_t{kSize / 2} * kSize + kSize / 2) * 4;
        return {out[at] / 255.0f, out[at + 1] / 255.0f, out[at + 2] / 255.0f};
    }

private:
    renderer::Shader m_shader;
    renderer::TexturePool m_pool;
    renderer::FrameBuffer m_frame;
    renderer::TextureHandle m_source = 0;
    unsigned m_vao = 0, m_texture = 0;
    std::string m_error;
};

// Whether `c` is the grey texel times the gold colour, to a step of 8 bits (the unchanged path).
bool gold(const Colour& c) {
    const float t = kGrey / 255.0f;
    const float step = 1.5f / 255.0f;
    return std::fabs(c.r - t * kGold[0]) <= step && std::fabs(c.g - t * kGold[1]) <= step
           && std::fabs(c.b - t * kGold[2]) <= step;
}

// Whether `c` is green: green well above red and blue.
bool green(const Colour& c) {
    return c.g > 2.0f * c.r && c.g > 2.0f * c.b;
}

// Returns the instance tint that carries `rgb` for energy: 1 - kBody + kBody x rgb a channel.
std::array<float, 3> energy_tint(const std::array<float, 3>& rgb) {
    return {
        1.0f - kBody + kBody * rgb[0], 1.0f - kBody + kBody * rgb[1], 1.0f - kBody + kBody * rgb[2]
    };
}

void print(const char* name, const Colour& c) {
    std::printf("%-18s %.3f %.3f %.3f (luminance %.3f)\n", name, c.r, c.g, c.b, c.luminance());
}

}  // namespace

int main() {
    test::GlContext context;
    std::string error;

    if (!context.open(error, 320, 240)) {
        std::puts(error.c_str());
        return test::kSkip;
    }

    Fixture fixture;

    if (!fixture.create()) {
        std::puts(fixture.error().c_str());
        return 1;
    }

    // An NG++ tier's colour: emerald (0x20FF40).
    const std::array<float, 3> emerald = {32.0f / 255.0f, 1.0f, 64.0f / 255.0f};
    const Colour surface = fixture.draw(kMaterialNone, 0);
    const Colour glow = fixture.draw(kMaterialNone, 1);
    const Colour charged = fixture.draw(kMaterialEnergy, 0, energy_tint(emerald));
    const Colour charged_glow = fixture.draw(kMaterialEnergy, 1, energy_tint(emerald));
    print("gold", surface);
    print("gold glow", glow);
    print("energy", charged);
    print("energy glow", charged_glow);

    // No material: the texel times its colour, surface and glow alike.
    CHECK(gold(surface));
    CHECK(gold(glow));
    // Energy's body keeps most of its gold: still brighter red than blue, a little greener, and
    // not much darker (no normal here, so no rim).
    CHECK(charged.r > charged.b);
    CHECK(charged.g / charged.r > surface.g / surface.r);
    CHECK(charged.luminance() >= 0.6f * surface.luminance());
    // Its glow packet is the mod's green, and brighter than the gold one's luminance.
    CHECK(green(charged_glow));
    CHECK(charged_glow.luminance() > glow.luminance());

    // Paint: an azure armour tier (0x40A0FF) over the gold-lit grey. The grey takes the colour:
    // far bluer and greener against its red than the gold surface, still lit (not much darker
    // than it), and its glow packet is the colour too.
    const std::array<float, 3> azure = {64.0f / 255.0f, 160.0f / 255.0f, 1.0f};
    const Colour painted = fixture.draw(kMaterialPaint, 0, azure);
    const Colour painted_glow = fixture.draw(kMaterialPaint, 1, emerald);
    print("paint", painted);
    print("paint glow", painted_glow);
    CHECK(painted.g / std::max(painted.r, 1e-3f) > surface.g / std::max(surface.r, 1e-3f));
    CHECK(painted.b / std::max(painted.r, 1e-3f) > 2.0f * surface.b / std::max(surface.r, 1e-3f));
    CHECK(painted.luminance() >= 0.5f * surface.luminance());
    CHECK(green(painted_glow));
    return test::result();
}
