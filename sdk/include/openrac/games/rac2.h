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

/* Bolt coins. Seven classes, OPENRAC_RAC2_BOLT_CLASS (0x0D) to + 6, worth 1, 5, 20, 50, 100, 500
 * and 1000 bolts. A burst (Bolt_SpawnPickups) makes its coins one at a time (Bolt_SpawnPickup), and
 * each coin the hero reaches is collected (Bolt_Collect). A mod's own kind of coin (platinum bolts):
 * mark the mobys Bolt_SpawnPickup returns, draw them with openrac_api.moby_material, and add to
 * what Bolt_Collect gives for them with openrac_api.bolts. */
#define OPENRAC_RAC2_BOLT_CLASS 0x0D
#define OPENRAC_RAC2_BOLT_CLASSES 7

/* Bolt_SpawnPickups (func_002ADB78, rac2.bolt.spawn_pickups): Spawns a burst of bolt coins from
 * moby: moby_spawn_bolt_pickups, and its copy in every level's code (part_spawn_bolt_pickups,
 * func_L00_00321878), one hook point. Rolls an amount from low to high (doubled under the bolt
 * multiplier, scaled by how fast the level's bolts are being earned) and splits it into the biggest
 * coins that fit, each made by Bolt_SpawnPickup. flags: 0 spawns nothing, 2 the coins fly to the
 * hero, 8 at most maximum coins (the rest goes to the word at 0x1A7A14), 32 not in the level's bolt
 * score; surface is a path the coins land on (-1 none), height a lift of the spawn point. Enemies'
 * and crates' drops (Moby_RecordDeath) and the levels' scripted bursts (part_spawn_scaled_bolts)
 * all come here. A replacement can change low and high, or make the coins itself. */
#define OPENRAC_RAC2_HOOK_Bolt_SpawnPickups "Bolt_SpawnPickups"
#define OPENRAC_RAC2_SIG_Bolt_SpawnPickups "void(u32,i32,i32,i32,i32,i32,f32)"
typedef void (*openrac_rac2_Bolt_SpawnPickups)(
    uint32_t moby,
    int32_t low,
    int32_t high,
    int32_t flags,
    int32_t surface,
    int32_t maximum,
    float height
);

/* Bolt_SpawnPickup (func_00302470, rac2.bolt.spawn_pickup): Makes one coin worth value (1, 5, 20,
 * 50, 100, 500 or 1000; another value makes a 1) at position with velocity (game addresses of four
 * floats each), from source; counted (the byte at +0x40 of its vars) says whether the level's bolt
 * score counts it, flags 2 flies it to the hero. Returns the coin's moby, or 0: none when the moby
 * pool is full, or when position is 64 units or more from the game's reference point (the value
 * goes to the word at 0x1A7A14 instead). Bolt_SpawnPickups calls it for each coin, and two level
 * mobys directly (func_L03_00383FE8, func_L02_003E7370). */
#define OPENRAC_RAC2_HOOK_Bolt_SpawnPickup "Bolt_SpawnPickup"
#define OPENRAC_RAC2_SIG_Bolt_SpawnPickup "u32(u32,u32,u32,u32,i32,i32)"
typedef uint32_t (*openrac_rac2_Bolt_SpawnPickup)(
    uint32_t source,
    uint32_t position,
    uint32_t velocity,
    uint32_t flags,
    int32_t value,
    int32_t counted
);

/* Bolt_Collect (func_00301D10, rac2.bolt.collect): A coin collected (CollectBolts): its class's value
 * added to the player's bolts (0x1A79F8) and, for a counted coin, to the level's bolt score, then its
 * sparks. The coin's update calls it as the coin reaches the hero, the moby still alive; it returns
 * nothing (SDK 1.28.0 and older said u32(u32), which the port refused). A replacement can add more
 * with openrac_api.bolts: for a coin worth five times, four times what the original added. */
#define OPENRAC_RAC2_HOOK_Bolt_Collect "Bolt_Collect"
#define OPENRAC_RAC2_SIG_Bolt_Collect "void(u32)"
typedef void (*openrac_rac2_Bolt_Collect)(uint32_t moby);

/* Arena_Prize (func_L02_003FCDF8, rac2.arena.prize): The bolts arena challenge `challenge` pays
 * (moby_compute_scaled_entry_value; one function in Maktar's and Joba's code): its record's prize
 * (+0xC), times 0.3 for each earlier win, kept between 500 and the prize, ten times that in
 * challenge mode. The arena's menu prints it and a win adds it to the bolts, so a replacement that
 * returns more pays and shows more. */
