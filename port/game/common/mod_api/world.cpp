// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "world": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

openrac_game_pose pose_of(const openrac_moby& m) {
    openrac_game_pose p{};
    std::copy(m.position, m.position + 3, p.position);
    std::copy(m.rotation, m.rotation + 3, p.rotation);
    p.scale = m.scale;
    p.seq = m.seq;
    p.frame = m.frame;
    p.next_frame = m.next_frame;
    p.anim_time = m.anim_time;
    p.anim_speed = m.anim_speed;
    p.item = -1;
    return p;
}

int api_moby_list(openrac_moby* out, int max) {
    if (g_play == nullptr || g_play->mobys == nullptr || max < 0) {
        return 0;
    }
    std::vector<openrac_game_moby_state> states(static_cast<std::size_t>(max));
    const int n = g_play->mobys(states.data(), max);
    for (int i = 0; i < std::min(n, max) && out != nullptr; ++i) {
        out[i].size = sizeof(openrac_moby);
        std::memcpy(
            &out[i].slot, &states[static_cast<std::size_t>(i)], sizeof(openrac_game_moby_state)
        );
        if (driver(out[i].slot) != nullptr) {
            out[i].flags |= OPENRAC_MOBY_DRIVEN;
        }
    }
    return n;
}

int api_moby_slot(int key) {
    return g_play != nullptr && g_play->moby_slot != nullptr ? g_play->moby_slot(key) : -1;
}

int api_moby_drive(openrac_mod* mod, int slot, int on) {
    if (mod == nullptr || moby_address(slot) == 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const openrac_mod* now = driver(slot);
    if (now != nullptr && now != mod) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_drives[static_cast<std::size_t>(slot)] = on != 0 ? Drive{mod, g_play->loads()} : Drive{};
    return OPENRAC_HOOK_OK;
}

int api_moby_pose(openrac_mod* mod, int slot, const openrac_moby* pose) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || pose == nullptr || pose->size < sizeof(openrac_moby)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const openrac_game_pose p = pose_of(*pose);
    g_play->moby_pose(m, &p);
    return OPENRAC_HOOK_OK;
}

int api_moby_kill(openrac_mod* mod, int slot) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->moby_kill(m);
    if (static_cast<std::size_t>(slot) < kSlots) {
        g_drives[static_cast<std::size_t>(slot)] = Drive{};
    }
    return OPENRAC_HOOK_OK;
}

