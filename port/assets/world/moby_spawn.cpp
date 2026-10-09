// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/moby_spawn.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The level loader's spawn test and the spaceships file.

#include "assets/world/moby_spawn.h"

#include "assets/world/known_games.h"

namespace openrac::assets {

std::size_t rac1_first_visit_ship(u32 level) {
    if (level <= 7) {
        return 0;
    }
    return level <= 13 ? 1 : 2;
}

SpaceshipFile read_spaceship_file(Game game, ByteView file) {
    require_world_layout(game, "spaceships file");
    const std::size_t ship = file.u32_at(0);
    const std::size_t texture = file.u32_at(4);
    const std::size_t extra = file.u32_at(8);
    const std::size_t extra_texture = file.u32_at(12);
    if (!(ship < extra && extra < texture && texture < extra_texture && extra_texture < file.size())) {
        fail(
            "spaceships file: unexpected block order {:#x} {:#x} {:#x} {:#x}",
            ship,
            texture,
            extra,
            extra_texture
        );
    }
    auto texture_list = [&](std::size_t at, u32 side) {
        if (file.u32_at(at) != 1 || file.u32_at(at + 4) != 0x10) {
            fail("spaceships texture list at {:#x}: not one PIF", at);
        }
        const PifImage pif = read_pif(game, file, at + 0x10);
        if (pif.width != side || pif.height != side) {
            fail("spaceships texture at {:#x}: {}x{}, expected {}x{}", at, pif.width, pif.height, side, side);
        }
        return pif;
    };
    return {
        file.sub(ship, extra - ship),
        file.sub(extra, texture - extra),
        texture_list(texture, 256),
        texture_list(extra_texture, 128),
    };
}

bool SpawnSave::killed_bit(s32 id) const {
    if (id < 0 || static_cast<std::size_t>(id >> 3) >= killed.size()) {
        return false;
    }
    return ((killed[static_cast<std::size_t>(id >> 3)] >> (id & 7)) & 1) != 0;
}

s32 SpawnSave::spawner_slot(s32 id) {
    if (id < 0) {
        return -1;
    }
    const auto value = static_cast<s16>(id + 1);
    for (s32 k = 63; k >= 0; --k) {
        s16& slot = spawner[static_cast<std::size_t>(k)];
        if (slot == value || slot == 0) {
            slot = value;
            return k;
        }
    }
    return -1;
}

SpawnDecision spawn_decision(const MobyInstance& instance, SpawnSave& save) {
    const auto flags = static_cast<u32>(instance.spawn_flags);
    const s32 id = instance.spawn_id;
    SpawnDecision d;
    d.b4 = static_cast<s16>(instance.start_value);
    d.b6 = static_cast<s16>(instance.start_value);
    if (flags == 0) {
        return d;
    }
    d.b1 = (flags & 0x10) != 0 ? static_cast<u8>(save.spawner_slot(id)) : u8{0xff};
    auto halve = [](s32 v) { return static_cast<s16>((v + 1) / 2); };
    const auto flag = save.id_flags.find(id);
    const bool flagged = id >= 0 && flag != save.id_flags.end() && flag->second != 0;
    if (flagged) {
        d.spawn = false;
    } else if ((flags & 3) != 0) {
        const bool done = instance.mission >= 0
                       && static_cast<std::size_t>(instance.mission) < save.missions.size()
                       && save.missions[static_cast<std::size_t>(instance.mission)] == 0xff;
        if (done) {
            if ((flags & 2) == 0) {
                d.spawn = false;
            } else {
                d.b6 = static_cast<s16>(instance.start_value_done);
                d.b4 = save.visit_deaths.contains(id) ? halve(instance.start_value_done)
                                                      : static_cast<s16>(instance.start_value_done);
            }
        } else if ((flags & 1) == 0) {
            d.spawn = false;
        } else if (save.killed_bit(id)) {
            d.b4 = halve(instance.start_value);
        }
    } else if ((flags & 8) != 0) {
        d.spawn = !save.visit_deaths.contains(id);
    } else if ((flags & 0xc) == 4) {
        d.spawn = !save.killed_bit(id);
    }
    return d;
}

std::vector<SpawnDecision> spawn_decisions(std::span<const MobyInstance> instances, SpawnSave& save) {
    std::vector<SpawnDecision> out;
    out.reserve(instances.size());
    for (const MobyInstance& m : instances) {
        out.push_back(spawn_decision(m, save));
    }
    return out;
}

std::vector<std::optional<std::size_t>> instance_to_moby(std::span<const SpawnDecision> decisions) {
    std::vector<std::optional<std::size_t>> out;
    std::size_t next = 0;
    for (const SpawnDecision& d : decisions) {
        out.push_back(d.spawn ? std::optional<std::size_t>(next++) : std::nullopt);
    }
    return out;
}

}  // namespace openrac::assets
