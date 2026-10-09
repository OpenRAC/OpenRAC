// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/src/gs_state.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/gs_pass.h"

#include <algorithm>

#include "renderer/world/gl_world.h"

namespace openrac::renderer::world {

using namespace openrac::renderer::world::gl;

AlphaRange AlphaRange::of(std::span<const std::uint8_t> values) {
    if (values.empty()) {
        return opaque();
    }
    const auto [lo, hi] = std::minmax_element(values.begin(), values.end());
    return {*lo, *hi};
}

AlphaRange AlphaRange::of(std::initializer_list<std::uint8_t> values) {
    return of(std::span<const std::uint8_t>(values.begin(), values.size()));
}

AlphaRange AlphaRange::of_image(const Rgba8Image& image) {
    if (image.pixels.size() < 4) {
        return opaque();
    }
    std::uint8_t lo = 0xff;
    std::uint8_t hi = 0;
    for (std::size_t i = 3; i < image.pixels.size(); i += 4) {
        lo = std::min(lo, image.pixels[i]);
        hi = std::max(hi, image.pixels[i]);
    }
    return {lo, hi};
}

AlphaRange AlphaRange::united(AlphaRange other) const {
    return {std::min(min, other.min), std::max(max, other.max)};
}

GsState state_of(GsPass pass) {
    switch (pass) {
        case GsPass::Opaque:
            return {BlendEquation::None, true, true, AlphaDiscard::None};
        case GsPass::OpaqueTested:
            return {BlendEquation::Mix, true, true, AlphaDiscard::Below};
        case GsPass::ColorOnlyLowAlpha:
            return {BlendEquation::Mix, false, true, AlphaDiscard::AtOrAbove};
        case GsPass::BlendNoZ:
            return {BlendEquation::Mix, false, true, AlphaDiscard::None};
        case GsPass::Additive:
            return {BlendEquation::Add, false, true, AlphaDiscard::None};
        case GsPass::Subtractive:
            return {BlendEquation::Subtract, false, true, AlphaDiscard::None};
        case GsPass::SkyDome:
            return {BlendEquation::Mix, true, false, AlphaDiscard::None};
        case GsPass::SkyTextured:
        case GsPass::Hud:
            return {BlendEquation::Mix, false, false, AlphaDiscard::None};
    }
    return {};
}

std::vector<GsDraw> draws(std::uint8_t aref, AlphaRange texel, AlphaRange vertex) {
    if (texel.is_opaque() && vertex.is_opaque()) {
        return {{GsPass::Opaque, 0}};
    }
    const unsigned lo = std::min(0xffu, (unsigned{texel.min} * vertex.min) >> 7);
    const unsigned hi = std::min(0xffu, (unsigned{texel.max} * vertex.max) >> 7);
    std::vector<GsDraw> out;
    if (hi >= aref) {
        out.push_back({GsPass::OpaqueTested, aref});
    }
    if (lo < aref) {
        out.push_back({GsPass::ColorOnlyLowAlpha, aref});
    }
    return out;
}

int alpha_test_mode(const GsDraw& draw) {
    switch (state_of(draw.pass).discard) {
        case AlphaDiscard::None:
            return 0;
        case AlphaDiscard::Below:
            return 1;
        case AlphaDiscard::AtOrAbove:
            return 2;
    }
    return 0;
}

int output_mode(const GsDraw& draw) {
    const BlendEquation b = state_of(draw.pass).blend;
    return b == BlendEquation::Add || b == BlendEquation::Subtract ? 1 : 0;
}

void apply(const GsDraw& draw) {
    const GsState s = state_of(draw.pass);
    switch (s.blend) {
        case BlendEquation::None:
            glDisable(GL_BLEND);
            break;
        case BlendEquation::Mix:
        case BlendEquation::Add:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendEquation::Subtract:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
            break;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(s.depth_test ? GL_GEQUAL : GL_ALWAYS);
    glDepthMask(s.depth_write ? GL_TRUE : GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void reset_state() {
    glDisable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
    // Sampler objects override a texture's own filtering; the renderers that
    // follow (the direct renderer) set theirs on the texture.
    for (GLuint unit = 0; unit < 8; ++unit) {
        glBindSampler(unit, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

}  // namespace openrac::renderer::world
