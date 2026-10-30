// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/tools/probes/activation.h"

#include <bit>
#include <cfenv>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include "common/json.h"
#include "enhanced/tools/probes/probe_fingerprint.h"

namespace openrac::lighting {
namespace {
// Normalises the optional world-space travel direction; null restores authored light sets.
std::optional<Float3> read_sunlight_direction(const json::Value& value) {
    // A mod can clear a direction inherited from the level profile.
    if (value.is_null()) {
        return std::nullopt;
    }

    // Directions contain exactly three coordinates, with Z as height.
    if (!value.is_array() || value.items().size() != 3) {
        throw std::runtime_error("sunlight_direction requires three finite coordinates or null");
    }

    Float3 direction{};

    for (unsigned axis = 0; axis < direction.size(); ++axis) {
        const double component = value[axis].number();

        // Bound components before narrowing and normalising the vector.
        if (!value[axis].is_number() || !std::isfinite(component) || std::abs(component) > 1e6) {
            throw std::runtime_error(
                "sunlight_direction requires finite coordinates within 1000000"
            );
        }

        direction[axis] = static_cast<float>(component);
    }

    const double length = std::hypot(static_cast<double>(direction[0]), direction[1], direction[2]);

    // A zero vector cannot define a sun or shadow projection.
    if (length <= 0) {
        throw std::runtime_error("sunlight_direction requires a nonzero vector");
    }

    for (float& component : direction) {
        component = static_cast<float>(component / length);
    }

    return direction;
}

// Reads room-local absolute surface gains without changing shared texture emission definitions.
std::vector<SurfaceEmissionRegion> read_surface_emission_regions(const json::Value& value) {
    // Profiles replace the full override list, including an explicit empty array.
    if (!value.is_array()) {
        throw std::runtime_error("surface_emission_regions requires an array");
    }

    std::vector<SurfaceEmissionRegion> regions;

    for (const auto& entry : value.items()) {
        // Each room supplies lower/upper triples and a finite nonnegative absolute gain.
        if (!entry.is_object() || !entry["minimum"].is_array() || !entry["maximum"].is_array()
            || entry["minimum"].items().size() != 3 || entry["maximum"].items().size() != 3
            || !entry["scale"].is_number() || !std::isfinite(entry["scale"].number())
            || entry["scale"].number() < 0 || entry["scale"].number() > 1e6) {
            throw std::runtime_error("invalid surface emission region");
        }

        SurfaceEmissionRegion region;
        region.scale = static_cast<float>(entry["scale"].number());

        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto& low = entry["minimum"][axis];
            const auto& high = entry["maximum"][axis];

            // Reversed or nonfinite bounds cannot define a room.
            if (!low.is_number() || !high.is_number() || !std::isfinite(low.number())
                || !std::isfinite(high.number()) || std::abs(low.number()) > 1e6
                || std::abs(high.number()) > 1e6 || low.number() > high.number()) {
                throw std::runtime_error("invalid surface emission region bounds");
            }

            region.minimum[axis] = static_cast<float>(low.number());
            region.maximum[axis] = static_cast<float>(high.number());
        }

        regions.push_back(region);
    }

