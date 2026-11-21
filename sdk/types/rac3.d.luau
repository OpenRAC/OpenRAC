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
    | "Bolt_Collect"
    | "Bolt_SpawnPickup"
    | "Bolt_SpawnPickups"
    | "Hero_DeathDone"
    | "Hero_TakeDamage"
    | "Item_Give"
    | "Item_Upgrade"
    | "Level_GiveItems"
    | "Level_Load"
    | "Moby_PostHit"
    | "Moby_RecordDeath"
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
}
