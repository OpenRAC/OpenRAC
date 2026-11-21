// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "visuals": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_play_notice(openrac_mod* mod, const char* text) {
    if (mod == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_notice_lock);
    g_notice = text != nullptr ? text : "";
    return OPENRAC_HOOK_OK;
}

std::uint32_t api_sprite_tint(openrac_mod* mod, std::uint32_t rgb) {
    const auto previous = g_sprite_tint;
    if (mod) {
        g_sprite_tint = rgb;
    }
    return previous;
}

// The mod's prop `id` of this level load, null for one that is not; one gone with its level is let
// go. Under g_props_lock.
Prop* own_prop(const openrac_mod* mod, int id) {
    if (mod == nullptr || id < 0 || id >= static_cast<int>(g_props.size())) {
        return nullptr;
    }
    Prop& p = g_props[static_cast<std::size_t>(id)];
    if (p.mod != mod) {
        return nullptr;
    }
    if (p.loads != loads_now()) {
        p = Prop{};
        return nullptr;
    }
    return &p;
}

std::map<const openrac_mod*, std::map<std::string, openrac_gaddr>> g_guest_text;

openrac_gaddr api_guest_text(openrac_mod* mod, const char* text) {
    if (!mod || !text) {
        return 0;
    }
    const std::size_t length = strnlen(text, 1024);
    if (length >= 1024) {
        return 0;
    }
    static std::uint64_t generation = UINT64_MAX;
    const auto now = openrac_guest_cache_generation();
    if (generation != now) {
        g_guest_text.clear();
        generation = now;
    }
    auto& strings = g_guest_text[mod];
    if (const auto found = strings.find(text); found != strings.end()) {
        return found->second;
    }
    std::size_t bytes = length + 1;
    for (const auto& [s, address] : strings) {
        (void)address;
        bytes += s.size() + 1;
    }
    if (bytes > 65536) {
        return 0;
    }
    const auto address = openrac_guest_string(text, length + 1);
    strings.emplace(text, address);
    return address;
}

int api_pickup_animation(openrac_mod* mod, int kind, int item) {
    return mod && openrac_game_play_host && openrac_game_play_host->pickup_animation
               ? openrac_game_play_host->pickup_animation(kind, item)
               : 0;
}

