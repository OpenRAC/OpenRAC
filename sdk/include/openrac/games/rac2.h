/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: Going Commando's hook points (docs/port/MODS.md, section 4), as rac1.h gives
 * Ratchet & Clank's: for each, its name, its signature as openrac_api.hook takes it, and the type of
 * a replacement. Each disc's table of them is the `hook` rows of port/game/rac2-<version>/libraries.tsv;
 * every hook point here is on the three discs (NTSC-U v1.01 "ntsc", PAL "pal" and NTSC-U v2.00
 * "ntsc2"). The addresses in the comments are NTSC-U v1.01's (games/rac2's names). Each also has a
 * permanent id (sdk/bindings/rac2.json, rac2.ids.lock), which openrac_api.hook takes as well; an id
 * reuses rac1's <area>.<name> where the job is the same, so a mod for both games reads alike
 * (openrac-mods docs/MULTI_GAME.md).
 *
 * Parameters are as the port translates the game: u32 is a game address or an unsigned word, i32 an
 * int, f32 a float.
 *
 * Each hook point has three names here: OPENRAC_RAC2_HOOK_<name>, the string openrac_api.hook takes;
 * OPENRAC_RAC2_SIG_<name>, its signature string; and openrac_rac2_<name>, the type of a function
 * that can replace it. OPENRAC_RAC2_HOOK takes them together. On a port that lacks one,
 * openrac_api.hook returns OPENRAC_HOOK_UNKNOWN. The first came with SDK 1.15.
 */
#ifndef OPENRAC_GAMES_RAC2_H
#define OPENRAC_GAMES_RAC2_H

#include "openrac/openrac_mod.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hero_TakeDamage (func_L00_002A4EF8, rac2.player.take_damage): Takes a hit's points from the
 * hero's health (the hero at 0x189E20 on NTSC-U, health at +0x24CC), after the armour's scale; the
 * hero's state machine calls it for each hit. */
#define OPENRAC_RAC2_HOOK_Hero_TakeDamage "Hero_TakeDamage"
#define OPENRAC_RAC2_SIG_Hero_TakeDamage "void(i32)"
typedef void (*openrac_rac2_Hero_TakeDamage)(int32_t amount);

/* Moby_GetHitMessage (func_002A9640, rac2.combat.get_hit): The pending hit for a moby from the hit
 * ring (the damage, f32 at +0x2C; its source at +0x38), or 0: what an enemy's damage path reads. */
#define OPENRAC_RAC2_HOOK_Moby_GetHitMessage "Moby_GetHitMessage"
#define OPENRAC_RAC2_SIG_Moby_GetHitMessage "u32(u32,u32,i32)"
typedef uint32_t (*openrac_rac2_Moby_GetHitMessage)(uint32_t moby, uint32_t mask, int32_t keep);

/* Weapon_TakeAmmo (func_L00_002D3F30, rac2.weapon.take_ammo): Takes rounds of a weapon's ammo
 * (item -1: the one in hand); a weapon with no capacity has endless ammo. Rac1's TakeAmmo. */
#define OPENRAC_RAC2_HOOK_Weapon_TakeAmmo "Weapon_TakeAmmo"
#define OPENRAC_RAC2_SIG_Weapon_TakeAmmo "i32(i32,i32)"
typedef int32_t (*openrac_rac2_Weapon_TakeAmmo)(int32_t item, int32_t amount);

/* Moby_RecordDeath (func_002A6C58, rac2.moby.record_death): A moby's death recorded: its
 * experience, the bestiary and its bolt drop (the moby's s16 at +0xB4); crates call it too. */
#define OPENRAC_RAC2_HOOK_Moby_RecordDeath "Moby_RecordDeath"
#define OPENRAC_RAC2_SIG_Moby_RecordDeath "void(u32,i32,i32)"
typedef void (*openrac_rac2_Moby_RecordDeath)(uint32_t moby, int32_t flags, int32_t arg);

/* Level_Load (func_L00_00303990, rac2.level.load): Loads a level (new, or resumed after a death);
 * it calls the gameplay file's loader. */
#define OPENRAC_RAC2_HOOK_Level_Load "Level_Load"
#define OPENRAC_RAC2_SIG_Level_Load "i32(i32,i32)"
typedef int32_t (*openrac_rac2_Level_Load)(int32_t load, int32_t resume);

/* Hero_ApproachSpeed (func_L00_002B41B8, rac2.player.approach_speed): Eases the hero's speed
 * (+0x1B4) toward his target speed (+0x1B0) each tick, at up when speeding up and down when
 * slowing; rac1's approach_speed. */
