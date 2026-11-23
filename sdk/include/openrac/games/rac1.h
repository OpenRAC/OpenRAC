/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank's hook points (docs/port/MODS.md, section 4): for each, its name, its signature
 * as openrac_api.hook takes it, and the type of a replacement. Each disc's table of them is the
 * `hook` rows of port/game/rac1-<version>/libraries.tsv; every hook point here is on both discs
 * (PAL and NTSC-U) unless it says otherwise. The comments say what the game's function does; the
 * addresses are the PAL disc's (games/rac1's names).
 *
 * Parameters are as the port translates the game: u32 is a game address or an unsigned word, i32 an
 * int, f32 a float.
 *
 * Each hook point has three names here: OPENRAC_RAC1_HOOK_<name>, the string openrac_api.hook takes;
 * OPENRAC_RAC1_SIG_<name>, its signature string; and openrac_rac1_<name>, the type of a function
 * that can replace it. OPENRAC_RAC1_HOOK takes them together. The points are in the order they
 * were added. The first six came with interface 1.0; the rest were added to the 1.x line without a
 * minor of their own, because the table does not change: on a port that lacks one, openrac_api.hook
 * returns OPENRAC_HOOK_UNKNOWN, and the mod decides whether to go on without it.
 */
#ifndef OPENRAC_GAMES_RAC1_H
#define OPENRAC_GAMES_RAC1_H

#include "openrac/openrac_mod.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------ *
 * The first hook points (interface 1.0)
 * ------------------------------------------------------------------------------------------ */

/* Pad_ProcessReport (func_002181F0): one pad report into the pad's state (PadState, at pad), each
 * poll, before the tick reads it. */
#define OPENRAC_RAC1_HOOK_Pad_ProcessReport "Pad_ProcessReport"
#define OPENRAC_RAC1_SIG_Pad_ProcessReport "void(u32,u32,i32)"
typedef void (*openrac_rac1_Pad_ProcessReport)(
    openrac_gaddr pad, openrac_gaddr report, int32_t size
);

/* Hero_UpdateMotionInput (func_L00_00211D28): the stick read into the hero, early in his update
 * each tick. */
#define OPENRAC_RAC1_HOOK_Hero_UpdateMotionInput "Hero_UpdateMotionInput"
#define OPENRAC_RAC1_SIG_Hero_UpdateMotionInput "void()"
typedef void (*openrac_rac1_Hero_UpdateMotionInput)(void);

/* Hero_TurnToHeading (func_L00_002124E8): eases the hero's facing (+0x98) toward his move yaw
 * (+0x180). */
#define OPENRAC_RAC1_HOOK_Hero_TurnToHeading "Hero_TurnToHeading"
#define OPENRAC_RAC1_SIG_Hero_TurnToHeading "void(i32,f32,f32,f32)"
typedef void (*openrac_rac1_Hero_TurnToHeading)(
    int32_t mode, float accel, float damping, float limit
);

/* Hero_SetPlanarVelocity (func_L00_00212790): the hero's velocity at his ground speed along an
 * angle; an angle above pi means his facing. */
#define OPENRAC_RAC1_HOOK_Hero_SetPlanarVelocity "Hero_SetPlanarVelocity"
#define OPENRAC_RAC1_SIG_Hero_SetPlanarVelocity "void(f32)"
typedef void (*openrac_rac1_Hero_SetPlanarVelocity)(float angle);

/* FollowCamera_ApplyOverrides (func_L00_002E74B0): the follow camera's per-tick requests from the
 * hero's state; the camera moby's update calls it before it places the eye. */
#define OPENRAC_RAC1_HOOK_FollowCamera_ApplyOverrides "FollowCamera_ApplyOverrides"
#define OPENRAC_RAC1_SIG_FollowCamera_ApplyOverrides "void(u32)"
typedef void (*openrac_rac1_FollowCamera_ApplyOverrides)(openrac_gaddr camera);

/* Hero_AdvanceAnimation (func_L00_00232EF0): Ratchet's animation moved on by his animation speed,
 * once a tick after his state's update. */
#define OPENRAC_RAC1_HOOK_Hero_AdvanceAnimation "Hero_AdvanceAnimation"
#define OPENRAC_RAC1_SIG_Hero_AdvanceAnimation "void()"
typedef void (*openrac_rac1_Hero_AdvanceAnimation)(void);

/* ------------------------------------------------------------------------------------------ *
 * Hook points added to the 1.x line (the table is unchanged: check openrac_api.hook's result)
 * ------------------------------------------------------------------------------------------ */

/* Item_Give (func_L00_002618D8, rac1.item.give): gives the player item id (GiveItem): its owned
 * and obtained bytes, its banner, its starting ammo, its vendor stock mark and the quick select;
 * equip non-zero also puts it in hand. Every item the game gives goes through it but the
 * Persuader's. */
#define OPENRAC_RAC1_HOOK_Item_Give "Item_Give"
#define OPENRAC_RAC1_SIG_Item_Give "void(i32,i32)"
typedef void (*openrac_rac1_Item_Give)(int32_t id, int32_t equip);

