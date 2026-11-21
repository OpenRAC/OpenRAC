// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/spatial_watershed.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <tuple>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_scene.h"
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
namespace {
using Bin = std::array<int, 2>;

float distance(Float3 a, Float3 b) {
    const auto d = add(a, mul(b, -1));
    return std::sqrt(dot(d, d));
}

bool clear(const Scene& scene, Float3 a, Float3 b, float bias) {
    const float length = distance(a, b);
    return length <= bias * 2
           || !scene.occluded(
               a, mul(add(b, mul(a, -1)), 1 / length), bias, std::max(bias, length - bias)
           );
}

void validate(const SpatialWatershedSettings& o) {
    if (!std::isfinite(o.horizontal_step) || o.horizontal_step <= 0
        || !std::isfinite(o.vertical_step) || o.vertical_step <= 0
        || !std::isfinite(o.air_height_scale) || o.air_height_scale <= 0
        || !std::isfinite(o.maximum_height) || o.maximum_height <= 0
        || !std::isfinite(o.maximum_clearance) || o.maximum_clearance <= 0
        || !std::isfinite(o.relative_prominence) || o.relative_prominence < 0
        || o.relative_prominence > 1 || o.maximum_cells == 0 || o.maximum_cells > 1000000
        || o.graph_memory_budget == 0) {
        throw std::runtime_error("invalid spatial watershed limits");
    }
}

// Reserve headroom for allocation growth, the incidence index, sorting and
// portal scratch. This budget applies to graph processing, not scene storage.
bool fits_graph_budget(
    std::size_t cells, std::size_t links, std::uint64_t budget, unsigned bytes_per_link
) {
    constexpr std::uint64_t bytes_per_cell = 160;
    const auto cell_bytes = std::uint64_t(cells) * bytes_per_cell;
    return cell_bytes <= budget && links <= (budget - cell_bytes) / bytes_per_link
           && links <= std::numeric_limits<unsigned>::max() / 2;
}

std::vector<unsigned> order_links_by_clearance(const SpatialWatershed& field) {
    constexpr unsigned digit_bits = 16;
    constexpr unsigned digit_count = 1u << digit_bits;
    constexpr unsigned digit_mask = digit_count - 1;
    const auto key = [](float clearance) {
        // Nonnegative IEEE floats sort by their bits. Reverse that order and
        // treat both signs of zero as equal, as in the floating-point comparison.
        return ~(clearance == 0 ? 0u : std::bit_cast<std::uint32_t>(clearance));
    };
    std::vector<unsigned> low(digit_count), high(digit_count);
    const auto link_count = field.link_count();
    for (unsigned link = 0; link < link_count; ++link) {
        const float clearance = field.clearance(link);
        if (!std::isfinite(clearance) || clearance < 0) {
            throw std::runtime_error("invalid spatial aperture");
        }
        const auto bits = key(clearance);
        ++low[bits & digit_mask];
        ++high[bits >> digit_bits];
    }
    for (auto* offsets : {&low, &high}) {
        unsigned next = 0;
        for (auto& count : *offsets) {
            const unsigned start = next;
            next += count;
            count = start;
        }
    }

    // Scatter in the original source-cell/incident order to preserve equal-key ties.
    std::vector<unsigned> by_low(link_count), sorted(link_count);
    for (unsigned cell = 0; cell < field.cells.size(); ++cell) {
        for (auto link : field.incident_links(cell)) {
            const auto edge = field.link(link);
            if (edge.a == cell) {
                by_low[low[key(field.clearance(edge)) & digit_mask]++] = link;
            }
        }
    }
    for (auto link : by_low) {
        sorted[high[key(field.clearance(link)) >> digit_bits]++] = link;
    }
    return sorted;
}
}  // namespace

