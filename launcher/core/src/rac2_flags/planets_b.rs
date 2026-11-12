// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Progress flags of levels 9 to 16 (Dobbo to Snivelak).
//!
//! Every place here is a byte of the story chunk (5) or a bit of the cinematics chunk (43) that
//! the level's own code reads; the functions are named in `launcher/docs/RAC2_SAVE_FORMAT.md`
//! ("Levels 9 to 16").
//!
//! The Electrolyzer puzzles ([`super::electrolyzer`]) are also the board's moby count (chunk 3005)
//! and object group (chunk 3004) in the level's section; they are in `RAC2_SAVE_FORMAT.md`,
//! "Advanced: Electrolyzer puzzles (L-7)".

use super::{count, electrolyzer, group_done, Confidence, Flag, Place};

/// A byte of the story chunk (0x001395B8), set to `on`.
const fn story(offset: usize, on: u8) -> Place {
    Place::Byte { chunk: 5, level: None, offset, on }
}

/// A bit of the cinematics chunk (0x00139768), by its number.
const fn cinematic(n: usize) -> Place {
    Place::Bit { chunk: 43, level: None, offset: n / 8, bit: (n % 8) as u8 }
}

/// One of the four ship challenges of a space level: the byte counts how often it was won.
const fn ship(id: &'static str, level: usize, name: &'static str, offset: usize, confidence: Confidence) -> Flag {
    Flag {
        id,
        level: Some(level),
        name,
        description: "Won at least once: the next challenge opens and this one pays its repeat reward.",
        place: story(offset, 1),
        confidence,
    }
}

/// One of Joba's arena challenges: the byte counts the wins (`func_L11_0043E108` gives the byte).
const fn arena(id: &'static str, name: &'static str, offset: usize, confidence: Confidence) -> Flag {
    Flag {
        id,
        level: Some(11),
        name,
        description: "Won at least once: the arena's menu marks it done and later wins pay the repeat prize.",
        place: story(offset, 1),
        confidence,
    }
}

/// One of the hoverbike races: the byte counts the wins (`func_L04_0040A1D0` gives it,
/// `func_L04_0040A220` adds 1 per win, the same code in Barlow and Joba). A race can be picked
/// once the race before it is won; the prize is by this count.
const fn race(id: &'static str, name: &'static str, offset: usize, description: &'static str) -> Flag {
    Flag { id, level: Some(11), name, description, place: story(offset, 1), confidence: Confidence::FromCode }
}