/* Planet_Unlock (func_L00_00261848, rac1.planet.unlock): unlocks a planet's coordinates
 * (UnlockPlanet): its flag, the galaxy list, and its banner unless it is the planet the player
 * is on. */
#define OPENRAC_RAC1_HOOK_Planet_Unlock "Planet_Unlock"
#define OPENRAC_RAC1_SIG_Planet_Unlock "void(i32)"
typedef void (*openrac_rac1_Planet_Unlock)(int32_t planet);

/* Vendor_InitLevelStock (func_L00_0023DB30, rac1.vendor.init_level_stock): the vendor's stock at
 * each level start (VendorStockLevelInit): the Bomb Glove while the player has none, the level's
 * new weapon. */
#define OPENRAC_RAC1_HOOK_Vendor_InitLevelStock "Vendor_InitLevelStock"
#define OPENRAC_RAC1_SIG_Vendor_InitLevelStock "void()"
typedef void (*openrac_rac1_Vendor_InitLevelStock)(void);

/* Vendor_BuildItemList (func_L00_0029B770, rac1.vendor.build_item_list): the vendor menu's list
 * from the stock and the items owned (VendorBuildItemList), each time it opens and after each
 * purchase; skip_unflagged leaves out what is for sale. */
#define OPENRAC_RAC1_HOOK_Vendor_BuildItemList "Vendor_BuildItemList"
#define OPENRAC_RAC1_SIG_Vendor_BuildItemList "void(i32)"
typedef void (*openrac_rac1_Vendor_BuildItemList)(int32_t skip_unflagged);

/* GoldBolts_CountCollected (func_002160E0, rac1.gold_bolt.count_collected): the gold bolts
 * collected, at most 40 (count_nonzero_entries_up_to_40): the pause screen's count; the unspent
 * ones are this less 4 per gold weapon. */
#define OPENRAC_RAC1_HOOK_GoldBolts_CountCollected "GoldBolts_CountCollected"
#define OPENRAC_RAC1_SIG_GoldBolts_CountCollected "i32()"
typedef int32_t (*openrac_rac1_GoldBolts_CountCollected)(void);

/* GoldBolts_CountUnspent (rac1.gold_bolt.count_unspent): gold bolts available to spend. */
#define OPENRAC_RAC1_HOOK_GoldBolts_CountUnspent "GoldBolts_CountUnspent"
#define OPENRAC_RAC1_SIG_GoldBolts_CountUnspent "i32()"
typedef int32_t (*openrac_rac1_GoldBolts_CountUnspent)(void);

/* Hero_TakeDamage (func_L00_002056D8, rac1.player.take_damage): takes hit points off the hero's
 * health, at most one a call (none for 0), then the HUD's health bank. Every hit but the instant
 * kills. */
#define OPENRAC_RAC1_HOOK_Hero_TakeDamage "Hero_TakeDamage"
#define OPENRAC_RAC1_SIG_Hero_TakeDamage "void(i32)"
typedef void (*openrac_rac1_Hero_TakeDamage)(int32_t amount);

/* Moby_GetHitMessage (func_L00_0025B478, rac1.combat.get_hit): the hit pending on a moby
 * (MobyGetHitMessage): the record of the last hit posted to it (+0x20 the attacker, +0x24 the
 * flags, +0x2C the damage, an f32) when its flags match mask, else 0; the hit is dropped unless
 * keep. Read each tick, so a hit may be read more than once. */
#define OPENRAC_RAC1_HOOK_Moby_GetHitMessage "Moby_GetHitMessage"
#define OPENRAC_RAC1_SIG_Moby_GetHitMessage "u32(u32,i32,i32)"
typedef openrac_gaddr (*openrac_rac1_Moby_GetHitMessage)(
    openrac_gaddr moby, int32_t mask, int32_t keep
);

/* Weapon_TakeAmmo (func_L00_00234638, rac1.weapon.take_ammo): takes ammo from an item for a shot
 * (TakeAmmo; -1 the item in hand): 1 when taken, 0 when there is too little. */
#define OPENRAC_RAC1_HOOK_Weapon_TakeAmmo "Weapon_TakeAmmo"
#define OPENRAC_RAC1_SIG_Weapon_TakeAmmo "i32(i32,i32)"
typedef int32_t (*openrac_rac1_Weapon_TakeAmmo)(int32_t item, int32_t amount);

/* Weapon_Blaster_PickTarget (func_L00_002B5998, rac1.weapon.blaster.pick_target): The Blaster's auto-aim target (select_blaster_auto_aim_target: the nearest creature within 20 units and about 9 degrees of the heading, in sight; writes the yaw and pitch toward it). */
#define OPENRAC_RAC1_HOOK_Weapon_Blaster_PickTarget "Weapon_Blaster_PickTarget"
#define OPENRAC_RAC1_SIG_Weapon_Blaster_PickTarget "u32(i32,u32,u32,u32)"
typedef openrac_gaddr (*openrac_rac1_Weapon_Blaster_PickTarget)(int32_t ignore, openrac_gaddr from, openrac_gaddr yaw, openrac_gaddr pitch);

