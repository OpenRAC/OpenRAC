// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! The save editor for Ratchet & Clank 3 / Up Your Arsenal (PAL `SCES_524.56`, NTSC-U `SCUS_973.53`),
//! a first view:
//! where the save resumes, when it was saved, the play time, the bolts and the weapons (owned,
//! version, ammo, experience). It changes the bolts and the ammo of the weapons the save owns, and
//! writes the save back so that the game still loads it, with rac1's safety steps (game running,
//! file changed since read, backup, read-back, rename: [`crate::save_edit`]). The format is in
//! `launcher/docs/RAC3_SAVE_FORMAT.md`.
//!
//! # The format, in short
//!
//! `saveN.bin` (N = 0..3) is the game's SavePack (`func_0039E9C0`), Going Commando's container
//! ([`crate::save_edit_rac2`]) with its own tables:
//!
//! ```text
//! +0  u32 global section size (14216)    +4  u32 level section size (3264)
//! +8  the global section, then 37 level sections (one per level number 0..36)
//! section: u32 data size, u32 crc16(data), data = chunks, then {s32 -1, u32 0}
//! chunk:   s32 id, u32 size, size bytes, padding to 4
//! ```
//!
//! The chunks are the PAL executable's save tables (global 0x00332388, per level 0x00332618);
//! each entry is `{address, size, id}` and a chunk is a copy of `size` bytes at `address` (a
//! level's at `address + level * size`). The card code reads each section back with
//! `func_0039ED78`, which checks the checksum (`func_0039EB68`: non-zero and equal) and counts a
//! chunk of an unknown id or another size as an error. The port's hidden runs showed the game's
//! own Load page take a save this editor's checksum wrote (Veldin, 12,345 bolts, 1:02 played).
//!
//! # What the weapon fields are
//!
//! - Owned: chunk 10 (`0x00142CA0`), a byte per item, set by the game's item_give.
//! - Version: chunk 30 (`0x001425C0`), a byte per item: its row in the weapon table, which is the
//!   item's own number at V1 (a new game's chunk holds 0, 1, 2 ... 159) and later rows as it
//!   levels up (`+0x46` of a row is the next one). The rows' names and full ammo below are read
//!   from the PAL disc's weapon table (Veldin's program, 0xF0 bytes a row: `+0x04` the name's
//!   text, `+0x8A` the full ammo) and its English text.
//! - Ammo: chunk 9 (`0x001427F0`), s32 by item, capped at the row's full ammo by the game.
//! - Experience: chunk 40 (`0x00142DE0`), s32 by item.
//!
//! Only the bolts and ammo are written; owning, versions and experience are shown, not changed,
//! until a save the game wrote in a level shows what else changes with them.

use crate::saves::{self, SaveBackupInfo};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::{Path, PathBuf};

// ---------------------------------------------------------------------------------------------
// The checksum and the container.

/// The global section's size; `func_0039EAC0` returns 0, a checksum that always fails, for data
/// longer than it.
pub const GLOBAL_SECTION: usize = 14216;
/// Each level section's size.
pub const LEVEL_SECTION: usize = 3264;
/// Level sections in a save (`func_0039E9C0` loops 0x25 times).
pub const LEVEL_SLOTS: usize = 37;
/// The whole file.
pub const FILE_SIZE: usize = 8 + GLOBAL_SECTION + LEVEL_SLOTS * LEVEL_SECTION;

/// `func_0039EAC0`: rac1's and Going Commando's CRC-16 (polynomial 0x1F45, register seeded with
/// 0xEDB88320, most significant bit first), with this game's length limit.
pub fn crc16(data: &[u8]) -> u16 {
    if data.len() > GLOBAL_SECTION {
        return 0;
    }
    let mut crc: u32 = 0xEDB8_8320;
    for &b in data {
        crc ^= (b as u32) << 8;
        for _ in 0..8 {
            crc = if crc & 0x8000 != 0 { (crc << 1) ^ 0x1F45 } else { crc << 1 };
        }
    }
    (crc & 0xFFFF) as u16
}

fn align4(n: usize) -> usize {
    (n + 3) & !3
}

fn u32_at(b: &[u8], at: usize) -> Option<u32> {
    b.get(at..at + 4).map(|w| u32::from_le_bytes(w.try_into().unwrap()))
}

/// One `{id, size, bytes}` chunk; `pad` is the bytes up to the next multiple of 4, kept as read.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Chunk {
    pub id: i32,
    pub data: Vec<u8>,
    pub pad: Vec<u8>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Section {
    pub chunks: Vec<Chunk>,
}

impl Section {
    /// Reads a section at the start of `bytes`; fails on a bad checksum, as the game's load does.
    pub fn decode(bytes: &[u8]) -> Result<Section, String> {
        let size = u32_at(bytes, 0).ok_or("a section is cut short")? as usize;
        let stored = u32_at(bytes, 4).ok_or("a section is cut short")?;
        let data = bytes.get(8..8 + size).ok_or("a section is longer than the file")?;
        if stored == 0 || crc16(data) as u32 != stored {
            return Err("a section's checksum does not match: the game would not load this save".into());
        }
        let mut chunks = Vec::new();
        let mut at = 0;
        loop {
            let id = u32_at(data, at).ok_or("a section has no end marker")? as i32;
            let len = u32_at(data, at + 4).ok_or("a section has no end marker")? as usize;
            if id == -1 {
                break;
            }
            let body = data.get(at + 8..at + 8 + align4(len)).ok_or("a chunk runs past its section")?;
            chunks.push(Chunk { id, data: body[..len].to_vec(), pad: body[len..].to_vec() });
            at += 8 + align4(len);
        }
        Ok(Section { chunks })
    }

