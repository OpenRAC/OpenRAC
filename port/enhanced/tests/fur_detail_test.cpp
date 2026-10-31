// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Checks strand orientation, filtering and production normal reconstruction without game assets.
#include <cmath>
#include <cstdio>
#include <exception>
#include <limits>
#include <string_view>

#include "enhanced/renderer/fur.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"
#ifdef OPENRAC_VULKAN
#include "renderer/vulkan/native_compute.h"
#include "renderer/vulkan/vkgl.h"
#endif

using namespace openrac;
using namespace openrac::gl;

namespace {
// Measures the signed UV-slope energy, independent of the encoded strand colour variation.
std::array<double, 2> slope_energy(const renderer::Rgba8Image& image) {
    std::array<double, 2> energy{};
    for (std::size_t n = 0; n < image.pixels.size(); n += 4) {
        for (unsigned axis = 0; axis < 2; ++axis) {
            const double slope = int(image.pixels[n + axis]) - 128;
            energy[axis] += slope * slope;
        }
        CHECK(image.pixels[n + 3] == 255);
    }
    return energy;
}

// The same seed repeats exactly, while rotating the groom rotates the high-frequency slope energy.
void test_strands() {
    renderer::Rgba8Image groom(1, 1);
    groom.set_texel(0, 0, renderer::rgba(255, 128, 255, 255));
    const auto vertical = enhanced::make_fur_detail_map(groom);
    CHECK(vertical.width == 512 && vertical.height == 512);
    CHECK(enhanced::make_fur_detail_map(groom).pixels == vertical.pixels);
    const auto v = slope_energy(vertical);
    CHECK(v[0] > v[1] * 2 && v[1] > 0);
    groom.set_texel(0, 0, renderer::rgba(255, 255, 128, 255));
    const auto h = slope_energy(enhanced::make_fur_detail_map(groom));
    CHECK(h[1] > h[0] * 2 && h[0] > 0);
    CHECK(enhanced::make_fur_detail_map({}).pixels.empty());
    groom.pixels.clear();
    CHECK(enhanced::make_fur_detail_map(groom).pixels.empty());
}

// Invalid live values are bounded and non-finite values return to the documented defaults.
void test_settings() {
    enhanced::FurMaterialSettings settings;
    settings.strength = -1;
    settings.contrast = 99;
    settings.fade_distance = std::numeric_limits<float>::quiet_NaN();
    settings.highlight = std::numeric_limits<float>::infinity();
    settings.softness = 0;
    settings.directionality = 5;
    settings.scattering = -1;
    settings.environment_strength = std::numeric_limits<float>::quiet_NaN();
    enhanced::set_fur_material_settings(settings);
    const auto bounded = enhanced::fur_material_settings();
    CHECK(bounded.strength == 0 && bounded.contrast == .15f && bounded.fade_distance == 103);
    CHECK(
        bounded.highlight == .52f && bounded.softness == .35f && bounded.directionality == .9f
        && bounded.scattering == 0
    );
    CHECK(bounded.environment_strength == 1.85f);
    enhanced::set_fur_material_settings({});
}

const char* const kVertex = R"(#version 430 core
uniform vec3 test_u, test_v;
uniform float test_scale, test_mirror;
out vec3 test_position;
out vec2 test_uv;
flat out int v_hero_fur;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0,1);
    test_position=test_u*p.x+test_v*p.y;
    test_uv=p*test_scale;test_uv.x*=test_mirror;
    v_hero_fur=1;
})";

// Compiles exactly the same material helper as the production mesh variants.
std::string fragment_source() {
    return std::string("#version 430 core\n") + lighting::shaders::fur_glsl
           + lighting::shaders::fur_detail_glsl + R"(
in vec3 test_position;
in vec2 test_uv;
uniform vec3 test_normal, test_eye;
uniform float test_wetness;
layout(location=0) out vec4 result;
void main() {
    FurSurfaceData detail=fur_surface_data(test_normal,test_position,test_uv,test_eye,test_wetness);
    result=vec4(detail.normal*.5+.5,detail.grain*.5);
})";
}

// Owns a hidden rendering target and constant, exactly known material slopes.
class DetailGpu {
public:
    DetailGpu() = default;
    DetailGpu(const DetailGpu&) = delete;
    DetailGpu& operator=(const DetailGpu&) = delete;

    ~DetailGpu() {
        glDeleteTextures(2, m_textures.data());
        glDeleteVertexArrays(1, &m_vao);
    }

    bool init(std::string& error) {
        if (!shader.build("directional fur surface reference", kVertex, fragment_source(), error)
            || !m_target.create(8, 8, error)) {
            return false;
        }
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glBindFramebuffer(GL_FRAMEBUFFER, m_target.id());
        glViewport(0, 0, 8, 8);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glGenTextures(2, m_textures.data());
        upload(0, renderer::rgba(255, 128, 255, 255));
        upload(1, renderer::rgba(255, 128, 255, 255));
        shader.use();
        glUniform1i(shader.uniform("hero_fur_map"), 26);
        glUniform1i(shader.uniform("hero_fur_detail"), 27);
        reset();
        return true;
    }

    // Restores a front-facing unscaled surface, fully resolved and dry, ten units from the eye.
    void reset() {
        glUniform1i(shader.uniform("hero_fur_detail_enabled"), 1);
        glUniform4f(shader.uniform("hero_fur_detail_parameters"), .25f, .025f, 40, 80);
        glUniform3f(shader.uniform("test_u"), 1, 0, 0);
        glUniform3f(shader.uniform("test_v"), 0, 1, 0);
        glUniform3f(shader.uniform("test_normal"), 0, 0, 1);
        glUniform3f(shader.uniform("test_eye"), 0, 0, 10);
        glUniform1f(shader.uniform("test_scale"), .01f);
        glUniform1f(shader.uniform("test_mirror"), 1);
        glUniform1f(shader.uniform("test_wetness"), 0);
    }