/* Weapon_Ryno_PickTarget (func_L00_002CD7B0, rac1.weapon.ryno.pick_target): A R.Y.N.O. missile's target (select_ryno_target: within the range and angles its caller gives, in sight, not one of the salvo's seven). */
#define OPENRAC_RAC1_HOOK_Weapon_Ryno_PickTarget "Weapon_Ryno_PickTarget"
#define OPENRAC_RAC1_SIG_Weapon_Ryno_PickTarget "u32(u32,u32,u32,f32,f32,f32)"
typedef openrac_gaddr (*openrac_rac1_Weapon_Ryno_PickTarget)(openrac_gaddr pos, openrac_gaddr aim, openrac_gaddr skip, float max_yaw, float max_pitch, float max_distance);

/* Weapon_GoldDevastator_PickTarget (func_L00_002B1290, rac1.weapon.gold_devastator.pick_target): The gold Devastator missile's target (select_gold_devastator_missile_target: within 40 units, in a cone that widens with the weapon's level, in sight; written into its data). */
#define OPENRAC_RAC1_HOOK_Weapon_GoldDevastator_PickTarget "Weapon_GoldDevastator_PickTarget"
#define OPENRAC_RAC1_SIG_Weapon_GoldDevastator_PickTarget "void(u32,u32,u32)"
typedef void (*openrac_rac1_Weapon_GoldDevastator_PickTarget)(openrac_gaddr missile, openrac_gaddr data, openrac_gaddr skip);

/* Weapon_Drone_PickTarget (func_L00_002CF3D8, rac1.weapon.drone.pick_target): A drone's target (select_nearest_drone_target: the nearest enemy within 4 to 6 units, not one of the nine others' targets). */
#define OPENRAC_RAC1_HOOK_Weapon_Drone_PickTarget "Weapon_Drone_PickTarget"
#define OPENRAC_RAC1_SIG_Weapon_Drone_PickTarget "u32(u32,u32)"
typedef openrac_gaddr (*openrac_rac1_Weapon_Drone_PickTarget)(openrac_gaddr from, openrac_gaddr skip);

/* Moby_RecordDeath (func_L00_002584A8, rac1.moby.record_death): records a placed moby's death
 * or a crate's break (SetDeathBits: its killed bits, none for a save id below 0) and drops its
 * bolts, about the s16 at +0xB4, which it then clears. */
#define OPENRAC_RAC1_HOOK_Moby_RecordDeath "Moby_RecordDeath"
#define OPENRAC_RAC1_SIG_Moby_RecordDeath "void(u32,i32,i32)"
typedef void (*openrac_rac1_Moby_RecordDeath)(openrac_gaddr moby, int32_t flags, int32_t index);

/* Level_LoadGameplay (func_L00_002422D8, rac1.level.load_gameplay): loads a level's gameplay
 * file (LoadLevelGameplay), once per level start: after it every placed moby exists and none has
 * run. Returns the heap cursor. */
#define OPENRAC_RAC1_HOOK_Level_LoadGameplay "Level_LoadGameplay"
#define OPENRAC_RAC1_SIG_Level_LoadGameplay "u32(i32)"
typedef openrac_gaddr (*openrac_rac1_Level_LoadGameplay)(int32_t full);

/* Hero_ApproachSpeed (func_L00_00212740, rac1.player.approach_speed): eases the hero's ground
 * speed (+0x194) toward his target speed (+0x190) by up below it and down above it, each tick he
 * moves on foot. */
#define OPENRAC_RAC1_HOOK_Hero_ApproachSpeed "Hero_ApproachSpeed"
#define OPENRAC_RAC1_SIG_Hero_ApproachSpeed "void(f32,f32)"
typedef void (*openrac_rac1_Hero_ApproachSpeed)(float up, float down);

/* Ship_TravelTo (func_L00_00290030, rac1.ship.travel_to): the ship's travel from the planet
 * select (ShipTravelTo), once its menu closes: to the planet it is on, the landing again (as
 * backing out of the select does); to another, the save to the card and the ship's flight out of
 * the level, which then ends and flies to dest. */
#define OPENRAC_RAC1_HOOK_Ship_TravelTo "Ship_TravelTo"
#define OPENRAC_RAC1_SIG_Ship_TravelTo "void(i32)"
typedef void (*openrac_rac1_Ship_TravelTo)(int32_t dest);

/* Moby_CollectBolt (func_L00_002A7E10, rac1.moby.collect_bolt): collects a bolt (collect_bolt),
 * from the bolt update once the bolt reaches the hero: its value (1, or 5 / 20 / 50 for classes
 * 14 / 15 / 16) added to the bolt count and the level's totals, the bolt counter shown, two
 * sparkles. The bolt update deletes the bolt after it, whether or not this ran. */
#define OPENRAC_RAC1_HOOK_Moby_CollectBolt "Moby_CollectBolt"
#define OPENRAC_RAC1_SIG_Moby_CollectBolt "void(u32)"
typedef void (*openrac_rac1_Moby_CollectBolt)(openrac_gaddr bolt);

