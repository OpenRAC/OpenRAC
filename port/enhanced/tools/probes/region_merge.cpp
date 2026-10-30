// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Stable heap merging with exact union and float-allocation coverage guards.
#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "enhanced/tools/probes/region_plan_internal.h"

namespace openrac::lighting::region_plan {
namespace {
// Bounded prefix sums accelerate exact occupied-cell containment; distant boxes use sparse cells.
class Coverage {
public:
    // Owns only integer occupancy, shared by every candidate merge within this pass.
    explicit Coverage(const std::vector<Box>& boxes) {
        const auto counts = contributor_counts(boxes);

        // Containment queries need occupancy rather than contributor multiplicity.
        for (const auto& [cell, count] : counts) {
            m_cells.insert(cell);
        }

        m_bounds = boxes.front();

        // Prefix coordinates are relative to the complete original union.
        for (const auto& box : boxes) {
            // Track both endpoints without expanding the occupied union.
            for (unsigned axis = 0; axis < 3; ++axis) {
                m_bounds[axis] = std::min(m_bounds[axis], box[axis]);
                m_bounds[axis + 3] = std::max(m_bounds[axis + 3], box[axis + 3]);
            }
        }

        const auto size = grid_count(m_bounds);

        // Distant disjoint regions must not cause an enormous prefix allocation.
        if (size <= static_cast<std::int64_t>(kMaximumPlanningCells)) {
            build_prefix(static_cast<std::size_t>(size));
        }
    }

    // A merge may only fill cells already occupied by the complete original field.
    bool contains(const Box& box) const {
        // Bounds checks keep every prefix corner within its allocated shape.
        for (unsigned axis = 0; axis < 3; ++axis) {
            // A union cannot contain cells outside its own bounding extent.
            if (box[axis] < m_bounds[axis] || box[axis + 3] > m_bounds[axis + 3]) {
                return false;
            }
        }

        // Sparse containment handles levels whose overall bounding box is too large.
        if (m_prefix.empty()) {
            const auto cells = box_cells(box);
            return std::all_of(cells.begin(), cells.end(), [this](const Cell& cell) {
                return m_cells.contains(cell);
            });
        }

        std::int64_t sum = 0;

        // Eight signed corners recover the exact occupied volume.
        for (unsigned corner = 0; corner < 8; ++corner) {
            Cell cell{};
            unsigned upper = 0;

            // Corner bits select lower or upper endpoints on each axis.
            for (unsigned axis = 0; axis < 3; ++axis) {
                const bool high = (corner & (1u << axis)) != 0;
                upper += high;
                cell[axis] = box[axis + (high ? 3 : 0)] - m_bounds[axis];
            }

            sum += (upper % 2 ? 1 : -1) * static_cast<std::int64_t>(m_prefix[index(cell)]);
        }

        return sum == cell_count(box);
    }

    // Returns the exact original union's cell count for compaction diagnostics.
    std::size_t size() const { return m_cells.size(); }

private:
    // Converts a local prefix corner into its X-major array index.
    std::size_t index(Cell cell) const {
        return (static_cast<std::size_t>(cell[0]) * m_shape[1] + cell[1]) * m_shape[2] + cell[2];
    }

    // Builds the same integer summed-volume table without allocating per-query cell collections.
    void build_prefix(std::size_t size) {
        // Prefix dimensions include the empty boundary plane.
        for (unsigned axis = 0; axis < 3; ++axis) {
            m_shape[axis] = m_bounds[axis + 3] - m_bounds[axis] + 1;
        }

        m_prefix.resize(size);

        // One-based coordinates leave the zero boundary planes empty.
        for (const auto& cell : m_cells) {
            m_prefix[index(
                {cell[0] - m_bounds[0] + 1, cell[1] - m_bounds[1] + 1, cell[2] - m_bounds[2] + 1}
            )] = 1;
        }

        const auto plane = static_cast<std::size_t>(m_shape[1]) * m_shape[2];

        // Accumulate planes in dependency order for exact integer sums.
        for (int x = 1; x < m_shape[0]; ++x) {
            // Rows combine the previous X and Y planes.
            for (int y = 1; y < m_shape[1]; ++y) {
                std::uint32_t running = 0;

                // A running Z sum avoids an extra prefix read per cell.
                for (int z = 1; z < m_shape[2]; ++z) {
                    const auto at = index({x, y, z});
                    running += m_prefix[at];
                    m_prefix[at] = running + m_prefix[at - plane] + m_prefix[at - m_shape[2]]
                                   - m_prefix[at - plane - m_shape[2]];
                }
            }
        }
    }

