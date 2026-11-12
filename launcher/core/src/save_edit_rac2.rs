// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! The save editor for Ratchet & Clank: Going Commando (`SCUS_972.68`, v1.01 and v2.00, and the
//! PAL release `SCES_516.07`, whose save tables are the same: [`edits_serial`]): reads one slot of
//! the memory card, shows the fields whose place in the file was proven, and writes it back so
//! that the game still loads it. The format and how each field was proven are in
//! `launcher/docs/RAC2_SAVE_FORMAT.md`; the safety steps (game running, file changed since read,
//! backup, read-back, rename) are rac1's ([`crate::save_edit`]).
//!
//! # The format, in short
//!
//! `saveN.bin` (N = 0..3) is the game's `SavePack` (0x0029ADA0), the same container as rac1's:
//!
//! ```text
//! +0  u32 global section size (7416)    +4  u32 level section size (3280)
//! +8  the global section, then 28 level sections (one per level number)
//! section: u32 data size, u32 crc16(data), data = chunks, then {s32 -1, u32 0}
//! chunk:   s32 id, u32 size, size bytes, padding to 4
//! ```
//!
//! The chunks are the game's save tables (global 0x00256138, per level 0x00256368): each entry
//! is `{address, size, id}`, and a chunk is a copy of `size` bytes at `address` (a level's copy at
//! `address + level * size`). On load (`SaveUnpack` 0x00298D28, `RestoreBlocks` 0x0029B168) the
//! two sizes must match the tables and each section's checksum (`SaveChecksum` 0x0029AEB0) must be
//! non-zero and equal; [`crc16`] is that checksum. That is all the load checks: it copies the
//! chunks back as they are, weapon versions included, so the editor's own checks
//! ([`validate`], [`SaveFile::apply`]) are what keep a save to states the game can reach.
//!
//! # Weapon versions
//!
//! Read from the NTSC-U v1.01 executable and level 0's program (addresses are the executable's;
//! every level runs its own copy at the place `config/levels/functions.tsv` of the decomp lists).
//!
//! - **Where.** A weapon's version is its record: chunk 30 (`D_00139568`, u8 by item) holds the
//!   number of its row in the record table (`D_00239BA0`, 0xE0 bytes a row). The first version is
//!   the item's own number; the others are later rows, linked by +0x4A (next) and +0x4C
//!   (previous). +0x04 is the row's version (0 for V1 to 3 for V4), +0x6C the experience step to
//!   the next one (compared with `experience >> 5`, 0 for none), +0x8E the full ammo, +0x80 the
//!   price. Experience is chunk 40 (`D_00139868`, s32 by item); ammo chunk 9 (`D_00139688`).
//! - **The four versions.** The sixteen weapons that upgrade by experience have four rows, for
//!   example the Lancer 30, 60, 79, 80 (V1 Lancer, V2 Heavy Lancer, then V3 and V4, which keep
//!   the Heavy Lancer's model). V1 becomes V2 at its step (`func_L00_002D5EB8` each tick, through
//!   `func_00287EB0`: the record moves on, experience is raised to the step if under it, and ammo
//!   is set to the new row's full ammo). V2 has no step. V3 is the challenge-mode upgrade (the
//!   one players call Mega): the vendor's list (`func_002F6608`) offers it for an owned weapon
//!   whose row is V1 or V2 with no step left, only while `func_00288D58` (the challenge mode
//!   counter, progress +0x12, non-zero) holds; the purchase (0x002F8F78 in the vendor's update)
//!   gives the V3 row's full ammo, then `func_002887B0` moves the record two rows down the chain
//!   (`func_00287FF8`) and sets experience to 0. V3 becomes V4 by experience at its own step,
//!   as V1 does (the tick moves a row past V1 only in challenge mode).
//! - **Two-row weapons.** The Bomb Glove, Visibomb Gun, Decoy Glove, Tesla Claw and Walloper
//!   (12, 14, 17, 18, 53: the case list in `func_002887B0` and the vendor's) have no experience
//!   step: their second row is the challenge-mode purchase (one row down). The Clank Zapper's
//!   second row comes by experience, in either mode.
//! - **The Sheepinator** is left out of the vendor's upgrade (the list skips item 16); its V3
//!   comes when challenge mode starts (`func_002DE990`: V1 becomes V3, V2 becomes V4) or when it
//!   is given in challenge mode (`func_00288308`: V3, experience 0).
//! - **Ammo** is the row's +0x8E ([`ItemDef::ammo`]): every version holds its own maximum.
//!
//! So the game reaches V3 and V4, and the two-row weapons' second row, only in challenge mode, but
//! a save loads with any of them in any mode (outside it the game only gates earning them, so a V3
//! there still earns experience but never steps to V4: `func_L00_002D5EB8` skips a row whose
//! vendor state is set while `func_00288D58` is 0, and a hidden run of the port with V3 at exactly
//! V4's step and no challenge mode left it V3 800 frames later; V4 has no step), and the editor offers them on every
//! save. The version of the weapon in Ratchet's hand is not changed (a save with its record moved crashed the port at the level start, in the held weapon's
//! update: launcher/docs/RAC2_SAVE_FORMAT.md).

use crate::rac2_flags::{self, Confidence, Place};
use crate::saves::{self, SaveBackupInfo, RAC2_PLANETS};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::{Path, PathBuf};

// ---------------------------------------------------------------------------------------------
// The checksum and the container.

/// The global section's size: `SaveChecksum` returns 0, a checksum that always fails, for data
/// longer than it (`func_0029AE78` of the global table).
pub const GLOBAL_SECTION: usize = 7416;
/// Each level section's size.
pub const LEVEL_SECTION: usize = 3280;
/// Level sections in a save (`SavePack` loops 0x1C times).
pub const LEVEL_SLOTS: usize = 28;
/// The whole file.
pub const FILE_SIZE: usize = 8 + GLOBAL_SECTION + LEVEL_SLOTS * LEVEL_SECTION;

/// `SaveChecksum` (0x0029AEB0): rac1's CRC-16 (polynomial 0x1F45, register seeded with
/// 0xEDB88320, most significant bit first), with Going Commando's length limit.
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

/// One `{id, size, bytes}` chunk; `pad` is the bytes up to the next multiple of 4, kept as read
/// (the checksum covers them).
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

    /// `SaveWriteBlocks` (0x0029AF98): `{size, crc16}`, the chunks, `{-1, 0}`. A checksum of 0
    /// would make the game refuse the section, so then one padding byte (never read) is changed.
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

/// The global table (0x00256138), `(id, size)` in file order, as the retail executable lists it.
pub const GLOBAL_LAYOUT: &[(i32, usize)] = &[
    (0, 4),
    (39, 64),
    (3, 4),
    (4, 8),
    (1003, 4),
    (5, 205),
    (7, 12),
    (8, 32),
    (9, 224),
    (10, 56),
    (11, 56),
    (40, 224),
    (12, 32),
    (13, 32),
    (14, 28),
    (20, 112),
    (15, 1824),
    (16, 1932),
    (17, 516),
    (18, 672),
    (38, 40),
    (30, 80),
    (32, 28),
    (37, 12),
    (41, 76),
    (42, 56),
    (45, 56),
    (43, 256),
    (44, 4),
    (46, 40),
    (47, 256),
    (1005, 4),
    (1010, 170),
    (1011, 4),
];

/// The per-level table (0x00256368).
pub const LEVEL_LAYOUT: &[(i32, usize)] = &[
    (3001, 1),
    (3002, 2048),
    (3003, 4),
    (3004, 16),
    (3005, 1024),
    (3007, 12),
    (4002, 4),
    (4005, 64),
    (4007, 4),
    (4008, 4),
];

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

    /// `SavePack`: the two sizes, then every section with a fresh checksum.
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
}

// ---------------------------------------------------------------------------------------------
// What the chunks hold (each proven by a save written after changing it in a running game, see
// the format document).

/// Global chunk ids (the address each is copied from in brackets).
mod id {
    /// The level the save resumes on, -1 in a slot never saved to (0x001A79F0).
    pub const LEVEL: i32 = 0;
    /// The progress block (0x001A79F8, 64 bytes); its fields are the `P_` offsets.
    pub const PROGRESS: i32 = 39;
    /// The clock when saved, BCD (0x001A7360).
    pub const CLOCK: i32 = 4;
    /// Skill points u8[32], 30 used (0x001A7A60).
    pub const SKILL_POINTS: i32 = 8;
    /// Ammo s32[56] by item (0x00139688).
    pub const AMMO: i32 = 9;
    /// Owned u8[56] by item (0x001A7AF8).
    pub const OWNED: i32 = 10;
    /// "New" mark u8[56] by item (0x001A7B30).
    pub const NEW: i32 = 11;
    /// The weapon list's order u8[32]: low 6 bits the item, 0x40 owned, 0xff end (0x001A7B68).
    pub const ORDER: i32 = 12;
    /// Quick select s32[8]: item ids, 0 empty (0x001A73B8).
    pub const QUICK_SELECT: i32 = 13;
    /// Levels available to the ship u8[28] (0x001A7BC8).
    pub const AVAILABLE: i32 = 14;
    /// The galactic map's levels in the order they became available, s32[28] (0x00139948).
    pub const MAP_ORDER: i32 = 20;
    /// Each item's record u8[80]: the item's own number, or a later version's (0x00139568).
    pub const RECORD: i32 = 30;
    /// What the hero has on, s32[7] (0x001A7398).
    pub const WORN: i32 = 32;
    /// Weapon experience s32[56] by item (0x00139868).
    pub const XP: i32 = 40;
    /// Ship customisation word (0x001A7AF0): bits 16..20 the paint job.
    pub const SHIP: i32 = 44;
    /// Per level: visited u8 (0x001A7BE8).
    pub const VISITED: i32 = 3001;
    /// Per level: platinum bolts found u8[4] (0x0019B278).
    pub const PLATINUM: i32 = 3003;
}

