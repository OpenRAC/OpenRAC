/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: Up Your Arsenal's hook points (docs/port/MODS.md, section
 * 4), as rac2.h gives Going Commando's: for each, its name, its signature as
 * openrac_api.hook takes it, and the type of a replacement. Each disc's table
 * of them is the `hook` rows of port/game/rac3-<version>/libraries.tsv; every
 * hook point here is on both discs (PAL "pal" and NTSC-U "ntsc"). The addresses
 * in the comments are PAL's (games/rac3's names); a level's function is one
 * hook point in every level that has it. Each also has a permanent id
 * (sdk/bindings/rac3.json, rac3.ids.lock), which openrac_api.hook takes as
 * well; an id reuses Going Commando's <area>.<name> where the job is the same,
 * so a mod for both games reads alike (openrac-mods docs/MULTI_GAME.md).
 *
 * Parameters are as the port translates the game: u32 is a game address or an
 * unsigned word, i32 an int, f32 a float.
 *
 * Each hook point has three names here: OPENRAC_RAC3_HOOK_<name>, the string
 * openrac_api.hook takes; OPENRAC_RAC3_SIG_<name>, its signature string; and
 * openrac_rac3_<name>, the type of a function that can replace it.
 * OPENRAC_RAC3_HOOK takes them together. On a port that lacks one,
 * openrac_api.hook returns OPENRAC_HOOK_UNKNOWN. The first came with
 * SDK 1.27.1.
 */
#ifndef OPENRAC_GAMES_RAC3_H
#define OPENRAC_GAMES_RAC3_H

#include "openrac/openrac_mod.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hero_TakeDamage (func_L01_004DFEA0, rac3.player.take_damage): Takes a hit's
 * points from the hero's health (the s32 at 0x001A7430, hero +0x2850), scaled
 * by the armour's factor and the help measure's, at least 1 and never below 0
 * (hero_damage); the hero's hit reaction calls it for each hit. Going
 * Commando's rac2.player.take_damage. */
#define OPENRAC_RAC3_HOOK_Hero_TakeDamage "Hero_TakeDamage"
#define OPENRAC_RAC3_SIG_Hero_TakeDamage "void(i32)"
typedef void (*openrac_rac3_Hero_TakeDamage)(int32_t amount);

/* Item_Give (func_L01_00419188, rac3.item.give): Gives the player item id
 * (item_give): its owned and new bytes (0x00142CA0, 0x00142D40, a byte per
 * item), its row's starting ammo to a gun that has none, the vendor's list
 * (Vendor_StockAdd) and the quick select. Givers, purchases and each level's
 * start (Level_GiveItems) go through it. */
#define OPENRAC_RAC3_HOOK_Item_Give "Item_Give"
#define OPENRAC_RAC3_SIG_Item_Give "void(i32)"
typedef void (*openrac_rac3_Item_Give)(int32_t id);

/* Item_Upgrade (func_L01_00419D48, rac3.item.upgrade): The vendor's Mega
 * upgrade of item id (weapon_buy_mega): its level set to 5, its ammo filled,
 * its experience cleared. The vendor calls it after the price is paid; a
 * replacement can carry out its own upgrade. */
#define OPENRAC_RAC3_HOOK_Item_Upgrade "Item_Upgrade"
#define OPENRAC_RAC3_SIG_Item_Upgrade "i32(i32)"
typedef int32_t (*openrac_rac3_Item_Upgrade)(int32_t id);

/* Weapon_LevelUp (func_L01_00418C90, rac3.weapon.level_up): Steps weapon id to
 * its next level (weapon_level_up): its row (the item's byte at 0x001425C0)
 * moves along the row's +0x46, its experience is raised to the old row's step
 * and its ammo filled. A replacement that does not call the original blocks the
 * step. */
#define OPENRAC_RAC3_HOOK_Weapon_LevelUp "Weapon_LevelUp"
#define OPENRAC_RAC3_SIG_Weapon_LevelUp "void(i32)"
typedef void (*openrac_rac3_Weapon_LevelUp)(int32_t id);

/* Weapon_TakeAmmo (func_L01_005118D0, rac3.weapon.take_ammo): Takes rounds of a
 * gun's ammo (ammo_use; item -1: the one in hand): 1 when there were enough, 0
 * otherwise; always 1 for a gun without a maximum. Every gun calls it as it
 * fires. Rac1's TakeAmmo. */
#define OPENRAC_RAC3_HOOK_Weapon_TakeAmmo "Weapon_TakeAmmo"
#define OPENRAC_RAC3_SIG_Weapon_TakeAmmo "i32(i32,i32)"
typedef int32_t (*openrac_rac3_Weapon_TakeAmmo)(int32_t item, int32_t amount);