void SpatialWatershed::prepare_link_storage(
    const SpatialWatershedSettings& options, std::size_t count
) {
    constexpr unsigned in_memory_bytes = 48, tiled_bytes = 28;
    if (!m_tiled_links
        && fits_graph_budget(cells.size(), count, options.graph_memory_budget, in_memory_bytes)) {
        return;
    }
    // The packed link index, incidence, radix scratch and portal processing stay resident.
    const auto cache_bytes = m_tiled_links
                                 ? m_tiled_links->cache_capacity()
                                 : TiledSpatialLinks::cache_size(options.graph_memory_budget);
    if (cache_bytes > options.graph_memory_budget
        || !fits_graph_budget(
            cells.size(), count, options.graph_memory_budget - cache_bytes, tiled_bytes
        )) {
        throw std::runtime_error("spatial watershed graph memory budget exceeded");
    }
    if (!m_tiled_links) {
        auto tiled =
            std::make_unique<TiledSpatialLinks>(double(options.horizontal_step) * 32, cache_bytes);
        for (const auto& edge : links) {
            if (edge.a >= cells.size()) {
                throw std::runtime_error("invalid spatial watershed link");
            }
            tiled->append(edge, cells[edge.a].position);
        }
        m_tiled_links = std::move(tiled);
        std::vector<SpatialLink>().swap(links);
    }
}

void SpatialWatershed::append_link(SpatialLink edge, const SpatialWatershedSettings& options) {
    prepare_link_storage(options, link_count() + 1);
    if (m_tiled_links) {
        if (edge.a >= cells.size()) {
            throw std::runtime_error("invalid spatial watershed link");
        }
        m_tiled_links->append(edge, cells[edge.a].position);
    } else {
        links.push_back(edge);
    }
}

void SpatialWatershed::index_links() {
    offsets.assign(cells.size() + 1, 0);
    const auto count = link_count();
    for (unsigned i = 0; i < count; ++i) {
        const auto edge = link(i);
        if (edge.a >= cells.size() || edge.b >= cells.size() || edge.a == edge.b) {
            throw std::runtime_error("invalid spatial watershed link");
        }
        ++offsets[edge.a + 1];
        ++offsets[edge.b + 1];
    }
    std::partial_sum(offsets.begin(), offsets.end(), offsets.begin());
    incident.resize(offsets.back());
    auto cursor = offsets;
    for (unsigned i = 0; i < count; ++i) {
        const auto edge = link(i);
        incident[cursor[edge.a]++] = i;
        incident[cursor[edge.b]++] = i;
    }
}

