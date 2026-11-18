-- Generated from sdk/bindings/rac2.json by sdk/tools/gen_binding.py. Do not edit.
-- SPDX-License-Identifier: MIT
--
-- The `openrac` global as a script mod for rac2 sees it (docs/port/MODS.md, section 11): put this
-- file where your editor's Luau language server reads definitions (luau-lsp: --definitions).
-- A hook point is named by its permanent id (rac2.player.take_damage) or one of its aliases.

type HookPoint =
      "rac2.player.take_damage"
    | "rac2.combat.get_hit"
    | "rac2.combat.post_hit"
    | "rac2.weapon.take_ammo"
    | "rac2.moby.record_death"
    | "rac2.level.load"
    | "rac2.player.approach_speed"
    | "rac2.player.nanotech_steps"
    | "rac2.item.give"
    | "rac2.item.equip"
    | "rac2.weapon.level_up"
    | "rac2.item.upgrade"
    | "rac2.vendor.build_list"
    | "rac2.vendor.price"
    | "rac2.item.colors"
    | "rac2.vendor.draw_list"
    | "rac2.item.version_color"
    | "rac2.text.by_id"
    | "rac2.planet.unlock"
    | "rac2.moby.dynamo_pickup.update"
    | "rac2.moby.tractor_beam_seller.update"
    | "rac2.moby.box_breaker_pickup.update"
    | "rac2.moby.thermanator_seller.update"
    | "rac2.moby.thermanator_area.update"
    | "rac2.moby.swingshot_area.update"
    | "rac2.moby.glider_pickup.update"
    | "rac2.moby.levitator_seller.update"
    | "rac2.moby.charge_boots_rider.update"
    | "rac2.moby.race_controller.update"
    | "rac2.moby.mapper_return.update"
    | "rac2.moby.hypnomatic_maker.update"
    | "rac2.moby.nanotech_ball.update"
    | "Hero_ApproachSpeed"
    | "Hero_NanotechSteps"
    | "Hero_TakeDamage"
    | "Item_Colors"
    | "Item_Equip"
    | "Item_Give"
    | "Item_Upgrade"
    | "Item_VersionColor"
    | "Level_Load"
    | "Moby_BoxBreakerPickup_Update"
    | "Moby_ChargeBootsRider_Update"
    | "Moby_DynamoPickup_Update"
    | "Moby_GetHitMessage"
    | "Moby_GliderPickup_Update"
    | "Moby_HypnomaticMaker_Update"
    | "Moby_LevitatorSeller_Update"
    | "Moby_MapperReturn_Update"
    | "Moby_NanotechBall_Update"
    | "Moby_PostHit"
    | "Moby_RaceController_Update"
    | "Moby_RecordDeath"
    | "Moby_SwingshotArea_Update"
    | "Moby_ThermanatorArea_Update"
    | "Moby_ThermanatorSeller_Update"
    | "Moby_TractorBeamSeller_Update"
    | "Planet_Unlock"
    | "Text_ById"
    | "Vendor_BuildList"
    | "Vendor_DrawList"
    | "Vendor_Price"
    | "Weapon_LevelUp"
    | "Weapon_TakeAmmo"
type MemoryType = "u8" | "i8" | "u16" | "i16" | "u32" | "i32" | "f32"

type Fields = {
}

