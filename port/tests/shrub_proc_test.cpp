// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "game/common/profile.h"
#include "game/rac1/game_state.h"
#include "game/rac1/shrub_proc.h"
#include "openrac/ps2float.h"
#include "tests/check.h"

namespace {
using namespace openrac;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;
using s32 = std::int32_t;

u32 bits(float f) {
    u32 u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

float fl(u32 u) {
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

float key_max(float a, float b) {
    const auto key = [](float f) {
        const u32 u = bits(f);
        return (u >> 31) ? -static_cast<int>(u & 0x7FFFFFFFu) : static_cast<int>(u);
    };
    return key(b) > key(a) ? b : a;
}

// The NTSC-U boot program's blocks, as ShrubProc reads them there.
const game::rac1::GameAddresses& kA = game::rac1::kRac1Ntsc;
constexpr u32 kBlock = 0x186F40;  // camera block (camera_position - 0x140)
constexpr u32 kClasses = 0x1D7F30;
constexpr u32 kClassData = 0x1000000, kInstances = 0x1100000, kMatrices = 0x1200000;

struct Ram {
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(32u * 1024 * 1024);

    template <class T>
    void put(u32 at, T v) {
        std::memcpy(bytes.data() + at, &v, sizeof(v));
    }

    template <class T>
    T get(u32 at) const {
        T v;
        std::memcpy(&v, bytes.data() + at, sizeof(v));
        return v;
    }
};

// ShrubProc's per-instance part and its lists as hostgen's asmc translates them (0x228F0C on),
// register for register: one entry per instance, the oracle the port's replay is checked against.
struct Oracle {
    std::uint8_t mesh = 0;
    std::uint8_t billboard = 0;
    bool off_sides = false;
};

std::vector<Oracle> asmc_shrub_proc(const Ram& ram) {
    float vf19[2], vf20[2], vf21[2], vf22[3], vf23[2], vf24[3], vf25[3], vf26[3], vf27[3];
    for (int i = 0; i < 3; ++i) {
        vf24[i] = ram.get<float>(kBlock + 0x140 + 4 * i);
        vf25[i] = ram.get<float>(kBlock + 0x00 + 4 * i);
        vf26[i] = ram.get<float>(kBlock + 0x10 + 4 * i);
        vf27[i] = ram.get<float>(kBlock + 0x20 + 4 * i);
        vf22[i] = ram.get<float>(kA.view_context + 0x1E0 + 4 * i);
    }
    for (int i = 0; i < 2; ++i) {
        vf19[i] = openrac_ps2_mul(ram.get<float>(kA.view_context + 0xA0 + 4 * i), fl(0x3A800000));
        vf20[i] = ram.get<float>(kA.view_context + 0xB0 + 4 * i);
        vf21[i] = ram.get<float>(kA.view_context + 0x1D0 + 4 * i);
        vf23[i] = openrac_ps2_mul(ram.get<float>(kA.view_context + 0x1F0 + 4 * i), fl(0x3A800000));
    }
    const s32 r15 = ram.get<s32>(kA.moby_end + 0x4C4);
    const s32 classes = ram.get<s32>(kA.moby_end + 0x4AC);
    std::vector<Oracle> out(static_cast<std::size_t>(ram.get<s32>(kA.moby_end + 0x4B0)));
    int billboard_list = 0, fading_list = 0, clipped_list = 0;
    for (s32 c = 0; c < classes; ++c) {
        const u32 cls = ram.get<u32>(kClasses + 4 * static_cast<u32>(c));
        if (ram.get<std::uint16_t>(cls + 0x14) & 1) {
            continue;
        }
        const u32 r14 = ram.get<u32>(cls + 0x1C);
        int opaque_list = 0;
        for (u32 n = 0; n < ram.get<std::uint16_t>(cls + 0x16); ++n) {
            const u32 r20 = ram.get<u32>(cls + 0x18) + 0x20 * n;
            float vf1[4], vf2[3], vf3[2], vf4[2], vf5[2], vf6[2], vf7[2], vf8[2], vf9[2];
            for (int i = 0; i < 4; ++i) {
                vf1[i] = ram.get<float>(r20 + 4 * static_cast<u32>(i));
            }
            u64 r2 = static_cast<u64>(std::min(ram.get<s32>(r20 + 16), r15)) << 32;
            for (int i = 0; i < 3; ++i) {
                vf2[i] = openrac_ps2_sub(vf1[i], vf24[i]);
            }
            vf3[0] = openrac_ps2_add(0.0f, vf1[3]);
            const float vf4y = fl(static_cast<u32>(r2 >> 32));
            if (r2 == 0) {
                continue;
            }
            vf3[1] = openrac_ps2_sub(0.0f, vf1[3]);
            float acc[3];
            for (int i = 0; i < 3; ++i) {
                acc[i] = openrac_ps2_mul(vf25[i], vf2[0]);
            }
            for (int i = 0; i < 3; ++i) {
                acc[i] = openrac_ps2_add(acc[i], openrac_ps2_mul(vf26[i], vf2[1]));
            }
            for (int i = 0; i < 3; ++i) {
                vf2[i] = openrac_ps2_add(acc[i], openrac_ps2_mul(vf27[i], vf2[2]));
            }
            vf19[1] = openrac_ps2_sub(vf4y, vf1[3]);
            for (int i = 0; i < 2; ++i) {
                vf8[i] = openrac_ps2_mul(vf22[i], vf1[3]);
            }
            for (int i = 0; i < 2; ++i) {
                vf1[i] = openrac_ps2_add(vf3[i], vf2[2]);
                vf5[i] = openrac_ps2_mul(vf20[i], vf2[2]);
                vf2[i] = fl(bits(vf2[i]) & 0x7FFFFFFFu);
                vf7[i] = openrac_ps2_mul(vf8[i], vf22[2]);
                vf3[i] = openrac_ps2_sub(vf19[i], vf1[i]);
                vf9[i] = openrac_ps2_mul(vf5[i], vf21[i]);
                vf6[i] = openrac_ps2_sub(vf2[i], vf8[i]);
                vf8[i] = openrac_ps2_add(vf2[i], vf7[i]);
                vf4[i] = openrac_ps2_sub(vf1[i], vf23[i]);
            }
            const float vf1z = key_max(0.0f, vf2[2]);
            u64 r1 = bits(vf3[0]) | (u64{bits(vf3[1])} << 32);
            for (int i = 0; i < 2; ++i) {
                vf7[i] = openrac_ps2_sub(vf5[i], vf6[i]);
                vf8[i] = openrac_ps2_sub(vf9[i], vf8[i]);
            }
            r2 = r1 << 32;
            if (static_cast<s64>(r1) < 0 || static_cast<s64>(r2) >= 0) {
                continue;
            }
            vf1[0] = vf1[1];
            vf1[1] = vf1z;
            vf3[0] = fl(static_cast<u32>(openrac_ps2_ftoi(openrac_ps2_mul(vf3[0], 4096.0f))));
            vf3[1] = fl(static_cast<u32>(openrac_ps2_ftoi(openrac_ps2_mul(vf3[1], 4096.0f))));
            const u32 r10 = ram.get<std::uint16_t>(r20 + 24);
            r2 = bits(vf7[0]) | (u64{bits(vf7[1])} << 32);
            const u64 r5 = 0x80000000u;
            u64 r3 = bits(vf8[0]) | (u64{bits(vf8[1])} << 32);
            const u64 r4 = (bits(vf4[0]) | (u64{bits(vf4[1])} << 32)) ^ r5;
            auto& o = out[r10];
            o.off_sides = static_cast<s64>(r2) < 0 || static_cast<s64>(r2 << 32) < 0;
            if (o.off_sides) {
                continue;
            }
            r3 = r3 | r4;
            if (static_cast<s64>(r3) < 0 || static_cast<s64>(r3 << 32) < 0) {
                if (clipped_list < 128) {
                    ++clipped_list;
                    o.mesh = 0x80;
                }
                continue;
            }
            vf5[1] = fl(static_cast<u32>(openrac_ps2_ftoi(openrac_ps2_mul(vf1[1], 4096.0f))));
            r3 = bits(vf3[0]) | (u64{bits(vf3[1])} << 32);
            bool mesh = true;
            if (r14 != 0) {
                u64 r1b = ram.get<std::uint8_t>(r20 + 23);
                r3 = u64{0x8000} << 32;
                u64 r6 = 0x8000;
                bool alone = true;
                if (r1b != 0) {
                    const u64 v0 = bits(vf5[0]) | (u64{bits(vf5[1])} << 32);
                    r1b <<= 44;
                    r3 = r3 + r1b;
                    r6 = v0 - r1b;
                    r3 = r3 - v0;
                    if (static_cast<s64>(r6) < 0) {
                        alone = false;
                    } else {
                        r6 = static_cast<u64>(static_cast<s64>(r6) >> 32);
                        r6 = static_cast<u32>(std::min<s32>(static_cast<s32>(r6), 0x8000));
                    }
                }
                if (alone) {
                    r6 = static_cast<u32>(r6) >> 8;
                    if (r6 == 0x80) {
                        if (billboard_list == 768) {
                            continue;
                        }
                        ++billboard_list;
                        u64 r3b = bits(vf3[0]) | (u64{bits(vf3[1])} << 32);
                        r3b = static_cast<u64>(static_cast<s64>(r3b) >> 33);
                        r3b = static_cast<u32>(std::min<s32>(static_cast<s32>(r3b), 0x8000));
                        o.billboard = static_cast<std::uint8_t>(static_cast<u32>(r3b) >> 8);
                        continue;
                    }
                    if (billboard_list == 768) {
                        continue;
                    }
                    ++billboard_list;
                    o.billboard = static_cast<std::uint8_t>(r6);
                }
            }
            if (mesh) {
                r3 = static_cast<u64>(static_cast<s64>(r3) >> 32);
                const s32 m = std::min<s32>(static_cast<s32>(r3), 0x8000);
                if (m == 0x8000) {
                    if (opaque_list < 256) {
                        ++opaque_list;
                        o.mesh = 0x80;
                    }
                } else if (fading_list < 256) {
                    ++fading_list;
                    o.mesh = static_cast<std::uint8_t>(static_cast<u32>(m) >> 8);
                }
            }
        }
    }
    return out;
}

struct Scene {
    int classes;
    int per_class;
    bool narrow;
};

// Fills `ram` with a camera, a view block and classes of instances around the eye.
void build(Ram& ram, std::mt19937& rng, const Scene& scene) {
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> spot(-300.0f, 300.0f);
    std::fill(ram.bytes.begin(), ram.bytes.end(), 0);
    // A rotation about Z then a tilt: rows of the view matrix (x right, y up, z depth lanes).
    const float yaw = unit(rng) * 3.14159f, pitch = unit(rng) * 0.6f;
    const float fwd[3] =
        {std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch), std::sin(pitch)};
    const float right[3] = {std::sin(yaw), -std::cos(yaw), 0};
    const float up[3] =
        {right[1] * fwd[2] - right[2] * fwd[1],
         right[2] * fwd[0] - right[0] * fwd[2],
         right[0] * fwd[1] - right[1] * fwd[0]};
    for (int i = 0; i < 3; ++i) {
        ram.put<float>(kBlock + 0x00 + 0x10 * static_cast<u32>(i) + 0, right[i]);
        ram.put<float>(kBlock + 0x00 + 0x10 * static_cast<u32>(i) + 4, up[i]);
        ram.put<float>(kBlock + 0x00 + 0x10 * static_cast<u32>(i) + 8, fwd[i]);
    }
    const float eye[3] = {spot(rng), spot(rng), spot(rng) * 0.1f};
    for (int i = 0; i < 3; ++i) {
        ram.put<float>(kBlock + 0x140 + 4 * static_cast<u32>(i), eye[i]);
    }
    const u32 vc = kA.view_context;
    // Level 0's, as the game leaves them (the sides wide open but in the 4:3 scenes).
    ram.put<float>(vc + 0xA0, 32.0f);
    ram.put<float>(vc + 0xB0, scene.narrow ? 0.63f : 1000.0f);
    ram.put<float>(vc + 0xB4, scene.narrow ? 0.48825f : 1000.0f);
    ram.put<float>(vc + 0x1D0, 4.0f);
    ram.put<float>(vc + 0x1D4, 4.0f);
    ram.put<float>(vc + 0x1E0, 1.1819053f);
    ram.put<float>(vc + 0x1E4, 1.1128291f);
    ram.put<float>(vc + 0x1E8, 2.1327794f);
    ram.put<float>(vc + 0x1F0, 745472.0f);
    ram.put<float>(vc + 0x1F4, 32.0f);
    ram.put<float>(kA.moby_end + 0x4C4, (rng() % 5) ? 500.0f : 120.0f);
    ram.put<s32>(kA.moby_end + 0x4AC, scene.classes);
    ram.put<u32>(kA.moby_end + 0x4BC, kMatrices);
    u32 index = 0;
    for (int c = 0; c < scene.classes; ++c) {
        const u32 cls = kClassData + 0x100 * static_cast<u32>(c);
        ram.put<u32>(kClasses + 4 * static_cast<u32>(c), cls);
        ram.put<std::uint16_t>(cls + 0x14, rng() % 12 == 0 ? 1 : 0);
        const auto count =
            static_cast<std::uint16_t>(1 + rng() % static_cast<u32>(scene.per_class));
        ram.put<std::uint16_t>(cls + 0x16, count);
        ram.put<u32>(cls + 0x18, kInstances + 0x20 * index);
        const bool billboard = rng() % 2 == 0;
        const int fade = rng() % 6 == 0 ? 0 : static_cast<int>(8 + rng() % 120);
        ram.put<u32>(cls + 0x1C, billboard ? cls + 0x80 : 0);
        ram.put<float>(cls + 0x80, static_cast<float>(fade));
        for (u32 n = 0; n < count; ++n, ++index) {
            const u32 inst = kInstances + 0x20 * index;
            const float depth = std::uniform_real_distribution<float>(-20.0f, 520.0f)(rng);
            const float across = unit(rng) * depth * 0.9f;
            const float high = unit(rng) * depth * 0.6f;
            for (int i = 0; i < 3; ++i) {
                ram.put<float>(
                    inst + 4 * static_cast<u32>(i),
                    eye[i] + fwd[i] * depth + right[i] * across + up[i] * high
                );
            }
            ram.put<float>(inst + 12, std::fabs(unit(rng)) * 4.0f);
            float d =
                rng() % 3 == 0 ? 32.0f : std::uniform_real_distribution<float>(16.0f, 512.0f)(rng);
            if (billboard) {
                d = std::max(d, static_cast<float>(fade) + 24.0f);
            }
            ram.put<float>(inst + 16, d);
            ram.put<std::uint8_t>(inst + 0x17, billboard ? static_cast<std::uint8_t>(fade) : 0);
            ram.put<std::uint16_t>(inst + 0x18, static_cast<std::uint16_t>(index));
        }
    }
    ram.put<s32>(kA.moby_end + 0x4B0, static_cast<s32>(index));
}

// The replay agrees with the translation on random scenes: wide sides with lists that overflow,
// and the 4:3 sides, where the port also draws what the game leaves out at the sides.
void matches_the_asm() {
    std::mt19937 rng(465);
    int compared = 0, fading = 0, billboards = 0, crossing = 0, dropped = 0;
    for (int round = 0; round < 400; ++round) {
        const Scene scene =
            round % 4 == 3 ? Scene{4, 60, true}
                           : Scene{1 + static_cast<int>(rng() % 6), round % 2 ? 900 : 120, false};
        Ram ram;
        build(ram, rng, scene);
        const auto expect = asmc_shrub_proc(ram);
        const auto got =
            game::rac1::shrub_proc(ram.bytes, kA, game::rac1::shrub_class_table(true, -1));
        CHECK(expect.size() == got.size());
        for (std::size_t i = 0; i < std::min(expect.size(), got.size()); ++i) {
            if (expect[i].off_sides) {
                continue;
            }
            CHECK(expect[i].mesh == got[i].mesh);
            CHECK(expect[i].billboard == got[i].billboard);
            ++compared;
            fading += expect[i].mesh > 0 && expect[i].mesh < 0x80;
            billboards += expect[i].billboard != 0;
            crossing += expect[i].billboard != 0 && expect[i].mesh != 0;
            dropped += expect[i].mesh == 0 && expect[i].billboard == 0;
        }
    }
    CHECK(
        compared > 100000 && fading > 1000 && billboards > 1000 && crossing > 100 && dropped > 1000
    );
}

// The sprite turns about Z towards the eye: width across the direction to it, height up, from its
// origin plus the bottom offset.
void billboard_corners() {
    Ram ram;
    std::mt19937 rng(1);
    build(ram, rng, {1, 1, false});
    const u32 cls = kClassData, inst = kInstances, block = kMatrices;
    for (int i = 0; i < 3; ++i) {
        ram.put<float>(kBlock + 0x140 + 4 * static_cast<u32>(i), 0.0f);
        ram.put<float>(kBlock + 0x00 + 0x10 * static_cast<u32>(i) + 8, i == 0 ? 1.0f : 0.0f);
    }
    ram.put<float>(kBlock + 0x00 + 4, 0.0f);
    ram.put<float>(cls + 0x80, 8.0f);
    ram.put<u32>(cls + 0x1C, cls + 0x80);
    ram.put<float>(cls + 0x84, 2048.0f);  // width
    ram.put<float>(cls + 0x88, 4096.0f);  // height
    ram.put<float>(cls + 0x8C, 512.0f);   // bottom offset
    const float sphere[4] = {100.0f, 0.0f, 0.0f, 1.0f};
    std::memcpy(ram.bytes.data() + inst, sphere, sizeof(sphere));
    ram.put<float>(inst + 16, 200.0f);
    ram.put<std::uint8_t>(inst + 0x17, 8);
    ram.put<u32>(block + 0x2C, 0x10001000u);  // column lengths 1.0
    ram.put<float>(block + 0x30, 100.0f);
    ram.put<float>(block + 0x34, 0.0f);
    ram.put<float>(block + 0x38, 0.0f);
    ram.put<float>(block + 0x3C, 1.0f);  // class scale
    const auto got = game::rac1::shrub_proc(ram.bytes, kA, game::rac1::shrub_class_table(true, -1));
    CHECK(got.size() == 1);
    if (got.size() == 1) {
        CHECK(got[0].billboard == 0x80 && got[0].mesh == 0);
        const auto corner = [&](float y, float z) {
            return renderer::transform_point(got[0].billboard_matrix, {0.0f, y, z});
        };
        const auto left_top = corner(-0.5f, 1.0f);
        CHECK(std::fabs(left_top[0] - 100.0f) < 1e-4f);
        CHECK(std::fabs(left_top[1] - 1.0f) < 1e-4f);  // -0.5 * 2 * (d.y, -d.x) = (0, 1)
        CHECK(std::fabs(left_top[2] - 4.5f) < 1e-4f);  // 4 high, 0.5 up
    }
}
}  // namespace

int main() {
    matches_the_asm();
    billboard_corners();
    return openrac::test::result();
}