/* Vendor_DrawIconStrip (func_00239A00, rac1.vendor.icon_strip.draw): draws the vendor's item and
 * ammunition icons. The original also advances the carousel's scrolling. */
#define OPENRAC_RAC1_HOOK_Vendor_DrawIconStrip "Vendor_DrawIconStrip"
#define OPENRAC_RAC1_SIG_Vendor_DrawIconStrip "void()"
typedef void (*openrac_rac1_Vendor_DrawIconStrip)(void);

/* Hero_EnterMoveState (func_L00_00222B80, rac1.player.enter_move_state): puts the hero into a
 * move state (+0x2084): HeroEnterMoveState, and the copy of it in each level with hero code of
 * its own (one hook point). It sets the state up, starts its animation when force is non-zero,
 * and returns 0 when it refuses the change (dying or a cutscene, unless force is 2). Every change
 * of his state goes through it. */
#define OPENRAC_RAC1_HOOK_Hero_EnterMoveState "Hero_EnterMoveState"
#define OPENRAC_RAC1_SIG_Hero_EnterMoveState "i32(i32,i32)"
typedef int32_t (*openrac_rac1_Hero_EnterMoveState)(int32_t state, int32_t force);

/* Hero_ResolveGravityMode (func_L00_00233CC0, rac1.player.resolve_gravity_mode): the hero's
 * gravity mode for the tick (+0x20B3: 0 the world's down, 1 the ground's normal), once a tick;
 * 1 on magnetic ground with the Magneboots and in the magnet walk (state 0x3F). */
#define OPENRAC_RAC1_HOOK_Hero_ResolveGravityMode "Hero_ResolveGravityMode"
#define OPENRAC_RAC1_SIG_Hero_ResolveGravityMode "i32()"
typedef int32_t (*openrac_rac1_Hero_ResolveGravityMode)(void);

/* Hero_UpdateEquippedItems (func_L00_00210FE0, rac1.player.update_equipped_items): what the hero
 * holds, once a tick: locks item selection and use (+0x20A7, +0x20AC) on magnetic ground with the
 * Magneboots, in water or dying, then runs the item mobys and the items. */
#define OPENRAC_RAC1_HOOK_Hero_UpdateEquippedItems "Hero_UpdateEquippedItems"
#define OPENRAC_RAC1_SIG_Hero_UpdateEquippedItems "void()"
typedef void (*openrac_rac1_Hero_UpdateEquippedItems)(void);

/* Hero_SteerPlanarVelocity (func_L00_00214520, rac1.player.steer_planar_velocity): steers the
 * hero's planar velocity (+0xE0) toward his target speed (+0x190) along his move yaw (+0x180), by
 * at most step a tick (up to three times that as it turns him): steer_hero_planar_velocity, and
 * its copy air_accel in the other levels (one hook point). The move states call it after setting
 * the target speed and the step, the Thruster-Pack's hover (state 0x81) among them. */
#define OPENRAC_RAC1_HOOK_Hero_SteerPlanarVelocity "Hero_SteerPlanarVelocity"
#define OPENRAC_RAC1_SIG_Hero_SteerPlanarVelocity "void(f32)"
typedef void (*openrac_rac1_Hero_SteerPlanarVelocity)(float step);

/* Hero_UpdateMovement (func_L00_002180F8, rac1.player.update_movement): the hero's movement for
 * the state he is in (+0x2084), once a tick from Hero_UpdateMotionInput: a case per state that
 * sets his speeds, turn and velocity (+0xE0), the Thruster-Pack's hover (state 0x81) among them.
 * hero_update_movement, and each other level's own build of it (one hook point). A replacement
 * can move him itself in one state and call the original for the others. */
#define OPENRAC_RAC1_HOOK_Hero_UpdateMovement "Hero_UpdateMovement"
#define OPENRAC_RAC1_SIG_Hero_UpdateMovement "void()"
typedef void (*openrac_rac1_Hero_UpdateMovement)(void);

/* FX_DrawWorldQuad (func_L00_001FD1D8, rac1.fx.world_quad.draw): queues one effect quad
 * (draw_world_quad): the record at quad holds four corners (+0x00), four RGBA words (+0x40),
 * four ST pairs (+0x50) and the GS words CLAMP, TEX0, TEX1, ALPHA (+0x70..+0x88), its corners
 * through matrix when it is not 0. Most effects' draws call it, many times a frame; a replacement
 * can rewrite the record's colours before it calls the original. */
#define OPENRAC_RAC1_HOOK_FX_DrawWorldQuad "FX_DrawWorldQuad"
#define OPENRAC_RAC1_SIG_FX_DrawWorldQuad "void(u32,u32,i32)"
typedef void (*openrac_rac1_FX_DrawWorldQuad)(openrac_gaddr quad, openrac_gaddr matrix, int32_t mode);

