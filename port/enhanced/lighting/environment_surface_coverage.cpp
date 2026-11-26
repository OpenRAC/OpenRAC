// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Surface receivers, independent of photograph selection and GI allocation.
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "common/log.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/environment_bake.h"

namespace openrac::lighting {
namespace {
using namespace bake_detail;
using Key = std::array<int, 4>;

struct Hash {
    size_t operator()(const Key& k) const {
        size_t h = 1469598103934665603ull;
        for (auto v : k) {
            h = (h ^ size_t(unsigned(v))) * 1099511628211ull;
        }
        return h;
    }
};

float length(Float3 p) {
    return std::sqrt(dot(p, p));
}

Float3 subtract(Float3 a, Float3 b) {
    return add(a, mul(b, -1));
}

Float3 normalise(Float3 p) {
    return mul(p, 1 / std::max(.000001f, length(p)));
}

Float3 cross3(Float3 a, Float3 b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

float fade(const EnvironmentInfluence& v, Float3 p) {
    p = v.frame.to_local(p);
    float edge = 1;
    for (unsigned k = 0; k < 3; ++k) {
        edge = std::min({edge, p[k] - v.minimum[k], v.maximum[k] - p[k]});
    }
    const float t = std::clamp(edge / .75f, 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

struct Cloud {
    Float3 p{}, n{}, lo{}, hi{};
    double area = 0;
    unsigned owner = ~0u;
};

struct Anchor {
    Float3 p;
    unsigned owner;
    bool underwater;
    std::optional<float> water;
};

std::array<Float3, 8> corners(Float3 lo, Float3 hi) {
    std::array<Float3, 8> p;
    for (unsigned i = 0; i < 8; ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            p[i][k] = i & (1 << k) ? hi[k] : lo[k];
        }
    }
    return p;
}

float coverage(const EnvironmentInfluence& v, const Cloud& c) {
    float result = 1;
    for (auto p : corners(c.lo, c.hi)) {
        result = std::min(result, fade(v, p));
    }
    return result;
}

bool clear(const Scene& scene, Float3 a, Float3 b, float bias) {
    const auto delta = subtract(b, a);
    const float d = length(delta);
    return d <= bias * 2 || !scene.occluded(a, mul(delta, 1 / d), bias, std::max(bias, d - bias));
}

struct Samples {
    std::vector<Cloud> cloud;
    std::vector<unsigned> sorted;
};
}  // namespace

EnvironmentPlacementReport::SurfaceCoverage measure_environment_surface_coverage(
    const BakeScene& render,
    const BakeScene& geometry,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    std::span<const EnvironmentInfluence> influences,
    std::span<const EnvironmentProbe> captures,
    const BakeExecution& execution
) {
    using namespace bake_detail;
    EnvironmentPlacementReport::SurfaceCoverage report;
    if (influences.empty() || floors.empty()) {
        return report;
    }
    if (influences.size() > kEnvironmentProbeLimit || !std::isfinite(settings.ray_bias)
        || settings.ray_bias <= 0 || !std::isfinite(settings.traversal.player_height)
        || settings.traversal.player_height <= 0) {
        throw std::runtime_error("invalid reflection surface coverage inputs");
    }
    const auto audit_start = std::chrono::steady_clock::now();
    const auto original = influences;
    Samples samples;
    const Scene collision(geometry, {}, {.prepare_emission_averages = false});
    const Scene visible(render, {}, {.prepare_emission_averages = false});
    std::vector<Anchor> anchors;
    std::unordered_map<Key, std::vector<unsigned>, Hash> grid;
    const auto bin = [](Float3 p) {
        return Key{
            int(std::floor(p[0] / 8)), int(std::floor(p[1] / 8)), int(std::floor(p[2] / 8)), 0
        };
    };
    float maximum_z = -INFINITY;
    for (const auto& t : geometry.triangles) {
        for (auto p : t.vertices) {
            maximum_z = std::max(maximum_z, p[2]);
        }
    }
    for (const auto& f : floors) {
        auto p = f.position;
        p[2] += .1f;
        const auto roof =
            collision.trace(p, {0, 0, 1}, settings.ray_bias, std::max(30.f, maximum_z - p[2] + 1));
        float top = roof.hit.geomID != RTC_INVALID_GEOMETRY_ID
                        ? p[2] + roof.ray.tfar
                        : f.position[2] + settings.traversal.player_height * 12;
        if (f.underwater && f.water_height) {
            top = std::min(top, *f.water_height);
        }
        auto body = f.position;
        body[2] = std::min(f.position[2] + 1.25f, top - .25f);
        unsigned owner = ~0u;
        float best = 0;
        for (unsigned i = 0; i < original.size(); ++i) {
            const auto centre = original[i].capture < captures.size()
                                    ? captures[original[i].capture].position
                                    : original[i].frame.to_world(
                                          mul(add(original[i].minimum, original[i].maximum), .5f)
                                      );
            const auto delta = subtract(body, centre);
            const float score = fade(original[i], body) / (1 + dot(delta, delta) / (24 * 24));
            if (score > best) {
                best = score;
                owner = i;
            }
        }
        if (owner == ~0u) {
            float nearest = INFINITY;
            for (unsigned i = 0; i < original.size(); ++i) {
                const auto centre =
                    original[i].capture < captures.size()
                        ? captures[original[i].capture].position
                        : original[i].frame.to_world(
                              mul(add(original[i].minimum, original[i].maximum), .5f)
                          );
                const auto d = subtract(body, centre);
                if (dot(d, d) < nearest) {
                    nearest = dot(d, d);
                    owner = i;
                }
            }
        }
        for (float z :
             {std::min(f.position[2] + .15f, top - .25f),
              body[2],
              std::min(f.position[2] + 2.5f, top - .25f),
              (body[2] + top - .5f) * .5f,
              top - .5f}) {
            p = body;
            p[2] = z;
            if (!clear(collision, body, p, settings.ray_bias)
                || !clear(visible, body, p, settings.ray_bias)) {
                continue;
            }
            grid[bin(p)].push_back(unsigned(anchors.size()));
            anchors.push_back({p, owner, f.underwater, f.water_height});
        }
    }
    std::unordered_map<Key, unsigned, Hash> sample_index;
    const auto add_sample = [&](Float3 p, Float3 n, double area) {
        const auto cell = bin(p);
        bool nearby = false;
        for (int reach = 1; reach <= 3 && !nearby; ++reach) {
            for (int x = -reach; x <= reach && !nearby; ++x) {
                for (int y = -reach; y <= reach && !nearby; ++y) {
                    for (int z = -reach; z <= reach && !nearby; ++z) {
                        if (reach > 1
                            && std::max({std::abs(x), std::abs(y), std::abs(z)}) != reach) {
                            continue;
                        }
                        auto found = grid.find({cell[0] + x, cell[1] + y, cell[2] + z, 0});
                        if (found == grid.end()) {
                            continue;
                        }
                        for (auto i : found->second) {
                            const auto d = subtract(anchors[i].p, p);
                            if (dot(d, n) > .35f && dot(d, d) <= 32 * 32) {
                                nearby = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!nearby) {
            return;
        }

        unsigned axis = 0;
        for (unsigned k = 1; k < 3; ++k) {
            if (std::abs(n[k]) > std::abs(n[axis])) {
                axis = k;
            }
        }
        const int direction =
            int(axis * 8) + (n[0] < 0 ? 1 : 0) + (n[1] < 0 ? 2 : 0) + (n[2] < 0 ? 4 : 0);
        Key key{int(std::floor(p[0])), int(std::floor(p[1])), int(std::floor(p[2])), direction};
        const auto u = normalise(
                       cross3(n, std::abs(n[2]) < .9f ? Float3{0, 0, 1} : Float3{0, 1, 0})
                   ),
                   v = cross3(n, u);
        auto lo = add(p, mul(n, .5f)), hi = lo;
        // Conservative normal/view-biased selection footprint, including grazing views.
        for (float a : {-1.f, 1.f}) {
            for (float b : {-1.f, 1.f}) {
                auto q = add(add(p, mul(n, .35f)), add(mul(u, a * .36f), mul(v, b * .36f)));
                for (unsigned k = 0; k < 3; ++k) {
                    lo[k] = std::min(lo[k], q[k]);
                    hi[k] = std::max(hi[k], q[k]);
                }
            }
        }
        const auto [it, inserted] = sample_index.try_emplace(key, unsigned(samples.cloud.size()));
        if (inserted) {
            if (samples.cloud.size() >= 4000000) {
                throw std::runtime_error("reflection surface sample budget exceeded");
            }
            samples.cloud.push_back({p, n, lo, hi, area});
        } else {
            auto& c = samples.cloud[it->second];
            c.area += area;
            for (unsigned k = 0; k < 3; ++k) {
                c.lo[k] = std::min(c.lo[k], lo[k]);
                c.hi[k] = std::max(c.hi[k], hi[k]);
            }
        }
    };

    struct Vertex {
        Float3 p;
        Float2 bary;
    };

    struct Triangle {
        std::array<Vertex, 3> v;
        unsigned depth;
    };

    std::vector<Triangle> pending;
    for (const auto& t : render.triangles) {
        pending.push_back(
            {{{{t.vertices[0], {0, 0}}, {t.vertices[1], {1, 0}}, {t.vertices[2], {0, 1}}}}, 0}
        );
        while (!pending.empty()) {
            auto part = pending.back();
            pending.pop_back();
            const auto normal =
                cross3(subtract(part.v[1].p, part.v[0].p), subtract(part.v[2].p, part.v[0].p));
            const float twice_area = length(normal);
            if (twice_area < 1e-7f) {
                continue;
            }
            unsigned edge = 0;
            float longest = 0;
            for (unsigned k = 0; k < 3; ++k) {
                const float d = length(subtract(part.v[k].p, part.v[(k + 1) % 3].p));
                if (d > longest) {
                    longest = d;
                    edge = k;
                }
            }
            if (longest > 2 && part.depth < 16) {
                const auto a = part.v[edge], b = part.v[(edge + 1) % 3], c = part.v[(edge + 2) % 3];
                Vertex mid{
                    mul(add(a.p, b.p), .5f),
                    {(a.bary[0] + b.bary[0]) * .5f, (a.bary[1] + b.bary[1]) * .5f}
                };
                pending.push_back({{{a, mid, c}}, part.depth + 1});
                pending.push_back({{{mid, b, c}}, part.depth + 1});
                continue;
            }
            const auto n = mul(normal, 1 / twice_area);
            Vertex centre{
                mul(add(add(part.v[0].p, part.v[1].p), part.v[2].p), 1.f / 3),
                {(part.v[0].bary[0] + part.v[1].bary[0] + part.v[2].bary[0]) / 3,
                 (part.v[0].bary[1] + part.v[1].bary[1] + part.v[2].bary[1]) / 3}
            };
            for (unsigned j = 0; j < 4; ++j) {
                const auto sample = j == 3 ? centre : part.v[j];
                if (t.texture >= 0
                    && bake_material(render, t, sample.bary[0], sample.bary[1])[3]
                           < t.alpha_cutoff) {
                    continue;
                }
                const double area = double(twice_area) * .5 * (j == 3 ? .5 : 1. / 6);
                add_sample(sample.p, n, area);
                add_sample(sample.p, mul(n, -1), area);
            }
        }
    }
    for (const auto& f : floors) {
        if (f.swimming_surface) {
            add_sample(f.position, {0, 0, 1}, 4);
            add_sample(f.position, {0, 0, -1}, 4);
        }
    }
    log::info(
        "reflection coverage: {} surface candidates, {} reachable witnesses",
        samples.cloud.size(),
        anchors.size()
    );
    // Each side needs a collision-clear witness in reachable air/water. Roof backs,
    // sealed cavities and distant background surfaces cannot claim a local probe.
    // Precompute the same search shells for every surface point.
    std::array<std::vector<Key>, 3> shells;
    for (int reach = 1; reach <= 3; ++reach) {
        for (int x = -reach; x <= reach; ++x) {
            for (int y = -reach; y <= reach; ++y) {
                for (int z = -reach; z <= reach; ++z) {
                    if (reach == 1 || std::max({std::abs(x), std::abs(y), std::abs(z)}) == reach) {
                        shells[reach - 1].push_back({x, y, z, 0});
                    }
                }
            }
        }
    }
    const auto witness_start = std::chrono::steady_clock::now();
    const auto& witness_grid = std::as_const(grid);
    const auto find_witness = [&](Cloud& c) {
        const auto target = add(c.p, mul(c.n, .35f));
        const auto key = bin(target);
        for (const auto& shell : shells) {
            // Keep exactly the original closest candidates, ordered by distance
            // then anchor ID. Water and visibility still filter after selection.
            using Candidate = std::pair<float, unsigned>;
            std::array<Candidate, 16> nearest;
            auto end = nearest.begin();
            for (const auto& offset : shell) {
                const auto found = witness_grid.find(
                    {key[0] + offset[0], key[1] + offset[1], key[2] + offset[2], 0}
                );
                if (found == witness_grid.end()) {
                    continue;
                }
                for (auto i : found->second) {
                    const auto delta = subtract(anchors[i].p, target);
                    const float squared = dot(delta, delta);
                    if (dot(delta, c.n) <= 0 || squared > 32 * 32) {
                        continue;
                    }
                    const Candidate candidate{squared, i};
                    if (end != nearest.end()) {
                        *end++ = candidate;
                        std::push_heap(nearest.begin(), end);
                    } else if (candidate < nearest.front()) {
                        std::pop_heap(nearest.begin(), end);
                        *(end - 1) = candidate;
                        std::push_heap(nearest.begin(), end);
                    }
                }
            }
            std::sort_heap(nearest.begin(), end);
            for (auto entry = nearest.begin(); entry != end; ++entry) {
                const auto& a = anchors[entry->second];
                if (a.water
                    && ((a.underwater && target[2] > *a.water)
                        || (!a.underwater && target[2] < *a.water))) {
                    continue;
                }
                if (clear(collision, a.p, target, settings.ray_bias)
                    && clear(visible, a.p, target, settings.ray_bias)) {
                    c.owner = a.owner;
                    if (c.owner != ~0u) {
                        return;
                    }
                    break;
                }
            }
        }
    };
    constexpr std::size_t chunk_size = 2048;
    parallel_bake_indices(
        (samples.cloud.size() + chunk_size - 1) / chunk_size, execution, [&](std::size_t chunk) {
            const auto begin = chunk * chunk_size;
            const auto end = std::min(begin + chunk_size, samples.cloud.size());
            for (auto i = begin; i < end; ++i) {
                find_witness(samples.cloud[i]);
            }
            collision.check_error();
            visible.check_error();
        }
    );
    // Preserve sample order and the original sequence of floating-point sums.
    std::erase_if(samples.cloud, [](const Cloud& c) { return c.owner == ~0u; });
    const auto witness_end = std::chrono::steady_clock::now();
    samples.sorted.resize(samples.cloud.size());
    std::iota(samples.sorted.begin(), samples.sorted.end(), 0u);
    std::stable_sort(samples.sorted.begin(), samples.sorted.end(), [&](unsigned a, unsigned b) {
        return samples.cloud[a].p[0] < samples.cloud[b].p[0];
    });
    report.parents.resize(influences.size());
    std::iota(report.parents.begin(), report.parents.end(), 0u);
    std::vector<float> confidence(samples.cloud.size());
    std::vector<unsigned> overlap_counts(samples.cloud.size());
    unsigned words = 1;
    for (const auto& box : original) {
        words = std::max(words, box.capture / 64 + 1);
    }
    std::vector<std::uint64_t> photographs(samples.cloud.size() * words);

    for (const auto& box : influences) {
        Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
        for (auto p : corners(box.minimum, box.maximum)) {
            p = box.frame.to_world(p);
            for (unsigned k = 0; k < 3; ++k) {
                lo[k] = std::min(lo[k], p[k] - 1);
                hi[k] = std::max(hi[k], p[k] + 1);
            }
        }
        auto first = std::lower_bound(
            samples.sorted.begin(), samples.sorted.end(), lo[0], [&](unsigned j, float x) {
                return samples.cloud[j].p[0] < x;
            }
        );
        for (auto it = first; it != samples.sorted.end() && samples.cloud[*it].p[0] <= hi[0];
             ++it) {
            const auto& c = samples.cloud[*it];
            if (c.p[1] < lo[1] || c.p[1] > hi[1] || c.p[2] < lo[2] || c.p[2] > hi[2]) {
                continue;
            }
            const float f = fade(box, add(c.p, mul(c.n, .4f)));
            overlap_counts[*it] += f >= .2f;
            if (f >= .2f) {
                photographs[size_t(*it) * words + box.capture / 64] |= std::uint64_t(1)
                                                                       << (box.capture % 64);
            }

            if (f > 0) {
                confidence[*it] = std::max(confidence[*it], coverage(box, c));
            }
        }
    }
    for (unsigned i = 0; i < samples.cloud.size(); ++i) {
        const auto& c = samples.cloud[i];
        const float strong = confidence[i];
        const unsigned overlaps = overlap_counts[i];
        if (c.area == 0) {
            continue;
        }
        unsigned distinct = 0;
        for (unsigned word = 0; word < words; ++word) {
            distinct += std::popcount(photographs[size_t(i) * words + word]);
        }
        report.distinct_overlaps += c.area * distinct;
        report.weighted_overlaps += c.area * overlaps;
        report.relevant_area += c.area;
        if (strong > .999f) {
            report.covered_area += c.area;
        } else if (strong >= .01f) {
            report.weak_area += c.area;
        } else {
            report.uncovered_area += c.area;
        }
        if (overlaps > 1) {
            report.overlap_area += c.area;
        }
        report.samples.push_back({c.p, c.n, float(c.area), strong, c.owner, overlaps, distinct});
    }
    // Compatibility fields describe the same immutable allocation.
    report.initial_covered_area = report.covered_area;
    report.initial_overlap_area = report.overlap_area;
    report.initial_distinct_overlaps = report.distinct_overlaps;
    report.initial_weighted_overlaps = report.weighted_overlaps;
    collision.check_error();
    visible.check_error();
    const auto audit_end = std::chrono::steady_clock::now();
    const auto seconds = [](auto elapsed) {
        return std::chrono::duration<double>(elapsed).count();
    };
    log::info(
        "reflection coverage audit: {:.3f}s total ({:.3f}s prepare, {:.3f}s witnesses, {:.3f}s "
        "measure)",
        seconds(audit_end - audit_start),
        seconds(witness_start - audit_start),
        seconds(witness_end - witness_start),
        seconds(audit_end - witness_end)
    );
    return report;
}
}  // namespace openrac::lighting