    Box m_bounds{};                       // Original occupancy bounding box.
    Cell m_shape{};                       // Prefix dimensions including zero boundary planes.
    std::set<Cell> m_cells;               // Sparse fallback and original union evidence.
    std::vector<std::uint32_t> m_prefix;  // At most four million occupancy sums.
};

// Bounding union; all source boxes remain contained.
Bounds united(const Bounds& first, const Bounds& second) {
    Bounds result{};

    // Union endpoints retain every source box.
    for (unsigned axis = 0; axis < 3; ++axis) {
        result[axis] = std::min(first[axis], second[axis]);
        result[axis + 3] = std::max(first[axis + 3], second[axis + 3]);
    }

    return result;
}

// Counts double world volume in the reference algorithm's axis order.
double world_volume(const Bounds& box) {
    double volume = 1;

    // Axis-order multiplication preserves the former pair-score arithmetic.
    for (unsigned axis = 0; axis < 3; ++axis) {
        volume *= std::max(0.0, box[axis + 3] - box[axis]);
    }

    return volume;
}

// Sorted source IDs make report identity independent of merging order.
std::vector<unsigned> source_union(
    const std::vector<unsigned>& first, const std::vector<unsigned>& second
) {
    std::set<unsigned> values(first.begin(), first.end());
    values.insert(second.begin(), second.end());
    return {values.begin(), values.end()};
}

// Immutable pair score, with source generations invalidating stale heap entries.
struct Pair {
    double score = 0;                // Probe count ratio, compared before stable IDs.
    unsigned first = 0;              // Earlier source survives every merge.
    unsigned second = 0;             // Later source is removed.
    unsigned first_generation = 0;   // Generation at scoring time.
    unsigned second_generation = 0;  // Generation at scoring time.
    Bounds bounds{};                 // Candidate bounding union.
    std::int64_t count = 0;          // Candidate endpoint probes.

    // Heap ordering mirrors Python's (score, i, j, generation_i, generation_j) tuple.
    bool operator>(const Pair& other) const {
        return std::tie(score, first, second, first_generation, second_generation) > std::tie(
                   other.score,
                   other.first,
                   other.second,
                   other.first_generation,
                   other.second_generation
               );
    }
};

// One deterministic merge pass, borrowing settings and owning all candidate/cache state.
class Merger {
public:
    // Final compaction alone enables both exact world and native float-allocation coverage guards.
    Merger(std::vector<Region> regions, const Settings& settings, bool preserve_height, bool exact)
        : m_settings(settings),
          m_preserve_height(preserve_height) {
        std::vector<Box> world;
        std::vector<Box> native;

        // Stable source IDs survive movement into the active map.
        for (unsigned index = 0; index < regions.size(); ++index) {
            m_counts.push_back(probe_count(regions[index].bounds, settings.spacing));
            m_active.emplace(index, std::move(regions[index]));

            // Exact compaction checks both world and float-allocated occupancy.
            if (exact) {
                world.push_back(lattice_box(m_active.at(index).bounds, settings.spacing));
                native.push_back(native_box(m_active.at(index).bounds, settings.spacing));
            }
        }

        m_generations.resize(regions.size());

        // Only final compaction owns complete-union containment evidence.
        if (exact) {
            m_world.emplace(world);
            m_native.emplace(native);
        }
    }