#define OPENRAC_RAC2_HOOK_Arena_Prize "Arena_Prize"
#define OPENRAC_RAC2_SIG_Arena_Prize "i32(i32)"
typedef int32_t (*openrac_rac2_Arena_Prize)(int32_t challenge);


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

/* ---- generated by sdk/tools/gen_binding.py from sdk/bindings/rac2.json: do not edit ---- */
/* Hook points added since this header was last written by hand, from the binding table.
 * As the others: on a port that lacks one, openrac_api.hook returns OPENRAC_HOOK_UNKNOWN. */

/* Moby_NanotechBall_Update: The nanotech orb's update (class 0x326, the orb in a nanotech
 * crate): flies its pieces to the hero, who gains health as each lands (+0x24CC of the hero, at
 * most his max health), 1 a piece or 4 for the special orb. Rac1's nanotech_ball update. */
#define OPENRAC_RAC2_HOOK_Moby_NanotechBall_Update "Moby_NanotechBall_Update"
#define OPENRAC_RAC2_SIG_Moby_NanotechBall_Update "void(u32)"
typedef void (*openrac_rac2_Moby_NanotechBall_Update)(uint32_t moby);

/* ArmorVendor_BuildList: Fills the armour vendor's list (the vendor in mode 3: a Megacorp
 * Vendor whose pvars +0x94 is 2): g_vendorUi +0x140, 12-byte rows {i32 icon (0xEA92 + armour, a
 * HUD icon id), i32 price (bolts), i32 armour (0 to 4: the armour the row sells, whose item
 * record names it and whose pack the preview loads)}, the count at +0x740 (up to 128): each
 * armour above the one worn (s16 at the save block +0x20) that its planet unlocked
 * (g_game_flag_table[0x8D] bit, or the extras of a finished game), icon 0xEA92 + armour, price
 * 0, 25000, 100000, 250000 or 1000000. With Carbonox (4) worn it lists nothing. The vendor
 * calls it as it opens. A replacement can add rows after the original's; a row's armour must
 * stay 0 to 4 (the preview, the name, the description table and the close index by it). The
 * armour vendors are on Dobbo, Boldan, Snivelak and Grelbin. */
#define OPENRAC_RAC2_HOOK_ArmorVendor_BuildList "ArmorVendor_BuildList"
#define OPENRAC_RAC2_SIG_ArmorVendor_BuildList "void()"
typedef void (*openrac_rac2_ArmorVendor_BuildList)(void);

/* ArmorVendor_Buy: The armour vendor's purchase dialog, once a frame while it is up
 * (shop_dispatch_purchase_confirm, mode 3; every level's copy). answer: the address of its i32
 * answer (1 yes); the second word the dispatcher passes (the answer's neighbour) is not read.
 * Its state is g_vendorUi +0x84: 1 opens it (too few bolts for the row's price: refused, state
 * 2), 2 waits for Cross (answer 1) or Triangle (answer 0), then 3 decides: answer 1, the bolts
 * at least the price and the armour worn not 4: pays the row's price, plays the purchase sound
 * and message and closes the menu, which then wears the row's armour (shop_close_sub_menu);
 * otherwise the refusal or nothing, state 0. The row is the one under the cursor (+0x80). A
 * replacement can buy a row of its own at state 3 (pay, then set the answer to 0 so that the
 * original closes the dialog as a refusal does) and call the original for the game's rows. */
#define OPENRAC_RAC2_HOOK_ArmorVendor_Buy "ArmorVendor_Buy"
#define OPENRAC_RAC2_SIG_ArmorVendor_Buy "void(u32,u32)"
typedef void (*openrac_rac2_ArmorVendor_Buy)(uint32_t answer, uint32_t unused);

/* Vendor_Preview: Sets up the vendor's preview of the row under the cursor
 * (PrepareVendorSelectionPreview), in every mode, as the cursor moves and the menu opens: the
 * armour vendor's (mode 3) loads the row's armour over Ratchet's class 0 when it is not the one
 * loaded and makes the preview objects (g_vendorUi +0x2C, the count at +0x34, 0x100-byte mobys
 * outside the level's list: class 0, Ratchet in that armour, then 1290 and 2675 for its extra
 * pieces), turned to the camera, and shows the armour's description; the weapon vendor's, the
 * weapon's model or picture. A replacement can call the original and then change the message or
 * the objects' look (moby_material_class with OPENRAC_MODEL_MENU). */
#define OPENRAC_RAC2_HOOK_Vendor_Preview "Vendor_Preview"
#define OPENRAC_RAC2_SIG_Vendor_Preview "void()"
typedef void (*openrac_rac2_Vendor_Preview)(void);

