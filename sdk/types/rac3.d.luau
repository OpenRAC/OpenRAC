-- Generated from sdk/bindings/rac3.json by sdk/tools/gen_binding.py. Do not edit.
-- SPDX-License-Identifier: MIT
--
-- The `openrac` global as a script mod for rac3 sees it (docs/port/MODS.md, section 11): put this
-- file where your editor's Luau language server reads definitions (luau-lsp: --definitions).
-- A hook point is named by its permanent id (rac3.player.take_damage) or one of its aliases.

type HookPoint =
      "rac3.player.take_damage"
    | "rac3.item.give"
    | "rac3.item.upgrade"
    | "rac3.weapon.level_up"
    | "rac3.weapon.take_ammo"
    | "rac3.weapon.add_ammo"
    | "rac3.vendor.price"
    | "rac3.vendor.build_list"
    | "rac3.vendor.stock_add"
    | "rac3.moby.record_death"
    | "rac3.combat.post_hit"
    | "rac3.bolt.spawn_pickups"
    | "rac3.bolt.spawn_pickup"
    | "rac3.bolt.collect"
    | "rac3.level.load"
    | "rac3.level.give_items"
    | "rac3.player.death_done"
    | "rac3.player.update"
    | "rac3.player.update_character"
    | "rac3.player.set_animation"
    | "rac3.player.remap_animation"
    | "rac3.player.switch_item"
    | "rac3.hud.markers.draw"
    | "rac3.combat.melee_at_joint"
    | "rac3.moby.update_collision_sphere"
    | "Bolt_Collect"
    | "Bolt_SpawnPickup"
    | "Bolt_SpawnPickups"
    | "Hero_DeathDone"
    | "Hero_RemapAnimation"
    | "Hero_SetAnimation"
    | "Hero_SwitchItem"
    | "Hero_TakeDamage"
    | "Hero_Update"
    | "Hero_UpdateCharacter"
    | "Hud_DrawMarkers"
    | "Item_Give"
    | "Item_Upgrade"
    | "Level_GiveItems"
    | "Level_Load"
    | "Moby_MeleeAtJoint"
    | "Moby_PostHit"
    | "Moby_RecordDeath"
    | "Moby_UpdateCollisionSphere"
    | "Vendor_BuildList"
    | "Vendor_Price"
    | "Vendor_StockAdd"
    | "Weapon_AddAmmo"
    | "Weapon_LevelUp"
    | "Weapon_TakeAmmo"
type MemoryType = "u8" | "i8" | "u16" | "i16" | "u32" | "i32" | "f32"

type Fields = {
}