    return regions;
}

// Per-level planning limits stay separate from the transport solver's light intensities.
BakeTraversalSettings read_traversal_profile(
    const json::Value& value, BakeTraversalSettings result
) {
    if (!value.is_object()) {
        throw std::runtime_error("playable_space requires an object");
    }

    for (const auto& [key, setting] : value.members()) {
        const double n = setting.number(-1);

        if (!setting.is_number() || !std::isfinite(n) || n < 0 || n > 64) {
            throw std::runtime_error("playable space setting requires a finite value from 0 to 64");
        }

        if (key == "jump_distance") {
            result.jump_distance = float(n);
        } else if (key == "jump_rise") {
            result.jump_rise = float(n);
        } else if (key == "jump_drop") {
            result.jump_drop = float(n);
        } else if (key == "player_radius" && n >= .1 && n <= 2) {
            result.player_radius = float(n);
        } else if (key == "player_height" && n >= .5 && n <= 8) {
            result.player_height = float(n);
        } else if (key == "minimum_swim_depth" && n >= .25 && n <= 8) {
            result.minimum_swim_depth = static_cast<float>(n);
        } else {
            throw std::runtime_error("unknown or invalid playable space setting");
        }
    }

    return result;
}

// Shared strict reflection schema, including partial overrides in a mod profile.
EnvironmentBakeSettings read_environment_profile(
    const json::Value& value, EnvironmentBakeSettings result
) {
    if (!value.is_object()) {
        throw std::runtime_error("environment_probes requires an object");
    }

    for (const auto& [key, setting] : value.members()) {
        if (key == "enabled" || key == "merge_enabled") {
            if (setting.type() != json::Value::Type::Bool) {
                throw std::runtime_error("environment enabled requires a boolean");
            }

            (key == "enabled" ? result.enabled : result.merge_enabled) = setting.boolean();
            continue;
        }

        const double n = setting.number(-1);

        if (!setting.is_number() || !std::isfinite(n)) {
            throw std::runtime_error("environment setting requires a finite number");
        }

        if (key == "resolution" || key == "maximum_probes" || key == "filter_samples"
            || key == "merge_max_misses") {
            const unsigned limit = key == "resolution"
                                       ? 256u
                                       : (key == "maximum_probes"     ? kEnvironmentProbeLimit
                                          : key == "merge_max_misses" ? 8u
                                                                      : 4096u);

            if (n < (key == "merge_max_misses" ? 0 : 1) || n > limit || std::floor(n) != n) {
                throw std::runtime_error("invalid environment integer setting");
            }

            const auto integer = static_cast<unsigned>(n);

            if (key == "resolution") {
                if (integer < 4 || !std::has_single_bit(integer)) {
                    throw std::runtime_error(
                        "environment resolution requires a power of two from 4 to 256"
                    );
                }

                result.resolution = integer;
            } else if (key == "maximum_probes") {
                result.maximum_probes = integer;
            } else if (key == "merge_max_misses") {
                result.merge_max_misses = integer;
            } else {
                result.filter_samples = integer;
            }
        } else if (key == "merge_brightness_ratio" || key == "merge_colour_distance") {
            const bool brightness = key == "merge_brightness_ratio";

            if (n < (brightness ? 1 : 0) || n > (brightness ? 8 : 1)) {
                throw std::runtime_error("environment colour threshold outside permitted range");
            }

            (brightness ? result.merge_brightness_ratio : result.merge_colour_distance) =
                static_cast<float>(n);
        } else if (key == "minimum_capture_spacing") {
            if (n < 0 || n > 64) {
                throw std::runtime_error("minimum capture spacing must be from zero to 64");
            }

            result.minimum_capture_spacing = static_cast<float>(n);
        } else if (key == "minimum_space_area") {
            if (n < 0 || n > 256) {
                throw std::runtime_error("environment minimum space area must be from zero to 256");
            }

            result.minimum_space_area = static_cast<float>(n);
        } else if (key == "merge_overlap" || key == "merge_loss_fraction") {
            const double low = key == "merge_overlap" ? .8 : 0,
                         high = key == "merge_overlap" ? 1 : .01;

            if (n < low || n > high) {
                throw std::runtime_error("environment merge threshold outside permitted range");
            }

            (key == "merge_overlap" ? result.merge_overlap : result.merge_loss_fraction) =
                static_cast<float>(n);
        } else if (key == "spacing" || key == "box_distance") {
            if (n <= 1 || n > 1024) {
                throw std::runtime_error(
                    "environment distances must exceed one and be at most 1024"
                );
            }

            (key == "spacing" ? result.spacing : result.box_distance) = static_cast<float>(n);
        } else if (key == "normal_bias" || key == "view_bias") {
            if (n < 0 || n > 4) {
                throw std::runtime_error("environment GI bias must be from zero to four");
            }

            (key == "normal_bias" ? result.normal_bias : result.view_bias) = static_cast<float>(n);
        } else {
            throw std::runtime_error("unknown environment setting: " + key);
        }
    }

    return result;
}

void apply_profile(const std::filesystem::path& path, LevelBakeProfile& profile) {
    if (!std::filesystem::exists(path)) {
        return;
    }

    json::Value root;
    std::string error;
    const auto fail = [&](const std::string& detail) {
        throw std::runtime_error(path.string() + ": " + detail);
    };

    if (!json::parse_file(path.string(), root, error)) {
        fail(error);
    }

    if (!root.is_object() || !root["version"].is_number() || root["version"].number() != 1) {
        fail("expected profile object with version 1");
    }

    auto number = [&](const json::Value& value, double minimum, double maximum) {
        const auto n = value.number();

        if (!value.is_number() || !std::isfinite(n) || n < minimum || n > maximum) {
            fail("number outside permitted range");
        }

        return static_cast<float>(n);
    };
    auto integer = [&](const json::Value& value, unsigned minimum, unsigned maximum) {
        const auto n = value.number();

        if (!value.is_number() || !std::isfinite(n) || std::floor(n) != n || n < minimum
            || n > maximum) {
            fail("expected integer in permitted range");
        }

        return static_cast<unsigned>(n);
    };
    auto vector = [&](const json::Value& value, float minimum) {
        if (!value.is_array() || value.items().size() != 3) {
            fail("expected three coordinates");
        }

        Float3 result{};

        for (unsigned i = 0; i < 3; ++i) {
            result[i] = number(value[i], minimum, 1e6);
        }

        return result;
    };
    auto& activation = profile.activation;
    auto& bake = activation.bake;

    for (const auto& [key, value] : root.members()) {
        if (key == "version") {
            continue;
        }

        if (key == "strategy") {
            // Strategies select placement, while transport and material settings remain shared.
            if (!value.is_string() || (value.string() != "gameplay" && value.string() != "scene")) {
                fail("strategy must be gameplay or scene");
            }

            // Scene-only exports do not require walkable floors or gameplay markers.
            profile.strategy =
                value.string() == "scene" ? BakeStrategy::Scene : BakeStrategy::Gameplay;
        } else if (key == "sky") {
            if (value.string() != "level" && value.string() != "uniform") {
                fail("sky must be level or uniform");
            }

            bake.use_level_sky = value.string() == "level";
        } else if (key == "surface_emission_regions") {
            bake.surface_emission_regions = read_surface_emission_regions(value);
        } else if (key == "surface_emission_scale") {
            bake.surface_emission_scale = number(value, 0, 1e6);
        } else if (key == "sky_intensity") {
            bake.sky_intensity = number(value, 0, 1e6);
        } else if (key == "sky_weight") {
            bake.sky_weight = number(value, 0, 1);
        } else if (key == "sunlight_scale") {
            bake.sunlight_scale = vector(value, 0);

            for (float v : bake.sunlight_scale) {
                if (v > 4) {
                    fail("sunlight_scale must not exceed 4");
                }
            }
        } else if (key == "sunlight_direction") {
            bake.sunlight_direction = read_sunlight_direction(value);
        } else if (key == "terrain_vertex_albedo") {
            if (value.type() != json::Value::Type::Bool) {
                fail("terrain_vertex_albedo requires a boolean");
            }

            bake.terrain_vertex_albedo = value.boolean();
        } else if (key == "rays") {
            bake.rays_per_probe = integer(value, 16, 1048576);
        } else if (key == "bounces") {
            bake.bounces = integer(value, 0, 32);
        } else if (key == "seed") {
            bake.seed = integer(value, 0, UINT32_MAX);
        } else if (key == "light_set") {
            activation.light_set = integer(value, 0, 255);
        } else if (key == "environment") {
            activation.environment = vector(value, 0);
        } else if (key == "sparse_spacing") {
            profile.sparse_spacing = vector(value, 0.001f);
        } else if (key == "dense_spacing") {
            profile.dense_spacing = vector(value, 0.001f);
        } else if (key == "environment_probes") {
            profile.environment = read_environment_profile(value, profile.environment);
        } else if (key == "playable_space") {
            bake.traversal = read_traversal_profile(value, bake.traversal);
        } else if (key == "water_sources") {
            // The offline source decoder owns the detailed table schema.
            if (!value.is_object()) {
                fail("water_sources requires an object");
            }
        } else if (key == "regions") {
            // Offline region planning shares the profile file and validates its own schema.
            if (!value.is_object()) {
                fail("regions requires an object");
            }
        } else if (key == "resolution") {
            profile.map_resolution = integer(value, 2, 64);
        } else if (key == "ray_bias") {
            bake.ray_bias = number(value, 0.000001, 1e6);
        } else if (key == "distance_exponent") {
            bake.distance_exponent = number(value, 1, 256);
        } else if (key == "min_inside_fraction") {
            bake.min_inside_fraction = number(value, 0.500001, 1);
        } else if (key == "relocation_iterations") {
            bake.relocation_iterations = integer(value, 0, 32);
        } else if (key == "relocation_max_fraction") {
            bake.relocation_max_fraction = number(value, 0, 0.45);
        } else {
            fail("unknown setting: " + key);
        }
    }
}
}  // namespace

