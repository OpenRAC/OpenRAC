// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "enhanced/lighting/environment.h"
#include "enhanced/tools/baker/sources/level_scene.h"

namespace openrac::lighting {
// Whether exports of `game` (a manifest's "game", "rac1") and `version` ("pal") can be baked and
// their bakes loaded: Ratchet & Clank's PAL and NTSC-U discs, and Going Commando's three discs.
bool bake_identity_supported(std::string_view game, std::string_view version);

// The same for a port id ("rac1-pal", "rac2-ntsc").
bool bake_identity_supported(std::string_view game_version);

struct ProbeActivation {
    BakeSettings bake;
    unsigned light_set = 0;
    Float3 environment{0.1f, 0.1f, 0.1f};
};

struct ProbeLookRegion {
    Float3 minimum{}, maximum{};
    float feather = 0;
    float diffuse_gain = 1;
};

inline constexpr unsigned kProbeLookRegions = 12;

// Selects floor-based gameplay analysis or one complete scene allocation.
enum class BakeStrategy {
    Gameplay,  // Floor-seeded local GI and reflection captures.
    Scene      // One coarse GI volume over complete render-scene bounds.
};

// Per-level transport and placement configuration, shared with runtime activation.
struct LevelBakeProfile {
    BakeStrategy strategy = BakeStrategy::Gameplay;  // Gameplay analysis is the normal level path.
    EnvironmentBakeSettings environment;
    ProbeActivation activation;
    std::vector<ProbeLookRegion> look_regions;
    std::vector<BakeAreaEmitter> area_lights;
    Float3 sparse_spacing{256, 256, 256};
    Float3 dense_spacing{4, 4, 4};
    unsigned map_resolution = 6;
};

LevelBakeProfile load_level_bake_profile(
    const std::filesystem::path& directory, const std::filesystem::path& overrides = {}
);

// One level-load session shares scene validation across sparse and dense assets.
// Construct a new session when the scene or lighting inputs change.
class ProbeAssetLoader {
public:
    ProbeAssetLoader(const scene::LevelData& level, const ProbeActivation& expected);
    ProbeVolume load(const std::filesystem::path& path, bool expand_linear_maps = false) const;

private:
    std::string m_version;
    int m_level;
    BakeSettings m_bake;
    std::uint64_t m_scene_hash;
};

// Retain GPU-ready file planes; offline diagnostics can request linear maps.
// Reject wrong game/level, stale scene and bake settings before texture upload.
// Never invokes Embree.
ProbeVolume load_level_probes(
    const std::filesystem::path& path,
    const scene::LevelData& level,
    const ProbeActivation& expected,
    bool expand_linear_maps = false
);
}  // namespace openrac::lighting
