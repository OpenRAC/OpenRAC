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
    }  // namespace combat
    namespace item {
        using equip = rac2::Item_Equip;
        using give = rac2::Item_Give;
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
    namespace weapon {
        using take_ammo = rac2::Weapon_TakeAmmo;
    }  // namespace weapon
}  // namespace hooks

}  // namespace openrac::rac2
