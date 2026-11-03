// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Progress flags of levels 0 to 8 (Aranos Tutorial to Tabora).

use super::{Confidence, Flag, Place};

pub const FLAGS: &[Flag] = &[
    Flag {
        id: "maktar-arena-entered",
        level: Some(2),
        name: "Arena entered",
        description: "Off, the next visit opens the arena's gate and plays its first-visit scene again.",
        place: Place::Byte { chunk: 5, level: None, offset: 5, on: 1 },
        confidence: Confidence::Proven,
    },
    Flag {
        id: "maktar-arena-intro-seen",
        level: Some(2),
        name: "Arena intro scene seen",
        description: "The scene the first visit to the arena plays (cinematic 331).",
        place: Place::Bit { chunk: 43, level: None, offset: 0x29, bit: 3 },
        confidence: Confidence::Proven,
    },
];
