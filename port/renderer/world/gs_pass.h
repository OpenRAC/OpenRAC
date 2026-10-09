// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/gs_state.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The pixel-pipeline state of every world draw: the GS's TEST_1 (alpha and
// depth test), the Z write and the ALPHA_1 blend, turned into GL state. The
// world renderers name each draw with a GsPass and set it with apply().
//
// The world passes all run with ALPHA_1 = (Cs - Cd) * As >> 7 + Cd and a
// TEST_1 of the form "alpha test GEQUAL AREF, AFAIL RGB_ONLY, Z test
// GEQUAL": a pixel with As >= AREF writes its blended colour and Z, a pixel
// with As < AREF still writes its blended colour but not Z. As is the
// fragment's alpha after the texture function (MODULATE: At * Af >> 7, both
// in the chip's units, 0x80 = 1.0), and the same As is the blend factor.
// A batch whose As can differ from 0x80 is therefore drawn twice
// (RENDERER.md section 5, OpenGOAL's double draw): OpaqueTested keeps the
// fragments that pass, blends them and writes Z; ColorOnlyLowAlpha keeps the
// ones that fail and blends them without Z. A batch whose As is 0x80
// everywhere is one Opaque draw. draws() picks which halves a batch needs.
//
// TEST_1 words, as ReRAC read them from the NTSC-U executable (addresses are
// NTSC-U, SCUS_971.99):
//
//   tfrag, tie (main list), moby, billboard pass 1   0x5360b  AREF 0x60
//       (ResetGsRegisters' table NTSC-U 0x13cfc0; mobys: DrawMobysSetup and
//       the per-moby block NTSC-U 0x1dedc0)
//   tie, second list (not modelled: every tie is drawn as the main list)
//                                                    0x5340b  AREF 0x40
//   shrub mesh, opaque list                          0x5320b  AREF 0x20
//   shrub mesh, fading list                          0x530cb  AREF 0x0c
//   shrub billboard pass 2: ATST NEVER, RGB_ONLY     0x53001  colour only
//   fading moby (distance fade below 0x80)           0x5308b  AREF 0x08
//   sky gouraud / textured shell                     0x30000 / 0x3180b: ZTST ALWAYS
//   HUD and 2D                                       0x5380b
//
// The world's colours are the chip's display bytes, and the frame buffer the
// renderers draw into stores those bytes (RENDERER.md section 5), so GL's
// fixed-function blend on that buffer is the chip's arithmetic up to rounding
// (GL rounds Cs * As / 128 where the chip truncates). Every blended world
// draw uses one blend function, ONE and ONE_MINUS_SRC_ALPHA, and its fragment
// shader writes the premultiplied term: (Cs * As, As) for ALPHA_1 0x44 and
// (Cs * As, 0) for the additive 0x48 (display_blend.glsl). As above 0x80 is
// clamped to 1 in the 0x44 destination factor.

#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "renderer/texture.h"

namespace openrac::renderer::world {

// The GS TEST_1 register's fields (public GS documentation, register 0x47).
struct Test1 {
    bool ate = false;
    std::uint8_t atst = 0;  // 0 NEVER, 1 ALWAYS, 2 LESS, 3 LEQUAL, 4 EQUAL, 5 GEQUAL, 6 GREATER, 7 NOTEQUAL
    std::uint8_t aref = 0;
    std::uint8_t afail = 0;  // 0 KEEP, 1 FB_ONLY, 2 ZB_ONLY, 3 RGB_ONLY
    bool zte = false;
    std::uint8_t ztst = 0;  // 0 NEVER, 1 ALWAYS, 2 GEQUAL, 3 GREATER

    static constexpr Test1 decode(std::uint64_t v) {
        return Test1{
            (v & 1) != 0,
            static_cast<std::uint8_t>((v >> 1) & 7),
            static_cast<std::uint8_t>((v >> 4) & 0xff),
            static_cast<std::uint8_t>((v >> 12) & 3),
            ((v >> 16) & 1) != 0,
            static_cast<std::uint8_t>((v >> 17) & 3),
        };
    }

