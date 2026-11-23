// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "hero": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_hero_pose(openrac_pose* out) {
    if (out == nullptr || out->size < sizeof(openrac_pose) || g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose pose{};
    if (g_play->hero_pose(&pose) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->level, &pose, sizeof pose);
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in(openrac_mod* mod, const float position[3], int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || position == nullptr
        || g_play == nullptr || g_play->stand_in == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in(position, f != nullptr ? f->moby : 0);
    return OPENRAC_HOOK_OK;
}

int api_hero_held(openrac_held* out) {
    if (out == nullptr || out->size < sizeof(openrac_held) || g_play == nullptr
        || g_play->hero_held == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_held held{};
    if (g_play->hero_held(&held) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(&out->count, &held, sizeof held);
    return OPENRAC_HOOK_OK;
}

int api_hero_shots(openrac_shot* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot));
    }
    return n;
}

int api_hero_place(openrac_mod* mod, const float position[3], float yaw, std::uint32_t flags) {
    if (mod == nullptr || position == nullptr || g_play == nullptr || g_play->hero_place == nullptr
        || !std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])
        || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_place(position, yaw, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_grab(openrac_mod* mod, int animation, std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_grab == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_grab(animation, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_release(openrac_mod* mod) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_let_go == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_let_go() != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_hold_at(openrac_mod* mod, const float position[3], float yaw) {
    if (mod == nullptr || position == nullptr || g_play == nullptr
        || g_play->hero_hold_at == nullptr || !std::isfinite(position[0])
        || !std::isfinite(position[1]) || !std::isfinite(position[2]) || !std::isfinite(yaw)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_hold_at(position, yaw) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_stand_in_as(openrac_mod* mod, const openrac_stand_in* as, int figure) {
    if (mod == nullptr || mod != g_gate.mod || g_before_slot < 0 || as == nullptr
        || g_play == nullptr || g_play->stand_in_as == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const Figure* f = figure >= 0 ? own_figure(mod, figure) : nullptr;
    g_play->stand_in_as(as->position, f != nullptr ? f->moby : 0, as->state, as->health, as->flags);
    return OPENRAC_HOOK_OK;
}

int api_hero_stand_in_read(openrac_mod* mod, openrac_stand_in* out) {
    if (mod == nullptr || mod != g_gate.mod || out == nullptr || g_play == nullptr
        || g_play->stand_in_read == nullptr
        || g_play->stand_in_read(out->position, &out->yaw, &out->state, &out->health, &out->flags)
               == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return OPENRAC_HOOK_OK;
}

int api_hero_revive(openrac_mod* mod, const float position[3], std::uint32_t flags) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_revive == nullptr
        || (position != nullptr
            && (!std::isfinite(position[0]) || !std::isfinite(position[1])
                || !std::isfinite(position[2])))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->hero_revive(position, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_hero_shots2(openrac_shot2* out, int max) {
    if (out == nullptr || max <= 0 || g_play == nullptr || g_play->hero_shots2 == nullptr) {
        return 0;
    }
    std::array<openrac_game_shot2, OPENRAC_SHOTS_MAX> shots{};
    const int n = g_play->hero_shots2(shots.data(), std::min(max, OPENRAC_SHOTS_MAX));
    for (int i = 0; i < n; ++i) {
        out[i].size = sizeof(openrac_shot2);
        std::memcpy(&out[i].item, &shots[static_cast<std::size_t>(i)], sizeof(openrac_game_shot2));
    }
    return n;
}

int api_hero_set_pose(openrac_mod* mod, const openrac_pose* pose) {
    if (mod == nullptr || pose == nullptr || pose->size < sizeof(openrac_pose) || g_play == nullptr
        || g_play->hero_set_pose == nullptr || !holds_hook(mod, g_play->hero_update_hook)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_pose p{};
    std::memcpy(&p, &pose->level, sizeof p);
    return g_play->hero_set_pose(&p) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

// Any mod may turn the hero's joints (ports before took them only from the holder of his update
// hook): its offsets replace its own last ones and are composed with the other mods' (g_joint_sets).
int api_hero_set_joints(openrac_mod* mod, const openrac_joint_offset* joints, int count) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_moby == nullptr
        || g_joint_offsets == nullptr || count < 0 || count > OPENRAC_JOINT_OFFSETS_MAX
        || (count > 0 && joints == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t moby = g_play->hero_moby();
    if (moby == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    auto it = std::find_if(g_joint_sets.begin(), g_joint_sets.end(), [&](const JointSet& s) {
        return s.mod == mod;
    });
    if (count == 0) {
        if (it != g_joint_sets.end()) {
            g_joint_sets.erase(it);
        }
    } else {
        if (it == g_joint_sets.end()) {
            it = g_joint_sets.insert(g_joint_sets.end(), JointSet{mod, {}});
        }
        it->joints.assign(joints, joints + count);
    }
    std::size_t total = 0;
    for (const JointSet& s : g_joint_sets) {
        total += s.joints.size();
    }
    if (total > OPENRAC_JOINT_OFFSETS_MAX) {
        // More than the hero takes in all: this mod's last call is undone.
        std::erase_if(g_joint_sets, [&](const JointSet& s) { return s.mod == mod; });
        apply_joint_sets(moby);
        return OPENRAC_HOOK_UNKNOWN;
    }
    apply_joint_sets(moby);
    return OPENRAC_HOOK_OK;
}

int api_hero_hurt(openrac_mod* mod, int damage) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_hurt == nullptr) {
        return -1;
    }
    return g_play->hero_hurt(damage);
}

// The hero's weapons and their shots (interface 1.29).

// A mod's callback for the shots fired or ended.
template <class Fn>
struct ShotWatch {
    const openrac_mod* mod = nullptr;
    Fn fn = nullptr;
    void* user = nullptr;
};

std::vector<ShotWatch<openrac_weapon_fire_fn>> g_fire_watch;
std::vector<ShotWatch<openrac_shot_end_fn>> g_end_watch;
// The mod whose on_weapon_fire runs now, and the shot it is told: weapon_refire's only time.
const openrac_mod* g_fire_mod = nullptr;
std::uint32_t g_fire_serial = 0;

// A mod's look for one weapon's shots (weapon_look), the last set drawn; under g_props_lock.
struct WeaponLook {
    const openrac_mod* mod = nullptr;
    std::int32_t item = -1;
    std::uint32_t material = OPENRAC_MOBY_MATERIAL_NONE;
    std::uint32_t rgb = 0xFFFFFF;
};

std::vector<WeaponLook> g_weapon_looks;

std::unordered_map<std::uint32_t, ParticleLook> g_particle_looks;
// The most particle records tied at once: the game's pool holds 2048.
constexpr std::size_t kParticleLooksMax = 4096;
constexpr std::int32_t kWeaponItems = 64;

void game_weapon_fire(const openrac_game_weapon_shot* shot) {
    openrac_weapon_shot out{};
    out.size = sizeof out;
    std::memcpy(&out.item, shot, sizeof *shot);
    for (const auto& w : g_fire_watch) {
        g_fire_mod = w.mod;
        g_fire_serial = shot->serial;
        w.fn(w.user, &out);
    }
    g_fire_mod = nullptr;
    g_fire_serial = 0;
}

void game_shot_end(const openrac_game_shot_end* end) {
    openrac_shot_end out{};
    out.size = sizeof out;
    std::memcpy(&out.item, end, sizeof *end);
    for (const auto& w : g_end_watch) {
        w.fn(w.user, &out);
    }
}

// The look drawn on weapon `item`'s shots: the last one set, or none.
const WeaponLook* weapon_look_of(std::int32_t item) {
    for (auto l = g_weapon_looks.rbegin(); l != g_weapon_looks.rend(); ++l) {
        if (l->item == item) {
            return &*l;
        }
    }
    return nullptr;
}

int game_moby_look(gaddr moby, std::int32_t item) {
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const WeaponLook* look = weapon_look_of(item);
    if (look == nullptr) {
        return 0;
    }
    std::erase_if(g_materials, [&](const MaterialEntry& e) {
        return e.loads != loads || (e.mod == look->mod && e.material.moby == moby);
    });
    const auto own = std::count_if(g_materials.begin(), g_materials.end(), [&](const auto& e) {
        return e.mod == look->mod;
    });
    if (own >= OPENRAC_MOBY_MATERIALS_MAX) {
        return 0;
    }
    g_materials.push_back({look->mod, loads, {moby, look->material, look->rgb}, -1});
    return 1;
}

void game_particle_look(gaddr record, std::int32_t item) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const WeaponLook* look = item >= 0 ? weapon_look_of(item) : nullptr;
    if (look == nullptr) {
        g_particle_looks.erase(record);
        return;
    }
    if (g_particle_looks.size() >= kParticleLooksMax && !g_particle_looks.contains(record)) {
        return;
    }
    g_particle_looks.insert_or_assign(
        record, ParticleLook{loads_now(), MobyMaterial{record, look->material, look->rgb}}
    );
}

int api_on_weapon_fire(openrac_mod* mod, openrac_weapon_fire_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_play == nullptr || g_play->weapon_refire == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_fire_watch.push_back({mod, fn, user});
    openrac_game_weapon_fired = game_weapon_fire;
    return OPENRAC_HOOK_OK;
}

int api_weapon_refire(openrac_mod* mod, std::uint32_t serial, float yaw, float pitch) {
    if (mod == nullptr || mod != g_fire_mod || serial == 0 || serial != g_fire_serial
        || !std::isfinite(yaw) || !std::isfinite(pitch) || g_play == nullptr
        || g_play->weapon_refire == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::int32_t slot = g_play->weapon_refire(serial, yaw, pitch);
    return slot >= 0 ? slot : OPENRAC_HOOK_UNKNOWN;
}

int api_on_shot_end(openrac_mod* mod, openrac_shot_end_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_play == nullptr || g_play->weapon_refire == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_end_watch.push_back({mod, fn, user});
    openrac_game_shot_ended = game_shot_end;
    return OPENRAC_HOOK_OK;
}

int api_weapon_look(
    openrac_mod* mod, std::int32_t item, std::uint32_t material, std::uint32_t rgb
) {
    constexpr std::uint32_t kRgbMax = 0xFFFFFF;
    constexpr std::uint32_t kEnergyLevels = 0xF00;
    const std::uint32_t kind = OPENRAC_MOBY_MATERIAL_KIND(material);
    const std::uint32_t extra = material & ~0xFFu;
    const bool known =
        kind <= OPENRAC_MOBY_MATERIAL_ENERGY
        && (extra == 0 || (kind == OPENRAC_MOBY_MATERIAL_ENERGY && (extra & ~kEnergyLevels) == 0));
    if (mod == nullptr || item < 0 || item >= kWeaponItems || !known || rgb > kRgbMax
        || g_play == nullptr || g_play->weapon_refire == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::erase_if(g_weapon_looks, [&](const WeaponLook& l) {
        return l.mod == mod && l.item == item;
    });
    if (material != OPENRAC_MOBY_MATERIAL_NONE) {
        g_weapon_looks.push_back({mod, item, material, rgb});
    }
    const bool any = !g_weapon_looks.empty();
    openrac_game_moby_look = any ? game_moby_look : nullptr;
    openrac_game_particle_look = any ? game_particle_look : nullptr;
    if (!any) {
        g_particle_looks.clear();
    }
    return OPENRAC_HOOK_OK;
}

openrac_gaddr api_weapon_record(openrac_mod* mod, std::int32_t item) {
    if (mod == nullptr || g_play == nullptr || g_play->weapon_record == nullptr || item < 0
        || item >= kWeaponItems) {
        return 0;
    }
    return g_play->weapon_record(item);
}

// The weapons whose target picks a mod asked to take every enemy in range (weapon_ignore_culling):
// one row a mod and weapon; the game's is on while any row names the weapon.
struct CullingHold {
    const openrac_mod* mod;
    std::int32_t item;
};

std::vector<CullingHold> g_culling_holds;

bool culling_held(std::int32_t item) {
    return std::ranges::any_of(g_culling_holds, [&](const CullingHold& h) {
        return h.item == item;
    });
}

// Forgets `mod`'s shot watchers, looks and culling holds (nullptr: every mod's), and the game's
// calls for them once none is left.
void drop_weapon_shots(const openrac_mod* mod) {
    const auto mine = [&](const auto& w) {
        return mod == nullptr || w.mod == mod;
    };
    std::erase_if(g_fire_watch, mine);
    std::erase_if(g_end_watch, mine);
    std::vector<std::int32_t> released;
    for (const CullingHold& h : g_culling_holds) {
        if (mine(h)) {
            released.push_back(h.item);
        }
    }
    std::erase_if(g_culling_holds, mine);
    for (const std::int32_t item : released) {
        if (!culling_held(item) && g_play != nullptr && g_play->weapon_ignore_culling != nullptr) {
            g_play->weapon_ignore_culling(item, 0);
        }
    }
    if (g_fire_watch.empty()) {
        openrac_game_weapon_fired = nullptr;
    }
    if (g_end_watch.empty()) {
        openrac_game_shot_ended = nullptr;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::erase_if(g_weapon_looks, mine);
    if (g_weapon_looks.empty()) {
        openrac_game_moby_look = nullptr;
        openrac_game_particle_look = nullptr;
        g_particle_looks.clear();
    }
}

// openrac_api.weapon_ignore_culling. Since 1.30.
int api_weapon_ignore_culling(openrac_mod* mod, std::int32_t item, std::int32_t enabled) {
    if (mod == nullptr || item < 0 || item >= kWeaponItems || g_play == nullptr
        || g_play->weapon_ignore_culling == nullptr) {
        return OPENRAC_E_UNKNOWN;
    }
    std::erase_if(g_culling_holds, [&](const CullingHold& h) {
        return h.mod == mod && h.item == item;
    });
    if (enabled != 0) {
        g_culling_holds.push_back({mod, item});
    }
    if (g_play->weapon_ignore_culling(item, culling_held(item) ? 1 : 0) == 0) {
        std::erase_if(g_culling_holds, [&](const CullingHold& h) { return h.item == item; });
        return OPENRAC_E_UNKNOWN;
    }
    return OPENRAC_OK;
}

}  // namespace openrac::mods::detail
