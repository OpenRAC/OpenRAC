// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Collision floor components and authored lighting moods, propagated through bounded enclosed air.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <queue>
#include <set>
#include <span>
#include <stdexcept>
#include <type_traits>

#include "enhanced/lighting/interior_inference_internal.h"
#include "enhanced/lighting/interiors.h"

#include <embree4/rtcore.h>

namespace openrac::lighting::interiors::detail {
namespace {
// ---- Authored endpoint evidence ----

// Assigns the endpoint's mood to every visible floor component, including disconnected support patches.
void label_component(Result& result, unsigned cell, unsigned label) {
    auto& labels = result.regions[result.cells[cell].component].labels;

    // Multiple samples and agreeing entrances must not turn one mood into an apparent conflict.
    if (std::find(labels.begin(), labels.end(), label) == labels.end()) {
        labels.push_back(label);
    }
}

// Keeps endpoint evidence from jumping across a different authored lighting boundary.
bool crosses_boundary(std::span<const Transition> transitions, Point from, Point to) {
    return std::any_of(transitions.begin(), transitions.end(), [&](const Transition& transition) {
        return crosses_transition(transition, from, to);
    });
}

// Borrows the collision scene and inference output while attaching authored endpoint evidence.
struct SeedContext {
    const Scene& scene;  // Collision acceleration remains alive throughout inference.
    std::span<const Transition> transitions;  // All authored boundaries, borrowed from the caller.
    const Settings& settings;  // Clearance and body-height settings used by the floor sampler.
    Result& result;            // Owns sampled components and receives their lighting labels.
};

// Labels every visible component on this endpoint and keeps the closest supported sample for diagnostics.
void attach_endpoint(const SeedContext& context, const Transition& transition, Seed& seed) {
    const float sign = seed.side ? 1.0f : -1.0f;
    constexpr float kSeedReach = 20;
    float best = kSeedReach;

    // Visibility can connect lighting evidence across floor gaps without joining the walkable graph.
    for (unsigned i = 0; i < context.result.cells.size(); ++i) {
        const auto& cell = context.result.cells[i];

        // The authored centre describes air; roofs above it cannot be the intended receiver floor.
        if (cell.position[2] > seed.target[2] - .25f) {
            continue;
        }

        auto body = cell.position;
        body[2] += context.settings.height * .5f;
        const auto position = local(transition, body);

        // Search beyond this endpoint within the authored transition's finite width and height.
        if (position[0] * sign < 1 || position[0] * sign > 3 || std::abs(position[1]) > 1.25f
            || std::abs(position[2]) > 1.5f) {
            continue;
        }

        const float distance = length(sub(body, seed.target));

        // Real walls and other authored boundaries separate evidence even when samples are nearby.
        if (distance > kSeedReach || !context.scene.clear(seed.target, body)
            || crosses_boundary(context.transitions, seed.target, body)) {
            continue;
        }

        label_component(context.result, i, seed.label);
        const std::size_t count = context.result.regions[cell.component].cells.size();
        // A bounded support penalty favours substantial floor patches for the diagnostic seed.
        const float score = distance + 2.0f / std::sqrt(static_cast<float>(count));

        if (score < best) {
            best = score;
            seed.cell = static_cast<int>(i);
        }
    }
}

// ---- Room-air propagation ----

// Extends an authored doorway boundary vertically so room-air routes cannot bypass it above the player.
bool crosses_air_boundary(std::span<const Transition> transitions, Point from, Point to) {
    for (const auto& transition : transitions) {
        const auto a = local(transition, from);
        const auto b = local(transition, to);

        // Parallel segments and points remaining on the same side do not cross the dividing plane.
        if ((a[0] < 0) == (b[0] < 0) || std::abs(a[0] - b[0]) < 1e-8f) {
            continue;
        }

        const float fraction = a[0] / (a[0] - b[0]);
        const float across = a[1] + (b[1] - a[1]) * fraction;

        // Retain the finite doorway width and its existing two-percent edge tolerance.
        if (std::abs(across) <= 1.02f) {
            return true;
        }
    }

    return false;
}

// A cached free-space voxel and every authored mood that can reach it.
struct LightingAirCell {
    bool enclosed = false;  // Collision and enclosure tests have accepted this voxel.
    unsigned distance =
        std::numeric_limits<unsigned>::max();  // Shortest enclosed-air route in cells.
    std::vector<unsigned> labels;              // Equally close moods remain conflicting evidence.
};

// Routes lighting evidence through enclosed air independently of fragmented walkable support.
class LightingAirGrid {
public:
    // Borrows the collision scene and result until propagation completes on the caller's thread.
    explicit LightingAirGrid(const SeedContext& context)
        : m_context(context),
          m_region_distance(context.result.regions.size(), std::numeric_limits<unsigned>::max()) {}