int api_moby_visuals(openrac_mod* mod, const openrac_moby_visual* visuals, int count) {
    if (!mod || count < 0 || count > OPENRAC_MOBY_VISUALS_MAX || (count && !visuals)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int i = 0; i < count; ++i) {
        const auto& v = visuals[i];
        if (!v.moby || v.moby > UINT32_MAX - 0x100u || v.oclass < -1 || v.oclass >= 0x800
            || !std::isfinite(v.height) || v.height < 0 || v.height > 100) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::erase_if(g_visuals, [&](const auto& set) {
        return set.mod == mod || set.loads != loads_now();
    });
    if (count) {
        g_visuals.push_back({mod, loads_now(), {visuals, visuals + count}});
    }
    for (int i = 0; i < count; ++i) {
        if (visuals[i].oclass < 0) {
            continue;
        }
        if (std::find(g_visual_classes.begin(), g_visual_classes.end(), visuals[i].oclass)
            == g_visual_classes.end()) {
            g_visual_classes.push_back(visuals[i].oclass);
            ++g_visual_generation;
        }
    }
    return OPENRAC_HOOK_OK;
}

// Sets, changes or clears (OPENRAC_MOBY_MATERIAL_NONE) `mod`'s material on `moby` for this level
// load (interface 1.23, obsidian 1.24, energy 1.25); the renderer reads them with moby_materials. Writes no
// game memory.
int api_moby_material(
    openrac_mod* mod, openrac_gaddr moby, std::uint32_t material, std::uint32_t rgb
) {
    // A moby record is 0x100 bytes; one that would end past the address space is no moby.
    constexpr openrac_gaddr kMobyBytes = 0x100;
    constexpr std::uint32_t kRgbMax = 0xFFFFFF;
    // Energy carries its level in bits 8 to 11 (OPENRAC_MOBY_MATERIAL_ENERGY_AT); the others none.
    constexpr std::uint32_t kEnergyLevels = 0xF00;
    const std::uint32_t kind = OPENRAC_MOBY_MATERIAL_KIND(material);
    const std::uint32_t extra = material & ~0xFFu;
    const bool known =
        kind <= OPENRAC_MOBY_MATERIAL_ENERGY
        && (extra == 0 || (kind == OPENRAC_MOBY_MATERIAL_ENERGY && (extra & ~kEnergyLevels) == 0));
    // A call the interface does not describe changes nothing.
    if (mod == nullptr || moby == 0 || moby > UINT32_MAX - kMobyBytes || !known || rgb > kRgbMax) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    std::erase_if(g_materials, [&](const MaterialEntry& e) {
        return e.loads != loads || (e.mod == mod && e.material.moby == moby);
    });
    // NONE only clears; the others go last, so this mod's newest material is drawn over older ones.
    if (material == OPENRAC_MOBY_MATERIAL_NONE) {
        return OPENRAC_HOOK_OK;
    }
    const auto own = std::count_if(g_materials.begin(), g_materials.end(), [&](const auto& e) {
        return e.mod == mod;
    });
    // A mod that never clears its materials stops at the cap, not at the memory's end.
    if (own >= OPENRAC_MOBY_MATERIALS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_materials.push_back({mod, loads, {moby, material, rgb}, -1});
    return OPENRAC_HOOK_OK;
}

int api_prop_create(openrac_mod* mod, const char* mesh_path) {
    std::int32_t level = -1;
    if (mod == nullptr || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::filesystem::path mesh = mod_file(mod, mesh_path);
    if (mesh.empty() || mesh.extension() != ".glb") {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const std::uint32_t loads = loads_now();
    for (Prop& p : g_props) {
        if (p.mod != nullptr && p.loads != loads) {
            p = Prop{};
        }
    }
    for (std::size_t i = 0; i < g_props.size(); ++i) {
        if (g_props[i].mod != nullptr) {
            continue;
        }
        g_props[i] = Prop{mod, mesh, loads};
        // A mesh no level has read yet: the scenery is read again with it.
        if (std::find(g_prop_meshes.begin(), g_prop_meshes.end(), mesh) == g_prop_meshes.end()) {
            g_prop_meshes.push_back(mesh);
            ++g_prop_generation;
        }
        return static_cast<int>(i);
    }
    return OPENRAC_HOOK_UNKNOWN;
}

int api_prop_pose(openrac_mod* mod, int id, const openrac_prop_pose* pose) {
    const auto finite = [](const float* v, int n) {
        return std::all_of(v, v + n, [](float f) { return std::isfinite(f); });
    };
    if (pose == nullptr || pose->size < sizeof(openrac_prop_pose)
        || !(
            (pose->flags & OPENRAC_PROP_MATRIX) != 0
                ? finite(pose->matrix, 16)
                : finite(pose->position, 3) && finite(pose->rotation, 4) && finite(&pose->scale, 1)
        )
        || ((pose->flags & OPENRAC_PROP_TINT) != 0 && !finite(pose->tint, 4))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if ((pose->flags & OPENRAC_PROP_MATRIX) != 0) {
        std::copy(pose->matrix, pose->matrix + 16, p->matrix.begin());
    } else {
        // The rotation's matrix (a unit quaternion), each axis scaled, then the place.
        const float x = pose->rotation[0], y = pose->rotation[1], z = pose->rotation[2],
                    w = pose->rotation[3], s = pose->scale;
        const float m[9] = {
            1 - 2 * (y * y + z * z),
            2 * (x * y + z * w),
            2 * (x * z - y * w),
            2 * (x * y - z * w),
            1 - 2 * (x * x + z * z),
            2 * (y * z + x * w),
            2 * (x * z + y * w),
            2 * (y * z - x * w),
            1 - 2 * (x * x + y * y),
        };
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                p->matrix[static_cast<std::size_t>(4 * c + r)] = m[3 * c + r] * s;
            }
            p->matrix[static_cast<std::size_t>(4 * c + 3)] = 0.0f;
        }
        p->matrix[12] = pose->position[0];
        p->matrix[13] = pose->position[1];
        p->matrix[14] = pose->position[2];
        p->matrix[15] = 1.0f;
    }
    if ((pose->flags & OPENRAC_PROP_TINT) != 0) {
        std::copy(pose->tint, pose->tint + 4, p->tint.begin());
        p->blend = pose->blend <= OPENRAC_PROP_ADD ? static_cast<int>(pose->blend) : 0;
    } else {
        p->tint = {1.0f, 1.0f, 1.0f, 1.0f};
        p->blend = 0;
    }
    p->posed = true;
    p->attached_moby = 0;
    p->attached_joint = -1;
    return OPENRAC_HOOK_OK;
}

// A prop fixed to a moby's joint (interface 1.18): the renderer places it each frame
// (frontend/frame.cpp), `matrix` kept as its offset in the joint's frame.
int api_prop_attach(
    openrac_mod* mod,
    int id,
    std::int32_t slot,
    std::int32_t joint,
    std::uint32_t flags,
    const float offset[16]
) {
    const gaddr moby = slot >= 0 ? moby_address(slot) : 0;
    // An attach point of the hero's (as hero_held names them): the game's own list says its joint.
    if ((flags & OPENRAC_ATTACH_HELD) != 0) {
        joint = g_play != nullptr && g_play->attach_point_joint != nullptr && joint >= 0
                    ? g_play->attach_point_joint(moby, joint)
                    : -1;
    }
    if (moby == 0 || joint < 0
        || (offset != nullptr
            && !std::all_of(offset, offset + 16, [](float f) { return std::isfinite(f); }))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (offset != nullptr) {
        std::copy(offset, offset + 16, p->matrix.begin());
    } else {
        p->matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }
    if (p->attached_moby != moby || p->attached_joint != joint) {
        p->was_placed = false;  // not drawn on this joint yet
    }
    p->attached_moby = moby;
    p->attached_joint = joint;
    p->posed = true;
    return OPENRAC_HOOK_OK;
}

// Where a prop was drawn last (interface 1.18): its pose, or, fixed to a joint, the matrix the
// renderer drew it with (prop_placed).
int api_prop_frame(openrac_mod* mod, int id, float out[16]) {
    if (out == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const Prop* p = own_prop(mod, id);
    if (p == nullptr || !p->posed || (p->attached_moby != 0 && !p->was_placed)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::array<float, 16>& m = p->attached_moby != 0 ? p->placed : p->matrix;
    std::copy(m.begin(), m.end(), out);
    return OPENRAC_HOOK_OK;
}

int api_prop_hide(openrac_mod* mod, int id, int hidden) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    p->hidden = hidden != 0;
    return OPENRAC_HOOK_OK;
}

int api_prop_remove(openrac_mod* mod, int id) {
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    *p = Prop{};
    return OPENRAC_HOOK_OK;
}

int api_haze_set(openrac_mod* mod, const openrac_haze* patches, int count) {
    std::int32_t level = -1;
    if (mod == nullptr || count < 0 || (count > 0 && patches == nullptr)
        || (count > 0 && !in_level(phase_now(&level)))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_haze_lock);
    const std::uint32_t loads = loads_now();
    std::size_t others = 0;
    for (const Haze& h : g_haze) {
        if (h.mod != mod && h.loads == loads) {
            others += h.patches.size();
        }
    }
    if (others + static_cast<std::size_t>(count) > OPENRAC_HAZE_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int i = 0; i < count; ++i) {
        const openrac_haze& p = patches[i];
        if (!std::all_of(p.position, p.position + 3, [](float v) { return std::isfinite(v); })
            || !std::isfinite(p.radius) || !std::isfinite(p.strength) || p.radius < 0.0f
            || p.strength < 0.0f) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    std::erase_if(g_haze, [&](const Haze& h) { return h.mod == mod || h.loads != loads; });
    if (count > 0) {
        g_haze.push_back({mod, loads, std::vector<openrac_haze>(patches, patches + count)});
    }
    return OPENRAC_HOOK_OK;
}

int api_hud_text_set(openrac_mod* mod, const openrac_hud_text* lines, int count) {
    std::int32_t level = -1;
    if (mod == nullptr || count < 0 || (count > 0 && lines == nullptr)
        || (count > 0 && !in_level(phase_now(&level)))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (int i = 0; i < count; ++i) {
        const openrac_hud_text& l = lines[i];
        const bool ended =
            std::find(std::begin(l.text), std::end(l.text), '\0') != std::end(l.text);
        const auto within = [](float v, float lo, float hi) {
            return std::isfinite(v) && v >= lo && v <= hi;
        };
        if (!ended || !within(l.x, 0.0f, 1.0f) || !within(l.y, 0.0f, 1.0f)
            || !within(l.scale, 0.25f, 4.0f)) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    const std::lock_guard<std::mutex> lock(g_hud_lock);
    const std::uint32_t loads = loads_now();
    std::size_t others = 0;
    for (const HudTexts& h : g_hud) {
        if (h.mod != mod && h.loads == loads) {
            others += h.lines.size();
        }
    }
    if (others + static_cast<std::size_t>(count) > OPENRAC_HUD_TEXTS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::erase_if(g_hud, [&](const HudTexts& h) { return h.mod == mod || h.loads != loads; });
    if (count > 0) {
        g_hud.push_back({mod, loads, std::vector<openrac_hud_text>(lines, lines + count)});
    }
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
