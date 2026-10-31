// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic adjacent-core reshaping with incremental blend-contributor scores.
#include <algorithm>
#include <tuple>
#include <unordered_map>

#include "enhanced/tools/baker/planning/region_plan_internal.h"

namespace openrac::lighting::region_plan {
namespace {
// Partitions box minus its intersection with cut into disjoint axis-ordered slabs.
std::vector<Box> subtract_box(Box box, const Box& cut) {
    Box inside{};

    // Intersection endpoints determine which source slabs remain.
    for (unsigned axis = 0; axis < 3; ++axis) {
        inside[axis] = std::max(box[axis], cut[axis]);
        inside[axis + 3] = std::min(box[axis + 3], cut[axis + 3]);

        // Disjoint boxes require no subtraction.
        if (inside[axis] >= inside[axis + 3]) {
            return {box};
        }
    }

    std::vector<Box> result;

    // Axis-ordered cuts keep residual slabs disjoint.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // The lower slab remains outside the bridge.
        if (box[axis] < inside[axis]) {
            auto part = box;
            part[axis + 3] = inside[axis];
            result.push_back(part);
            box[axis] = inside[axis];
        }

        // The upper slab remains outside the bridge.
        if (box[axis + 3] > inside[axis + 3]) {
            auto part = box;
            part[axis] = inside[axis + 3];
            result.push_back(part);
            box[axis + 3] = inside[axis + 3];
        }
    }

    return result;
}

// Rejects edge/corner contacts before two neighbouring core faces can be bridged.
bool shares_face(const Box& first, const Box& second, unsigned face_axis) {
    // A shared face requires positive overlap on its other axes.
    for (unsigned axis = 0; axis < 3; ++axis) {
        // Edge and corner contacts cannot form a volume bridge.
        if (axis != face_axis
            && std::max(first[axis], second[axis]) >= std::min(first[axis + 3], second[axis + 3])) {
            return false;
        }
    }

    return true;
}

// Appends one plane's neighbours in the former upper/lower insertion order.
void append_neighbours(
    const std::map<unsigned, Box>& active,
    unsigned axis,
    const std::vector<unsigned>& upper,
    const std::vector<unsigned>& lower,
    std::vector<std::array<unsigned, 3>>& result
) {
    // Upper-face insertion order breaks equivalent candidate ties.
    for (const auto first : upper) {
        // Lower-face insertion order completes the stable neighbour ordering.
        for (const auto second : lower) {
            // Only actual face neighbours can be reshaped together.
            if (shares_face(active.at(first), active.at(second), axis)) {
                result.push_back({first, second, axis});
            }
        }
    }
}

// Face neighbours retain axis/plane order, then the insertion order of each source core.
std::vector<std::array<unsigned, 3>> neighbours(const std::map<unsigned, Box>& active) {
    std::map<std::pair<unsigned, int>, std::vector<unsigned>> lower;
    std::map<std::pair<unsigned, int>, std::vector<unsigned>> upper;

    // Stable IDs preserve the original core insertion order.
    for (const auto& [id, box] : active) {
        // Each axis has an independent plane-neighbour table.
        for (unsigned axis = 0; axis < 3; ++axis) {
            lower[{axis, box[axis]}].push_back(id);
            upper[{axis, box[axis + 3]}].push_back(id);
        }
    }

    std::vector<std::array<unsigned, 3>> result;

    // Sorted planes reproduce the reference neighbour traversal.
    for (const auto& [key, rights] : lower) {
        append_neighbours(active, key.first, upper[key], rights, result);
    }

    return result;
}

// Bridges the common footprint and leaves the two original cores' remaining slabs untouched.
std::vector<Box> bridge(const Box& first, const Box& second, unsigned axis) {
    Box joined{};

    // The common footprint is the only part that may bridge both cores.
    for (unsigned component = 0; component < 3; ++component) {
        joined[component] = std::max(first[component], second[component]);
        joined[component + 3] = std::min(first[component + 3], second[component + 3]);
    }

    joined[axis] = std::min(first[axis], second[axis]);
    joined[axis + 3] = std::max(first[axis + 3], second[axis + 3]);
    std::vector<Box> parts{joined};

    // Retain both sources outside the shared footprint.
    for (const auto& source : {first, second}) {
        const auto remainder = subtract_box(source, joined);
        parts.insert(parts.end(), remainder.begin(), remainder.end());
    }

    return parts;
}

// One serial refinement owner; no worker completion order participates in placement decisions.
class Refiner {
public:
    // Fixed thin grids contribute cost but never participate in reshaping.
    Refiner(const std::vector<Box>& boxes, unsigned budget, std::span<const Box> fixed)
        : m_budget(budget) {
        std::vector<Box> all = boxes;
        all.insert(all.end(), fixed.begin(), fixed.end());
        const auto counts = contributor_counts(all);
        m_counts.reserve(counts.size());
        m_counts.insert(counts.begin(), counts.end());

        // Fixed grids contribute cost without becoming mutable cores.
        for (const auto& box : boxes) {
            Box core = box;

            // Stored cores exclude their blend apron during neighbour discovery.
            for (unsigned axis = 0; axis < 3; ++axis) {
                ++core[axis];
                --core[axis + 3];
            }

            m_active.emplace(m_next++, core);
        }
    }

