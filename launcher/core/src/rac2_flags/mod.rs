// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Going Commando's story and progress flags for the save editor's Advanced view, one file per
//! group of planets so they can be filled in separately: `planets_a` levels 0 to 8, `planets_b`
//! 9 to 16, `planets_c` 17 to 30 and the flags of no one planet. [`all`] gives them in planet
//! order, then each planet's cut scenes (`scenes`). How each was found is in `launcher/docs/RAC2_SAVE_FORMAT.md`.

mod planets_a;
mod planets_b;
mod planets_c;
mod scenes;

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
    /// A global byte written to `on` when the flag is set and left as it is when it is cleared:
    /// what the game gives at the event (an item owned) but does not take back elsewhere.
    SetOnly { chunk: i32, offset: usize, on: u8 },
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

/// Scene `id`'s "seen" bit in the cinematics chunk (43): the scene players set bit `id & 31` of
/// the word `id >> 2` (ids under 0xB1), so byte `(id >> 2) * 4 + (id & 31) / 8`, bit `id & 7`.
pub(crate) const fn scene(id: usize) -> Place {
    Place::Bit { chunk: 43, level: None, offset: (id >> 2) * 4 + (id & 31) / 8, bit: (id & 7) as u8 }
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

/// Every flag, in planet order: each planet's from its planet file, then its cut scenes.
pub fn all() -> impl Iterator<Item = &'static Flag> {
    let mut v: Vec<&'static Flag> =
        planets_a::FLAGS.iter().chain(planets_b::FLAGS).chain(planets_c::FLAGS).chain(scenes::FLAGS).collect();
    // Stable: each planet's own flags, then its scenes; the flags of no one planet last.
    v.sort_by_key(|f| f.level.unwrap_or(usize::MAX));
    v.into_iter()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashSet;

    #[test]
    fn a_scene_is_offered_once() {
        // Every chunk 43 bit is one flag's, whether a named one or a numbered scene.
        let mut bits = HashSet::new();
        let mut add = |p: Place| {
            if let Place::Bit { chunk: 43, offset, bit, .. } = p {
                assert!(bits.insert((offset, bit)), "scene bit {offset:#x}.{bit} twice");
            }
        };
        for f in all() {
            match f.place {
                Place::All(ps) => ps.iter().copied().for_each(&mut add),
                p => add(p),
            }
        }
    }

    #[test]
    fn scenes_fall_in_their_planets_range() {
        for f in scenes::FLAGS {
            let l = f.level.unwrap();
            let Place::Bit { offset, bit, .. } = f.place else { panic!() };
            let ok = (scenes::FIRST_SCENE[l]..scenes::FIRST_SCENE[l + 1]).any(|id| scene(id) == f.place);
            assert!(ok, "{} at {offset:#x}.{bit}", f.id);
        }
    }

    #[test]
    fn races_are_the_win_counts() {
        let byte = |id: &str| match all().find(|f| f.id == id).unwrap().place {
            Place::Byte { chunk: 5, offset, .. } => offset,
            p => panic!("{p:?}"),
        };
        assert_eq!(byte("barlow-race-1"), 0x4D);
        assert_eq!(byte("barlow-race-5"), 0x51);
        assert_eq!(byte("joba-race-1"), 0x57);
        assert_eq!(byte("joba-race-5"), 0x5B);
    }
}
