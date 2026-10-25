// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Collision floor components and authored lighting moods, propagated through bounded enclosed air.

#include "enhanced/lighting/interiors.h"

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

#include <embree4/rtcore.h>

namespace openrac::lighting::interiors {
namespace {

// ---- Collision queries ----

Point sub(Point a, Point b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Point add(Point a, Point b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Point mul(Point a, float b) {
    return {a[0] * b, a[1] * b, a[2] * b};
}

float dot(Point a, Point b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Point cross(Point a, Point b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

float length(Point a) {
    return std::sqrt(dot(a, a));
}

// Owns collision acceleration; partially constructed scenes release every successful Embree allocation.
struct Scene {
    std::unique_ptr<std::remove_pointer_t<RTCDevice>, decltype(&rtcReleaseDevice)> device{
        rtcNewDevice(nullptr), rtcReleaseDevice
    };
    std::unique_ptr<std::remove_pointer_t<RTCScene>, decltype(&rtcReleaseScene)> scene{
        nullptr, rtcReleaseScene
    };

    // Builds one immutable collision scene while local geometry remains independently owned.
    explicit Scene(const std::vector<Triangle>& triangles) {
        // Failed allocation leaves the remaining owners empty.
        if (!device) {
            throw std::runtime_error("Embree device unavailable");
        }
        scene.reset(rtcNewScene(device.get()));
        std::unique_ptr<std::remove_pointer_t<RTCGeometry>, decltype(&rtcReleaseGeometry)> geometry{
            rtcNewGeometry(device.get(), RTC_GEOMETRY_TYPE_TRIANGLE), rtcReleaseGeometry
        };
        // Buffer setup requires both valid acceleration objects.
        if (!scene || !geometry) {
            throw std::runtime_error("Embree collision allocation failed");
        }
        auto* vertices = static_cast<Point*>(rtcSetNewGeometryBuffer(
            geometry.get(),
            RTC_BUFFER_TYPE_VERTEX,
            0,
            RTC_FORMAT_FLOAT3,
            sizeof(Point),
            triangles.size() * 3
        ));
        auto* indices = static_cast<std::array<unsigned, 3>*>(rtcSetNewGeometryBuffer(
            geometry.get(),
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT3,
            sizeof(std::array<unsigned, 3>),
            triangles.size()
        ));
        // Out-of-memory failures must not become writes through null buffer pointers.
        if (!vertices || !indices) {
            throw std::runtime_error("Embree collision buffer allocation failed");
        }
        for (unsigned i = 0; i < triangles.size(); ++i) {
            for (unsigned k = 0; k < 3; ++k) {
                vertices[i * 3 + k] = triangles[i].vertices[k];
            }
            indices[i] = {i * 3, i * 3 + 1, i * 3 + 2};
        }
        rtcCommitGeometry(geometry.get());
        rtcAttachGeometry(scene.get(), geometry.get());
        rtcCommitScene(scene.get());
        // The owners also release resources when Embree rejects the build.
        if (rtcGetDeviceError(device.get()) != RTC_ERROR_NONE) {
            throw std::runtime_error("Embree collision build failed");
        }
    }

    // Finds the nearest two-sided collision hit along a finite or unbounded ray.
    RTCRayHit hit(Point p, Point d, float far) const {
        RTCRayHit h{};
        h.ray.org_x = p[0];
        h.ray.org_y = p[1];
        h.ray.org_z = p[2];
        h.ray.dir_x = d[0];
        h.ray.dir_y = d[1];
        h.ray.dir_z = d[2];
        h.ray.tnear = 0.015f;
        h.ray.tfar = far;
        h.ray.mask = ~0u;
        h.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcIntersect1(scene.get(), &h);
        return h;
    }

    bool clear(Point a, Point b) const {
        auto d = sub(b, a);
        const float distance = length(d);
        return distance < 0.02f
               || hit(a, mul(d, 1 / distance), distance).hit.geomID == RTC_INVALID_GEOMETRY_ID;
    }
};

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
}  // namespace

// ---- Sampled floor inference ----

Point local(const Transition& t, Point p) {
    Point out{};

    for (unsigned i = 0; i < 3; ++i) {
        out[i] = p[0] * t.inverse[i] + p[1] * t.inverse[4 + i] + p[2] * t.inverse[8 + i]
                 + t.inverse[12 + i];
    }

    return out;
}

Point world(const Transition& t, Point p) {
    Point a{t.inverse[0], t.inverse[4], t.inverse[8]}, b{t.inverse[1], t.inverse[5], t.inverse[9]},
        c{t.inverse[2], t.inverse[6], t.inverse[10]};
    const float determinant = dot(a, cross(b, c));

    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f) {
        throw std::runtime_error("singular transition transform");
    }

    p = sub(p, {t.inverse[12], t.inverse[13], t.inverse[14]});
    return mul(
        add(add(mul(cross(b, c), p[0]), mul(cross(c, a), p[1])), mul(cross(a, b), p[2])),
        1 / determinant
    );
}

bool crosses_transition(const Transition& t, Point a, Point b) {
    a = local(t, a);
    b = local(t, b);

    if ((a[0] < 0) == (b[0] < 0) || std::abs(a[0] - b[0]) < 1e-8f) {
        return false;
    }

    const float f = a[0] / (a[0] - b[0]);
    const auto p = add(a, mul(sub(b, a), f));
    return std::abs(p[1]) <= 1.02f && std::abs(p[2]) <= 1.02f;
}

namespace {
// Collision extent and lattice dimensions validated before any sampling allocation.
struct SamplingDomain {
    // Sentinel bounds retain the established sampling extent for exported levels.
    static constexpr float kExtent = 1e30f;
    Point low{kExtent, kExtent, kExtent}, high{-kExtent, -kExtent, -kExtent};
    unsigned nx = 0, ny = 0;
};

// Rejects invalid lengths, nonfinite ray settings and unaddressable collision indices.
void validate_sampling(const std::vector<Triangle>& triangles, const Settings& settings) {
    const std::array values =
        {settings.spacing,
         settings.height,
         settings.radius,
         settings.step,
         settings.slope_cosine,
         settings.sky_distance};
    // Nonfinite values must not bypass ordered range comparisons.
    if (triangles.empty() || triangles.size() > std::numeric_limits<unsigned>::max() / 3
        || std::any_of(
            values.begin(), values.end(), [](float value) { return !std::isfinite(value); }
        )
        || settings.spacing < .25f || settings.height <= 0 || settings.radius < 0
        || settings.step < 0 || settings.slope_cosine <= 0 || settings.slope_cosine > 1
        || settings.sky_distance <= 0) {
        throw std::runtime_error("invalid inference input/settings");
    }
}

// Bounds all floor columns before narrowing their dimensions or allocating worker data.
SamplingDomain sampling_domain(const std::vector<Triangle>& triangles, const Settings& settings) {
    SamplingDomain domain;
    for (const auto& triangle : triangles) {
        for (const auto& point : triangle.vertices) {
            // A nonfinite vertex cannot enter Embree or lattice arithmetic.
            if (std::any_of(point.begin(), point.end(), [](float value) {
                    return !std::isfinite(value);
                })) {
                throw std::runtime_error("nonfinite collision vertex");
            }
            for (unsigned axis = 0; axis < 3; ++axis) {
                domain.low[axis] = std::min(domain.low[axis], point[axis]);
                domain.high[axis] = std::max(domain.high[axis], point[axis]);
            }
        }
    }
    const double nx =
        std::ceil((static_cast<double>(domain.high[0]) - domain.low[0]) / settings.spacing) + 1;
    const double ny =
        std::ceil((static_cast<double>(domain.high[1]) - domain.low[1]) / settings.spacing) + 1;
    // Check the product in double before converting either axis to an allocation index.
    if (nx > settings.max_cells || ny > settings.max_cells || nx * ny > settings.max_cells) {
        throw std::runtime_error("inference XY budget exceeded");
    }
    domain.nx = static_cast<unsigned>(nx);
    domain.ny = static_cast<unsigned>(ny);
    return domain;
}

// Tests a sampled body column and its four radius offsets without authored mesh normals.
bool body_clear(const Scene& scene, Point floor, const Settings& settings) {
    constexpr float kFloorMargin = .06f;
    const Point foot{floor[0], floor[1], floor[2] + kFloorMargin};
    const Point head{floor[0], floor[1], floor[2] + settings.height};
    // Central clearance is required before the more expensive radius samples.
    if (!scene.clear(foot, head)) {
        return false;
    }
    for (const Point offset :
         {Point{settings.radius, 0, 0},
          Point{-settings.radius, 0, 0},
          Point{0, settings.radius, 0},
          Point{0, -settings.radius, 0}}) {
        const Point centre{floor[0], floor[1], floor[2] + settings.height * .5f};
        // A nearby wall or low ceiling makes this support unusable.
        if (!scene.clear(centre, add(centre, offset))
            || !scene.clear(add(foot, offset), add(head, offset))) {
            return false;
        }
    }
    return true;
}

// Estimates finite-distance sky visibility with the existing fixed upper-hemisphere sequence.
float sample_sky(const Scene& scene, Point floor, const Settings& settings) {
    constexpr unsigned kSkySamples = 16;
    constexpr float kGoldenAngle = 2.39996323f;
    unsigned sky = 0;
    const Point eye{floor[0], floor[1], floor[2] + settings.height * .6f};
    for (unsigned ray = 0; ray < kSkySamples; ++ray) {
        const float dz = (static_cast<float>(ray) + .5f) / kSkySamples;
        const float angle = static_cast<float>(ray) * kGoldenAngle;
        const float radius = std::sqrt(1 - dz * dz);
        sky += scene
                   .hit(
                       eye,
                       {radius * std::cos(angle), radius * std::sin(angle), dz},
                       settings.sky_distance
                   )
                   .hit.geomID
               == RTC_INVALID_GEOMETRY_ID;
    }
    return static_cast<float>(sky) / kSkySamples;
}

// Owns sampled floors, adjacency and component results for one deterministic inference pass.
class FloorSampler {
public:
    // Collision, authored boundaries and settings remain borrowed until run completes.
    FloorSampler(
        const Scene& scene,
        const std::vector<Triangle>& triangles,
        const Settings& settings,
        SamplingDomain domain
    )
        : m_scene(scene),
          m_triangles(triangles),
          m_settings(settings),
          m_domain(domain),
          m_columns(static_cast<std::size_t>(domain.nx) * domain.ny) {}

    // Preserves row, column, layer and neighbour order from the original inference traversal.
    Result run(const std::vector<Transition>& transitions) {
        for (unsigned y = 0; y < m_domain.ny; ++y) {
            for (unsigned x = 0; x < m_domain.nx; ++x) {
                sample_column(x, y);
            }
        }
        m_edges.resize(m_result.cells.size());
        for (unsigned y = 0; y < m_domain.ny; ++y) {
            for (unsigned x = 0; x < m_domain.nx; ++x) {
                for (const auto cell : m_columns[x + m_domain.nx * y]) {
                    connect_cell(cell, x, y, transitions);
                }
            }
        }
        for (unsigned cell = 0; cell < m_result.cells.size(); ++cell) {
            // Each connected component is emitted once, in first-cell order.
            if (m_result.cells[cell].component == ~0u) {
                component(cell);
            }
        }
        return std::move(m_result);
    }

private:
    // Finds supported surfaces from top to bottom while keeping the existing bounded layer count.
    void sample_column(unsigned x, unsigned y) {
        constexpr unsigned kMaximumLayers = 256;
        constexpr float kLayerMargin = .04f;
        Point top{
            m_domain.low[0] + x * m_settings.spacing,
            m_domain.low[1] + y * m_settings.spacing,
            m_domain.high[2] + 1
        };
        for (unsigned layer = 0; layer < kMaximumLayers && top[2] > m_domain.low[2]; ++layer) {
            const auto hit = m_scene.hit(top, {0, 0, -1}, top[2] - m_domain.low[2] + 1);
            // No further collision ends this column.
            if (hit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                break;
            }
            const float z = top[2] - hit.ray.tfar;
            top[2] = z - kLayerMargin;
            const auto& triangle = m_triangles[hit.hit.primID];
            const Point normal{hit.hit.Ng_x, hit.hit.Ng_y, hit.hit.Ng_z};
            // Water and steep surfaces cannot support a receiver.
            if (!triangle.support
                || std::abs(normal[2]) < m_settings.slope_cosine * length(normal)) {
                continue;
            }
            const Point floor{top[0], top[1], z};
            // Receiver clearance is checked before it can consume the sample budget.
            if (!body_clear(m_scene, floor, m_settings)) {
                continue;
            }
            // Bound retained floor samples independently of the XY allocation.
            if (m_result.cells.size() >= m_settings.max_cells) {
                throw std::runtime_error("inference sample budget exceeded");
            }
            m_columns[x + m_domain.nx * y].push_back(static_cast<unsigned>(m_result.cells.size()));
            m_result.cells.push_back({floor, ~0u, sample_sky(m_scene, floor, m_settings)});
        }
    }

    // Tests a full body-width step sweep above the higher support.
    bool traversable(unsigned a, unsigned b, unsigned dx) const {
        for (const float height : {.2f, m_settings.height * .5f, m_settings.height}) {
            auto p = m_result.cells[a].position, q = m_result.cells[b].position;
            p[2] += height;
            q[2] += height;
            p[2] = q[2] = std::max(p[2], q[2]);
            const Point lateral =
                dx ? Point{0, m_settings.radius, 0} : Point{m_settings.radius, 0, 0};
            // Reject obstructed centre or side sweeps before trying the next height.
            if (!m_scene.clear(p, q) || !m_scene.clear(add(p, lateral), add(q, lateral))
                || !m_scene.clear(sub(p, lateral), sub(q, lateral))) {
                return false;
            }
        }
        return true;
    }

    // Keeps authored transition cuts separate from physical walkability checks.
    void connect_pair(
        unsigned a, unsigned b, unsigned dx, const std::vector<Transition>& transitions
    ) {
        auto p = m_result.cells[a].position, q = m_result.cells[b].position;
        // Different storeys do not share a walkable edge.
        if (std::abs(p[2] - q[2]) > m_settings.step) {
            return;
        }
        p[2] += m_settings.height * .5f;
        q[2] += m_settings.height * .5f;
        // Authored boxes are evaluated at body height, as their bounds need not reach the floor.
        if (std::any_of(transitions.begin(), transitions.end(), [&](const Transition& transition) {
                return crosses_transition(transition, p, q);
            })) {
            ++m_result.blocked_portal_edges;
            return;
        }
        // Symmetric edges retain the original neighbour insertion order.
        if (traversable(a, b, dx)) {
            m_edges[a].push_back(b);
            m_edges[b].push_back(a);
        }
    }

    // Visits only positive X/Y neighbours to avoid evaluating each undirected edge twice.
    void connect_cell(
        unsigned cell, unsigned x, unsigned y, const std::vector<Transition>& transitions
    ) {
        for (const auto [dx, dy] : {std::pair{1u, 0u}, std::pair{0u, 1u}}) {
            // Grid-edge columns have no neighbour on this axis.
            if (x + dx >= m_domain.nx || y + dy >= m_domain.ny) {
                continue;
            }
            for (const auto other : m_columns[x + dx + m_domain.nx * (y + dy)]) {
                connect_pair(cell, other, dx, transitions);
            }
        }
    }

    // Floods one component with stable cell ordering and its mean sky visibility.
    void component(unsigned first) {
        const auto id = static_cast<unsigned>(m_result.regions.size());
        m_result.regions.push_back({});
        std::queue<unsigned> pending;
        pending.push(first);
        m_result.cells[first].component = id;
        auto& region = m_result.regions[id];
        while (!pending.empty()) {
            const auto current = pending.front();
            pending.pop();
            region.cells.push_back(current);
            region.sky += m_result.cells[current].sky;
            for (const auto next : m_edges[current]) {
                // Visit every supported cell once, including isolated floor patches.
                if (m_result.cells[next].component == ~0u) {
                    m_result.cells[next].component = id;
                    pending.push(next);
                }
            }
        }
        region.sky /= static_cast<float>(region.cells.size());
    }

    const Scene& m_scene;  // Borrowed collision owner for all receiver and step queries.
    const std::vector<Triangle>& m_triangles;  // Source support flags indexed by collision hits.
    const Settings& m_settings;                // Validated lengths and budgets.
    SamplingDomain m_domain;                   // Bounded XY lattice and collision height range.
    Result m_result;  // Floor cells and components in stable traversal order.
    std::vector<std::vector<unsigned>> m_columns, m_edges;  // Layer lookup and symmetric adjacency.
};
}  // namespace

Result infer(
    const std::vector<Triangle>& triangles,
    const std::vector<Transition>& transitions,
    const Settings& settings
) {
    validate_sampling(triangles, settings);
    const auto domain = sampling_domain(triangles, settings);
    for (const auto& transition : transitions) {
        // A malformed transform must fail before any seed or air-grid work.
        if (std::any_of(transition.inverse.begin(), transition.inverse.end(), [](float value) {
                return !std::isfinite(value);
            })) {
            throw std::runtime_error("nonfinite transition transform");
        }
        (void)world(transition, {0, 0, 0});
    }
    Scene scene(triangles);
    auto result = FloorSampler(scene, triangles, settings, domain).run(transitions);
    assign_transition_labels(scene, transitions, settings, result);
    return result;
}
}  // namespace openrac::lighting::interiors
