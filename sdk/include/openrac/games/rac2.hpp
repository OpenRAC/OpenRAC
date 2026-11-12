// Generated from sdk/bindings/rac2.json by sdk/tools/gen_binding.py. Do not edit.
// SPDX-License-Identifier: MIT
//
// rac2's hook points as C++ types, for the SDK's C++ layer (openrac/mod.hpp, docs/port/MODS.md):
// each one's permanent id, its name and signature as openrac_api.hook takes them, and the type of a
// function of it. mod.hook<...>() asks for a hook point by its id. The same types by id are in
// openrac::rac2::hooks (rac2.player.take_damage: hooks::player::take_damage).
#pragma once

#include <cstdint>

namespace openrac::rac2 {

// Takes a hit's points from the hero's health (the hero at 0x189E20 on NTSC-U, health at +0x24CC), after the armour's scale; the hero's state machine calls it for each hit.
struct Hero_TakeDamage {
    static constexpr const char* id = "rac2.player.take_damage";
    static constexpr const char* name = "Hero_TakeDamage";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// The pending hit for a moby from the hit ring (the damage, f32 at +0x2C; its source at +0x38), or 0: what an enemy's damage path reads.
struct Moby_GetHitMessage {
    static constexpr const char* id = "rac2.combat.get_hit";
    static constexpr const char* name = "Moby_GetHitMessage";
    static constexpr const char* signature = "u32(u32,u32,i32)";
    using fn = uint32_t (*)(uint32_t, uint32_t, int32_t);
};

// Posts a hit to moby victim from request (a game address: +0x00 the place, +0x10 the attacker, +0x14 the weapon or attack mask, +0x1C the damage, f32), into the hit ring Moby_GetHitMessage reads (0x1BF200, 64 entries of 0x40 bytes); a weaker hit on a moby with one pending only adds its mask. The weapons' shots, explosions and the tethers call it, each with the damage of its weapon's version. A replacement can change the request's damage for the call (the caller may post the same request to other victims: put it back after).
struct Moby_PostHit {
    static constexpr const char* id = "rac2.combat.post_hit";
    static constexpr const char* name = "Moby_PostHit";
    static constexpr const char* signature = "void(u32,u32)";
    using fn = void (*)(uint32_t, uint32_t);
};

// Takes rounds of a weapon's ammo (item -1: the one in hand); a weapon with no capacity has endless ammo. Rac1's TakeAmmo.
struct Weapon_TakeAmmo {
    static constexpr const char* id = "rac2.weapon.take_ammo";
    static constexpr const char* name = "Weapon_TakeAmmo";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// A moby's death recorded: its experience, the bestiary and its bolt drop (the moby's s16 at +0xB4); crates call it too.
struct Moby_RecordDeath {
    static constexpr const char* id = "rac2.moby.record_death";
    static constexpr const char* name = "Moby_RecordDeath";
    static constexpr const char* signature = "void(u32,i32,i32)";
    using fn = void (*)(uint32_t, int32_t, int32_t);
};

// Loads a level (new, or resumed after a death); it calls the gameplay file's loader.
struct Level_Load {
    static constexpr const char* id = "rac2.level.load";
    static constexpr const char* name = "Level_Load";
    static constexpr const char* signature = "i32(i32,i32)";
    using fn = int32_t (*)(int32_t, int32_t);
};

// Eases the hero's speed (+0x1B4) toward his target speed (+0x1B0) each tick, at up when speeding up and down when slowing; rac1's approach_speed.
struct Hero_ApproachSpeed {
    static constexpr const char* id = "rac2.player.approach_speed";
    static constexpr const char* name = "Hero_ApproachSpeed";
    static constexpr const char* signature = "void(f32,f32)";
    using fn = void (*)(float, float);
};

// Adds the nanotech levels' steps of health to *count; the recount of the most health each tick adds the boosts after it (the HUD's experience bar calls it too, ignoring count).
struct Hero_NanotechSteps {
    static constexpr const char* id = "rac2.player.nanotech_steps";
    static constexpr const char* name = "Hero_NanotechSteps";
    static constexpr const char* signature = "i32(u32,u32,u32)";
    using fn = int32_t (*)(uint32_t, uint32_t, uint32_t);
};

// Gives the player item id (GiveInventoryItem): nothing when it is owned already; else sets its owned and new bytes, its starting ammo, adds it to the vendor's list and the quick select. The NPCs, pickups, arena prizes, the vendor's purchases and the start's items go through it; a few places set the owned bytes themselves (Endako's packs and Grindboots, Aranos Prison's Qwark Statuette). Rac1's GiveItem.
struct Item_Give {
    static constexpr const char* id = "rac2.item.give";
    static constexpr const char* name = "Item_Give";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Puts item id in the hero's hand, or on him for the boots, helmets and packs (by the state of its record in the item table): a request his update carries out; 1 when one was made. Most givers call it right after Item_Give with the same id (Todano's Sheepinator, the Tractor Beam, the Thermanator, the Swingshot, the Mapper, ...), the vendor after a purchase; a mod that gives another item in a giver's place hands it this one's too.
struct Item_Equip {
    static constexpr const char* id = "rac2.item.equip";
    static constexpr const char* name = "Item_Equip";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// Steps weapon id to its next version by experience: its record (g_itemEquippedSlot) moves to the next row (+0x4A), its experience is kept at least the step's (+0x6C << 5), and its ammo filled to the new row's (+0x8E); nothing at its last row. The hero's experience check (func_L00_002D5EB8) calls it each tick a weapon's experience reaches its step, challenge mode's rows only past func_00288D58. A replacement that does not call the original blocks the step.
struct Weapon_LevelUp {
    static constexpr const char* id = "rac2.weapon.level_up";
    static constexpr const char* name = "Weapon_LevelUp";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// The vendor's Mega purchase of item id: its record moves two rows on (one for the two-version weapons 12, 14, 17, 18 and 53) and its experience is cleared; 1 when it moved, 0 at its last row. The vendor calls it after a purchase of an upgrade entry (Vendor_BuildList's +0x14 set) and its price is paid; a replacement can carry out an upgrade of its own and return 1.
struct Item_Upgrade {
    static constexpr const char* id = "rac2.item.upgrade";
    static constexpr const char* name = "Item_Upgrade";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// Fills the vendor's list (g_vendorUi +0x140, 64 entries of 0x18 bytes: i32 item, kind (1 ammo), class (the preview model), price text, marked, upgrade; the count at +0x740): the weapons for sale, their ammo and, in challenge mode, their Mega upgrades. The vendor calls it as it opens and after each purchase. A replacement can add entries after the original's.
struct Vendor_BuildList {
    static constexpr const char* id = "rac2.vendor.build_list";
    static constexpr const char* name = "Vendor_BuildList";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// The price in bolts of the vendor entry under the cursor (g_vendorUi +0x80): an upgrade entry's from the row it moves to, a weapon's from its record (+0x80), 0 for ammo (priced per round). The vendor's dialog and its purchase both ask it, so a replacement prices an entry everywhere it shows.
struct Vendor_Price {
    static constexpr const char* id = "rac2.vendor.price";
    static constexpr const char* name = "Vendor_Price";
    static constexpr const char* signature = "i32()";
    using fn = int32_t (*)();
};

// Writes item id's colours to three u32 the caller has (game addresses): the two of its experience bar and tint, its version's colour (0x00BBGGRR in the low bytes, from D_001ADAF8 or D_001ADB08 by version) that its icon and name are drawn in. The quick select (func_0033FB98) and the weapons screen's grid (func_00337210) call it for each item they show; a replacement can call the original and write its own.
struct Item_Colors {
    static constexpr const char* id = "rac2.item.colors";
    static constexpr const char* name = "Item_Colors";
    static constexpr const char* signature = "void(i32,u32,u32,u32)";
    using fn = void (*)(int32_t, uint32_t, uint32_t, uint32_t);
};

// Draws the vendor's list (g_vendorUi +0x140, the entries Vendor_BuildList made): for each of the rows -5 to +6 around the cursor (+0x80) that is in the list (under the count, +0x740), in that order, its item's icon in Item_VersionColor's colour (an upgrade entry, +0x14 set, in its kind 2, the Mega gold), then its label. A replacement can call the original and know which entry each Item_VersionColor call is for.
struct Vendor_DrawList {
    static constexpr const char* id = "rac2.vendor.draw_list";
    static constexpr const char* name = "Vendor_DrawList";
    static constexpr const char* signature = "void()";
    using fn = void (*)();
};

// The colour of an item version kind (0x00BBGGRR in the low bytes; 2 the Mega gold): the vendor's list (Vendor_DrawList) asks it for each icon it draws. A replacement can return its own.
struct Item_VersionColor {
    static constexpr const char* id = "rac2.item.version_color";
    static constexpr const char* name = "Item_VersionColor";
    static constexpr const char* signature = "u32(i32)";
    using fn = uint32_t (*)(int32_t);
};

// The game's text of an id: the address of its string in game memory, which every label, name and description the game prints asks (the vendor's list its upgrade entries' label, 0x325F, "Mega"; its dialog the item's name). Under the port's own texts (host/options.c). For a text of its own a replacement writes into one of the port's eight slots, 96 bytes each with the ending 0, which this lookup gives for ids 0x7F10 to 0x7F17 (call the game function by name, not the original), and returns that address. A replacement that calls the original gets the game's.
struct Text_ById {
    static constexpr const char* id = "rac2.text.by_id";
    static constexpr const char* name = "Text_ById";
    static constexpr const char* signature = "i32(i32)";
    using fn = int32_t (*)(int32_t);
};

// Puts a level on the ship's map (MarkLevelAvailable): its byte at 0x1A7BC8 and its place in the map's order (0x139948, on NTSC-U v1.01). Coordinate sellers, story scenes and ship challenges call it, and every level load calls it for the level loaded. Rac1's UnlockPlanet.
struct Planet_Unlock {
    static constexpr const char* id = "rac2.planet.unlock";
    static constexpr const char* name = "Planet_Unlock";
    static constexpr const char* signature = "void(i32)";
    using fn = void (*)(int32_t);
};

// Oozla's Dynamo pickup outside the Megacorp store (class 2709): plays its scene and gives item 36; tests item 36's owned byte.
struct Moby_DynamoPickup_Update {
    static constexpr const char* id = "rac2.moby.dynamo_pickup.update";
    static constexpr const char* name = "Moby_DynamoPickup_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Oozla's Megacorp scientist (class 2766): sells the Tractor Beam (item 46) for its record's price; tests item 46's owned byte.
struct Moby_TractorBeamSeller_Update {
    static constexpr const char* id = "rac2.moby.tractor_beam_seller.update";
    static constexpr const char* name = "Moby_TractorBeamSeller_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Oozla's Box Breaker pickup (class 4688): removes itself while item 50 is owned, else gives it.
struct Moby_BoxBreakerPickup_Update {
    static constexpr const char* id = "rac2.moby.box_breaker_pickup.update";
    static constexpr const char* name = "Moby_BoxBreakerPickup_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Barlow's inventor (class 3390): sells the Thermanator (item 39); tests item 39's owned byte.
struct Moby_ThermanatorSeller_Update {
    static constexpr const char* id = "rac2.moby.thermanator_seller.update";
    static constexpr const char* name = "Moby_ThermanatorSeller_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Barlow's level start: with item 39 owned and the area's story byte (+0x12 of 0x1395B8) clear, starts the hero in the Thermanator's area.
struct Moby_ThermanatorArea_Update {
    static constexpr const char* id = "rac2.moby.thermanator_area.update";
    static constexpr const char* name = "Moby_ThermanatorArea_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Endako's level start: with item 13 owned and the area's story byte (+0x0D of 0x1395B8) clear, starts the hero in the Swingshot's area.
struct Moby_SwingshotArea_Update {
    static constexpr const char* id = "rac2.moby.swingshot_area.update";
    static constexpr const char* name = "Moby_SwingshotArea_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Tabora's Glider (class 3675): after its scene gives item 21; tests item 21's owned byte.
struct Moby_GliderPickup_Update {
    static constexpr const char* id = "rac2.moby.glider_pickup.update";
    static constexpr const char* name = "Moby_GliderPickup_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Joba's shady salesman (class 3733): sells the Levitator (item 8); tests item 8's owned byte.
struct Moby_LevitatorSeller_Update {
    static constexpr const char* id = "rac2.moby.levitator_seller.update";
    static constexpr const char* name = "Moby_LevitatorSeller_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Joba's hoverbike race rider (class 2940): after the race's scene gives the Charge Boots (item 54) while they are not owned.
struct Moby_ChargeBootsRider_Update {
    static constexpr const char* id = "rac2.moby.charge_boots_rider.update";
    static constexpr const char* name = "Moby_ChargeBootsRider_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// The hoverbike race's controller of Barlow and Joba (class 3004): on Joba gives the Charge Boots (item 54) while they are not owned; on Barlow puts Feltzin on the map.
struct Moby_RaceController_Update {
    static constexpr const char* id = "rac2.moby.race_controller.update";
    static constexpr const char* name = "Moby_RaceController_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Damosel's return from the Giant Clank battle (class 3841): gives the Mapper (item 5) while it is not owned.
struct Moby_MapperReturn_Update {
    static constexpr const char* id = "rac2.moby.mapper_return.update";
    static constexpr const char* name = "Moby_MapperReturn_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// Damosel's Hypnomatic maker (class 4754): with the three parts sells the Hypnomatic (item 55); tests item 55's owned byte.
struct Moby_HypnomaticMaker_Update {
    static constexpr const char* id = "rac2.moby.hypnomatic_maker.update";
    static constexpr const char* name = "Moby_HypnomaticMaker_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

// The nanotech orb's update (class 0x326, the orb in a nanotech crate): flies its pieces to the hero, who gains health as each lands (+0x24CC of the hero, at most his max health), 1 a piece or 4 for the special orb. Rac1's nanotech_ball update.
struct Moby_NanotechBall_Update {
    static constexpr const char* id = "rac2.moby.nanotech_ball.update";
    static constexpr const char* name = "Moby_NanotechBall_Update";
    static constexpr const char* signature = "void(u32)";
    using fn = void (*)(uint32_t);
};

namespace hooks {
    namespace combat {
        using get_hit = rac2::Moby_GetHitMessage;
        using post_hit = rac2::Moby_PostHit;
    }  // namespace combat
    namespace item {
        using colors = rac2::Item_Colors;
        using equip = rac2::Item_Equip;
        using give = rac2::Item_Give;
        using upgrade = rac2::Item_Upgrade;
        using version_color = rac2::Item_VersionColor;
    }  // namespace item
    namespace level {
        using load = rac2::Level_Load;
    }  // namespace level
    namespace moby {
        namespace box_breaker_pickup {
            using update = rac2::Moby_BoxBreakerPickup_Update;
        }  // namespace box_breaker_pickup
        namespace charge_boots_rider {
            using update = rac2::Moby_ChargeBootsRider_Update;
        }  // namespace charge_boots_rider
        namespace dynamo_pickup {
            using update = rac2::Moby_DynamoPickup_Update;
        }  // namespace dynamo_pickup
        namespace glider_pickup {
            using update = rac2::Moby_GliderPickup_Update;
        }  // namespace glider_pickup
        namespace hypnomatic_maker {
            using update = rac2::Moby_HypnomaticMaker_Update;
        }  // namespace hypnomatic_maker
        namespace levitator_seller {
            using update = rac2::Moby_LevitatorSeller_Update;
        }  // namespace levitator_seller
        namespace mapper_return {
            using update = rac2::Moby_MapperReturn_Update;
        }  // namespace mapper_return
        namespace nanotech_ball {
            using update = rac2::Moby_NanotechBall_Update;
        }  // namespace nanotech_ball
        namespace race_controller {
            using update = rac2::Moby_RaceController_Update;
        }  // namespace race_controller
        using record_death = rac2::Moby_RecordDeath;
        namespace swingshot_area {
            using update = rac2::Moby_SwingshotArea_Update;
        }  // namespace swingshot_area
        namespace thermanator_area {
            using update = rac2::Moby_ThermanatorArea_Update;
        }  // namespace thermanator_area
        namespace thermanator_seller {
            using update = rac2::Moby_ThermanatorSeller_Update;
        }  // namespace thermanator_seller
        namespace tractor_beam_seller {
            using update = rac2::Moby_TractorBeamSeller_Update;
        }  // namespace tractor_beam_seller
    }  // namespace moby
    namespace planet {
        using unlock = rac2::Planet_Unlock;
    }  // namespace planet
    namespace player {
        using approach_speed = rac2::Hero_ApproachSpeed;
        using nanotech_steps = rac2::Hero_NanotechSteps;
        using take_damage = rac2::Hero_TakeDamage;
    }  // namespace player
    namespace text {
        using by_id = rac2::Text_ById;
    }  // namespace text
    namespace vendor {
        using build_list = rac2::Vendor_BuildList;
        using draw_list = rac2::Vendor_DrawList;
        using price = rac2::Vendor_Price;
    }  // namespace vendor
    namespace weapon {
        using level_up = rac2::Weapon_LevelUp;
        using take_ammo = rac2::Weapon_TakeAmmo;
    }  // namespace weapon
}  // namespace hooks

}  // namespace openrac::rac2
