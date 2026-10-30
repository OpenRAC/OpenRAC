// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Exact union compaction and deterministic rectangular core partitioning before grid alignment.
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "common/profiler.h"
#include "enhanced/tools/probes/region_plan_internal.h"

namespace openrac::lighting::region_plan {
namespace {
// The six permutations are visited in Python itertools.permutations' lexicographic order.
constexpr std::array<Cell, 6> kOrders{
    {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}
};

// Grows one occupied core while retaining its budgeted one-cell outer fade border.
Cell grow_core(
    Cell seed, const std::unordered_set<Cell, CellHash>& remaining, unsigned budget, Cell order
) {
    Cell high{seed[0] + 1, seed[1] + 1, seed[2] + 1};

    // Growth-order trials preserve deterministic tie breaking.
    for (const auto axis : order) {
        // Stop extending this axis when coverage or budget blocks the next face.
        for (;;) {
            auto candidate = high;
            ++candidate[axis];
            const Box bounds{seed[0], seed[1], seed[2], candidate[0], candidate[1], candidate[2]};

            // Each core must retain room for its outer blend apron.
            if (grid_count(expanded_box(bounds)) > budget) {
                break;
            }

            Box face = bounds;
            face[axis] = high[axis];
            const auto cells = box_cells(face);

            // An incomplete face would fill a hole in the original union.
            if (!std::all_of(cells.begin(), cells.end(), [&remaining](Cell cell) {
                    return remaining.contains(cell);
                })) {
                break;
            }

            high = candidate;
        }
    }

    return high;
}

// Chooses the largest available core at each deterministic seed, preserving all three tuple ties.
std::vector<Box> partition(const std::vector<Cell>& cells, unsigned budget, Cell seed_order) {
    // Membership is unordered; the sorted seed list below owns every placement tie.
    std::unordered_set<Cell, CellHash> remaining(cells.begin(), cells.end());
    std::vector<Cell> ordered(cells.begin(), cells.end());
    std::sort(ordered.begin(), ordered.end(), [seed_order](const Cell& first, const Cell& second) {
        return Cell{first[seed_order[0]], first[seed_order[1]], first[seed_order[2]]}
               < Cell{second[seed_order[0]], second[seed_order[1]], second[seed_order[2]]};
    });
    std::vector<Box> result;

    // Ordered seeds make partitions independent of container implementation.
    for (const auto& seed : ordered) {
        // Earlier accepted cores already own these cells.
        if (!remaining.contains(seed)) {
            continue;
        }

        std::tuple<std::int64_t, std::int64_t, Cell> best{-1, 0, {}};

        // Try every axis order before choosing the largest valid core.
        for (const auto& order : kOrders) {
            const auto high = grow_core(seed, remaining, budget, order);
            const Box box{seed[0], seed[1], seed[2], high[0], high[1], high[2]};
            best =
                std::max(best, std::tuple{cell_count(box), -grid_count(expanded_box(box)), high});
        }

        const auto high = std::get<2>(best);
        const Box box{seed[0], seed[1], seed[2], high[0], high[1], high[2]};

        // Accepted cells cannot belong to another full-strength core.
        for (const auto& cell : box_cells(box)) {
            remaining.erase(cell);
        }

        result.push_back(expanded_box(box));
    }

    return result;
}

// Selects the same compact baseline before applying the complete blend-junction cost.
std::vector<Box> choose_partition(
    const std::vector<Cell>& cells, const std::vector<Box>& thin, unsigned budget
) {
    std::optional<std::tuple<std::int64_t, std::int64_t, std::size_t, Cell>> score;
    std::vector<Box> best;

    // Seed-order trials choose a stable low-allocation baseline.
    for (const auto& order : kOrders) {
        auto candidate = partition(cells, budget, order);
        std::int64_t volume = 0;
        std::int64_t probes = 0;
        auto all = candidate;
        all.insert(all.end(), thin.begin(), thin.end());

        // Thin allocations contribute to the same candidate cost.
        for (const auto& box : all) {
            volume += cell_count(box);
            probes += grid_count(box);
        }

        const auto current = std::tuple{volume, probes, all.size(), order};

        // Tuple order resolves equal costs without platform-dependent ties.
        if (!score || current < *score) {
            score = current;
            best = std::move(candidate);
        }
    }

    std::sort(best.begin(), best.end());
    auto result = refine_regions(best, budget, thin);
    result.insert(result.end(), thin.begin(), thin.end());
    std::sort(result.begin(), result.end());
    return result;
}

// Retains provenance from every original grid with positive overlap, never merely a touching edge.
Region materialise(const Box& box, const std::vector<Region>& originals, const Settings& settings) {
    std::set<unsigned> mobys;
    std::set<unsigned> transitions;

    // Every overlapping original source retains its provenance.
    for (const auto& original : originals) {
        const auto source = expanded_box(native_box(original.bounds, settings.spacing));
        bool overlaps = true;

        // Positive overlap excludes merely touching allocations.
        for (unsigned axis = 0; axis < 3; ++axis) {
            overlaps = overlaps && box[axis] < source[axis + 3] && source[axis] < box[axis + 3];
        }

        // Only contributing source IDs belong in this final region.
        if (overlaps) {
            mobys.insert(original.mobys.begin(), original.mobys.end());
            transitions.insert(original.transitions.begin(), original.transitions.end());
        }
    }

    return {
        world_box(box, settings.spacing),
        {mobys.begin(), mobys.end()},
        {transitions.begin(), transitions.end()}
    };
}

// Measures union coverage and summed allocation before writing any plan.
std::pair<std::size_t, std::int64_t> coverage_totals(const std::vector<Box>& boxes) {
    std::int64_t probes = 0;

    // Probe totals include shared boundary endpoints.
    for (const auto& box : boxes) {
        probes += grid_count(box);
    }

    return {contributor_counts(boxes).size(), probes};
}

// Compacts only within the complete original union and records the unchanged allocation policy.
void compact(State& state) {
    auto& regions = state.regions;
    const auto& settings = state.settings;
    std::vector<Box> before;
    before.reserve(regions.size());

    // Compaction starts from the complete original lattice union.
    for (const auto& region : regions) {
        before.push_back(lattice_box(region.bounds, settings.spacing));
    }

    const auto [covered, probes_before] = coverage_totals(before);
    const auto original_count = regions.size();
    regions = merge_regions(std::move(regions), settings, true, true);
    std::sort(regions.begin(), regions.end(), [](const Region& a, const Region& b) {
        return a.bounds < b.bounds;
    });
    std::int64_t probes_after = 0;

    // Stable world order preserves report and bake identities.
    for (const auto& region : regions) {
        probes_after += probe_count(region.bounds, settings.spacing);
    }

    state.compaction =
        {{"before", static_cast<std::int64_t>(original_count)},
         {"after", static_cast<std::int64_t>(regions.size())},
         {"covered_cells", static_cast<std::int64_t>(covered)},
         {"probes_before", probes_before},
         {"probes_after", probes_after}};
}

// Core occupancy and fixed thin grids used by all candidate growth orders.
struct PaddedCores {
    std::vector<Box> boxes;   // Original allocations after adding their blend apron.
    std::vector<Box> thin;    // Allocations too thin to inset safely.
    std::vector<Cell> cells;  // Exact full-strength core union.
};

// Bounds sparse core growth during insertion rather than after a whole source box.
void append_core(std::set<Cell>& cells, const Box& core) {
    // Bound the combined union during insertion, before trying six partitions.
    for (const auto& cell : box_cells(core)) {
        cells.insert(cell);

        // Distant boxes must not accumulate an unbounded sparse union.
        if (cells.size() > kMaximumPlanningCells) {
            throw std::runtime_error("region core coverage exceeds the planning cell limit");
        }
    }
}

// Separates core occupancy without borrowing cells from a previously chosen partition.
PaddedCores padded_cores(const std::vector<Region>& originals, const Settings& settings) {
    PaddedCores result;
    std::set<Cell> cells;

    // Every original allocation contributes its supported core.
    for (const auto& region : originals) {
        const auto box = expanded_box(native_box(region.bounds, settings.spacing));
        result.boxes.push_back(box);
        Box core = box;

        // Insetting removes the one-cell blend apron.
        for (unsigned axis = 0; axis < 3; ++axis) {
            ++core[axis];
            --core[axis + 3];
        }

        // Thin grids cannot lose their remaining full-strength coverage.
        if (box[3] - box[0] <= 2 || box[4] - box[1] <= 2 || box[5] - box[2] <= 2) {
            result.thin.push_back(box);
        } else {
            append_core(cells, core);
        }
    }

    result.cells.assign(cells.begin(), cells.end());
    return result;
}

// Measures final overlap independently of world-space conversion and source metadata.
void report_partition(State& state, const PaddedCores& cores) {
    const auto& settings = state.settings;
    const auto [union_cells, padded_probes] = coverage_totals(cores.boxes);
    state.layout_before = layout_metrics(cores.boxes, settings.budget);
    state.layout_after = layout_metrics(state.final_boxes, settings.budget);
    std::int64_t volume_before = 0;
    std::int64_t volume_after = 0;

    // Compare occupied volume before changing the partition.
    for (const auto& box : cores.boxes) {
        volume_before += cell_count(box);
    }

    // Final volume reveals duplicate blend-band coverage.
    for (const auto& box : state.final_boxes) {
        volume_after += cell_count(box);
    }

    state.repartition =
        {{"before", static_cast<std::int64_t>(cores.boxes.size())},
         {"after", static_cast<std::int64_t>(state.final_boxes.size())},
         {"covered_cells", static_cast<std::int64_t>(union_cells)},
         {"overlap_cells_before", volume_before - static_cast<std::int64_t>(union_cells)},
         {"overlap_cells_after", volume_after - static_cast<std::int64_t>(union_cells)},
         {"probes_before", padded_probes},
         {"probes_after", state.layout_after.at("probes")},
         {"border_cells", 1},
         {"thin_grids_retained", static_cast<std::int64_t>(cores.thin.size())}};
}
}  // namespace

void partition_regions(State& state) {
    const profiling::Zone timing("Partition probe regions");
    auto& regions = state.regions;
    const auto& settings = state.settings;

    // Empty levels still emit valid diagnostic objects.
    if (regions.empty()) {
        state.compaction = {{"before", 0}, {"after", 0}, {"covered_cells", 0}};
        state.repartition =
            {{"before", 0}, {"after", 0}, {"overlap_cells_before", 0}, {"overlap_cells_after", 0}};
        return;
    }

    compact(state);
    const auto originals = regions;
    const auto cores = padded_cores(originals, settings);
    state.final_boxes = choose_partition(cores.cells, cores.thin, settings.budget);
    report_partition(state, cores);
    regions.clear();
    regions.reserve(state.final_boxes.size());

    // Final allocations must satisfy the budget after all refinement.
    for (const auto& box : state.final_boxes) {
        // A retained thin allocation cannot silently exceed the native grid limit.
        if (grid_count(box) > settings.budget) {
            throw std::runtime_error("native thin grid exceeds the per-volume probe budget");
        }

        regions.push_back(materialise(box, originals, settings));
    }
}
}  // namespace openrac::lighting::region_plan