-- rac3.player.take_damage (Hero_TakeDamage) void(i32): Takes a hit's points from the hero's health (the s32 at 0x001A7430, hero +0x2850), scaled by the armour's factor and the help measure's, at least 1 and never below 0 (hero_damage); the hero's hit reaction calls it for each hit. Going Commando's rac2.player.take_damage.
type Hero_TakeDamageFn = (amount: number) -> ()
-- rac3.item.give (Item_Give) void(i32): Gives the player item id (item_give): its owned and new bytes (0x00142CA0, 0x00142D40, a byte per item), its row's starting ammo to a gun that has none, the vendor's list (Vendor_StockAdd) and the quick select. Givers, purchases and each level's start (Level_GiveItems) go through it.
type Item_GiveFn = (id: number) -> ()
-- rac3.item.upgrade (Item_Upgrade) i32(i32): The vendor's Mega upgrade of item id (weapon_buy_mega): its level set to 5, its ammo filled, its experience cleared. The vendor calls it after the price is paid; a replacement can carry out its own upgrade.
type Item_UpgradeFn = (id: number) -> number
-- rac3.weapon.level_up (Weapon_LevelUp) void(i32): Steps weapon id to its next level (weapon_level_up): its row (the item's byte at 0x001425C0) moves along the row's +0x46, its experience is raised to the old row's step and its ammo filled. A replacement that does not call the original blocks the step.
type Weapon_LevelUpFn = (id: number) -> ()
-- rac3.weapon.take_ammo (Weapon_TakeAmmo) i32(i32,i32): Takes rounds of a gun's ammo (ammo_use; item -1: the one in hand): 1 when there were enough, 0 otherwise; always 1 for a gun without a maximum. Every gun calls it as it fires. Rac1's TakeAmmo.
type Weapon_TakeAmmoFn = (item: number, amount: number) -> number
-- rac3.weapon.add_ammo (Weapon_AddAmmo) i32(i32,i32): Adds rounds to a gun's ammo (ammo_add, the words at 0x001427F0), capped at its row's maximum (+0x8A); returns what did not fit. Ammo pickups and the vendor call it.
type Weapon_AddAmmoFn = (item: number, amount: number) -> number
-- rac3.vendor.price (Vendor_Price) i32(): The price in bolts of the vendor's row under the cursor (vendor_price): the gun's (+0x7C of its row), its level-5 row's for a Mega row, 9/10 of it with the discount. The vendor's screen and the purchase both ask it.
type Vendor_PriceFn = () -> number
-- rac3.vendor.build_list (Vendor_BuildList) i32(i32): Fills the vendor's weapons rows from its stock (vendor_weapon_list): a rounds row for each owned gun that is not full, a special row, a row for each gun for sale (Mega rows in challenge mode), and keeps the cursor on its item. The vendor calls it as it opens and after a purchase.
type Vendor_BuildListFn = (mode: number) -> number
-- rac3.vendor.stock_add (Vendor_StockAdd) i32(i32): Lists item id for the vendor (vendor_stock_add; vendor_stock at 0x001D54D8, a u16 per entry: the item, bit 15 owned, bit 14 not yet for sale). Item_Give calls it.
type Vendor_StockAddFn = (id: number) -> number
-- rac3.moby.record_death (Moby_RecordDeath) void(u32,i32,i32): An enemy's death (moby_killed): its class's experience to the gun that killed it (by its damage record), the hero's experience and its bolt drop (Bolt_SpawnPickups). Enemies call it as they die. Going Commando's rac2.moby.record_death.
type Moby_RecordDeathFn = (moby: number, hit: number, killer: number) -> ()
-- rac3.combat.post_hit (Moby_PostHit) void(u32,u32): Queues a damage record (a game address) for moby victim in the hit ring (damage_queue; 0x001EED00, 64 entries): a moby keeps one pending (+0xA8), which a hit at least as strong replaces and a weaker one only adds its flags to. The guns and the shock mod call it. Going Commando's rac2.combat.post_hit.
type Moby_PostHitFn = (victim: number, record: number) -> ()
-- rac3.bolt.spawn_pickups (Bolt_SpawnPickups) void(u32,i32,i32,i32,i32,i32,f32): Spawns a burst of bolt coins from moby (bolts_drop): an amount from low to high split into the biggest coins that fit, each made by Bolt_SpawnPickup; flag 8 at most maximum coins (the rest banked, 0x0014269C). Enemies' drops (Moby_Killed) and the levels' scripted bursts come here. A replacement can change low and high, or make the coins itself.
type Bolt_SpawnPickupsFn = (moby: number, low: number, high: number, flags: number, surface: number, maximum: number, height: number) -> ()
-- rac3.bolt.spawn_pickup (Bolt_SpawnPickup) i32(i32,i32,i32,i32,i32,i32): Makes one bolt coin worth value at position with velocity (game addresses of four floats each) from source (bolt_coin_spawn): its moby, or 0; 64 units or more from the camera the value is banked instead.
type Bolt_SpawnPickupFn = (source: number, position: number, velocity: number, flags: number, value: number, counted: number) -> number
-- rac3.bolt.collect (Bolt_Collect) i32(i32): A bolt coin collected (bolt_pickup_collect): its value added to the player's bolts (0x00142660), the bolt HUD shown for 180 frames and its sparkle. The coin's update calls it as it reaches the hero. A replacement can add more with openrac_api.bolts.
type Bolt_CollectFn = (moby: number) -> number
-- rac3.level.load (Level_Load) i32(): The way to the next level (level_load, each level's copy): picks the transition for the level asked for, sets level_current (0x001D545C) and starts reading its files. Going Commando's rac2.level.load.
type Level_LoadFn = () -> number
-- rac3.level.give_items (Level_GiveItems) void(): A level's start (give_level_items): the standing items (the suit, the camera options, the thruster-pack) through Item_Give, then the level's own. Every level calls it as it starts.
type Level_GiveItemsFn = () -> ()
-- rac3.player.death_done (Hero_DeathDone) void(): The hero's death over (hero_death_done: state 57 ended, or a fall out of the level): the death counted in the stats and the respawn asked for (0x001A71D2), which the level carries out at the end of the frame.
type Hero_DeathDoneFn = () -> ()
-- rac3.player.update (Hero_Update) void(): The hero's whole update each frame (hero_tick), from the level's frame in play (gameplay_update, game mode 0) after every moby's update (moby_update_all) and before the frame's draw: the pad, his state machine and movement, ledges, his collision and animation and the camera's copy of him. Florana (level 2) and levels 4, 8, 9 and 27 have copies of their own; together they are one hook point over the 34 story levels. While he is in another body (hero +0x25E4 not 0) it runs rac3.player.update_character in place of its own steps. Going Commando's rac2.player.update.
type Hero_UpdateFn = () -> ()
-- rac3.player.update_character (Hero_UpdateCharacter) void(): The hero's update while he is in another body (hero +0x25E4: 1 Clank, 2 Clank in Holostar Studios, 3 a disguise such as the Tyhrra-Guise, 5 Qwark in the vid-comics): rac3.player.update calls it each frame in place of its own steps, for that body's moby, state, collision and animation. Levels 2 and 9, level 23 and the vid-comics (30 to 36) have copies of their own; together they are one hook point over the 34 story levels.
type Hero_UpdateCharacterFn = () -> ()
-- rac3.player.set_animation (Hero_SetAnimation) void(i32,i32,f32): Starts a hero animation (hero_anim_set): sequence from keyframe start_key, blended in from the pose he is in over blend ticks (negative: a blend curve, -1 is 11 ticks, -2 is 16). In a disguise (hero +0x25E4 is 3) the sequence goes through rac3.player.remap_animation first. The hero's states call it.
type Hero_SetAnimationFn = (sequence: number, start_key: number, blend: number) -> ()
-- rac3.player.remap_animation (Hero_RemapAnimation) i32(i32): The body's sequence for a hero animation (hero_guise_anim_remap) on the disguise classes 0x1A4E and 0x173B (the class of the hero's moby, its +0xAA); any other class gets the animation back. rac3.player.set_animation calls it while he wears a disguise (hero +0x25E4 is 3).
type Hero_RemapAnimationFn = (animation: number) -> number
-- rac3.player.switch_item (Hero_SwitchItem) void(i32): What the hero holds in a hand slot (hero_item_switch; slot 0 is his hand): Square brings out the wrench, Circle (or R1 where the settings say R1 fires) the equipped gun, L2 and R2 with the wrench out the gun with its lock-on mod. The hero's states call it each frame; a replacement that does not call the original keeps what he holds.
type Hero_SwitchItemFn = (slot: number) -> ()
-- rac3.hud.markers.draw (Hud_DrawMarkers) void(i32): Draws a player's screen markers (reticle_markers_draw, the levels' copy of the front end's), the ones queued this frame with reticle_marker_add (func_L01_003F6F20 on PAL), from the frame's draw after the level's update; with one view (0x001D6698 is 0) it then empties the queue. The 34 story levels.
type Hud_DrawMarkersFn = (player: number) -> ()
-- rac3.combat.melee_at_joint (Moby_MeleeAtJoint) i32(u32,i32,f32,i32,f32,f32,i32,i32,i32): Runs a ball damage query at an attacker's joint (ball_damage_pushed_at_joint), passing flags into the query before collision filtering. Returns the query's result. The PAL catalogue lists this function in levels 1, 2, 4-10, 12, 13, 16, 18-24 and 26-29; it does not list it in 3, 11, 14, 17 or 30-36.
type Moby_MeleeAtJointFn = (attacker: number, joint: number, radius: number, flags: number, distance: number, scale: number, first: number, second: number, mode: number) -> number
-- rac3.moby.update_collision_sphere (Moby_UpdateCollisionSphere) void(u32,i32,u32,f32,f32): Updates an indexed collision sphere using a guest vector and two scale parameters. The moby collision pointer must refer to compatible writable sphere records. PAL catalogue lists a copy in all 49 levels; Veldin 0x00439790, Florana 0x004119C8, Marcadia 0x00448F10. Hooks must preserve this requirement when forwarding.
type Moby_UpdateCollisionSphereFn = (moby: number, index: number, vector: number, fa: number, fb: number) -> ()

declare openrac: {
    game: string,
    version: "pal" | "ntsc",
    platform: string,
    api: string,
    log: (...any) -> (),
    warn: (...any) -> (),
    setting: (key: string) -> any,
    hook: (name: HookPoint, fn: (original: (...number) -> ...number, ...number) -> ...number) -> (),
    before: (name: HookPoint, fn: (...number) -> ()) -> (),
    after: (name: HookPoint, fn: (...number) -> ()) -> (),
    on_frame: (fn: () -> ()) -> (),
    on_pad: (fn: (port: number, report: buffer) -> ()) -> (),
    read: (kind: MemoryType, address: number) -> number,
    write: (kind: MemoryType, address: number, value: number) -> (),
    fields: Fields,
    -- interface 1.2: the keyboard and mouse; a bind is a name of openrac/keys.h ("KeyW", "MouseLeft", "WheelUp")
    key: (bind: string) -> boolean,
    pressed: (bind: string) -> boolean,
    mouse: () -> (number, number, number, number),
    window: () -> (boolean, boolean, boolean),
    -- interface 1.3: the cursor over the game's picture, 0..1 each way (x * 512, y * 416 the game's 2D pixel), and whether it is over it
    pointer: () -> (number, number, boolean),
    take_input: (...("keyboard" | "mouse")) -> (),
    capture_mouse: (capture: boolean) -> (),
    -- interface 1.13: the on-screen pad's action buttons (MODS.md, "Touch controls")
    touch_define: (json: string) -> boolean,
    touch_icon: (name: string, svg: string) -> boolean,
    touch_push: (context: string?) -> (),
    touch_hint: (hint: string, on: boolean) -> (),
    -- interface 1.15: the mod's own settings set (for the run and in the launcher's list), and its changes heard
    set_setting: (key: string, value: any) -> boolean,
    on_settings: (fn: () -> ()) -> (),
    -- interface 1.31: a pad button's label (an OPENRAC_PAD_* bit: 0x8000 Square) for the device in use, and its kind
    button_label: (button: number) -> (string?, ("playstation" | "letters" | "keys")?),
    -- interface 1.37: the pads' motors as the game's vibration drives them (small on or off, large 0 to 255)
    set_motors: (port: number, small_motor: boolean, large_motor: number) -> boolean,
    -- interface 1.37: an option of the port's menu greyed out or made usable, by its name (menu-options.md)
    menu_option_set: (option: string, state: "on" | "off" | "default") -> boolean,
}
