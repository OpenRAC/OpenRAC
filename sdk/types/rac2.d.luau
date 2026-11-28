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
    | "rac2.bolt.spawn_pickups"
    | "rac2.bolt.spawn_pickup"
    | "rac2.bolt.collect"
    | "rac2.arena.prize"
    | "rac2.armor_vendor.build_list"
    | "rac2.armor_vendor.buy"
    | "rac2.vendor.preview"
    | "rac2.player.update"
    | "rac2.collectibles.platinum_count"
    | "rac2.hero.update_collection_progress"
    | "rac2.save.serialize"
    | "rac2.save.new_game"
    | "rac2.moby.endako_rescue.update"
    | "rac2.moby.tabora_wrench.update"
    | "rac2.moby.prison_wrench.init"
    | "rac2.moby.todano_trader.update"
    | "rac2.moby.prison_plumber.update"
    | "rac2.moby.prison_controller.update"
    | "rac2.moby.siberius_thief.update"
    | "rac2.moby.boldan_cutscene.update"
    | "rac2.moby.hypnomatic_part.update"
    | "rac2.combat.count_kill"
    | "rac2.menu.extras"
    | "rac2.camera.follow.read_stick"
    | "rac2.moby.boldan_clock.update"
    | "rac2.moby.item_reward_trigger.update"
    | "Arena_Prize"
    | "ArmorVendor_BuildList"
    | "ArmorVendor_Buy"
    | "Bolt_Collect"
    | "Bolt_SpawnPickup"
    | "Bolt_SpawnPickups"
    | "FollowCamera_ReadStick"
    | "Hero_ApproachSpeed"
    | "Hero_NanotechSteps"
    | "Hero_TakeDamage"
    | "Hero_Update"
    | "Hero_UpdateCollectionProgress"
    | "Item_Colors"
    | "Item_Equip"
    | "Item_Give"
    | "Item_Upgrade"
    | "Item_VersionColor"
    | "Level_Load"
    | "Menu_Extras"
    | "Moby_BoldanClock_Update"
    | "Moby_BoldanCutscene_Update"
    | "Moby_BoxBreakerPickup_Update"
    | "Moby_ChargeBootsRider_Update"
    | "Moby_CountKill"
    | "Moby_DynamoPickup_Update"
    | "Moby_EndakoRescue_Update"
    | "Moby_GetHitMessage"
    | "Moby_GliderPickup_Update"
    | "Moby_HypnomaticMaker_Update"
    | "Moby_HypnomaticPart_Update"
    | "Moby_ItemRewardTrigger_Update"
    | "Moby_LevitatorSeller_Update"
    | "Moby_MapperReturn_Update"
    | "Moby_NanotechBall_Update"
    | "Moby_PostHit"
    | "Moby_PrisonController_Update"
    | "Moby_PrisonPlumber_Update"
    | "Moby_PrisonWrench_Init"
    | "Moby_RaceController_Update"
    | "Moby_RecordDeath"
    | "Moby_SiberiusThief_Update"
    | "Moby_SwingshotArea_Update"
    | "Moby_TaboraWrench_Update"
    | "Moby_ThermanatorArea_Update"
    | "Moby_ThermanatorSeller_Update"
    | "Moby_TodanoTrader_Update"
    | "Moby_TractorBeamSeller_Update"
    | "Planet_Unlock"
    | "PlatinumBolts_CountTotal"
    | "Save_NewGame"
    | "Save_Serialize"
    | "Text_ById"
    | "Vendor_BuildList"
    | "Vendor_DrawList"
    | "Vendor_Preview"
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
-- rac2.bolt.spawn_pickups (Bolt_SpawnPickups) void(u32,i32,i32,i32,i32,i32,f32): Spawns a burst of bolt coins from moby (moby_spawn_bolt_pickups in the executable and its copy in every level's code, part_spawn_bolt_pickups: one hook point): rolls an amount from low to high (doubled under the bolt multiplier, scaled by how fast the level's bolts are being earned), splits it into the biggest coins that fit (1, 5, 20, 50, 100, 500 and 1000, classes 0x0D to 0x13) and makes each with Bolt_SpawnPickup. flags 0 spawns nothing, 2 flies the coins to the hero, 8 makes at most maximum coins (the rest goes to the word at 0x1A7A14), 32 leaves them out of the level's bolt score; surface is a path the coins land on (-1 none), height a lift of the spawn point. Every drop comes here: Moby_RecordDeath's (enemies and crates; in a level its copy calls the level copy) and the levels' scripted bursts (part_spawn_scaled_bolts and its callers). A replacement can change low and high, or make the coins itself.
type Bolt_SpawnPickupsFn = (moby: number, low: number, high: number, flags: number, surface: number, maximum: number, height: number) -> ()
-- rac2.bolt.spawn_pickup (Bolt_SpawnPickup) u32(u32,u32,u32,u32,i32,i32): Makes one bolt coin worth value (1, 5, 20, 50, 100, 500 or 1000: classes 0x0D to 0x13; another value makes a 1) at position with velocity (game addresses of four floats each), from source (the moby it came from); counted (the byte at +0x40 of the coin's vars) says whether the level's bolt score counts it, flags 2 flies it to the hero. Returns the coin's moby, or 0: none when the pool is full, or when position is 64 units or more from the game's reference point, where the value goes to the word at 0x1A7A14 instead. Bolt_SpawnPickups calls it for each coin, and two level mobys directly (Endako's func_L03_00383FE8, Maktar's func_L02_003E7370). A replacement sees each coin and the moby made (to mark it, with openrac_api.moby_material for one), and can make another or none.
type Bolt_SpawnPickupFn = (source: number, position: number, velocity: number, flags: number, value: number, counted: number) -> number
-- rac2.bolt.collect (Bolt_Collect) void(u32): A bolt coin collected (CollectBolts): adds its class's value (classes 0x0D to 0x13: 1, 5, 20, 50, 100, 500, 1000; 1 for another) to the player's bolts (0x1A79F8) and, for a counted coin, to the level's bolt score, then throws its sparks. The coin's update (func_00300F58) calls it as the coin reaches the hero, the moby still alive, and reads no result. A replacement can change what a coin is worth: call the original and add more with openrac_api.bolts (a coin it marked worth five times: four times what the original added), or not call it.
type Bolt_CollectFn = (moby: number) -> ()
-- rac2.arena.prize (Arena_Prize) i32(i32): The bolts an arena challenge pays (moby_compute_scaled_entry_value, Maktar's and Joba's arena: one function in both levels): the challenge record's prize (+0xC of its entry in the level's challenge table), times 0.3 for each time it was won before, kept between 500 and the prize, ten times that in challenge mode. The arena's menu prints it and the win adds it to the bolts (and keeps it for the results screen), so a replacement changes a prize everywhere it shows.
type Arena_PrizeFn = (challenge: number) -> number
-- rac2.armor_vendor.build_list (ArmorVendor_BuildList) void(): Fills the armour vendor's list (the vendor in mode 3: a Megacorp Vendor whose pvars +0x94 is 2): g_vendorUi +0x140, 12-byte rows {i32 icon (0xEA92 + armour, a HUD icon id), i32 price (bolts), i32 armour (0 to 4: the armour the row sells, whose item record names it and whose pack the preview loads)}, the count at +0x740 (up to 128): each armour above the one worn (s16 at the save block +0x20) that its planet unlocked (g_game_flag_table[0x8D] bit, or the extras of a finished game), icon 0xEA92 + armour, price 0, 25000, 100000, 250000 or 1000000. With Carbonox (4) worn it lists nothing. The vendor calls it as it opens. A replacement can add rows after the original's; a row's armour must stay 0 to 4 (the preview, the name, the description table and the close index by it). The armour vendors are on Dobbo, Boldan, Snivelak and Grelbin.
type ArmorVendor_BuildListFn = () -> ()
-- rac2.armor_vendor.buy (ArmorVendor_Buy) void(u32,u32): The armour vendor's purchase dialog, once a frame while it is up (shop_dispatch_purchase_confirm, mode 3; every level's copy). answer: the address of its i32 answer (1 yes); the second word the dispatcher passes (the answer's neighbour) is not read. Its state is g_vendorUi +0x84: 1 opens it (too few bolts for the row's price: refused, state 2), 2 waits for Cross (answer 1) or Triangle (answer 0), then 3 decides: answer 1, the bolts at least the price and the armour worn not 4: pays the row's price, plays the purchase sound and message and closes the menu, which then wears the row's armour (shop_close_sub_menu); otherwise the refusal or nothing, state 0. The row is the one under the cursor (+0x80). A replacement can buy a row of its own at state 3 (pay, then set the answer to 0 so that the original closes the dialog as a refusal does) and call the original for the game's rows.
type ArmorVendor_BuyFn = (answer: number, unused: number) -> ()
-- rac2.vendor.preview (Vendor_Preview) void(): Sets up the vendor's preview of the row under the cursor (PrepareVendorSelectionPreview), in every mode, as the cursor moves and the menu opens: the armour vendor's (mode 3) loads the row's armour over Ratchet's class 0 when it is not the one loaded and makes the preview objects (g_vendorUi +0x2C, the count at +0x34, 0x100-byte mobys outside the level's list: class 0, Ratchet in that armour, then 1290 and 2675 for its extra pieces), turned to the camera, and shows the armour's description; the weapon vendor's, the weapon's model or picture. A replacement can call the original and then change the message or the objects' look (moby_material_class with OPENRAC_MODEL_MENU).
type Vendor_PreviewFn = () -> ()
-- rac2.player.update (Hero_Update) void(): The hero's whole update each tick: his animation, the stick, ledges, grind rails, his state machine, his moby and what he holds. Each group of levels has its own copy; together they are one hook point. A ship or vehicle update runs in its place while one drives him (hero +0x22B4). A replacement that skips the original moves him itself and shows him with openrac_api.hero_set_pose.
type Hero_UpdateFn = () -> ()
-- rac2.collectibles.platinum_count (PlatinumBolts_CountTotal) i32(): Counts the platinum bolts recorded in the save. A replacement can return received currency independently of physical pickups.
type PlatinumBolts_CountTotalFn = () -> number
-- rac2.hero.update_collection_progress (Hero_UpdateCollectionProgress) void(): Recounts nanotech and weapon collection progress from inventory; mods can separate pickup flags from received ownership.
type Hero_UpdateCollectionProgressFn = () -> ()
-- rac2.save.serialize (Save_Serialize) void(u32): Serializes live game state to the save packet at output. Inventory substitutions made by a mod must be restored before calling the original.
type Save_SerializeFn = (output: number) -> ()
-- rac2.save.new_game (Save_NewGame) void(): Resets game data from disc defaults for a new campaign, before its starting level is entered.
type Save_NewGameFn = () -> ()
-- rac2.moby.endako_rescue.update (Moby_EndakoRescue_Update) void(u32): Updates the Endako rescue sequence that returns Clank and the Swingshot.
type Moby_EndakoRescue_UpdateFn = (moby: number) -> ()
-- rac2.moby.tabora_wrench.update (Moby_TaboraWrench_Update) void(u32): Updates the Tabora controller, including its wrench reward.
type Moby_TaboraWrench_UpdateFn = (moby: number) -> ()
-- rac2.moby.prison_wrench.init (Moby_PrisonWrench_Init) void(u32): Initializes the Aranos prison wrench pickup.
type Moby_PrisonWrench_InitFn = (moby: number) -> ()
-- rac2.moby.todano_trader.update (Moby_TodanoTrader_Update) void(u32): Updates the Todano trader and his reward exchange.
type Moby_TodanoTrader_UpdateFn = (moby: number) -> ()
-- rac2.moby.prison_plumber.update (Moby_PrisonPlumber_Update) void(i32): Updates the Aranos plumber and his reward. The game declaration takes a signed 32-bit moby address.
type Moby_PrisonPlumber_UpdateFn = (moby: number) -> ()
-- rac2.moby.prison_controller.update (Moby_PrisonController_Update) void(u32): Updates the Aranos prison controller, including inventory restoration.
type Moby_PrisonController_UpdateFn = (moby: number) -> ()
-- rac2.moby.siberius_thief.update (Moby_SiberiusThief_Update) void(u32): Updates the Siberius thief sequence and its progression state.
type Moby_SiberiusThief_UpdateFn = (moby: number) -> ()
-- rac2.moby.boldan_cutscene.update (Moby_BoldanCutscene_Update) void(u32): Updates the Boldan cutscene trigger and its temporary inventory state.
type Moby_BoldanCutscene_UpdateFn = (moby: number) -> ()
-- rac2.moby.hypnomatic_part.update (Moby_HypnomaticPart_Update) void(u32): Updates the Hypnomatic part pickup on Damosel.
type Moby_HypnomaticPart_UpdateFn = (moby: number) -> ()
-- rac2.combat.count_kill (Moby_CountKill) void(u32,i32): Records a kill or event and grants its rewards. A replacement can adjust reward state around the original call.
type Moby_CountKillFn = (moby: number, type: number) -> ()
-- rac2.menu.extras (Menu_Extras) i32(): Processes the extras menu and returns its menu result.
type Menu_ExtrasFn = () -> number
-- rac2.camera.follow.read_stick (FollowCamera_ReadStick) void(u32,u32,u32): The follow camera's read of the right stick, each tick: camera (its data at +0x70), and the guest addresses of the floats it then turns and tilts by. By then the tick's goals are set: the eye's distance and height goals (data +0x1A0 and +0x1AC), which the camera eases its distance and height (+0x18C, +0x190) towards, and the eye it wants from Ratchet (+0x160); after it the camera turns, eases and builds its view, so a mod that calls the original and then changes those moves the camera this very tick. Rac1's FollowCamera_ReadStickInput.
type FollowCamera_ReadStickFn = (camera: number, turn: number, tilt: number) -> ()
-- rac2.moby.boldan_clock.update (Moby_BoldanClock_Update) void(u32): Updates Boldan's clock tower: sets its hands from the console's clock and, once a second of its time, shows the Nanotech Upgrade (the moby at its pvars +0x28, class 0x11C4) on an even hour of the twelve-hour dial and hides it on an odd one.
type Moby_BoldanClock_UpdateFn = (moby: number) -> ()
-- rac2.moby.item_reward_trigger.update (Moby_ItemRewardTrigger_Update) void(u32): Updates a reward trigger (item_reward_trigger_update): the scene of a story or reward spot on any level. It waits for the hero in its region, plays its scenes, then puts its planet on the ship's map (Planet_Unlock) and marks its moby group done. At its first update it deletes itself, unplayed, when its planet is on the map already (the byte at 0x1A7BC8 + planet, NTSC-U v1.01) or its group is done, so a replacement can show it another state of the map around the original.
type Moby_ItemRewardTrigger_UpdateFn = (moby: number) -> ()

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
    -- interface 1.31: a pad button's label (an OPENRAC_PAD_* bit: 0x8000 Square) for the device in use, and its kind
    button_label: (button: number) -> (string?, ("playstation" | "letters" | "keys")?),
    -- interface 1.37: the pads' motors as the game's vibration drives them (small on or off, large 0 to 255)
    set_motors: (port: number, small_motor: boolean, large_motor: number) -> boolean,
    -- interface 1.37: an option of the port's menu greyed out or made usable, by its name (menu-options.md)
    menu_option_set: (option: string, state: "on" | "off" | "default") -> boolean,
    -- interface 1.42: the version of another mod when it is loaded, else nil
    mod_loaded: (id: string) -> string?,
}