#define OPENRAC_RAC2_HOOK_Hero_ApproachSpeed "Hero_ApproachSpeed"
#define OPENRAC_RAC2_SIG_Hero_ApproachSpeed "void(f32,f32)"
typedef void (*openrac_rac2_Hero_ApproachSpeed)(float up, float down);

/* Hero_NanotechSteps (func_L00_002D5E30, rac2.player.nanotech_steps): Adds the nanotech levels'
 * steps of health to *count; the recount of the most health each tick adds the boosts after it
 * (the HUD's experience bar calls it too, ignoring count). */
#define OPENRAC_RAC2_HOOK_Hero_NanotechSteps "Hero_NanotechSteps"
#define OPENRAC_RAC2_SIG_Hero_NanotechSteps "i32(u32,u32,u32)"
typedef int32_t (*openrac_rac2_Hero_NanotechSteps)(uint32_t a0, uint32_t a1, uint32_t count);

/* Item_Give (func_00288308, rac2.item.give): Gives the player item id (GiveInventoryItem): nothing
 * when it is owned already; else its owned and new bytes, its starting ammo, the vendor's list and
 * the quick select. NPCs, pickups, arena prizes, purchases and the start's items go through it; a
 * few places set the owned bytes themselves. Rac1's GiveItem. */
#define OPENRAC_RAC2_HOOK_Item_Give "Item_Give"
#define OPENRAC_RAC2_SIG_Item_Give "void(i32)"
typedef void (*openrac_rac2_Item_Give)(int32_t id);

/* Planet_Unlock (func_002AD7B8, rac2.planet.unlock): Puts a level on the ship's map
 * (MarkLevelAvailable); every level load calls it for the level loaded too. Rac1's UnlockPlanet. */
#define OPENRAC_RAC2_HOOK_Planet_Unlock "Planet_Unlock"
#define OPENRAC_RAC2_SIG_Planet_Unlock "void(i32)"
typedef void (*openrac_rac2_Planet_Unlock)(int32_t level);


/* The givers' updates of NTSC-U v1.01 (rac2.moby.<name>.update), each void(u32 moby): their
 * replacements see the moby; the other discs have none of them yet. */
#define OPENRAC_RAC2_HOOK_Moby_DynamoPickup_Update "Moby_DynamoPickup_Update"
#define OPENRAC_RAC2_SIG_Moby_DynamoPickup_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_TractorBeamSeller_Update "Moby_TractorBeamSeller_Update"
#define OPENRAC_RAC2_SIG_Moby_TractorBeamSeller_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_BoxBreakerPickup_Update "Moby_BoxBreakerPickup_Update"
#define OPENRAC_RAC2_SIG_Moby_BoxBreakerPickup_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_ThermanatorSeller_Update "Moby_ThermanatorSeller_Update"
#define OPENRAC_RAC2_SIG_Moby_ThermanatorSeller_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_ThermanatorArea_Update "Moby_ThermanatorArea_Update"
#define OPENRAC_RAC2_SIG_Moby_ThermanatorArea_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_SwingshotArea_Update "Moby_SwingshotArea_Update"
#define OPENRAC_RAC2_SIG_Moby_SwingshotArea_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_GliderPickup_Update "Moby_GliderPickup_Update"
#define OPENRAC_RAC2_SIG_Moby_GliderPickup_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_LevitatorSeller_Update "Moby_LevitatorSeller_Update"
#define OPENRAC_RAC2_SIG_Moby_LevitatorSeller_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_ChargeBootsRider_Update "Moby_ChargeBootsRider_Update"
#define OPENRAC_RAC2_SIG_Moby_ChargeBootsRider_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_RaceController_Update "Moby_RaceController_Update"
#define OPENRAC_RAC2_SIG_Moby_RaceController_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_MapperReturn_Update "Moby_MapperReturn_Update"
#define OPENRAC_RAC2_SIG_Moby_MapperReturn_Update "void(u32)"
#define OPENRAC_RAC2_HOOK_Moby_HypnomaticMaker_Update "Moby_HypnomaticMaker_Update"
#define OPENRAC_RAC2_SIG_Moby_HypnomaticMaker_Update "void(u32)"
typedef void (*openrac_rac2_Moby_Update)(uint32_t moby);

/* Takes a hook point by its name here, its signature and a replacement of its type; the original
 * is written to `original` (OPENRAC_HOOK_OK, or the reason). Only during openrac_mod_load. */
#define OPENRAC_RAC2_HOOK(api, mod, point, replacement, original)                                  \
    (api)->hook(                                                                                   \
        (mod),                                                                                     \
        OPENRAC_RAC2_HOOK_##point,                                                                 \
        OPENRAC_RAC2_SIG_##point,                                                                  \
        (void*)(replacement),                                                                      \
        (void**)(original)                                                                         \
    )

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAMES_RAC2_H */