    // Re-scores only neighbours of the surviving stable ID after an accepted merge.
    std::vector<Region> run() {
        // Initial pair insertion order matches the reference heap.
        for (const auto& [first, region] : m_active) {
            // Each unordered pair is scored once.
            for (unsigned second = first + 1; second < m_counts.size(); ++second) {
                offer(first, second);
            }
        }

        // Drain the heap until no compatible merge remains.
        while (!m_pairs.empty()) {
            const Pair pair = m_pairs.top();
            m_pairs.pop();

            // Changed generations invalidate candidates scored before an accepted merge.
            if (!m_active.contains(pair.first) || !m_active.contains(pair.second)
                || m_generations[pair.first] != pair.first_generation
                || m_generations[pair.second] != pair.second_generation) {
                continue;
            }

            auto& first = m_active.at(pair.first);
            const auto& second = m_active.at(pair.second);
            first =
                {pair.bounds,
                 source_union(first.mobys, second.mobys),
                 source_union(first.transitions, second.transitions)};
            m_active.erase(pair.second);
            ++m_generations[pair.first];
            m_counts[pair.first] = pair.count;

            // Only the surviving region needs new candidate scores.
            for (const auto& [other, region] : m_active) {
                // A region cannot merge with itself.
                if (other != pair.first) {
                    offer(std::min(other, pair.first), std::max(other, pair.first));
                }
            }
        }

        std::vector<Region> result;
        result.reserve(m_active.size());

        // Stable IDs determine final output order.
        for (auto& [index, region] : m_active) {
            result.push_back(std::move(region));
        }

        return result;
    }

private:
    // Rejects new empty height columns and separation before considering allocation cost.
    bool compatible(const Bounds& first, const Bounds& second, const Bounds& bounds) const {
        // A taller footprint must not spread over unsupported lower columns.
        if (m_preserve_height && !m_world && first[5] != second[5]) {
            const auto& high = first[5] > second[5] ? first : second;
            const auto& low = first[5] > second[5] ? second : first;

            // The lower footprint must be contained horizontally.
            for (unsigned axis = 0; axis < 2; ++axis) {
                // Extending the taller footprint would invent new high coverage.
                if (high[axis] > low[axis] || high[axis + 3] < low[axis + 3]) {
                    return false;
                }
            }
        }

        Bounds overlap{};

        // Separated sources cannot become one bounding box.
        for (unsigned axis = 0; axis < 3; ++axis) {
            // A gap on any axis would introduce unsupported cells.
            if (first[axis] > second[axis + 3] || second[axis] > first[axis + 3]) {
                return false;
            }

            overlap[axis] = std::max(first[axis], second[axis]);
            overlap[axis + 3] = std::min(first[axis + 3], second[axis + 3]);
        }

        return m_world || !m_preserve_height
               || world_volume(bounds)
                      <= world_volume(first) + world_volume(second) - world_volume(overlap);
    }

    // Float snapping must neither lose a source grid nor add unsupported cells.
    bool exact_contains(const Bounds& bounds, const Bounds& first, const Bounds& second) const {
        // Seed merging has no complete-union evidence to test.
        if (!m_world || !m_native) {
            return true;
        }

        const auto target = native_box(bounds, m_settings.spacing);

        // Float snapping must retain both original source allocations.
        for (const auto& source :
             {native_box(first, m_settings.spacing), native_box(second, m_settings.spacing)}) {
            // Containment must hold on all three axes.
            for (unsigned axis = 0; axis < 3; ++axis) {
                // A rounded union must not lose an endpoint from either source.
                if (target[axis] > source[axis] || target[axis + 3] < source[axis + 3]) {
                    return false;
                }
            }
        }

        return m_native->contains(target)
               && m_world->contains(lattice_box(bounds, m_settings.spacing));
    }

    // Stable pair IDs and generations preserve the former heap's accepted merge sequence.
    void offer(unsigned first, unsigned second) {
        const auto& a = m_active.at(first).bounds;
        const auto& b = m_active.at(second).bounds;
        const auto bounds = united(a, b);
        const auto count = probe_count(bounds, m_settings.spacing);
        const double score =
            static_cast<double>(count) / static_cast<double>(m_counts[first] + m_counts[second]);

        // The existing budget, ratio and coverage guards jointly select merge candidates.
        if (count > m_settings.budget || score > 1.15 || !compatible(a, b, bounds)
            || !exact_contains(bounds, a, b)) {
            return;
        }

        // Bound stale heap entries as well as active candidate memory.
        if (m_pairs.size() >= 2'000'000) {
            throw std::runtime_error("region merge exceeds the candidate memory limit");
        }

        m_pairs.push(
            {score, first, second, m_generations[first], m_generations[second], bounds, count}
        );
    }

    const Settings& m_settings;           // Borrowed until this synchronous pass returns.
    bool m_preserve_height;               // Prevent taller bounds spreading over a lower footprint.
    std::map<unsigned, Region> m_active;  // Stable source/insertion order.
    std::vector<unsigned> m_generations;  // Updated only on accepted merges.
    std::vector<std::int64_t> m_counts;   // Cached endpoint counts per stable source.
    std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>>
        m_pairs;                       // Lowest tuple first.
    std::optional<Coverage> m_world;   // Original world-cell union.
    std::optional<Coverage> m_native;  // Original float-snapped allocation union.
};
}  // namespace

std::vector<Region> merge_regions(
    std::vector<Region> regions, const Settings& settings, bool preserve_height, bool exact_coverage
) {
    // An empty level has no pair scores or coverage bounds to build.
    if (regions.empty()) {
        return {};
    }

    return Merger(std::move(regions), settings, preserve_height, exact_coverage).run();
}
}  // namespace openrac::lighting::region_plan