LevelBakeProfile load_level_bake_profile(
    const std::filesystem::path& directory, const std::filesystem::path& overrides
) {
    // The game runs with FE_TOWARDZERO. Canonical profile values must match the
    // offline baker, including the float bits used by the settings fingerprint.
    struct Rounding {
        int previous = std::fegetround();

        Rounding() { std::fesetround(FE_TONEAREST); }

        ~Rounding() { std::fesetround(previous); }

        Rounding(const Rounding&) = delete;
        Rounding& operator=(const Rounding&) = delete;
    } rounding;

    LevelBakeProfile profile;
    apply_profile(directory / "probe-bake.json", profile);

    if (!overrides.empty()) {
        apply_profile(overrides / "probe-bake.json", profile);
    }

    // Scene strategy has no playable-floor graph for local reflection capture placement.
    if (profile.strategy == BakeStrategy::Scene) {
        profile.environment.enabled = false;
    }

    return profile;
}

ProbeAssetLoader::ProbeAssetLoader(const scene::LevelData& level, const ProbeActivation& expected)
    : m_level(level.level),
      m_bake(expected.bake) {
    json::Value manifest;
    std::string error;

    if (!json::parse_file((level.dir / "manifest.json").string(), manifest, error)) {
        throw std::runtime_error(error);
    }

    m_version = manifest["game"].string() + "-" + manifest["version"].string();

    if ((m_version != "rac1-pal" && m_version != "rac1-ntsc") || m_level < 0) {
        throw std::runtime_error("probe game version or level does not match loaded scene");
    }

    m_scene_hash =
        export_probe_scene(level, expected.light_set, expected.environment, false).scene_hash;
}