SpatialWatershed build_spatial_watershed(
    const Scene& scene,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const SpatialWatershedSettings& options,
    const BakeExecution& execution
) {
    validate(options);
    if (!std::isfinite(settings.ray_bias) || settings.ray_bias <= 0
        || !std::isfinite(settings.traversal.player_height) || settings.traversal.player_height <= 0
        || !std::isfinite(settings.traversal.player_radius)
        || settings.traversal.player_radius < 0) {
        throw std::runtime_error("invalid spatial watershed clearance settings");
    }
    SpatialWatershed field;
    std::vector<bool> reached(floors.size());
    std::queue<unsigned> roots;
    for (unsigned i = 0; i < floors.size(); ++i) {
        if (floors[i].anchor) {
            reached[i] = true;
            roots.push(i);
        }
    }
    while (!roots.empty()) {
        const auto i = roots.front();
        roots.pop();
        for (auto j : floors[i].neighbours) {
            if (j >= floors.size()) {
                throw std::runtime_error("invalid spatial source edge");
            }
            if (!reached[j]) {
                reached[j] = true;
                roots.push(j);
            }
        }
    }
    std::map<Bin, std::vector<unsigned>> columns;
    const auto bin = [&](Float3 p) {
        Bin result{};
        for (unsigned a = 0; a < 2; ++a) {
            const double v = std::floor(double(p[a]) / options.horizontal_step);
            if (!std::isfinite(v) || std::abs(v) > 10000000) {
                throw std::runtime_error("spatial watershed coordinate out of bounds");
            }
            result[a] = int(v);
        }
        return result;
    };
    const float margin = std::max(.05f, settings.ray_bias * 2);
    for (unsigned f = 0; f < floors.size(); ++f) {
        if (!reached[f]) {
            ++field.rejected_spans;
            continue;
        }
        const auto& source = floors[f];
        const auto key = bin(source.position);
        if (!std::isfinite(source.position[2])) {
            throw std::runtime_error("invalid spatial floor");
        }
        auto start = source.position;
        start[2] += margin;
        const auto roof = scene.trace(start, {0, 0, 1}, settings.ray_bias, options.maximum_height);
        const bool hit = roof.hit.geomID != RTC_INVALID_GEOMETRY_ID;
        float top =
            source.position[2]
            + std::min(
                options.maximum_height, settings.traversal.player_height * options.air_height_scale
            );
        if (source.underwater) {
            if (!source.water_height || !std::isfinite(*source.water_height)) {
                ++field.rejected_spans;
                continue;
            }
            top = std::min(
                *source.water_height - margin, source.position[2] + options.maximum_height
            );
        }
        const auto ceiling = hit ? std::optional<float>(start[2] + roof.ray.tfar) : std::nullopt;
        if (ceiling) {
            top = std::min(top, *ceiling - margin);
        }
        if (top <= start[2] + margin
            || (!source.underwater && ceiling
                && *ceiling - source.position[2] + margin < settings.traversal.player_height)) {
            ++field.rejected_spans;
            continue;
        }
        // Duplicate evidence at the same XY and overlapping free interval does
        // not create duplicate volumes. A real intervening slab prevents overlap.
        unsigned existing = ~0u;
        for (auto s : columns[key]) {
            const auto& span = field.spans[s];
            if (span.underwater == source.underwater
                && std::hypot(
                       span.floor[0] - source.position[0], span.floor[1] - source.position[1]
                   ) < .001f
                && start[2] <= span.top && top >= span.bottom) {
                existing = s;
                break;
            }
        }
        if (existing != ~0u) {
            auto& span = field.spans[existing];
            span.bottom = std::min(span.bottom, start[2]);
            span.top = std::max(span.top, top);
            span.floor[2] = std::min(span.floor[2], source.position[2]);
            span.sources.push_back(f);
        } else {
            if (field.spans.size() >= options.maximum_cells) {
                throw std::runtime_error("spatial watershed span budget exceeded");
            }
            columns[key].push_back(unsigned(field.spans.size()));
            SpatialSpan span;
            span.floor = source.position;
            span.bottom = start[2];
            span.top = top;
            span.ceiling = ceiling;
            span.underwater = source.underwater;
            span.capped = !ceiling || top < *ceiling - margin;
            span.sources.push_back(f);
            field.spans.push_back(std::move(span));
        }
    }
    scene.check_error();
    auto collision_execution = execution;
    std::unique_ptr<BakeThreadPool> local_pool;
    if (!collision_execution.pool && collision_execution.threads != 1) {
        local_pool = std::make_unique<BakeThreadPool>(collision_execution.threads);
        collision_execution.pool = local_pool.get();
    }
    constexpr std::size_t batch_size = 16 * 1024;
    const auto chunk_size =
        collision_execution.pool && collision_execution.pool->size() > 1 && execution.threads != 1
            ? std::size_t{1024}
            : batch_size;
    const auto query_batch = [&](std::size_t size, const auto& query) {
        parallel_bake_indices(
            (size + chunk_size - 1) / chunk_size, collision_execution, [&](std::size_t chunk) {
                const auto start = chunk * chunk_size;
                query(chunk, start, std::min(size, start + chunk_size));
                scene.check_error();
            }
        );
    };

    std::vector<SpatialCell> pending_cells;
    pending_cells.reserve(batch_size);
    const auto publish_cells = [&] {
        query_batch(pending_cells.size(), [&](std::size_t, std::size_t start, std::size_t end) {
            for (auto i = start; i < end; ++i) {
                auto& cell = pending_cells[i];
                cell.lateral_clearance = std::min(
                    options.maximum_clearance, room_wall_distance(scene, cell.position).distance
                );
            }
        });
        for (const auto& cell : pending_cells) {
            // Regions describe usable interior space, not thin slivers at walls.
            if (cell.lateral_clearance + .001f >= settings.traversal.player_radius) {
                field.spans[cell.span].cells.push_back(unsigned(field.cells.size()));
                field.cells.push_back(cell);
            }
        }
        pending_cells.clear();
    };
    for (unsigned s = 0; s < field.spans.size(); ++s) {
        auto& span = field.spans[s];
        const double layers = std::ceil(double(span.top - span.bottom) / options.vertical_step);
        if (!std::isfinite(layers) || layers > options.maximum_cells) {
            throw std::runtime_error("spatial watershed column budget exceeded");
        }
        const unsigned count = std::max(1u, unsigned(layers));
        // Resolve queued samples before testing a column that might exceed the cell budget.
        if (count > options.maximum_cells - field.cells.size() - pending_cells.size()) {
            publish_cells();
        }
        if (count > options.maximum_cells - field.cells.size()) {
            throw std::runtime_error("spatial watershed cell budget exceeded");
        }
        for (unsigned z = 0; z < count; ++z) {
            auto p = span.floor;
            p[2] = span.bottom + (span.top - span.bottom) * (float(z) + .5f) / float(count);
            pending_cells.push_back({p, 0, s});
            if (pending_cells.size() == batch_size) {
                publish_cells();
            }
        }
    }
    publish_cells();
    field.prepare_link_storage(options, 0);

    struct MidpointClearance {
        std::array<std::uint32_t, 3> position{};
        float radius = -1;
        float distance = 0;
    };

    // Nearby links can share an exact midpoint. Bound cache memory independently of graph size.
    constexpr unsigned cache_bits = 12;
    using MidpointCache = std::array<MidpointClearance, 1u << cache_bits>;
    std::vector<MidpointCache> midpoint_caches(batch_size / chunk_size);
    const auto midpoint_clearance = [&](MidpointCache& cache, Float3 midpoint, float radius) {
        const auto key = std::bit_cast<std::array<std::uint32_t, 3>>(midpoint);
        const std::uint64_t hash = std::uint64_t(key[0]) * 0x9e3779b185ebca87ull
                                   + std::uint64_t(key[1]) * 0xc2b2ae3d27d4eb4full
                                   + std::uint64_t(key[2]) * 0x165667b19e3779f9ull;
        auto& entry = cache[hash >> (64 - cache_bits)];
        // A wall hit gives the nearest distance at any radius. A miss only covers
        // searches up to the cached radius.
        if (entry.position == key && (radius <= entry.radius || entry.distance < entry.radius)) {
            return std::min(entry.distance, radius);
        }
        const float distance = room_wall_distance(scene, midpoint, radius).distance;
        entry = {key, radius, distance};
        return distance;
    };
    std::vector<SpatialLink> pending_links;
    pending_links.reserve(batch_size);
    const auto publish_links = [&](bool lateral) {
        query_batch(
            pending_links.size(), [&](std::size_t chunk, std::size_t start, std::size_t end) {
                auto& cache = midpoint_caches[chunk];
                for (auto i = start; i < end; ++i) {
                    auto& link = pending_links[i];
                    const auto& a = field.cells[link.a];
                    const auto& b = field.cells[link.b];
                    if (lateral) {
                        // The graph caps aperture at the tighter of the two usable endpoints.
                        const auto midpoint = mul(add(a.position, b.position), .5f);
                        const float radius = std::min(a.lateral_clearance, b.lateral_clearance);
                        link.aperture = midpoint_clearance(cache, midpoint, radius);
                    }
                    if (link.aperture + .001f < settings.traversal.player_radius
                        || !clear(scene, a.position, b.position, settings.ray_bias)) {
                        link.aperture = -1;
                    }
                }
            }
        );
        // Retire in generation order. Equal-aperture links rely on stable graph ordering.
        for (const auto& link : pending_links) {
            if (link.aperture >= 0) {
                field.append_link(
                    {std::min(link.a, link.b), std::max(link.a, link.b), link.aperture}, options
                );
            }
        }
        pending_links.clear();
    };
    const auto queue_link = [&](unsigned a, unsigned b, bool lateral) {
        pending_links.push_back({a, b, INFINITY});
        if (pending_links.size() == batch_size) {
            publish_links(lateral);
        }
    };
    for (const auto& span : field.spans) {
        for (unsigned i = 1; i < span.cells.size(); ++i) {
            const auto a = span.cells[i - 1], b = span.cells[i];
            if (distance(field.cells[a].position, field.cells[b].position)
                <= options.vertical_step * 1.01f) {
                queue_link(a, b, false);
            }
        }
    }
    publish_links(false);
    for (unsigned a = 0; a < field.spans.size(); ++a) {
        const auto& span = field.spans[a];
        const auto key = bin(span.floor);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                const auto found = columns.find({key[0] + x, key[1] + y});
                if (found == columns.end()) {
                    continue;
                }
                for (auto b : found->second) {
                    const auto& other = field.spans[b];
                    if (b <= a || span.underwater != other.underwater
                        || std::hypot(
                               span.floor[0] - other.floor[0], span.floor[1] - other.floor[1]
                           ) > options.horizontal_step * 1.415f
                        || std::max(span.bottom, other.bottom) >= std::min(span.top, other.top)) {
                        continue;
                    }
                    // Both spans are ordered by height, so the window only moves up.
                    // Compare differences rather than rounded height bounds to retain
                    // the same inclusive threshold and visit order as the full scan.
                    auto first = other.cells.begin(), last = first;
                    for (auto i : span.cells) {
                        const float height = field.cells[i].position[2];
                        while (first != other.cells.end()
                               && field.cells[*first].position[2] - height
                                      < -options.vertical_step) {
                            ++first;
                        }
                        last = std::max(last, first);
                        while (last != other.cells.end()
                               && field.cells[*last].position[2] - height
                                      <= options.vertical_step) {
                            ++last;
                        }
                        for (auto cell = first; cell != last; ++cell) {
                            queue_link(i, *cell, true);
                        }
                    }
                }
            }
        }
    }
    publish_links(true);
    segment_spatial_watershed(field, options);
    if (options.refine_visibility) {
        refine_spatial_visibility(scene, field, settings, options);
    }
    scene.check_error();
    return field;
}