/* FX_ThrusterFlame_Draw (func_L00_002B4918, rac1.fx.thruster_flame.draw): draws one of the
 * Thruster-Pack's flames (a class 0xA7 moby; draw_joint_aligned_beam_effect), each frame it
 * burns: two quads in 0x80808080 and a glow in 0x7F2020FF through FX_DrawWorldQuad, then a
 * ribbon coloured from joint_beam_ribbon_colors. */
#define OPENRAC_RAC1_HOOK_FX_ThrusterFlame_Draw "FX_ThrusterFlame_Draw"
#define OPENRAC_RAC1_SIG_FX_ThrusterFlame_Draw "void(u32)"
typedef void (*openrac_rac1_FX_ThrusterFlame_Draw)(openrac_gaddr flame);

/* FX_SpawnBurst (func_L00_0026DA50, rac1.fx.burst.spawn): spawns a spark (spawn_burst) at pos
 * moving by dir for ticks ticks, its colour fading from colour to fade as it shrinks from size,
 * throwing off sparks of its colour while children is set. Returns it, or 0. The Thruster-Pack's
 * flames spawn theirs (0x4F007FFF) from their update, rac1.moby.moby167.update. */
#define OPENRAC_RAC1_HOOK_FX_SpawnBurst "FX_SpawnBurst"
#define OPENRAC_RAC1_SIG_FX_SpawnBurst "u32(u32,u32,i32,i32,i32,i32,f32)"
typedef openrac_gaddr (*openrac_rac1_FX_SpawnBurst)(openrac_gaddr pos, openrac_gaddr dir, int32_t colour, int32_t fade,
                                                    int32_t ticks, int32_t children, float size);

/* ------------------------------------------------------------------------------------------ *
 * Taking a hook point
 * ------------------------------------------------------------------------------------------ */

/* Takes a hook point by its short name: OPENRAC_RAC1_HOOK(api, mod, Hero_TurnToHeading, my_turn,
 * &s_turn_original). `api` and `mod` are what openrac_mod_load received, `replacement` a function of
 * the hook point's type (openrac_rac1_<point>), and `original` the address of a variable of that
 * type, which receives what the replacement goes on to. Evaluates to openrac_api.hook's result
 * (OPENRAC_HOOK_OK, or the reason). Only during openrac_mod_load. Since 1.0. */
#define OPENRAC_RAC1_HOOK(api, mod, point, replacement, original)                                  \
    (api)->hook(                                                                                   \
        (mod),                                                                                     \
        OPENRAC_RAC1_HOOK_##point,                                                                 \
        OPENRAC_RAC1_SIG_##point,                                                                  \
        (void*)(replacement),                                                                      \
        (void**)(original)                                                                         \
    )

/* ---- generated by sdk/tools/gen_binding.py from sdk/bindings/rac1.json: do not edit ---- */
/* Hook points added since this header was last written by hand, from the binding table.
 * As the others: on a port that lacks one, openrac_api.hook returns OPENRAC_HOOK_UNKNOWN. */

/* Hero_Update: The hero's whole update, once a tick from the level loop
 * (coordinate_hero_frame_update, and its copies update_hero_frame and HeroL03_FrameUpdate in
 * the other families of levels: one hook point): his animation's advance, the stick, his state
 * machine and movement (Hero_UpdateMotionInput), ledges and rails, his state's transitions, his
 * body's easing, his moby and matrices, what he holds and his drawing; it fades out when he
 * leaves the world's bounds. A replacement that does not call the original moves him by itself
 * and shows him with openrac_api.hero_set_pose. */
#define OPENRAC_RAC1_HOOK_Hero_Update "Hero_Update"
#define OPENRAC_RAC1_SIG_Hero_Update "void()"
typedef void (*openrac_rac1_Hero_Update)(void);

/* Help_Request: Queues help message message in the help window's slot (Help_Request): 1 when
 * queued, 0 when it cannot be. Every tutorial and hint of the levels comes through it. A mod
 * may replace the message: call the original with the new id and the same slot, and return its
 * result. The id must be in the level's table; one that is not shows nothing, though the result
 * is still 1. */
#define OPENRAC_RAC1_HOOK_Help_Request "Help_Request"
#define OPENRAC_RAC1_SIG_Help_Request "i32(i32,i32)"
typedef int32_t (*openrac_rac1_Help_Request)(int32_t message, int32_t slot);

/* Ship_InitTransition: Puts the hero in the ship for a landing or take-off
 * (initialize_level_transition_with_audio): the ship's records, the sound, and the chunks to
 * load. On Orxon its choice of chunks goes by whether the O2 Mask is owned. */
#define OPENRAC_RAC1_HOOK_Ship_InitTransition "Ship_InitTransition"
#define OPENRAC_RAC1_SIG_Ship_InitTransition "void(i32)"
typedef void (*openrac_rac1_Ship_InitTransition)(int32_t mode);

/* Menu_ItemGrid_Update: Moves the pause menu item grid's focus and takes its selection
 * (update_menu_grid_selection), each tick the grid is open. Returns the menu's action. */
#define OPENRAC_RAC1_HOOK_Menu_ItemGrid_Update "Menu_ItemGrid_Update"
#define OPENRAC_RAC1_SIG_Menu_ItemGrid_Update "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_ItemGrid_Update)(uint32_t grid);

