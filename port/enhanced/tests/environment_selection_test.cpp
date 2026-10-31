// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Tiled candidate masks must preserve the full-loop reflection result, including float bits.
#include <bit>
#include <cstdio>
#include <fstream>
#include <iterator>

#include "enhanced/lighting/environment_filter.h"
#include "enhanced/viewer/environment_selection.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
constexpr int kWidth = 96, kHeight = 80;
constexpr const char* kVertex = R"(#version 410 core
void main() {
 vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
 gl_Position=vec4(p*2.-1.,0,1);
})";

lighting::EnvironmentAsset synthetic(unsigned count, bool overlap) {
    lighting::EnvironmentAsset asset;
    asset.resolution = 8;

    for (unsigned i = 0; i < count; ++i) {
        const float x = float(i % 6) * 2 - 5, y = float((i / 6) % 4) * 2 - 3;
        lighting::EnvironmentProbe p{
            {x, y, -8}, {x - 1.4f, y - 1.4f, -18}, {x + 1.4f, y + 1.4f, -2}
        };

        if (i >= 24 && i < 28) {
            p = {{40, 0, -8}, {35, -5, -20}, {45, 5, -2}};
        }

        if (i == 28 || i == 29 || overlap) {
            p = {{0, 0, -8}, {-12, -10, -30}, {12, 10, 3}};
        }

        if (!overlap && i == 30) {
            p = {{0, 0, -.1f}, {-1, -1, -.2f}, {1, 1, .1f}};
        }

        if (!overlap && i == 31) {
            p = {{0, 0, 5}, {-3, -3, 2}, {3, 3, 8}};
        }

        asset.probes.push_back(p);
    }

    asset.pixels.resize(std::size_t(asset.width()) * asset.height());

    for (unsigned i = 0; i < count; ++i) {
        std::vector<lighting::Float3> faces(6 * 8 * 8);

        for (unsigned p = 0; p < faces.size(); ++p) {
            faces[p] =
                {.03f + float(i % 7) * .1f,
                 .04f + float((i * 3) % 11) * .05f,
                 float(p / (8 * 8) + 1) * .08f};
        }

        lighting::filter_environment(asset, i, faces, 16);
    }

    lighting::integrate_environment_brdf(asset, 32);
    return asset;
}

// A synthetic receiver plane can be anywhere along each view ray, independent of opaque depth.
struct Fixture {
    renderer::Shader shader;
    renderer::FrameBuffer target;
    lighting::EnvironmentSelection selection;
    unsigned texture = 0, vao = 0;
    unsigned comparisons = 0;
    std::string error;

    bool open() {
        if (!target.create(kWidth, kHeight, error)) {
            return false;
        }

        const std::string fragment = std::string("#version 410 core\n")
                                     + std::string(lighting::shaders::environment_glsl) + R"(
uniform mat4 receiver_clip;
uniform vec3 receiver_eye;
uniform vec4 receiver_viewport;
uniform float receiver_depth;
uniform int channel;
uniform vec3 receiver_offset;
layout(location=0) out vec4 result;
void main() {
 uint bits;
 if(channel<0) bits=environment_candidates(0);
 else {
  vec2 ndc=(gl_FragCoord.xy-receiver_viewport.xy)/receiver_viewport.zw*2.-1.;
  vec4 h=inverse(receiver_clip)*vec4(ndc,receiver_depth*2.-1.,1);
  vec3 p=h.xyz/h.w,eye=normalize(receiver_eye-p);
  vec3 colour=environment_specular_surface(p,p+receiver_offset,eye,eye,.37,.65,vec3(.013,.027,.041));
  bits=floatBitsToUint(colour[channel]);
 }
 result=vec4(bits&255u,(bits>>8)&255u,(bits>>16)&255u,bits>>24)/255.;
})";

        if (!shader.build("tiled reflection comparison", kVertex, fragment, error)) {
            return false;
        }

        glGenVertexArrays(1, &vao);
        glGenTextures(1, &texture);
        glBindVertexArray(vao);
        glBindFramebuffer(GL_FRAMEBUFFER, target.id());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(0, 0, 0, 0);
        return true;
    }

    void upload(const lighting::EnvironmentAsset& asset) {
        const auto metadata = lighting::environment_metadata(asset.probes);
        CHECK(selection.prepare(metadata, error));
        shader.use();
        glUniform1i(shader.uniform("environment_count"), static_cast<int>(asset.probes.size()));
        glUniform1i(shader.uniform("environment_resolution"), static_cast<int>(asset.resolution));
        glUniform1i(shader.uniform("environment_columns"), static_cast<int>(asset.columns()));
        glUniform1i(shader.uniform("environment_map"), 20);
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
        glActiveTexture(GL_TEXTURE0);
    }

    // Read one channel or candidate word through the requested reflection selection path.
    std::vector<std::uint8_t> draw(
        const renderer::Camera& camera,
        std::array<int, 4> viewport,
        float depth,
        int channel,
        bool tiled,
        float receiver_offset = 0
    ) {
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        shader.use();
        const auto clip = renderer::multiply(camera.projection, camera.view);
        glUniformMatrix4fv(shader.uniform("receiver_clip"), 1, GL_FALSE, clip.data());
        glUniform3f(
            shader.uniform("receiver_eye"),
            camera.position[0],
            camera.position[1],
            camera.position[2]
        );
        glUniform4f(
            shader.uniform("receiver_viewport"),
            float(viewport[0]),
            float(viewport[1]),
            float(viewport[2]),
            float(viewport[3])
        );
        glUniform1f(shader.uniform("receiver_depth"), depth);
        glUniform1i(shader.uniform("channel"), channel);
        glUniform3f(shader.uniform("receiver_offset"), 0, 0, receiver_offset);
        selection.bind(shader, tiled ? &camera : nullptr);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        return target.read_rgba();
    }

    // Biased receivers must preserve every float bit between full-loop and tiled selection.
    void compare(
        const renderer::Camera& camera,
        std::array<int, 4> viewport,
        float depth,
        float receiver_offset = 0
    ) {
        for (int channel = 0; channel < 3; ++channel) {
            const auto reference = draw(camera, viewport, depth, channel, false, receiver_offset);
            const auto tiled = draw(camera, viewport, depth, channel, true, receiver_offset);
            CHECK(reference == tiled);
            ++comparisons;
        }
    }

    void release() {
        selection.release();
        shader.release();
        glDeleteTextures(1, &texture);
        glDeleteVertexArrays(1, &vao);
        target.release();
    }
};
}  // namespace

