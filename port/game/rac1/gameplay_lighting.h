// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Ratchet & Clank gameplay subjects for the enhanced renderer's GI-coloured rim.
// Class identities follow the existing editor/moby_classes.tsv labels and probe seed whitelist.
// Scenery, debris, projectiles and particle emitters are deliberately excluded.

#pragma once

#include <algorithm>
#include <array>

namespace openrac::game::rac1 {

// Sorted class identities shared by the PAL and NTSC gameplay render paths.
inline constexpr std::array kEssentialMobys{
    0,     // ratchet
    11,    // vendor
    13,    // bolt
    14,    // bolt_hexnut
    15,    // bolt_starnut
    16,    // bolt_circlenut
    18,    // magnetboots
    28,    // plasmabot
    29,    // blarg_turret
    30,    // machinegun_turret
    31,    // minibomber
    35,    // sharkigator
    36,    // blarg_gunner
    44,    // sentrybot
    63,    // robosquawker
    71,    // wrench
    87,    // clank
    90,    // bob
    101,   // blarg_space_fighter
    111,   // blarg_space_fighter
    114,   // resort_owner
    123,   // spiderbot
    133,   // amoeboid
    157,   // devastator
    163,   // visibomb
    168,   // blaster
    175,   // taunter
    176,   // pyrocitor
    177,   // tesla_claw
    180,   // walloper
    185,   // morphoray
    186,   // doom_bot
    188,   // trespasser
    190,   // mine_glove
    192,   // bomb_glove
    203,   // decoy
    204,   // devestator_ammo
    208,   // swingshot
    209,   // swingshot_hook
    211,   // plasmabot
    213,   // tesla_claw_ammo
    214,   // pyrocitor_ammo
    217,   // big_axebot
    222,   // visibomb_ammo
    223,   // glove_of_doom_ammo
    224,   // ammocrate
    225,   // mine_glove_ammo
    226,   // bomb_glove_ammo
    228,   // jet_fighter_health_crate
    229,   // glove_of_doom
    252,   // scoutbot
    253,   // bomber_tank
    280,   // bolt_crank
    282,   // miner
    294,   // blarg_trooper
    298,   // fred_pokitaru
    304,   // gold_weapon_case
    326,   // blarg_helicopter
    328,   // edwina
    333,   // blarg_bombthrower
    340,   // circular_saw_enemy
    347,   // dreks_fleet_turret
    419,   // giant_clank
    427,   // blarg_trooper
    433,   // sonic_summoner
    439,   // hoverboard
    447,   // qwark_umbris
    452,   // bolt_thief
    454,   // ryno
    459,   // birdbot
    479,   // drone
    491,   // ultramech_unlimited
    500,   // bolt_crate
    501,   // nanotech_crate
    502,   // metal_crate
    505,   // oom_oom_box
    511,   // ammo_crate
    562,   // decoy_glove
    563,   // woodcut_bot
    572,   // large_amoeboid
    573,   // robot_dog
    577,   // peckbot
    578,   // mine_layer
    580,   // sand_shark
    583,   // thruster_pack_lock_green
    585,   // metal_detector
    586,   // thruster_pack_lock_red
    607,   // helipack
    612,   // constructorbot
    615,   // tresspasser_pad
    623,   // extermibot
    625,   // extermitank
    631,   // blarg_helicopter
    638,   // blarg_elite_commando
    660,   // fighter_jet
    688,   // bomber_jet
    717,   // skid_mcmarx
    749,   // horny_toad
    750,   // infobot
    758,   // swingshot_versatarget
    774,   // the_plumber
    786,   // skids_agent
    788,   // skid_mcmarx
    803,   // swingshot_swinging_versatarget
    806,   // nanotech_ball
    815,   // beehive
    849,   // suckcannon
    851,   // qwark_oltanis
    857,   // gadgebot
    865,   // medium_amoeboid
    866,   // small_amoeboid
    871,   // boltcrate_possibly_for_metal_detector
    890,   // helga
    909,   // big_al
    914,   // robot_qwark
    915,   // fred_1
    916,   // fred_2
    917,   // fred_3
    918,   // hoverboard_girl
    919,   // qwarks_bouncer
    920,   // qwark_rilgar
    923,   // bolt_thief
    924,   // sam
    925,   // shady_salesman
    1005,  // tresspasser
    1006,  // blaster_ammo
    1016,  // hydrodisplacer
    1023,  // blarg_paratrooper
    1048,  // alien_snapper
    1051,  // alien_queen
    1059,  // sharkigator
    1068,  // blarg_space_commando
    1105,  // fred
    1106,  // snagglebeast
    1120,  // suck_cannon
    1126,  // machinegun_turret
    1130,  // commando
    1134,  // gold_bolt
    1143,  // vendor_orb
    1144,  // batalia_deserter
    1179,  // thruster_pack_lock
    1190,  // lieutenant
    1196,  // screamer
    1201,  // turret
    1202,  // toxic_crab
    1208,  // constructorbot
    1218,  // ammocrate
    1220,  // jet_fighter_health_crate
    1231,  // psytcopus
    1246,  // puffoid
    1249,  // drek
    1251,  // hydrodisplacer
    1262,  // blarg_space_commando
    1267,  // hoven_turret
    1283,  // the_plumber_batalia
    1289,  // o2_mask
    1290,  // pilots_helmet
    1326,  // nanotech_upgrade_vendor
    1371,  // blarg_commander
    1377,  // help_desk_lady
    1382,  // robot_dog
    1388,  // bolt_grabber
    1417,  // blarg_heavy_interceptor
    1422,  // drek_final_boss_robot
    1428,  // codebot
    1430,  // codebot_slot
    1438,  // ryno_ammo
    1440,  // veldin_robot_enemy
    1445,  // chomper
    1446,  // ultramech_scientist
    1447,  // drone_device_ammo
    1449,  // decoy_ammo
    1454,  // rocket_tank
    1455,  // gadgetron_ceo
    1456,  // gold_bomb_glove
    1457,  // gold_pyrocitor
    1458,  // gold_blaster
    1459,  // gold_glove_of_doom
    1460,  // gold_mine_glove
    1461,  // gold_devastator
    1462,  // gold_decoy_glove
    1463,  // gold_tesla_claw
    1464,  // gold_suck_cannon
    1465,  // gold_morphoray
    1564,  // pterodactyl
    1786,  // hoverboard
    1900,  // gold_decoy
    1906,  // gadgebot
};

// Returns whether a class represents a character, usable item, pickup or enemy.
constexpr bool essential_moby(int class_id) {
    return std::binary_search(kEssentialMobys.begin(), kEssentialMobys.end(), class_id);
}

}  // namespace openrac::game::rac1