/* Hero_Update: The hero's whole update each tick: his animation, the stick, ledges, grind
 * rails, his state machine, his moby and what he holds. Each group of levels has its own copy;
 * together they are one hook point. A ship or vehicle update runs in its place while one drives
 * him (hero +0x22B4). A replacement that skips the original moves him itself and shows him with
 * openrac_api.hero_set_pose. */
#define OPENRAC_RAC2_HOOK_Hero_Update "Hero_Update"
#define OPENRAC_RAC2_SIG_Hero_Update "void()"
typedef void (*openrac_rac2_Hero_Update)(void);

/* PlatinumBolts_CountTotal: Counts the platinum bolts recorded in the save. A replacement can
 * return received currency independently of physical pickups. */
#define OPENRAC_RAC2_HOOK_PlatinumBolts_CountTotal "PlatinumBolts_CountTotal"
#define OPENRAC_RAC2_SIG_PlatinumBolts_CountTotal "i32()"
typedef int32_t (*openrac_rac2_PlatinumBolts_CountTotal)(void);

/* Hero_UpdateCollectionProgress: Recounts nanotech and weapon collection progress from
 * inventory; mods can separate pickup flags from received ownership. */
#define OPENRAC_RAC2_HOOK_Hero_UpdateCollectionProgress "Hero_UpdateCollectionProgress"
#define OPENRAC_RAC2_SIG_Hero_UpdateCollectionProgress "void()"
typedef void (*openrac_rac2_Hero_UpdateCollectionProgress)(void);

/* Save_Serialize: Serializes live game state to the save packet at output. Inventory
 * substitutions made by a mod must be restored before calling the original. */
#define OPENRAC_RAC2_HOOK_Save_Serialize "Save_Serialize"
#define OPENRAC_RAC2_SIG_Save_Serialize "void(u32)"
typedef void (*openrac_rac2_Save_Serialize)(uint32_t output);

/* Save_NewGame: Resets game data from disc defaults for a new campaign, before its starting
 * level is entered. */
#define OPENRAC_RAC2_HOOK_Save_NewGame "Save_NewGame"
#define OPENRAC_RAC2_SIG_Save_NewGame "void()"
typedef void (*openrac_rac2_Save_NewGame)(void);

/* Moby_EndakoRescue_Update: Updates the Endako rescue sequence that returns Clank and the
 * Swingshot. */
#define OPENRAC_RAC2_HOOK_Moby_EndakoRescue_Update "Moby_EndakoRescue_Update"
#define OPENRAC_RAC2_SIG_Moby_EndakoRescue_Update "void(u32)"
typedef void (*openrac_rac2_Moby_EndakoRescue_Update)(uint32_t moby);

/* Moby_TaboraWrench_Update: Updates the Tabora controller, including its wrench reward. */
#define OPENRAC_RAC2_HOOK_Moby_TaboraWrench_Update "Moby_TaboraWrench_Update"
#define OPENRAC_RAC2_SIG_Moby_TaboraWrench_Update "void(u32)"
typedef void (*openrac_rac2_Moby_TaboraWrench_Update)(uint32_t moby);

/* Moby_PrisonWrench_Init: Initializes the Aranos prison wrench pickup. */
#define OPENRAC_RAC2_HOOK_Moby_PrisonWrench_Init "Moby_PrisonWrench_Init"
#define OPENRAC_RAC2_SIG_Moby_PrisonWrench_Init "void(u32)"
typedef void (*openrac_rac2_Moby_PrisonWrench_Init)(uint32_t moby);

/* Moby_TodanoTrader_Update: Updates the Todano trader and his reward exchange. */
#define OPENRAC_RAC2_HOOK_Moby_TodanoTrader_Update "Moby_TodanoTrader_Update"
#define OPENRAC_RAC2_SIG_Moby_TodanoTrader_Update "void(u32)"
typedef void (*openrac_rac2_Moby_TodanoTrader_Update)(uint32_t moby);

/* Moby_PrisonPlumber_Update: Updates the Aranos plumber and his reward. The game declaration
 * takes a signed 32-bit moby address. */
#define OPENRAC_RAC2_HOOK_Moby_PrisonPlumber_Update "Moby_PrisonPlumber_Update"
#define OPENRAC_RAC2_SIG_Moby_PrisonPlumber_Update "void(i32)"
typedef void (*openrac_rac2_Moby_PrisonPlumber_Update)(int32_t moby);

/* Moby_PrisonController_Update: Updates the Aranos prison controller, including inventory
 * restoration. */
#define OPENRAC_RAC2_HOOK_Moby_PrisonController_Update "Moby_PrisonController_Update"
#define OPENRAC_RAC2_SIG_Moby_PrisonController_Update "void(u32)"
typedef void (*openrac_rac2_Moby_PrisonController_Update)(uint32_t moby);