    /// `func_0039EBA8`: `{size, crc16}`, the chunks, `{-1, 0}`. A checksum of 0 would make the game
    /// refuse the section, so then one padding byte (never read) is changed.
    pub fn encode(&self) -> Vec<u8> {
        let mut out = self.encode_raw();
        if u32_at(&out, 4) == Some(0) {
            let mut fixed = self.clone();
            if let Some(at) = fixed.chunks.iter().position(|c| !c.pad.is_empty()) {
                for v in 1..=255u8 {
                    fixed.chunks[at].pad[0] = v;
                    out = fixed.encode_raw();
                    if u32_at(&out, 4) != Some(0) {
                        break;
                    }
                }
            }
        }
        out
    }

    fn encode_raw(&self) -> Vec<u8> {
        let mut v = vec![0u8; 8];
        for c in &self.chunks {
            v.extend_from_slice(&c.id.to_le_bytes());
            v.extend_from_slice(&(c.data.len() as u32).to_le_bytes());
            v.extend_from_slice(&c.data);
            let n = align4(c.data.len()) - c.data.len();
            v.extend((0..n).map(|k| c.pad.get(k).copied().unwrap_or(0)));
        }
        v.extend_from_slice(&(-1i32).to_le_bytes());
        v.extend_from_slice(&0u32.to_le_bytes());
        let size = (v.len() - 8) as u32;
        let crc = crc16(&v[8..]) as u32;
        v[0..4].copy_from_slice(&size.to_le_bytes());
        v[4..8].copy_from_slice(&crc.to_le_bytes());
        v
    }

    fn get(&self, id: i32) -> &[u8] {
        &self.chunks.iter().find(|c| c.id == id).expect("the layout check guarantees every chunk").data
    }

    fn get_mut(&mut self, id: i32) -> &mut Vec<u8> {
        &mut self.chunks.iter_mut().find(|c| c.id == id).expect("the layout check guarantees every chunk").data
    }

    fn layout(&self) -> Vec<(i32, usize)> {
        self.chunks.iter().map(|c| (c.id, c.data.len())).collect()
    }
}

/// The global table (0x00332388), `(id, size)` in file order, as the PAL executable lists it.
pub const GLOBAL_LAYOUT: &[(i32, usize)] = &[
    (0, 4),
    (39, 128),
    (3, 4),
    (4, 8),
    (1003, 4),
    (5, 272),
    (7, 19),
    (8, 32),
    (9, 624),
    (10, 156),
    (11, 156),
    (40, 624),
    (12, 64),
    (13, 32),
    (14, 60),
    (20, 240),
    (15, 1824),
    (16, 4272),
    (17, 516),
    (18, 1872),
    (38, 188),
    (30, 160),
    (32, 28),
    (37, 19),
    (41, 268),
    (42, 156),
    (45, 156),
    (43, 256),
    (44, 4),
    (46, 40),
    (47, 256),
    (1005, 4),
    (1010, 800),
    (1011, 4),
    (7002, 32),
    (7003, 12),
    (7004, 6),
    (7005, 192),
    (7006, 192),
    (7007, 192),
];

/// The per-level table (0x00332618).
pub const LEVEL_LAYOUT: &[(i32, usize)] =
    &[(3001, 1), (3002, 2048), (3003, 8), (3004, 16), (3005, 1024), (4002, 4), (4005, 64), (4007, 4), (4008, 4)];

fn section_size(layout: &[(i32, usize)]) -> usize {
    8 + layout.iter().map(|&(_, n)| 8 + align4(n)).sum::<usize>() + 8
}

/// A whole `saveN.bin`.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SaveFile {
    pub global: Section,
    pub levels: Vec<Section>,
}

impl SaveFile {
    /// Reads a save, refusing anything the game would not load or the editor does not know.
    pub fn parse(bytes: &[u8]) -> Result<SaveFile, String> {
        let gs = u32_at(bytes, 0).ok_or("the file is too short to be a save")? as usize;
        let ls = u32_at(bytes, 4).ok_or("the file is too short to be a save")? as usize;
        if gs != GLOBAL_SECTION || ls != LEVEL_SECTION {
            return Err("this save was not written by a version of the game the editor knows".into());
        }
        if bytes.len() != FILE_SIZE {
            return Err(format!("the file is {} bytes; a save is {FILE_SIZE}", bytes.len()));
        }
        let global = Section::decode(&bytes[8..8 + gs])?;
        let mut levels = Vec::with_capacity(LEVEL_SLOTS);
        for l in 0..LEVEL_SLOTS {
            let at = 8 + gs + l * ls;
            levels.push(Section::decode(&bytes[at..at + ls])?);
        }
        let file = SaveFile { global, levels };
        let known = file.global.layout() == GLOBAL_LAYOUT && file.levels.iter().all(|l| l.layout() == LEVEL_LAYOUT);
        if !known {
            return Err("this save holds data the editor does not know; it is left as it is".into());
        }
        Ok(file)
    }

