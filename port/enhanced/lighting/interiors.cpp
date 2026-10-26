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

#include "enhanced/lighting/interior_inference_internal.h"

#include <embree4/rtcore.h>

namespace openrac::lighting::interiors {
using namespace detail;

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
