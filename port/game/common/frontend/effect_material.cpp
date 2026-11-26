// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A native material's checks and renderer state, and the capture of its quad (effect_material.h).

#include "game/common/frontend/effect_material.h"

#include <cstdint>
#include <cstring>

namespace openrac::frontend {
namespace {

// Every flag bit openrac/material.h names.
constexpr std::uint32_t kKnownFlags = OPENRAC_MATERIAL_BLEND | OPENRAC_MATERIAL_DEPTH_WRITE
                                      | OPENRAC_MATERIAL_TEXTURE_ALPHA | OPENRAC_MATERIAL_LINEAR_MAG
                                      | OPENRAC_MATERIAL_LINEAR_MIN | OPENRAC_MATERIAL_REPEAT_S
                                      | OPENRAC_MATERIAL_REPEAT_T | OPENRAC_MATERIAL_UNTEXTURED;

// The games' unit of a weight: 0x80 is 1.0.
constexpr float kUnit = 128.0f;

// Whether a blend term is one draw_state.h's Blend holds: k of -1, 0 or 1 and m of 0 or 1.
bool term_in_range(std::int8_t k, std::uint8_t m) {
    return k >= -1 && k <= 1 && m <= 1;
}

// Whether every field of `material` is one its header names.
bool in_range(const openrac_material& material) {
    return term_in_range(material.source_k, material.source_m)
           && term_in_range(material.dest_k, material.dest_m)
           && material.weight <= OPENRAC_MATERIAL_WEIGHT_CONSTANT
           && material.alpha_test <= OPENRAC_MATERIAL_TEST_NOT_EQUAL
           && material.alpha_fail <= OPENRAC_MATERIAL_FAIL_RGB
           && material.depth_test <= OPENRAC_MATERIAL_DEPTH_NEARER
           && material.combine <= OPENRAC_MATERIAL_HIGHLIGHT2
           && (material.writes & ~OPENRAC_MATERIAL_WRITE_COLOUR) == 0
           && (material.flags & ~kKnownFlags) == 0;
}

// The blend `material` names; an unblended one keeps Blend's opaque default whatever its terms.
renderer::Blend blend_of(const openrac_material& material) {
    renderer::Blend blend;

    // Without OPENRAC_MATERIAL_BLEND the coefficients weigh nothing.
    if ((material.flags & OPENRAC_MATERIAL_BLEND) == 0) {
        return blend;
    }

    blend.enabled = true;
    blend.source_k = material.source_k;
    blend.source_m = material.source_m;
    blend.dest_k = material.dest_k;
    blend.dest_m = material.dest_m;
    blend.weight = static_cast<renderer::BlendWeight>(material.weight);

    // The constant is the weight only of a blend by a constant (draw_state.h keeps 0 otherwise).
    if (blend.weight == renderer::BlendWeight::Constant) {
        blend.constant = static_cast<float>(material.constant) / kUnit;
    }

    return blend;
}

// The depth test `test` names, against the reversed depth (bigger is nearer).
renderer::DepthTest depth_test_of(std::uint8_t test) {
    // Each depth test the header names; in_range let no other value through.
    switch (test) {
        case OPENRAC_MATERIAL_DEPTH_NEVER:
            return {true, renderer::DepthCompare::Never};
        case OPENRAC_MATERIAL_DEPTH_NEARER_OR_EQUAL:
            return {true, renderer::DepthCompare::GreaterEqual};
        case OPENRAC_MATERIAL_DEPTH_NEARER:
            return {true, renderer::DepthCompare::Greater};
        default:
            // OPENRAC_MATERIAL_DEPTH_ALWAYS: not tested.
            return {false, renderer::DepthCompare::Always};
    }
}

// The index + 1 of `image` in `images` (EffectQuad::image), added the first time.
std::uint32_t keep_image(std::vector<renderer::UiImage>& images, const renderer::UiImage& image) {
    // The same content already kept for this tick is shared by every quad that samples it.
    for (std::size_t i = 0; i < images.size(); ++i) {
        // Kept already: the quads share its index.
        if (images[i].hash == image.hash && images[i].image == image.image) {
            return static_cast<std::uint32_t>(i + 1);
        }
    }

    images.push_back(image);
    return static_cast<std::uint32_t>(images.size());
}

}  // namespace

std::optional<renderer::EffectState> effect_state(const openrac_material& material) {
    // A field past its range is a host's mistake or a later contract's: it is not guessed at.
    if (!in_range(material)) {
        return std::nullopt;
    }

    const std::uint32_t flags = material.flags;
    renderer::EffectState state;
    state.blend = blend_of(material);

    // ALWAYS is no test: every pixel passes, so no reference and no fail mode apply.
    if (material.alpha_test != OPENRAC_MATERIAL_TEST_ALWAYS) {
        state.alpha_test.compare = static_cast<renderer::Compare>(material.alpha_test);
        state.alpha_test.reference = material.alpha_reference;
        state.alpha_test.fail = static_cast<renderer::FailWrites>(material.alpha_fail);
    }

    state.depth = depth_test_of(material.depth_test);
    state.depth_write = (flags & OPENRAC_MATERIAL_DEPTH_WRITE) != 0;
    state.colour_writes = material.writes;
    state.combine = static_cast<renderer::TextureCombine>(material.combine);
    state.texture_alpha = (flags & OPENRAC_MATERIAL_TEXTURE_ALPHA) != 0;
    state.linear_mag = (flags & OPENRAC_MATERIAL_LINEAR_MAG) != 0;
    state.linear_min = (flags & OPENRAC_MATERIAL_LINEAR_MIN) != 0;
    state.repeat_s = (flags & OPENRAC_MATERIAL_REPEAT_S) != 0;
    state.repeat_t = (flags & OPENRAC_MATERIAL_REPEAT_T) != 0;
    return state;
}

bool capture_material_quad(
    const openrac_game_material_quad& quad,
    const EffectImageLookup& lookup,
    std::vector<renderer::EffectQuad>& quads,
    std::vector<renderer::UiImage>& images
) {
    const std::optional<renderer::EffectState> state = effect_state(quad.material);

    // A material the renderer cannot hold is left to the caller's older way.
    if (!state) {
        return false;
    }

    renderer::EffectQuad out{};
    std::memcpy(out.corner, quad.corner, sizeof(out.corner));
    std::memcpy(out.rgba, quad.rgba, sizeof(out.rgba));
    std::memcpy(out.st, quad.st, sizeof(out.st));
    out.id = quad.id;
    out.parent = quad.parent;
    out.attachment = quad.attachment;
    out.native = true;
    out.state = *state;
    out.untextured = (quad.material.flags & OPENRAC_MATERIAL_UNTEXTURED) != 0;

    // A textured quad's image is read now, while game memory holds what the call names.
    if (!out.untextured) {
        const renderer::UiImage found = lookup ? lookup(quad.image) : renderer::UiImage{};

        // No image, no quad: the caller draws it its way rather than the window a placeholder.
        if (!found.image) {
            return false;
        }

        out.image = keep_image(images, found);
    }

    quads.push_back(out);
    return true;
}

}  // namespace openrac::frontend