SpatialWatershed build_spatial_watershed(
    const BakeScene& geometry,
    std::span<const EnvironmentFloorSample> floors,
    const BakeSettings& settings,
    const SpatialWatershedSettings& options,
    const BakeExecution& execution
) {
    Scene scene(geometry);
    return build_spatial_watershed(scene, floors, settings, options, execution);
}

void segment_spatial_watershed(SpatialWatershed& field, const SpatialWatershedSettings& options) {
    validate(options);
    const unsigned n = unsigned(field.cells.size());
    if (n > options.maximum_cells) {
        throw std::runtime_error("spatial watershed cell budget exceeded");
    }
    field.basins.clear();
    field.portals.clear();
    field.visibility_splits.clear();
    field.visibility_assessments.clear();
    std::vector<unsigned> parent(n), peak(n), merge(n, ~0u);
    std::vector<float> saddle(n);
    std::iota(parent.begin(), parent.end(), 0u);
    std::iota(peak.begin(), peak.end(), 0u);
    field.prepare_link_storage(options, field.link_count());
    field.index_links();
    for (const auto& cell : field.cells) {
        if (cell.span >= field.spans.size()) {
            throw std::runtime_error("invalid spatial span");
        }
    }
    const auto root = [&](unsigned v) {
        unsigned r = v;
        while (parent[r] != r) {
            r = parent[r];
        }
        while (parent[v] != v) {
            const auto next = parent[v];
            parent[v] = r;
            v = next;
        }
        return r;
    };
    // Filter edges by measured aperture. A doorway can be narrower than either
    // endpoint in a coarse heightfield; its actual saddle must not disappear.
    {
        const auto sorted = order_links_by_clearance(field);
        for (auto link : sorted) {
            const auto e = field.link(link);
            auto a = root(e.a), b = root(e.b);
            if (a == b) {
                continue;
            }
            auto p = peak[a], q = peak[b];
            if (field.cells[p].lateral_clearance < field.cells[q].lateral_clearance
                || (field.cells[p].lateral_clearance == field.cells[q].lateral_clearance
                    && p > q)) {
                std::swap(a, b);
                std::swap(p, q);
            }
            merge[q] = p;
            saddle[q] = field.clearance(e);
            parent[b] = a;
            peak[a] = p;
        }
    }
    field.raw_peaks = 0;
    std::map<unsigned, unsigned> components;
    std::vector<unsigned> seeds;
    for (unsigned i = 0; i < n; ++i) {
        const auto id = components.try_emplace(root(i), unsigned(components.size())).first;
        field.cells[i].component = id->second;
        const float prominence = field.cells[i].lateral_clearance - saddle[i];
        field.raw_peaks += merge[i] == ~0u || prominence > 1e-4f;
        if (merge[i] == ~0u || prominence > 1e-4f) {
            seeds.push_back(i);
        }
    }
    field.components = unsigned(components.size());

    const auto flood = [&](std::span<const unsigned> sources) {
        field.basins.clear();
        struct Visit {
            float capacity, distance;
            unsigned node, basin;
        };

        const auto less = [](const Visit& a, const Visit& b) {
            return std::tuple{a.capacity, -a.distance, ~a.basin, ~a.node}
                   < std::tuple{b.capacity, -b.distance, ~b.basin, ~b.node};
        };
        std::priority_queue<Visit, std::vector<Visit>, decltype(less)> pending(less);
        std::vector<float> capacity(n, -1), length(n, INFINITY);
        std::vector<unsigned> plateau;
        for (unsigned s = 0; s < sources.size(); ++s) {
            const auto i = sources[s];
            capacity[i] = field.cells[i].lateral_clearance;
            length[i] = 0;
            field.cells[i].basin = s;
            SpatialBasin basin;
            basin.centre = field.cells[i].position;
            basin.peak_clearance = capacity[i];
            basin.component = field.cells[i].component;
            basin.underwater = field.spans[field.cells[i].span].underwater;
            basin.minimum = {INFINITY, INFINITY, INFINITY};
            basin.maximum = {-INFINITY, -INFINITY, -INFINITY};
            field.basins.push_back(basin);
            // A flat regional maximum is one source region, not one arbitrary
            // first-indexed cell. Start the distance tie-break at its entire extent.
            // Only peak-level edges qualify: a lower saddle must keep peaks separate.
            plateau = {i};
            for (unsigned head = 0; head < plateau.size(); ++head) {
                const auto node = plateau[head];
                pending.push({capacity[node], 0, node, s});
                for (auto link : field.incident_links(node)) {
                    const auto edge = field.link(link);
                    const auto j = edge.other(node);
                    if (capacity[j] >= 0 || field.clearance(edge) < basin.peak_clearance) {
                        continue;
                    }
                    capacity[j] = basin.peak_clearance;
                    length[j] = 0;
                    field.cells[j].basin = s;
                    plateau.push_back(j);
                }
            }
        }
        // Widest-path flood, with geodesic ties, assigns flat plateaus spatially.
        while (!pending.empty()) {
            const auto v = pending.top();
            pending.pop();
            if (v.capacity != capacity[v.node] || v.distance != length[v.node]
                || v.basin != field.cells[v.node].basin) {
                continue;
            }
            for (auto link : field.incident_links(v.node)) {
                const auto edge = field.link(link);
                const auto j = edge.other(v.node);
                const float c = std::min(v.capacity, field.clearance(edge));
                const float d =
                    v.distance + distance(field.cells[v.node].position, field.cells[j].position);
                if (c > capacity[j] || (c == capacity[j] && d < length[j])) {
                    capacity[j] = c;
                    length[j] = d;
                    field.cells[j].basin = v.basin;
                    pending.push({c, d, j, v.basin});
                }
            }
        }
    };
    // Measure the unsimplified catchments before pruning peaks. A shallow but
    // sustained corridor has more support than a compact surface bump.
    const auto raw_seeds = seeds;
    flood(raw_seeds);

    struct Footprint {
        unsigned count = 0;
        double x = 0, y = 0, xx = 0, xy = 0, yy = 0;

        void add(double px, double py) {
            ++count;
            const double dx = px - x, dy = py - y;
            x += dx / count;
            y += dy / count;
            xx += dx * (px - x);
            xy += dx * (py - y);
            yy += dy * (py - y);
        }

        std::array<float, 2> axes() const {
            if (!count) {
                return {};
            }
            const double delta = std::hypot(xx - yy, 2 * xy);
            // Equivalent uniform-rectangle lengths, independent of world yaw.
            return {
                float(std::sqrt(std::max(0., 6 * (xx + yy + delta) / count))),
                float(std::sqrt(std::max(0., 6 * (xx + yy - delta) / count)))
            };
        }
    };

    std::vector<Footprint> support(raw_seeds.size());
    std::vector<std::tuple<unsigned, float, float>> footprint;
    footprint.reserve(n);
    for (const auto& cell : field.cells) {
        footprint.emplace_back(cell.basin, cell.position[0], cell.position[1]);
    }
    std::sort(footprint.begin(), footprint.end());
    footprint.erase(std::unique(footprint.begin(), footprint.end()), footprint.end());
    // Count each XY location once. More vertical bands or a taller analysis
    // envelope must not make a small feature look like a long corridor.
    for (const auto& [basin, x, y] : footprint) {
        support[basin].add(x, y);
    }
    seeds.clear();
    std::vector<unsigned> retained;
    std::vector<float> salience(raw_seeds.size());
    for (unsigned r = 0; r < raw_seeds.size(); ++r) {
        const auto i = raw_seeds[r];
        const auto [length, width] = support[r].axes();
        const float peak_clearance = field.cells[i].lateral_clearance;
        const float relative = (peak_clearance - saddle[i]) / std::max(peak_clearance, 1e-4f);
        // Require support beyond a full clearance diameter (and sample step)
        // before rewarding elongation. Compact bumps retain the old threshold.
        const float breadth = std::max({width, 2 * peak_clearance, options.horizontal_step});
        salience[r] = relative * std::max(1.f, length / breadth);
        if (merge[i] == ~0u || salience[r] > options.relative_prominence) {
            seeds.push_back(i);
            retained.push_back(r);
        }
    }
    flood(seeds);
    for (unsigned b = 0; b < seeds.size(); ++b) {
        auto& basin = field.basins[b];
        const auto r = retained[b];
        const auto [length, width] = support[r].axes();
        basin.seed_cell = seeds[b];
        basin.watershed_parent = b;
        basin.watershed_peak_clearance = basin.peak_clearance;
        basin.support_length = length;
        basin.support_width = width;
        basin.relative_prominence =
            (basin.peak_clearance - saddle[seeds[b]]) / std::max(basin.peak_clearance, 1e-4f);
        basin.salience = salience[r];
    }
    rebuild_spatial_basins(field);
}

