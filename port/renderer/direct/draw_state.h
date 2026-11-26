// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The GPU state of one batched 2D draw, in native values (docs/port/FRAME_RECORDER.md, section 5):
// blend, alpha test, depth, texture combine, filter, wrap, write mask, scissor and targets. No GS
// register word and no field of one: the frame recorder's commands make it directly, and the GIF
// interpreter translates the game's registers into it once, at its edge (blend_from_gs and the
// rest, which go with the interpreter, P8). It knows no GL; DirectRenderer applies it.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace openrac::renderer {

// What weighs a blend: the source's alpha, the destination's, or a constant.
enum class BlendWeight : std::uint8_t {
    SourceAlpha,
    DestinationAlpha,
    Constant
};

// The fixed set of blends the frame recorder's commands name (openrac/frame.h OPENRAC_BLEND_*).
enum class BlendMode : std::uint8_t {
    Opaque = 0,    // the source as it is
    Mix = 1,       // Cs w + Cd (1 - w): drawn over, by the source's alpha
    Add = 2,       // Cs w + Cd
    Subtract = 3,  // Cd - Cs w
    Multiply = 4,  // Cd (1 - w): the destination darkened by the source's alpha
};

// A blend as linear coefficients: colour = (source_k w + source_m) Cs + (dest_k w + dest_m) Cd,
// each k in {-1, 0, 1} and each m in {0, 1}, w the weight. `constant` is the weight when it is
// Constant: 1.0 full, up to about 2 (the games' constants reach 0xFF / 0x80). Colours are in the
// 2D path's units (0x80 = 1.0); alpha is written as the source's, never blended.
struct Blend {
    bool enabled = false;
    std::int8_t source_k = 1;
    std::uint8_t source_m = 0;
    std::int8_t dest_k = -1;
    std::uint8_t dest_m = 1;
    BlendWeight weight = BlendWeight::SourceAlpha;
    float constant = 0.0f;

    // One of the named modes, weighed by the source's alpha.
    static Blend of(BlendMode mode) {
        Blend b;
        switch (mode) {
            case BlendMode::Opaque:
                return b;
            case BlendMode::Mix:
                b.enabled = true;
                return b;
            case BlendMode::Add:
                b.enabled = true;
                b.dest_k = 0;
                return b;
            case BlendMode::Subtract:
                b.enabled = true;
                b.source_k = -1;
                b.dest_k = 0;
                return b;
            case BlendMode::Multiply:
                b.enabled = true;
                b.source_k = 0;
                return b;
        }
        return b;
    }

    bool operator==(const Blend&) const = default;
};

// A comparison, in the values the 2D shader reads (direct.frag alpha_test).
enum class Compare : std::uint8_t {
    Never = 0,
    Always = 1,
    Less = 2,
    LessEqual = 3,
    Equal = 4,
    GreaterEqual = 5,
    Greater = 6,
    NotEqual = 7,
};

// What a pixel that fails the alpha test still writes.
enum class FailWrites : std::uint8_t {
    Nothing = 0,
    Colour = 1,  // colour and alpha, no depth
    Depth = 2,   // depth only
    Rgb = 3,     // colour without alpha
};

struct AlphaTest {
    Compare compare = Compare::Always;
    std::uint8_t reference = 0;  // in the colour units, 0x80 = 1.0
    FailWrites fail = FailWrites::Nothing;

    bool operator==(const AlphaTest&) const = default;
};

// Against the reversed-depth buffer (bigger is nearer, cleared to 0).
enum class DepthCompare : std::uint8_t {
    Always,
    Never,
    GreaterEqual,
    Greater,
};

struct DepthTest {
    bool enabled = false;
    DepthCompare compare = DepthCompare::Always;

    bool operator==(const DepthTest&) const = default;
};

// The largest depth of a `bits`-bit depth buffer (16, 24 or 32): the games' buffers' formats.
inline double depth_max(int bits) {
    return std::ldexp(1.0, bits) - 1.0;
}

// A depth `z` of a buffer whose largest value is `max` (bigger is nearer) where the 2D path draws
// it: the window's [0, 1] through GL's -1..1 clip range. The GIF interpreter places its vertices'
// depth and the frame recorder its depth rectangles' through this one function, so the two
// compare equal at the same value. 0 is the far value a depth clear leaves.
inline float depth_clip(double z, double max) {
    const double depth = std::clamp(z / max, 0.0, 1.0);
    return static_cast<float>(depth * 2.0 - 1.0);
}

// How a texel and the vertex colour combine (the shader's tfx).
enum class TextureCombine : std::uint8_t {
    Modulate = 0,
    Decal = 1,
    Highlight = 2,
    Highlight2 = 3,
};

// A rectangle of the target's 2D pixels, both ends inclusive (the picture's 512 x 448, or an
// off-screen target's 512 square).
struct PixelRect {
    std::uint32_t x0 = 0, x1 = 511, y0 = 0, y1 = 447;

    bool operator==(const PixelRect&) const = default;
};

// Colour channels written (ColourWrites bits).
inline constexpr std::uint8_t kWriteRed = 1, kWriteGreen = 2, kWriteBlue = 4, kWriteAlpha = 8,
                              kWriteColour = 15;

// The target namespace: off-screen panels by port id. Ids below kRecordedTargets are the GIF
// interpreter's (its GS blocks, until P8); the frame recorder's panels have ids from it up.
inline constexpr std::uint32_t kRecordedTargets = 0x10000;

}  // namespace openrac::renderer
