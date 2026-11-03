// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Going Commando's story and progress flags for the save editor's Advanced view, one file per
//! group of planets so they can be filled in separately: `planets_a` levels 0 to 8, `planets_b`
//! 9 to 16, `planets_c` 17 to 30 and the flags of no one planet. [`all`] gives them in planet
//! order. How each was found is in `launcher/docs/RAC2_SAVE_FORMAT.md`.

mod planets_a;
mod planets_b;
mod planets_c;

/// Where a flag is in the save. `chunk` is a chunk id of the save (global, or per level when
/// `level` is set: that level's section).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Place {
    /// A byte: set when non-zero; setting writes `on`, clearing writes 0.
    Byte { chunk: i32, level: Option<usize>, offset: usize, on: u8 },
    /// One bit of a byte.
    Bit { chunk: i32, level: Option<usize>, offset: usize, bit: u8 },
}

/// How sure the place and meaning are.
#[derive(Clone, Copy, Debug, PartialEq, Eq, serde::Serialize, serde::Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "Rac2FlagConfidence")]
pub enum Confidence {
    /// Seen set by the game and, edited in a save, honoured by the game at load.
    Proven,
    /// Read from the decompiled code, with a sample of its group proven; not tried one by one.
    FromCode,
}

/// A flag the Advanced view offers.
#[derive(Clone, Copy, Debug)]
pub struct Flag {
    /// Unique and stable: the page and the saved fields name the flag by it.
    pub id: &'static str,
    /// The level whose progress it is (RAC2_PLANETS numbering); `None` for no one planet.
    pub level: Option<usize>,
    /// What the player sees it as, in a few words.
    pub name: &'static str,
    /// One plain sentence: what it changes in the game.
    pub description: &'static str,
    pub place: Place,
    pub confidence: Confidence,
}

/// Every flag, in planet order (`planets_a`, `planets_b`, `planets_c`).
pub fn all() -> impl Iterator<Item = &'static Flag> {
    planets_a::FLAGS.iter().chain(planets_b::FLAGS).chain(planets_c::FLAGS)
}