    /// SavePack: the two sizes, then every section with a fresh checksum.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut v = Vec::with_capacity(FILE_SIZE);
        v.extend_from_slice(&(section_size(GLOBAL_LAYOUT) as u32).to_le_bytes());
        v.extend_from_slice(&(section_size(LEVEL_LAYOUT) as u32).to_le_bytes());
        v.extend_from_slice(&self.global.encode());
        for l in &self.levels {
            v.extend_from_slice(&l.encode());
        }
        v
    }

    /// The level the save resumes on; -1 in a slot never saved to (the game writes all four
    /// slots so when it makes its folder).
    pub fn level(&self) -> i32 {
        i32_at(self.global.get(id::LEVEL), 0)
    }

    pub fn is_empty(&self) -> bool {
        self.level() <= 0
    }
}

// ---------------------------------------------------------------------------------------------
// What the chunks hold.

/// Global chunk ids (the address each is copied from in brackets).
mod id {
    /// The level the save resumes on, -1 in an empty slot (0x001D545C).
    pub const LEVEL: i32 = 0;
    /// The progress block (0x00142660, 128 bytes); its first word is the bolts.
    pub const PROGRESS: i32 = 39;
    /// The play time in ticks, 60 a second (0x001D5528; the Load page shows ticks / 216000 hours).
    pub const TIME: i32 = 3;
    /// The clock when saved, BCD (0x001D4BE0).
    pub const CLOCK: i32 = 4;
    /// Ammo s32[156] by item (0x001427F0).
    pub const AMMO: i32 = 9;
    /// Owned u8[156] by item (0x00142CA0).
    pub const OWNED: i32 = 10;
    /// Experience s32[156] by item (0x00142DE0).
    pub const XP: i32 = 40;
    /// Each item's row in the weapon table u8[160] (0x001425C0).
    pub const ROW: i32 = 30;
}

/// The most bolts the editor writes (the HUD's seven digits).
pub const MAX_BOLTS: u32 = 9_999_999;

const TICKS_A_SECOND: u32 = 60;

/// A row of the weapon table: its number, its name, its full ammo (0: the weapon uses none).
#[derive(Clone, Copy, Debug)]
pub struct Row {
    pub row: u8,
    pub name: &'static str,
    pub max_ammo: u32,
}

/// A weapon: its item number (its V1 row) and its rows from V1 up, in the order levelling up
/// follows them.
#[derive(Clone, Copy, Debug)]
pub struct Weapon {
    pub id: usize,
    pub name: &'static str,
    pub rows: &'static [Row],
}

const fn r(row: u8, name: &'static str, max_ammo: u32) -> Row {
    Row { row, name, max_ammo }
}

const fn w(id: usize, name: &'static str, rows: &'static [Row]) -> Weapon {
    Weapon { id, name, rows }
}

