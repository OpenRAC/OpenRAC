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
    /// A moby's count in its level's chunk 3005 (`D_0019B4A8 + level * 0x400`): three bits, the low
    /// nibble of byte `uid >> 1` for an even `uid`, the high one for an odd one, which the game
    /// raises (up to 7) when the moby is done: an enemy killed, a crate broken, a puzzle solved.
    /// Set when non-zero; setting writes 1 to a count of 0, clearing writes 0 (bit 3 of the nibble,
    /// not part of the count, is left as it is).
    Count { level: usize, uid: u16 },
    /// Several places written together, as the game writes them at one event; the first says
    /// whether the flag is on.
    All(&'static [Place]),
}

/// The chunk of a level's moby counts ([`Place::Count`]).
pub const MOBY_COUNTS: i32 = 3005;
/// The chunk of a level's object-group states: bit 7 of byte `group` is the group done
/// (`func_002A0430`).
pub const GROUP_STATES: i32 = 3004;

/// A level's moby count ([`Place::Count`]).
pub(crate) const fn count(level: usize, uid: u16) -> Place {
    Place::Count { level, uid }
}

/// A level's object group done (bit 7 of byte `group` of chunk 3004).
pub(crate) const fn group_done(level: usize, group: usize) -> Place {
    Place::Bit { chunk: GROUP_STATES, level: Some(level), offset: group, bit: 7 }
}

/// One of the Electrolyzer puzzle boards (class 2198, `func_L02_003C26F0`, the same code in levels
/// 2, 3, 4, 9, 12, 17, 20 and 30). Solved (its state 8), a board raises its own moby count and,
/// when it belongs to an object group, marks the group done; at the level start it shows itself
/// solved, and sets off again what it is linked to, while that count is non-zero. `places` are
/// those two, then what the linked moby saves when it is set off; clearing them all puts the
/// board, and what it opens, back as they were before it was solved.
pub(crate) const fn electrolyzer(
    id: &'static str,
    level: usize,
    name: &'static str,
    description: &'static str,
    places: &'static [Place],
    confidence: Confidence,
) -> Flag {
    Flag { id, level: Some(level), name, description, place: Place::All(places), confidence }
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
