// Generated from sdk/bindings/rac3.json by sdk/tools/gen_binding.py. Do not edit.
// SPDX-License-Identifier: MIT
//
// rac3's hook points as C++ types, for the SDK's C++ layer (openrac/mod.hpp, docs/port/MODS.md):
// each one's permanent id, its name and signature as openrac_api.hook takes them, and the type of a
// function of it. mod.hook<...>() asks for a hook point by its id. The same types by id are in
// openrac::rac3::hooks (rac3.player.take_damage: hooks::player::take_damage).
#pragma once

#include <cstdint>

namespace openrac::rac3 {

// Takes a hit's points from the hero's health (the s32 at 0x001A7430, hero +0x2850), scaled by the armour's factor and the help measure's, at least 1 and never below 0 (hero_damage); the hero's hit reaction calls it for each hit. Going Commando's rac2.player.take_damage.
struct Hero_TakeDamage {
    static constexpr const char* id = "rac3.player.take_damage";
    static constexpr const char* name = "Hero_TakeDamage";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Gives the player item id (item_give): its owned and new bytes (0x00142CA0, 0x00142D40, a byte per item), its row's starting ammo to a gun that has none, the vendor's list (Vendor_StockAdd) and the quick select. Givers, purchases and each level's start (Level_GiveItems) go through it.
struct Item_Give {
    static constexpr const char* id = "rac3.item.give";
    static constexpr const char* name = "Item_Give";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// The vendor's Mega upgrade of item id (weapon_buy_mega): its level set to 5, its ammo filled, its experience cleared. The vendor calls it after the price is paid; a replacement can carry out its own upgrade.
struct Item_Upgrade {
    static constexpr const char* id = "rac3.item.upgrade";
    static constexpr const char* name = "Item_Upgrade";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// Steps weapon id to its next level (weapon_level_up): its row (the item's byte at 0x001425C0) moves along the row's +0x46, its experience is raised to the old row's step and its ammo filled. A replacement that does not call the original blocks the step.
struct Weapon_LevelUp {
    static constexpr const char* id = "rac3.weapon.level_up";
    static constexpr const char* name = "Weapon_LevelUp";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Takes rounds of a gun's ammo (ammo_use; item -1: the one in hand): 1 when there were enough, 0 otherwise; always 1 for a gun without a maximum. Every gun calls it as it fires. Rac1's TakeAmmo.
struct Weapon_TakeAmmo {
    static constexpr const char* id = "rac3.weapon.take_ammo";
    static constexpr const char* name = "Weapon_TakeAmmo";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// Adds rounds to a gun's ammo (ammo_add, the words at 0x001427F0), capped at its row's maximum (+0x8A); returns what did not fit. Ammo pickups and the vendor call it.
struct Weapon_AddAmmo {
    static constexpr const char* id = "rac3.weapon.add_ammo";
    static constexpr const char* name = "Weapon_AddAmmo";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// The price in bolts of the vendor's row under the cursor (vendor_price): the gun's (+0x7C of its row), its level-5 row's for a Mega row, 9/10 of it with the discount. The vendor's screen and the purchase both ask it.
struct Vendor_Price {
    static constexpr const char* id = "rac3.vendor.price";
    static constexpr const char* name = "Vendor_Price";
    static constexpr const char* signature = "i32()";
    using fn = int32_t (*)();
};

// Fills the vendor's weapons rows from its stock (vendor_weapon_list): a rounds row for each owned gun that is not full, a special row, a row for each gun for sale (Mega rows in challenge mode), and keeps the cursor on its item. The vendor calls it as it opens and after a purchase.
struct Vendor_BuildList {
    static constexpr const char* id = "rac3.vendor.build_list";
    static constexpr const char* name = "Vendor_BuildList";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// Lists item id for the vendor (vendor_stock_add; vendor_stock at 0x001D54D8, a u16 per entry: the item, bit 15 owned, bit 14 not yet for sale). Item_Give calls it.
struct Vendor_StockAdd {
    static constexpr const char* id = "rac3.vendor.stock_add";
    static constexpr const char* name = "Vendor_StockAdd";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// An enemy's death (moby_killed): its class's experience to the gun that killed it (by its damage record), the hero's experience and its bolt drop (Bolt_SpawnPickups). Enemies call it as they die. Going Commando's rac2.moby.record_death.
struct Moby_RecordDeath {
    static constexpr const char* id = "rac3.moby.record_death";
    static constexpr const char* name = "Moby_RecordDeath";
    static constexpr const char* signature = "void(u32,i32,i32)";
    using fn = void (*)(uint32_t, int32_t, int32_t);
};

// Queues a damage record (a game address) for moby victim in the hit ring (damage_queue; 0x001EED00, 64 entries): a moby keeps one pending (+0xA8), which a hit at least as strong replaces and a weaker one only adds its flags to. The guns and the shock mod call it. Going Commando's rac2.combat.post_hit.
struct Moby_PostHit {
    static constexpr const char* id = "rac3.combat.post_hit";
    static constexpr const char* name = "Moby_PostHit";
    static constexpr const char* signature = "void(u32,u32)";
    using fn = void (*)(uint32_t, uint32_t);
};

// Spawns a burst of bolt coins from moby (bolts_drop): an amount from low to high split into the biggest coins that fit, each made by Bolt_SpawnPickup; flag 8 at most maximum coins (the rest banked, 0x0014269C). Enemies' drops (Moby_Killed) and the levels' scripted bursts come here. A replacement can change low and high, or make the coins itself.
struct Bolt_SpawnPickups {
    static constexpr const char* id = "rac3.bolt.spawn_pickups";
    static constexpr const char* name = "Bolt_SpawnPickups";
    static constexpr const char* signature = "void(u32,i32,i32,i32,i32,i32,f32)";
    using fn = void (*)(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, float);
};

// Makes one bolt coin worth value at position with velocity (game addresses of four floats each) from source (bolt_coin_spawn): its moby, or 0; 64 units or more from the camera the value is banked instead.
struct Bolt_SpawnPickup {
    static constexpr const char* id = "rac3.bolt.spawn_pickup";
    static constexpr const char* name = "Bolt_SpawnPickup";
    static constexpr const char* signature = "i32(i32,i32,i32,i32,i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
};

// A bolt coin collected (bolt_pickup_collect): its value added to the player's bolts (0x00142660), the bolt HUD shown for 180 frames and its sparkle. The coin's update calls it as it reaches the hero. A replacement can add more with openrac_api.bolts.
struct Bolt_Collect {
    static constexpr const char* id = "rac3.bolt.collect";
    static constexpr const char* name = "Bolt_Collect";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// The way to the next level (level_load, each level's copy): picks the transition for the level asked for, sets level_current (0x001D545C) and starts reading its files. Going Commando's rac2.level.load.
struct Level_Load {
    static constexpr const char* id = "rac3.level.load";
    static constexpr const char* name = "Level_Load";
    static constexpr const char* signature = "i32()";
    using fn = int32_t (*)();
};

// A level's start (give_level_items): the standing items (the suit, the camera options, the thruster-pack) through Item_Give, then the level's own. Every level calls it as it starts.
struct Level_GiveItems {
    static constexpr const char* id = "rac3.level.give_items";
    static constexpr const char* name = "Level_GiveItems";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// The hero's death over (hero_death_done: state 57 ended, or a fall out of the level): the death counted in the stats and the respawn asked for (0x001A71D2), which the level carries out at the end of the frame.
struct Hero_DeathDone {
    static constexpr const char* id = "rac3.player.death_done";
    static constexpr const char* name = "Hero_DeathDone";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// The hero's whole update each frame (hero_tick), from the level's frame in play (gameplay_update, game mode 0) after every moby's update (moby_update_all) and before the frame's draw: the pad, his state machine and movement, ledges, his collision and animation and the camera's copy of him. Florana (level 2) and levels 4, 8, 9 and 27 have copies of their own; together they are one hook point over the 34 story levels. While he is in another body (hero +0x25E4 not 0) it runs rac3.player.update_character in place of its own steps. Going Commando's rac2.player.update.
struct Hero_Update {
    static constexpr const char* id = "rac3.player.update";
    static constexpr const char* name = "Hero_Update";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// The hero's update while he is in another body (hero +0x25E4: 1 Clank, 2 Clank in Holostar Studios, 3 a disguise such as the Tyhrra-Guise, 5 Qwark in the vid-comics): rac3.player.update calls it each frame in place of its own steps, for that body's moby, state, collision and animation. Levels 2 and 9, level 23 and the vid-comics (30 to 36) have copies of their own; together they are one hook point over the 34 story levels.
struct Hero_UpdateCharacter {
    static constexpr const char* id = "rac3.player.update_character";
    static constexpr const char* name = "Hero_UpdateCharacter";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// Starts a hero animation (hero_anim_set): sequence from keyframe start_key, blended in from the pose he is in over blend ticks (negative: a blend curve, -1 is 11 ticks, -2 is 16). In a disguise (hero +0x25E4 is 3) the sequence goes through rac3.player.remap_animation first. The hero's states call it.
struct Hero_SetAnimation {
    static constexpr const char* id = "rac3.player.set_animation";
    static constexpr const char* name = "Hero_SetAnimation";
    static constexpr const char* signature = "void(i32,i32,f32)";
    using fn = void (*)(int32_t, int32_t, float);
};

// The body's sequence for a hero animation (hero_guise_anim_remap) on the disguise classes 0x1A4E and 0x173B (the class of the hero's moby, its +0xAA); any other class gets the animation back. rac3.player.set_animation calls it while he wears a disguise (hero +0x25E4 is 3).
struct Hero_RemapAnimation {
    static constexpr const char* id = "rac3.player.remap_animation";
    static constexpr const char* name = "Hero_RemapAnimation";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// What the hero holds in a hand slot (hero_item_switch; slot 0 is his hand): Square brings out the wrench, Circle (or R1 where the settings say R1 fires) the equipped gun, L2 and R2 with the wrench out the gun with its lock-on mod. The hero's states call it each frame; a replacement that does not call the original keeps what he holds.
struct Hero_SwitchItem {
    static constexpr const char* id = "rac3.player.switch_item";
    static constexpr const char* name = "Hero_SwitchItem";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Draws a player's screen markers (reticle_markers_draw, the levels' copy of the front end's), the ones queued this frame with reticle_marker_add (func_L01_003F6F20 on PAL), from the frame's draw after the level's update; with one view (0x001D6698 is 0) it then empties the queue. The 34 story levels.
struct Hud_DrawMarkers {
    static constexpr const char* id = "rac3.hud.markers.draw";
    static constexpr const char* name = "Hud_DrawMarkers";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Runs a ball damage query at an attacker's joint (ball_damage_pushed_at_joint), passing flags into the query before collision filtering. Returns the query's result. The PAL catalogue lists this function in levels 1, 2, 4-10, 12, 13, 16, 18-24 and 26-29; it does not list it in 3, 11, 14, 17 or 30-36.
struct Moby_MeleeAtJoint {
    static constexpr const char* id = "rac3.combat.melee_at_joint";
    static constexpr const char* name = "Moby_MeleeAtJoint";
    static constexpr const char* signature = "i32(u32,i32,f32,i32,f32,f32,i32,i32,i32)";
    using fn = int32_t (*)(uint32_t, int32_t, float, int32_t, float, float, int32_t, int32_t, int32_t);
};

// Updates an indexed collision sphere using a guest vector and two scale parameters. The moby collision pointer must refer to compatible writable sphere records. PAL catalogue lists a copy in all 49 levels; Veldin 0x00439790, Florana 0x004119C8, Marcadia 0x00448F10. Hooks must preserve this requirement when forwarding.
struct Moby_UpdateCollisionSphere {
    static constexpr const char* id = "rac3.moby.update_collision_sphere";
    static constexpr const char* name = "Moby_UpdateCollisionSphere";
    static constexpr const char* signature = "void(u32,i32,u32,f32,f32)";
    using fn = void (*)(uint32_t, int32_t, uint32_t, float, float);
};

namespace hooks {
    namespace bolt {
        using collect = rac3::Bolt_Collect;
        using spawn_pickup = rac3::Bolt_SpawnPickup;
        using spawn_pickups = rac3::Bolt_SpawnPickups;
    }  // namespace bolt
    namespace combat {
        using melee_at_joint = rac3::Moby_MeleeAtJoint;
        using post_hit = rac3::Moby_PostHit;
    }  // namespace combat
    namespace hud {
        namespace markers {
            using draw = rac3::Hud_DrawMarkers;
        }  // namespace markers
    }  // namespace hud
    namespace item {
        using give = rac3::Item_Give;
        using upgrade = rac3::Item_Upgrade;
    }  // namespace item
    namespace level {
        using give_items = rac3::Level_GiveItems;
        using load = rac3::Level_Load;
    }  // namespace level
    namespace moby {
        using record_death = rac3::Moby_RecordDeath;
        using update_collision_sphere = rac3::Moby_UpdateCollisionSphere;
    }  // namespace moby
    namespace player {
        using death_done = rac3::Hero_DeathDone;
        using remap_animation = rac3::Hero_RemapAnimation;
        using set_animation = rac3::Hero_SetAnimation;
        using switch_item = rac3::Hero_SwitchItem;
        using take_damage = rac3::Hero_TakeDamage;
        using update = rac3::Hero_Update;
        using update_character = rac3::Hero_UpdateCharacter;
    }  // namespace player
    namespace vendor {
        using build_list = rac3::Vendor_BuildList;
        using price = rac3::Vendor_Price;
        using stock_add = rac3::Vendor_StockAdd;
    }  // namespace vendor
    namespace weapon {
        using add_ammo = rac3::Weapon_AddAmmo;
        using level_up = rac3::Weapon_LevelUp;
        using take_ammo = rac3::Weapon_TakeAmmo;
    }  // namespace weapon
}  // namespace hooks

}  // namespace openrac::rac3
