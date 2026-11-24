// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// EffectRenderer's CPU decisions for its batches (effect_state.h).

#include "renderer/effects/effect_state.h"

#include "renderer/gl/gl.h"

namespace openrac::renderer {
namespace {

using namespace gl;

// The colour channels without alpha.
constexpr std::uint8_t kWriteRgb = kWriteRed | kWriteGreen | kWriteBlue;

// Appends `pass` to `passes` (at most two are ever planned).
void add(EffectPasses& passes, const EffectPass& pass) {
    passes.pass[passes.count++] = pass;
}

// The GL comparison for `test` under reversed depth (bigger is nearer, cleared to 0).
unsigned depth_function(const DepthTest& test) {
    // A depth test that is off still writes depth in GL only with the test on: ALWAYS stands in.
    if (!test.enabled) {
        return GL_ALWAYS;
    }

    // Each comparison, its GL name.
    switch (test.compare) {
        case DepthCompare::Always:
            return GL_ALWAYS;
        case DepthCompare::Never:
            return GL_NEVER;
        case DepthCompare::GreaterEqual:
            return GL_GEQUAL;
        case DepthCompare::Greater:
            return GL_GREATER;
    }

    // An out-of-range comparison draws as the effects always tested: as near or nearer.
    return GL_GEQUAL;
}

}  // namespace

EffectPasses effect_passes(
    const AlphaTest& test, bool depth_write, std::uint8_t colour_writes, bool tested_shader
) {
    const bool fail_colour = test.fail == FailWrites::Colour || test.fail == FailWrites::Rgb;
    const auto fail_alpha =
        static_cast<std::uint8_t>(test.fail == FailWrites::Colour ? kWriteAlpha : 0);
    const auto fail_writes =
        static_cast<std::uint8_t>(((fail_colour ? kWriteRgb : 0) | fail_alpha) & colour_writes);
    const bool fail_depth = test.fail == FailWrites::Depth && depth_write;
    EffectPasses passes;

    // NEVER fails every pixel: no shader test, only what a failing pixel still writes.
    if (test.compare == Compare::Never) {
        // A failing pixel that keeps nothing leaves no draw at all.
        if (test.fail != FailWrites::Nothing) {
            add(passes, {0, false, fail_writes, fail_depth});
        }

        return passes;
    }

    // No test, or no shader that keeps it: every covered pixel once, with every write.
    if (test.compare == Compare::Always || !tested_shader) {
        add(passes, {0, false, colour_writes, depth_write});
        return passes;
    }

    const auto compare = static_cast<std::uint8_t>(test.compare);
    add(passes, {compare, false, colour_writes, depth_write});

    // The pixels the test rejects are drawn again with what their fail mode keeps (OpenGOAL's
    // double draw).
    if (test.fail != FailWrites::Nothing) {
        add(passes, {compare, true, fail_writes, fail_depth});
    }

    return passes;
}

bool source_over(const Blend& blend) {
    return blend.source_k == 1 && blend.source_m == 0 && blend.dest_k == -1 && blend.dest_m == 1;
}

EffectGlState effect_gl_state(const EffectState& state) {
    EffectGlState out;
    out.blend = state.blend.enabled;
    out.inscatter = state.blend.enabled && source_over(state.blend);
    out.tcc = state.texture_alpha ? 1 : 0;
    out.tfx = static_cast<int>(state.combine);
    out.mag_filter = state.linear_mag ? GL_LINEAR : GL_NEAREST;
    out.min_filter = state.linear_min ? GL_LINEAR : GL_NEAREST;
    out.repeat_s = state.repeat_s;
    out.repeat_t = state.repeat_t;
    out.depth_function = depth_function(state.depth);
    return out;
}

}  // namespace openrac::renderer
