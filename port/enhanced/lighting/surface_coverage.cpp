// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Surface coverage's bounded enclosure flood, accelerated by two-sided Embree collision queries.
#include "enhanced/lighting/surface_coverage.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <set>
#include <stdexcept>
#include <tuple>

#include "common/profiler.h"
#include "enhanced/lighting/surface_coverage_internal.h"

namespace openrac::lighting {
namespace {
// Existing planner limits bound work and reject open outdoor overhangs.
constexpr unsigned kMaximumSamples = 4096;
constexpr double kRayNear = 0.05;
constexpr double kEyeHeight = 1.5;

// One queued cell at a specific occupied-height reference.
struct Sample {
    int x = 0;             // World XY cell coordinate.
    int y = 0;             // World XY cell coordinate.
    double reference = 0;  // Height from which support is searched downwards.
    bool grown = false;    // Flood neighbours already have source-floor evidence.
};

// Retains source-floor evidence before allowing a padded seed to start new room coverage.
bool connected(
    const CoverageSurfaces& surfaces, const CoverageSeed& seed, CoveragePoint point, double step
) {
    // Any actual source floor may establish connectivity for this seed.
    for (const auto& anchor : seed.anchors) {
        // Separate storeys must not seed each other's ceiling expansion.
        if (std::abs(point[2] - anchor[2]) > step) {
            continue;
        }

        CoveragePoint direction{};
        double squared = 0;

        // Connection distance includes height changes rather than only XY displacement.
        for (unsigned axis = 0; axis < direction.size(); ++axis) {
            direction[axis] = point[axis] - anchor[axis];
            squared += direction[axis] * direction[axis];
        }

        const double distance = std::sqrt(squared);

        // Coincident floor evidence needs no visibility ray.
        if (distance <= kRayNear) {
            return true;
        }

        // Embree distances use unit directions.
        for (double& axis : direction) {
            axis /= distance;
        }

        // A clear player-height segment prevents padded seeds from crossing solid cliffs.
        if (!surfaces.ray({anchor[0], anchor[1], anchor[2] + kEyeHeight}, direction, distance)) {
            return true;
        }
    }

    return false;
}

// Bounded per-seed state, owned only by the calling level controller.
class Flood {
public:
    // Borrows immutable geometry and settings until run() returns.
    Flood(
        const CoverageSurfaces& surfaces, const CoverageSeed& seed, const CoverageSettings& settings
    )
        : m_surfaces(surfaces),
          m_seed(seed),
          m_settings(settings) {}

    // Starts with occupied seed bands, then grows only through connected enclosed cells.
    CoverageResult run() {
        initialise();

        // Each accepted sample can enqueue four neighbours; seen keys and the limit terminate cycles.
        while (!m_queue.empty()) {
            const Sample sample = m_queue.front();
            m_queue.pop_front();
            const auto key = std::tuple(
                sample.x,
                sample.y,
                std::nearbyint(sample.reference / m_settings.spacing[2]),
                sample.grown
            );

            // The same occupied band may arrive through multiple accepted neighbours.
            if (m_seen.contains(key)) {
                continue;
            }

            // Bounded work prevents a malformed or very large enclosure from flooding indefinitely.
            if (m_seen.size() >= kMaximumSamples) {
                m_result.sample_limit_reached = true;
                break;
            }

            m_seen.insert(key);
            visit(sample);
        }

        m_result.seen = static_cast<unsigned>(m_seen.size());
        return std::move(m_result);
    }

private:
    // Retains all original outdoor cells and samples tall seeds at the reference planner's interval.
    void initialise() {
        const auto& bounds = m_seed.bounds;

        // Retain every original XY cell even when no enclosed room is found.
        for (int x = static_cast<int>(std::floor(bounds[0] / cell(0)));
             x < std::ceil(bounds[3] / cell(0));
             ++x) {
            // Original cells remain at their authored height until enclosure is confirmed.
            for (int y = static_cast<int>(std::floor(bounds[1] / cell(1)));
                 y < std::ceil(bounds[4] / cell(1));
                 ++y) {
                m_result.cells[{x, y}] = {bounds[2], bounds[5]};
                enqueue_bands(x, y);
            }
        }
    }

