// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "scene/shadow_draws.h"

namespace openrac::scene {

std::vector<ShadowDrawRange> prepare_shadow_draws(
    std::span<const Primitive> primitives,
    std::span<const Material> materials,
    const std::vector<bool>& solid_primitives,
    bool allow_solid
) {
    std::vector<ShadowDrawRange> draws;
    ShadowDrawRange run;
    const auto flush = [&] {
        if (run.count) {
            draws.push_back(run);
            run = {};
        }
    };
    for (std::size_t i = 0; i < primitives.size(); ++i) {
        const auto& primitive = primitives[i];
        const auto& material = materials[static_cast<std::size_t>(primitive.material)];
        if (material.metal || material.blend || material.colour[3] < .5f) {
            flush();
        } else if (!material.cutout && material.colour[3] >= 1) {
            const bool solid = allow_solid && solid_primitives[i];
            if (run.count
                && (primitive.first_index != run.first + run.count || solid != run.solid)) {
                flush();
            }
            if (!run.count) {
                run.first = primitive.first_index;
                run.solid = solid;
            }
            run.count += primitive.index_count;
        } else {
            flush();
            draws.push_back(
                {primitive.first_index, primitive.index_count, primitive.material, false}
            );
        }
    }
    flush();
    return draws;
}

}  // namespace openrac::scene