/// The OmniWrench and the twenty weapons, from the PAL disc's weapon table (Veldin's program)
/// and its English text: each weapon's rows from its V1 row along `+0x46`.
pub const WEAPONS: &[Weapon] = &[
    w(
        9,
        "OmniWrench",
        &[
            r(9, "OmniWrench", 0),
            r(164, "OmniWrench V2", 0),
            r(165, "OmniWrench V3", 0),
            r(198, "OmniWrench V4", 0),
            r(199, "OmniWrench V5", 0),
            r(200, "OmniWrench V6", 0),
            r(201, "OmniWrench V7", 0),
            r(202, "OmniWrench V8", 0),
        ],
    ),
    w(
        39,
        "Shock Blaster",
        &[
            r(39, "Shock Blaster", 30),
            r(40, "Shock Blaster V2", 35),
            r(41, "Shock Blaster V3", 40),
            r(42, "Shock Blaster V4", 40),
            r(43, "Shock Cannon", 50),
            r(44, "Mega Shock Cannon", 50),
            r(45, "Giga Shock Cannon", 55),
            r(46, "Omega Shock Cannon", 60),
        ],
    ),
    w(
        119,
        "Nitro Launcher",
        &[
            r(119, "Nitro Launcher", 8),
            r(120, "Nitro Launcher V2", 8),
            r(121, "Nitro Launcher V3", 10),
            r(122, "Nitro Launcher V4", 10),
            r(123, "Nitro Eruptor", 12),
            r(124, "Mega Nitro Eruptor", 12),
            r(125, "Giga Nitro Eruptor", 14),
            r(126, "Omega Nitro Eruptor", 16),
        ],
    ),
    w(
        47,
        "N60 Storm",
        &[
            r(47, "N60 Storm", 150),
            r(48, "N60 Storm V2", 175),
            r(49, "N60 Storm V3", 200),
            r(50, "N60 Storm V4", 225),
            r(51, "N90 Hurricane", 300),
            r(52, "Mega N90 Hurricane", 300),
            r(53, "Giga N90 Hurricane", 350),
            r(54, "Omega N90 Hurricane", 400),
        ],
    ),
    w(
        127,
        "Plasma Whip",
        &[
            r(127, "Plasma Whip", 25),
            r(128, "Plasma Whip V2", 30),
            r(129, "Plasma Whip V3", 35),
            r(130, "Plasma Whip V4", 40),
            r(131, "Quantum Whip", 50),
            r(132, "Mega Quantum Whip", 50),
            r(133, "Giga Quantum Whip", 55),
            r(134, "Omega Quantum Whip", 60),
        ],
    ),
    w(
        55,
        "Infector",
        &[
            r(55, "Infector", 15),
            r(56, "Infector V2", 15),
            r(57, "Infector V3", 18),
            r(58, "Infector V4", 18),
            r(59, "Infecto-Bomb", 20),
            r(60, "Mega Infecto-Bomb", 20),
            r(61, "Giga Infecto-Bomb", 25),
            r(62, "Omega Infecto-Bomb", 30),
        ],
    ),
    w(
        135,
        "Suck Cannon",
        &[
            r(135, "Suck Cannon", 0),
            r(136, "Suck Cannon V2", 0),
            r(137, "Suck Cannon V3", 0),
            r(138, "Suck Cannon V4", 0),
            r(139, "Vortex Cannon", 0),
            r(140, "Mega Vortex Cannon", 0),
            r(141, "Giga Vortex Cannon", 0),
            r(142, "Omega Vortex Cannon", 0),
        ],
    ),
    w(
        71,
        "Spitting Hydra",
        &[
            r(71, "Spitting Hydra", 15),
            r(72, "Spitting Hydra V2", 15),
            r(73, "Spitting Hydra V3", 15),
            r(74, "Spitting Hydra V4", 15),
            r(75, "Tempest", 15),
            r(76, "Mega Tempest", 15),
            r(77, "Giga Tempest", 18),
            r(78, "Omega Tempest", 20),
        ],
    ),
    w(
        87,
        "Agents of Doom",
        &[
            r(87, "Agents of Doom", 6),
            r(88, "Agents of Doom V2", 6),
            r(89, "Agents of Doom V3", 8),
            r(90, "Agents of Doom V4", 8),
            r(91, "Agents of Dread", 8),
            r(92, "Mega Agents of Dread", 8),
            r(93, "Giga Agents of Dread", 10),
            r(94, "Omega Agents of Dread", 12),
        ],
    ),
    w(
        111,
        "Flux Rifle",
        &[
            r(111, "Flux Rifle", 10),
            r(112, "Flux Rifle V2", 12),
            r(113, "Flux Rifle V3", 12),
            r(114, "Flux Rifle V4", 12),
            r(115, "Splitter Rifle", 15),
            r(116, "Mega Splitter Rifle", 15),
            r(117, "Giga Splitter Rifle", 18),
            r(118, "Omega Splitter Rifle", 20),
        ],
    ),
    w(
        63,
        "Annihilator",
        &[
            r(63, "Annihilator", 20),
            r(64, "Annihilator V2", 20),
            r(65, "Annihilator V3", 22),
            r(66, "Annihilator V4", 22),
            r(67, "Decimator", 25),
            r(68, "Mega Decimator", 25),
            r(69, "Giga Decimator", 28),
            r(70, "Omega Decimator", 30),
        ],
    ),
    w(
        103,
        "Holo Shield Glove",
        &[
            r(103, "Holo Shield Glove", 8),
            r(104, "Holo Shield Glove V2", 8),
            r(105, "Holo Shield Glove V3", 10),
            r(106, "Holo Shield Glove V4", 10),
            r(107, "Ultrashield Launcher", 12),
            r(108, "Mega Ultrashield Launcher", 12),
            r(109, "Giga Ultrashield Launcher", 12),
            r(110, "Omega Ultrashield Launcher", 14),
        ],
    ),
    w(
        79,
        "Disc Blade Gun",
        &[
            r(79, "Disc Blade Gun", 25),
            r(80, "Disc Blade Gun V2", 25),
            r(81, "Disc Blade Gun V3", 25),
            r(82, "Disc Blade Gun V4", 25),
            r(83, "Multi-Disc Gun", 25),
            r(84, "Mega Multi-Disc Gun", 25),
            r(85, "Giga Multi-Disc Gun", 28),
            r(86, "Omega Multi-Disc Gun", 30),
        ],
    ),
    w(
        95,
        "Rift Inducer",
        &[
            r(95, "Rift Inducer", 8),
            r(96, "Rift Inducer V2", 8),
            r(97, "Rift Inducer V3", 10),
            r(98, "Rift Inducer V4", 10),
            r(99, "Rift Ripper", 12),
            r(100, "Mega Rift Ripper", 12),
            r(101, "Giga Rift Ripper", 14),
            r(102, "Omega Rift Ripper", 16),
        ],
    ),
    w(
        143,
        "Qwack-O-Ray",
        &[
            r(143, "Qwack-O-Ray", 0),
            r(144, "Qwack-O-Ray V2", 0),
            r(145, "Qwack-O-Ray V3", 0),
            r(146, "Qwack-O-Ray V4", 0),
            r(147, "Qwack-O-Blitzer", 0),
            r(148, "Mega Qwack-O-Blitzer", 0),
            r(149, "Giga Qwack-O-Blitzer", 0),
            r(150, "Omega Qwack-O-Blitzer", 0),
        ],
    ),
    w(
        151,
        "RY3NO",
        &[
            r(151, "RY3NO", 25),
            r(152, "RY3NO V2", 30),
            r(153, "RY3NO V3", 35),
            r(154, "RY3NO V4", 40),
            r(155, "RYNOCIRATOR", 50),
        ],
    ),
    w(
        21,
        "Miniturret Glove",
        &[
            r(21, "Miniturret Glove", 10),
            r(162, "Miniturret Glove V2", 10),
            r(168, "Miniturret Glove V3", 12),
            r(169, "Miniturret Glove V4", 12),
            r(170, "Megaturret Glove", 12),
            r(171, "Mega Megaturret Glove", 12),
            r(172, "Giga Megaturret Glove", 14),
            r(173, "Omega Megaturret Glove", 16),
        ],
    ),
    w(
        17,
        "Lava Gun",
        &[
            r(17, "Lava Gun", 150),
            r(161, "Lava Gun V2", 150),
            r(174, "Lava Gun V3", 175),
            r(175, "Lava Gun V4", 175),
            r(176, "Liquid Nitrogen Gun", 200),
            r(177, "Mega Liquid Nitrogen Gun", 200),
            r(178, "Giga Liquid Nitrogen Gun", 250),
            r(179, "Omega Liquid Nitrogen Gun", 300),
        ],
    ),
    w(
        22,
        "Shield Charger",
        &[
            r(22, "Shield Charger", 3),
            r(167, "Shield Charger V2", 3),
            r(192, "Shield Charger V3", 3),
            r(193, "Shield Charger V4", 4),
            r(194, "Tesla Barrier", 4),
            r(195, "Mega Tesla Barrier", 4),
            r(196, "Giga Tesla Barrier", 5),
            r(197, "Omega Tesla Barrier", 5),
        ],
    ),
    w(
        19,
        "Bouncer",
        &[
            r(19, "Bouncer", 10),
            r(166, "Bouncer V2", 10),
            r(180, "Bouncer V3", 12),
            r(181, "Bouncer V4", 12),
            r(182, "Heavy Bouncer", 12),
            r(183, "Mega Heavy Bouncer", 12),
            r(184, "Giga Heavy Bouncer", 14),
            r(185, "Omega Heavy Bouncer", 16),
        ],
    ),
    w(
        16,
        "Plasma Coil",
        &[
            r(16, "Plasma Coil", 15),
            r(160, "Plasma Coil V2", 15),
            r(186, "Plasma Coil V3", 18),
            r(187, "Plasma Coil V4", 18),
            r(188, "Plasma Storm", 20),
            r(189, "Mega Plasma Storm", 20),
            r(190, "Giga Plasma Storm", 22),
            r(191, "Omega Plasma Storm", 25),
        ],
    ),
];