/* Menu_ItemGrid_Draw: Draws the pause menu item grid (draw_menu_item_grid). Returns the render-
 * pass flags the menu passes on. */
#define OPENRAC_RAC1_HOOK_Menu_ItemGrid_Draw "Menu_ItemGrid_Draw"
#define OPENRAC_RAC1_SIG_Menu_ItemGrid_Draw "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_ItemGrid_Draw)(uint32_t grid);

/* Menu_ItemSlots_Update: Puts the highlighted item in one of the quick select's eight slots
 * (update_menu_item_slots), in the pause menu's quick select editor. */
#define OPENRAC_RAC1_HOOK_Menu_ItemSlots_Update "Menu_ItemSlots_Update"
#define OPENRAC_RAC1_SIG_Menu_ItemSlots_Update "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_ItemSlots_Update)(uint32_t list);

/* Menu_QuickSelect_Draw: Draws the pause menu's quick select ring (draw_pause_ring). Returns
 * the render-pass flags. */
#define OPENRAC_RAC1_HOOK_Menu_QuickSelect_Draw "Menu_QuickSelect_Draw"
#define OPENRAC_RAC1_SIG_Menu_QuickSelect_Draw "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_QuickSelect_Draw)(uint32_t screen);

/* Menu_QuickSelect_Load: Copies the eight saved quick select items into the pause menu's editor
 * (load_selection_list). */
#define OPENRAC_RAC1_HOOK_Menu_QuickSelect_Load "Menu_QuickSelect_Load"
#define OPENRAC_RAC1_SIG_Menu_QuickSelect_Load "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_QuickSelect_Load)(uint32_t list);

/* Menu_QuickSelect_Store: Copies the editor's eight quick select items (+0x30 of screen) back
 * to the saved quick select (pause_stage_sprite_lists). */
#define OPENRAC_RAC1_HOOK_Menu_QuickSelect_Store "Menu_QuickSelect_Store"
#define OPENRAC_RAC1_SIG_Menu_QuickSelect_Store "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_QuickSelect_Store)(uint32_t screen);

/* Menu_ItemText_Draw: Draws a menu text label from its configuration
 * (render_configured_text_label): the selected item's name and description among others. */
#define OPENRAC_RAC1_HOOK_Menu_ItemText_Draw "Menu_ItemText_Draw"
#define OPENRAC_RAC1_SIG_Menu_ItemText_Draw "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_ItemText_Draw)(uint32_t label);

/* Menu_ItemAmmo_Draw: Draws the selected item's ammo panel in the pause menu
 * (draw_selected_item_ammo). */
#define OPENRAC_RAC1_HOOK_Menu_ItemAmmo_Draw "Menu_ItemAmmo_Draw"
#define OPENRAC_RAC1_SIG_Menu_ItemAmmo_Draw "i32(i32)"
typedef int32_t (*openrac_rac1_Menu_ItemAmmo_Draw)(int32_t screen);

/* Menu_Preview_Bind: Binds the selected item, and what it carries, to the pause menu's preview
 * (update_item_preview_binding). */
#define OPENRAC_RAC1_HOOK_Menu_Preview_Bind "Menu_Preview_Bind"
#define OPENRAC_RAC1_SIG_Menu_Preview_Bind "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_Preview_Bind)(uint32_t preview);

/* Menu_Preview_Update: Makes or replaces the moby of the pause menu's item preview
 * (update_item_preview_moby). */
#define OPENRAC_RAC1_HOOK_Menu_Preview_Update "Menu_Preview_Update"
#define OPENRAC_RAC1_SIG_Menu_Preview_Update "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_Preview_Update)(uint32_t screen);

/* Menu_Preview_DrawAvailable: Draws the preview mobys of the owned, selected item
 * (draw_available_item_preview_mobys). Returns the render-pass flags. */
#define OPENRAC_RAC1_HOOK_Menu_Preview_DrawAvailable "Menu_Preview_DrawAvailable"
#define OPENRAC_RAC1_SIG_Menu_Preview_DrawAvailable "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_Preview_DrawAvailable)(uint32_t screen);

/* Menu_Preview_DrawSelected: Draws the selected item's preview, or the message for an item not
 * owned (draw_item_preview_screen). */
#define OPENRAC_RAC1_HOOK_Menu_Preview_DrawSelected "Menu_Preview_DrawSelected"
#define OPENRAC_RAC1_SIG_Menu_Preview_DrawSelected "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_Preview_DrawSelected)(uint32_t screen);

/* Menu_EquippedPreview_Update: Loads the preview of the gadget or weapon in hand
 * (LoadHandGadget). */
#define OPENRAC_RAC1_HOOK_Menu_EquippedPreview_Update "Menu_EquippedPreview_Update"
#define OPENRAC_RAC1_SIG_Menu_EquippedPreview_Update "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_EquippedPreview_Update)(uint32_t hand);

/* Menu_EquippedPreview_Draw: Draws the pause menu's preview of the hero and what he holds
 * (draw_menu_preview_objects). */
