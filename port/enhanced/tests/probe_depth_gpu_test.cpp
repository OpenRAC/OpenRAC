// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compares the production depth sampler and atlas upload against independently filtered scalar codes.
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>

#include "enhanced/lighting/probe_compression_tile.h"
#include "enhanced/lighting/probe_depth.h"
#include "enhanced/lighting/probe_texture_encode.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/framebuffer.h"
#include "tests/check.h"
#include "tests/renderer_gl_context.h"

namespace {
using namespace openrac;
using namespace gl;
constexpr unsigned kSide =
    16;  // Includes exact octahedral edges and interior interpolation samples.

// Fullscreen triangle needs no source vertex attributes.
constexpr const char* kDepthVertex = R"(#version 410 core
void main() {
    vec2 p = vec2(float((gl_VertexID<<1)&2), float(gl_VertexID&2));
    gl_Position = vec4(p*2.-1., 0, 1);
})";

// Encodes the sampler result as float bits for independent CPU comparison.
constexpr const char* kDepthFragment = R"(
uniform int sample_probe;
uniform float fixture_distance;
layout(location=0) out vec4 result;
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 uv = vec2(pixel.x / 2, pixel.y) / 15.;
    vec3 direction = vec3(uv*2.-1., 0);
    direction.z = 1.-abs(direction.x)-abs(direction.y);
    if (direction.z < 0.) {
        direction.xy = (1.-abs(direction.yx)) * mix(vec2(-1),vec2(1),greaterThanEqual(direction.xy,vec2(0)));
    }
    ivec2 tile = ivec2(sample_probe % probe_tiles_x, sample_probe / probe_tiles_x);
    float value = probe_depth_sample(tile, direction, fixture_distance)[pixel.x % 2];
    uint bits = floatBitsToUint(value);
    result = vec4(bits & 255u, (bits>>8)&255u, (bits>>16)&255u, bits>>24)/255.;
})";

// Owns fixture objects until all reads have completed or setup has failed.
struct Fixture {
    std::array<unsigned, 3> scalar_textures{};  // Independent raw scalar fixture for ASTC-less CI.
    unsigned vao = 0;                           // Fullscreen triangle has no vertex buffer.
    renderer::FrameBuffer frame;
    renderer::Shader shader;
    lighting::ProbeLighting probes;

    Fixture() = default;
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    // Releases the VAO while the hidden Vulkan context still exists.
    ~Fixture() {
        glDeleteTextures(static_cast<GLsizei>(scalar_textures.size()), scalar_textures.data());
        glDeleteVertexArrays(1, &vao);
    }
};

// Builds open, shallow low-spread and ordinary mixed fields without exceeding the ray cap.
lighting::Float2 source_moments(std::size_t probe, unsigned x, unsigned y, float cap) {
    // The first tile is an exact open field.
    if (probe == 0) {
        return {cap, cap * cap};
    }

    float mean = 2.f + static_cast<float>(x + y + probe % 11);
    float variance = static_cast<float>(x % 3);

    // Tiny shallow spreads share a tile with broad uncertainty, reproducing RG8 outliers.
    if (probe == 1) {
        mean = x <= 3 ? .033f + .0001f * static_cast<float>(x + y) : 16.f;
        variance = x <= 3 ? .008f * .008f : 15.f * 15.f;
    }

    return {mean, mean * mean + variance};
}

// Includes open, constant-depth and mixed-depth tiles to exercise zero ranges and total variance.
lighting::ProbeVolume source_volume() {
    lighting::ProbeGrid grid;
    grid.max_distance = 32;
    grid.dimensions = {5, 4, 8};  // 160 tiles cross both file and widened runtime row boundaries.
    auto volume = lighting::ProbeVolume::allocate(grid);
    for (std::size_t p = 0; p < volume.probes.size(); ++p) {
        volume.probes[p].valid = true;
        for (unsigned y = 1; y <= 6; ++y) {
            for (unsigned x = 1; x <= 6; ++x) {
                const auto at = p * 64 + static_cast<std::size_t>(y) * 8 + x;
                volume.distance_moments[at] = source_moments(p, x, y, grid.max_distance);
                volume.irradiance[at] = {1, 1, 1};
            }
        }
    }
    lighting::update_probe_gutters(volume);
    return volume;
}