fn i32_at(b: &[u8], i: usize) -> i32 {
    i32::from_le_bytes(b[i * 4..i * 4 + 4].try_into().unwrap())
}

fn set_i32(b: &mut [u8], i: usize, v: i32) {
    b[i * 4..i * 4 + 4].copy_from_slice(&v.to_le_bytes());
}

impl Weapon {
    /// The weapon's version in a save: its rows' index of the row chunk 30 holds (0 for V1), or
    /// None for a row that is not one of the weapon's.
    fn version(&self, rows: &[u8]) -> Option<usize> {
        let row = *rows.get(self.id)?;
        self.rows.iter().position(|r| r.row == row)
    }
}

// ---------------------------------------------------------------------------------------------
// The page's types.

/// One weapon as the save has it.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac3WeaponState {
    /// The item number.
    pub id: usize,
    pub owned: bool,
    /// Rounds; only an owned weapon's are written, up to its version's full ammo.
    pub ammo: u32,
}

/// What the editor can change.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac3SaveFields {
    pub bolts: u32,
    pub weapons: Vec<Rac3WeaponState>,
}

/// What the page shows of a weapon, read only.
#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac3WeaponInfo {
    pub id: usize,
    pub name: String,
    /// The name of the version the save has (V1 when the row is not one of the weapon's).
    pub version_name: String,
    /// 1 for V1 up to 8 for Omega.
    pub version: u32,
    /// The version's full ammo; 0 for a weapon that uses none.
    pub max_ammo: u32,
    /// Experience as stored (the game counts it in 1/32 points).
    pub xp: i32,
}

/// A slot as the page shows it.
#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac3SaveView {
    pub serial: String,
    pub slot: usize,
    /// A digest of the file as read: a write is refused if the file changed since.
    pub fingerprint: String,
    /// Where the save resumes (read only).
    pub planet_name: Option<String>,
    /// When it was saved (read only).
    pub saved_at: Option<String>,
    /// Seconds played (read only).
    pub play_seconds: u32,
    pub fields: Rac3SaveFields,
    pub weapons: Vec<Rac3WeaponInfo>,
    pub max_bolts: u32,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac3WriteResult {
    pub backup: SaveBackupInfo,
    pub view: Rac3SaveView,
}

