// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Progress flags of levels 0 to 8 (Aranos Tutorial to Tabora).
//!
//! Each place is a byte of the story chunk (5, at 0x001395B8) or a bit of the cinematics chunk
//! (43), named from the level code that reads and writes it. The notes per flag, and the flags of
//! these planets left out, are in `RAC2_SAVE_FORMAT.md`, "Planets 0 to 8 (L-6a)".
//!
//! The Electrolyzer puzzles ([`super::electrolyzer`]) are also the board's moby count (chunk 3005)
//! and object group (chunk 3004) in the level's section; they are in `RAC2_SAVE_FORMAT.md`,
//! "Advanced: Electrolyzer puzzles (L-7)".

use super::{count, electrolyzer, group_done, Confidence, Flag, Place};

/// A byte of the story chunk, set to `on`.
const fn story(offset: usize, on: u8) -> Place {
    Place::Byte { chunk: 5, level: None, offset, on }
}

/// One of Maktar's arena challenges: the byte counts the wins (`func_L02_003F9B70` gives the byte).
const fn arena(id: &'static str, name: &'static str, offset: usize) -> Flag {
    Flag {
        id,
        level: Some(2),
        name,
        description: "Won at least once: the arena's menu marks it done and later wins pay the repeat prize.",
        place: story(offset, 1),
        confidence: Confidence::FromCode,
    }
}