/// Offsets in the progress block (chunk 39).
const P_BOLTS: usize = 0x00;
const P_RARITANIUM: usize = 0x04;
const P_MAX_HEALTH: usize = 0x08;
/// Times challenge mode was started, u8 (`func_002DE990` adds 1; `func_00288D58` tests it).
const P_CHALLENGE: usize = 0x12;
const P_NANOTECH_XP: usize = 0x14;
const P_ARMOR: usize = 0x20;
const P_BOOSTS: usize = 0x30;

/// Nanotech Boosts: ten bytes, each found one adding 1 to the maximum health
/// (`func_L00_002D5EB8` counts them).
pub const BOOSTS: usize = 10;
/// Skill points the game counts (`CountSkillPointsCompleted` clamps to 30).
pub const SKILL_POINT_COUNT: usize = 30;
/// Platinum bolt places per level.
pub const PLATINUM_PER_LEVEL: usize = 4;
/// Armour levels: 0 (none) to 4 (the shop's four armours; item 6's records 56..59 are their levels).
pub const MAX_ARMOR: u8 = 4;
/// Ship paint jobs (the disc's 18 paint pictures, RC2.HDR +0x48F0).
pub const SHIP_PAINTS: u8 = 18;
/// The editor's limit for bolts and raritanium (the HUD shows seven digits).
pub const MAX_BOLTS: u32 = 9_999_999;
/// The editor's limit for nanotech and weapon experience.
pub const MAX_XP: u32 = 9_999_999;

const ORDER_OWNED: u8 = 0x40;
const ORDER_END: u8 = 0xff;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub enum Rac2ItemKind {
    Weapon,
    Gadget,
}

/// A weapon's upgrade by experience: its name, and the experience it needs (the base record's
/// +0x6C, compared with experience >> 5 at each tick by `func_L00_002D5EB8`).
#[derive(Clone, Copy, Debug)]
pub struct Upgrade {
    pub name: &'static str,
    pub xp: u32,
}

/// An item the editor offers. `chain` is its records from the base version on (the record table's
/// +0x4A links); `ammo` is each one's full ammo (+0x8E), 0 for an item with none.
#[derive(Clone, Copy, Debug)]
pub struct ItemDef {
    pub id: usize,
    pub name: &'static str,
    pub kind: Rac2ItemKind,
    pub chain: &'static [u8],
    pub ammo: &'static [u32],
    pub upgrade: Option<Upgrade>,
}

const fn w(id: usize, name: &'static str, chain: &'static [u8], ammo: &'static [u32]) -> ItemDef {
    ItemDef { id, name, kind: Rac2ItemKind::Weapon, chain, ammo, upgrade: None }
}
const fn wu(
    id: usize,
    name: &'static str,
    chain: &'static [u8],
    ammo: &'static [u32],
    up: &'static str,
    xp: u32,
) -> ItemDef {
    ItemDef { id, name, kind: Rac2ItemKind::Weapon, chain, ammo, upgrade: Some(Upgrade { name: up, xp: xp << 5 }) }
}
const fn g(id: usize, name: &'static str) -> ItemDef {
    ItemDef { id, name, kind: Rac2ItemKind::Gadget, chain: &[], ammo: &[], upgrade: None }
}

/// The items, in id order. Ids, chains, ammo and thresholds are the retail record table's (read
/// from a level's RAM); the names match the records' moby classes and HUD icons to the item list
/// of the APRac2 randomizer.
pub const ITEMS: &[ItemDef] = &[
    g(2, "Heli-Pack"),
    g(3, "Thruster-Pack"),
    g(4, "Hydro-Pack"),
    g(5, "Mapper"),
    g(7, "Armor Magnetizer"),
    g(8, "Levitator"),
    wu(9, "Clank Zapper", &[9, 73], &[30, 30], "Clank Shocker", 2500),
    w(12, "Bomb Glove", &[12, 115], &[40, 60]),
    g(13, "Swingshot"),
    w(14, "Visibomb Gun", &[14, 117], &[20, 30]),
    wu(16, "Sheepinator", &[16, 72, 109, 110], &[0, 0, 0, 0], "Black Sheepinator", 900),
    w(17, "Decoy Glove", &[17, 118], &[20, 30]),
    w(18, "Tesla Claw", &[18, 114], &[300, 400]),
    g(19, "Gravity Boots"),
    g(20, "Grindboots"),
    g(21, "Glider"),
    wu(22, "Chopper", &[22, 65, 83, 84], &[35, 35, 50, 70], "Multi-Star", 400),
    wu(23, "Pulse Rifle", &[23, 66, 87, 88], &[8, 8, 12, 20], "Vaporizer", 150),
    wu(24, "Seeker Gun", &[24, 67, 85, 86], &[25, 20, 40, 50], "HK-22", 1050),
    wu(25, "Hoverbomb Gun", &[25, 70, 103, 104], &[10, 10, 15, 25], "Tetrabomb Gun", 3500),
    wu(26, "Blitz Gun", &[26, 68, 91, 92], &[40, 40, 50, 60], "Blitz Cannon", 750),
    wu(27, "Minirocket Tube", &[27, 69, 99, 100], &[25, 25, 40, 60], "Megarocket Cannon", 4000),
    wu(28, "Plasma Coil", &[28, 62, 101, 102], &[15, 15, 25, 50], "Plasma Storm", 2100),
    wu(29, "Lava Gun", &[29, 63, 95, 96], &[200, 200, 300, 400], "Meteor Gun", 1400),
    wu(30, "Lancer", &[30, 60, 79, 80], &[200, 300, 400, 500], "Heavy Lancer", 280),
    wu(31, "Synthenoid", &[31, 61, 93, 94], &[12, 12, 20, 40], "Kilonoid", 1200),
    wu(32, "Spiderbot Glove", &[32, 78, 105, 106], &[8, 8, 16, 24], "Tankbot Glove", 1000),
    g(36, "Dynamo"),
    wu(37, "Bouncer", &[37, 76, 97, 98], &[25, 25, 40, 60], "Heavy Bouncer", 1700),
    g(38, "Electrolyzer"),
    g(39, "Thermanator"),
    wu(41, "Miniturret Glove", &[41, 64, 89, 90], &[20, 20, 30, 50], "Megaturret Glove", 750),
    wu(42, "Gravity Bomb", &[42, 71, 81, 82], &[8, 8, 20, 30], "Mini-Nuke", 200),
    w(43, "Zodiac", &[43], &[4]),
    w(44, "RYNO II", &[44], &[100]),
    wu(45, "Shield Charger", &[45, 77, 107, 108], &[5, 8, 10, 15], "Tesla Barrier", 450),
    g(46, "Tractor Beam"),
    g(49, "Qwark Statuette"),
    g(50, "Box Breaker"),
    g(51, "Infiltrator"),
    w(53, "Walloper", &[53, 116], &[0, 0]),
    g(54, "Charge Boots"),
    g(55, "Hypnomatic"),
];

/// The experience a third version needs (its second version's record +0x6C, compared with
/// experience >> 5), by that record: the record table's later steps.
const LATER_STEPS: &[(u8, u32)] = &[
    (79, 8000),
    (81, 12000),
    (83, 5000),
    (85, 10600),
    (87, 4400),
    (89, 6000),
    (91, 7000),
    (93, 4000),
    (95, 4000),
    (97, 5000),
    (99, 8000),
    (101, 8000),
    (103, 10000),
    (105, 8000),
    (107, 6000),
    (109, 5000),
];

/// The Sheepinator, which the vendor never upgrades (its V3 comes when challenge mode starts).
const SHEEPINATOR: usize = 16;

/// How the game gets a weapon to one of its versions.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub enum Rac2VersionHow {
    /// The version an item is given in.
    First,
    /// The previous version upgrades itself at an amount of experience.
    Experience,
    /// Bought from a vendor in challenge mode (the Mega upgrade); experience starts again at 0.
    Bought,
    /// Given when challenge mode starts (the Sheepinator).
    ChallengeStart,
}

/// What the game requires of a weapon in one of its versions.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct VersionRule {
    how: Rac2VersionHow,
    /// Only reached in challenge mode.
    challenge: bool,
    /// The experience the game can leave it with, `(least, most)`, for a weapon that counts it.
    xp: Option<(u32, u32)>,
}

impl ItemDef {
    /// The experience at which the game moves the weapon from the version in `record` to the next,
    /// at its next tick in a level; `None` when nothing follows by experience.
    fn next_step(&self, record: u8) -> Option<u32> {
        match self.depth(record)? {
            0 => self.upgrade.map(|u| u.xp),
            _ => LATER_STEPS.iter().find(|&&(r, _)| r == record).map(|&(_, xp)| xp << 5),
        }
    }

    /// The versions the game can leave this item in, V1 first (empty for an item with one), with
    /// what each requires: see the module's "Weapon versions".
    fn versions(&self) -> Vec<VersionRule> {
        let n = self.chain.len();
        if n < 2 {
            return Vec::new();
        }
        let has_xp = self.upgrade.is_some();
        let xp = |min: u32, max: u32| has_xp.then_some((min, max));
        let mut out = Vec::with_capacity(n);
        for depth in 0..n {
            let step_in = match depth {
                0 => None,
                _ => self.next_step(self.chain[depth - 1]),
            };
            let max = self.next_step(self.chain[depth]).map_or(MAX_XP, |s| s - 1);
            let rule = match (depth, step_in) {
                (0, _) => VersionRule { how: Rac2VersionHow::First, challenge: false, xp: xp(0, max) },
                // Raised by experience: the game leaves at least the step (func_00287EB0).
                (1, Some(step)) => VersionRule { how: Rac2VersionHow::Experience, challenge: false, xp: xp(step, max) },
                (_, Some(step)) => VersionRule { how: Rac2VersionHow::Experience, challenge: true, xp: xp(step, max) },
                // Bought (experience 0, then counting up to the next step) or, for the
                // Sheepinator, given when challenge mode starts.
                (_, None) => {
                    let how =
                        if self.id == SHEEPINATOR { Rac2VersionHow::ChallengeStart } else { Rac2VersionHow::Bought };
                    VersionRule { how, challenge: true, xp: xp(0, max) }
                }
            };
            out.push(rule);
        }
        out
    }