ProbeVolume ProbeAssetLoader::load(
    const std::filesystem::path& path, bool expand_linear_maps
) const {
    std::ifstream input(path, std::ios::binary | std::ios::ate);

    if (!input) {
        throw std::runtime_error("cannot open probe asset");
    }

    const auto size = input.tellg();

    if (size <= 0 || size > 512ll * 1024 * 1024) {
        throw std::runtime_error("unsupported probe asset size");
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);

    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("cannot read probe asset");
    }

    auto volume = expand_linear_maps ? decode_probes(bytes) : decode_gpu_probes(std::move(bytes));

    if (volume.grid.game_version != m_version
        || volume.grid.level != static_cast<std::uint32_t>(m_level)) {
        throw std::runtime_error("probe game version or level does not match loaded scene");
    }

    if (volume.grid.settings_hash != probe_bake_settings_hash(volume.grid, m_bake)) {
        throw std::runtime_error(
            "probe bake settings differ; check OPENRAC_PROBE_RAYS and rebake if needed"
        );
    }

    if (volume.grid.scene_hash != m_scene_hash) {
        throw std::runtime_error(
            "stale probe scene or different lighting inputs; rebake this level"
        );
    }

    return volume;
}

ProbeVolume load_level_probes(
    const std::filesystem::path& path,
    const scene::LevelData& level,
    const ProbeActivation& expected,
    bool expand_linear_maps
) {
    return ProbeAssetLoader(level, expected).load(path, expand_linear_maps);
}
}  // namespace openrac::lighting
