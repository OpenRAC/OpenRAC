// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Authored transition and reviewed placement seeds, with double-precision geometric floor projection.
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "assets/version.h"
#include "assets/world/gameplay.h"
#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "enhanced/tools/baker/planning/region_plan_internal.h"
#include "enhanced/tools/baker/planning/region_plan_json.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting::region_plan {
namespace {
// ---- Authored seeds ----

// Converts one axis of a world box to an inward coarse-clipped local lattice endpoint.
Bounds clipped(const Bounds& source, const Settings& settings) {
    Bounds result{};

    // Each endpoint snaps inward against the coarse-supported domain.
    for (unsigned axis = 0; axis < 3; ++axis) {
        const double step = settings.spacing[axis];
        result[axis] = std::max(
            std::floor(source[axis] / step) * step, std::ceil(settings.lower[axis] / step) * step
        );
        result[axis + 3] = std::min(
            std::ceil(source[axis + 3] / step) * step,
            std::floor(settings.upper[axis] / step) * step
        );

        // Clipping can remove all usable extent on one axis.
        if (result[axis + 3] - result[axis] < step) {
            throw std::runtime_error("outside sparse grid or no usable extent");
        }
    }

    return result;
}

// Six columns hold each three-axis inverse alongside the identity used by elimination.
using InverseRows = std::array<std::array<double, 6>, 3>;

// Eliminates one inverse-matrix column in the original pivot and arithmetic order.
void eliminate_column(InverseRows& rows, unsigned column) {
    unsigned pivot = column;

    // Choose the largest pivot to match the reference inverse.
    for (unsigned row = column + 1; row < 3; ++row) {
        // Strict comparison retains the first row at equal magnitude.
        if (std::abs(rows[row][column]) > std::abs(rows[pivot][column])) {
            pivot = row;
        }
    }

    // Singular authored bounds cannot define a finite seed.
    if (std::abs(rows[pivot][column]) < 1e-12) {
        throw std::runtime_error("singular transition transform");
    }

    std::swap(rows[column], rows[pivot]);
    const double scale = rows[column][column];

    // Normalising the pivot row precedes every elimination.
    for (double& value : rows[column]) {
        value /= scale;
    }

    // Eliminate this column from the remaining axes.
    for (unsigned row = 0; row < 3; ++row) {
        // The pivot row already has the required unit coefficient.
        if (row == column) {
            continue;
        }

        const double factor = rows[row][column];

        // All inverse and identity columns must undergo the same operation.
        for (unsigned item = 0; item < 6; ++item) {
            rows[row][item] -= factor * rows[column][item];
        }
    }
}

// Reads finite authored axes and inverts them without contracting multiply/subtract operations.
InverseRows inverse_axes(const assets::EnvTransition& transition) {
    // Reject invalid transforms before inverting or projecting corners.
    for (const auto& row : transition.inverse) {
        // A nonfinite matrix cannot produce trustworthy coverage.
        if (!std::all_of(row.begin(), row.end(), [](float value) {
                return std::isfinite(value);
            })) {
            throw std::runtime_error("nonfinite transition transform");
        }
    }

    InverseRows rows{};

    // Build the row-vector inverse in the original arithmetic order.
    for (unsigned row = 0; row < 3; ++row) {
        // Identity columns recover the forward transform.
        for (unsigned column = 0; column < 3; ++column) {
            rows[row][column] = transition.inverse[column][row];
            rows[row][column + 3] = row == column;
        }
    }

    // Column order preserves floating-point parity with the reference.
    for (unsigned column = 0; column < 3; ++column) {
        eliminate_column(rows, column);
    }

    return rows;
}

// Transforms corners in the former order after inverting the authored row-vector axes.
Bounds transition_bounds(const assets::EnvTransition& transition) {
    const auto rows = inverse_axes(transition);
    Bounds bounds{INFINITY, INFINITY, INFINITY, -INFINITY, -INFINITY, -INFINITY};

    // All eight corners determine the axis-aligned source bounds.
    for (unsigned corner = 0; corner < 8; ++corner) {
        // Accumulate each world component independently.
        for (unsigned axis = 0; axis < 3; ++axis) {
            double position = 0;

            // Corner bits follow the reference product-order traversal.
            for (unsigned component = 0; component < 3; ++component) {
                const double coordinate = (corner & (1u << (2 - component))) ? 1 : -1;
                position +=
                    rows[axis][component + 3] * (coordinate - transition.inverse[3][component]);
            }

            // Overflowing transformed corners cannot seed an allocation.
            if (!std::isfinite(position)) {
                throw std::runtime_error("nonfinite transition transform");
            }

            bounds[axis] = std::min(bounds[axis], position);
            bounds[axis + 3] = std::max(bounds[axis + 3], position);
        }
    }

    return bounds;
}

// Expands each supported authored boundary and preserves rejected source diagnostics.
void authored_seeds(State& state, const assets::GameplayFile& gameplay) {
    const auto transitions = assets::read_env_transitions(gameplay).transitions;
    state.authored_count = static_cast<unsigned>(transitions.size());

    // Keep authored source indices even for rejected transitions.
    for (unsigned index = 0; index < transitions.size(); ++index) {
        // Fog-only transitions do not describe hero lighting coverage.
        if (!transitions[index].lerps_hero_light()) {
            state.skipped.push_back(
                "{\"source\":" + std::to_string(index)
                + ",\"reason\":\"no hero-light transition flag\"}"
            );
            continue;
        }

        // One malformed transition must not discard unrelated valid seeds.
        try {
            auto bounds = transition_bounds(transitions[index]);

            // Authored boundaries receive separate horizontal and vertical padding.
            for (unsigned axis = 0; axis < 3; ++axis) {
                const double padding =
                    axis == 2 ? state.settings.vertical_padding : state.settings.padding;
                bounds[axis] -= padding;
                bounds[axis + 3] += padding;
            }

            bounds = clipped(bounds, state.settings);

            // Every authored piece must fit the allocation budget.
            for (const auto& part : split_bounds(bounds, state.settings)) {
                state.regions.push_back({part, {}, {index}});
            }
        } catch (const std::runtime_error& error) {
            state.skipped.push_back(
                "{\"source\":" + std::to_string(index)
                + ",\"reason\":" + coordinator_string(error.what()) + '}'
            );
        }
    }
}

// ---- Floor support ----

// Bounds XY hashing before integer conversion or potentially enormous bucket loops.
int tile_coordinate(double coordinate) {
    const double tile = std::floor(coordinate / 16);

    // Reject enormous coordinates before integer conversion or bucket iteration.
    if (!std::isfinite(tile) || tile < -1'000'000 || tile > 1'000'000) {
        throw std::runtime_error("collision exceeds region planning coordinate limits");
    }

    return static_cast<int>(tile);
}

// A walkable triangle's double-precision XY projection, independent of authored normal or winding.
struct GroundTriangle {
    CoveragePoint point{};   // First source vertex.
    CoveragePoint first{};   // Edge to the second vertex.
    CoveragePoint second{};  // Edge to the third vertex.
    double determinant = 0;  // Signed projected area used by barycentric coordinates.
};

// One collision XY index shared by every initial gameplay placement.
class Ground {
public:
    // Native collision is already transformed into the same exported world coordinates.
    explicit Ground(const std::vector<interiors::Triangle>& triangles) {
        // Build the floor index once for every reviewed placement.
        for (const auto& triangle : triangles) {
            GroundTriangle ground;

            // Edges retain double arithmetic independent of authored normals.
            for (unsigned axis = 0; axis < 3; ++axis) {
                ground.point[axis] = triangle.vertices[0][axis];
                ground.first[axis] =
                    static_cast<double>(triangle.vertices[1][axis]) - triangle.vertices[0][axis];
                ground.second[axis] =
                    static_cast<double>(triangle.vertices[2][axis]) - triangle.vertices[0][axis];
            }

            const auto& u = ground.first;
            const auto& v = ground.second;
            const CoveragePoint normal{
                u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]
            };
            const double length =
                std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);