    // Floods authored evidence, then attaches it to previously unlabelled floor components.
    void propagate() {
        for (const auto& cell : m_context.result.cells) {
            const auto& labels = m_context.result.regions[cell.component].labels;

            for (const unsigned label : labels) {
                attach(cell.position, label);
            }
        }

        flood();
        std::vector<bool> authored(m_context.result.regions.size());

        for (unsigned region = 0; region < authored.size(); ++region) {
            authored[region] = !m_context.result.regions[region].labels.empty();
        }

        for (const auto& cell : m_context.result.cells) {
            // Direct endpoint evidence has priority over any indirect route through the room.
            if (!authored[cell.component]) {
                receive(cell);
            }
        }

        for (auto& region : m_context.result.regions) {
            std::sort(region.labels.begin(), region.labels.end());
        }
    }

private:
    // Integer coordinates on the inference grid; positions are reconstructed without accumulated steps.
    using Key = std::array<int, 3>;

    // Finds the nearest voxel at the same resolution as the sampled floor grid.
    Key key(Point point) const {
        Key result{};

        for (unsigned axis = 0; axis < 3; ++axis) {
            const double coordinate =
                std::round(static_cast<double>(point[axis]) / m_context.settings.spacing);

            // Untrusted world coordinates must remain representable before narrowing to a voxel key.
            if (!std::isfinite(coordinate) || coordinate < std::numeric_limits<int>::min()
                || coordinate > std::numeric_limits<int>::max()) {
                throw std::runtime_error("interior lighting coordinate exceeds grid range");
            }

            result[axis] = static_cast<int>(coordinate);
        }

        return result;
    }

    // Reconstructs a voxel centre in game world units.
    Point position(Key index) const {
        Point result{};

        for (unsigned axis = 0; axis < 3; ++axis) {
            result[axis] = static_cast<float>(index[axis]) * m_context.settings.spacing;
        }

        return result;
    }

    // Requires a nearby ceiling and surrounding walls without depending on walkable floor support.
    bool enclosed(Point point) const {
        constexpr float kRoomReach = 32;
        constexpr unsigned kWallDirections = 8;
        constexpr unsigned kRequiredWalls = 5;

        if (m_context.scene.hit(point, {0, 0, 1}, kRoomReach).hit.geomID
            == RTC_INVALID_GEOMETRY_ID) {
            return false;
        }

        unsigned walls = 0;

        for (unsigned ray = 0; ray < kWallDirections; ++ray) {
            const float angle =
                static_cast<float>(ray) * 2 * std::numbers::pi_v<float> / kWallDirections;
            walls += m_context.scene.hit(point, {std::cos(angle), std::sin(angle), 0}, kRoomReach)
                         .hit.geomID
                     != RTC_INVALID_GEOMETRY_ID;
        }

        return walls >= kRequiredWalls;
    }

    // Caches rejected space as well as enclosed voxels, bounding all visited memory by max_cells.
    LightingAirCell& cell(Key index) {
        const auto found = m_cells.find(index);

        if (found != m_cells.end()) {
            return found->second;
        }

        if (m_cells.size() >= m_context.settings.max_cells) {
            throw std::runtime_error("interior lighting air-grid budget exceeded");
        }

        return m_cells
            .emplace(
                index,
                LightingAirCell{enclosed(position(index)), std::numeric_limits<unsigned>::max(), {}}
            )
            .first->second;
    }

    // Keeps every air edge collision-free and on the same side of authored lighting boundaries.
    bool clear(Point from, Point to) const {
        return !crosses_air_boundary(m_context.transitions, from, to)
               && m_context.scene.clear(from, to);
    }

    // Queues each closest label once per voxel, retaining equal-distance conflicts.
    void label(Key index, unsigned value, unsigned distance) {
        auto& target = cell(index);

        if (!target.enclosed || distance > target.distance) {
            return;
        }

        // A shorter collision-checked route replaces more distant evidence; equal routes retain ties.
        if (distance < target.distance) {
            target.distance = distance;
            target.labels.clear();
        }

        if (std::find(target.labels.begin(), target.labels.end(), value) == target.labels.end()) {
            target.labels.push_back(value);
            m_pending.emplace(index, value);
        }
    }