#define OPENRAC_RAC1_HOOK_Menu_EquippedPreview_Draw "Menu_EquippedPreview_Draw"
#define OPENRAC_RAC1_SIG_Menu_EquippedPreview_Draw "i32(u32)"
typedef int32_t (*openrac_rac1_Menu_EquippedPreview_Draw)(uint32_t screen);

/* Hud_QuickSelect_Init: Sets up the quick select's eight-slot selector in play
 * (initialize_eight_slot_hud_selector). */
#define OPENRAC_RAC1_HOOK_Hud_QuickSelect_Init "Hud_QuickSelect_Init"
#define OPENRAC_RAC1_SIG_Hud_QuickSelect_Init "void(u32)"
typedef void (*openrac_rac1_Hud_QuickSelect_Init)(uint32_t selector);

/* Hud_QuickSelect_Update: The quick select in play, each tick
 * (update_hud_item_selector_selection_and_visibility): its focus, whether it shows, and the
 * item chosen. */
#define OPENRAC_RAC1_HOOK_Hud_QuickSelect_Update "Hud_QuickSelect_Update"
#define OPENRAC_RAC1_SIG_Hud_QuickSelect_Update "void(u32)"
typedef void (*openrac_rac1_Hud_QuickSelect_Update)(uint32_t selector);

/* Hud_QuickSelect_Draw: Draws the quick select's item icons in play
 * (draw_hud_item_selector_panel). */
#define OPENRAC_RAC1_HOOK_Hud_QuickSelect_Draw "Hud_QuickSelect_Draw"
#define OPENRAC_RAC1_SIG_Hud_QuickSelect_Draw "i32(u32)"
typedef int32_t (*openrac_rac1_Hud_QuickSelect_Draw)(uint32_t panel);

/* Vendor_MenuUpdate: The vendor's purchase menu, each tick it is open (update_vendor_purchase):
 * buying items and ammo. */
#define OPENRAC_RAC1_HOOK_Vendor_MenuUpdate "Vendor_MenuUpdate"
#define OPENRAC_RAC1_SIG_Vendor_MenuUpdate "void()"
typedef void (*openrac_rac1_Vendor_MenuUpdate)(void);

/* Vendor_InitializeSelected: Sets up the mobys and text of the vendor's selected row
 * (initialize_vendor_selected_item_mobys). */
#define OPENRAC_RAC1_HOOK_Vendor_InitializeSelected "Vendor_InitializeSelected"
#define OPENRAC_RAC1_SIG_Vendor_InitializeSelected "void()"
typedef void (*openrac_rac1_Vendor_InitializeSelected)(void);

/* Ammo_ChooseDrop: Chooses the ammo a dying moby or a broken crate drops (ChooseDropItem):
 * writes the item to out_item and returns how many pickups. */
#define OPENRAC_RAC1_HOOK_Ammo_ChooseDrop "Ammo_ChooseDrop"
#define OPENRAC_RAC1_SIG_Ammo_ChooseDrop "i32(u32,u32)"
typedef int32_t (*openrac_rac1_Ammo_ChooseDrop)(uint32_t moby, uint32_t out_item);

/* Ammo_SpawnDrop: Makes an ammo pickup moby of item at pos (spawn_on_path); amount -1 takes the
 * item's own. Returns the moby, or 0. */
#define OPENRAC_RAC1_HOOK_Ammo_SpawnDrop "Ammo_SpawnDrop"
#define OPENRAC_RAC1_SIG_Ammo_SpawnDrop "u32(u32,i32,i32,i32)"
typedef uint32_t (*openrac_rac1_Ammo_SpawnDrop)(uint32_t pos, int32_t item, int32_t amount, int32_t flags);

/* Save_Prepare: Writes the game's state into a save and starts a full memory card write
 * (prepare_save_game). */
#define OPENRAC_RAC1_HOOK_Save_Prepare "Save_Prepare"
#define OPENRAC_RAC1_SIG_Save_Prepare "void(i32,i32)"
typedef void (*openrac_rac1_Save_Prepare)(int32_t buffer, int32_t slot);

/* Save_RequestAutosave: Writes the save's packets and asks for their memory card update
 * (memcard_Save), as an autosave does. */
#define OPENRAC_RAC1_HOOK_Save_RequestAutosave "Save_RequestAutosave"
#define OPENRAC_RAC1_SIG_Save_RequestAutosave "i32(i32,i32)"
typedef int32_t (*openrac_rac1_Save_RequestAutosave)(int32_t slot, int32_t flags);

/* Save_Restore: Restores a whole save from memory (memcard_RestoreGame); a new game's default
 * state goes through it too. The port keeps the player's options around it
 * (rac1/host/options.c). */
#define OPENRAC_RAC1_HOOK_Save_Restore "Save_Restore"
#define OPENRAC_RAC1_SIG_Save_Restore "void(u32)"
typedef void (*openrac_rac1_Save_Restore)(uint32_t save);

/* Save_RestoreData: Restores one checked save packet (memcard_RestoreData); loading a file
 * calls it directly. Returns non-zero when it took the packet. */
