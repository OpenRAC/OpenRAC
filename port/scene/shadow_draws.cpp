// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "scene/shadow_draws.h"

#include <algorithm>
#include <cmath>

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

std::vector<ShadowGeometryRange> prepare_shadow_geometry(
    std::span<const ShadowDrawRange> draws,
    std::span<const Vertex> vertices,
    std::span<const std::uint32_t> indices
) {
    constexpr unsigned kChunkIndices = 128 * 3;
    std::vector<ShadowGeometryRange> result;
    for (const auto& draw : draws) {
        const auto end = std::size_t(draw.first) + draw.count;
        if (draw.count % 3 != 0 || end > indices.size()) {
            result.push_back({draw});
            continue;
        }
        for (unsigned offset = 0; offset < draw.count;) {
            ShadowGeometryRange chunk{
                {draw.first + offset,
                 draw.solid ? std::min(kChunkIndices, draw.count - offset) : draw.count,
                 draw.material,
                 draw.solid}
            };
            chunk.bounded = true;
            chunk.low.fill(3e38f);
            chunk.high.fill(-3e38f);
            for (std::size_t i = chunk.draw.first;
                 i < std::size_t(chunk.draw.first) + chunk.draw.count;
                 ++i) {
                if (indices[i] >= vertices.size()) {
                    chunk.bounded = false;
                    break;
                }
                const auto& p = vertices[indices[i]].position;
                for (unsigned k = 0; k < 3; ++k) {
                    chunk.bounded &= std::isfinite(p[k]);
                    chunk.low[k] = std::min(chunk.low[k], p[k]);
                    chunk.high[k] = std::max(chunk.high[k], p[k]);
                }
            }
            offset += chunk.draw.count;
            result.push_back(chunk);
        }
    }
    return result;
}

}  // namespace openrac::scene