/// Whether this editor knows the saves of the game with this disc serial: Up Your Arsenal's PAL
/// release, whose executable's save tables were read, and NTSC-U's (`SCUS_973.53`), whose two
/// tables (its 0x00332408 and 0x00332698) list the same chunks at the same addresses and sizes, and
/// whose weapon table (Veldin's program, 0x002C5640) has the same rows: compared from both discs,
/// 2026-11-21.
pub fn edits_serial(serial: &str) -> bool {
    matches!(serial, "SCES_524.56" | "SCUS_973.53")
}

impl SaveFile {
    /// The fields the editor offers.
    pub fn fields(&self) -> Rac3SaveFields {
        let owned = self.global.get(id::OWNED);
        let ammo = self.global.get(id::AMMO);
        Rac3SaveFields {
            bolts: i32_at(self.global.get(id::PROGRESS), 0).max(0) as u32,
            weapons: WEAPONS
                .iter()
                .map(|w| Rac3WeaponState { id: w.id, owned: owned[w.id] != 0, ammo: i32_at(ammo, w.id).max(0) as u32 })
                .collect(),
        }
    }

    fn max_ammo(&self, w: &Weapon) -> u32 {
        w.rows[w.version(self.global.get(id::ROW)).unwrap_or(0)].max_ammo
    }

    /// Writes `want` in: the bolts, and the ammo of the weapons the save owns. Owning is not
    /// changed here; a request that changes it, or asks more than a version's full ammo, is
    /// refused.
    pub fn apply(&mut self, want: &Rac3SaveFields) -> Result<(), String> {
        validate(want)?;
        let have = self.fields();
        for (h, n) in have.weapons.iter().zip(&want.weapons) {
            let d = WEAPONS.iter().find(|w| w.id == n.id).ok_or("an unknown weapon")?;
            if h.id != n.id || h.owned != n.owned {
                return Err(format!("the {} cannot be given or taken here yet", d.name));
            }
            if n.ammo != h.ammo && (!n.owned || n.ammo > self.max_ammo(d)) {
                return Err(format!("the {} holds at most {} rounds", d.name, self.max_ammo(d)));
            }
        }
        let progress = self.global.get_mut(id::PROGRESS);
        set_i32(progress, 0, want.bolts as i32);
        let ammo = self.global.get_mut(id::AMMO);
        for (h, n) in have.weapons.iter().zip(&want.weapons) {
            if n.ammo != h.ammo {
                set_i32(ammo, n.id, n.ammo as i32);
            }
        }
        Ok(())
    }
}

/// The request's own checks: the bolts in range and one entry per weapon, in [`WEAPONS`]' order.
pub fn validate(f: &Rac3SaveFields) -> Result<(), String> {
    if f.bolts > MAX_BOLTS {
        return Err(format!("bolts go up to {MAX_BOLTS}"));
    }
    if f.weapons.len() != WEAPONS.len() || f.weapons.iter().zip(WEAPONS).any(|(s, w)| s.id != w.id) {
        return Err("the weapons are not the editor's list".into());
    }
    Ok(())
}

fn clock_text(clock: &[u8]) -> Option<String> {
    let bcd = |b: u8| (b >> 4 <= 9 && b & 15 <= 9).then_some((b >> 4) as u32 * 10 + (b & 15) as u32);
    if clock.len() < 8 || clock.iter().all(|&b| b == 0) {
        return None;
    }
    let (s, m, h, d, mo, y) =
        (bcd(clock[1])?, bcd(clock[2])?, bcd(clock[3])?, bcd(clock[5])?, bcd(clock[6])?, bcd(clock[7])?);
    Some(format!("{:04}-{mo:02}-{d:02} {h:02}:{m:02}:{s:02}", 2000 + y))
}

/// The page's view of a decoded save.
pub fn view(serial: &str, slot: usize, bytes: &[u8], file: &SaveFile) -> Rac3SaveView {
    let rows = file.global.get(id::ROW);
    let xp = file.global.get(id::XP);
    Rac3SaveView {
        serial: serial.to_string(),
        slot,
        fingerprint: crate::save_edit::fingerprint(bytes),
        planet_name: u32::try_from(file.level()).ok().and_then(|l| saves::get_planet_name(serial, l)),
        saved_at: clock_text(file.global.get(id::CLOCK)),
        play_seconds: u32::from_le_bytes(file.global.get(id::TIME)[..4].try_into().unwrap()) / TICKS_A_SECOND,
        fields: file.fields(),
        weapons: WEAPONS
            .iter()
            .map(|w| {
                let v = w.version(rows).unwrap_or(0);
                Rac3WeaponInfo {
                    id: w.id,
                    name: w.name.to_string(),
                    version_name: w.rows[v].name.to_string(),
                    version: v as u32 + 1,
                    max_ammo: w.rows[v].max_ammo,
                    xp: i32_at(xp, w.id),
                }
            })
            .collect(),
        max_bolts: MAX_BOLTS,
    }
}