/* Weapon_AddAmmo (func_L01_00511958, rac3.weapon.add_ammo): Adds rounds to a
 * gun's ammo (ammo_add, the words at 0x001427F0), capped at its row's maximum
 * (+0x8A); returns what did not fit. Ammo pickups and the vendor call it. */
#define OPENRAC_RAC3_HOOK_Weapon_AddAmmo "Weapon_AddAmmo"
#define OPENRAC_RAC3_SIG_Weapon_AddAmmo "i32(i32,i32)"
typedef int32_t (*openrac_rac3_Weapon_AddAmmo)(int32_t item, int32_t amount);

/* Vendor_Price (func_L01_004BA830, rac3.vendor.price): The price in bolts of
 * the vendor's row under the cursor (vendor_price): the gun's (+0x7C of its
 * row), its level-5 row's for a Mega row, 9/10 of it with the discount. The
 * vendor's screen and the purchase both ask it. */
#define OPENRAC_RAC3_HOOK_Vendor_Price "Vendor_Price"
#define OPENRAC_RAC3_SIG_Vendor_Price "i32()"
typedef int32_t (*openrac_rac3_Vendor_Price)(void);

/* Vendor_BuildList (func_L01_004BAA20, rac3.vendor.build_list): Fills the
 * vendor's weapons rows from its stock (vendor_weapon_list): a rounds row for
 * each owned gun that is not full, a special row, a row for each gun for sale
 * (Mega rows in challenge mode), and keeps the cursor on its item. The vendor
 * calls it as it opens and after a purchase. */
#define OPENRAC_RAC3_HOOK_Vendor_BuildList "Vendor_BuildList"
#define OPENRAC_RAC3_SIG_Vendor_BuildList "i32(i32)"
typedef int32_t (*openrac_rac3_Vendor_BuildList)(int32_t mode);

/* Vendor_StockAdd (func_L01_00419248, rac3.vendor.stock_add): Lists item id for
 * the vendor (vendor_stock_add; vendor_stock at 0x001D54D8, a u16 per entry:
 * the item, bit 15 owned, bit 14 not yet for sale). Item_Give calls it. */
#define OPENRAC_RAC3_HOOK_Vendor_StockAdd "Vendor_StockAdd"
#define OPENRAC_RAC3_SIG_Vendor_StockAdd "i32(i32)"
typedef int32_t (*openrac_rac3_Vendor_StockAdd)(int32_t id);

/* Moby_RecordDeath (func_L01_004443E8, rac3.moby.record_death): An enemy's
 * death (moby_killed): its class's experience to the gun that killed it (by its
 * damage record), the hero's experience and its bolt drop (Bolt_SpawnPickups).
 * Enemies call it as they die. Going Commando's rac2.moby.record_death. */
#define OPENRAC_RAC3_HOOK_Moby_RecordDeath "Moby_RecordDeath"
#define OPENRAC_RAC3_SIG_Moby_RecordDeath "void(u32,i32,i32)"
typedef void (*openrac_rac3_Moby_RecordDeath)(uint32_t moby, int32_t hit,
                                              int32_t killer);

/* Moby_PostHit (func_L01_00446978, rac3.combat.post_hit): Queues a damage
 * record (a game address) for moby victim in the hit ring (damage_queue;
 * 0x001EED00, 64 entries): a moby keeps one pending
 * (+0xA8), which a hit at least as strong replaces and a weaker one only adds
 * its flags to. The guns and the shock mod call it. Going Commando's
 * rac2.combat.post_hit. */
#define OPENRAC_RAC3_HOOK_Moby_PostHit "Moby_PostHit"
#define OPENRAC_RAC3_SIG_Moby_PostHit "void(u32,u32)"
typedef void (*openrac_rac3_Moby_PostHit)(uint32_t victim, uint32_t record);

/* Bolt_SpawnPickups (func_L01_0044D360, rac3.bolt.spawn_pickups): Spawns a
 * burst of bolt coins from moby (bolts_drop): an amount from low to high split
 * into the biggest coins that fit, each made by Bolt_SpawnPickup; flag 8 at
 * most maximum coins (the rest banked, 0x0014269C). Enemies' drops
 * (Moby_Killed) and the levels' scripted bursts come here. A replacement can
 * change low and high, or make the coins itself. */
#define OPENRAC_RAC3_HOOK_Bolt_SpawnPickups "Bolt_SpawnPickups"
#define OPENRAC_RAC3_SIG_Bolt_SpawnPickups "void(u32,i32,i32,i32,i32,i32,f32)"
typedef void (*openrac_rac3_Bolt_SpawnPickups)(uint32_t moby, int32_t low,
                                               int32_t high, int32_t flags,
                                               int32_t surface, int32_t maximum,
                                               float height);