    // At most twelve sweeps match the bounded reference convergence policy.
    std::vector<Box> run() {
        // A bounded number of sweeps prevents pathological refinement work.
        for (unsigned sweep = 0; sweep < 12; ++sweep) {
            bool changed = false;

            // Neighbour snapshots preserve deterministic within-sweep decisions.
            for (const auto& pair : neighbours(m_active)) {
                changed = replace(pair) || changed;
            }

            // Convergence needs no further neighbour reconstruction.
            if (!changed) {
                break;
            }
        }

        std::vector<Box> result;
        result.reserve(m_active.size());

        // Restore blend aprons after choosing full-strength core shapes.
        for (const auto& [id, box] : m_active) {
            result.push_back(expanded_box(box));
        }

        return result;
    }

private:
    // Evaluates only cells touched by this local bridge proposal.
    std::int64_t changed_cost(const std::map<Cell, int>& delta) {
        std::int64_t cost = 0;

        // Unchanged cells cancel from the cost difference.
        for (const auto& [cell, change] : delta) {
            cost += blend_cell_cost(m_counts[cell] + change) - blend_cell_cost(m_counts[cell]);
        }

        return cost;
    }

    // Accepts strict cost improvements only; IDs from the current sweep are never reused.
    bool replace(std::array<unsigned, 3> pair) {
        const auto first = pair[0];
        const auto second = pair[1];

        // Earlier replacements may invalidate neighbours from this sweep.
        if (!m_active.contains(first) || !m_active.contains(second)) {
            return false;
        }

        const auto parts = bridge(m_active.at(first), m_active.at(second), pair[2]);
        std::map<Cell, int> delta;

        // Replacement parts must fit before scoring changed coverage.
        for (const auto& part : parts) {
            const auto expanded = expanded_box(part);

            // Oversized parts cannot be accepted even when their overlap score improves.
            if (grid_count(expanded) > m_budget) {
                return false;
            }

            // Only affected cells participate in incremental scoring.
            for (const auto& cell : box_cells(expanded)) {
                ++delta[cell];
            }
        }

        // Remove both old allocations from the candidate contributor delta.
        for (const auto id : {first, second}) {
            // Old blend aprons must be removed along with their cores.
            for (const auto& cell : box_cells(expanded_box(m_active.at(id)))) {
                --delta[cell];
            }
        }

        // A bridge replaces exactly two existing allocations.
        std::int64_t cost = region_cost(m_budget) * (static_cast<std::int64_t>(parts.size()) - 2);

        cost += changed_cost(delta);

        // Equal-cost reshapes cannot improve the layout.
        if (cost >= 0) {
            return false;
        }

        // Apply only the delta accepted by the complete cost test.
        for (const auto& [cell, change] : delta) {
            m_counts[cell] += change;
        }

        m_active.erase(first);
        m_active.erase(second);

        // New stable IDs prevent reuse of stale neighbour pairs.
        for (const auto& part : parts) {
            m_active.emplace(m_next++, part);
        }

        return true;
    }

    unsigned m_budget;                 // Probe allocation budget used by both scoring and guards.
    unsigned m_next = 0;               // Monotonically increasing stable core ID.
    std::map<unsigned, Box> m_active;  // Insertion-ordered cores represented by stable IDs.
    std::unordered_map<Cell, int, CellHash>
        m_counts;  // Order-independent integer contributor lookup.
};
}  // namespace

std::vector<Box> refine_regions(
    const std::vector<Box>& boxes, unsigned budget, std::span<const Box> fixed
) {
    return Refiner(boxes, budget, fixed).run();
}
}  // namespace openrac::lighting::region_plan