            // Two-sided geometric slope rejects walls and degenerate triangles.
            if (length < 1e-8 || std::abs(normal[2]) < .6 * length) {
                continue;
            }

            ground.determinant = normal[2];
            const auto& points = triangle.vertices;
            const int x0 = tile_coordinate(std::min({points[0][0], points[1][0], points[2][0]}));
            const int x1 = tile_coordinate(std::max({points[0][0], points[1][0], points[2][0]}));
            const int y0 = tile_coordinate(std::min({points[0][1], points[1][1], points[2][1]}));
            const int y1 = tile_coordinate(std::max({points[0][1], points[1][1], points[2][1]}));

            // A single enormous triangle must not allocate unbounded floor buckets.
            if (static_cast<std::int64_t>(x1 - x0 + 1) * (y1 - y0 + 1)
                > static_cast<std::int64_t>(kMaximumPlanningCells)) {
                throw std::runtime_error(
                    "collision triangle exceeds region planning bucket limits"
                );
            }

            // Bucket footprints include shared triangle edges.
            for (int x = x0; x <= x1; ++x) {
                // The same triangle may support placements in adjacent bins.
                for (int y = y0; y <= y1; ++y) {
                    m_tiles[{x, y}].push_back(ground);
                }
            }
        }
    }

    // Retains the nearest walkable hit in the original 32-below/two-above placement band.
    std::optional<double> below(CoveragePoint position) const {
        const auto found =
            m_tiles.find({tile_coordinate(position[0]), tile_coordinate(position[1])});

        // Missing floor evidence leaves the placement height unchanged.
        if (found == m_tiles.end()) {
            return {};
        }

        std::optional<double> closest;

        // The closest supported height wins independently of winding.
        for (const auto& triangle : found->second) {
            const double x = position[0] - triangle.point[0];
            const double y = position[1] - triangle.point[1];
            const double s =
                (x * triangle.second[1] - y * triangle.second[0]) / triangle.determinant;
            const double t = (triangle.first[0] * y - triangle.first[1] * x) / triangle.determinant;

            // Barycentric tolerance retains points on shared floor edges.
            if (s < -1e-6 || t < -1e-6 || s + t > 1 + 1e-6) {
                continue;
            }

            const double height =
                triangle.point[2] + s * triangle.first[2] + t * triangle.second[2];

            // Only the established placement search band can supply support.
            if (height >= position[2] - 32 && height <= position[2] + 2
                && (!closest || height > *closest)) {
                closest = height;
            }
        }

        return closest;
    }

