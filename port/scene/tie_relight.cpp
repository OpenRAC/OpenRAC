// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/tie_relight.h"

#include <algorithm>
#include <cmath>

namespace openrac::scene {

namespace {

// A double result as float32 rounded toward zero, the vector unit's rounding (gc_tie_colours._t32).
float t32(double x) {
    if (x == 0.0) {
        return 0.0f;
    }
    float f = static_cast<float>(x);
    if (std::fabs(static_cast<double>(f)) > std::fabs(x)) {
        f = std::nextafter(f, 0.0f);
    }
    return f;
}

float mul(float a, float b) {
    return t32(static_cast<double>(a) * static_cast<double>(b));
}

float add(float a, float b) {
    return t32(static_cast<double>(a) + static_cast<double>(b));
}

// The executable's table of (cos, sin) of k turns / 256 that a slot's normal code indexes.
const std::array<std::array<float, 2>, 256>& trig() {
    static const auto table = [] {
        std::array<std::array<float, 2>, 256> t{};
        for (int k = 0; k < 256; ++k) {
            const double a = k * 2.0 * 3.14159265358979323846 / 256.0;
            t[static_cast<std::size_t>(k)] =
                {static_cast<float>(std::cos(a)), static_cast<float>(std::sin(a))};
        }
        return t;
    }();
    return table;
}

std::array<float, 3> unit(const float* v) {
    const float length = add(add(mul(v[0], v[0]), mul(v[1], v[1])), mul(v[2], v[2]));
    if (length <= 0.0f) {
        return {0.0f, 0.0f, 0.0f};
    }
    const float q = t32(1.0 / std::sqrt(static_cast<double>(length)));
    return {mul(v[0], q), mul(v[1], q), mul(v[2], q)};
}

using Rgba = std::array<int, 4>;

Rgba bytes(std::uint32_t w) {
    return {
        static_cast<int>(w & 0xFF),
        static_cast<int>((w >> 8) & 0xFF),
        static_cast<int>((w >> 16) & 0xFF),
        static_cast<int>(w >> 24)
    };
}

}  // namespace

void relight_tie(
    const TieRelightClass& cls,
    std::span<const std::uint32_t> slots,
    const renderer::Mat4& m,
    std::span<const float, 16> set,
    std::vector<std::uint32_t>& out,
    std::span<const TiePointLight> points,
    const std::array<float, 4>& sphere
) {
    // The light directions in the instance's space, through its unit axes (gc_tie_colours.light_slots).
    std::array<std::array<float, 3>, 3> axes{};
    for (std::size_t j = 0; j < 3; ++j) {
        const double length = std::sqrt(
            static_cast<double>(m[4 * j]) * m[4 * j]
            + static_cast<double>(m[4 * j + 1]) * m[4 * j + 1]
            + static_cast<double>(m[4 * j + 2]) * m[4 * j + 2]
        );
        const float w = length != 0.0 ? t32(1.0 / length) : 0.0f;
        axes[j] = {mul(m[4 * j], w), mul(m[4 * j + 1], w), mul(m[4 * j + 2], w)};
    }
    auto into = [&](const std::array<float, 3>& light) {
        std::array<float, 3> r{};
        for (std::size_t j = 0; j < 3; ++j) {
            r[j] = -add(
                add(mul(axes[j][0], light[0]), mul(axes[j][1], light[1])), mul(axes[j][2], light[2])
            );
        }
        return r;
    };
    const float* c0 = set.data();
    const float* c1 = set.data() + 8;
    const auto r0 = into(unit(set.data() + 4));
    const auto r1 = into(unit(set.data() + 12));
    const float ceiling = t32(65537.9);

    std::array<Rgba, 512> lit{};
    const std::size_t count = std::min({slots.size(), cls.normals.size(), std::size_t{512}});
    for (std::size_t i = 0; i < count; ++i) {
        const Rgba colour = bytes(slots[i]);
        const std::uint16_t code = cls.normals[i];
        const auto& lo = trig()[code & 0xFF];
        const auto& hi = trig()[code >> 8];
        const std::array<float, 3> n{mul(lo[0], hi[0]), mul(lo[1], hi[0]), hi[1]};
        float d0 = add(add(mul(n[0], r0[0]), mul(n[1], r0[1])), mul(n[2], r0[2]));
        float d1 = add(add(mul(n[0], r1[0]), mul(n[1], r1[1])), mul(n[2], r1[2]));
        d0 = std::max(d0, mul(d0, c0[3]));
        d1 = std::max(d1, mul(d1, c1[3]));
        Rgba v{0, 0, 0, colour[3]};
        for (std::size_t k = 0; k < 3; ++k) {
            float a = add(65536.0f, static_cast<float>(colour[k] / 128.0));
            a = add(a, mul(c0[k], d0));
            a = add(a, mul(c1[k], d1));
            a = std::min(a, ceiling);
            const double scaled = std::nearbyint((static_cast<double>(a) - 65536.0) * 128.0);
            v[k] = static_cast<int>(std::clamp(scaled, 0.0, 255.0));
        }
        lit[i] = v;
    }
    if (cls.flat) {
        lit[511] = bytes(*cls.flat);
    }

    // The point lights' pass, on the lit slots (0x2F4688): the slot's position by the matrix's
    // columns times the class scale / 1024, its normal by the unit columns.
    const std::size_t placed = std::min(count, cls.positions.size() / 3);
    const float s = mul(cls.scale, 1.0f / 1024.0f);
    for (const TiePointLight& light : points) {
        const float reach = std::max(light.reach, 1.0f);
        const float away[3] =
            {add(sphere[0], -light.position[0]),
             add(sphere[1], -light.position[1]),
             add(sphere[2], -light.position[2])};
        const float both = add(sphere[3], reach);
        const float d2 =
            add(add(mul(away[0], away[0]), mul(away[1], away[1])), mul(away[2], away[2]));
        if (add(d2, -mul(both, both)) >= 0.0f) {
            continue;
        }
        const float q = t32(1.0 / static_cast<double>(reach));
        std::array<float, 3> colour{};
        for (std::size_t k = 0; k < 3; ++k) {
            colour[k] = mul(mul(light.colour[k], 128.0f), q);
        }
        for (std::size_t i = 0; i < placed; ++i) {
            const float x = cls.positions[3 * i], y = cls.positions[3 * i + 1],
                        z = cls.positions[3 * i + 2];
            std::array<float, 3> p{};
            for (std::size_t r = 0; r < 3; ++r) {
                p[r] =
                    add(add(add(mul(mul(m[r], s), x), mul(mul(m[4 + r], s), y)),
                            mul(mul(m[8 + r], s), z)),
                        m[12 + r]);
            }
            const std::uint16_t code = cls.normals[i];
            const auto& lo = trig()[code & 0xFF];
            const auto& hi = trig()[code >> 8];
            const float n0 = mul(lo[0], hi[0]), n1 = mul(lo[1], hi[0]), n2 = hi[1];
            std::array<float, 3> n{};
            for (std::size_t r = 0; r < 3; ++r) {
                n[r] = add(add(mul(axes[0][r], n0), mul(axes[1][r], n1)), mul(axes[2][r], n2));
            }
            // The first slot's normal is turned by vf17 to vf19, not the unit columns: the
            // directional programs (0x2C0, 0x420) leave there their last batch's light terms times
            // the back factors (0x2F48xx's prologue). With both back factors 0 those are 0, so
            // the slot is lit as if it faced the light edge-on; otherwise they hold the dots of
            // slots past the class's count, which are not known here, and 0 is taken too.
            if (i == 0) {
                n = {0.0f, 0.0f, 0.0f};
            }
            const float v[3] =
                {add(light.position[0], -p[0]),
                 add(light.position[1], -p[1]),
                 add(light.position[2], -p[2])};
            const float dd = add(add(mul(v[0], v[0]), mul(v[1], v[1])), mul(v[2], v[2]));
            if (dd <= 0.0f) {
                continue;
            }
            const float inv = t32(1.0 / std::sqrt(static_cast<double>(dd)));
            const float d = mul(dd, inv);
            const float rd = add(reach, -d);
            const float fall = mul(q, rd);
            const float dot =
                add(add(mul(mul(v[0], inv), n[0]), mul(mul(v[1], inv), n[1])),
                    mul(mul(v[2], inv), n[2]));
            const float f = std::max(add(dot, fall), 0.0f);
            for (std::size_t k = 0; k < 3; ++k) {
                const int term = std::max(static_cast<int>(mul(mul(colour[k], rd), f)), 0);
                lit[i][k] = std::min(lit[i][k] + term, 255);
            }
        }
    }

    // The class's remap, vertex by vertex (gc_tie_colours.mix).
    const std::size_t vertices = cls.vertices.size() / 5;
    out.resize(vertices);
    for (std::size_t v = 0; v < vertices; ++v) {
        const std::uint16_t* r = cls.vertices.data() + 5 * v;
        const auto& a = lit[r[1] & 511];
        const auto& b = lit[r[2] & 511];
        const auto& c = lit[r[3] & 511];
        const auto& d = lit[r[4] & 511];
        Rgba x{};
        for (std::size_t k = 0; k < 4; ++k) {
            switch (r[0]) {
                case 1:
                    x[k] = a[k];
                    break;
                case 2:
                    x[k] = (a[k] + b[k]) >> 1;
                    break;
                case 3:
                    x[k] = (3 * a[k] + b[k]) >> 2;
                    break;
                case 4:
                    x[k] = (2 * a[k] + b[k] + c[k]) >> 2;
                    break;
                case 5:
                    x[k] = (a[k] + b[k] + c[k] + d[k]) >> 2;
                    break;
                default:
                    x[k] = 0;
                    break;
            }
        }
        out[v] = static_cast<std::uint32_t>(x[0]) | static_cast<std::uint32_t>(x[1]) << 8
                 | static_cast<std::uint32_t>(x[2]) << 16 | static_cast<std::uint32_t>(x[3]) << 24;
    }
}

}  // namespace openrac::scene
