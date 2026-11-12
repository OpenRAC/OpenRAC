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

/* Moby_PostHit (func_002A8E20, rac2.combat.post_hit): Posts a hit to moby victim from request, a
 * game address (+0x10 the attacker, +0x14 the weapon or attack mask, +0x1C the damage, f32), into
 * the hit ring Moby_GetHitMessage reads. The weapons' shots, explosions and tethers call it with
 * their version's damage. A replacement can change the request's damage for the call; a caller may
 * post the same request to several victims, so put it back after. */
#define OPENRAC_RAC2_HOOK_Moby_PostHit "Moby_PostHit"
#define OPENRAC_RAC2_SIG_Moby_PostHit "void(u32,u32)"
typedef void (*openrac_rac2_Moby_PostHit)(uint32_t victim, uint32_t request);

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

/* Item_Equip (func_002AD860, rac2.item.equip): Puts item id in the hero's hand, or on him for the
 * boots, helmets and packs (by the state of its item record): a request his update carries out; 1
 * when one was made. Most givers call it right after Item_Give with the same id, the vendor after a
 * purchase; a mod that gives another item in a giver's place hands it this one's too. */
#define OPENRAC_RAC2_HOOK_Item_Equip "Item_Equip"
#define OPENRAC_RAC2_SIG_Item_Equip "i32(i32)"
typedef int32_t (*openrac_rac2_Item_Equip)(int32_t id);

/* Weapon_LevelUp (func_00287EB0, rac2.weapon.level_up): Steps weapon id to its next version by
 * experience: its record (the item's byte at 0x139568) moves to the next row of the item table
 * (+0x4A), its experience is kept at least the step's, its ammo filled to the new row's (+0x8E);
 * nothing at its last row. The hero's experience check calls it when a weapon's experience (the
 * item's s32 at 0x139868, >> 5) reaches its row's step (+0x6C). A replacement that does not call
 * the original blocks the step. */
#define OPENRAC_RAC2_HOOK_Weapon_LevelUp "Weapon_LevelUp"
#define OPENRAC_RAC2_SIG_Weapon_LevelUp "void(i32)"
typedef void (*openrac_rac2_Weapon_LevelUp)(int32_t id);

/* Item_Upgrade (func_002887B0, rac2.item.upgrade): The vendor's Mega purchase of item id: its
 * record moves two rows on (one for the two-version weapons 12, 14, 17, 18 and 53) and its
 * experience is cleared; 1 when it moved, 0 at its last row. The vendor calls it after a purchase
 * of an upgrade entry, once the price is paid; a replacement can carry out its own upgrade. */
#define OPENRAC_RAC2_HOOK_Item_Upgrade "Item_Upgrade"
#define OPENRAC_RAC2_SIG_Item_Upgrade "i32(i32)"
typedef int32_t (*openrac_rac2_Item_Upgrade)(int32_t id);

/* Vendor_BuildList (func_002F6608, rac2.vendor.build_list): Fills the vendor's list (the vendor's
 * state, 0x21F240 on NTSC-U v1.01: +0x140 64 entries of 0x18 bytes, each i32 item, kind (1 its
 * ammo), class (the preview model), price text, marked and upgrade (1 a Mega upgrade); the count at +0x740; the entry
 * under the cursor at +0x80). The vendor calls it as it opens and after each purchase; a
 * replacement can add entries after the original's. */
#define OPENRAC_RAC2_HOOK_Vendor_BuildList "Vendor_BuildList"
#define OPENRAC_RAC2_SIG_Vendor_BuildList "void(i32)"
typedef void (*openrac_rac2_Vendor_BuildList)(int32_t hide_unowned);

/* Vendor_Price (func_002F6510, rac2.vendor.price): The price in bolts of the vendor entry under the
 * cursor: an upgrade's from the row it moves to, a weapon's from its record, 0 for ammo. The dialog
 * and the purchase both ask it. */
#define OPENRAC_RAC2_HOOK_Vendor_Price "Vendor_Price"
#define OPENRAC_RAC2_SIG_Vendor_Price "i32()"
typedef int32_t (*openrac_rac2_Vendor_Price)(void);

/* Item_Colors (func_00336F20, rac2.item.colors): Writes item id's colours to three u32 at game
 * addresses: its experience bar's two and its tint, its version's colour (0x00BBGGRR in the low
 * bytes) that its icon and name are drawn in. The quick select and the weapons screen's grid call
 * it for each item they show; a replacement can call the original, then write its own. */
#define OPENRAC_RAC2_HOOK_Item_Colors "Item_Colors"
#define OPENRAC_RAC2_SIG_Item_Colors "void(i32,u32,u32,u32)"
typedef void (*openrac_rac2_Item_Colors)(int32_t id, uint32_t first, uint32_t second, uint32_t tint);

/* Vendor_DrawList (func_002F9C80, rac2.vendor.draw_list): Draws the vendor's list: for each row -5
 * to +6 around the cursor that is in the list, in that order, its item's icon in
 * Item_VersionColor's colour (an upgrade entry in kind 2, the Mega gold), then its label. A
 * replacement can call the original and know which entry each Item_VersionColor call is for. */
#define OPENRAC_RAC2_HOOK_Vendor_DrawList "Vendor_DrawList"
#define OPENRAC_RAC2_SIG_Vendor_DrawList "void()"
typedef void (*openrac_rac2_Vendor_DrawList)(void);

/* Item_VersionColor (func_00336EF8, rac2.item.version_color): The colour of an item version kind
 * (0x00BBGGRR in the low bytes; 2 the Mega gold): the vendor's list asks it for each icon it
 * draws. A replacement can return its own. */
#define OPENRAC_RAC2_HOOK_Item_VersionColor "Item_VersionColor"
#define OPENRAC_RAC2_SIG_Item_VersionColor "u32(i32)"
typedef uint32_t (*openrac_rac2_Item_VersionColor)(int32_t kind);

/* Text_ById (func_00288FB8, rac2.text.by_id): The game's text of an id, the address of its string
 * in game memory, under the port's own texts; the vendor's list asks it for its upgrade entries'
 * label (0x325F, "Mega") and its dialog for the item's name. A replacement returns a game address:
 * for a text of its own it writes into one of the port's slots, which this lookup (the game function
 * by name, not the original) gives for ids OPENRAC_RAC2_TEXT_SLOT to + 7, 96 bytes each with the
 * ending 0. */
#define OPENRAC_RAC2_HOOK_Text_ById "Text_ById"
#define OPENRAC_RAC2_SIG_Text_ById "i32(i32)"
#define OPENRAC_RAC2_TEXT_SLOT 0x7F10
#define OPENRAC_RAC2_TEXT_SLOTS 8
#define OPENRAC_RAC2_TEXT_SLOT_BYTES 96
typedef int32_t (*openrac_rac2_Text_ById)(int32_t id);

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
