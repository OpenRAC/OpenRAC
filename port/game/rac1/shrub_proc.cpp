// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// shrub_proc.h: ShrubProc's per-instance tests, fades and billboards, instruction for instruction
// (NTSC-U boot 0x228BE8..0x22A324), on the VU's own arithmetic (common/ps2_float.h).

#include "game/rac1/shrub_proc.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "common/ps2_float.h"
#include "openrac/guest.h"

namespace openrac::game::rac1 {
namespace {

using namespace openrac::ps2;

template <typename T>
T at(std::span<const std::uint8_t> ram, std::uint32_t address) {
    T value{};
    address = openrac_guest_ram_address_for_size(address, static_cast<std::uint32_t>(ram.size()));
    if (address + sizeof(T) <= ram.size()) {
        std::memcpy(&value, ram.data() + address, sizeof(T));
    }
    return value;
}

// ShrubProc's class pointer table in each program: the boot program's and levels 0..18's, found
// as the lui/addiu pair after the prologue of each program's copy of the function (the copies
// differ only in their addresses). NTSC-U, then PAL.
constexpr std::uint32_t kNtscBootClasses = 0x001D7F30;
constexpr std::array<std::uint32_t, 19> kNtscLevelClasses = {
    0x001BC030, 0x001BC4B0, 0x001BC630, 0x001BC130, 0x001BC1B0, 0x001BC4B0, 0x001BC7B0,
    0x001BC0B0, 0x001BC830, 0x001BC1B0, 0x001BC4B0, 0x001BCA30, 0x001BC4B0, 0x001BC330,
    0x001BC730, 0x001BC630, 0x001BC4B0, 0x001BC930, 0x001BCA30,
};
constexpr std::uint32_t kPalBootClasses = 0x001D82C0;
constexpr std::array<std::uint32_t, 19> kPalLevelClasses = {
    0x001BC3C0, 0x001BC7C0, 0x001BC940, 0x001BC440, 0x001BC4C0, 0x001BC840, 0x001BCB40,
    0x001BC3C0, 0x001BCB40, 0x001BC4C0, 0x001BC7C0, 0x001BCD40, 0x001BC7C0, 0x001BC640,
    0x001BCAC0, 0x001BC940, 0x001BC7C0, 0x001BCC40, 0x001BCD40,
};

// ShrubProc's lists in the scratchpad, each a run of 4-byte entries: the billboards (0x2000 to
// 0x2C00), a class's opaque meshes (0x2C00 to 0x3000), the fading meshes (0x3700 to 0x3B00) and
// the meshes clipped at the guard band (0x3D00 to 0x3F00). An instance that finds its list full
// is not drawn.
constexpr int kBillboardSlots = 0xC00 / 4;
constexpr int kOpaqueSlots = 0x400 / 4;
constexpr int kFadingSlots = 0x400 / 4;
constexpr int kClippedSlots = 0x200 / 4;

// The shrub globals, at the same distance from the moby table's end pointer in every program of
// both versions (gp - 0x6834 on in the NTSC-U boot program).
constexpr std::uint32_t kClassCount = 0x4AC;  // the classes in the class table
constexpr std::uint32_t kMatrices = 0x4BC;    // 0x40 bytes an instance: columns, colour, scale
constexpr std::uint32_t kDrawCap = 0x4C4;     // the draw distance cap (500), a float's bits

// What ShrubProc loads before the classes: the eye (camera block +0x140, vf24), the view
// matrix's rows (+0x00..+0x20, vf25..vf27) and the view context's planes (vf19..vf23).
struct View {
    std::array<u32, 3> eye{};
    std::array<std::array<u32, 3>, 3> rows{};
    u32 near_plane = 0;           // vf19.x: view context +0xA0 / 1024
    std::array<u32, 2> sides{};   // vf20: +0xB0
    std::array<u32, 2> guard{};   // vf21: +0x1D0
    std::array<u32, 3> radius{};  // vf22: +0x1E0
    std::array<u32, 2> depth{};   // vf23: +0x1F0 / 1024
    s32 cap = 0;
};

View read_view(std::span<const std::uint8_t> ram, const GameAddresses& a) {
    View v;
    const std::uint32_t block = a.camera_position - 0x140;
    const std::uint32_t context = a.view_context;
    const u32 inverse = 0x3A800000;  // 1 / 1024
    for (int i = 0; i < 3; ++i) {
        v.eye[i] = at<u32>(ram, block + 0x140 + 4 * i);
        for (int j = 0; j < 3; ++j) {
            v.rows[i][j] = at<u32>(ram, block + 0x10 * i + 4 * j);
        }
        v.radius[i] = at<u32>(ram, context + 0x1E0 + 4 * i);
    }
    v.near_plane = mul(at<u32>(ram, context + 0xA0), inverse);
    for (int i = 0; i < 2; ++i) {
        v.sides[i] = at<u32>(ram, context + 0xB0 + 4 * i);
        v.guard[i] = at<u32>(ram, context + 0x1D0 + 4 * i);
        v.depth[i] = mul(at<u32>(ram, context + 0x1F0 + 4 * i), inverse);
    }
    v.cap = at<s32>(ram, a.moby_end + kDrawCap);
    return v;
}

// What the per-instance part (0x228F0C..0x229124) makes of one instance.
struct Verdict {
    bool skipped = false;     // a draw distance of 0
    bool culled = false;      // past its draw distance or wholly behind the near plane
    bool off_sides = false;   // wholly outside the 4:3 view's sides
    bool clipped = false;     // across the guard band: drawn whole by the clipping program
    bool billboard = false;   // a billboard goes to the list, at billboard_alpha
    bool mesh = true;         // the mesh is drawn (false: the billboard alone)
    u32 billboard_alpha = 0;  // GS units
    s32 mesh_fade = 0x8000;   // min(.., 0x8000); 0x8000 the opaque list, else alpha fade >> 8
};

bool sign(u32 x) {
    return (x & kSign) != 0;
}

Verdict judge(
    const u32 sphere[4], u32 draw_distance, u32 fade_distance, bool has_billboard, const View& v
) {
    Verdict out;
    // pminw with the cap on the float's bits, dsll32 into vf4.y; 0 skips the instance.
    const s32 d = std::min(static_cast<s32>(draw_distance), v.cap);
    if (d == 0) {
        out.skipped = true;
        return out;
    }
    const u32 r = sphere[3];
    std::array<u32, 3> rel{};
    for (int i = 0; i < 3; ++i) {
        rel[i] = sub(sphere[i], v.eye[i]);
    }
    // vmula / vmadda / vmadd: the view-space position, lane by lane.
    std::array<u32, 3> view{};
    for (int j = 0; j < 3; ++j) {
        view[j] =
            add(add(mul(v.rows[0][j], rel[0]), mul(v.rows[1][j], rel[1])),
                mul(v.rows[2][j], rel[2]));
    }
    const u32 z = view[2];
    const u32 near_r = add(0, r);  // vf3.x = vf0.x + r
    const u32 far_r = sub(0, r);   // vf3.y = vf0.y - r
    const u32 reach = sub(static_cast<u32>(d), r);
    const std::array<u32, 2> radius_xy = {mul(v.radius[0], r), mul(v.radius[1], r)};
    const std::array<u32, 2> side = {add(near_r, z), add(far_r, z)};             // vf1.xy
    const std::array<u32, 2> scaled = {mul(v.sides[0], z), mul(v.sides[1], z)};  // vf5
    const std::array<u32, 2> across = {view[0] & ~kSign, view[1] & ~kSign};      // vabs
    const std::array<u32, 2> padded =
        {mul(radius_xy[0], v.radius[2]), mul(radius_xy[1], v.radius[2])};
    const u32 near_test = sub(v.near_plane, side[0]);
    const u32 far_test = sub(reach, side[1]);
    const std::array<u32, 2> guard =
        {mul(scaled[0], v.guard[0]), mul(scaled[1], v.guard[1])};  // vf9
    const std::array<u32, 2> inner = {sub(across[0], radius_xy[0]), sub(across[1], radius_xy[1])};
    const std::array<u32, 2> outer = {add(across[0], padded[0]), add(across[1], padded[1])};
    const std::array<u32, 2> depth = {sub(side[0], v.depth[0]), sub(side[1], v.depth[1])};  // vf4
    const u32 nearest = max(0, z);                                                          // vf1.z
    const std::array<u32, 2> inside = {sub(scaled[0], inner[0]), sub(scaled[1], inner[1])};  // vf7
    const std::array<u32, 2> clip = {sub(guard[0], outer[0]), sub(guard[1], outer[1])};      // vf8
    // qmfc2 vf3: bltz on the far lane, bgez on the near one.
    if (sign(far_test) || !sign(near_test)) {
        out.culled = true;
        return out;
    }
    const s32 far_fade = ftoi(far_test, 12);
    out.off_sides = sign(inside[1]) || sign(inside[0]);
    // or of vf8 with vf4, its x lane's sign flipped: across the guard band or the depth guards.
    out.clipped = sign(clip[1] | depth[1]) || sign(clip[0] | (depth[0] ^ kSign));
    if (out.clipped) {
        return out;
    }
    // The mesh's fade with no billboard: far_fade, the high word of vf3 (dsra32).
    s64 mesh = far_fade;
    if (has_billboard) {
        // The cross-fade, on 64-bit words: v0 = vf5 (x the float bits of vf20.x z, y
        // ftoi12(max(0, z))), at = F << 44. Below F the mesh alone; from F + 8 the billboard alone.
        const u64 v0 = (static_cast<u64>(static_cast<u32>(ftoi(nearest, 12))) << 32) | scaled[0];
        const u64 f = static_cast<u64>(fade_distance & 0xFF) << 44;
        s64 billboard = 0x8000;
        if ((fade_distance & 0xFF) != 0) {
            const u64 into = v0 - f;
            mesh = static_cast<s64>((u64{0x8000} << 32) + f - v0) >> 32;
            if (static_cast<s64>(into) < 0) {
                out.mesh_fade = static_cast<s32>(std::min<s64>(static_cast<s32>(mesh), 0x8000));
                return out;
            }
            billboard = std::min<s32>(static_cast<s32>(static_cast<s64>(into) >> 32), 0x8000);
        }
        const u32 alpha = static_cast<u32>(billboard) >> 8;
        out.billboard = true;
        if (alpha == 0x80) {
            // The billboard alone, fading at half the mesh's rate towards the draw distance.
            out.mesh = false;
            out.billboard_alpha = static_cast<u32>(std::min<s32>(far_fade >> 1, 0x8000)) >> 8;
            return out;
        }
        out.billboard_alpha = alpha;
    }
    out.mesh_fade = std::min<s32>(static_cast<s32>(mesh), 0x8000);
    return out;
}

// The billboard sprite of instance `matrices` (its 0x40-byte block) from the class's record
// `record` (fade, width, height, bottom offset): turned about Z towards the eye (the direction
// to the instance from it, normalised in 3D, its x and y kept), corners origin
// + y W (d.y, -d.x, 0) + (z H + offset) Z for (y, z) in (+-1/2, 0 or 1), the sizes times the
// class scale and the instance's column lengths (4.12, as the loader packs them), in the sphere's
// 1/1024 units.
renderer::Mat4 billboard_matrix(
    std::span<const std::uint8_t> ram, std::uint32_t block, std::uint32_t record, const View& v
) {
    const u32 lengths = at<u32>(ram, block + 0x2C);
    const u32 across = itof12(static_cast<s32>(lengths & 0xFFFF));
    const u32 up = itof12(static_cast<s32>(lengths >> 16));
    const u32 scale = at<u32>(ram, block + 0x3C);
    const u32 width = mul(mul(at<u32>(ram, record + 4), across), scale);
    const u32 height = mul(mul(at<u32>(ram, record + 8), up), scale);
    const u32 offset = mul(mul(at<u32>(ram, record + 12), up), scale);
    std::array<u32, 3> rel{};
    for (int i = 0; i < 3; ++i) {
        rel[i] = sub(at<u32>(ram, block + 0x30 + 4 * i), v.eye[i]);
    }
    const u32 length2 =
        add(add(mul(rel[0], rel[0]), mul(rel[1], rel[1])), mul(kOne, mul(rel[2], rel[2])));
    const u32 q = rsqrt(kOne, length2);
    const float dx = to_float(mul(rel[0], q)), dy = to_float(mul(rel[1], q));
    const float w = to_float(width) / 1024.0f;
    const float h = to_float(height) / 1024.0f;
    const float o = to_float(offset) / 1024.0f;
    const float x = at<float>(ram, block + 0x30);
    const float y = at<float>(ram, block + 0x34);
    const float zz = at<float>(ram, block + 0x38);
    return {dx, dy, 0, 0, w * dy, -w * dx, 0, 0, 0, 0, h, 0, x, y, zz + o, 1};
}

}  // namespace

std::uint32_t shrub_class_table(bool ntsc, int level) {
    const auto& levels = ntsc ? kNtscLevelClasses : kPalLevelClasses;
    if (level >= 0 && level < static_cast<int>(levels.size())) {
        return levels[static_cast<std::size_t>(level)];
    }
    return ntsc ? kNtscBootClasses : kPalBootClasses;
}

std::vector<host::ShrubDraw> shrub_proc(
    std::span<const std::uint8_t> ram, const GameAddresses& a, std::uint32_t class_table
) {
    std::vector<host::ShrubDraw> out;
    if (class_table == 0) {
        return out;
    }
    const View v = read_view(ram, a);
    const s32 classes = at<s32>(ram, a.moby_end + kClassCount);
    const std::uint32_t matrices = at<std::uint32_t>(ram, a.moby_end + kMatrices);
    const s32 count = at<s32>(ram, a.moby_end + kClassCount + 4);
    if (classes <= 0 || classes > 0x100 || count <= 0 || count > 0x10000) {
        return out;
    }
    out.resize(static_cast<std::size_t>(count));
    int billboards = 0, fading = 0, clipped = 0;
    for (s32 c = 0; c < classes; ++c) {
        const auto cls = at<std::uint32_t>(ram, class_table + 4 * static_cast<std::uint32_t>(c));
        // Mode bit 0 leaves the class out.
        if ((at<std::uint16_t>(ram, cls + 0x14) & 1) != 0) {
            continue;
        }
        const std::uint16_t instances = at<std::uint16_t>(ram, cls + 0x16);
        const auto first = at<std::uint32_t>(ram, cls + 0x18);
        const auto record = at<std::uint32_t>(ram, cls + 0x1C);
        int opaque = 0;
        for (std::uint32_t n = 0; n < instances; ++n) {
            const std::uint32_t inst = first + 0x20 * n;
            u32 sphere[4];
            for (int i = 0; i < 4; ++i) {
                sphere[i] = at<u32>(ram, inst + 4 * static_cast<std::uint32_t>(i));
            }
            const auto index = at<std::uint16_t>(ram, inst + 0x18);
            if (index >= out.size()) {
                continue;
            }
            const Verdict verdict = judge(
                sphere,
                at<u32>(ram, inst + 0x10),
                at<std::uint8_t>(ram, inst + 0x17),
                record != 0,
                v
            );
            if (verdict.skipped || verdict.culled) {
                continue;
            }
            // Wholly outside the 4:3 view's sides the game culls it; the port's wider pictures
            // draw it as the game would there, and it takes no place in the game's lists.
            const bool counted = !verdict.off_sides;
            auto& draw = out[index];
            if (verdict.clipped) {
                if (!counted || clipped < kClippedSlots) {
                    clipped += counted ? 1 : 0;
                    draw.mesh = 0x80;
                }
                continue;
            }
            if (verdict.billboard) {
                if (counted && billboards == kBillboardSlots) {
                    continue;
                }
                billboards += counted ? 1 : 0;
                draw.billboard = static_cast<std::uint8_t>(verdict.billboard_alpha);
                draw.billboard_matrix = billboard_matrix(ram, matrices + 0x40u * index, record, v);
            }
            if (!verdict.mesh) {
                continue;
            }
            if (verdict.mesh_fade == 0x8000) {
                if (!counted || opaque < kOpaqueSlots) {
                    opaque += counted ? 1 : 0;
                    draw.mesh = 0x80;
                }
            } else if (!counted || fading < kFadingSlots) {
                fading += counted ? 1 : 0;
                draw.mesh = static_cast<std::uint8_t>(static_cast<u32>(verdict.mesh_fade) >> 8);
            }
        }
    }
    return out;
}

}  // namespace openrac::game::rac1
