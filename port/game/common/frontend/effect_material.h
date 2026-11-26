// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game thread's side of a world quad with a native material (openrac_game_material_quad): the
// C material checked and turned into the renderer's state (renderer::EffectState), and the quad's
// image read once, at the call, into the tick's own images. What the render thread later draws is
// that capture alone; it never reads game memory or a handle map. Pure: the frontend's state is
// handed in.
#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "openrac/game_host.h"
#include "renderer/renderer.h"

namespace openrac::frontend {

// The renderer's state for `material`, or none when a field is out of its range (an unknown
// weight, comparison, fail mode, depth test, texture function, write or flag bit, or a
// coefficient k that is not -1, 0 or 1, or m that is not 0 or 1).
std::optional<renderer::EffectState> effect_state(const openrac_material& material);

// An image as the game keeps it, decoded now (frontend/ui_images: the recorder's cache); an empty
// UiImage when it cannot be read.
using EffectImageLookup = std::function<renderer::UiImage(const openrac_game_ui_image&)>;

// Appends `quad` to the tick's `quads` with its state and, unless untextured, its image as
// `lookup` reads it now into the tick's `images` (EffectQuad::image - 1; an image already there is
// not added twice). False, with nothing added, when the material is out of range or the image
// cannot be read: the caller draws the quad its older way.
[[nodiscard]] bool capture_material_quad(
    const openrac_game_material_quad& quad,
    const EffectImageLookup& lookup,
    std::vector<renderer::EffectQuad>& quads,
    std::vector<renderer::UiImage>& images
);

}  // namespace openrac::frontend
