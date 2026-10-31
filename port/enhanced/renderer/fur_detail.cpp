// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Deterministic directional strand detail, generated once when a supported body is loaded.
#include <algorithm>
#include <cmath>
#include <mutex>

#include "enhanced/renderer/fur.h"

namespace openrac::enhanced {
namespace {
// Two detail samples per original atlas texel resolve short hairs in close views.
constexpr int kDetailSide = 512;
constexpr int kWrap = kDetailSide - 1;
using Point = std::array<float, 2>;

// Owns the UI/render-worker tuning without exposing mutable references across threads.
struct DetailState {
    std::mutex mutex;
    FurMaterialSettings settings;
};

DetailState& detail_state() {
    static DetailState state;
    return state;
}

// Fixed integer noise makes the same strands survive level reloads and arbitrary animation.
float noise(int x, int y) {
    std::uint32_t value = unsigned(x & kWrap) + unsigned(y & kWrap) * unsigned(kDetailSide);
    value ^= 0x9e3779b9u;
    value = (value ^ (value >> 16)) * 0x7feb352du;
    value = (value ^ (value >> 15)) * 0x846ca68bu;
    value ^= value >> 16;
    return float(value & 65535u) / 32767.5f - 1;
}

// Bilinear noise avoids introducing discontinuous steps along the sampled strand paths.
float filtered_noise(Point p) {
    const int x = int(std::floor(p[0])), y = int(std::floor(p[1]));
    const float u = p[0] - float(x), v = p[1] - float(y);
    return std::lerp(
        std::lerp(noise(x, y), noise(x + 1, y), u),
        std::lerp(noise(x, y + 1), noise(x + 1, y + 1), u),
        v
    );
}

// Bilinear groom sampling matches the control map's linear, repeating GPU sampler.
Point flow_at(const renderer::Rgba8Image& groom, int x, int y) {
    const float u = (float(x) + .5f) * float(groom.width) / float(kDetailSide) - .5f;
    const float v = (float(y) + .5f) * float(groom.height) / float(kDetailSide) - .5f;
    const int ix = int(std::floor(u)), iy = int(std::floor(v));
    Point flow{};
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            const int sx = ((ix + dx) % groom.width + groom.width) % groom.width;
            const int sy = ((iy + dy) % groom.height + groom.height) % groom.height;
            const auto texel = groom.texel(sx, sy);
            const float weight =
                (dx ? u - float(ix) : 1 - u + float(ix)) * (dy ? v - float(iy) : 1 - v + float(iy));
            flow[0] += (float((texel >> 8) & 255u) * 2 / 255 - 1) * weight;
            flow[1] += (float((texel >> 16) & 255u) * 2 / 255 - 1) * weight;
        }
    }
    const float length = std::hypot(flow[0], flow[1]);
    // Outside the padded groom, use a stable direction rather than normalising near-zero data.
    if (length < .01f) {
        return {0, 1};
    }
    return {flow[0] / length, flow[1] / length};
}

// Nine weighted samples elongate the noise into irregular strands, avoiding periodic grooves.
float strand_height(Point p, Point flow) {
    constexpr std::array<float, 9> kWeights{1, 3, 6, 9, 10, 9, 6, 3, 1};
    float result = 0;
    for (int tap = 0; tap < int(kWeights.size()); ++tap) {
        const float offset = float(tap - 4) * 2;
        result += kWeights[std::size_t(tap)]
                  * filtered_noise({p[0] + flow[0] * offset, p[1] + flow[1] * offset});
    }
    return result / 48;
}

// Encodes signed slopes around byte 128, which also makes flat mip levels exactly neutral.
unsigned signed_byte(float value) {
    return unsigned(std::lround(128 + 127 * std::clamp(value, -1.f, 1.f)));
}

renderer::Rgba8Image encode_detail(const std::vector<float>& height) {
    renderer::Rgba8Image image(kDetailSide, kDetailSide);
    const auto at = [&](int x, int y) {
        return height[std::size_t(y & kWrap) * std::size_t(kDetailSide) + std::size_t(x & kWrap)];
    };
    for (int y = 0; y < kDetailSide; ++y) {
        for (int x = 0; x < kDetailSide; ++x) {
            // Bounded gains keep the directional slopes and low-contrast colour grain in UNORM.
            const float u = (at(x + 1, y) - at(x - 1, y)) * 3;
            const float v = (at(x, y + 1) - at(x, y - 1)) * 3;
            image.set_texel(
                x, y, renderer::rgba(signed_byte(u), signed_byte(v), signed_byte(at(x, y) * 3), 255)
            );
        }
    }
    return image;
}
}  // namespace

FurMaterialSettings fur_material_settings() {
    auto& state = detail_state();
    const std::lock_guard lock(state.mutex);
    return state.settings;
}

void set_fur_material_settings(FurMaterialSettings settings) {
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    const FurMaterialSettings defaults;
    settings.strength = bounded(settings.strength, 0, 1, defaults.strength);
    settings.contrast = bounded(settings.contrast, 0, .15f, defaults.contrast);
    settings.fade_distance = bounded(settings.fade_distance, 8, 160, defaults.fade_distance);
    settings.highlight = bounded(settings.highlight, 0, 2, defaults.highlight);
    settings.softness = bounded(settings.softness, .35f, 1, defaults.softness);
    settings.directionality = bounded(settings.directionality, 0, .9f, defaults.directionality);
    settings.scattering = bounded(settings.scattering, 0, .5f, defaults.scattering);
    settings.environment_strength =
        bounded(settings.environment_strength, 0, 4, defaults.environment_strength);
    auto& state = detail_state();
    const std::lock_guard lock(state.mutex);
    state.settings = settings;
}

renderer::Rgba8Image make_fur_detail_map(const renderer::Rgba8Image& groom) {
    if (groom.width <= 0 || groom.height <= 0
        || groom.pixels.size() != std::size_t(groom.width) * std::size_t(groom.height) * 4) {
        return {};
    }
    std::vector<float> height(std::size_t(kDetailSide) * std::size_t(kDetailSide));
    for (int y = 0; y < kDetailSide; ++y) {
        for (int x = 0; x < kDetailSide; ++x) {
            const auto flow = flow_at(groom, x, y);
            height[std::size_t(y) * std::size_t(kDetailSide) + std::size_t(x)] =
                strand_height({float(x) + .5f, float(y) + .5f}, flow);
        }
    }
    // Coverage is applied during shading, never to height: material boundaries cannot form raised seams.
    return encode_detail(height);
}
}  // namespace openrac::enhanced
