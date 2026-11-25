// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "visuals": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"
#include "renderer/textures/sharp_cells.h"

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

// Whether moby_material may draw `material` with `rgb`: a kind the port has, with a level only on
// energy and paint, an accent and the effects bit only on paint.
bool moby_material_known(std::uint32_t material, std::uint32_t rgb) {
    constexpr std::uint32_t kRgbMax = 0xFFFFFF;
    // Energy (1.25) and paint (1.34) carry a level in bits 8 to 11; the others none.
    constexpr std::uint32_t kLevels = 0xF00;
    // Paint (1.38) may carry an accent: its flag and colour in bits 12 to 24, the colour only with
    // the flag.
    constexpr std::uint32_t kAccentColour = 0xFFF000;
    constexpr std::uint32_t kAccentBits = kAccentColour | OPENRAC_MOBY_MATERIAL_PAINT_ACCENT;
    const std::uint32_t kind = OPENRAC_MOBY_MATERIAL_KIND(material);
    std::uint32_t extra = material & ~0xFFu;
    const bool levelled =
        kind == OPENRAC_MOBY_MATERIAL_ENERGY || kind == OPENRAC_MOBY_MATERIAL_PAINT;
    const bool accent_ok =
        kind == OPENRAC_MOBY_MATERIAL_PAINT
        && ((extra & OPENRAC_MOBY_MATERIAL_PAINT_ACCENT) != 0 || (extra & kAccentColour) == 0);
    if (accent_ok) {
        extra &= ~kAccentBits;
    }
    // Paint (1.39) may draw the moby's effects in its colour too.
    if (kind == OPENRAC_MOBY_MATERIAL_PAINT) {
        extra &= ~OPENRAC_MOBY_MATERIAL_PAINT_EFFECTS;
    }
    return kind <= OPENRAC_MOBY_MATERIAL_PAINT
           && (extra == 0 || (levelled && (extra & ~kLevels) == 0)) && rgb <= kRgbMax;
}

