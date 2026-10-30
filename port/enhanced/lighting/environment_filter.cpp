// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline importance sampling; roughness levels use the same GGX alpha = roughness squared as water.
#include "enhanced/lighting/environment_filter.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include "common/profiler.h"

namespace openrac::lighting {
namespace {
constexpr float kPi = std::numbers::pi_v<float>;

// Scalar and vector arithmetic for filtering, independent of the Embree module.
float dot(Float3 a, Float3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Float3 unit(Float3 a) {
    const float length = std::sqrt(dot(a, a));

    for (auto& x : a) {
        x /= length;
    }

    return a;
}

Float3 cross(Float3 a, Float3 b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

// Stable low-discrepancy sequence, identical on all worker counts.
Float2 sequence(unsigned i, unsigned count) {
    unsigned bits = i;
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xaaaaaaaau) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xccccccccu) >> 2);
    bits = ((bits & 0x0f0f0f0fu) << 4) | ((bits & 0xf0f0f0f0u) >> 4);
    bits = ((bits & 0x00ff00ffu) << 8) | ((bits & 0xff00ff00u) >> 8);
    return {(float(i) + .5f) / float(count), float(bits) * 2.3283064365386963e-10f};
}

// Importance-sample a GGX half vector around the supplied surface normal.
Float3 half_vector(Float3 n, float roughness, Float2 sample) {
    const float alpha = roughness * roughness;
    const float cosine = std::sqrt((1 - sample[1]) / (1 + (alpha * alpha - 1) * sample[1]));
    const float sine = std::sqrt(std::max(0.f, 1 - cosine * cosine)), angle = 2 * kPi * sample[0];
    const auto tangent = unit(cross(std::abs(n[2]) < .99f ? Float3{0, 0, 1} : Float3{0, 1, 0}, n));
    const auto bitangent = cross(n, tangent);
    Float3 result{};

    for (unsigned c = 0; c < 3; ++c) {
        result[c] = tangent[c] * sine * std::cos(angle) + bitangent[c] * sine * std::sin(angle)
                    + n[c] * cosine;
    }

    return result;
}

// Cross a cube edge before reading the neighbouring source face.
Float3 source_texel(std::span<const Float3> faces, unsigned size, unsigned face, int x, int y) {
    if (x < 0 || y < 0 || x >= int(size) || y >= int(size)) {
        const auto mapped = environment_face(environment_direction(
            face, {(float(x) + .5f) / float(size), (float(y) + .5f) / float(size)}
        ));
        face = mapped.first;
        x = std::clamp(int(mapped.second[0] * float(size)), 0, int(size) - 1);
        y = std::clamp(int(mapped.second[1] * float(size)), 0, int(size) - 1);
    }

    return faces[(std::size_t(face) * size + std::size_t(y)) * size + std::size_t(x)];
}

// Bilinear cube sampling is seam-aware before convolution and at mip gutters.
Float3 source_sample(std::span<const Float3> faces, unsigned size, Float3 direction) {
    const auto [face, uv] = environment_face(direction);
    const float x = uv[0] * float(size) - .5f, y = uv[1] * float(size) - .5f;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - float(ix), fy = y - float(iy);
    Float3 result{};

    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            const auto value = source_texel(faces, size, face, ix + column, iy + row);
            const float weight = (column ? fx : 1 - fx) * (row ? fy : 1 - fy);

            for (unsigned c = 0; c < 3; ++c) {
                result[c] += value[c] * weight;
            }
        }
    }

    return result;
}

// Split-sum environment convolution with the reflected direction centred on N = V.
Float3 filtered(
    std::span<const Float3> faces,
    unsigned size,
    Float3 n,
    float roughness,
    std::span<const Float2> samples
) {
    if (roughness == 0) {
        return source_sample(faces, size, n);
    }

    Float3 sum{};
    float weight = 0;

    for (const auto sample : samples) {
        const auto h = half_vector(n, roughness, sample);
        const float nh = dot(n, h);
        const Float3 light{2 * nh * h[0] - n[0], 2 * nh * h[1] - n[1], 2 * nh * h[2] - n[2]};
        const float nl = std::max(0.f, dot(n, light));

        if (nl <= 0) {
            continue;
        }

        const auto value = source_sample(faces, size, light);

        for (unsigned c = 0; c < 3; ++c) {
            sum[c] += value[c] * nl;
        }

        weight += nl;
    }

    for (auto& c : sum) {
        c /= std::max(weight, 1e-6f);
    }

    return sum;
}