int main(int argc, char** argv) {
    test::GlContext context;
    std::string error;

    if (!context.open(error) || !is_vulkan() || !enhanced_rendering_available()) {
        return test::kSkip;
    }

    Fixture fixture;
    CHECK(fixture.open());

    if (!fixture.error.empty()) {
        std::fprintf(stderr, "%s\n", fixture.error.c_str());
    }

    renderer::Camera camera;
    camera.view = renderer::identity();
    camera.projection = renderer::perspective_reversed(1, 1.2f, .1f, 100);
    fixture.upload(synthetic(32, false));

    for (auto viewport :
         {std::array{0, 0, 96, 80}, std::array{3, 5, 73, 51}, std::array{0, 0, 47, 35}}) {
        for (float depth : {.005f, .02f, .6f}) {
            fixture.compare(camera, viewport, depth);
        }
    }

    // Masks must really remove candidates; this also covers bit 31 without signed shifts.
    const auto masks = fixture.draw(camera, {0, 0, 96, 80}, .02f, -1, true);
    bool reduced = false;

    for (std::size_t i = 0; i < masks.size(); i += 4) {
        const auto word = unsigned(masks[i]) | (unsigned(masks[i + 1]) << 8)
                          | (unsigned(masks[i + 2]) << 16) | (unsigned(masks[i + 3]) << 24);
        reduced |= word != 0 && std::popcount(word) < 32;
    }

    CHECK(reduced);
    fixture.upload(synthetic(32, true));
    fixture.compare(camera, {0, 0, 96, 80}, .02f);  // All candidates and exact score ties.
    fixture.compare(camera, {0, 0, 96, 80}, .02f, .5f);   // Water above the interface.
    fixture.compare(camera, {0, 0, 96, 80}, .02f, -.5f);  // Camera below the interface.
    auto large = synthetic(lighting::kEnvironmentProbeLimit, false);

    for (auto& p : large.probes) {
        p = {{40, 0, -8}, {35, -5, -20}, {45, 5, -2}};
    }

    large.probes.back() = {{0, 0, -8}, {-12, -10, -30}, {12, 10, 3}};
    fixture.upload(large);
    fixture.compare(camera, {0, 0, 96, 80}, .02f);  // Only the last supported probe contributes.
    const auto high = fixture.draw(camera, {0, 0, 96, 80}, .02f, 0, true);
    const auto centre = (std::size_t(kHeight / 2) * kWidth + kWidth / 2) * 4;
    const auto bits = unsigned(high[centre]) | (unsigned(high[centre + 1]) << 8)
                      | (unsigned(high[centre + 2]) << 16) | (unsigned(high[centre + 3]) << 24);
    const auto brdf = [&](unsigned row) {
        return lighting::unpack_environment_colour(
            large.pixels[std::size_t(large.brdf_row() + row) * large.width() + 63]
        );
    };
    const auto a = brdf(23), b = brdf(24);
    const float radiance = lighting::unpack_environment_colour(
        lighting::pack_environment_colour(
            {.03f + float((lighting::kEnvironmentProbeLimit - 1) % 7) * .1f, 0, 0}
        )
    )[0];
    const float expected =
        radiance * (.65f * (a[0] * .69f + b[0] * .31f) + a[1] * .69f + b[1] * .31f);
    CHECK(std::abs(std::bit_cast<float>(bits) - expected) < .002f);
    auto rotated = synthetic(32, false);

    for (unsigned i = 0; i < rotated.probes.size(); ++i) {
        auto& p = rotated.probes[i];
        const float angle = float(i % 9) * .17f;
        p.frame.axis = {std::cos(angle), std::sin(angle)};
        p.position = p.frame.to_world(p.position);
    }

    fixture.upload(rotated);

    for (float depth : {.005f, .02f, .6f}) {
        fixture.compare(camera, {3, 5, 73, 51}, depth);
    }

    fixture.upload(synthetic(7, false));
    fixture.compare(camera, {0, 0, 96, 80}, .02f);

    if (argc > 1) {
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        const auto asset = lighting::decode_environment(bytes);
        fixture.upload(asset);
        const auto stride = std::max(std::size_t{1}, asset.probes.size() / 8);

        for (std::size_t i = 0; i < asset.probes.size(); i += stride) {
            camera.position = asset.probes[i].position;
            camera.view = renderer::look_along(camera.position, {.4f, .7f, -.6f}, {0, 0, 1});
            fixture.compare(camera, {3, 5, 73, 51}, .02f);
            fixture.compare(camera, {0, 0, 96, 80}, .005f);
        }
    }

    fixture.release();
    std::printf("%u RGB-channel frame comparisons (float-bit exact)\n", fixture.comparisons);
    return test::result();
}