    // Inserts evidence only when snapping a supported body point does not cross blocking geometry.
    void attach(Point floor, unsigned value) {
        const auto body = add(floor, {0, 0, m_context.settings.height * .5f});
        const auto index = key(body);

        if (clear(body, position(index))) {
            label(index, value, 0);
        }
    }

    // Checks one bounded face neighbour before adding its next-distance evidence.
    void visit_neighbour(Key index, unsigned value, unsigned axis, int sign) {
        auto next = index;
        const auto coordinate = static_cast<std::int64_t>(index[axis]) + sign;

        // The bounded grid cannot expand beyond the integer key range.
        if (coordinate < std::numeric_limits<int>::min()
            || coordinate > std::numeric_limits<int>::max()) {
            return;
        }

        next[axis] = static_cast<int>(coordinate);

        // Every traversed edge needs clear air and must remain within the authored mood boundary.
        if (cell(next).enclosed && clear(position(index), position(next))) {
            label(next, value, cell(index).distance + 1);
        }
    }

    // Expands six face neighbours, including vertical routes through stairways and floor openings.
    void flood() {
        while (!m_pending.empty()) {
            const auto [index, value] = m_pending.front();
            m_pending.pop();

            for (unsigned axis = 0; axis < 3; ++axis) {
                for (const int sign : {-1, 1}) {
                    visit_neighbour(index, value, axis, sign);
                }
            }
        }
    }

    // Transfers the closest air-path evidence without merging floor geometry.
    void receive(const Cell& receiver) {
        const auto body = add(receiver.position, {0, 0, m_context.settings.height * .5f});
        const auto index = key(body);
        const auto found = m_cells.find(index);

        if (found == m_cells.end() || !clear(body, position(index))) {
            return;
        }

        auto& distance = m_region_distance[receiver.component];

        // The closest supported part of a component supplies its mood; equal-distance evidence remains ambiguous.
        if (found->second.distance > distance) {
            return;
        }

        auto& labels = m_context.result.regions[receiver.component].labels;

        if (found->second.distance < distance) {
            distance = found->second.distance;
            labels.clear();
        }

        for (const unsigned value : found->second.labels) {
            if (std::find(labels.begin(), labels.end(), value) == labels.end()) {
                labels.push_back(value);
            }
        }
    }

    const SeedContext&
        m_context;  // Scene and output outlive this single-threaded propagation pass.
    std::map<Key, LightingAirCell>
        m_cells;  // Cached enclosure and labels for visited voxel centres.
    std::queue<std::pair<Key, unsigned>> m_pending;  // Unexpanded voxel/label pairs.
    std::vector<unsigned>
        m_region_distance;  // Closest received air-path distance for each original component.
};

// Fills enclosed air so lighting can travel around corners without requiring continuous floor samples.
void propagate_lighting_labels(const SeedContext& context) {
    LightingAirGrid(context).propagate();
}

// ---- Transition labels ----

// Shares label IDs across matching authored light-set/ambient pairs and attaches both portal endpoints.
}  // namespace

void assign_transition_labels(
    const Scene& scene,
    std::span<const Transition> transitions,
    const Settings& settings,
    Result& result
) {
    std::vector<std::pair<int, std::array<unsigned, 3>>> labels;
    const SeedContext context{scene, transitions, settings, result};

    // Each endpoint has separate authored evidence; agreeing entrances reuse the same label.
    for (const auto& transition : transitions) {
        for (unsigned side = 0; side < 2; ++side) {
            const auto key = std::pair{transition.light[side], transition.ambient[side]};
            const auto found = std::find(labels.begin(), labels.end(), key);
            const auto label = static_cast<unsigned>(found - labels.begin());

            if (found == labels.end()) {
                labels.push_back(key);
            }

            const float sign = side ? 1.0f : -1.0f;
            // The endpoint lies just beyond the authored blend band in local X.
            Seed seed{transition.id, side, -1, label, world(transition, {sign * 1.15f, 0, 0})};
            attach_endpoint(context, transition, seed);
            result.seeds.push_back(seed);
        }
    }

    propagate_lighting_labels(context);
}
}  // namespace openrac::lighting::interiors::detail