    fn has_ammo(&self) -> bool {
        self.ammo.iter().any(|&a| a > 0)
    }
    /// Where a record is in the chain, if it is in it.
    fn depth(&self, record: u8) -> Option<usize> {
        self.chain.iter().position(|&r| r == record)
    }
}

/// "V1" to "V4".
fn version_label(depth: usize) -> String {
    format!("V{}", depth + 1)
}

/// A weapon's name in a version: the upgraded name from V2 on, for one that upgrades by
/// experience (V3 and V4 keep the V2's model and name); its own name otherwise.
fn version_name(d: &ItemDef, depth: usize) -> String {
    match d.upgrade {
        Some(u) if depth > 0 => u.name.to_string(),
        _ => d.name.to_string(),
    }
}

/// The levels the editor lists: Going Commando's numbered levels that have a section in the save.
fn listed_levels() -> impl Iterator<Item = (usize, &'static str)> {
    RAC2_PLANETS.iter().map(|&(id, name)| (id as usize, name)).filter(|&(id, _)| id < LEVEL_SLOTS)
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ItemState {
    pub id: usize,
    pub owned: bool,
    /// Ammo, for an item that has it.
    pub ammo: Option<u32>,
    /// Experience, for a weapon that upgrades by it: in a level, the game upgrades the weapon
    /// itself once this reaches the upgrade's threshold.
    pub xp: Option<u32>,
    /// The version, 0 for V1 to 3 for V4 (an index into the item's `versions`), for an item with
    /// more than one: the record chunk 30 holds.
    pub version: Option<u32>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2PlanetState {
    pub id: usize,
    /// On the ship's list of destinations.
    pub available: bool,
    pub visited: bool,
    /// One per platinum bolt place on the level: found or not.
    pub platinum: Vec<bool>,
}

/// The fields the page edits, sent back unchanged or changed.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2SaveFields {
    pub bolts: u32,
    pub raritanium: u32,
    /// Nanotech experience: the game raises the maximum health from it at each tick.
    pub nanotech_xp: u32,
    /// The ten Nanotech Boosts, found or not.
    pub boosts: Vec<bool>,
    /// The armour worn, 0 (none) to 4.
    pub armor: u8,
    /// The ship's paint job, 0 to 17.
    pub ship_paint: u8,
    pub items: Vec<Rac2ItemState>,
    pub planets: Vec<Rac2PlanetState>,
    /// The 30 skill points, in the order the game keeps them.
    pub skill_points: Vec<bool>,
    /// The Advanced view's progress flags ([`rac2_flags::all`]), in that order.
    pub flags: Vec<Rac2FlagState>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2FlagState {
    pub id: String,
    pub on: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2FlagInfo {
    pub id: String,
    /// The planet whose progress it is; `None` for no one planet.
    pub level: Option<usize>,
    pub name: String,
    /// One plain sentence: what it changes in the game.
    pub description: String,
    pub confidence: Confidence,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ItemInfo {
    pub id: usize,
    pub name: String,
    pub kind: Rac2ItemKind,
    /// The most ammo the version in the save holds; `None` for an item with no ammo.
    pub max_ammo: Option<u32>,
    /// The upgraded version's name, for a weapon that upgrades by experience.
    pub upgrade_name: Option<String>,
    /// The experience the upgrade needs.
    pub upgrade_xp: Option<u32>,
    /// The most experience the editor takes for the version in the save: one under the game's next
    /// upgrade step, or the editor's limit when nothing follows by experience.
    pub max_xp: Option<u32>,
    /// The item's versions, V1 first; empty for an item with one. The version in the save is
    /// the item's `version` in the fields.
    pub versions: Vec<Rac2VersionInfo>,
    /// Worn by the hero (taking it away, or changing its version, is refused).
    pub worn: bool,
}

/// One version of a weapon, with what the game requires of it.
#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2VersionInfo {
    /// "V1" to "V4".
    pub label: String,
    /// The weapon's name in this version: the first version's, or the upgraded one's after it.
    pub name: String,
    pub how: Rac2VersionHow,
    /// The experience the previous version upgrades at, for [`Rac2VersionHow::Experience`].
    pub step_xp: Option<u32>,
    /// The game reaches it only in challenge mode (New Game+).
    pub challenge_only: bool,
    /// The most ammo it holds; `None` for a weapon with no ammo.
    pub max_ammo: Option<u32>,
    /// The experience the game can leave it with, for a weapon that counts it: at least the step
    /// it was reached at, and under the step to the next version.
    pub min_xp: Option<u32>,
    pub max_xp: Option<u32>,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2PlanetInfo {
    pub id: usize,
    pub name: String,
}

/// A slot as the page shows it.
#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2SaveView {
    pub serial: String,
    pub slot: usize,
    /// A digest of the file as read: a write is refused if the file changed since.
    pub fingerprint: String,
    /// Where the save resumes (read only).
    pub planet_name: Option<String>,
    /// When it was saved (read only).
    pub saved_at: Option<String>,
    /// The maximum health in the save (read only: the game works it out from the nanotech
    /// experience and the boosts at the next tick).
    pub max_health: i32,
    pub fields: Rac2SaveFields,
    pub items: Vec<Rac2ItemInfo>,
    pub planets: Vec<Rac2PlanetInfo>,
    pub max_bolts: u32,
    pub max_xp: u32,
    /// Times challenge mode (New Game+) was started (read only): 0 for a first playthrough, where
    /// the game gives no weapon a version past V2.
    pub challenge_mode: u32,
    /// The Advanced view's progress flags, in the order of `fields.flags`.
    pub flags: Vec<Rac2FlagInfo>,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2WriteResult {
    pub backup: SaveBackupInfo,
    pub view: Rac2SaveView,
}

/// Whether this editor knows the saves of the game with this disc serial: Going Commando's three
/// releases. v1.01 and v2.00 share `SCUS_972.68`; the PAL release is `SCES_516.07`. All three list
/// the same save tables, entry for entry in id and size (the global table's 34 at 0x00256138 on
/// v1.01 and PAL, 0x002560B8 on v2.00; the per-level table's 10 right after it), so their saves
/// are the same chunks.
pub fn edits_serial(serial: &str) -> bool {
    serial == "SCUS_972.68" || serial == "SCES_516.07"
}

fn i32_at(b: &[u8], i: usize) -> i32 {
    i32::from_le_bytes(b[i * 4..i * 4 + 4].try_into().unwrap())
}

fn set_i32(b: &mut [u8], i: usize, v: i32) {
    b[i * 4..i * 4 + 4].copy_from_slice(&v.to_le_bytes());
}

fn word(b: &[u8], at: usize) -> u32 {
    u32::from_le_bytes(b[at..at + 4].try_into().unwrap())
}

fn set_word(b: &mut [u8], at: usize, v: u32) {
    b[at..at + 4].copy_from_slice(&v.to_le_bytes());
}

impl SaveFile {
    /// Whether the slot holds a game (the level is -1 in a slot never saved to).
    pub fn is_empty(&self) -> bool {
        i32_at(self.global.get(id::LEVEL), 0) < 0
    }

    fn worn(&self, item: usize) -> bool {
        let worn = self.global.get(id::WORN);
        (0..worn.len() / 4).any(|i| i32_at(worn, i) == item as i32)
    }

    pub fn fields(&self) -> Rac2SaveFields {
        let g = &self.global;
        let p = g.get(id::PROGRESS);
        let (owned, ammo, xp) = (g.get(id::OWNED), g.get(id::AMMO), g.get(id::XP));
        let items = ITEMS
            .iter()
            .map(|d| Rac2ItemState {
                id: d.id,
                owned: owned[d.id] != 0,
                ammo: d.has_ammo().then(|| i32_at(ammo, d.id).max(0) as u32),
                xp: d.upgrade.map(|_| i32_at(xp, d.id).max(0) as u32),
                version: (d.chain.len() > 1).then(|| self.depth(d) as u32),
            })
            .collect();
        let available = g.get(id::AVAILABLE);
        let planets = listed_levels()
            .map(|(l, _)| Rac2PlanetState {
                id: l,
                available: available[l] != 0,
                visited: self.levels[l].get(id::VISITED)[0] != 0,
                platinum: self.levels[l].get(id::PLATINUM).iter().map(|&b| b != 0).collect(),
            })
            .collect();
        Rac2SaveFields {
            bolts: word(p, P_BOLTS),
            raritanium: word(p, P_RARITANIUM),
            nanotech_xp: word(p, P_NANOTECH_XP),
            boosts: p[P_BOOSTS..P_BOOSTS + BOOSTS].iter().map(|&b| b != 0).collect(),
            armor: p[P_ARMOR],
            ship_paint: ((word(g.get(id::SHIP), 0) >> 16) & 31) as u8,
            items,
            planets,
            skill_points: g.get(id::SKILL_POINTS)[..SKILL_POINT_COUNT].iter().map(|&b| b != 0).collect(),
            flags: rac2_flags::all().map(|f| Rac2FlagState { id: f.id.to_string(), on: self.flag(f.place) }).collect(),
        }
    }

    /// The byte a flag's place names: a global chunk's, or a level section's.
    fn flag_byte(&mut self, chunk: i32, level: Option<usize>, offset: usize) -> &mut u8 {
        let section = match level {
            Some(l) => &mut self.levels[l],
            None => &mut self.global,
        };
        &mut section.get_mut(chunk)[offset]
    }

    fn flag(&self, place: Place) -> bool {
        let get = |chunk: i32, level: Option<usize>, offset: usize| match level {
            Some(l) => self.levels[l].get(chunk)[offset],
            None => self.global.get(chunk)[offset],
        };
        match place {
            Place::Byte { chunk, level, offset, .. } => get(chunk, level, offset) != 0,
            Place::SetOnly { chunk, offset, .. } => get(chunk, None, offset) != 0,
            Place::Bit { chunk, level, offset, bit } => get(chunk, level, offset) & (1 << bit) != 0,
            Place::Count { level, uid } => {
                let (offset, shift) = count_at(uid);
                (get(rac2_flags::MOBY_COUNTS, Some(level), offset) >> shift) & 7 != 0
            }
            Place::All(places) => places.first().is_some_and(|&p| self.flag(p)),
        }
    }

    fn set_flag(&mut self, place: Place, on: bool) {
        match place {
            Place::Byte { chunk, level, offset, on: value } => {
                *self.flag_byte(chunk, level, offset) = if on { value } else { 0 }
            }
            Place::SetOnly { chunk, offset, on: value } => {
                if on {
                    *self.flag_byte(chunk, None, offset) = value;
                }
            }
            Place::Bit { chunk, level, offset, bit } => {
                let byte = self.flag_byte(chunk, level, offset);
                if on {
                    *byte |= 1 << bit;
                } else {
                    *byte &= !(1 << bit);
                }
            }
            Place::Count { level, uid } => {
                let (offset, shift) = count_at(uid);
                let byte = self.flag_byte(rac2_flags::MOBY_COUNTS, Some(level), offset);
                // The count's three bits only, as the game raises them; bit 3 of the nibble is not
                // the count's.
                if !on {
                    *byte &= !(7 << shift);
                } else if (*byte >> shift) & 7 == 0 {
                    *byte |= 1 << shift;
                }
            }
            Place::All(places) => places.iter().for_each(|&p| self.set_flag(p, on)),
        }
    }

    /// Where `d`'s record is in its chain: its version (the first when the record is not one of
    /// its chain).
    fn depth(&self, d: &ItemDef) -> usize {
        d.depth(self.global.get(id::RECORD)[d.id]).unwrap_or(0)
    }

    /// The most ammo `d` holds in the version this save has.
    fn max_ammo(&self, d: &ItemDef) -> u32 {
        d.ammo.get(self.depth(d)).copied().unwrap_or(0)
    }

    /// Times challenge mode was started (progress +0x12).
    pub fn challenge_mode(&self) -> u32 {
        self.global.get(id::PROGRESS)[P_CHALLENGE] as u32
    }

    /// Applies `want` after checking it, changing only what differs from the save and keeping the
    /// chunks the game keeps together in step (owning a weapon also marks it new and puts it in
    /// the weapon list; an available level goes on the galactic map's list).
    pub fn apply(&mut self, want: &Rac2SaveFields) -> Result<(), String> {
        let have = self.fields();
        validate(want)?;
        for (d, (w, h)) in ITEMS.iter().zip(want.items.iter().zip(&have.items)) {
            if !w.owned && h.owned && self.worn(d.id) {
                return Err(format!("Ratchet has the {} in hand or on: change it in the game first", d.name));
            }
            let versions = d.versions();
            let depth = w.version.map_or(0, |v| v as usize);
            let rule = versions.get(depth).copied();
            let name = version_name(d, depth);
            if w.version != h.version {
                if self.worn(d.id) {
                    return Err(format!(
                        "Ratchet has the {} in hand: switch to another weapon in the game and save, then change its version",
                        d.name
                    ));
                }
                if !w.owned {
                    return Err(format!("the {} is not owned: give it to change its version", d.name));
                }
            }
            // The game never saves experience past the next step: it upgrades the weapon at the
            // next tick. A save loaded that way leaves the hero stuck at the level start (a hidden
            // run of the port, format document), so the editor keeps it under the step; and a
            // version reached by experience keeps at least its step (func_00287EB0).
            if let (Some(x), Some((min, max))) = (w.xp, rule.and_then(|r| r.xp)) {
                if (w.xp, w.version) != (h.xp, h.version) {
                    if x > max {
                        return Err(format!("the {name} upgrades at {} experience: keep it under that", max + 1));
                    }
                    if x < min {
                        return Err(format!("the {name} has at least {min} experience: the game upgrades at that"));
                    }
                }
            }
            if let Some(a) = w.ammo {
                let max = d.ammo.get(depth).copied().unwrap_or(0);
                if a > max && (w.ammo, w.version) != (h.ammo, h.version) {
                    return Err(format!("the {name} holds at most {max} ammo"));
                }
            }
        }

        let p = self.global.get_mut(id::PROGRESS);
        if want.bolts != have.bolts {
            set_word(p, P_BOLTS, want.bolts);
        }
        if want.raritanium != have.raritanium {
            set_word(p, P_RARITANIUM, want.raritanium);
        }
        if want.nanotech_xp != have.nanotech_xp {
            set_word(p, P_NANOTECH_XP, want.nanotech_xp);
        }
        if want.boosts != have.boosts {
            for (k, &on) in want.boosts.iter().enumerate() {
                p[P_BOOSTS + k] = on as u8;
            }
        }
        if want.armor != have.armor {
            p[P_ARMOR] = want.armor;
        }
        if want.ship_paint != have.ship_paint {
            let ship = self.global.get_mut(id::SHIP);
            let v = (word(ship, 0) & !(31 << 16)) | ((want.ship_paint as u32) << 16);
            set_word(ship, 0, v);
        }

        for (d, (w, h)) in ITEMS.iter().zip(want.items.iter().zip(&have.items)) {
            if w.owned != h.owned {
                if w.owned {
                    self.give(d);
                } else {
                    self.take(d.id);
                }
            }
            if let Some(v) = w.version {
                if w.version != h.version {
                    self.global.get_mut(id::RECORD)[d.id] = d.chain[v as usize];
                }
            }
            if let (Some(x), Some(old)) = (w.xp, h.xp) {
                if x != old {
                    set_i32(self.global.get_mut(id::XP), d.id, x as i32);
                }
            }
            if let (Some(a), Some(old)) = (w.ammo, h.ammo) {
                if a != old {
                    set_i32(self.global.get_mut(id::AMMO), d.id, a as i32);
                }
            }
        }

        for (w, h) in want.planets.iter().zip(&have.planets) {
            if w.available != h.available {
                if w.available {
                    self.make_available(w.id);
                } else {
                    self.make_unavailable(w.id);
                }
            }
            if w.visited != h.visited {
                self.levels[w.id].get_mut(id::VISITED)[0] = w.visited as u8;
            }
            if w.platinum != h.platinum {
                let pb = self.levels[w.id].get_mut(id::PLATINUM);
                for (k, &found) in w.platinum.iter().enumerate() {
                    pb[k] = found as u8;
                }
            }
        }

        for (d, (w, h)) in rac2_flags::all().zip(want.flags.iter().zip(&have.flags)) {
            if w.on != h.on {
                self.set_flag(d.place, w.on);
            }
        }

        if want.skill_points != have.skill_points {
            let sp = self.global.get_mut(id::SKILL_POINTS);
            for (k, &on) in want.skill_points.iter().enumerate() {
                sp[k] = on as u8;
            }
        }
        Ok(())
    }

    /// The parts of the game's item grant that are saved: owned, marked new, and a weapon at the
    /// end of the weapon list (the grant in a level, as the port's `give_item` mirrors it).
    fn give(&mut self, d: &ItemDef) {
        self.global.get_mut(id::OWNED)[d.id] = 1;
        self.global.get_mut(id::NEW)[d.id] = 1;
        if d.kind == Rac2ItemKind::Weapon {
            let order = self.global.get_mut(id::ORDER);
            if let Some(s) = order.iter().position(|&b| b != ORDER_END && (b & 0x3f) as usize == d.id) {
                order[s] |= ORDER_OWNED;
            } else if let Some(s) = order.iter().position(|&b| b == ORDER_END) {
                order[s] = d.id as u8 | ORDER_OWNED;
            }
        }
    }

    /// Gives an item up: not owned, not marked owned in the weapon list, not in the quick select.
    fn take(&mut self, item: usize) {
        self.global.get_mut(id::OWNED)[item] = 0;
        self.global.get_mut(id::NEW)[item] = 0;
        let order = self.global.get_mut(id::ORDER);
        for b in order.iter_mut().filter(|b| **b != ORDER_END && (**b & 0x3f) as usize == item) {
            *b &= !ORDER_OWNED;
        }
        let qs = self.global.get_mut(id::QUICK_SELECT);
        for i in 0..qs.len() / 4 {
            if i32_at(qs, i) == item as i32 {
                set_i32(qs, i, 0);
            }
        }
    }

    /// Available, and added to the galactic map's list after the levels available before it.
    fn make_available(&mut self, l: usize) {
        let n = self.global.get(id::AVAILABLE).iter().filter(|&&u| u != 0).count();
        self.global.get_mut(id::AVAILABLE)[l] = 1;
        let order = self.global.get_mut(id::MAP_ORDER);
        let listed = (0..n.min(order.len() / 4)).any(|i| i32_at(order, i) == l as i32);
        if !listed && n < order.len() / 4 {
            set_i32(order, n, l as i32);
        }
    }

    /// The reverse: not available, and off the galactic map's list, the later levels moving up.
    fn make_unavailable(&mut self, l: usize) {
        let n = self.global.get(id::AVAILABLE).iter().filter(|&&u| u != 0).count();
        self.global.get_mut(id::AVAILABLE)[l] = 0;
        let order = self.global.get_mut(id::MAP_ORDER);
        let len = n.min(order.len() / 4);
        let mut list: Vec<i32> = (0..len).map(|i| i32_at(order, i)).collect();
        if let Some(at) = list.iter().position(|&q| q == l as i32) {
            list.remove(at);
        }
        for i in 0..len {
            set_i32(order, i, list.get(i).copied().unwrap_or(0));
        }
    }
}

/// Checks a request against the editor's lists and limits before anything is changed.
pub fn validate(f: &Rac2SaveFields) -> Result<(), String> {
    if f.bolts > MAX_BOLTS {
        return Err(format!("bolts go up to {MAX_BOLTS}"));
    }
    if f.raritanium > MAX_BOLTS {
        return Err(format!("raritanium goes up to {MAX_BOLTS}"));
    }
    if f.nanotech_xp > MAX_XP {
        return Err(format!("nanotech experience goes up to {MAX_XP}"));
    }
    if f.boosts.len() != BOOSTS {
        return Err(format!("there are {BOOSTS} Nanotech Boosts"));
    }
    if f.armor > MAX_ARMOR {
        return Err(format!("armour goes from 0 to {MAX_ARMOR}"));
    }
    if f.ship_paint >= SHIP_PAINTS {
        return Err(format!("there are {SHIP_PAINTS} paint jobs (0 to {})", SHIP_PAINTS - 1));
    }
    if f.items.len() != ITEMS.len() || f.items.iter().zip(ITEMS).any(|(s, d)| s.id != d.id) {
        return Err("the item list does not match the game's".into());
    }
    for (s, d) in f.items.iter().zip(ITEMS) {
        if s.ammo.is_some() != d.has_ammo() {
            return Err(format!("{}'s ammo does not match the game's", d.name));
        }
        if s.xp.is_some() != d.upgrade.is_some() {
            return Err(format!("{}'s experience does not match the game's", d.name));
        }
        if s.xp.is_some_and(|x| x > MAX_XP) {
            return Err(format!("experience goes up to {MAX_XP}"));
        }
        let n = d.chain.len();
        match s.version {
            None if n > 1 => return Err(format!("{}'s version is missing", d.name)),
            Some(_) if n < 2 => return Err(format!("{} has one version", d.name)),
            Some(v) if v as usize >= n => return Err(format!("{} has {n} versions", d.name)),
            _ => {}
        }
    }
    let levels: Vec<usize> = listed_levels().map(|(l, _)| l).collect();
    if f.planets.len() != levels.len() || f.planets.iter().zip(&levels).any(|(s, &l)| s.id != l) {
        return Err("the planet list does not match the game's".into());
    }
    if f.planets.iter().any(|s| s.platinum.len() != PLATINUM_PER_LEVEL) {
        return Err(format!("each level has {PLATINUM_PER_LEVEL} platinum bolt places"));
    }
    if f.flags.len() != rac2_flags::all().count() || f.flags.iter().zip(rac2_flags::all()).any(|(s, d)| s.id != d.id) {
        return Err("the progress flags do not match the editor's".into());
    }
    if f.skill_points.len() != SKILL_POINT_COUNT {
        return Err(format!("there are {SKILL_POINT_COUNT} skill points"));
    }
    Ok(())
}

fn clock_text(clock: &[u8]) -> Option<String> {
    let bcd = |b: u8| (b >> 4 <= 9 && b & 15 <= 9).then_some((b >> 4) as u32 * 10 + (b & 15) as u32);
    if clock.iter().all(|&b| b == 0) {
        return None;
    }
    let (s, m, h, d, mo, y) =
        (bcd(clock[1])?, bcd(clock[2])?, bcd(clock[3])?, bcd(clock[5])?, bcd(clock[6])?, bcd(clock[7])?);
    Some(format!("{:04}-{mo:02}-{d:02} {h:02}:{m:02}:{s:02}", 2000 + y))
}

/// Where moby `uid`'s count is in its level's chunk 3005: the byte, and the shift of its nibble
/// (`func_L02_003C26F0`, `func_002A6C58`: the low nibble for an even id, the high for an odd one).
fn count_at(uid: u16) -> (usize, u32) {
    (usize::from(uid >> 1), if uid & 1 == 1 { 4 } else { 0 })
}

/// The page's view of a decoded save.
pub fn view(serial: &str, slot: usize, bytes: &[u8], file: &SaveFile) -> Rac2SaveView {
    let level = i32_at(file.global.get(id::LEVEL), 0);
    let record = file.global.get(id::RECORD);
    Rac2SaveView {
        serial: serial.to_string(),
        slot,
        fingerprint: crate::save_edit::fingerprint(bytes),
        planet_name: u32::try_from(level).ok().and_then(|l| saves::get_planet_name(serial, l)),
        saved_at: clock_text(file.global.get(id::CLOCK)),
        max_health: i32_at(&file.global.get(id::PROGRESS)[P_MAX_HEALTH..P_MAX_HEALTH + 4], 0),
        fields: file.fields(),
        items: ITEMS
            .iter()
            .map(|d| Rac2ItemInfo {
                id: d.id,
                name: d.name.to_string(),
                kind: d.kind,
                max_ammo: d.has_ammo().then(|| file.max_ammo(d)),
                upgrade_name: d.upgrade.map(|u| u.name.to_string()),
                upgrade_xp: d.upgrade.map(|u| u.xp),
                max_xp: d.upgrade.map(|_| d.next_step(record[d.id]).map_or(MAX_XP, |s| s - 1)),
                versions: d
                    .versions()
                    .iter()
                    .enumerate()
                    .map(|(k, r)| Rac2VersionInfo {
                        label: version_label(k),
                        name: version_name(d, k),
                        how: r.how,
                        step_xp: (k > 0 && r.how == Rac2VersionHow::Experience)
                            .then(|| d.next_step(d.chain[k - 1]))
                            .flatten(),
                        challenge_only: r.challenge,
                        max_ammo: d.has_ammo().then(|| d.ammo[k]),
                        min_xp: r.xp.map(|x| x.0),
                        max_xp: r.xp.map(|x| x.1),
                    })
                    .collect(),
                worn: file.worn(d.id),
            })
            .collect(),
        planets: listed_levels().map(|(id, name)| Rac2PlanetInfo { id, name: name.to_string() }).collect(),
        max_bolts: MAX_BOLTS,
        max_xp: MAX_XP,
        challenge_mode: file.challenge_mode(),
        flags: rac2_flags::all()
            .map(|f| Rac2FlagInfo {
                id: f.id.to_string(),
                level: f.level,
                name: f.name.to_string(),
                description: f.description.to_string(),
                confidence: f.confidence,
            })
            .collect(),
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

/// The clock slot `slot`'s save was stamped with (chunk 4, `g_saveClock`): what mods key the state
/// they keep beside a save by (mod_save_data.rs).
pub fn save_clock(card_dir: &Path, serial: &str, slot: usize) -> Result<[u8; 8], String> {
    let (_, _, file) = read_slot(card_dir, serial, slot)?;
    file.global.get(id::CLOCK).get(..8).and_then(|c| c.try_into().ok()).ok_or_else(|| "the save has no clock".into())
}

/// Reads slot `slot` of the card in `card_dir` for the editor.
pub fn read_save(card_dir: &Path, serial: &str, slot: usize) -> Result<Rac2SaveView, String> {
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
    want: &Rac2SaveFields,
    in_use: impl Fn() -> Result<Option<String>, String>,
) -> Result<Rac2WriteResult, String> {
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
    Ok(Rac2WriteResult { backup, view: view(serial, slot, &out, &check) })
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A save written by the game (scratch card: Oozla, after a Seeker Gun and 123,456 bolts were
    /// given in a hidden run and the pause menu saved; see the format document).
    const OOZLA: &[u8] = include_bytes!("../tests/fixtures/rac2/save-oozla.bin");

    fn oozla() -> SaveFile {
        SaveFile::parse(OOZLA).unwrap()
    }

    fn item(f: &Rac2SaveFields, id: usize) -> &Rac2ItemState {
        f.items.iter().find(|i| i.id == id).unwrap()
    }

    fn item_mut(f: &mut Rac2SaveFields, id: usize) -> &mut Rac2ItemState {
        f.items.iter_mut().find(|i| i.id == id).unwrap()
    }

    #[test]
    fn the_checksum_is_the_games() {
        assert_eq!(crc16(b"123456789"), 0xd989, "rac1's CRC, the same routine");
        assert_eq!(crc16(&vec![0; GLOBAL_SECTION + 1]), 0);
        // The global section's data (7408 bytes) is longer than rac1's limit, not Going Commando's.
        let g = &OOZLA[8..8 + GLOBAL_SECTION];
        assert_eq!(crc16(&g[8..]) as u32, u32_at(g, 4).unwrap());
    }

    #[test]
    fn a_game_save_round_trips_byte_for_byte() {
        assert_eq!(OOZLA.len(), FILE_SIZE);
        assert_eq!(section_size(GLOBAL_LAYOUT), GLOBAL_SECTION);
        assert_eq!(section_size(LEVEL_LAYOUT), LEVEL_SECTION);
        assert_eq!(oozla().to_bytes(), OOZLA);
    }

    #[test]
    fn a_damaged_or_unknown_save_is_refused() {
        let mut bytes = OOZLA.to_vec();
        bytes[40] ^= 1;
        assert!(SaveFile::parse(&bytes).unwrap_err().contains("checksum"));
        let mut bytes = OOZLA.to_vec();
        bytes[8 + GLOBAL_SECTION + 30] ^= 1;
        assert!(SaveFile::parse(&bytes).unwrap_err().contains("checksum"), "a level section too");
        assert!(SaveFile::parse(&OOZLA[..FILE_SIZE - 4]).is_err());
        assert!(SaveFile::parse(&[0; 8]).is_err());
        let rac1 = [0x30u8, 0x15, 0, 0, 0xa4, 0x0a, 0, 0];
        assert!(SaveFile::parse(&rac1).unwrap_err().contains("version"));
    }

    #[test]
    fn reads_the_fields() {
        let f = oozla().fields();
        assert_eq!(f.bolts, 123_456);
        assert_eq!(f.raritanium, 0);
        assert_eq!(f.nanotech_xp, 0x102d);
        assert_eq!(f.armor, 0);
        assert_eq!(f.ship_paint, 0);
        assert!(f.boosts.iter().all(|&b| !b));
        let seeker = item(&f, 24);
        assert!(seeker.owned);
        assert_eq!(seeker.ammo, Some(25));
        let lancer = item(&f, 30);
        assert!(lancer.owned);
        assert_eq!(lancer.xp, Some(0x7b0));
        assert!(!item(&f, 2).owned, "no Heli-Pack yet");
        let first = f.planets.iter().find(|p| p.id == 1).unwrap();
        assert!(first.available && first.visited);
        assert!(f.planets.iter().filter(|p| p.available).count() == 1);
        assert_eq!(f.planets.len(), 26, "the levels with a section in the save");
        assert!(f.skill_points.iter().all(|&s| !s));
        let v = view("SCUS_972.68", 0, OOZLA, &oozla());
        assert_eq!(v.planet_name.as_deref(), Some("Oozla"));
        assert_eq!(v.max_health, 4);
        assert!(v.saved_at.is_some());
    }

    /// Every changed byte between two saves, as (section, chunk id, offset in the chunk).
    fn changed(a: &SaveFile, b: &SaveFile) -> Vec<(Option<usize>, i32, usize)> {
        let mut out = Vec::new();
        let mut sec = |s: Option<usize>, x: &Section, y: &Section| {
            for (c, d) in x.chunks.iter().zip(&y.chunks) {
                out.extend((0..c.data.len()).filter(|&k| c.data[k] != d.data[k]).map(|k| (s, c.id, k)));
            }
        };
        sec(None, &a.global, &b.global);
        for (l, (x, y)) in a.levels.iter().zip(&b.levels).enumerate() {
            sec(Some(l), x, y);
        }
        out
    }

    #[test]
    fn each_field_lands_where_the_game_keeps_it() {
        // The places the game's own writes showed (format document, experiment e1).
        let before = oozla();
        let mut want = before.fields();
        want.bolts = 1;
        want.raritanium = 777;
        want.nanotech_xp = 20_000;
        want.boosts[0] = true;
        want.boosts[2] = true;
        want.armor = 2;
        want.ship_paint = 5;
        want.skill_points[0] = true;
        want.planets.iter_mut().find(|p| p.id == 1).unwrap().platinum[..2].fill(true);
        want.planets.iter_mut().find(|p| p.id == 3).unwrap().visited = true;
        item_mut(&mut want, 28).ammo = Some(10);
        let mut after = before.clone();
        after.apply(&want).unwrap();
        let mut c = changed(&before, &after);
        c.sort();
        let progress = |k| (None, id::PROGRESS, k);
        let expect = vec![
            (None, id::SKILL_POINTS, 0),
            (None, id::AMMO, 0x70),
            (None, id::SHIP, 2),
            progress(0),
            progress(1),
            progress(2),
            progress(4),
            progress(5),
            progress(0x14),
            progress(0x15),
            progress(0x20),
            progress(0x30),
            progress(0x32),
            (Some(1), id::PLATINUM, 0),
            (Some(1), id::PLATINUM, 1),
            (Some(3), id::VISITED, 0),
        ];
        let mut expect = expect;
        expect.sort();
        assert_eq!(c, expect);
        assert_eq!(SaveFile::parse(&after.to_bytes()).unwrap().fields(), want);
    }

    #[test]
    fn giving_a_weapon_writes_what_the_game_writes() {
        // The game's grant of the Seeker Gun (experiment e2): owned, new, ammo, the weapon list.
        let mut f = oozla();
        let mut want = f.fields();
        item_mut(&mut want, 24).owned = false;
        f.apply(&want).unwrap();
        assert_eq!(f.global.get(id::OWNED)[24], 0);
        assert_eq!(f.global.get(id::ORDER)[4], 24, "still listed, not owned");
        assert_eq!(i32_at(f.global.get(id::QUICK_SELECT), 2), 0, "off the quick select");
        let mut want = f.fields();
        item_mut(&mut want, 24).owned = true;
        f.apply(&want).unwrap();
        assert_eq!(f.global.get(id::ORDER)[4], 24 | 0x40);
        assert_eq!(f.global.get(id::NEW)[24], 1);
        // A weapon never listed goes at the end.
        let mut want = f.fields();
        item_mut(&mut want, 44).owned = true;
        f.apply(&want).unwrap();
        let order = f.global.get(id::ORDER);
        let end = order.iter().position(|&b| b == 0xff).unwrap();
        assert_eq!(order[end - 1], 44 | 0x40);
    }

    #[test]
    fn worn_items_cannot_be_taken() {
        let mut f = oozla();
        let worn = i32_at(f.global.get(id::WORN), 0) as usize;
        assert_eq!(worn, 30, "the Lancer is in hand");
        let mut want = f.fields();
        item_mut(&mut want, 30).owned = false;
        assert!(f.apply(&want).unwrap_err().contains("on"));
    }

    #[test]
    fn experience_and_ammo_follow_the_version_in_the_save() {
        let mut f = oozla();
        let mut want = f.fields();
        item_mut(&mut want, 30).xp = Some(280 * 32);
        assert!(f.clone().apply(&want).unwrap_err().contains("8960"), "the Lancer's step");
        item_mut(&mut want, 30).version = Some(1);
        assert!(f.clone().apply(&want).unwrap_err().contains("in hand"), "the Lancer is held");
        item_mut(&mut want, 30).version = Some(0);
        item_mut(&mut want, 30).xp = Some(280 * 32 - 1);
        f.apply(&want).unwrap();
        assert_eq!(i32_at(f.global.get(id::XP), 30), 8959);
        assert_eq!(f.global.get(id::RECORD)[30], 30, "the version is the game's to change");
        let v = view("SCUS_972.68", 0, &f.to_bytes(), &f);
        assert_eq!(v.items.iter().find(|i| i.id == 30).unwrap().max_xp, Some(8959));
        let mut over = f.fields();
        item_mut(&mut over, 30).ammo = Some(201);
        assert!(f.clone().apply(&over).unwrap_err().contains("200"));
        // A save with the Heavy Lancer holds 300.
        f.global.get_mut(id::RECORD)[30] = 60;
        let v = view("SCUS_972.68", 0, &f.to_bytes(), &f);
        let lancer = v.items.iter().find(|i| i.id == 30).unwrap();
        assert_eq!((item(&v.fields, 30).version, lancer.max_ammo, lancer.max_xp), (Some(1), Some(300), Some(MAX_XP)));
        let mut over = f.fields();
        item_mut(&mut over, 30).ammo = Some(300);
        item_mut(&mut over, 30).xp = Some(100_000);
        f.apply(&over).unwrap();
        // Later versions too.
        f.global.get_mut(id::RECORD)[30] = 79;
        let v = view("SCUS_972.68", 0, &f.to_bytes(), &f);
        let lancer = v.items.iter().find(|i| i.id == 30).unwrap();
        assert_eq!(
            (item(&v.fields, 30).version, lancer.max_ammo, lancer.max_xp),
            (Some(2), Some(400), Some(8000 * 32 - 1))
        );
    }

    /// The fixture as the same save would be once challenge mode was started.
    fn in_challenge_mode() -> SaveFile {
        let mut f = oozla();
        assert_eq!(f.challenge_mode(), 0, "the fixture is a first playthrough");
        f.global.get_mut(id::PROGRESS)[P_CHALLENGE] = 1;
        f
    }

    fn rule(d: usize, depth: usize) -> VersionRule {
        ITEMS.iter().find(|i| i.id == d).unwrap().versions()[depth]
    }

    #[test]
    fn weapon_versions_are_the_games() {
        use Rac2VersionHow::*;
        // The Lancer (records 30, 60, 79, 80): V2 at 280 << 5, V3 bought, V4 at 8000 << 5.
        let lancer: Vec<_> = (0..4).map(|k| rule(30, k)).collect();
        assert_eq!(lancer[0], VersionRule { how: First, challenge: false, xp: Some((0, 8959)) });
        assert_eq!(lancer[1], VersionRule { how: Experience, challenge: false, xp: Some((8960, MAX_XP)) });
        assert_eq!(lancer[2], VersionRule { how: Bought, challenge: true, xp: Some((0, 255_999)) });
        assert_eq!(lancer[3], VersionRule { how: Experience, challenge: true, xp: Some((256_000, MAX_XP)) });
        // Two-row weapons with no step: the second row is the challenge-mode purchase.
        for id in [12, 14, 17, 18, 53] {
            let d = ITEMS.iter().find(|i| i.id == id).unwrap();
            assert_eq!(d.versions().len(), 2);
            assert_eq!(rule(id, 1), VersionRule { how: Bought, challenge: true, xp: None }, "{}", d.name);
        }
        // The Clank Zapper's second row comes by experience, in either mode.
        assert_eq!(rule(9, 1), VersionRule { how: Experience, challenge: false, xp: Some((2500 * 32, MAX_XP)) });
        // The Sheepinator's V3 comes when challenge mode starts.
        assert_eq!(rule(16, 2).how, ChallengeStart);
        // Every four-row weapon has a step from V3 to V4; one-row items have no versions.
        for d in ITEMS.iter().filter(|d| d.chain.len() == 4) {
            assert!(d.next_step(d.chain[2]).is_some(), "{}", d.name);
            assert_eq!(d.ammo.len(), 4, "{}", d.name);
        }
        assert!(ITEMS.iter().filter(|d| d.chain.len() == 1).all(|d| d.versions().is_empty()));
        let v = view("SCUS_972.68", 0, OOZLA, &oozla());
        let seeker = v.items.iter().find(|i| i.id == 24).unwrap();
        let labels: Vec<_> = seeker.versions.iter().map(|x| (x.label.as_str(), x.name.as_str(), x.max_ammo)).collect();
        assert_eq!(
            labels,
            [
                ("V1", "Seeker Gun", Some(25)),
                ("V2", "HK-22", Some(20)),
                ("V3", "HK-22", Some(40)),
                ("V4", "HK-22", Some(50))
            ]
        );
        assert_eq!(seeker.versions[1].step_xp, Some(1050 * 32));
        assert_eq!(seeker.versions[3].step_xp, Some(10600 * 32));
        assert_eq!(v.challenge_mode, 0);
    }

    #[test]
    fn a_version_round_trips_as_the_game_leaves_it() {
        // The Seeker Gun (24, owned, not in hand) through its four versions in challenge mode,
        // with the experience and ammo the game's own upgrade and purchase leave.
        let mut f = in_challenge_mode();
        for (version, xp, ammo, record) in
            [(1, 1050 * 32, 20, 67), (2, 0, 40, 85), (3, 10600 * 32, 50, 86), (0, 100, 25, 24)]
        {
            let before = f.clone();
            let mut want = f.fields();
            let s = item_mut(&mut want, 24);
            (s.version, s.xp, s.ammo) = (Some(version), Some(xp), Some(ammo));
            f.apply(&want).unwrap();
            assert_eq!(f.global.get(id::RECORD)[24], record);
            let mut c = changed(&before, &f);
            c.retain(|&(_, chunk, _)| chunk != id::XP && chunk != id::AMMO);
            assert_eq!(c, [(None, id::RECORD, 24)], "only the record, experience and ammo change");
            let back = SaveFile::parse(&f.to_bytes()).unwrap();
            assert_eq!(back.fields(), want);
            let v = view("SCUS_972.68", 0, &f.to_bytes(), &back);
            assert_eq!(v.items.iter().find(|i| i.id == 24).unwrap().max_ammo, Some(ammo));
        }
    }

    #[test]
    fn experience_stays_in_the_versions_range() {
        let f = in_challenge_mode();
        let try_ = |version: u32, xp: u32, ammo: u32| {
            let mut want = f.fields();
            let s = item_mut(&mut want, 24);
            (s.version, s.xp, s.ammo) = (Some(version), Some(xp), Some(ammo));
            f.clone().apply(&want)
        };
        assert!(try_(1, 1050 * 32 - 1, 20).unwrap_err().contains("at least 33600"), "V2 keeps its step");
        assert!(try_(1, 1050 * 32, 21).unwrap_err().contains("at most 20"), "V2's ammo");
        assert!(try_(2, 10600 * 32, 40).unwrap_err().contains("339200"), "V3 is under its step");
        assert!(try_(3, 10600 * 32 - 1, 50).unwrap_err().contains("at least"), "V4 keeps its step");
        assert!(try_(2, 10600 * 32 - 1, 40).is_ok());
        assert!(try_(3, MAX_XP, 50).is_ok());
    }

    #[test]
    fn mega_versions_need_no_challenge_mode() {
        // A first playthrough: V3, V4 and the two-row purchases load like any other version.
        let f = oozla();
        assert_eq!(f.challenge_mode(), 0);
        let mut want = f.fields();
        let s = item_mut(&mut want, 24);
        (s.version, s.xp, s.ammo) = (Some(2), Some(0), Some(40));
        let mut a = f.clone();
        a.apply(&want).unwrap();
        assert_eq!(a.global.get(id::RECORD)[24], 85);
        let s = item_mut(&mut want, 24);
        (s.version, s.xp, s.ammo) = (Some(3), Some(10600 * 32), Some(50));
        let mut b = f.clone();
        b.apply(&want).unwrap();
        assert_eq!(b.global.get(id::RECORD)[24], 86);
        // The ranges still hold.
        let s = item_mut(&mut want, 24);
        (s.version, s.xp, s.ammo) = (Some(2), Some(10600 * 32), Some(40));
        assert!(f.clone().apply(&want).unwrap_err().contains("339200"));
        let s = item_mut(&mut want, 24);
        (s.version, s.xp, s.ammo) = (Some(2), Some(0), Some(41));
        assert!(f.clone().apply(&want).unwrap_err().contains("at most 40"));
        let mut glove = f.fields();
        let s = item_mut(&mut glove, 12);
        (s.owned, s.version) = (true, Some(1));
        let mut c = f.clone();
        c.apply(&glove).unwrap();
        assert_eq!(c.global.get(id::RECORD)[12], 115);
        // Only an owned weapon changes version.
        let mut lost = c.fields();
        let s = item_mut(&mut lost, 14);
        (s.owned, s.version) = (false, Some(1));
        assert!(c.clone().apply(&lost).unwrap_err().contains("not owned"));
        // A save already past V2 keeps loading and editing as it is.
        let mut past = oozla();
        past.global.get_mut(id::RECORD)[24] = 85;
        let mut want = past.fields();
        want.bolts = 5;
        past.apply(&want).unwrap();
    }

    #[test]
    fn progress_flags_land_where_the_game_keeps_them() {
        // The game's own writes on entering Maktar's arena the first time (format document, a0/a1).
        let before = oozla();
        let f = before.fields();
        // The fixture is on Oozla, past its first scenes: those are on, no story flag is.
        assert!(f.flags.iter().filter(|x| !x.id.contains("-scene-")).all(|x| !x.on));
        const ARENA: [&str; 2] = ["maktar-arena-entered", "maktar-arena-intro-seen"];
        let mut want = f.clone();
        want.flags.iter_mut().filter(|x| ARENA.contains(&x.id.as_str())).for_each(|x| x.on = true);
        let mut after = before.clone();
        after.apply(&want).unwrap();
        let mut c = changed(&before, &after);
        c.sort();
        assert_eq!(c, [(None, 5, 5), (None, 43, 0x29)]);
        assert_eq!(after.global.get(43)[0x29], 0x08);
        assert_eq!(SaveFile::parse(&after.to_bytes()).unwrap().fields(), want);
        let mut off = want.clone();
        off.flags.iter_mut().filter(|x| x.id == ARENA[1]).for_each(|x| x.on = false);
        after.apply(&off).unwrap();
        assert_eq!(after.global.get(43)[0x29], 0);
        let mut bad = want;
        bad.flags.pop();
        assert!(after.apply(&bad).is_err());
    }

    #[test]
    fn every_flag_names_a_place_in_the_save() {
        let mut ids = std::collections::HashSet::new();
        let mut last_level = 0;
        for f in rac2_flags::all() {
            assert!(ids.insert(f.id), "flag ids are unique: {}", f.id);
            assert!(!f.name.is_empty() && !f.description.is_empty(), "{} is described", f.id);
            if let Some(l) = f.level {
                assert!(RAC2_PLANETS.iter().any(|&(p, _)| p as usize == l), "{}: level {l}", f.id);
                assert!(l >= last_level, "{} is in planet order", f.id);
                last_level = l;
            }
            check_place(f.id, f.place);
        }
    }

    fn check_place(id: &str, place: Place) {
        let (chunk, level, offset, bit) = match place {
            Place::Byte { chunk, level, offset, on } => {
                assert_ne!(on, 0, "{id}: a set byte is not 0");
                (chunk, level, offset, 0)
            }
            Place::SetOnly { chunk, offset, on } => {
                assert_ne!(on, 0, "{id}: a set byte is not 0");
                (chunk, None, offset, 0)
            }
            Place::Bit { chunk, level, offset, bit } => (chunk, level, offset, bit),
            Place::Count { level, uid } => (rac2_flags::MOBY_COUNTS, Some(level), usize::from(uid >> 1), 0),
            Place::All(places) => {
                assert!(!places.is_empty(), "{id}: names its places");
                for &p in places {
                    assert!(!matches!(p, Place::All(_)), "{id}: one level of places");
                    check_place(id, p);
                }
                return;
            }
        };
        assert!(bit < 8, "{id}");
        let layout = if level.is_some() { LEVEL_LAYOUT } else { GLOBAL_LAYOUT };
        let size = layout.iter().find(|&&(c, _)| c == chunk).map(|&(_, n)| n);
        assert!(size.is_some_and(|n| offset < n), "{id}: chunk {chunk} offset {offset}");
        assert!(level.is_none_or(|l| l < LEVEL_SLOTS), "{id}");
    }

    #[test]
    fn aranos_prisons_section_done_writes_the_controllers_groups_and_the_packs() {
        const ID: &str = "aranos-prison-clank-section-done";
        let mut before = oozla();
        before.global.get_mut(id::OWNED)[2] = 0;
        before.global.get_mut(id::NEW)[2] = 0;
        let f = before.fields();
        assert!(!f.flags.iter().find(|x| x.id == ID).unwrap().on);
        let mut want = f.clone();
        want.flags.iter_mut().filter(|x| x.id == ID).for_each(|x| x.on = true);
        let mut after = before.clone();
        after.apply(&want).unwrap();
        assert_eq!(after.levels[14].get(3004)[3] & 0x80, 0x80);
        assert_eq!(after.levels[14].get(3004)[7] & 0x80, 0x80);
        assert_eq!(after.levels[14].get(3004)[8] & 0x80, 0x80);
        assert_eq!(after.levels[14].get(3004)[9] & 0x80, 0x80);
        assert_eq!(after.global.get(5)[0x29], 1);
        for item in [2, 3] {
            assert_eq!(after.global.get(id::OWNED)[item], 1);
            assert_eq!(after.global.get(id::NEW)[item], 1);
        }
        // Cleared, the groups and the story byte go; the packs stay.
        let mut off = after.fields();
        off.flags.iter_mut().filter(|x| x.id == ID).for_each(|x| x.on = false);
        after.apply(&off).unwrap();
        assert_eq!(after.levels[14].get(3004)[3] & 0x80, 0);
        assert_eq!(after.global.get(5)[0x29], 0);
        assert_eq!(after.global.get(id::OWNED)[2], 1);
    }

    #[test]
    fn an_electrolyzer_puzzle_is_reset_with_what_it_opened() {
        // Dobbo's board, moby 182 (even: the low nibble of byte 91 of level 9's chunk 3005), its
        // object group 5 (chunk 3004) and the seller's story byte 0x1F, as the game writes them.
        const ID: &str = "dobbo-electrolyzer";
        let before = oozla();
        let f = before.fields();
        assert!(!f.flags.iter().find(|x| x.id == ID).unwrap().on);
        let mut want = f.clone();
        want.flags.iter_mut().filter(|x| x.id == ID).for_each(|x| x.on = true);
        let mut after = before.clone();
        after.apply(&want).unwrap();
        let mut c = changed(&before, &after);
        c.sort();
        assert_eq!(c, [(None, 5, 0x1F), (Some(9), 3004, 5), (Some(9), 3005, 91)]);
        assert_eq!(after.levels[9].get(3005)[91], 0x01);
        assert_eq!(after.levels[9].get(3004)[5], 0x80);
        assert_eq!(after.global.get(5)[0x1F], 1);
        assert_eq!(SaveFile::parse(&after.to_bytes()).unwrap().fields(), want);

        // A count the game raised further, beside another moby's count in the same byte, is
        // cleared alone; bit 3 of the nibble is not the count's.
        after.levels[9].get_mut(3005)[91] = 0x5B;
        let solved = after.fields();
        assert!(solved.flags.iter().find(|x| x.id == ID).unwrap().on);
        let mut off = solved.clone();
        off.flags.iter_mut().filter(|x| x.id == ID).for_each(|x| x.on = false);
        after.apply(&off).unwrap();
        assert_eq!(after.levels[9].get(3005)[91], 0x58);
        assert_eq!(after.levels[9].get(3004)[5], 0);
        assert_eq!(after.global.get(5)[0x1F], 0);

        // An odd id is the high nibble: Maktar's moby 593 is byte 296's.
        let mut maktar = f.clone();
        maktar.flags.iter_mut().filter(|x| x.id == "maktar-electrolyzer-1").for_each(|x| x.on = true);
        let mut m = before.clone();
        m.apply(&maktar).unwrap();
        assert_eq!(m.levels[2].get(3005)[296], 0x10);
        assert_eq!(m.levels[2].get(3004)[6], 0x80);
        assert_eq!(m.global.get(5)[3], 1);
    }

    #[test]
    fn levels_keep_the_galactic_map_in_order() {
        let mut f = oozla();
        let mut want = f.fields();
        for l in [3, 4] {
            want.planets.iter_mut().find(|p| p.id == l).unwrap().available = true;
        }
        f.apply(&want).unwrap();
        let order = f.global.get(id::MAP_ORDER);
        assert_eq!((0..4).map(|i| i32_at(order, i)).collect::<Vec<_>>(), [1, 3, 4, 0]);
        let mut want = f.fields();
        want.planets.iter_mut().find(|p| p.id == 1).unwrap().available = false;
        f.apply(&want).unwrap();
        let order = f.global.get(id::MAP_ORDER);
        assert_eq!((0..3).map(|i| i32_at(order, i)).collect::<Vec<_>>(), [3, 4, 0]);
        assert_eq!(f.global.get(id::AVAILABLE)[1], 0);
    }

    #[test]
    fn the_limits_are_checked() {
        let f = oozla().fields();
        let bad = |change: &dyn Fn(&mut Rac2SaveFields)| {
            let mut w = f.clone();
            change(&mut w);
            oozla().apply(&w).is_err()
        };
        assert!(bad(&|w| w.bolts = MAX_BOLTS + 1));
        assert!(bad(&|w| w.raritanium = MAX_BOLTS + 1));
        assert!(bad(&|w| w.nanotech_xp = MAX_XP + 1));
        assert!(bad(&|w| w.armor = 5));
        assert!(bad(&|w| w.ship_paint = 18));
        assert!(bad(&|w| w.boosts.push(true)));
        assert!(bad(&|w| w.skill_points.pop().map(|_| ()).unwrap()));
        assert!(bad(&|w| w.items.pop().map(|_| ()).unwrap()));
        assert!(bad(&|w| w.planets[0].platinum.push(true)));
        assert!(bad(&|w| item_mut(w, 2).ammo = Some(1)));
        assert!(bad(&|w| item_mut(w, 12).xp = Some(1)), "upgrades by other means");
        assert!(bad(&|w| item_mut(w, 24).version = None), "a weapon with versions");
        assert!(bad(&|w| item_mut(w, 24).version = Some(4)), "four versions");
        assert!(bad(&|w| item_mut(w, 43).version = Some(0)), "the Zodiac has one");
        assert!(bad(&|w| item_mut(w, 12).version = Some(2)), "the Bomb Glove has two");
        assert!(!bad(&|w| w.bolts = MAX_BOLTS));
    }

    fn card() -> tempfile::TempDir {
        let dir = tempfile::tempdir().unwrap();
        let game = dir.path().join("BASCUS-97268RATCHET");
        fs::create_dir_all(&game).unwrap();
        fs::write(game.join("save1.bin"), OOZLA).unwrap();
        let mut empty = oozla();
        set_i32(empty.global.get_mut(id::LEVEL), 0, -1);
        fs::write(game.join("save2.bin"), empty.to_bytes()).unwrap();
        dir
    }

    #[test]
    fn a_write_backs_up_first_and_replaces_the_file() {
        let card = card();
        let v = read_save(card.path(), "SCUS_972.68", 1).unwrap();
        let mut want = v.fields.clone();
        want.raritanium = 4321;
        let r = write_save(card.path(), "SCUS_972.68", 1, &v.fingerprint, &want, || Ok(None)).unwrap();
        assert_eq!(r.view.fields.raritanium, 4321);
        assert!(r.backup.name.ends_with("before_edit_slot2"));
        let kept = fs::read(Path::new(&r.backup.path).join("BASCUS-97268RATCHET/save1.bin")).unwrap();
        assert_eq!(kept, OOZLA, "the backup is the save before the edit");
        assert_eq!(read_save(card.path(), "SCUS_972.68", 1).unwrap().fields.raritanium, 4321);
        assert_eq!(names(&card.path().join("BASCUS-97268RATCHET")), ["save1.bin", "save2.bin"]);
    }

    /// What a folder holds, by name.
    fn names(dir: &Path) -> Vec<String> {
        let mut v: Vec<String> =
            fs::read_dir(dir).unwrap().map(|e| e.unwrap().file_name().to_string_lossy().into_owned()).collect();
        v.sort();
        v
    }

    #[test]
    fn the_save_folder_holds_only_the_games_files() {
        // The game takes a folder with anything more than its own files for no save at all, so
        // the editor writes nothing there but the save it replaces: its backups and its
        // temporary file go beside the folder, and the temporary file never stays.
        let card = card();
        let game = card.path().join("BASCUS-97268RATCHET");
        for extra in ["icon.sys", "save0.bin", "save3.bin"] {
            fs::write(game.join(extra), b"the game's").unwrap();
        }
        let before = names(&game);
        let v = read_save(card.path(), "SCUS_972.68", 1).unwrap();
        let mut want = v.fields.clone();
        let s = item_mut(&mut want, 24);
        (s.version, s.xp, s.ammo) = (Some(1), Some(1050 * 32), Some(20));
        let refused = write_save(
            card.path(),
            "SCUS_972.68",
            1,
            &v.fingerprint,
            &{
                let mut w = want.clone();
                item_mut(&mut w, 24).xp = Some(1);
                w
            },
            || Ok(None),
        );
        assert!(refused.is_err());
        let r = write_save(card.path(), "SCUS_972.68", 1, &v.fingerprint, &want, || Ok(None)).unwrap();
        assert_eq!(item(&r.view.fields, 24).version, Some(1));
        let extra = crate::save_edit_rac2_extra::read_save(card.path(), "SCUS_972.68", 1).unwrap();
        crate::save_edit_rac2_extra::write_save(
            card.path(),
            "SCUS_972.68",
            1,
            &extra.fingerprint,
            &extra.fields,
            || Ok(None),
        )
        .unwrap();
        assert_eq!(names(&game), before, "only the game's files in its folder");
        let root = names(card.path());
        assert!(root.iter().all(|n| n == "BASCUS-97268RATCHET" || n.starts_with(".backup_")), "{root:?}");
        assert_eq!(root.len(), 3, "the folder and one backup per write");
        for b in root.iter().filter(|n| n.starts_with(".backup_")) {
            assert_eq!(names(&card.path().join(b).join("BASCUS-97268RATCHET")), before, "a backup is the folder");
        }
        // A write that cannot be put in place leaves nothing behind either.
        let blocked = game.join("save9.bin");
        fs::create_dir_all(blocked.join("x")).unwrap();
        assert!(crate::save_edit::replace_save(&blocked, b"new").is_err());
        fs::remove_dir_all(&blocked).unwrap();
        assert_eq!(names(&game), before);
        assert_eq!(names(card.path()), root);
    }

    #[test]
    fn a_write_is_refused_when_it_is_not_safe() {
        let card = card();
        let v = read_save(card.path(), "SCUS_972.68", 1).unwrap();
        let running =
            write_save(card.path(), "SCUS_972.68", 1, &v.fingerprint, &v.fields, || Ok(Some("The game".into())));
        assert!(running.unwrap_err().contains("running"));
        let stale = write_save(card.path(), "SCUS_972.68", 1, "0000", &v.fields, || Ok(None));
        assert!(stale.unwrap_err().contains("changed"));
        assert_eq!(fs::read(card.path().join("BASCUS-97268RATCHET/save1.bin")).unwrap(), OOZLA);
        assert!(read_save(card.path(), "SCUS_972.68", 2).unwrap_err().contains("empty"));
        assert!(read_save(card.path(), "SCUS_972.68", 4).is_err());
        assert!(read_save(card.path(), "SCES_509.16", 1).is_err(), "rac1's editor's serial");
        assert!(edits_serial("SCUS_972.68") && edits_serial("SCES_516.07") && !edits_serial("SCUS_971.99"));
    }

    #[test]
    fn the_game_running_counts_as_using_the_card() {
        let card = Path::new("/Users/you/Library/Application Support/OpenRAC/memcard/SCUS_972.68");
        let bundle = "/x/apps/openrac-rac2-ntsc/Ratchet & Clank - Going Commando (NTSC).app/Contents/MacOS/Ratchet & Clank - Going Commando (NTSC) --data /d";
        assert!(crate::save_edit::uses_card(bundle, card, "SCUS_972.68"));
        assert!(crate::save_edit::uses_card("/x/openrac-rac2-ntsc --data /d", card, "SCUS_972.68"));
    }
}