// The reference reconstructs local means before summing within- and between-texel variance.
lighting::Float2 reference(
    const lighting::ProbeVolume& volume,
    const std::vector<lighting::Float2>& codes,
    std::size_t probe,
    lighting::Float2 uv
) {
    // Match the fragment's single octahedral decode/encode before selecting neighbours.
    uv = lighting::oct_encode(lighting::oct_decode(uv));
    const float px = .5f + uv[0] * 6, py = .5f + uv[1] * 6;
    const unsigned x = static_cast<unsigned>(px), y = static_cast<unsigned>(py);
    const float u = px - static_cast<float>(x), v = py - static_cast<float>(y);
    const std::array<float, 4> weights{(1 - u) * (1 - v), u * (1 - v), (1 - u) * v, u * v};
    std::array<lighting::Float2, 4> samples{};
    float mean = 0;
    for (unsigned i = 0; i < samples.size(); ++i) {
        const unsigned p = lighting::compression::gutter_source((y + i / 2) * 8 + x + i % 2);
        const auto at =
            (probe / lighting::kProbeAtlasTilesX * 8 + p / 8) * lighting::kProbeAtlasTilesX * 8
            + probe % lighting::kProbeAtlasTilesX * 8 + p % 8;
        samples[i] = codes[at];
        mean += weights[i] * samples[i][0];
    }
    const auto ranges = lighting::probe_depth_parameters(volume, probe);
    float between = 0, within = 0;
    for (unsigned i = 0; i < samples.size(); ++i) {
        const float delta = samples[i][0] - mean;
        between += weights[i] * delta * delta;
        const float world_mean = ranges[0] + ranges[1] * samples[i][0];
        const float spread = ranges[2] * samples[i][1];
        within += weights[i]
                  * std::min(spread * spread, std::max(0.f, world_mean * (ranges[3] - world_mean)));
    }
    return {ranges[0] + ranges[1] * mean, ranges[1] * ranges[1] * between + within};
}

// Recovers an exact float from four little-endian render-target bytes without alignment assumptions.
float read_float(std::span<const std::uint8_t> pixels, std::size_t offset) {
    std::uint32_t bits = 0;
    for (unsigned byte = 0; byte < sizeof(float); ++byte) {
        bits |= std::uint32_t(pixels[offset + byte]) << (byte * 8);
    }
    return std::bit_cast<float>(bits);
}

// Allows FP32 arithmetic error between paths reading identical codes, without hiding codec errors.
void check_moments(std::span<const std::uint8_t> pixels, lighting::Float2 expected) {
    constexpr float kAbsoluteTolerance = .0000001f, kRelativeTolerance = .00002f;

    // Mean and variance have different units, so each uses its own magnitude for relative error.
    for (std::size_t channel = 0; channel < expected.size(); ++channel) {
        const float tolerance =
            kAbsoluteTolerance + kRelativeTolerance * std::abs(expected[channel]);
        CHECK(std::abs(read_float(pixels, channel * 4) - expected[channel]) < tolerance);
    }
}

// Checks seam and interior directions independently of the shader's filter implementation.
void check_pixels(
    const lighting::ProbeVolume& volume,
    const std::vector<lighting::Float2>& codes,
    unsigned probe,
    std::span<const std::uint8_t> pixels,
    bool front_guard
) {
    for (unsigned pixel = 0; pixel < kSide * kSide; ++pixel) {
        const unsigned x = pixel % kSide, y = pixel / kSide;
        auto expected = reference(
            volume,
            codes,
            probe,
            {static_cast<float>(x) / (kSide - 1), static_cast<float>(y) / (kSide - 1)}
        );
        // A receiver at zero distance is before every fixture mean, so variance is unused.
        if (front_guard) {
            expected[1] = 0;
        }
        const auto offset = pixel * 2 * 4;
        check_moments(pixels.subspan(offset, 8), expected);
    }
}