/* Bolt_SpawnPickup (func_L01_002F8F20, rac3.bolt.spawn_pickup): Makes one bolt
 * coin worth value at position with velocity (game addresses of four floats
 * each) from source (bolt_coin_spawn): its moby, or 0; 64 units or more from
 * the camera the value is banked instead. */
#define OPENRAC_RAC3_HOOK_Bolt_SpawnPickup "Bolt_SpawnPickup"
#define OPENRAC_RAC3_SIG_Bolt_SpawnPickup "i32(i32,i32,i32,i32,i32,i32)"
typedef int32_t (*openrac_rac3_Bolt_SpawnPickup)(int32_t source,
                                                 int32_t position,
                                                 int32_t velocity,
                                                 int32_t flags, int32_t value,
                                                 int32_t counted);

/* Bolt_Collect (func_L01_002F88E0, rac3.bolt.collect): A bolt coin collected
 * (bolt_pickup_collect): its value added to the player's bolts (0x00142660),
 * the bolt HUD shown for 180 frames and its sparkle. The coin's update calls it
 * as it reaches the hero. A replacement can add more with openrac_api.bolts. */
#define OPENRAC_RAC3_HOOK_Bolt_Collect "Bolt_Collect"
#define OPENRAC_RAC3_SIG_Bolt_Collect "i32(i32)"
typedef int32_t (*openrac_rac3_Bolt_Collect)(int32_t moby);

/* Level_Load (func_L01_004D7658, rac3.level.load): The way to the next level
 * (level_load, each level's copy): picks the transition for the level asked
 * for, sets level_current (0x001D545C) and starts reading its files. Going
 * Commando's rac2.level.load. */
#define OPENRAC_RAC3_HOOK_Level_Load "Level_Load"
#define OPENRAC_RAC3_SIG_Level_Load "i32()"
typedef int32_t (*openrac_rac3_Level_Load)(void);

/* Level_GiveItems (func_L01_004227D8, rac3.level.give_items): A level's start
 * (give_level_items): the standing items (the suit, the camera options, the
 * thruster-pack) through Item_Give, then the level's own. Every level calls it
 * as it starts. */
#define OPENRAC_RAC3_HOOK_Level_GiveItems "Level_GiveItems"
#define OPENRAC_RAC3_SIG_Level_GiveItems "void()"
typedef void (*openrac_rac3_Level_GiveItems)(void);

/* Hero_DeathDone (func_L01_004EDE10, rac3.player.death_done): The hero's death
 * over (hero_death_done: state 57 ended, or a fall out of the level): the death
 * counted in the stats and the respawn asked for (0x001A71D2), which the level
 * carries out at the end of the frame. */
#define OPENRAC_RAC3_HOOK_Hero_DeathDone "Hero_DeathDone"
#define OPENRAC_RAC3_SIG_Hero_DeathDone "void()"
typedef void (*openrac_rac3_Hero_DeathDone)(void);

/* Takes a hook point by its name here, its signature and a replacement of its
 * type; the original is written to `original` (OPENRAC_HOOK_OK, or the reason).
 * Only during openrac_mod_load. */
#define OPENRAC_RAC3_HOOK(api, mod, point, replacement, original)              \
  (api)->hook((mod), OPENRAC_RAC3_HOOK_##point, OPENRAC_RAC3_SIG_##point,      \
              (void *)(replacement), (void **)(original))

/* ---- generated by sdk/tools/gen_binding.py from sdk/bindings/rac3.json: do not edit ---- */
/* Hook points added since this header was last written by hand, from the binding table.
 * As the others: on a port that lacks one, openrac_api.hook returns OPENRAC_HOOK_UNKNOWN. */

/* Hero_Update: The hero's whole update each frame (hero_tick), from the level's frame in play
 * (gameplay_update, game mode 0) after every moby's update (moby_update_all) and before the
 * frame's draw: the pad, his state machine and movement, ledges, his collision and animation
 * and the camera's copy of him. Florana (level 2) and levels 4, 8, 9 and 27 have copies of
 * their own; together they are one hook point over the 34 story levels. While he is in another
 * body (hero +0x25E4 not 0) it runs rac3.player.update_character in place of its own steps.
 * Going Commando's rac2.player.update. */
#define OPENRAC_RAC3_HOOK_Hero_Update "Hero_Update"
#define OPENRAC_RAC3_SIG_Hero_Update "void()"
typedef void (*openrac_rac3_Hero_Update)(void);

/* Hero_UpdateCharacter: The hero's update while he is in another body (hero +0x25E4: 1 Clank, 2
 * Clank in Holostar Studios, 3 a disguise such as the Tyhrra-Guise, 5 Qwark in the vid-comics):
 * rac3.player.update calls it each frame in place of its own steps, for that body's moby,
 * state, collision and animation. Levels 2 and 9, level 23 and the vid-comics (30 to 36) have
 * copies of their own; together they are one hook point over the 34 story levels. */
