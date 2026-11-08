// Generated from sdk/bindings/rac2.json by sdk/tools/gen_binding.py. Do not edit.
// SPDX-License-Identifier: MIT
//
// rac2's hook points as C++ types, for the SDK's C++ layer (openrac/mod.hpp, docs/port/MODS.md):
// each one's permanent id, its name and signature as openrac_api.hook takes them, and the type of a
// function of it. mod.hook<...>() asks for a hook point by its id. The same types by id are in
// openrac::rac2::hooks (rac2.player.take_damage: hooks::player::take_damage).
#pragma once

#include <cstdint>

namespace openrac::rac2 {

// Takes a hit's points from the hero's health (the hero at 0x189E20 on NTSC-U, health at +0x24CC), after the armour's scale; the hero's state machine calls it for each hit.
struct Hero_TakeDamage {
    static constexpr const char* id = "rac2.player.take_damage";
    static constexpr const char* name = "Hero_TakeDamage";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// The pending hit for a moby from the hit ring (the damage, f32 at +0x2C; its source at +0x38), or 0: what an enemy's damage path reads.
struct Moby_GetHitMessage {
    static constexpr const char* id = "rac2.combat.get_hit";
    static constexpr const char* name = "Moby_GetHitMessage";
    static constexpr const char* signature = "u32(u32,u32,i32)";
    using fn = uint32_t (*)(uint32_t, uint32_t, int32_t);
};

// Takes rounds of a weapon's ammo (item -1: the one in hand); a weapon with no capacity has endless ammo. Rac1's TakeAmmo.
struct Weapon_TakeAmmo {
    static constexpr const char* id = "rac2.weapon.take_ammo";
    static constexpr const char* name = "Weapon_TakeAmmo";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// A moby's death recorded: its experience, the bestiary and its bolt drop (the moby's s16 at +0xB4); crates call it too.
struct Moby_RecordDeath {
    static constexpr const char* id = "rac2.moby.record_death";
    static constexpr const char* name = "Moby_RecordDeath";
    static constexpr const char* signature = "void(u32,i32,i32)";
    using fn = void (*)(uint32_t, int32_t, int32_t);
};

// Loads a level (new, or resumed after a death); it calls the gameplay file's loader.
struct Level_Load {
    static constexpr const char* id = "rac2.level.load";
    static constexpr const char* name = "Level_Load";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// Eases the hero's speed (+0x1B4) toward his target speed (+0x1B0) each tick, at up when speeding up and down when slowing; rac1's approach_speed.
struct Hero_ApproachSpeed {
    static constexpr const char* id = "rac2.player.approach_speed";
    static constexpr const char* name = "Hero_ApproachSpeed";
    static constexpr const char* signature = "void(f32,f32)";
    using fn = void (*)(float, float);
};

// Adds the nanotech levels' steps of health to *count; the recount of the most health each tick adds the boosts after it (the HUD's experience bar calls it too, ignoring count).
struct Hero_NanotechSteps {
    static constexpr const char* id = "rac2.player.nanotech_steps";
    static constexpr const char* name = "Hero_NanotechSteps";
    static constexpr const char* signature = "i32(u32,u32,u32)";
    using fn = int32_t (*)(uint32_t, uint32_t, uint32_t);
};

namespace hooks {
    namespace combat {
        using get_hit = rac2::Moby_GetHitMessage;
    }  // namespace combat
    namespace level {
        using load = rac2::Level_Load;
    }  // namespace level
    namespace moby {
        using record_death = rac2::Moby_RecordDeath;
    }  // namespace moby
    namespace player {
        using approach_speed = rac2::Hero_ApproachSpeed;
        using nanotech_steps = rac2::Hero_NanotechSteps;
        using take_damage = rac2::Hero_TakeDamage;
    }  // namespace player
    namespace weapon {
        using take_ammo = rac2::Weapon_TakeAmmo;
    }  // namespace weapon
}  // namespace hooks

}  // namespace openrac::rac2