// Exercises the real upload first and an independent scalar upload second on ASTC-less CI.
void draw_and_check(
    Fixture& fixture,
    const lighting::ProbeVolume& volume,
    const std::vector<lighting::Float2>& codes,
    bool front_guard = false
) {
    glUniform1f(fixture.shader.uniform("fixture_distance"), front_guard ? 0.f : 1e30f);
    for (unsigned probe = 0; probe < volume.probes.size(); ++probe) {
        glUniform1i(fixture.shader.uniform("sample_probe"), static_cast<int>(probe));
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::vector<std::uint8_t> pixels(std::size_t{kSide} * 2 * kSide * 4);
        glReadPixels(0, 0, kSide * 2, kSide, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        check_pixels(volume, codes, probe, pixels, front_guard);
    }
}

// Forces the native ASTC arithmetic using decoded scalar texels without requiring ASTC hardware.
void prepare_manual_filter(
    Fixture& fixture,
    const lighting::ProbeVolume& volume,
    const std::vector<lighting::Float2>& codes
) {
    constexpr std::array<unsigned, 3> kUnits{4, 30, 31};
    constexpr unsigned kTileSide = 8;
    std::vector<lighting::Float2> spread(codes.size());
    for (std::size_t p = 0; p < codes.size(); ++p) {
        spread[p] = {codes[p][1], 0};
    }
    glGenTextures(static_cast<GLsizei>(kUnits.size()), fixture.scalar_textures.data());
    for (unsigned i = 0; i < kUnits.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + kUnits[i]);
        glBindTexture(GL_TEXTURE_2D, fixture.scalar_textures[i]);
        const bool ranges = i == 2;
        const auto width = lighting::kProbeAtlasTilesX * (ranges ? 1 : kTileSide);
        const auto height = static_cast<unsigned>(volume.atlas_rows()) * (ranges ? 1 : kTileSide);
        const void* data = codes.data();
        // The spread texture's first channel must be independent of the mean texture.
        if (i == 1) {
            data = spread.data();
        } else if (ranges) {
            data = volume.gpu_plane(4).data();
        }
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            ranges ? GL_RGBA32F : GL_RG32F,
            static_cast<GLsizei>(width),
            static_cast<GLsizei>(height),
            0,
            ranges ? GL_RGBA : GL_RG,
            GL_FLOAT,
            data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    // Independent scalar textures retain file width rather than the runtime's widened atlas.
    glUniform1i(fixture.shader.uniform("probe_tiles_x"), lighting::kProbeAtlasTilesX);
    glUniform1i(fixture.shader.uniform("probe_depth_mode"), 1);
}

// Directly invokes the production shader helper, including its native or expanded format selection.
void test_depth_sampler(lighting::ProbeDepthFormat format) {
    const auto source = source_volume();
    const auto encoded =
        lighting::encode_baked_probes(source, lighting::ProbeTextureFormat::R11G11B10, {}, format);
    // Distinct depth tiles must survive concurrent row writes.
    lighting::BakeThreadPool workers(3);
    CHECK(
        encoded
        == lighting::encode_baked_probes(
            source, lighting::ProbeTextureFormat::R11G11B10, {0, &workers}, format
        )
    );
    const auto volume = lighting::decode_gpu_probes(encoded);
    const auto codes = lighting::decode_probe_depth(volume);
    Fixture fixture;
    std::string error;
    CHECK(fixture.probes.upload(volume, error));
    CHECK(error.empty());
    const std::string fragment =
        "#version 410 core\n" + std::string(lighting::shaders::probes_glsl) + kDepthFragment;
    CHECK(fixture.shader.build("probe depth filtering", kDepthVertex, fragment, error));
    CHECK(error.empty());
    CHECK(fixture.frame.create(kSide * 2, kSide, error));
    glGenVertexArrays(1, &fixture.vao);
    glBindVertexArray(fixture.vao);
    glBindFramebuffer(GL_FRAMEBUFFER, fixture.frame.id());
    glViewport(0, 0, kSide * 2, kSide);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    fixture.shader.use();
    fixture.probes.bind(fixture.shader, scene::Layer::Terrain);
    draw_and_check(fixture, volume, codes);
    draw_and_check(fixture, volume, codes, true);
    prepare_manual_filter(fixture, volume, codes);
    draw_and_check(fixture, volume, codes);
    CHECK(glGetError() == GL_NO_ERROR);
}
}  // namespace

// Both fallback sampling and native scalar arithmetic run on desktop GPUs without ASTC support.
int main() {
    try {
        test::GlContext context;
        std::string error;
        // A missing graphics surface skips the fixture rather than failing CPU-only CI.
        if (!context.open(error)) {
            std::printf("SKIP: %s\n", error.c_str());
            return test::kSkip;
        }
        test_depth_sampler(lighting::ProbeDepthFormat::Rg8);
        test_depth_sampler(lighting::ProbeDepthFormat::Rg16);
        return test::result();
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "%s\n", failure.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "unknown depth GPU fixture failure\n");
        return 1;
    }
}