// ---------------------------------------------------------------------------------------------
// The card on disk.

/// Save slots on a card (`save0.bin` .. `save3.bin`).
pub const SLOTS: usize = 4;

fn slot_path(card_dir: &Path, slot: usize) -> Result<PathBuf, String> {
    if slot >= SLOTS {
        return Err(format!("there is no slot {}", slot + 1));
    }
    let folder = saves::find_game_folder(card_dir).ok_or("there are no saves on this card yet")?;
    Ok(folder.join(format!("save{slot}.bin")))
}

fn read_slot(card_dir: &Path, serial: &str, slot: usize) -> Result<(PathBuf, Vec<u8>, SaveFile), String> {
    if !edits_serial(serial) {
        return Err("the save editor does not know this game's saves yet".into());
    }
    let path = slot_path(card_dir, slot)?;
    let bytes = fs::read(&path).map_err(|e| format!("slot {} cannot be read: {e}", slot + 1))?;
    let file = SaveFile::parse(&bytes)?;
    if file.is_empty() {
        return Err(format!("slot {} is empty", slot + 1));
    }
    Ok((path, bytes, file))
}

/// Reads slot `slot` of the card in `card_dir` for the editor.
pub fn read_save(card_dir: &Path, serial: &str, slot: usize) -> Result<Rac3SaveView, String> {
    let (_, bytes, file) = read_slot(card_dir, serial, slot)?;
    Ok(view(serial, slot, &bytes, &file))
}