    // True for "As >= AREF writes Z, else colour only" (ATE, GEQUAL,
    // RGB_ONLY, ZTE, ZTST GEQUAL): the form of every world pass.
    constexpr bool is_z_write_split() const {
        return ate && atst == 5 && afail == 3 && zte && ztst == 2;
    }
};

namespace test1 {
inline constexpr std::uint64_t kWorld = 0x5360b;
inline constexpr std::uint64_t kTieSecondList = 0x5340b;
inline constexpr std::uint64_t kShrubOpaqueList = 0x5320b;
inline constexpr std::uint64_t kShrubFadingList = 0x530cb;
inline constexpr std::uint64_t kBillboardFading = 0x53001;
inline constexpr std::uint64_t kMobyFading = 0x5360b + (8 << 4) - 0x600;  // 0x5308b
inline constexpr std::uint64_t kSkyGouraud = 0x30000;
inline constexpr std::uint64_t kSkyTextured = 0x3180b;
inline constexpr std::uint64_t kHud = 0x5380b;
}  // namespace test1

inline constexpr std::uint8_t kArefWorld = Test1::decode(test1::kWorld).aref;
inline constexpr std::uint8_t kArefShrubOpaque = Test1::decode(test1::kShrubOpaqueList).aref;
inline constexpr std::uint8_t kArefShrubFading = Test1::decode(test1::kShrubFadingList).aref;
inline constexpr std::uint8_t kArefMobyFading = Test1::decode(test1::kMobyFading).aref;

// The range of the chip's alpha values (0..0xff, 0x80 = 1.0) a texture or a
// set of vertex colours can produce.
struct AlphaRange {
    std::uint8_t min = 0x80;
    std::uint8_t max = 0x80;

    static constexpr AlphaRange opaque() { return {0x80, 0x80}; }

    // The range of `values`; opaque() when empty.
    static AlphaRange of(std::span<const std::uint8_t> values);
    static AlphaRange of(std::initializer_list<std::uint8_t> values);
    // The alpha bytes of an image (every fourth byte).
    static AlphaRange of_image(const Rgba8Image& image);

    AlphaRange united(AlphaRange other) const;

    bool is_opaque() const { return min == 0x80 && max == 0x80; }

    bool operator==(const AlphaRange&) const = default;
};

// One draw's GS state.
enum class GsPass : std::uint8_t {
    Opaque,             // As = 0x80 everywhere: writes Cs and Z
    OpaqueTested,       // the fragments with As >= AREF: blended, Z written
    ColorOnlyLowAlpha,  // the fragments with As < AREF: blended, no Z
    BlendNoZ,           // blended, no alpha test, no Z (billboard pass 2, water)
    Additive,           // Cs * As + Cd (ALPHA_1 0x48), no Z
    Subtractive,        // Cd - Cs * As (ALPHA_1 0x62 with As = FIX), no Z
    SkyDome,            // blended, Z written, ZTST ALWAYS
    SkyTextured,        // blended, no Z, ZTST ALWAYS
    Hud,                // blended, no Z, no depth test
};

// What a fragment shader discards for its half of the alpha-test split.
enum class AlphaDiscard : std::uint8_t {
    None,
    Below,      // As < AREF: the passing half
    AtOrAbove,  // As >= AREF: the failing half
};

enum class BlendEquation : std::uint8_t {
    None,
    Mix,       // ALPHA_1 0x44, premultiplied: ONE, ONE_MINUS_SRC_ALPHA
    Add,       // ALPHA_1 0x48, premultiplied with alpha 0: the same function
    Subtract,  // reverse subtract, ONE, ONE
};

struct GsState {
    BlendEquation blend = BlendEquation::None;
    bool depth_write = true;
    bool depth_test = true;  // GEQUAL when on, ALWAYS when off
    AlphaDiscard discard = AlphaDiscard::None;
};

GsState state_of(GsPass pass);

// One draw: the pass and the AREF its alpha-test half uses.
struct GsDraw {
    GsPass pass = GsPass::Opaque;
    std::uint8_t aref = 0;

    bool operator==(const GsDraw&) const = default;
};

// The draws that reproduce a batch under a Z-writing alpha test with `aref`,
// given the texel alpha range of its texture (every mip level) and the range
// of its vertex alphas (Af). Bilinear filtering only interpolates between
// texels, so As = At * Af >> 7 lies within [tmin * vmin >> 7, tmax * vmax >> 7];
// a half of the split no fragment can reach is not drawn.
std::vector<GsDraw> draws(std::uint8_t aref, AlphaRange texel, AlphaRange vertex);

// The shader-side values of a draw's alpha-test half (u_alpha_test: 0 none,
// 1 keep As >= AREF, 2 keep As < AREF) and its output (u_output: 0 the mix
// term, 1 the additive term).
int alpha_test_mode(const GsDraw& draw);
int output_mode(const GsDraw& draw);

// Set GL's blend, depth and colour-write state for `draw`.
void apply(const GsDraw& draw);

// Back to the state the Renderer leaves between buckets: no blend, depth
// test GEQUAL with writes, every colour channel written, no stencil.
void reset_state();

}  // namespace openrac::renderer::world