private:
    std::map<std::array<int, 2>, std::vector<GroundTriangle>> m_tiles;  // Sixteen-unit XY bins.
};

// ---- Gameplay seeds ----

// Computes the placement's supported vertical range without constructing a tall airborne column.
Bounds marker_bounds(
    CoveragePoint position,
    std::optional<double> floor,
    std::optional<double> travel,
    const json::Value& entry,
    double vertical_spacing
) {
    const double z = position[2];
    const double low =
        std::min({z, floor.value_or(z), z + travel.value_or(0)}) - std::max(2.0, vertical_spacing);
    const double high =
        std::max(z, z + travel.value_or(0)) + std::max(entry["above"].number(3), vertical_spacing);
    const double radius = entry["radius"].number();
    return {
        position[0] - radius,
        position[1] - radius,
        low,
        position[0] + radius,
        position[1] + radius,
        high
    };
}

// Reads optional reviewed pvar travel through data-defined offsets, never shared game constants.
std::optional<double> travel(
    const State& state,
    const assets::GameplayFile& gameplay,
    const json::Value& moby,
    const json::Value& entry
) {
    const auto& layout = entry["travel"];

    // Travel layouts apply only to their configured version and level.
    if (!layout.is_object() || layout["game_version"].string() != state.game_version
        || layout["level"].number(-1) != state.level) {
        return {};
    }

    const double source_index = moby["fields"]["pvar_index"].number(-1);

    // Placements without pvars have no authored travel evidence.
    if (source_index < 0) {
        return {};
    }

    const int index =
        static_cast<int>(coordinator_integer(moby["fields"]["pvar_index"], 1'000'000));

    // Malformed optional pvars must not prevent static seed placement.
    try {
        const auto block = assets::read_pvar_block(gameplay, index);
        const auto minimum = coordinator_integer(layout["minimum_bytes"], 65'536);

        // Short pvar blocks cannot provide every reviewed field.
        if (!block || block->size() < minimum) {
            return {};
        }

        const assets::ByteView bytes(*block);
        const auto mode = bytes.u8_at(
            static_cast<std::size_t>(coordinator_integer(layout["mode_offset"], 65'536))
        );
        const double distance = bytes.f32_at(
            static_cast<std::size_t>(coordinator_integer(layout["distance_offset"], 65'536))
        );
        const double period = bytes.f32_at(
            static_cast<std::size_t>(coordinator_integer(layout["period_offset"], 65'536))
        );

        // Invalid movement fields must not create enormous vertical columns.
        if (mode > 1 || !std::isfinite(distance) || distance < 0
            || distance > layout["maximum_distance"].number() || !std::isfinite(period)
            || period <= 0) {
            return {};
        }

        return mode == 0 ? -distance : distance;
    } catch (const assets::AssetError&) {
        return {};
    }
}

// Builds marker diagnostics and supported bands before clipping or merging any allocation.
void placement_seed(
    State& state,
    const json::Value& moby,
    const json::Value& entry,
    const Ground& ground,
    const assets::GameplayFile& gameplay
) {
    Marker marker;
    marker.index = static_cast<unsigned>(coordinator_integer(moby["index"], 1'000'000));
    marker.class_id = static_cast<unsigned>(coordinator_integer(moby["class"], 1'000'000));
    marker.name = entry["name"].string();
    marker.kind = entry["kind"].is_string() ? entry["kind"].string() : "gameplay";

    // Placement matrices store their translation in the final row.
    for (unsigned axis = 0; axis < 3; ++axis) {
        marker.position[axis] = moby["matrix"][12 + axis].number(NAN);
    }

    // Nonfinite placements are recorded and skipped rather than entering geometry queries.
    if (!std::all_of(marker.position.begin(), marker.position.end(), [](double x) {
            return std::isfinite(x);
        })) {
        state.skipped.push_back(
            "{\"moby\":" + std::to_string(marker.index) + ",\"reason\":\"nonfinite placement\"}"
        );
        return;
    }

    marker.ground = ground.below(marker.position);
    marker.travel = travel(state, gameplay, moby, entry);
    marker.bounds = marker_bounds(
        marker.position, marker.ground, marker.travel, entry, state.settings.spacing[2]
    );
    marker.bands = {marker.bounds};

    // Airborne items retain separate floor and item bands instead of a tall empty column.
    if (marker.ground && !marker.travel
        && marker.position[2] - *marker.ground > std::max(8.0, state.settings.spacing[2] * 2)) {
        auto floor = marker.position;
        floor[2] = *marker.ground;
        marker.bands =
            {marker_bounds(floor, marker.ground, {}, entry, state.settings.spacing[2]),
             marker_bounds(marker.position, {}, {}, entry, state.settings.spacing[2])};
    }

    state.markers.push_back(std::move(marker));
}

// Validates reviewed extents before any placement can allocate coverage.
std::map<unsigned, const json::Value*> allowed_classes(const json::Value& whitelist) {
    // An omitted or malformed whitelist must not silently remove gameplay coverage.
    if (!whitelist["classes"].is_array() || whitelist["classes"].items().size() > 16'384) {
        throw std::runtime_error("invalid moby seed class list");
    }

    std::map<unsigned, const json::Value*> allowed;

    // Only reviewed classes may seed gameplay coverage.
    for (const auto& entry : whitelist["classes"].items()) {
        const auto id = static_cast<unsigned>(coordinator_integer(entry["class"], 1'000'000));

        // Duplicate keys and invalid extents make the whitelist ambiguous.
        if (!allowed.emplace(id, &entry).second || !entry["radius"].is_number()
            || entry["radius"].number() <= 0 || entry["above"].number(3) <= 0
            || (!entry["above"].is_null() && !entry["above"].is_number())) {
            throw std::runtime_error("duplicate class or invalid moby seed extent");
        }
    }

    return allowed;
}

// Records rejected extents while retaining every valid seed in its source order.
std::vector<Region> clipped_seeds(State& state) {
    std::vector<Region> clipped_regions;

    // Clip each source before overlap merging.
    for (auto& region : state.regions) {
        // Out-of-domain seeds remain diagnostic evidence rather than bake allocations.
        try {
            region.bounds = clipped(region.bounds, state.settings);
            clipped_regions.push_back(std::move(region));
        } catch (const std::runtime_error& error) {
            state.skipped.push_back(
                "{\"bounds\":" + sequence_json(region.bounds)
                + ",\"source_mobys\":" + sequence_json(region.mobys)
                + ",\"source_transitions\":" + sequence_json(region.transitions)
                + ",\"reason\":" + coordinator_string(error.what()) + '}'
            );
        }
    }

    return clipped_regions;
}

// Collects floor anchors independently of local probe allocation or interior emission.
void placement_markers(
    State& state,
    const json::Value& placements,
    const json::Value& whitelist,
    const assets::GameplayFile& gameplay
) {
    // Bound placement traversal and reject an omitted export table.
    if (!placements["mobys"].is_array() || placements["mobys"].items().size() > 1'000'000) {
        throw std::runtime_error("invalid region planning placement list");
    }

    const Ground ground(state.triangles);
    const auto allowed = allowed_classes(whitelist);

    // Placement order preserves source provenance and merge ties.
    for (const auto& moby : placements["mobys"].items()) {
        const auto id = static_cast<unsigned>(coordinator_integer(moby["class"], 1'000'000));

        // Unlisted scenery must not expand playable coverage.
        if (allowed.contains(id)) {
            placement_seed(state, moby, *allowed.at(id), ground, gameplay);
        }
    }
}

// Clips every initial seed before bounded overlap merging and recursive budget splitting.
void combine_seeds(State& state) {
    // Supported height bands become independent initial seed boxes.
    for (const auto& marker : state.markers) {
        // Airborne placements can contribute more than one supported band.
        for (const auto& band : marker.bands) {
            state.regions.push_back({band, {marker.index}, {}});
        }
    }

    auto clipped_regions = clipped_seeds(state);
    state.regions =
        merge_regions(std::move(clipped_regions), state.settings, state.settings.geometry);
    std::vector<Region> result;

    // Merged seeds can still need budget subdivision.
    for (const auto& region : state.regions) {
        // Budget cuts retain the combined source metadata.
        for (const auto& bounds : split_bounds(region.bounds, state.settings)) {
            result.push_back({bounds, region.mobys, region.transitions});
        }
    }

    std::sort(result.begin(), result.end(), [](const Region& a, const Region& b) {
        return a.bounds < b.bounds;
    });
    state.regions = std::move(result);
}
}  // namespace

// ---- Planning inputs ----

void seed_regions(
    State& state, const std::filesystem::path& level, const json::Value& request, bool coverage
) {
    const profiling::Zone timing("Seed probe regions");
    const auto bytes = source_bytes(request["gameplay"].string());
    state.gameplay_hash = source_hash(bytes);
    const auto* version = assets::find_version(state.game_version);

    // Gameplay layouts are supplied by the existing version registry.
    if (!version) {
        throw std::runtime_error("unsupported gameplay identity for region planning");
    }

    const assets::GameplayFile gameplay(version->game, bytes);
    // Lighting boundaries allocate local coverage only when that feature is enabled.
    if (coverage) {
        authored_seeds(state, gameplay);
    }
    const bool collision = std::filesystem::is_regular_file(level / "collision.glb");

    // Only requested floor or enclosure queries need collision geometry.
    if (collision
        && (state.settings.mobys || (!state.regions.empty() && state.settings.geometry))) {
        auto triangles = load_coverage_collision(level);

        // Collision decoding errors must stop planning before publication.
        if (!triangles) {
            throw std::runtime_error(triangles.error().message);
        }

        state.triangles = std::move(*triangles);
        auto identity = bake_source_identity(level / "collision.glb");

        // Collision size must not require a second complete resident copy for hashing.
        if (!identity) {
            throw std::runtime_error(identity.error().message);
        }

        state.geometry_hash = std::move(*identity);
    }

    // Moby seeding needs both collision support and a reviewed whitelist.
    if (collision && state.settings.mobys) {
        const std::filesystem::path path = request["whitelist"].string();
        const auto whitelist = document(path);
        const auto manifest = document(level / "manifest.json");

        // Whitelist identity must match the exported game.
        if (whitelist["version"].number() != 1
            || whitelist["game"].string() != manifest["game"].string()) {
            throw std::runtime_error("unsupported moby seed whitelist");
        }

        state.whitelist_hash = source_hash(source_bytes(path));
        placement_markers(state, document(level / "placements.json"), whitelist, gameplay);

        // Reflection anchors must not accidentally allocate, merge or split local GI volumes.
        if (coverage) {
            combine_seeds(state);
        }
    }
}
}  // namespace openrac::lighting::region_plan