pub const FLAGS: &[Flag] = &[
    // Dobbo (9)
    Flag {
        id: "dobbo-joba-task-given",
        level: Some(9),
        name: "Joba coordinates: task given",
        description: "The character who trades Joba's coordinates has given his task, so he skips his first scene.",
        place: story(0x1D, 1),
        confidence: Confidence::Proven,
    },
    Flag {
        id: "dobbo-joba-task-done",
        level: Some(9),
        name: "Joba coordinates: task done",
        description: "Talking to him next plays the hand-over scene and puts Joba on the ship's map.",
        place: story(0x2E, 1),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "dobbo-hrugis-offer-seen",
        level: Some(9),
        name: "Hrugis Cloud coordinates offered",
        description: "The seller of Hrugis Cloud's coordinates skips his scenes and goes straight to the sale.",
        place: cinematic(768),
        confidence: Confidence::Proven,
    },
    electrolyzer(
        "dobbo-electrolyzer",
        9,
        "Electrolyzer puzzle solved",
        "The board that is the Hrugis Cloud coordinates seller's task (moby 182), with the seller's \
         own note that it is done (story byte 0x1F). Off, the board can be played again and the \
         seller waits for it.",
        &[count(9, 182), group_done(9, 5), story(0x1F, 1)],
        Confidence::Proven,
    ),
    // Hrugis Cloud (10)
    ship("hrugis-challenge-1", 10, "Ship challenge 1 won", 0x66, Confidence::Proven),
    ship("hrugis-challenge-2", 10, "Ship challenge 2 won", 0x67, Confidence::Proven),
    ship("hrugis-challenge-3", 10, "Ship challenge 3 won", 0x68, Confidence::Proven),
    ship("hrugis-challenge-4", 10, "Ship challenge 4 won", 0x69, Confidence::FromCode),
    // Joba (11)
    Flag {
        id: "joba-arena-entered",
        level: Some(11),
        name: "Arena entered",
        description: "Off, the next visit opens the arena's gate and plays its first-visit scene again.",
        place: story(0x4A, 1),
        confidence: Confidence::Proven,
    },
    arena("joba-arena-1", "Arena: Battle for the Gravity Boots", 61, Confidence::Proven),
    arena("joba-arena-2", "Arena: Cage Match for the Infiltrator", 62, Confidence::FromCode),
    arena("joba-arena-3", "Arena: Arachnoid Battle", 63, Confidence::FromCode),
    arena("joba-arena-4", "Arena: Damage Avoidance: Don't Take A Hit!", 64, Confidence::FromCode),
    arena("joba-arena-5", "Arena: Hazard Cage Match: Look Out Below!", 65, Confidence::FromCode),
    arena("joba-arena-6", "Arena: Time Challenge: Kill 90 Enemies in 90 Seconds", 66, Confidence::FromCode),
    arena("joba-arena-7", "Arena: Megapede Battle", 67, Confidence::FromCode),
    arena("joba-arena-8", "Arena: Limited Weapon: Use Only The Wrench", 68, Confidence::FromCode),
    arena("joba-arena-9", "Arena: Timed Limited Weapon: Lancer All Enemies in 60 Seconds", 71, Confidence::FromCode),
    arena("joba-arena-10", "Arena: Time Challenge: Kill Megapede in 3 Minutes", 69, Confidence::FromCode),
    arena("joba-arena-11", "Arena: The Impossible Challenge!", 70, Confidence::FromCode),
    arena("joba-arena-12", "Arena: Damage Avoidance: Don't Get Hit by Megapede!", 72, Confidence::FromCode),
    arena("joba-arena-13", "Arena: Timed Cage Match: Kill 100 Enemies in 100 Seconds", 75, Confidence::FromCode),
    // Todano (12)
    // Joba's five hoverbike races (wins, chunk 5 0x57..0x5B).
    race(
        "joba-race-1",
        "Hoverbike race 1 won",
        0x57,
        "Won at least once: race 2 can be picked, and the next visit gives the Charge Boots if they are not owned.",
    ),
    race(
        "joba-race-2",
        "Hoverbike race 2 won",
        0x58,
        "Won at least once: the next race can be picked and later wins pay the smaller prize.",
    ),
    race(
        "joba-race-3",
        "Hoverbike race 3 won",
        0x59,
        "Won at least once: the next race can be picked and later wins pay the smaller prize.",
    ),
    race(
        "joba-race-4",
        "Hoverbike race 4 won",
        0x5A,
        "Won at least once: the next race can be picked and later wins pay the smaller prize.",
    ),
    race("joba-race-5", "Hoverbike race 5 won", 0x5B, "Won at least once: later wins pay the smaller prize."),
    electrolyzer(
        "todano-electrolyzer",
        12,
        "Electrolyzer puzzle solved",
        "The board that powers the dam's lift (moby 665). Off, the lift does not run and the board can \
         be played again.",
        &[count(12, 665), group_done(12, 1)],
        Confidence::Proven,
    ),
    // Gorn (15)
    // Aranos Prison (level 14): the prison controller, class 4351 (`func_L14_004265B8`, one in the
    // level, moby 691), picks its start from four object groups of its pvars (read from the
    // disc's gameplay file): 9 guard beaten (with story byte 0x29), 8 panel done, 7 exit reached,
    // 3 switch thrown. With 3 done it gives the Heli-Pack and Thruster-Pack back and the hero is
    // as usual; without it the level takes both packs away (Clank off Ratchet's back) and resumes
    // at the step after the last group done.
    Flag {
        id: "aranos-prison-clank-section-done",
        level: Some(14),
        name: "Prison section done: Clank back with Ratchet",
        description: "On, the level starts past the prison escape: object groups 9, 8, 7 and 3 done, story byte \
                      0x29 set, and the Heli-Pack and Thruster-Pack owned (as the game gives them back). Off, \
                      the escape is played again from the start (the packs are left as they are; the level \
                      itself takes them while the escape is not done).",
        place: Place::All(&[
            group_done(14, 3),
            group_done(14, 7),
            group_done(14, 8),
            group_done(14, 9),
            story(0x29, 1),
            Place::SetOnly { chunk: 10, offset: 2, on: 1 },
            Place::SetOnly { chunk: 11, offset: 2, on: 1 },
            Place::SetOnly { chunk: 10, offset: 3, on: 1 },
            Place::SetOnly { chunk: 11, offset: 3, on: 1 },
        ]),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "aranos-prison-pad-reached",
        level: Some(14),
        name: "Prison pad reached",
        description: "The pad class 4411 (moby 810) saves when the hero reaches it: story byte 0x2B and object \
                      group 10. With 0x2B set, arriving on Aranos Prison skips its capture arrival scene; with \
                      group 10 and the prison section done, the door (moby 812) starts open.",
        place: Place::All(&[story(0x2B, 1), group_done(14, 10)]),
        confidence: Confidence::FromCode,
    },
    ship("gorn-challenge-1", 15, "Ship challenge 1 won", 0x6B, Confidence::Proven),
    ship("gorn-challenge-2", 15, "Ship challenge 2 won", 0x6C, Confidence::Proven),
    ship("gorn-challenge-3", 15, "Ship challenge 3 won", 0x6D, Confidence::FromCode),
    ship("gorn-challenge-4", 15, "Ship challenge 4 won", 0x6E, Confidence::FromCode),
    // Snivelak (16)
    Flag {
        id: "snivelak-boss-beaten",
        level: Some(16),
        name: "Boss beaten",
        description: "The level's boss is gone on the next visit; off, he is back and the fight can be played again.",
        place: story(0x83, 2),
        confidence: Confidence::Proven,
    },
];
