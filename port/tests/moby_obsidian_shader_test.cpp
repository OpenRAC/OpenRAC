// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's obsidian (interface 1.24) as the production mesh.frag draws it: a grey texel lit gold, as
// Going Commando's bolt coins are, comes out near black with a violet glow in it, and a gold glow
// packet comes out violet. The mod's rgb, which mesh.vert multiplies into the colour as the moby's
// tint, picks the glow's colour without making the body brighter or darker. Without the material
// both are drawn as the texel times their colour, as before.
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

using namespace openrac;
using namespace openrac::gl;

namespace {

constexpr int kSize = 16;
constexpr int kMaterialNone = 0;
constexpr int kMaterialObsidian = 3;
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

        if (!m_shader.build("obsidian mesh", kVertex, fragment, m_error)) {
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
        glDrawArrays(GL_TRIANGLES, 0, 3);
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

// Whether `c` is violet: blue and red both above green, blue the most.
bool violet(const Colour& c) {
    return c.b > c.g && c.r > c.g && c.b >= c.r;
}

// Whether `c` is red: red well above green and blue.
bool red(const Colour& c) {
    return c.r > 2.0f * c.g && c.r > 2.0f * c.b;
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

    // The glow a mod gives one size of coin: ember red (0xFF2020).
    const std::array<float, 3> ember = {1.0f, 32.0f / 255.0f, 32.0f / 255.0f};
    const Colour surface = fixture.draw(kMaterialNone, 0);
    const Colour obsidian = fixture.draw(kMaterialObsidian, 0);
    const Colour glow = fixture.draw(kMaterialNone, 1);
    const Colour obsidian_glow = fixture.draw(kMaterialObsidian, 1);
    const Colour ember_surface = fixture.draw(kMaterialObsidian, 0, ember);
    const Colour ember_glow = fixture.draw(kMaterialObsidian, 1, ember);
    print("gold", surface);
    print("obsidian", obsidian);
    print("gold glow", glow);
    print("obsidian glow", obsidian_glow);
    print("ember obsidian", ember_surface);
    print("ember glow", ember_glow);

    // No material: the texel times its colour, surface and glow alike.
    CHECK(gold(surface));
    CHECK(gold(glow));
    // Obsidian: a gold-lit grey texture is near black, with violet in it, never pure black.
    CHECK(violet(obsidian));
    CHECK(obsidian.luminance() <= 0.35f * surface.luminance());
    CHECK(obsidian.luminance() > 0.0f);
    // Its glow packet is violet and still a glow, not blown out.
    CHECK(violet(obsidian_glow));
    CHECK(obsidian_glow.luminance() >= 0.1f);
    CHECK(obsidian_glow.r < 1.0f && obsidian_glow.b < 1.0f);
    // The mod's rgb colours the glow (red), and leaves the body as dark as without it.
    CHECK(red(ember_glow));
    CHECK(ember_surface.r > ember_surface.b);
    CHECK(std::fabs(ember_surface.luminance() - obsidian.luminance()) <= 0.05f);
    return test::result();
}