pub const FLAGS: &[Flag] = &[
    // Maktar Nebula (level 2).
    Flag {
        id: "maktar-arena-entered",
        level: Some(2),
        name: "Arena entered",
        description: "Off, the next visit opens the arena's gate and plays its first-visit scene again.",
        place: story(5, 1),
        confidence: Confidence::Proven,
    },
    Flag {
        id: "maktar-arena-intro-seen",
        level: Some(2),
        name: "Arena intro scene seen",
        description: "The scene the first visit to the arena plays (cinematic 43).",
        place: Place::Bit { chunk: 43, level: None, offset: 0x29, bit: 3 },
        confidence: Confidence::Proven,
    },
    Flag {
        id: "maktar-arena-first-challenge-won",
        level: Some(2),
        name: "Arena: Battle for the Electrolyzer",
        description: "The fight that wins the Electrolyzer. On, the arena counts it as won; it does \
                      not give the Electrolyzer.",
        place: story(0x31, 1),
        confidence: Confidence::Proven,
    },
    arena("maktar-arena-2", "Arena: Hazard Challenge: Watch Your Step!", 0x34),
    arena("maktar-arena-3", "Arena: Chainblade Challenge", 0x32),
    arena("maktar-arena-4", "Arena: Time Challenge: Kill 30 Enemies in 30 Seconds", 0x33),
    arena("maktar-arena-5", "Arena: Battle the B2 Brawler", 0x35),
    arena("maktar-arena-6", "Arena: Time Challenge: Kill 60 Enemies in 60 Seconds", 0x36),
    arena("maktar-arena-7", "Arena: Mega Challenge!", 0x37),
    arena("maktar-arena-8", "Arena: Limited Weapon: Use Only 300 Heavy Lancer Shots", 0x38),
    arena("maktar-arena-9", "Arena: Limited Weapon: Kill All Enemies with 8 Mini-Nuke Shots", 0x91),
    arena("maktar-arena-10", "Arena: Tag Team Battle: Chainblade and B2 Brawler", 0x39),
    arena("maktar-arena-11", "Arena: Endurance Challenge!", 0x3A),
    arena("maktar-arena-12", "Arena: Ultimate Challenge!", 0x3B),
    arena("maktar-arena-13", "Arena: Timed Avoidance: Dodge Chainblade for 2 Minutes", 0x92),
    arena("maktar-arena-14", "Arena: Wrench Expert: Wrench Only and Don't Take a Hit", 0x93),
    // Maktar's two Electrolyzer puzzles (boards 593 and 40, nearer and farther from the landing).
    electrolyzer(
        "maktar-electrolyzer-1",
        2,
        "Electrolyzer puzzle 1 solved",
        "The board nearer the landing (moby 593): solved, its two platforms stay moved into place. Off, \
         the board can be played again.",
        &[count(2, 593), group_done(2, 6), story(3, 1)],
        Confidence::Proven,
    ),
    electrolyzer(
        "maktar-electrolyzer-2",
        2,
        "Electrolyzer puzzle 2 solved",
        "The board farther on, toward the arena (moby 40): solved, its two platforms stay moved into \
         place. Off, the board can be played again.",
        &[count(2, 40), group_done(2, 9)],
        Confidence::Proven,
    ),
    // Endako (level 3).
    Flag {
        id: "endako-swingshot-start-passed",
        level: Some(3),
        name: "Swingshot area passed",
        description: "Off while the Swingshot is owned, entering Endako puts Ratchet at a set point until \
                      he reaches the area past it.",
        place: story(0x0D, 1),
        confidence: Confidence::Proven,
    },
    // Endako's four Electrolyzer puzzles, nearest to farthest from the landing.
    electrolyzer(
        "endako-electrolyzer-1",
        3,
        "Electrolyzer puzzle 1 solved",
        "The board beside the factory door (moby 217): solved, the door is open. Off, the door is \
         shut and the board can be played again.",
        &[count(3, 217)],
        Confidence::Proven,
    ),
    electrolyzer(
        "endako-electrolyzer-2",
        3,
        "Electrolyzer puzzle 2 solved",
        "The board that powers the first control panel (moby 437). Off, the panel is dead and the \
         board can be played again.",
        &[count(3, 437)],
        Confidence::Proven,
    ),
    electrolyzer(
        "endako-electrolyzer-3",
        3,
        "Electrolyzer puzzle 3 solved",
        "The board that powers the second control panel (moby 466). Off, the panel is dead and the \
         board can be played again.",
        &[count(3, 466)],
        Confidence::Proven,
    ),
    electrolyzer(
        "endako-electrolyzer-4",
        3,
        "Electrolyzer puzzle 4 solved",
        "The board farthest from the landing (moby 20), with what it sets off: scene 50 and, after it, \
         story byte 7. Off, the board can be played again and the scene plays when it is solved.",
        &[count(3, 20), Place::Bit { chunk: 43, level: None, offset: 0x32, bit: 2 }, story(7, 1)],
        Confidence::Proven,
    ),
    // Barlow (level 4).
    Flag {
        id: "barlow-thermanator-start-passed",
        level: Some(4),
        name: "Thermanator area passed",
        description: "Off while the Thermanator is owned, entering Barlow puts Ratchet at a set point until \
                      he reaches the area past it.",
        place: story(0x12, 1),
        confidence: Confidence::FromCode,
    },
    electrolyzer(
        "barlow-electrolyzer",
        4,
        "Electrolyzer puzzle solved",
        "Barlow's board (moby 561), with what it sets off: a switch that saves story byte 0x0F, and a \
         watcher that plays scene 56 and marks object group 7 done. Off, the board can be played again.",
        &[count(4, 561), story(0x0F, 1), group_done(4, 7), Place::Bit { chunk: 43, level: None, offset: 0x3B, bit: 0 }],
        Confidence::Proven,
    ),
    // Feltzin System (level 5): the four space combat missions, a count of wins each (the game
    // keeps up to 127 and lowers the bolt prize after the second win).
    Flag {
        id: "feltzin-mission-1-won",
        level: Some(5),
        name: "Space mission 1 won",
        description: "The first space combat mission, the one the story needs.",
        place: story(0x61, 1),
        confidence: Confidence::Proven,
    },
    Flag {
        id: "feltzin-mission-2-won",
        level: Some(5),
        name: "Space mission 2 won",
        description: "The second space combat mission (an optional challenge for bolts).",
        place: story(0x62, 1),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "feltzin-mission-3-won",
        level: Some(5),
        name: "Space mission 3 won",
        description: "The third space combat mission (an optional challenge for bolts).",
        place: story(0x63, 1),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "feltzin-mission-4-won",
        level: Some(5),
        name: "Space mission 4 won",
        description: "The fourth space combat mission (an optional challenge for bolts).",
        place: story(0x64, 1),
        confidence: Confidence::FromCode,
    },
];