    // Replaces one constant map, using normal production UNORM texture sampling.
    void upload(unsigned index, std::uint32_t pixel) {
        glActiveTexture(GL_TEXTURE0 + 26 + index);
        glBindTexture(GL_TEXTURE_2D, m_textures[index]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    std::vector<std::uint8_t> draw() {
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return m_target.read_rgba();
    }

    renderer::Shader shader;

private:
    renderer::FrameBuffer m_target;
    std::array<unsigned, 2> m_textures{};
    unsigned m_vao = 0;
};

// Known slopes rotate with the surface and flip with mirrored UVs, without changing their amplitude.
void test_frame(DetailGpu& gpu) {
    const auto front = gpu.draw();
    CHECK(
        std::abs(int(front[0]) - 97) <= 1 && std::abs(int(front[1]) - 128) <= 1 && front[2] >= 250
    );
    CHECK(
        front[3] > 128
    );  // Positive strand variation is visible even under flat indirect lighting.
    glUniform1f(gpu.shader.uniform("test_mirror"), -1);
    CHECK(std::abs(int(gpu.draw()[0]) - 158) <= 1);
    gpu.reset();
    glUniform3f(gpu.shader.uniform("test_u"), 0, 1, 0);
    glUniform3f(gpu.shader.uniform("test_v"), -1, 0, 0);
    const auto rotated = gpu.draw();
    CHECK(std::abs(int(rotated[0]) - 128) <= 1 && std::abs(int(rotated[1]) - 97) <= 1);
    gpu.reset();
    glUniform3f(gpu.shader.uniform("test_u"), 2, 0, 0);
    glUniform3f(gpu.shader.uniform("test_v"), 0, .5f, 0);
    CHECK(gpu.draw()[0] > front[0]);  // Stretched UV direction has a shallower physical slope.
    gpu.reset();
    glUniform3f(gpu.shader.uniform("test_u"), 0, 1, 0);
    glUniform3f(gpu.shader.uniform("test_v"), 0, 0, 1);
    glUniform3f(gpu.shader.uniform("test_normal"), 1, 0, 0);
    glUniform3f(gpu.shader.uniform("test_eye"), 10, 0, 0);
    const auto posed = gpu.draw();
    CHECK(
        posed[0] >= 250 && std::abs(int(posed[1]) - 97) <= 1 && std::abs(int(posed[2]) - 128) <= 1
    );
    gpu.reset();
}

// Coverage, disabled detail, far/minified/grazing surfaces and singular UVs all retain the base normal.
void test_filtering(DetailGpu& gpu) {
    glUniform1i(gpu.shader.uniform("hero_fur_detail_enabled"), 0);
    const auto flat = gpu.draw();
    CHECK(flat[0] == 128 && flat[1] == 128 && flat[2] == 255 && flat[3] == 128);
    gpu.reset();
    glUniform3f(gpu.shader.uniform("test_eye"), 0, 0, 90);
    CHECK(gpu.draw() == flat);
    gpu.reset();
    glUniform3f(gpu.shader.uniform("test_eye"), 10, 0, 0);
    CHECK(gpu.draw() == flat);
    gpu.reset();
    glUniform1f(gpu.shader.uniform("test_scale"), 64);
    CHECK(gpu.draw() == flat);
    gpu.reset();
    glUniform1f(gpu.shader.uniform("test_scale"), 0);
    CHECK(gpu.draw() == flat);
    gpu.reset();
    glUniform1f(gpu.shader.uniform("test_wetness"), 1);
    const auto wet = gpu.draw();
    CHECK(wet[0] > 115 && wet[0] < 125 && wet[3] <= 129);
    gpu.reset();
    gpu.upload(0, renderer::rgba(0, 128, 128, 255));
    CHECK(gpu.draw() == flat);
}

int test_gpu() {
    test::GlContext context;
    std::string error;
    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        std::printf("SKIP %s\n", error.c_str());
        return test::kSkip;
    }
    DetailGpu gpu;
#ifdef OPENRAC_VULKAN
    const auto before = vkgl::submission_counters().shader_fp16_ops;
#endif
    const bool ready = gpu.init(error);
    if (!ready) {
        std::printf("%s\n", error.c_str());
    }
    CHECK(ready);
#ifdef OPENRAC_VULKAN
    vkgl::NativeDevice device;
    CHECK(vkgl::native_device(device));
    const auto arithmetic = vkgl::submission_counters().shader_fp16_ops - before;
    CHECK(device.float16 ? arithmetic > 0 : arithmetic == 0);
    std::printf(
        "Fur detail: %llu typed FP16 SPIR-V operations; enabled: %d\n",
        static_cast<unsigned long long>(arithmetic),
        int(device.float16)
    );
#endif
    if (ready) {
        test_frame(gpu);
        test_filtering(gpu);
    }
#ifdef OPENRAC_VULKAN
    CHECK(vkgl::submission_counters().validation_errors == 0);
#endif
    return test::result();
}
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string_view(argv[1]) == "--gpu") {
            return test_gpu();
        }
        test_settings();
        test_strands();
        return test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Fur detail test: %s\n", error.what());
        return 1;
    }
}
