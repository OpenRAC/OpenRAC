// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <map>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_bake.h"
#include "enhanced/lighting/probe_regions.h"

namespace openrac::lighting::bake_detail {
// Coarse bins restrict CPU region selection without changing its boundary score or stable ties.
class CaptureGi {
public:
    explicit CaptureGi(std::span<const ProbeVolume> volumes) : m_volumes(volumes) {
        for (unsigned i = 1; i < volumes.size(); ++i) {
            index_volume(i);
        }
    }

    // Same three highest interior-boundary scores as probe_choose_regions, without camera culling.
    std::optional<ProbeSample> sample_result(
        Float3 p, Float3 n, Float3 view, const EnvironmentBakeSettings& options
    ) const {
        std::array<WeightedProbeRegion, 3> selected{};
        std::array<float, 3> scores{};

        if (const auto found = m_bins.find(bin(p)); found != m_bins.end()) {
            for (unsigned id : found->second) {
                const auto& volume = m_volumes[id];
                const auto& spacing = volume.grid.spacing;
                const float score =
                    probe_boundary_weight(volume.grid, p) / (spacing[0] * spacing[1] * spacing[2]);
                insert(selected, scores, score, volume);
            }
        }

        return sample_probe_regions(
            m_volumes[0], selected, p, n, {options.normal_bias, mul(view, options.view_bias)}
        );
    }

    Float3 sample(Float3 p, Float3 n, Float3 view, const EnvironmentBakeSettings& options) const {
        const auto value = sample_result(p, n, view, options);
        return value ? value->irradiance : Float3{};
    }

private:
    // Each volume contributes its stable ID to the bins intersecting its nominal bounds.
    void index_volume(unsigned id) {
        const auto& grid = m_volumes[id].grid;
        Float3 minimum = grid.origin, maximum = grid.origin;

        for (unsigned corner = 0; corner < 8; ++corner) {
            Float3 local{};

            for (unsigned a = 0; a < 3; ++a) {
                local[a] = corner & (1u << a) ? grid.spacing[a] * float(grid.dimensions[a] - 1) : 0;
            }

            const auto world = add(grid.origin, grid.frame.to_world(local));

            for (unsigned a = 0; a < 3; ++a) {
                minimum[a] = std::min(minimum[a], world[a]);
                maximum[a] = std::max(maximum[a], world[a]);
            }
        }

        const auto lo = bin(minimum), hi = bin(maximum);

        for (int z = lo[2]; z <= hi[2]; ++z) {
            for (int y = lo[1]; y <= hi[1]; ++y) {
                for (int x = lo[0]; x <= hi[0]; ++x) {
                    m_bins[{x, y, z}].push_back(id);
                }
            }
        }
    }

    // Insertion preserves runtime region-ID tie breaking while retaining only three contributors.
    static void insert(
        std::array<WeightedProbeRegion, 3>& selected,
        std::array<float, 3>& scores,
        float score,
        const ProbeVolume& volume
    ) {
        for (unsigned slot = 0; slot < 3; ++slot) {
            if (score <= scores[slot]) {
                continue;
            }

            for (unsigned move = 2; move > slot; --move) {
                scores[move] = scores[move - 1];
                selected[move] = selected[move - 1];
            }

            scores[slot] = score;
            selected[slot] = {&volume, 1};
            return;
        }
    }

    // Fixed spatial bins are a lookup acceleration, unrelated to either probe spacing.
    static std::array<int, 3> bin(Float3 p) {
        constexpr float kBinSize = 32;
        return {
            int(std::floor(p[0] / kBinSize)),
            int(std::floor(p[1] / kBinSize)),
            int(std::floor(p[2] / kBinSize))
        };
    }

    std::span<const ProbeVolume> m_volumes;
    std::map<std::array<int, 3>, std::vector<unsigned>> m_bins;
};

}  // namespace openrac::lighting::bake_detail
