// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>

#include "renderer/math.h"

namespace openrac::scene::vendor_preview {
struct Adjustment {
    float x = 0, y = 0, size = 1;
    float pitch = 0, yaw = 0, roll = 0;
    bool spin = true;
    float pivot_x = 0, pivot_y = 0, pivot_z = 0;
    bool pivot_enabled = false;
};

inline int key(int id, bool middle) {
    return id + (middle ? 65536 : 0);
}

inline int normal_weapon_class(int id) {
    constexpr int bases[] = {192, 176, 168, 229, 190, 157, 562, 177, 849, 185};
    return id >= 1456 && id < 1466 ? bases[id - 1456] : id;
}

// Approved in-game vendor framing from Batches 1, 2 and 3; displays are independent.
inline Adjustment defaults(int id) {
    switch (id) {
        case 173:
            return {
                42.75f, -12.75f, 1.45299887f, 12.0f, 8.0f, -102.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 185:
            return {36.0f, -3.25f, 1.0f, 0.0f, 0.0f, 0.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 188:
            return {38.5f, -4.25f, 0.738999307f, 0.0f, -90.0f, 0.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 195:
            return {
                42.5f, -10.25f, 1.44799888f, 12.0f, 8.5f, -102.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 208:
            return {46.75f, -13.0f, 1.72099888f, -2.5f, 0.0f, 13.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 407:
            return {92.5f, -34.5f, 4.0f, 0.0f, 0.0f, 198.5f, true, 0.0f, 0.0f, 0.0f, false};
        case 433:
            return {41.0f, -9.0f, 1.26399886f, 14.5f, 6.5f, -137.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 439:
            return {
                22.25f, -0.25f, 0.212999865f, 37.0f, 7.0f, -84.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 483:
            return {
                36.0f, -4.75f, 1.1779989f, 124.5f, -41.0f, 84.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 585:
            return {
                39.5f, -18.0f, 1.49399876f, 0.0f, -116.0f, -15.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 607:
            return {
                25.25f, -4.75f, 0.681999981f, -27.5f, 9.0f, -338.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 608:
            return {
                42.0f, -5.75f, 1.45299887f, 25.0f, -8.5f, -139.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 609:
            return {44.5f, -17.5f, 1.6219989f, -25.5f, 16.5f, 28.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 614:
            return {
                61.25f, -12.75f, 2.5579977f, 5.5f, -15.0f, -58.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 618:
            return {90.0f, -4.0f, 3.8939979f, 0.0f, 0.0f, -253.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 619:
            return {
                17.25f, -5.0f, 0.193999856f, 120.5f, -57.0f, 97.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 750:
            return {15.75f, 0.0f, 0.517799854f, 0.0f, 0.0f, 0.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 849:
            return {36.5f, -5.0f, 1.0f, 0.0f, 0.0f, 0.0f, true, 0.0f, 0.0f, 0.0f, false};
        case 1134:
            return {23.0f, 1.0f, 0.75879997f, -39.0f, -1.0f, 39.5f, true, 0.0f, 0.0f, 0.0f, false};
        case 1251:
            return {
                31.75f, -5.25f, 0.798999428f, 0.0f, 90.5f, 17.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1289:
            return {
                35.0f, -12.75f, 1.20899891f, 14.5f, 4.0f, -138.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1290:
            return {
                35.0f, -12.25f, 1.30699884f, 15.0f, -6.0f, -139.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1456:
            return {
                18.5f, 0.0f, 0.0275999903f, -176.0f, -204.0f, 113.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1457:
            return {
                26.5f, 0.0f, 0.0846999585f, -78.5f, 12.0f, -88.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1458:
            return {
                12.0f, -3.5f, 0.0856999605f, 85.0f, -180.0f, 89.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1459:
            return {
                17.25f, -2.25f, 0.0497999787f, 3.5f, 25.5f, -73.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1460:
            return {
                22.25f, 0.0f, 0.0385999791f, -8.0f, 23.0f, -59.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1461:
            return {
                25.5f, 0.0f, 0.119899988f, -270.0f, 171.0f, 90.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1462:
            return {
                16.75f, 0.0f, 0.0366999804f, -6.5f, 26.0f, -71.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1463:
            return {
                15.0f, 5.0f, 0.131699919f, -81.5f, 12.5f, -90.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1464:
            return {
                21.0f, 0.5f, 0.132799923f, -77.5f, -193.5f, 90.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 1465:
            return {
                18.75f, -0.25f, 0.102899961f, 97.5f, -185.5f, 90.5f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 65709:
            return {
                13.4399986f,
                -1.5f,
                1.55199885f,
                0.0f,
                16.0f,
                90.0f,
                true,
                0.427999705f,
                0.267999708f,
                1.98499882f,
                true
            };
        case 65724:
            return {
                18.75f,
                0.0f,
                0.83899939f,
                0.0f,
                -90.0f,
                -235.0f,
                true,
                0.0f,
                -0.149999856f,
                -2.44999766f,
                true
            };
        case 65731:
            return {
                18.6099967f,
                0.75f,
                1.50399887f,
                0.0f,
                16.0f,
                0.0f,
                true,
                -0.0779999271f,
                1.76799881f,
                -0.0509999655f,
                true
            };
        case 65744:
            return {
                18.0f, 0.0f, 1.70899891f, 0.0f, 0.0f, -32.0f, true, -0.949999451f, 1.0f, 0.0f, true
            };
        case 65943:
            return {
                13.5f, 0.0f, 4.0f, 0.0f, 6.5f, 65.0f, true, 0.622999727f, 0.199999928f, 0.0f, true
            };
        case 65969:
            return {
                10.8799982f,
                0.0f,
                1.5429989f,
                0.0f,
                0.0f,
                129.0f,
                true,
                0.772999405f,
                -1.09699881f,
                1.5739988f,
                true
            };
        case 65975:
            return {
                10.1799983f,
                0.0f,
                0.279999703f,
                -21.0f,
                17.0f,
                90.0f,
                true,
                4.87199544f,
                0.0f,
                0.0f,
                true
            };
        case 65990:
            return {
                15.5f,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.0f,
                true,
                -6.92399835f,
                5.29899835f,
                2.29899907f,
                true
            };
        case 66019:
            return {
                15.7999982f,
                -11.25f,
                1.30799889f,
                -5.5f,
                -90.0f,
                -74.5f,
                true,
                0.0f,
                0.475999712f,
                2.47599768f,
                true
            };
        case 66121:
            return {
                18.1599979f,
                7.0f,
                1.66299891f,
                0.0f,
                -90.0f,
                -48.5f,
                true,
                0.0f,
                1.61899888f,
                1.29599881f,
                true
            };
        case 66143:
            return {
                18.0f,
                -13.0f,
                1.0f,
                32.5f,
                0.0f,
                -65.5f,
                true,
                -3.05699992f,
                0.770999968f,
                -2.7559998f,
                true
            };
        case 66144:
            return {
                30.0599975f,
                0.0f,
                1.75899887f,
                0.0f,
                -11.0f,
                -30.0f,
                true,
                -1.36399888f,
                1.79599881f,
                -0.0688475891f,
                true
            };
        case 66145:
            return {
                15.1099987f,
                0.0f,
                1.77399885f,
                0.0f,
                0.0f,
                53.0f,
                true,
                1.02199888f,
                1.07299888f,
                0.0f,
                true
            };
        case 66150:
            return {
                12.1299982f,
                0.0f,
                2.41399765f,
                0.0f,
                0.0f,
                -47.5f,
                true,
                -0.621999442f,
                1.24799883f,
                -6.67299556f,
                true
            };
        case 66154:
            return {
                17.579998f,
                0.0f,
                4.0f,
                0.0f,
                0.0f,
                -141.0f,
                true,
                -0.395999759f,
                -0.498999744f,
                0.798999488f,
                true
            };
        case 66155:
            return {
                16.0599975f,
                -23.75f,
                0.23999986f,
                -126.0f,
                -90.0f,
                -100.5f,
                true,
                0.0739999264f,
                -7.34799528f,
                -7.67399549f,
                true
            };
        case 66286:
            return {0.0f, 44.0f, 0.747899711f, 0.0f, 0.0f, 0.0f, true, 0.0f, 0.0f, 0.0f, true};
        case 66670:
            return {10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, true, 0.0f, 1.64799976f, 0.0f, true};
        case 66787:
            return {
                33.8399925f,
                0.0f,
                1.0f,
                0.0f,
                90.0f,
                -234.0f,
                true,
                -0.199999853f,
                0.167999863f,
                2.89899778f,
                true
            };
        case 66825:
            return {
                19.0699977f,
                0.0f,
                1.30899882f,
                0.0f,
                0.0f,
                -73.0f,
                true,
                -1.9229989f,
                0.39899972f,
                0.0f,
                true
            };
        case 66826:
            return {
                15.6399984f,
                0.0f,
                1.30799889f,
                0.0f,
                -11.5f,
                64.5f,
                true,
                2.14599776f,
                0.671999454f,
                0.0f,
                true
            };
        case 66992:
            return {
                35.6899948f,
                23.0f,
                0.0735999494f,
                -19.0f,
                1.0f,
                -56.5f,
                true,
                -45.1869735f,
                25.1169872f,
                -8.12199401f,
                true
            };
        case 66993:
            return {
                -4.63999938f,
                8.5f,
                0.10769996f,
                -65.0f,
                87.0f,
                -66.0f,
                true,
                43.266983f,
                53.2939834f,
                0.0f,
                true
            };
        case 66994:
            return {
                -2.5f, 0.0f, 0.0848999619f, -90.0f, 124.5f, -90.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 66995:
            return {
                -1.06999993f,
                16.0f,
                0.0878999605f,
                -27.0f,
                -7.0f,
                0.0f,
                true,
                35.5449829f,
                43.7209815f,
                0.0f,
                true
            };
        case 66996:
            return {
                16.3999977f,
                0.0f,
                0.0668999627f,
                -26.0f,
                0.0f,
                0.0f,
                true,
                21.9979915f,
                60.115982f,
                43.721981f,
                true
            };
        case 66997:
            return {
                7.5f,
                6.25f,
                0.147699996f,
                360.0f,
                90.0f,
                -360.0f,
                true,
                26.5139999f,
                44.8739967f,
                -4.86230468f,
                true
            };
        case 66998:
            return {
                0.0f, 10.25f, 0.0677999556f, -23.0f, 8.5f, -61.0f, true, 0.0f, 0.0f, 0.0f, false
            };
        case 66999:
            return {
                24.75f,
                25.75f,
                0.145899921f,
                -90.0f,
                0.0f,
                -90.0f,
                true,
                -27.2979907f,
                1.99899947f,
                0.0f,
                true
            };
        case 67000:
            return {
                8.76999855f,
                0.0f,
                0.156899914f,
                97.0f,
                0.0f,
                -90.0f,
                true,
                -21.5229911f,
                -8.42399597f,
                -24.6509914f,
                true
            };
        case 67001:
            return {
                16.8199977f,
                0.0f,
                0.10989996f,
                90.0f,
                0.0f,
                90.0f,
                true,
                14.2239961f,
                0.0f,
                0.0f,
                true
            };
        default:
            return {};
    }
}

// Immutable profiles approved during playtesting; no runtime editor or file overrides.
inline Adjustment get(int id) {
    return defaults(id);
}

inline renderer::Mat4 orient(
    renderer::Mat4 source, const Adjustment& a, const renderer::Vec3& pivot, bool scale
) {
    if (a.pitch == 0 && a.yaw == 0 && a.roll == 0 && (!scale || a.size == 1)) {
        return source;
    }
    constexpr float radians = 3.14159265359f / 180;
    auto rx = renderer::identity(), ry = rx, rz = rx;
    float c = std::cos(a.pitch * radians), s = std::sin(a.pitch * radians);
    rx[5] = rx[10] = c;
    rx[6] = s;
    rx[9] = -s;
    c = std::cos(a.yaw * radians);
    s = std::sin(a.yaw * radians);
    ry[0] = ry[10] = c;
    ry[2] = -s;
    ry[8] = s;
    c = std::cos(a.roll * radians);
    s = std::sin(a.roll * radians);
    rz[0] = rz[5] = c;
    rz[1] = s;
    rz[4] = -s;
    auto change = renderer::multiply(rz, renderer::multiply(ry, rx));
    if (scale) {
        change = renderer::multiply(change, renderer::scale(a.size));
    }
    change = renderer::multiply(
        renderer::translation(pivot),
        renderer::multiply(change, renderer::translation({-pivot[0], -pivot[1], -pivot[2]}))
    );
    return renderer::multiply(source, change);
}

inline renderer::Mat4 freeze_spin(renderer::Mat4 source, float native_angle, bool spin) {
    if (spin || !std::isfinite(native_angle)) {
        return source;
    }
    // Undo the game's Z spin, rather than freezing a random frame. Saved
    // orientation therefore starts from the same pose after reopening a vendor.
    const float c = std::cos(-native_angle), s = std::sin(-native_angle);
    for (int col = 0; col < 3; ++col) {
        const float x = source[col * 4], y = source[col * 4 + 1];
        source[col * 4] = c * x - s * y;
        source[col * 4 + 1] = s * x + c * y;
    }
    return source;
}

// The approved framing was calibrated at Novalis. Express a stationary pivot
// offset in that display's horizontal axes, then rotate it into this vendor's
// axes. A fixed world-space offset otherwise changes sides as the vendor turns.
inline constexpr renderer::Vec3 reference_right{.803219854f, -.595682740f, 0};

inline renderer::Vec3 pivot_anchor(renderer::Vec3 offset, renderer::Vec3 right) {
    const float length = std::hypot(right[0], right[1]);
    if (length < .0001f) {
        return offset;
    }
    right[0] /= length;
    right[1] /= length;
    const float c = reference_right[0] * right[0] + reference_right[1] * right[1];
    const float s = reference_right[0] * right[1] - reference_right[1] * right[0];
    return {c * offset[0] - s * offset[1], s * offset[0] + c * offset[1], offset[2]};
}

inline renderer::Mat4 spin_pivot(
    renderer::Mat4 source,
    float angle,
    const Adjustment& a,
    const renderer::Vec3& right = reference_right
) {
    if (!a.pivot_enabled) {
        return source;
    }
    const renderer::Vec3 p{a.pivot_x, a.pivot_y, a.pivot_z};
    // Keep the chosen local point at its zero-spin location. Rotation changes
    // its axes, but no longer swings that point around the old origin.
    const auto neutral = freeze_spin(source, angle, false);
    renderer::Vec3 anchor{};
    for (int axis = 0; axis < 3; ++axis) {
        for (int col = 0; col < 3; ++col) {
            anchor[axis] += neutral[col * 4 + axis] * p[col];
        }
    }
    anchor = pivot_anchor(anchor, right);
    for (int axis = 0; axis < 3; ++axis) {
        source[12 + axis] += anchor[axis];
        for (int col = 0; col < 3; ++col) {
            source[12 + axis] -= source[col * 4 + axis] * p[col];
        }
    }
    return source;
}

// Scale around the centre of the 128x128 item monitor, not the 512px backing texture.
inline renderer::Mat4 projection(const renderer::Mat4& source, const Adjustment& a) {
    constexpr float half = 256;
    auto transform = renderer::identity();
    transform[0] = transform[5] = a.size;
    transform[12] = (1 - a.size) * (64 / half - 1) + a.x / half;
    transform[13] = (1 - a.size) * (1 - 64 / half) - a.y / half;
    return renderer::multiply(transform, source);
}
}  // namespace openrac::scene::vendor_preview