#define OPENRAC_RAC1_HOOK_Save_RestoreData "Save_RestoreData"
#define OPENRAC_RAC1_SIG_Save_RestoreData "i32(u32,i32,u32)"
typedef int32_t (*openrac_rac1_Save_RestoreData)(uint32_t src, int32_t slot, uint32_t table);

/* Save_NewGame: A new game (NewGameInit): the disc's default state, and level 0. The port's
 * --level N runs around it (rac1/host/boot.c). */
#define OPENRAC_RAC1_HOOK_Save_NewGame "Save_NewGame"
#define OPENRAC_RAC1_SIG_Save_NewGame "void()"
typedef void (*openrac_rac1_Save_NewGame)(void);

/* Save_NewGamePlus: Challenge mode's start (load_level_zero_preserving_progress): the game
 * reset to level 0 with the progress it carries kept. */
#define OPENRAC_RAC1_HOOK_Save_NewGamePlus "Save_NewGamePlus"
#define OPENRAC_RAC1_SIG_Save_NewGamePlus "void(i32)"
typedef void (*openrac_rac1_Save_NewGamePlus)(int32_t slot);

/* Save_Serialize: Writes the whole save into out (memcard_MakeWholeSave), with no memory card
 * access. */
#define OPENRAC_RAC1_HOOK_Save_Serialize "Save_Serialize"
#define OPENRAC_RAC1_SIG_Save_Serialize "void(u32)"
typedef void (*openrac_rac1_Save_Serialize)(uint32_t out);

/* Save_UpdateState: The memory card's state machine, each tick (memcard_update_state): saves
 * and loads go through it. */
#define OPENRAC_RAC1_HOOK_Save_UpdateState "Save_UpdateState"
#define OPENRAC_RAC1_SIG_Save_UpdateState "void()"
typedef void (*openrac_rac1_Save_UpdateState)(void);

/* msg_string: Resolves a game message id to its text. A mod may substitute the randomized
 * reward name while the game's reward banner is drawn. */
#define OPENRAC_RAC1_HOOK_msg_string "msg_string"
#define OPENRAC_RAC1_SIG_msg_string "i32(i32)"
typedef int32_t (*openrac_rac1_msg_string)(int32_t id);

/* Vendor_OpenMenu: Opens the vendor purchase menu, including the Gadgetron PDA remote vendor
 * when moby is zero. */
#define OPENRAC_RAC1_HOOK_Vendor_OpenMenu "Vendor_OpenMenu"
#define OPENRAC_RAC1_SIG_Vendor_OpenMenu "void(u32)"
typedef void (*openrac_rac1_Vendor_OpenMenu)(uint32_t moby);

/* Vendor_DrawDetails: Draws the vendor selected-item name and price panel. */
#define OPENRAC_RAC1_HOOK_Vendor_DrawDetails "Vendor_DrawDetails"
#define OPENRAC_RAC1_SIG_Vendor_DrawDetails "void()"
typedef void (*openrac_rac1_Vendor_DrawDetails)(void);

/* UI_IconFrame: Resolves an icon and animation frame to its HUD sprite frame. */
#define OPENRAC_RAC1_HOOK_UI_IconFrame "UI_IconFrame"
#define OPENRAC_RAC1_SIG_UI_IconFrame "i32(i32,i32)"
typedef int32_t (*openrac_rac1_UI_IconFrame)(int32_t icon, int32_t frame);

/* NPC_TryDialog: Tests an NPC interaction request; identifies the owner of a purchase dialogue. */
#define OPENRAC_RAC1_HOOK_NPC_TryDialog "NPC_TryDialog"
#define OPENRAC_RAC1_SIG_NPC_TryDialog "i32(u32,u32)"
typedef int32_t (*openrac_rac1_NPC_TryDialog)(uint32_t moby, uint32_t request);

/* UI_DrawSprite: Draws a HUD sprite as one packed GIF packet. */
#define OPENRAC_RAC1_HOOK_UI_DrawSprite "UI_DrawSprite"
#define OPENRAC_RAC1_SIG_UI_DrawSprite "void(i32,i32,i32,i32,i32,i32)"
typedef void (*openrac_rac1_UI_DrawSprite)(int32_t frame, int32_t x, int32_t y, int32_t width, int32_t height, int32_t alpha);

/* Pickup_StartScene: Reward acquisition presentation. */
#define OPENRAC_RAC1_HOOK_Pickup_StartScene "Pickup_StartScene"
#define OPENRAC_RAC1_SIG_Pickup_StartScene "void(i32)"
typedef void (*openrac_rac1_Pickup_StartScene)(int32_t scene);

/* GoldCase_StartDemo: Reward acquisition presentation. */
#define OPENRAC_RAC1_HOOK_GoldCase_StartDemo "GoldCase_StartDemo"
#define OPENRAC_RAC1_SIG_GoldCase_StartDemo "void(u32,i32)"
typedef void (*openrac_rac1_GoldCase_StartDemo)(uint32_t moby, int32_t item);

/* ---- end of the generated hook points ---- */

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAMES_RAC1_H */