-- rac2.player.take_damage (Hero_TakeDamage) void(i32): Takes a hit's points from the hero's health (the hero at 0x189E20 on NTSC-U, health at +0x24CC), after the armour's scale; the hero's state machine calls it for each hit.
type Hero_TakeDamageFn = (amount: number) -> ()
-- rac2.combat.get_hit (Moby_GetHitMessage) u32(u32,u32,i32): The pending hit for a moby from the hit ring (the damage, f32 at +0x2C; its source at +0x38), or 0: what an enemy's damage path reads.
type Moby_GetHitMessageFn = (moby: number, mask: number, keep: number) -> number
-- rac2.combat.post_hit (Moby_PostHit) void(u32,u32): Posts a hit to moby victim from request (a game address: +0x00 the place, +0x10 the attacker, +0x14 the weapon or attack mask, +0x1C the damage, f32), into the hit ring Moby_GetHitMessage reads (0x1BF200, 64 entries of 0x40 bytes); a weaker hit on a moby with one pending only adds its mask. The weapons' shots, explosions and the tethers call it, each with the damage of its weapon's version. A replacement can change the request's damage for the call (the caller may post the same request to other victims: put it back after).
type Moby_PostHitFn = (victim: number, request: number) -> ()
-- rac2.weapon.take_ammo (Weapon_TakeAmmo) i32(i32,i32): Takes rounds of a weapon's ammo (item -1: the one in hand); a weapon with no capacity has endless ammo. Rac1's TakeAmmo.
type Weapon_TakeAmmoFn = (item: number, amount: number) -> number
-- rac2.moby.record_death (Moby_RecordDeath) void(u32,i32,i32): A moby's death recorded: its experience, the bestiary and its bolt drop (the moby's s16 at +0xB4); crates call it too.
type Moby_RecordDeathFn = (moby: number, flags: number, arg: number) -> ()
-- rac2.level.load (Level_Load) i32(i32,i32): Loads a level (new, or resumed after a death); it calls the gameplay file's loader.
type Level_LoadFn = (load: number, resume: number) -> number
-- rac2.player.approach_speed (Hero_ApproachSpeed) void(f32,f32): Eases the hero's speed (+0x1B4) toward his target speed (+0x1B0) each tick, at up when speeding up and down when slowing; rac1's approach_speed.
type Hero_ApproachSpeedFn = (up: number, down: number) -> ()
-- rac2.player.nanotech_steps (Hero_NanotechSteps) i32(u32,u32,u32): Adds the nanotech levels' steps of health to *count; the recount of the most health each tick adds the boosts after it (the HUD's experience bar calls it too, ignoring count).
type Hero_NanotechStepsFn = (a0: number, a1: number, count: number) -> number
-- rac2.item.give (Item_Give) void(i32): Gives the player item id (GiveInventoryItem): nothing when it is owned already; else sets its owned and new bytes, its starting ammo, adds it to the vendor's list and the quick select. The NPCs, pickups, arena prizes, the vendor's purchases and the start's items go through it; a few places set the owned bytes themselves (Endako's packs and Grindboots, Aranos Prison's Qwark Statuette). Rac1's GiveItem.
type Item_GiveFn = (id: number) -> ()
-- rac2.item.equip (Item_Equip) i32(i32): Puts item id in the hero's hand, or on him for the boots, helmets and packs (by the state of its record in the item table): a request his update carries out; 1 when one was made. Most givers call it right after Item_Give with the same id (Todano's Sheepinator, the Tractor Beam, the Thermanator, the Swingshot, the Mapper, ...), the vendor after a purchase; a mod that gives another item in a giver's place hands it this one's too.
type Item_EquipFn = (id: number) -> number
-- rac2.weapon.level_up (Weapon_LevelUp) void(i32): Steps weapon id to its next version by experience: its record (g_itemEquippedSlot) moves to the next row (+0x4A), its experience is kept at least the step's (+0x6C << 5), and its ammo filled to the new row's (+0x8E); nothing at its last row. The hero's experience check (func_L00_002D5EB8) calls it each tick a weapon's experience reaches its step, challenge mode's rows only past func_00288D58. A replacement that does not call the original blocks the step.
type Weapon_LevelUpFn = (id: number) -> ()
-- rac2.item.upgrade (Item_Upgrade) i32(i32): The vendor's Mega purchase of item id: its record moves two rows on (one for the two-version weapons 12, 14, 17, 18 and 53) and its experience is cleared; 1 when it moved, 0 at its last row. The vendor calls it after a purchase of an upgrade entry (Vendor_BuildList's +0x14 set) and its price is paid; a replacement can carry out an upgrade of its own and return 1.
type Item_UpgradeFn = (id: number) -> number
-- rac2.vendor.build_list (Vendor_BuildList) void(i32): Fills the vendor's list (g_vendorUi +0x140, 64 entries of 0x18 bytes: i32 item, kind (1 ammo), class (the preview model), price text, marked, upgrade; the count at +0x740): the weapons for sale, their ammo and, in challenge mode, their Mega upgrades. The vendor calls it as it opens and after each purchase. A replacement can add entries after the original's.
type Vendor_BuildListFn = (hide_unowned: number) -> ()
-- rac2.vendor.price (Vendor_Price) i32(): The price in bolts of the vendor entry under the cursor (g_vendorUi +0x80): an upgrade entry's from the row it moves to, a weapon's from its record (+0x80), 0 for ammo (priced per round). The vendor's dialog and its purchase both ask it, so a replacement prices an entry everywhere it shows.
type Vendor_PriceFn = () -> number
-- rac2.item.colors (Item_Colors) void(i32,u32,u32,u32): Writes item id's colours to three u32 the caller has (game addresses): the two of its experience bar and tint, its version's colour (0x00BBGGRR in the low bytes, from D_001ADAF8 or D_001ADB08 by version) that its icon and name are drawn in. The quick select (func_0033FB98) and the weapons screen's grid (func_00337210) call it for each item they show; a replacement can call the original and write its own.
type Item_ColorsFn = (id: number, first: number, second: number, tint: number) -> ()
-- rac2.vendor.draw_list (Vendor_DrawList) void(): Draws the vendor's list (g_vendorUi +0x140, the entries Vendor_BuildList made): for each of the rows -5 to +6 around the cursor (+0x80) that is in the list (under the count, +0x740), in that order, its item's icon in Item_VersionColor's colour (an upgrade entry, +0x14 set, in its kind 2, the Mega gold), then its label. A replacement can call the original and know which entry each Item_VersionColor call is for.
type Vendor_DrawListFn = () -> ()
-- rac2.item.version_color (Item_VersionColor) u32(i32): The colour of an item version kind (0x00BBGGRR in the low bytes; 2 the Mega gold): the vendor's list (Vendor_DrawList) asks it for each icon it draws. A replacement can return its own.
type Item_VersionColorFn = (kind: number) -> number
-- rac2.text.by_id (Text_ById) i32(i32): The game's text of an id: the address of its string in game memory, which every label, name and description the game prints asks (the vendor's list its upgrade entries' label, 0x325F, "Mega"; its dialog the item's name). Under the port's own texts (host/options.c). For a text of its own a replacement writes into one of the port's eight slots, 96 bytes each with the ending 0, which this lookup gives for ids 0x7F10 to 0x7F17 (call the game function by name, not the original), and returns that address. A replacement that calls the original gets the game's.
type Text_ByIdFn = (id: number) -> number
-- rac2.planet.unlock (Planet_Unlock) void(i32): Puts a level on the ship's map (MarkLevelAvailable): its byte at 0x1A7BC8 and its place in the map's order (0x139948, on NTSC-U v1.01). Coordinate sellers, story scenes and ship challenges call it, and every level load calls it for the level loaded. Rac1's UnlockPlanet.
type Planet_UnlockFn = (level: number) -> ()
-- rac2.moby.dynamo_pickup.update (Moby_DynamoPickup_Update) void(u32): Oozla's Dynamo pickup outside the Megacorp store (class 2709): plays its scene and gives item 36; tests item 36's owned byte.
type Moby_DynamoPickup_UpdateFn = (moby: number) -> ()
-- rac2.moby.tractor_beam_seller.update (Moby_TractorBeamSeller_Update) void(u32): Oozla's Megacorp scientist (class 2766): sells the Tractor Beam (item 46) for its record's price; tests item 46's owned byte.
type Moby_TractorBeamSeller_UpdateFn = (moby: number) -> ()
-- rac2.moby.box_breaker_pickup.update (Moby_BoxBreakerPickup_Update) void(u32): Oozla's Box Breaker pickup (class 4688): removes itself while item 50 is owned, else gives it.
type Moby_BoxBreakerPickup_UpdateFn = (moby: number) -> ()
-- rac2.moby.thermanator_seller.update (Moby_ThermanatorSeller_Update) void(u32): Barlow's inventor (class 3390): sells the Thermanator (item 39); tests item 39's owned byte.
type Moby_ThermanatorSeller_UpdateFn = (moby: number) -> ()
-- rac2.moby.thermanator_area.update (Moby_ThermanatorArea_Update) void(u32): Barlow's level start: with item 39 owned and the area's story byte (+0x12 of 0x1395B8) clear, starts the hero in the Thermanator's area.
type Moby_ThermanatorArea_UpdateFn = (moby: number) -> ()
-- rac2.moby.swingshot_area.update (Moby_SwingshotArea_Update) void(u32): Endako's level start: with item 13 owned and the area's story byte (+0x0D of 0x1395B8) clear, starts the hero in the Swingshot's area.
type Moby_SwingshotArea_UpdateFn = (moby: number) -> ()
-- rac2.moby.glider_pickup.update (Moby_GliderPickup_Update) void(u32): Tabora's Glider (class 3675): after its scene gives item 21; tests item 21's owned byte.
type Moby_GliderPickup_UpdateFn = (moby: number) -> ()
-- rac2.moby.levitator_seller.update (Moby_LevitatorSeller_Update) void(u32): Joba's shady salesman (class 3733): sells the Levitator (item 8); tests item 8's owned byte.
type Moby_LevitatorSeller_UpdateFn = (moby: number) -> ()
-- rac2.moby.charge_boots_rider.update (Moby_ChargeBootsRider_Update) void(u32): Joba's hoverbike race rider (class 2940): after the race's scene gives the Charge Boots (item 54) while they are not owned.
type Moby_ChargeBootsRider_UpdateFn = (moby: number) -> ()
-- rac2.moby.race_controller.update (Moby_RaceController_Update) void(u32): The hoverbike race's controller of Barlow and Joba (class 3004): on Joba gives the Charge Boots (item 54) while they are not owned; on Barlow puts Feltzin on the map.
type Moby_RaceController_UpdateFn = (moby: number) -> ()
-- rac2.moby.mapper_return.update (Moby_MapperReturn_Update) void(u32): Damosel's return from the Giant Clank battle (class 3841): gives the Mapper (item 5) while it is not owned.
type Moby_MapperReturn_UpdateFn = (moby: number) -> ()
-- rac2.moby.hypnomatic_maker.update (Moby_HypnomaticMaker_Update) void(u32): Damosel's Hypnomatic maker (class 4754): with the three parts sells the Hypnomatic (item 55); tests item 55's owned byte.
type Moby_HypnomaticMaker_UpdateFn = (moby: number) -> ()
-- rac2.moby.nanotech_ball.update (Moby_NanotechBall_Update) void(u32): The nanotech orb's update (class 0x326, the orb in a nanotech crate): flies its pieces to the hero, who gains health as each lands (+0x24CC of the hero, at most his max health), 1 a piece or 4 for the special orb. Rac1's nanotech_ball update.
type Moby_NanotechBall_UpdateFn = (moby: number) -> ()

declare openrac: {
    game: string,
    version: "ntsc" | "pal" | "ntsc2",
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