    // Samples every occupied band so a tall seed cannot skip an enclosed lower room.
    void enqueue_bands(int x, int y) {
        const double first = m_seed.bounds[2] + std::max(m_settings.spacing[2], 2.0) + 0.25;

        // Two probe layers, at least four units, retain the reference occupied-band sampling.
        for (double reference = first; reference <= m_seed.bounds[5];
             reference += std::max(m_settings.spacing[2] * 2, 4.0)) {
            m_queue.push_back({x, y, reference, false});
        }
    }

    // Returns the coverage cell width, twice the corresponding probe spacing.
    double cell(unsigned axis) const { return m_settings.spacing[axis] * 2; }

    // Keeps the expansion within supported coarse cells and the original seed's floor band.
    void visit(const Sample& sample) {
        CoveragePoint point{
            (sample.x + 0.5) * cell(0), (sample.y + 0.5) * cell(1), sample.reference
        };

        // Coverage needs one probe cell of support inside each coarse XY boundary.
        for (unsigned axis = 0; axis < 2; ++axis) {
            // Unsupported edge cells must not extend into unallocated coarse space.
            if (point[axis] < m_settings.lower[axis] + m_settings.spacing[axis]
                || point[axis] > m_settings.upper[axis] - m_settings.spacing[axis]) {
                return;
            }
        }

        const auto room = m_surfaces.room(point, m_settings);

        // Outdoor samples and different storeys cannot grow this seed.
        if (!room || (*room)[0] < m_seed.bounds[2] - m_settings.spacing[2]
            || (*room)[0] > m_seed.bounds[5]) {
            return;
        }

        point[2] = (*room)[0];
        const double step = std::max(2.0, m_settings.spacing[2]);

        // Only initial moby samples need explicit source-floor evidence.
        if (!sample.grown && m_seed.require_anchor && !connected(m_surfaces, m_seed, point, step)) {
            ++m_result.disconnected;
            return;
        }

        // Flood neighbours may follow steps but cannot jump to another storey.
        if (sample.grown && std::abs(point[2] - (sample.reference - 1)) > step) {
            return;
        }

        accept(sample, *room);
    }

    // Extends only enclosed cells, then follows unblocked neighbours at the accepted floor height.
    void accept(const Sample& sample, const std::array<double, 2>& room) {
        const auto& settings = m_settings;
        const double z = settings.spacing[2];
        const double limit = std::min(settings.upper[2], m_seed.bounds[5] + settings.max_rise);
        const double padding = std::max(settings.padding, z);
        std::array<double, 2> bounds{
            std::max(settings.lower[2], std::floor((room[0] - z) / z) * z),
            std::max(
                m_seed.bounds[5],
                std::min(std::floor(limit / z) * z, std::ceil((room[1] + padding) / z) * z)
            )
        };
        ++m_result.indoor;
        m_result.limited += room[1] + padding > limit;
        const std::array<int, 2> key{sample.x, sample.y};
        const auto old = m_result.cells.find(key);

        // Different occupied bands can extend the same XY coverage cell.
        if (old != m_result.cells.end()) {
            bounds[0] = std::min(bounds[0], old->second[0]);
            bounds[1] = std::max(bounds[1], old->second[1]);
        } else {
            ++m_result.discovered;
        }

        m_result.cells[key] = bounds;

        // Traversal order matches the reference flood, including its sample-cap behaviour.
        for (const auto delta :
             std::array<std::array<int, 2>, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}}) {
            const double distance = delta[0] != 0 ? cell(0) : cell(1);

            // Neighbour growth requires an unobstructed player-height segment.
            if (!m_surfaces.ray(
                    {(sample.x + 0.5) * cell(0), (sample.y + 0.5) * cell(1), room[0] + kEyeHeight},
                    {static_cast<double>(delta[0]), static_cast<double>(delta[1]), 0},
                    distance
                )) {
                m_queue.push_back({sample.x + delta[0], sample.y + delta[1], room[0] + 1, true});
            }
        }
    }