void rebuild_spatial_basins(SpatialWatershed& field) {
    const auto n = unsigned(field.cells.size());
    field.portals.clear();

    struct Edge {
        unsigned a, b;
        float clearance;
    };

    std::vector<Edge> crossings;
    std::vector<std::vector<unsigned>> incident(n);
    for (auto& basin : field.basins) {
        basin.cells = 0;
        basin.peak_clearance = -1;
        basin.minimum = {INFINITY, INFINITY, INFINITY};
        basin.maximum = {-INFINITY, -INFINITY, -INFINITY};
    }
    for (unsigned i = 0; i < n; ++i) {
        const auto& cell = field.cells[i];
        auto& basin = field.basins[cell.basin];
        if (cell.lateral_clearance > basin.peak_clearance) {
            basin.peak_clearance = cell.lateral_clearance;
            basin.seed_cell = i;
        }
        for (auto link : field.incident_links(i)) {
            const auto connection = field.link(link);
            if (connection.a != i || cell.basin == field.cells[connection.b].basin) {
                continue;
            }
            Edge crossing{i, connection.b, field.clearance(link)};
            if (cell.basin > field.cells[crossing.b].basin) {
                std::swap(crossing.a, crossing.b);
            }
            incident[crossing.a].push_back(unsigned(crossings.size()));
            crossings.push_back(crossing);
        }
    }
    std::vector<Float3> means(field.basins.size());
    for (const auto& cell : field.cells) {
        auto& basin = field.basins[cell.basin];
        ++basin.cells;
        means[cell.basin] = add(means[cell.basin], cell.position);
        for (unsigned a = 0; a < 3; ++a) {
            basin.minimum[a] = std::min(basin.minimum[a], cell.position[a]);
            basin.maximum[a] = std::max(basin.maximum[a], cell.position[a]);
        }
    }
    // A basin pair can meet at several separate doors. Group crossing edges
    // only when their endpoints touch through the local graph on BOTH sides.
    // This uses collision-checked topology, never a world-distance weld.
    std::vector<unsigned> patch(crossings.size(), ~0u), queue;
    for (unsigned seed = 0; seed < crossings.size(); ++seed) {
        if (patch[seed] != ~0u) {
            continue;
        }
        const auto id = unsigned(field.portals.size());
        const auto& first = crossings[seed];
        SpatialPortal portal;
        portal.a = field.cells[first.a].basin;
        portal.b = field.cells[first.b].basin;
        queue = {seed};
        patch[seed] = id;
        Float3 mean{};
        for (unsigned head = 0; head < queue.size(); ++head) {
            const auto& e = crossings[queue[head]];
            portal.crossings.emplace_back(e.a, e.b);
            mean = add(mean, mul(add(field.cells[e.a].position, field.cells[e.b].position), .5f));
            portal.saddle_clearance = std::max(portal.saddle_clearance, e.clearance);
            const auto visit = [&](unsigned node) {
                for (const auto next : incident[node]) {
                    const auto& f = crossings[next];
                    if (patch[next] != ~0u || field.cells[f.b].basin != portal.b) {
                        continue;
                    }
                    const auto neighbours = field.neighbours(e.b);
                    if (f.b == e.b
                        || std::find(neighbours.begin(), neighbours.end(), f.b)
                               != neighbours.end()) {
                        patch[next] = id;
                        queue.push_back(next);
                    }
                }
            };
            visit(e.a);
            for (const auto node : field.neighbours(e.a)) {
                if (field.cells[node].basin == portal.a) {
                    visit(node);
                }
            }
        }
        portal.connections = unsigned(queue.size());
        mean = mul(mean, 1.f / float(portal.connections));
        float closest = INFINITY;
        for (const auto index : queue) {
            const auto& e = crossings[index];
            if (e.clearance + 1e-4f < portal.saddle_clearance) {
                continue;
            }
            const auto point = mul(add(field.cells[e.a].position, field.cells[e.b].position), .5f);
            const float d = distance(point, mean);
            if (d < closest) {
                closest = d;
                portal.centre = point;
            }
        }
        field.portals.push_back(std::move(portal));
    }
    // A representative doorway stays on a real clear connection, and a basin
    // centre stays on a deepest-interior cell near its clearance plateau centre.
    for (unsigned i = 0; i < means.size(); ++i) {
        means[i] = mul(means[i], 1.f / float(field.basins[i].cells));
    }
    std::vector<float> closest(means.size(), INFINITY);
    for (const auto& cell : field.cells) {
        auto& basin = field.basins[cell.basin];
        if (cell.lateral_clearance + 1e-4f < basin.peak_clearance) {
            continue;
        }
        const float d = distance(cell.position, means[cell.basin]);
        if (d < closest[cell.basin]) {
            closest[cell.basin] = d;
            basin.centre = cell.position;
        }
    }
}
}  // namespace openrac::lighting::bake_detail