// Sets, changes or clears (OPENRAC_MOBY_MATERIAL_NONE) `mod`'s material on `moby` for this level
// load (interface 1.23, obsidian 1.24, energy 1.25, paint 1.34); the renderer reads them with
// moby_materials. Writes no game memory.
int api_moby_material(
    openrac_mod* mod, openrac_gaddr moby, std::uint32_t material, std::uint32_t rgb
) {
    // A moby record is 0x100 bytes; one that would end past the address space is no moby.
    constexpr openrac_gaddr kMobyBytes = 0x100;
    // A call the interface does not describe changes nothing.
    if (mod == nullptr || moby == 0 || moby > UINT32_MAX - kMobyBytes
        || !moby_material_known(material, rgb)) {
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

// Sets or clears (OPENRAC_MOBY_MATERIAL_NONE) `mod`'s material on every moby of `oclass` in `scope`
// for this level load (interface 1.34); the renderer reads them with class_materials. Writes no game
// memory.
int api_moby_material_class(
    openrac_mod* mod, int oclass, std::uint32_t scope, std::uint32_t material, std::uint32_t rgb
) {
    constexpr std::uint32_t kScopes = OPENRAC_MODEL_WORLD | OPENRAC_MODEL_MENU;
    // A call the interface does not describe changes nothing.
    if (mod == nullptr || oclass < 0 || scope == 0 || (scope & ~kScopes) != 0
        || !moby_material_known(material, rgb)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t loads = loads_now();
    const std::lock_guard<std::mutex> lock(g_props_lock);
    // Each scope the call names, on its own.
    for (const std::uint32_t bit : {OPENRAC_MODEL_WORLD, OPENRAC_MODEL_MENU}) {
        if ((scope & bit) == 0) {
            continue;
        }
        const auto held = g_class_materials.find({oclass, bit});
        // NONE takes this mod's off; another mod's, or one of an older level load, stays or goes
        // as the last set would.
        if (material == OPENRAC_MOBY_MATERIAL_NONE) {
            if (held != g_class_materials.end()
                && (held->second.mod == mod || held->second.loads != loads)) {
                g_class_materials.erase(held);
            }
            continue;
        }
        g_class_materials[{oclass, bit}] = ClassMaterial{mod, loads, material, rgb};
    }
    return OPENRAC_HOOK_OK;
}

// A mesh a prop of level load `loads` is made of: the front end holds it in the scene it has. The
// first mesh of a level load starts the list again (the last one's props are all gone, and their
// meshes are not needed), and a mesh the load has asked for already is not added twice. Under
// g_props_lock.
void note_prop_mesh(std::uint32_t loads, const std::filesystem::path& mesh) {
    if (g_prop_meshes_loads != loads) {
        g_prop_meshes_loads = loads;

        if (!g_prop_meshes.empty()) {
            g_prop_meshes.clear();
            ++g_prop_generation;
        }
    }

    // A mesh no prop of this level load has asked for yet: the front end loads it into the scene
    // it has (no new read of the level).
    if (std::find(g_prop_meshes.begin(), g_prop_meshes.end(), mesh) == g_prop_meshes.end()) {
        g_prop_meshes.push_back(mesh);
        ++g_prop_generation;
    }
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
    // A new mesh of this level load is refused past OPENRAC_PROP_MESHES_MAX, before a prop is made.
    const bool same_load = g_prop_meshes_loads == loads;
    const bool listed =
        same_load
        && std::find(g_prop_meshes.begin(), g_prop_meshes.end(), mesh) != g_prop_meshes.end();
    if (!listed && same_load && g_prop_meshes.size() >= OPENRAC_PROP_MESHES_MAX) {
        log::warn(
            "mods: {} asked for prop mesh {} past the {} meshes a level load may use: not made",
            mod_id(mod),
            mesh.filename().string(),
            OPENRAC_PROP_MESHES_MAX
        );
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (std::size_t i = 0; i < g_props.size(); ++i) {
        if (g_props[i].mod != nullptr) {
            continue;
        }
        g_props[i] = Prop{mod, mesh, loads};
        note_prop_mesh(loads, mesh);
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

// Interface 1.33: the prop's joint matrices, replacing its last (none: its bind pose). All of them or
// none: a value that is not a number, or too many joints, leaves the skin as it was.
int api_prop_set_skin(
    openrac_mod* mod, int id, const float* joint_matrices, std::uint32_t joint_count
) {
    if (joint_count > OPENRAC_PROP_JOINTS_MAX || (joint_count > 0 && joint_matrices == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::shared_ptr<std::vector<std::array<float, 16>>> skin;
    if (joint_count > 0) {
        const std::size_t count = static_cast<std::size_t>(joint_count);
        for (std::size_t i = 0; i < count * 16; ++i) {
            if (!std::isfinite(joint_matrices[i])) {
                return OPENRAC_HOOK_UNKNOWN;
            }
        }
        skin = std::make_shared<std::vector<std::array<float, 16>>>(count);
        std::memcpy(skin->data(), joint_matrices, count * sizeof(std::array<float, 16>));
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    p->skin = std::move(skin);
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

// The mod's model `handle` of this level load, null for one that is not; one gone with its level is
// let go. Under g_props_lock.
Model* own_model(const openrac_mod* mod, int handle) {
    if (mod == nullptr || handle < 0 || handle >= static_cast<int>(g_models.size())) {
        return nullptr;
    }
    Model& m = g_models[static_cast<std::size_t>(handle)];
    if (m.mod != mod) {
        return nullptr;
    }
    if (m.loads != loads_now()) {
        m = Model{};
        return nullptr;
    }
    return &m;
}

int api_model_create(openrac_mod* mod, const char* mesh_path) {
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
    int free_handle = -1;
    for (std::size_t i = 0; i < g_models.size(); ++i) {
        Model& m = g_models[i];
        if (m.mod != nullptr && m.loads != loads) {
            m = Model{};
        }
        if (m.mod == mod && m.mesh == mesh) {
            return static_cast<int>(i);
        }
        if (m.mod == nullptr && free_handle < 0) {
            free_handle = static_cast<int>(i);
        }
    }
    if (free_handle < 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_models[static_cast<std::size_t>(free_handle)] = Model{mod, mesh, loads};
    note_prop_mesh(loads, mesh);
    return free_handle;
}

int api_moby_model_class(
    openrac_mod* mod, int oclass, std::uint32_t scope, const openrac_moby_model* m
) {
    constexpr std::uint32_t kScopes = OPENRAC_MODEL_WORLD | OPENRAC_MODEL_MENU;
    if (mod == nullptr || oclass < 0 || scope == 0 || (scope & ~kScopes) != 0 || m == nullptr
        || m->size < sizeof(openrac_moby_model) || m->flags != 0
        || !std::all_of(m->matrix, m->matrix + 16, [](float f) { return std::isfinite(f); })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const std::uint32_t loads = loads_now();
    // One mod per class and scope: a live override of another mod in any of the scopes asked for.
    for (const std::uint32_t bit : {OPENRAC_MODEL_WORLD, OPENRAC_MODEL_MENU}) {
        const auto held = g_class_models.find({oclass, bit});
        if ((scope & bit) != 0 && held != g_class_models.end()) {
            if (held->second.loads != loads) {
                g_class_models.erase(held);
            } else if (held->second.mod != mod) {
                return OPENRAC_HOOK_TAKEN;
            }
        }
    }
    const Model* model = m->model >= 0 ? own_model(mod, m->model) : nullptr;
    if (m->model >= 0 && model == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    for (const std::uint32_t bit : {OPENRAC_MODEL_WORLD, OPENRAC_MODEL_MENU}) {
        if ((scope & bit) == 0) {
            continue;
        }
        if (model == nullptr) {
            g_class_models.erase({oclass, bit});
            continue;
        }
        ClassModel& c = g_class_models[{oclass, bit}];
        c.mod = mod;
        c.loads = loads;
        c.mesh = model->mesh;
        std::copy(m->matrix, m->matrix + 16, c.matrix.begin());
    }
    return OPENRAC_HOOK_OK;
}

int api_moby_model_slot(openrac_mod* mod, int slot, const openrac_moby_model* m) {
    if (mod == nullptr || g_play == nullptr || m == nullptr || m->size < sizeof(openrac_moby_model)
        || m->flags != 0
        || !std::all_of(m->matrix, m->matrix + 16, [](float f) { return std::isfinite(f); })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const gaddr address = moby_address(slot);
    const int oclass = address != 0 ? MobyClasses().of(slot) : -1;
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const auto now = g_slot_models.find(slot);
    // An override the moby it was set on has left (its slot freed or made again) is no one's.
    bool held = false;
    if (now != g_slot_models.end()) {
        held = now->second.loads == loads_now() && now->second.address == address
               && now->second.oclass == oclass;
        if (!held) {
            g_slot_models.erase(now);
        } else if (now->second.mod != mod) {
            return OPENRAC_HOOK_TAKEN;
        }
    }
    if (m->model < 0) {
        if (held) {
            g_slot_models.erase(slot);
        }
        return OPENRAC_HOOK_OK;
    }
    const Model* model = own_model(mod, m->model);
    if (model == nullptr || address == 0 || oclass < 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    SlotModel& s = g_slot_models[slot];
    s.mod = mod;
    s.address = address;
    s.oclass = oclass;
    s.loads = loads_now();
    s.mesh = model->mesh;
    std::copy(m->matrix, m->matrix + 16, s.matrix.begin());
    s.hidden = false;
    return OPENRAC_HOOK_OK;
}

// Interface 1.33: the moby at `address`, whether or not the level's list holds it. Its class is the game's
// own at that address (moby_class_at), so an override goes when the class there changes.
int api_moby_model_at(openrac_mod* mod, std::uint32_t address, const openrac_moby_model* m) {
    if (mod == nullptr || g_play == nullptr || m == nullptr || m->size < sizeof(openrac_moby_model)
        || (m->flags & ~OPENRAC_MODEL_HIDE_NATIVE) != 0
        || ((m->flags & OPENRAC_MODEL_HIDE_NATIVE) != 0 && m->model >= 0)
        || !std::all_of(m->matrix, m->matrix + 16, [](float f) { return std::isfinite(f); })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::int32_t oclass =
        address != 0 && g_play->moby_class_at != nullptr ? g_play->moby_class_at(address) : -1;
    const std::lock_guard<std::mutex> lock(g_props_lock);
    const auto now = g_address_models.find(address);
    // An override whose moby's class changed, or whose level is gone, is no one's.
    bool held = false;
    if (now != g_address_models.end()) {
        held = now->second.loads == loads_now() && now->second.oclass == oclass;
        if (!held) {
            g_address_models.erase(now);
        } else if (now->second.mod != mod) {
            return OPENRAC_HOOK_TAKEN;
        }
    }
    if (m->model < 0 && (m->flags & OPENRAC_MODEL_HIDE_NATIVE) == 0) {
        if (held) {
            g_address_models.erase(address);
        }
        return OPENRAC_HOOK_OK;
    }
    if ((m->flags & OPENRAC_MODEL_HIDE_NATIVE) != 0) {
        if (address == 0 || oclass < 0) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        SlotModel& s = g_address_models[address];
        s.mod = mod;
        s.address = address;
        s.oclass = oclass;
        s.loads = loads_now();
        s.mesh.clear();
        std::copy(m->matrix, m->matrix + 16, s.matrix.begin());
        s.hidden = true;
        return OPENRAC_HOOK_OK;
    }
    const Model* model = own_model(mod, m->model);
    if (model == nullptr || oclass < 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    SlotModel& s = g_address_models[address];
    s.mod = mod;
    s.address = address;
    s.oclass = oclass;
    s.loads = loads_now();
    s.mesh = model->mesh;
    std::copy(m->matrix, m->matrix + 16, s.matrix.begin());
    s.hidden = false;
    return OPENRAC_HOOK_OK;
}

// Both particle calls: the play phase and the game's host. Returns the answer of the call.
int particle_start_ex(const openrac_game_particle_spawn& spawn) {
    std::int32_t level = -1;
    if (g_play == nullptr || g_play->particle_spawn_ex == nullptr
        || phase_now(&level) != OPENRAC_PLAY_PLAYING) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int made = g_play->particle_spawn_ex(&spawn);
    if (made > 0) {
        return OPENRAC_HOOK_OK;
    }
    return made == 0 ? OPENRAC_HOOK_FULL : OPENRAC_HOOK_UNKNOWN;
}

bool particle_finite(const float v[3]) {
    return std::all_of(v, v + 3, [](float x) { return std::isfinite(x); });
}

int api_particle_spawn_ex(openrac_mod* mod, const openrac_particle_spawn* spawn) {
    if (mod == nullptr || spawn == nullptr || spawn->size < sizeof(openrac_particle_spawn)
        || spawn->kind > OPENRAC_PARTICLE_SPARK || !particle_finite(spawn->position)
        || !particle_finite(spawn->velocity) || !std::isfinite(spawn->particle_size)
        || spawn->particle_size <= 0.0f || spawn->particle_size > OPENRAC_PARTICLE_SIZE_MAX
        || spawn->life_ticks < 1 || spawn->life_ticks > OPENRAC_PARTICLE_LIFE_MAX
        || (spawn->flags & ~OPENRAC_PARTICLE_CHILDREN) != 0 || spawn->texture < -1
        || spawn->texture > OPENRAC_PARTICLE_BYTE_MAX || spawn->blend < -1
        || spawn->blend > OPENRAC_PARTICLE_BYTE_MAX || !std::isfinite(spawn->rot)
        || std::fabs(spawn->rot) > OPENRAC_PARTICLE_ROT_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    openrac_game_particle_spawn g{};
    g.kind = spawn->kind;
    std::copy(spawn->position, spawn->position + 3, g.position);
    std::copy(spawn->velocity, spawn->velocity + 3, g.velocity);
    g.size = spawn->particle_size;
    g.life_ticks = spawn->life_ticks;
    g.color = spawn->color;
    g.color_end = spawn->color_end;
    g.flags = spawn->flags;
    g.texture = spawn->texture;
    g.blend = spawn->blend;
    g.rot = spawn->rot;
    return particle_start_ex(g);
}

// openrac_api.particle_texture_register (1.33): the texture is checked here, then the game's renderer
// stages it. Played levels only, as the particle calls.
int api_particle_texture_register(openrac_mod* mod, const openrac_particle_texture* texture) {
    std::int32_t level = -1;
    if (mod == nullptr || texture == nullptr || texture->size < sizeof(openrac_particle_texture)
        || g_play == nullptr || g_play->particle_texture_register == nullptr
        || phase_now(&level) != OPENRAC_PLAY_PLAYING) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int id = g_play->particle_texture_register(texture->indices, texture->clut);
    if (id >= OPENRAC_PARTICLE_TEXTURE_FIRST) {
        return id;
    }
    return id == 0 ? OPENRAC_HOOK_FULL : OPENRAC_HOOK_UNKNOWN;
}

int api_hud_hide(openrac_mod* mod, int hidden) {
    if (mod == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_hud_lock);
    const std::uint32_t loads = loads_now();
    std::erase_if(g_hud_hidden, [&](const auto& h) { return h.first == mod || h.second != loads; });
    if (hidden != 0) {
        g_hud_hidden.emplace_back(mod, loads);
    }
    return OPENRAC_HOOK_OK;
}

int api_prop_animate(openrac_mod* mod, int id, int animation, float seconds, int loop) {
    if (!std::isfinite(seconds)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_props_lock);
    Prop* p = own_prop(mod, id);
    if (p == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    p->animation = std::max(animation, -1);
    p->seconds = seconds;
    p->loop = loop != 0;
    return OPENRAC_HOOK_OK;
}

int api_scene_air(openrac_mod* mod, const openrac_air* air) {
    std::int32_t level = -1;
    if (mod == nullptr || !in_level(phase_now(&level))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_air_lock);
    if (air == nullptr) {
        if (g_air && g_air->mod == mod) {
            g_air.reset();
        }
        return OPENRAC_HOOK_OK;
    }
    if (air->size < sizeof(openrac_air)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const auto in_unit = [](float v) {
        return std::isfinite(v) && v >= 0.0f && v <= 1.0f;
    };
    bool ok = in_unit(air->weight) && in_unit(air->fog_thickness) && std::isfinite(air->fog_near)
              && std::isfinite(air->fog_far) && air->fog_near >= 0.0f
              && air->fog_far > air->fog_near;
    for (int i = 0; i < 3; ++i) {
        ok = ok && in_unit(air->fog_colour[i]) && in_unit(air->background[i]);
    }
    if (!ok) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_air = AirSet{mod, loads_now(), *air};
    return OPENRAC_HOOK_OK;
}

namespace {

// What one sharp_cells call takes at most (openrac_api.sharp_cells): pages, cells a page, bytes of
// colours and pictures in all; a texture's side and a picture's (texels).
constexpr int kSharpPagesMax = 64;
constexpr int kSharpCellsMax = 4096;
constexpr std::size_t kSharpBytesMax = std::size_t{64} << 20;
constexpr int kSharpTextureMax = 1024;
constexpr int kSharpPictureMax = 4096;
constexpr int kSharpScaleMax = 16;
// The longest struct a mod built for a later interface may give: room to grow, not to overflow.
constexpr std::uint32_t kSharpStructMax = 1024;

// Element `i` of a mod's array of structs that start with their size, `stride` bytes apart (the
// first one's size: a mod built for a later interface has longer ones).
template <class T>
const T& sized_element(const T* first, std::uint32_t stride, int i) {
    return *reinterpret_cast<const T*>(
        reinterpret_cast<const unsigned char*>(first)
        + static_cast<std::size_t>(stride) * static_cast<std::size_t>(i)
    );
}

// The stride of a mod's array of `T`: its first element's size, 0 when too short or too long.
template <class T>
std::uint32_t sized_stride(const T* first) {
    const std::uint32_t size = first->size;
    return size >= sizeof(T) && size <= kSharpStructMax ? size : 0;
}

// The mod's page `p` as the renderer keeps it, or false for one out of range; `bytes` counts its
// colours and pictures.
bool sharp_page(const openrac_sharp_page& p, renderer::sharp_cells::Page& out, std::size_t& bytes) {
    if (p.width < 1 || p.width > kSharpTextureMax || p.height < 1 || p.height > kSharpTextureMax
        || p.scale < 1 || p.scale > kSharpScaleMax || p.width * p.scale > kSharpPictureMax
        || p.height * p.scale > kSharpPictureMax || p.count < 1 || p.count > kSharpCellsMax
        || p.cells == nullptr) {
        return false;
    }
    const std::uint32_t stride = sized_stride(p.cells);
    if (stride == 0) {
        return false;
    }
    out = {p.width, p.height, p.scale, {}};
    out.cells.reserve(static_cast<std::size_t>(p.count));
    for (int i = 0; i < p.count; ++i) {
        const openrac_sharp_cell& c = sized_element(p.cells, stride, i);
        if (c.size != stride || c.x < 0 || c.y < 0 || c.w < 1 || c.h < 1 || c.x > p.width - c.w
            || c.y > p.height - c.h || c.rgb == nullptr || c.sharp == nullptr) {
            return false;
        }
        const std::size_t colours =
            static_cast<std::size_t>(c.w) * static_cast<std::size_t>(c.h) * 3;
        const std::size_t sharp = colours * static_cast<std::size_t>(p.scale * p.scale);
        bytes += colours + sharp;
        if (bytes > kSharpBytesMax) {
            return false;
        }
        out.cells.push_back(
            {c.x, c.y, c.w, c.h, {c.rgb, c.rgb + colours}, {c.sharp, c.sharp + sharp}}
        );
    }
    return true;
}

}  // namespace

// openrac_api.sharp_cells. Since 1.36.
int api_sharp_cells(openrac_mod* mod, const openrac_sharp_page* pages, int count) {
    if (mod == nullptr || count < 0 || count > kSharpPagesMax || (count > 0 && pages == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::uint32_t stride = count > 0 ? sized_stride(pages) : 0;
    if (count > 0 && stride == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // Every page read before any is set: a refusal leaves the mod's pages as they were.
    std::vector<renderer::sharp_cells::Page> kept(static_cast<std::size_t>(count));
    std::size_t bytes = 0;
    for (int i = 0; i < count; ++i) {
        const openrac_sharp_page& p = sized_element(pages, stride, i);
        if (p.size != stride || !sharp_page(p, kept[static_cast<std::size_t>(i)], bytes)) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    renderer::sharp_cells::set(mod, std::move(kept));
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