int api_moby_spawn(openrac_mod* mod, int oclass, const openrac_moby* pose) {
    if (mod == nullptr || g_play == nullptr || g_play->moby_spawn == nullptr || pose == nullptr
        || pose->size < sizeof(openrac_moby)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int slot = g_play->moby_spawn(oclass);
    if (slot < 0 || static_cast<std::size_t>(slot) >= kSlots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_drives[static_cast<std::size_t>(slot)] = Drive{mod, g_play->loads()};
    api_moby_pose(mod, slot, pose);
    return slot;
}

// Copies of a level's placed enemies (interface 1.25): the host's check and copy, by slot.
int api_moby_copy_check(openrac_mod* mod, int slot) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || g_play->moby_copy_check == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int why = g_play->moby_copy_check(m);
    return why >= 0 ? why : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_copy(openrac_mod* mod, int slot, int index) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || index < 0 || g_play->moby_copy == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int copy = g_play->moby_copy(m, index);
    if (copy < 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // The game's moby, never driven: a mod that drove a moby gone from this slot lets go of it.
    if (static_cast<std::size_t>(copy) < kSlots) {
        g_drives[static_cast<std::size_t>(copy)] = Drive{};
    }
    return copy;
}

// The pool of spawned mobys (interface 1.27): the host's raise and status.
int api_moby_pool_raise(openrac_mod* mod, int extra) {
    if (mod == nullptr || extra < 0 || g_play == nullptr || g_play->moby_pool_raise == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->moby_pool_raise(extra);
}

int api_moby_pool_status(openrac_mod* mod, openrac_moby_pool* out) {
    if (mod == nullptr || out == nullptr || out->size < sizeof(openrac_moby_pool)
        || g_play == nullptr || g_play->moby_pool_status == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::int32_t pool[5] = {};
    if (g_play->moby_pool_status(pool) <= 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->capacity = pool[0];
    out->live = pool[1];
    out->free = pool[2];
    out->extra = pool[3];
    out->requested = pool[4];
    return OPENRAC_HOOK_OK;
}

int api_on_moby_update(openrac_mod* mod, openrac_moby_update_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_gate.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_gate = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_on_moby_hit(openrac_mod* mod, openrac_hit_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_hits.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_hits = {mod, fn, user};
    return OPENRAC_HOOK_OK;
}

int api_moby_hit(openrac_mod* mod, const openrac_hit* hit) {
    const gaddr victim = hit != nullptr ? moby_address(hit->victim) : 0;
    if (mod == nullptr || victim == 0 || hit->size < sizeof(openrac_hit) || g_play == nullptr
        || g_play->post_hit == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->post_hit(
        victim, moby_address(hit->attacker), hit->mask, hit->damage, hit->position, hit->velocity
    );
    return OPENRAC_HOOK_OK;
}

int api_play_eyes(openrac_mod* mod, const float* xyz, int count) {
    if (mod == nullptr || g_play == nullptr || g_play->eyes == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->eyes(xyz, xyz != nullptr ? std::clamp(count, 0, OPENRAC_EYES_MAX) : 0);
    return OPENRAC_HOOK_OK;
}

int api_moby_slot_of(openrac_gaddr moby);

int api_moby_slot_of(openrac_gaddr moby) {
    return g_play != nullptr && g_play->slot_of != nullptr
                   && moby_address(g_play->slot_of(moby)) == moby
               ? g_play->slot_of(moby)
               : -1;
}

// The game's own segment query for a mod (interface 1.17): the host's, the moby hit named by its
// slot.
int api_collision_line(
    const float from[3],
    const float to[3],
    std::uint32_t flags,
    std::int32_t ignore,
    openrac_line_hit* out
) {
    if (g_play == nullptr || g_play->collision_line == nullptr || from == nullptr || to == nullptr
        || out == nullptr || out->size < sizeof(openrac_line_hit)
        || (flags & (OPENRAC_LINE_LEVEL | OPENRAC_LINE_MOBYS)) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int k = 0; k < 3; ++k) {
        if (!std::isfinite(from[k]) || !std::isfinite(to[k])) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    const gaddr skip = ignore >= 0 ? moby_address(ignore) : 0;
    float point[3] = {}, normal[3] = {};
    std::int32_t surface = -1;
    std::uint32_t moby = 0;
    const int hit = g_play->collision_line(
        from,
        to,
        (flags & OPENRAC_LINE_LEVEL) != 0,
        (flags & OPENRAC_LINE_MOBYS) != 0,
        skip,
        point,
        normal,
        &surface,
        &moby
    );
    const std::uint32_t size = out->size;
    *out = openrac_line_hit{};
    out->size = size;
    out->hit = hit != 0;
    out->surface = -1;
    out->moby = -1;
    if (hit != 0) {
        for (int k = 0; k < 3; ++k) {
            out->point[k] = point[k];
            out->normal[k] = normal[k];
        }
        out->surface = surface;
        out->moby = moby != 0 ? api_moby_slot_of(moby) : -1;
    }
    return OPENRAC_HOOK_OK;
}

int api_moby_hide(openrac_mod* mod, int slot, int hidden) {
    const gaddr m = moby_address(slot);
    if (mod == nullptr || m == 0 || g_play->moby_hide == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->moby_hide(m, hidden != 0 ? 1 : 0);
    return OPENRAC_HOOK_OK;
}

int api_moby_deletion(int slot, openrac_moby_deletion* out) {
    if (out == nullptr || out->size < sizeof(openrac_moby_deletion) || g_play == nullptr
        || g_play->moby_deletion == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_moby_deletion d{};
    if (g_play->moby_deletion(slot, &d) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->size = sizeof(openrac_moby_deletion);
    std::memcpy(&out->by, &d, sizeof d);
    return OPENRAC_HOOK_OK;
}

int api_moby_run(int slot, openrac_moby_run* out) {
    if (out == nullptr || out->size < sizeof(openrac_moby_run) || g_play == nullptr
        || g_play->moby_run == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_moby_run r{};
    if (g_play->moby_run(slot, &r) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->size = sizeof(openrac_moby_run);
    std::memcpy(&out->runs, &r, sizeof r);
    return OPENRAC_HOOK_OK;
}

int api_moby_list2(openrac_moby2* out, int max) {
    if (g_play == nullptr || g_play->mobys == nullptr || max < 0) {
        return 0;
    }
    std::vector<openrac_game_moby_state> states(static_cast<std::size_t>(max));
    const int n = g_play->mobys(states.data(), max);
    for (int i = 0; i < std::min(n, max) && out != nullptr; ++i) {
        const openrac_game_moby_state& s = states[static_cast<std::size_t>(i)];
        openrac_moby2& m = out[i];
        m.size = sizeof(openrac_moby2);
        m.slot = s.slot;
        m.key = s.key;
        m.oclass = s.oclass;
        m.flags = s.flags | (driver(s.slot) != nullptr ? OPENRAC_MOBY_DRIVEN : 0u);
        m.state = s.state;
        m.mission = g_play->moby_mission != nullptr ? g_play->moby_mission(s.slot) : -1;
        std::copy(std::begin(s.position), std::end(s.position), m.position);
        std::copy(std::begin(s.rotation), std::end(s.rotation), m.rotation);
        m.scale = s.scale;
        m.seq = s.seq;
        m.frame = s.frame;
        m.next_frame = s.next_frame;
        m.anim_time = s.anim_time;
        m.anim_speed = s.anim_speed;
    }
    return n;
}

int game_metal_detect(std::int32_t slot, std::int32_t key) {
    return g_metal.fn != nullptr ? g_metal.fn(g_metal.user, slot, key) : 1;
}

int api_on_metal_detect(openrac_mod* mod, openrac_metal_detect_fn fn, void* user) {
    if (mod == nullptr || fn == nullptr || mod != g_loading_mod) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (g_metal.mod != nullptr) {
        return OPENRAC_HOOK_TAKEN;
    }
    g_metal = {mod, fn, user};
    openrac_game_metal_detect = game_metal_detect;
    return OPENRAC_HOOK_OK;
}

int api_level_line(const float from[3], const float to[3], int* blocked) {
    if (from == nullptr || to == nullptr || blocked == nullptr || g_play == nullptr
        || g_play->level_line == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->level_line(from, to, blocked) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_collision_triangles(
    const float lo[3], const float hi[3], openrac_collision_triangle* out, int max
) {
    if (g_play == nullptr || g_play->collision_triangles == nullptr || lo == nullptr
        || hi == nullptr) {
        return 0;
    }
    return g_play->collision_triangles(lo, hi, out, out != nullptr ? std::max(max, 0) : 0);
}

// A moby made live and a status put on one (interface 1.29).
int api_moby_spawn_live(openrac_mod* mod, const openrac_spawn_live* spawn) {
    if (mod == nullptr || spawn == nullptr || spawn->size < sizeof(openrac_spawn_live)
        || g_play == nullptr || g_play->moby_spawn_live == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int k = 0; k < 3; ++k) {
        if (!std::isfinite(spawn->position[k]) || !std::isfinite(spawn->velocity[k])) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    openrac_game_spawn_live in{};
    std::memcpy(&in, &spawn->oclass, sizeof in);
    const std::int32_t slot = g_play->moby_spawn_live(&in);
    return slot >= 0 ? slot : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_status(openrac_mod* mod, std::int32_t slot, std::uint32_t status, std::int32_t ticks) {
    if (mod == nullptr || g_play == nullptr || g_play->moby_status == nullptr
        || status > OPENRAC_MOBY_STATUS_SLOW_THIRD) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->moby_status(slot, status, ticks) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_uid(int slot) {
    return g_play != nullptr && g_play->moby_uid != nullptr ? g_play->moby_uid(slot) : -1;
}

int api_moby_set_joints(openrac_mod* mod, int slot, const openrac_joint_offset* joints, int count) {
    std::int32_t level = -1;
    if (mod == nullptr || g_play == nullptr || g_joint_offsets == nullptr || count < 0
        || count > OPENRAC_JOINT_OFFSETS_MAX || (count > 0 && joints == nullptr)
        || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const gaddr moby = moby_address(slot);
    if (moby == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return set_joint_set(mod, moby, joints, count);
}

// moby_set_joints with each joint's pose set, not composed: its number carries kJointPoseSet, which
// the host's joint_offsets reads (mods.h).
int api_moby_set_joint_poses(
    openrac_mod* mod, int slot, const openrac_joint_offset* joints, int count
) {
    if (count < 0 || count > OPENRAC_JOINT_OFFSETS_MAX || (count > 0 && joints == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::vector<openrac_joint_offset> set(joints, joints + count);
    for (openrac_joint_offset& j : set) {
        if (j.joint >= kJointPoseSet) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        j.joint |= kJointPoseSet;
    }
    return api_moby_set_joints(mod, slot, set.data(), count);
}

int api_moby_joint_frame(int slot, int joint, float out[12]) {
    const gaddr moby = moby_address(slot);
    if (out == nullptr || moby == 0 || joint < 0 || g_joint_frame == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_joint_frame(moby, joint, out) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_moby_joint_position(int slot, int joint, float out[3]) {
    float frame[12];
    if (out == nullptr || api_moby_joint_frame(slot, joint, frame) != OPENRAC_HOOK_OK) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::copy(frame + 9, frame + 12, out);
    return OPENRAC_HOOK_OK;
}

int api_tie_hide(openrac_mod* mod, int tie, int hidden) {
    std::int32_t level = -1;
    if (mod == nullptr || tie < 0 || tie > 0xFFFF || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_air_lock);
    const std::uint32_t loads = loads_now();
    std::erase_if(g_ties_hidden, [&](const TieHidden& h) {
        return h.loads != loads || (h.mod == mod && h.tie == tie);
    });
    if (hidden != 0) {
        g_ties_hidden.push_back({mod, loads, tie});
    }
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