// Integrate the Fresnel-independent A/B terms using water's correlated Smith visibility.
Float3 brdf(float nv, float roughness, std::span<const Float2> samples) {
    const Float3 view{std::sqrt(1 - nv * nv), 0, nv};
    const float alpha = roughness * roughness, a2 = alpha * alpha;
    Float3 result{};

    for (const auto sample : samples) {
        const auto h = half_vector({0, 0, 1}, roughness, sample);
        const float vh = std::max(0.f, dot(view, h));
        const float nl = 2 * vh * h[2] - nv;

        if (nl <= 0) {
            continue;
        }

        const float denominator =
            nl * std::sqrt(nv * nv * (1 - a2) + a2) + nv * std::sqrt(nl * nl * (1 - a2) + a2);
        const float visible = 2 * nl * vh / std::max(denominator * h[2], 1e-6f);
        const float fresnel = std::pow(1 - vh, 5.f);
        result[0] += (1 - fresnel) * visible;
        result[1] += fresnel * visible;
    }

    for (auto& c : result) {
        c /= float(samples.size());
    }

    return result;
}
}  // namespace

void filter_environment(
    EnvironmentAsset& asset, unsigned probe, std::span<const Float3> faces, unsigned samples
) {
    const profiling::Zone zone("Filter environment GGX mips");

    if (samples == 0 || faces.size() != std::size_t(6) * asset.resolution * asset.resolution
        || probe >= asset.probes.size()) {
        throw std::runtime_error("invalid environment filter input");
    }

    const auto column = asset.probe_x(probe);
    std::vector<Float2> directions(samples);

    for (unsigned i = 0; i < samples; ++i) {
        directions[i] = sequence(i, samples);
    }

    for (unsigned mip = 0; mip < asset.levels(); ++mip) {
        const unsigned size = asset.face_resolution(mip), side = size + 2;
        const unsigned row = asset.probe_y(probe) + asset.level_row(mip);
        // Preserve the original GGX schedule rather than treating retained mip one as a mirror.
        const float roughness =
            float(mip + kEnvironmentFirstMip) / float(asset.levels() + kEnvironmentFirstMip - 1);

        for (unsigned face = 0; face < 6; ++face) {
            for (unsigned y = 0; y < side; ++y) {
                for (unsigned x = 0; x < side; ++x) {
                    const auto direction = environment_direction(
                        face, {(float(x) - .5f) / float(size), (float(y) - .5f) / float(size)}
                    );
                    asset.pixels[std::size_t(row + y) * asset.width() + column + face * side + x] =
                        pack_environment_colour(
                            filtered(faces, asset.resolution, direction, roughness, directions)
                        );
                }
            }
        }
    }
}

void integrate_environment_brdf(EnvironmentAsset& asset, unsigned samples) {
    if (samples == 0) {
        throw std::runtime_error("empty environment BRDF integration");
    }

    std::vector<Float2> directions(samples);

    for (unsigned i = 0; i < samples; ++i) {
        directions[i] = sequence(i, samples);
    }

    const unsigned begin = asset.brdf_row();

    for (unsigned y = 0; y < kEnvironmentBrdfSize; ++y) {
        for (unsigned x = 0; x < kEnvironmentBrdfSize; ++x) {
            asset.pixels[std::size_t(begin + y) * asset.width() + x] = pack_environment_colour(brdf(
                (float(x) + .5f) / kEnvironmentBrdfSize,
                (float(y) + .5f) / kEnvironmentBrdfSize,
                directions
            ));
        }
    }
}
}  // namespace openrac::lighting