/// Writes `want` into slot `slot` with rac1's guards ([`crate::save_edit::write_save`]): no game
/// using the card, the file unchanged since read, the request checked, the card backed up, the
/// new file read back and checked, then put in place in one rename.
pub fn write_save(
    card_dir: &Path,
    serial: &str,
    slot: usize,
    fingerprint_seen: &str,
    want: &Rac3SaveFields,
    in_use: impl Fn() -> Result<Option<String>, String>,
) -> Result<Rac3WriteResult, String> {
    if let Some(game) = in_use()? {
        return Err(format!("{game} is running with this memory card: close it first, so the save is not overwritten"));
    }
    let (path, bytes, mut file) = read_slot(card_dir, serial, slot)?;
    if crate::save_edit::fingerprint(&bytes) != fingerprint_seen {
        return Err("the save changed since it was opened (the game saved?): open it again".into());
    }
    file.apply(want)?;
    let out = file.to_bytes();
    let check = SaveFile::parse(&out)?;
    if check.fields() != *want {
        return Err("the edited save did not read back as asked; nothing was written".into());
    }
    let backup = saves::backup_card(card_dir, Some(&format!("before_edit_slot{}", slot + 1)))?;
    crate::save_edit::replace_save(&path, &out)?;
    Ok(Rac3WriteResult { backup, view: view(serial, slot, &out, &check) })
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Slot 0 as the PAL game writes it when it makes its folder on a fresh card (a hidden run of
    /// the port): an empty slot, level -1, a new game's item rows.
    const EMPTY: &[u8] = include_bytes!("../tests/fixtures/rac3/save-empty.bin");

    /// The game's empty slot made a save of Veldin, as the port's hidden run loaded it.
    fn veldin() -> SaveFile {
        let mut file = SaveFile::parse(EMPTY).unwrap();
        file.global.get_mut(id::LEVEL).copy_from_slice(&1i32.to_le_bytes());
        file.global.get_mut(id::TIME).copy_from_slice(&223_380u32.to_le_bytes());
        file.global.get_mut(id::CLOCK).copy_from_slice(&[0, 0x30, 0x15, 0x14, 0, 0x19, 0x11, 0x26]);
        set_i32(file.global.get_mut(id::PROGRESS), 0, 12_345);
        // The Shock Blaster owned at V2 with 20 rounds.
        file.global.get_mut(id::OWNED)[39] = 1;
        file.global.get_mut(id::ROW)[39] = 40;
        set_i32(file.global.get_mut(id::AMMO), 39, 20);
        set_i32(file.global.get_mut(id::XP), 39, 150 * 32);
        file
    }

    #[test]
    fn the_checksum_and_the_tables_are_the_games() {
        assert_eq!(crc16(b"123456789"), 0xd989, "rac1's CRC, the same routine");
        assert_eq!(crc16(&vec![0; GLOBAL_SECTION + 1]), 0);
        assert_eq!(section_size(GLOBAL_LAYOUT), GLOBAL_SECTION);
        assert_eq!(section_size(LEVEL_LAYOUT), LEVEL_SECTION);
        assert_eq!(EMPTY.len(), FILE_SIZE);
        assert_eq!(FILE_SIZE, 134_992);
    }

    #[test]
    fn a_game_save_round_trips_byte_for_byte() {
        let file = SaveFile::parse(EMPTY).unwrap();
        assert_eq!(file.to_bytes(), EMPTY);
        assert!(file.is_empty());
        assert_eq!(file.level(), -1);
        // A new game's rows: each item its own.
        assert!(file.global.get(id::ROW).iter().enumerate().all(|(i, &r)| r as usize == i || i >= 156));
    }

    #[test]
    fn a_damaged_or_unknown_save_is_refused() {
        let mut bytes = EMPTY.to_vec();
        bytes[40] ^= 1;
        assert!(SaveFile::parse(&bytes).unwrap_err().contains("checksum"));
        let mut bytes = EMPTY.to_vec();
        bytes[8 + GLOBAL_SECTION + 30] ^= 1;
        assert!(SaveFile::parse(&bytes).unwrap_err().contains("checksum"), "a level section too");
        assert!(SaveFile::parse(&EMPTY[..FILE_SIZE - 4]).is_err());
        let rac2 = [0xf8u8, 0x1c, 0, 0, 0xd0, 0x0c, 0, 0];
        assert!(SaveFile::parse(&rac2).unwrap_err().contains("version"));
    }

    #[test]
    fn the_view_reads_the_slot_lists_fields() {
        let file = veldin();
        let bytes = file.to_bytes();
        let v = view("SCES_524.56", 1, &bytes, &SaveFile::parse(&bytes).unwrap());
        assert_eq!(v.planet_name.as_deref(), Some("Veldin"));
        assert_eq!(v.saved_at.as_deref(), Some("2026-11-19 14:15:30"));
        assert_eq!(v.play_seconds, 3723);
        assert_eq!(v.fields.bolts, 12_345);
        let shock = v.weapons.iter().find(|w| w.id == 39).unwrap();
        assert_eq!(
            (shock.version_name.as_str(), shock.version, shock.max_ammo, shock.xp),
            ("Shock Blaster V2", 2, 35, 4800)
        );
        let state = v.fields.weapons.iter().find(|w| w.id == 39).unwrap();
        assert!(state.owned);
        assert_eq!(state.ammo, 20);
        assert!(!v.fields.weapons.iter().find(|w| w.id == 47).unwrap().owned);
        assert_eq!(v.weapons.len(), 21);
    }

    #[test]
    fn bolts_and_ammo_are_written_and_read_back() {
        let mut file = veldin();
        let mut want = file.fields();
        want.bolts = 1_000_000;
        want.weapons.iter_mut().find(|w| w.id == 39).unwrap().ammo = 35;
        file.apply(&want).unwrap();
        let back = SaveFile::parse(&file.to_bytes()).unwrap();
        assert_eq!(back.fields(), want);
        // Nothing else moved: the global section differs from before only in those two words.
        let before = veldin().global.encode();
        let after = back.global.encode();
        let changed = before[8..].iter().zip(&after[8..]).filter(|(a, b)| a != b).count();
        assert!(changed <= 8, "{changed} bytes changed");
    }

    #[test]
    fn requests_the_game_would_not_reach_are_refused() {
        let mut file = veldin();
        let mut want = file.fields();
        want.weapons.iter_mut().find(|w| w.id == 39).unwrap().ammo = 36;
        assert!(file.apply(&want).unwrap_err().contains("at most 35"));
        let mut want = file.fields();
        want.weapons.iter_mut().find(|w| w.id == 47).unwrap().owned = true;
        assert!(file.apply(&want).unwrap_err().contains("cannot be given"));
        let mut want = file.fields();
        want.weapons.iter_mut().find(|w| w.id == 47).unwrap().ammo = 10;
        assert!(file.apply(&want).is_err(), "an unowned weapon's ammo");
        let mut want = file.fields();
        want.bolts = MAX_BOLTS + 1;
        assert!(file.apply(&want).is_err());
        let mut want = file.fields();
        want.weapons.pop();
        assert!(file.apply(&want).is_err());
    }

    #[test]
    fn the_card_is_read_and_written_with_a_backup() {
        let dir = tempfile::tempdir().unwrap();
        let folder = dir.path().join("BESCES-52456RATCHET");
        fs::create_dir_all(&folder).unwrap();
        fs::write(folder.join("icon.sys"), [0u8; 964]).unwrap();
        fs::write(folder.join("save0.bin"), EMPTY).unwrap();
        fs::write(folder.join("save1.bin"), veldin().to_bytes()).unwrap();
        assert!(read_save(dir.path(), "SCES_524.56", 0).unwrap_err().contains("empty"));
        assert!(read_save(dir.path(), "SCUS_973.53", 1).is_ok(), "NTSC-U's tables are PAL's");
        assert!(read_save(dir.path(), "SLUS_209.99", 1).is_err(), "another game's saves are not read");
        let v = read_save(dir.path(), "SCES_524.56", 1).unwrap();
        let mut want = v.fields.clone();
        want.bolts = 50_000;
        assert!(write_save(dir.path(), "SCES_524.56", 1, "stale", &want, || Ok(None)).is_err());
        let running = write_save(dir.path(), "SCES_524.56", 1, &v.fingerprint, &want, || Ok(Some("The game".into())));
        assert!(running.unwrap_err().contains("running"));
        let done = write_save(dir.path(), "SCES_524.56", 1, &v.fingerprint, &want, || Ok(None)).unwrap();
        assert_eq!(done.view.fields.bolts, 50_000);
        assert_eq!(read_save(dir.path(), "SCES_524.56", 1).unwrap().fields.bolts, 50_000);
    }
}
