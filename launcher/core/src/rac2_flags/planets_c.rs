// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Progress flags of levels 17 to 30 (Smolg to the Insomniac Museum), and the flags of no one planet.
//!
//! Story bytes are chunk 5 (0x1395B8), indexed by constants in each level's code. A cinematic's bit
//! in chunk 43 (0x139768) is where the game's own setter puts it: bit `id & 31` of the word
//! `id >> 2` (`func_002EB7E8`, `func_00288B38`), so scene `id` is byte `(id >> 2) * 4 + (id & 31) / 8`,
//! bit `id & 7`. Scene ids are numbered by level (the first id of each level's scenes is the table at
//! 0x1AD030: Smolg 137, Damosel 143, Grelbin 150, Yeedil 155). Each flag names the function that
//! reads it in `launcher/docs/RAC2_SAVE_FORMAT.md`, "Advanced: progress flags, Smolg to the Museum".
//!
//! The Electrolyzer puzzles ([`super::electrolyzer`]) are also the board's moby count (chunk 3005)
//! and object group (chunk 3004) in the level's section; they are in `RAC2_SAVE_FORMAT.md`,
//! "Advanced: Electrolyzer puzzles (L-7)".

use super::{count, electrolyzer, scene, Confidence, Flag, Place};

/// A story byte (chunk 5): set is 1, as the game writes it.
const fn story(offset: usize) -> Place {
    Place::Byte { chunk: 5, level: None, offset, on: 1 }
}

pub const FLAGS: &[Flag] = &[
    // Smolg (17)
    Flag {
        id: "smolg-hypnomatic-part",
        level: Some(17),
        name: "Hypnomatic part found",
        description: "The Smolg part of the three the Hypnomatic maker on Damosel asks for; the inventory and the maker count it.",
        place: story(0x84),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "smolg-coordinates-trader-met",
        level: Some(17),
        name: "Damosel coordinates seller met",
        description: "Off, the trader who sells Damosel's coordinates plays his introduction again before he sells them.",
        place: scene(137),
        confidence: Confidence::FromCode,
    },
    electrolyzer(
        "smolg-electrolyzer",
        17,
        "Electrolyzer puzzle solved",
        "The board that powers the lift beside it (moby 32). Off, the lift waits and the board can be \
         played again.",
        &[count(17, 32)],
        Confidence::Proven,
    ),
    // Damosel (18)
    Flag {
        id: "damosel-hypnomatic-part",
        level: Some(18),
        name: "Hypnomatic part found",
        description: "The Damosel part of the three; off, its pickup sparkles and can be taken again.",
        place: story(0x86),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "damosel-hypnomatic-maker-met",
        level: Some(18),
        name: "Hypnomatic maker met",
        description: "Off, the Hypnomatic maker plays his introduction again before he asks for the parts.",
        place: scene(144),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "damosel-giant-clank-return",
        level: Some(18),
        name: "Coming back from Giant Clank",
        description: "On, the next arrival on Damosel starts at Giant Clank's launch pad and plays the return scene; the game clears it then.",
        place: story(0x9F),
        confidence: Confidence::FromCode,
    },
    // Grelbin (19)
    Flag {
        id: "grelbin-hypnomatic-part",
        level: Some(19),
        name: "Hypnomatic part traded",
        description: "The Grelbin part of the three; on, the moonstone trader only buys moonstones for bolts.",
        place: story(0x89),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "grelbin-moonstone-trader-met",
        level: Some(19),
        name: "Moonstone trader met",
        description: "Off (with no trade made), the moonstone trader waits for his introduction scene before he trades.",
        place: scene(152),
        confidence: Confidence::FromCode,
    },
    Flag {
        id: "grelbin-first-moonstone-trade",
        level: Some(19),
        name: "First moonstone trade made",
        description: "The scene of the first trade; on, the trader offers the Hypnomatic part for more moonstones.",
        place: scene(154),
        confidence: Confidence::FromCode,
    },
    // Yeedil (20)
    electrolyzer(
        "yeedil-electrolyzer",
        20,
        "Electrolyzer puzzle solved",
        "The board that powers the lift beside it (moby 26). Off, the lift waits and the board can be \
         played again.",
        &[count(20, 26)],
        Confidence::Proven,
    ),
    // Damosel Orbit (23)
    Flag {
        id: "damosel-orbit-battle-won",
        level: Some(23),
        name: "Giant Clank battle won",
        description: "With Coming back from Giant Clank on, the return to Damosel gives the Mapper if it is not owned yet.",
        place: story(0xA1),
        confidence: Confidence::FromCode,
    },
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scene_bits_are_where_the_games_setter_puts_them() {
        // Maktar's arena intro, scene 43, proven at byte 0x29 bit 3 (planets_a).
        assert_eq!(scene(43), Place::Bit { chunk: 43, level: None, offset: 0x29, bit: 3 });
        assert_eq!(scene(137), Place::Bit { chunk: 43, level: None, offset: 0x89, bit: 1 });
        assert_eq!(scene(144), Place::Bit { chunk: 43, level: None, offset: 0x92, bit: 0 });
        assert_eq!(scene(152), Place::Bit { chunk: 43, level: None, offset: 0x9B, bit: 0 });
        assert_eq!(scene(154), Place::Bit { chunk: 43, level: None, offset: 0x9B, bit: 2 });
    }
}