#define OPENRAC_RAC3_HOOK_Hero_UpdateCharacter "Hero_UpdateCharacter"
#define OPENRAC_RAC3_SIG_Hero_UpdateCharacter "void()"
typedef void (*openrac_rac3_Hero_UpdateCharacter)(void);

/* Hero_SetAnimation: Starts a hero animation (hero_anim_set): sequence from keyframe start_key,
 * blended in from the pose he is in over blend ticks (negative: a blend curve, -1 is 11 ticks,
 * -2 is 16). In a disguise (hero +0x25E4 is 3) the sequence goes through
 * rac3.player.remap_animation first. The hero's states call it. */
#define OPENRAC_RAC3_HOOK_Hero_SetAnimation "Hero_SetAnimation"
#define OPENRAC_RAC3_SIG_Hero_SetAnimation "void(i32,i32,f32)"
typedef void (*openrac_rac3_Hero_SetAnimation)(int32_t sequence, int32_t start_key, float blend);

/* Hero_RemapAnimation: The body's sequence for a hero animation (hero_guise_anim_remap) on the
 * disguise classes 0x1A4E and 0x173B (the class of the hero's moby, its +0xAA); any other class
 * gets the animation back. rac3.player.set_animation calls it while he wears a disguise (hero
 * +0x25E4 is 3). */
#define OPENRAC_RAC3_HOOK_Hero_RemapAnimation "Hero_RemapAnimation"
#define OPENRAC_RAC3_SIG_Hero_RemapAnimation "i32(i32)"
typedef int32_t (*openrac_rac3_Hero_RemapAnimation)(int32_t animation);

/* Hero_SwitchItem: What the hero holds in a hand slot (hero_item_switch; slot 0 is his hand):
 * Square brings out the wrench, Circle (or R1 where the settings say R1 fires) the equipped
 * gun, L2 and R2 with the wrench out the gun with its lock-on mod. The hero's states call it
 * each frame; a replacement that does not call the original keeps what he holds. */
#define OPENRAC_RAC3_HOOK_Hero_SwitchItem "Hero_SwitchItem"
#define OPENRAC_RAC3_SIG_Hero_SwitchItem "void(i32)"
typedef void (*openrac_rac3_Hero_SwitchItem)(int32_t slot);

/* Hud_DrawMarkers: Draws a player's screen markers (reticle_markers_draw, the levels' copy of
 * the front end's), the ones queued this frame with reticle_marker_add (func_L01_003F6F20 on
 * PAL), from the frame's draw after the level's update; with one view (0x001D6698 is 0) it then
 * empties the queue. The 34 story levels. */
#define OPENRAC_RAC3_HOOK_Hud_DrawMarkers "Hud_DrawMarkers"
#define OPENRAC_RAC3_SIG_Hud_DrawMarkers "void(i32)"
typedef void (*openrac_rac3_Hud_DrawMarkers)(int32_t player);

/* Moby_MeleeAtJoint: Runs a ball damage query at an attacker's joint
 * (ball_damage_pushed_at_joint), passing flags into the query before collision filtering.
 * Returns the query's result. The PAL catalogue lists this function in levels 1, 2, 4-10, 12,
 * 13, 16, 18-24 and 26-29; it does not list it in 3, 11, 14, 17 or 30-36. */
#define OPENRAC_RAC3_HOOK_Moby_MeleeAtJoint "Moby_MeleeAtJoint"
#define OPENRAC_RAC3_SIG_Moby_MeleeAtJoint "i32(u32,i32,f32,i32,f32,f32,i32,i32,i32)"
typedef int32_t (*openrac_rac3_Moby_MeleeAtJoint)(uint32_t attacker, int32_t joint, float radius, int32_t flags, float distance, float scale, int32_t first, int32_t second, int32_t mode);

/* Moby_UpdateCollisionSphere: Updates an indexed collision sphere using a guest vector and two
 * scale parameters. The moby collision pointer must refer to compatible writable sphere
 * records. PAL catalogue lists a copy in all 49 levels; Veldin 0x00439790, Florana 0x004119C8,
 * Marcadia 0x00448F10. Hooks must preserve this requirement when forwarding. */
#define OPENRAC_RAC3_HOOK_Moby_UpdateCollisionSphere "Moby_UpdateCollisionSphere"
#define OPENRAC_RAC3_SIG_Moby_UpdateCollisionSphere "void(u32,i32,u32,f32,f32)"
typedef void (*openrac_rac3_Moby_UpdateCollisionSphere)(uint32_t moby, int32_t index, uint32_t vector, float fa, float fb);

/* ---- end of the generated hook points ---- */

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAMES_RAC3_H */
