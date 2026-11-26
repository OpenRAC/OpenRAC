// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The draw state of an effect quad whose host C gave a native material (openrac/material.h,
// EffectQuad::native), in the native values of draw_state.h, and what EffectRenderer decides from
// it before any GL call: the draws its alpha test needs and the GL values of its batch. CPU only.
// The quads that still carry the chip's words keep their own reading (effects.h); the alpha
// test's draws are this one function for both.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "renderer/direct/draw_state.h"

namespace openrac::renderer {

// A native effect quad's state: its blend (alpha written as the source's, draw_state.h), alpha
// test, depth test against the world (reversed depth) and depth write, the channels it writes,
// its texture function and whether the texel's alpha counts, and its image's filter and wrap.
struct EffectState {
    Blend blend = Blend::of(BlendMode::Mix);
    AlphaTest alpha_test{};
    DepthTest depth{true, DepthCompare::GreaterEqual};
    bool depth_write = false;
    std::uint8_t colour_writes = kWriteColour;
    TextureCombine combine = TextureCombine::Modulate;
    bool texture_alpha = true;
    bool linear_mag = false;
    bool linear_min = false;
    bool repeat_s = true;
    bool repeat_t = true;

    bool operator==(const EffectState&) const = default;
};

// One draw of a batch under an alpha test: the shader's test (0 none, else a Compare's value),
// whether it keeps the pixels that fail it rather than those that pass, what it writes (kWrite*
// bits) and whether it writes depth.
struct EffectPass {
    std::uint8_t alpha_test = 0;
    bool keep_failing = false;
    std::uint8_t colour_writes = kWriteColour;
    bool depth_write = false;

    bool operator==(const EffectPass&) const = default;
};

// The draws of one batch, in order (none, one or two).
struct EffectPasses {
    std::array<EffectPass, 2> pass{};
    std::size_t count = 0;
};

// The draws a batch with alpha test `test` needs, as the chip writes it: the pixels that pass with
// every write (depth with `depth_write`), then those that fail with what `test.fail` still writes.
// `colour_writes` limits every draw's channels. Without `tested_shader` (no shader that keeps the
// test) every covered pixel is drawn once, as the effects drew before; NEVER needs no shader.
EffectPasses effect_passes(
    const AlphaTest& test, bool depth_write, std::uint8_t colour_writes, bool tested_shader
);

// Whether `blend` is drawn over by a weight, (Cs - Cd) w + Cd, whatever the weight: the blends
// that add the medium's in-scattering (EffectRenderer, the effect shader's atmosphere_inscatter).
bool source_over(const Blend& blend);

// The GL values EffectRenderer sets for a batch of native quads in `state`: blending on (with
// translate_blend(state.blend)), in-scattering, the shader's tcc and tfx, the image's filters
// (GL_LINEAR or GL_NEAREST) and wrap, and the depth comparison (reversed depth).
struct EffectGlState {
    bool blend = false;
    bool inscatter = false;
    int tcc = 0;
    int tfx = 0;
    unsigned mag_filter = 0;
    unsigned min_filter = 0;
    bool repeat_s = true;
    bool repeat_t = true;
    unsigned depth_function = 0;
};

EffectGlState effect_gl_state(const EffectState& state);

}  // namespace openrac::renderer
