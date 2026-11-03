// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Progress flags of levels 0 to 8 (Aranos Tutorial to Tabora).
//!
//! Each place is a byte of the story chunk (5, at 0x001395B8) or a bit of the cinematics chunk
//! (43), named from the level code that reads and writes it. The notes per flag, and the flags of
//! these planets left out, are in `RAC2_SAVE_FORMAT.md`, "Planets 0 to 8 (L-6a)".

use super::{Confidence, Flag, Place};

/// A byte of the story chunk, set to `on`.
const fn story(offset: usize, on: u8) -> Place {
    Place::Byte { chunk: 5, level: None, offset, on }
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
        name: "Arena: first challenge won",
        description: "The fight that wins the Electrolyzer. On, the arena counts it as won; it does \
                      not give the Electrolyzer.",
        place: story(0x31, 1),
        confidence: Confidence::Proven,
    },
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