    const CoverageSurfaces& m_surfaces;  // Borrowed immutable collision scenes.
    const CoverageSeed& m_seed;          // Borrowed source-floor evidence.
    const CoverageSettings& m_settings;  // Borrowed lattice and height limits.
    CoverageResult m_result;             // Owned expanded cells and diagnostics.
    std::deque<Sample> m_queue;  // Breadth-first frontier preserves deterministic sample limits.
    std::set<std::tuple<int, int, double, bool>> m_seen;  // Prevents revisiting an occupied band.
};

// Rejects seeds whose initial cells or height bands would exceed the bounded coverage protocol.
void validate_seeds(const std::vector<CoverageSeed>& seeds, const CoverageSettings& settings) {
    // Initial cells can exceed the flood's visited cap, but must fit one bounded response line.
    constexpr double kMaximumInitialCells = 8192;
    constexpr double kMaximumQueuedSamples = 262144;
    constexpr double kMaximumCellCoordinate = 100000000;

    // Validate every seed before allocating collision acceleration or flood storage.
    for (const auto& seed : seeds) {
        double count = 1;

        // Coordinates must fit the integer cell keys used by the frontier.
        for (unsigned axis = 0; axis < settings.spacing.size(); ++axis) {
            const double lower = seed.bounds[axis];
            const double upper = seed.bounds[axis + 3];
            const double cell = settings.spacing[axis] * 2;

            // Reject inverted, nonfinite or unrepresentable seed bounds before conversion.
            if (!std::isfinite(lower) || !std::isfinite(upper) || upper <= lower
                || std::abs(lower / cell) > kMaximumCellCoordinate
                || std::abs(upper / cell) > kMaximumCellCoordinate) {
                throw std::runtime_error("invalid surface coverage seed bounds");
            }

            // Only XY dimensions allocate original coverage cells.
            if (axis < 2) {
                count *= std::ceil(upper / cell) - std::floor(lower / cell);
            }
        }

        const double bands =
            std::ceil((seed.bounds[5] - seed.bounds[2]) / std::max(settings.spacing[2] * 2, 4.0));

        // Initial frontier storage must remain bounded independently of the visited-sample cap.
        if (count > kMaximumInitialCells || count * bands > kMaximumQueuedSamples) {
            throw std::runtime_error("surface coverage seed exceeds bounded sample budget");
        }

        // Source-floor evidence must not introduce nonfinite visibility rays.
        for (const auto& anchor : seed.anchors) {
            // Reject corrupt anchors rather than losing their connectivity evidence.
            if (!std::ranges::all_of(anchor, [](double value) { return std::isfinite(value); })) {
                throw std::runtime_error("nonfinite surface coverage anchor");
            }
        }
    }
}
}  // namespace

Result<std::vector<CoverageResult>> extend_surface_coverage(
    const std::vector<interiors::Triangle>& triangles,
    const std::vector<CoverageSeed>& seeds,
    const CoverageSettings& settings
) {
    // Convert allocation and malformed-input failures to the shared Result interface.
    try {
        const profiling::Zone zone("Extend surface coverage");

        // Invalid lattice arithmetic must be rejected before integer conversion or allocation.
        for (unsigned axis = 0; axis < settings.spacing.size(); ++axis) {
            // Invalid axis bounds would make lattice conversion or flood traversal unbounded.
            if (!std::isfinite(settings.spacing[axis]) || settings.spacing[axis] <= 0
                || !std::isfinite(settings.lower[axis]) || !std::isfinite(settings.upper[axis])
                || settings.upper[axis] < settings.lower[axis]) {
                return unexpected(Error{"invalid surface coverage lattice"});
            }
        }

        // Height limits must remain usable before any ceiling arithmetic.
        if (!std::isfinite(settings.max_rise) || settings.max_rise <= 0
            || !std::isfinite(settings.padding) || settings.padding < 0) {
            return unexpected(Error{"invalid surface coverage height limits"});
        }

        validate_seeds(seeds, settings);
        CoverageSurfaces surfaces(triangles);
        std::vector<CoverageResult> results;
        results.reserve(seeds.size());

        // Independent floods share the same immutable collision acceleration.
        for (const auto& seed : seeds) {
            results.push_back(Flood(surfaces, seed, settings).run());
        }

        return results;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