/* Moby_SiberiusThief_Update: Updates the Siberius thief sequence and its progression state. */
#define OPENRAC_RAC2_HOOK_Moby_SiberiusThief_Update "Moby_SiberiusThief_Update"
#define OPENRAC_RAC2_SIG_Moby_SiberiusThief_Update "void(u32)"
typedef void (*openrac_rac2_Moby_SiberiusThief_Update)(uint32_t moby);

/* Moby_BoldanCutscene_Update: Updates the Boldan cutscene trigger and its temporary inventory
 * state. */
#define OPENRAC_RAC2_HOOK_Moby_BoldanCutscene_Update "Moby_BoldanCutscene_Update"
#define OPENRAC_RAC2_SIG_Moby_BoldanCutscene_Update "void(u32)"
typedef void (*openrac_rac2_Moby_BoldanCutscene_Update)(uint32_t moby);

/* Moby_HypnomaticPart_Update: Updates the Hypnomatic part pickup on Damosel. */
#define OPENRAC_RAC2_HOOK_Moby_HypnomaticPart_Update "Moby_HypnomaticPart_Update"
#define OPENRAC_RAC2_SIG_Moby_HypnomaticPart_Update "void(u32)"
typedef void (*openrac_rac2_Moby_HypnomaticPart_Update)(uint32_t moby);

/* Moby_CountKill: Records a kill or event and grants its rewards. A replacement can adjust
 * reward state around the original call. */
#define OPENRAC_RAC2_HOOK_Moby_CountKill "Moby_CountKill"
#define OPENRAC_RAC2_SIG_Moby_CountKill "void(u32,i32)"
typedef void (*openrac_rac2_Moby_CountKill)(uint32_t moby, int32_t type);

/* Menu_Extras: Processes the extras menu and returns its menu result. */
#define OPENRAC_RAC2_HOOK_Menu_Extras "Menu_Extras"
#define OPENRAC_RAC2_SIG_Menu_Extras "i32()"
typedef int32_t (*openrac_rac2_Menu_Extras)(void);

/* FollowCamera_ReadStick: The follow camera's read of the right stick, each tick: camera (its
 * data at +0x70), and the guest addresses of the floats it then turns and tilts by. By then the
 * tick's goals are set: the eye's distance and height goals (data +0x1A0 and +0x1AC), which the
 * camera eases its distance and height (+0x18C, +0x190) towards, and the eye it wants from
 * Ratchet (+0x160); after it the camera turns, eases and builds its view, so a mod that calls
 * the original and then changes those moves the camera this very tick. Rac1's
 * FollowCamera_ReadStickInput. */
#define OPENRAC_RAC2_HOOK_FollowCamera_ReadStick "FollowCamera_ReadStick"
#define OPENRAC_RAC2_SIG_FollowCamera_ReadStick "void(u32,u32,u32)"
typedef void (*openrac_rac2_FollowCamera_ReadStick)(uint32_t camera, uint32_t turn, uint32_t tilt);

/* Moby_BoldanClock_Update: Updates Boldan's clock tower: sets its hands from the console's
 * clock and, once a second of its time, shows the Nanotech Upgrade (the moby at its pvars
 * +0x28, class 0x11C4) on an even hour of the twelve-hour dial and hides it on an odd one. */
#define OPENRAC_RAC2_HOOK_Moby_BoldanClock_Update "Moby_BoldanClock_Update"
#define OPENRAC_RAC2_SIG_Moby_BoldanClock_Update "void(u32)"
typedef void (*openrac_rac2_Moby_BoldanClock_Update)(uint32_t moby);

/* Moby_ItemRewardTrigger_Update: Updates a reward trigger (item_reward_trigger_update): the
 * scene of a story or reward spot on any level. It waits for the hero in its region, plays its
 * scenes, then puts its planet on the ship's map (Planet_Unlock) and marks its moby group done.
 * At its first update it deletes itself, unplayed, when its planet is on the map already (the
 * byte at 0x1A7BC8 + planet, NTSC-U v1.01) or its group is done, so a replacement can show it
 * another state of the map around the original. */
#define OPENRAC_RAC2_HOOK_Moby_ItemRewardTrigger_Update "Moby_ItemRewardTrigger_Update"
#define OPENRAC_RAC2_SIG_Moby_ItemRewardTrigger_Update "void(u32)"
typedef void (*openrac_rac2_Moby_ItemRewardTrigger_Update)(uint32_t moby);

/* ---- end of the generated hook points ---- */

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAMES_RAC2_H */
