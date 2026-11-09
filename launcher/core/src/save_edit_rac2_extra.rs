// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! The Going Commando save editor's advanced fields (the "More" part of its Advanced section): the
//! level the save resumes on, weapon mods, platinum bolts spent, the ship's parts, crystals and
//! moonstones. The container, the checksum and the main fields are [`crate::save_edit_rac2`]'s;
//! this module reads and writes only its own places, with the same guards (game running, file
//! changed since read, backup, read-back, rename). How each place was proven is in
//! `launcher/docs/RAC2_SAVE_FORMAT.md`, "The advanced fields".
//!
//! # Where Ratchet stands
//!
//! The save keeps the level (chunk 0) and nothing of the hero's position: a save made after moving
//! the hero differs from one made without moving only in the clock, and a loaded save always puts
//! the hero at the level's own start (its landing spot), wherever he was when it was saved. So the
//! editor changes the level, and the position is the level's start.

use crate::save_edit_rac2::{self as base, SaveFile};
use crate::saves::{self, SaveBackupInfo, RAC2_PLANETS};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::{Path, PathBuf};

/// Global chunk ids this module reads and writes.
mod id {
    /// The level the save resumes on, s32 (0x001A79F0).
    pub const LEVEL: i32 = 0;
    /// The progress block (0x001A79F8): crystals at +0x22 (s16), moonstones at +0x24 (s32).
    pub const PROGRESS: i32 = 39;
    /// Progress flags (0x001395B8): +0x90 the platinum bolts spent, u8 (`func_002B0D60`).
    pub const FLAGS: i32 = 5;
    /// Weapon mods bought, u8 per item (0x001A7A80, `g_itemStateFlags`).
    pub const MODS: i32 = 42;
    /// The ship's parts and paint (0x001A7AF0, `g_shipCustomization`).
    pub const SHIP: i32 = 44;
    /// Per level: platinum bolts found, u8[4] (0x0019B278).
    pub const PLATINUM: i32 = 3003;
}

const P_CRYSTALS: usize = 0x22;
const P_MOONSTONES: usize = 0x24;
const F_PLATINUM_SPENT: usize = 0x90;

/// The editor's limit for crystals and moonstones (Tabora's and Grelbin's counters; crystals are
/// an s16 in the game).
pub const MAX_GEMS: u32 = 9_999;

// ---------------------------------------------------------------------------------------------
// The levels a save can resume on.

/// The levels a save loaded from the title reached control on with only chunk 0 changed (a hidden
/// run of the port per level, format document). The Insomniac Museum (30) is left out: it has no
/// level section in the save, and the run crashed loading it.
pub const RESUME_LEVELS: &[u32] =
    &[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26];

fn level_name(l: u32) -> String {
    RAC2_PLANETS.iter().find(|&&(id, _)| id == l).map_or_else(|| format!("Level {l}"), |&(_, n)| n.to_string())
}

// ---------------------------------------------------------------------------------------------
// Weapon mods.

/// The three mods a weapon can have: each takes two bits of the item's byte in chunk 42, bought
/// (`1 << 2 * slot`, what the mod vendor and the weapon menu test) and switched on (the next bit up;
/// the lock-on targeting tests 0x20).
pub const MOD_NAMES: [&str; 3] = ["Shock Mod", "Acid Mod", "Lock On Mod"];

/// `(item, slot, price in platinum bolts)`: the mods the game's records offer (each record's
/// +0x98, +0x9C, +0xA0, read from a level's RAM; the mod vendor offers a slot only when it has a
/// price).
pub const WEAPON_MODS: &[(usize, u8, u32)] = &[
    (22, 1, 2),
    (22, 2, 1),
    (23, 0, 1),
    (24, 0, 1),
    (24, 2, 1),
    (25, 1, 3),
    (25, 2, 1),
    (26, 1, 2),
    (26, 2, 1),
    (27, 1, 2),
    (27, 2, 2),
    (28, 1, 2),
    (28, 2, 1),
    (29, 1, 2),
    (29, 2, 1),
    (30, 0, 2),
    (30, 2, 3),
    (32, 1, 2),
    (37, 1, 3),
    (37, 2, 1),
    (41, 0, 2),
    (42, 1, 2),
    (42, 2, 1),
    (44, 2, 1),
];

fn mod_bits(slot: u8) -> u8 {
    0b11 << (2 * slot)
}

fn item_name(item: usize) -> String {
    base::ITEMS.iter().find(|d| d.id == item).map_or_else(|| format!("Item {item}"), |d| d.name.to_string())
}

// ---------------------------------------------------------------------------------------------
// The ship's parts.

/// A part of the ship's word (chunk 44): the bits it takes and its choices, `(name, value)`, the
/// first the ship's own. Names and values are the Ship Shack's list (`func_002F7738`'s tables at
/// 0x0026D528: text id, mask, value); the paint (bits 16..20) is the main editor's.
#[derive(Clone, Copy, Debug)]
pub struct ShipPartDef {
    pub name: &'static str,
    pub mask: u32,
    pub choices: &'static [(&'static str, u32)],
}

pub const SHIP_PARTS: &[ShipPartDef] = &[
    ShipPartDef { name: "Nose", mask: 0x3, choices: &[("Standard Nose", 0), ("Scoop Nose", 0x2), ("Split Nose", 0x1)] },
    ShipPartDef {
        name: "Wings",
        mask: 0xC,
        choices: &[("Standard Wings", 0), ("Hi-Lift Wings", 0x4), ("Heavy Ordinance Wings", 0x8)],
    },
    ShipPartDef {
        name: "Engine",
        mask: 0x30,
        choices: &[("Standard engine", 0), ("Triple Boost Acceleration Engine", 0x10)],
    },
    ShipPartDef { name: "Lasers", mask: 0x40, choices: &[("Standard lasers", 0), ("Fusion Laser Cannons", 0x40)] },
    ShipPartDef {
        name: "Mines",
        mask: 0x180,
        choices: &[("None", 0), ("Electro Mine Launcher", 0x80), ("Mega-Mine Launcher", 0x100)],
    },
    ShipPartDef {
        name: "Missiles",
        mask: 0x200,
        choices: &[("Standard Missile Launcher", 0), ("Fast-lock Missile Launcher", 0x200)],
    },
    ShipPartDef {
        name: "Torpedoes",
        mask: 0xC00,
        choices: &[("None", 0), ("Torpedo Launcher", 0x400), ("Multi-Torpedo Launcher", 0x800)],
    },
    ShipPartDef {
        name: "Shields",
        mask: 0xC000,
        choices: &[
            ("Basic Shielding System", 0),
            ("Advanced Shielding System", 0x4000),
            ("Ultra-Charge Shielding System", 0x8000),
        ],
    },
    ShipPartDef { name: "Nuclear Detonation Device", mask: 0x1000, choices: &[("No", 0), ("Yes", 0x1000)] },
    ShipPartDef { name: "Hyperspace Warp System", mask: 0x2000, choices: &[("No", 0), ("Yes", 0x2000)] },
];

impl ShipPartDef {
    /// The choice the word holds; the engine's second value (0x20, the Ship Shack's second entry of
    /// the same engine) reads as the engine. `None` for a value the Ship Shack never sets.
    fn choice(&self, word: u32) -> Option<u32> {
        let v = word & self.mask;
        let v = if self.mask == 0x30 && v == 0x20 { 0x10 } else { v };
        self.choices.iter().position(|&(_, c)| c == v).map(|k| k as u32)
    }
}

// ---------------------------------------------------------------------------------------------
// What the page sends and shows.

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ModState {
    pub item: usize,
    /// 0 Shock, 1 Acid, 2 Lock On.
    pub slot: u8,
    pub bought: bool,
}

/// The advanced fields, sent back unchanged or changed.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ExtraFields {
    /// The level the save resumes on; the hero starts at its landing spot.
    pub level: u32,
    pub crystals: u32,
    pub moonstones: u32,
    /// Platinum bolts spent on mods: the game shows those found less these.
    pub platinum_spent: u32,
    /// Per part of [`SHIP_PARTS`], the index of its choice; `None` for a value the Ship Shack never
    /// sets (kept as it is unless changed).
    pub ship: Vec<Option<u32>>,
    pub mods: Vec<Rac2ModState>,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ModInfo {
    pub item: usize,
    pub item_name: String,
    pub slot: u8,
    pub name: String,
    /// Its price at the mod vendor, in platinum bolts.
    pub price: u32,
    /// The weapon is in the save's inventory.
    pub owned: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ShipPartInfo {
    pub name: String,
    pub choices: Vec<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2LevelInfo {
    pub id: u32,
    pub name: String,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ExtraView {
    pub serial: String,
    pub slot: usize,
    /// A digest of the file as read: a write is refused if the file changed since.
    pub fingerprint: String,
    pub fields: Rac2ExtraFields,
    /// The levels a save can resume on (the save's own level among them, even if not proven).
    pub levels: Vec<Rac2LevelInfo>,
    pub mods: Vec<Rac2ModInfo>,
    pub ship: Vec<Rac2ShipPartInfo>,
    /// Platinum bolts found on every level (the most that can be spent).
    pub platinum_found: u32,
    pub max_gems: u32,
}

#[derive(Clone, Debug, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Rac2ExtraWriteResult {
    pub backup: SaveBackupInfo,
    pub view: Rac2ExtraView,
}

// ---------------------------------------------------------------------------------------------
// Reading and writing.

fn chunk(file: &SaveFile, cid: i32) -> &[u8] {
    &file.global.chunks.iter().find(|c| c.id == cid).expect("SaveFile::parse checks the layout").data
}

fn chunk_mut(file: &mut SaveFile, cid: i32) -> &mut Vec<u8> {
    &mut file.global.chunks.iter_mut().find(|c| c.id == cid).expect("SaveFile::parse checks the layout").data
}

fn word(b: &[u8], at: usize) -> u32 {
    u32::from_le_bytes(b[at..at + 4].try_into().unwrap())
}

fn set_word(b: &mut [u8], at: usize, v: u32) {
    b[at..at + 4].copy_from_slice(&v.to_le_bytes());
}

/// Platinum bolts found on every level, as the game counts them (`func_002B0DB8`).
pub fn platinum_found(file: &SaveFile) -> u32 {
    file.levels
        .iter()
        .map(|l| {
            let pb = &l.chunks.iter().find(|c| c.id == id::PLATINUM).expect("the layout").data;
            pb.iter().filter(|&&b| b != 0).count() as u32
        })
        .sum()
}

pub fn fields(file: &SaveFile) -> Rac2ExtraFields {
    let p = chunk(file, id::PROGRESS);
    let mods = chunk(file, id::MODS);
    let ship = word(chunk(file, id::SHIP), 0);
    Rac2ExtraFields {
        level: word(chunk(file, id::LEVEL), 0),
        crystals: i16::from_le_bytes([p[P_CRYSTALS], p[P_CRYSTALS + 1]]).max(0) as u32,
        moonstones: (word(p, P_MOONSTONES) as i32).max(0) as u32,
        platinum_spent: chunk(file, id::FLAGS)[F_PLATINUM_SPENT] as u32,
        ship: SHIP_PARTS.iter().map(|d| d.choice(ship)).collect(),
        mods: WEAPON_MODS
            .iter()
            .map(|&(item, slot, _)| Rac2ModState { item, slot, bought: mods[item] & (1 << (2 * slot)) != 0 })
            .collect(),
    }
}

/// Checks a request against the lists and limits before anything is changed.
pub fn validate(file: &SaveFile, f: &Rac2ExtraFields) -> Result<(), String> {
    let have = fields(file);
    if f.level != have.level && !RESUME_LEVELS.contains(&f.level) {
        return Err(format!("the editor cannot move a save to {}", level_name(f.level)));
    }
    if f.crystals > MAX_GEMS || f.moonstones > MAX_GEMS {
        return Err(format!("crystals and moonstones go up to {MAX_GEMS}"));
    }
    let found = platinum_found(file);
    if f.platinum_spent != have.platinum_spent && f.platinum_spent > found {
        return Err(format!("only {found} platinum bolts are found in this save: no more can be spent"));
    }
    if f.ship.len() != SHIP_PARTS.len() {
        return Err("the ship's parts do not match the game's".into());
    }
    for ((w, h), d) in f.ship.iter().zip(&have.ship).zip(SHIP_PARTS) {
        if w != h && !w.is_some_and(|k| (k as usize) < d.choices.len()) {
            return Err(format!("{} has no such choice", d.name));
        }
    }
    if f.mods.len() != WEAPON_MODS.len()
        || f.mods.iter().zip(WEAPON_MODS).any(|(m, &(item, slot, _))| m.item != item || m.slot != slot)
    {
        return Err("the weapon mod list does not match the game's".into());
    }
    Ok(())
}

/// Applies `want` after checking it, changing only what differs from the save.
pub fn apply(file: &mut SaveFile, want: &Rac2ExtraFields) -> Result<(), String> {
    validate(file, want)?;
    let have = fields(file);
    if want.level != have.level {
        set_word(chunk_mut(file, id::LEVEL), 0, want.level);
    }
    let p = chunk_mut(file, id::PROGRESS);
    if want.crystals != have.crystals {
        p[P_CRYSTALS..P_CRYSTALS + 2].copy_from_slice(&(want.crystals as i16).to_le_bytes());
    }
    if want.moonstones != have.moonstones {
        set_word(p, P_MOONSTONES, want.moonstones);
    }
    if want.platinum_spent != have.platinum_spent {
        chunk_mut(file, id::FLAGS)[F_PLATINUM_SPENT] = want.platinum_spent as u8;
    }
    if want.ship != have.ship {
        let ship = chunk_mut(file, id::SHIP);
        let mut v = word(ship, 0);
        for ((w, h), d) in want.ship.iter().zip(&have.ship).zip(SHIP_PARTS) {
            if let (Some(k), true) = (w, w != h) {
                v = (v & !d.mask) | d.choices[*k as usize].1;
            }
        }
        set_word(ship, 0, v);
    }
    let mods = chunk_mut(file, id::MODS);
    for (w, h) in want.mods.iter().zip(&have.mods) {
        if w.bought != h.bought {
            let bits = mod_bits(w.slot);
            if w.bought {
                mods[w.item] |= bits;
            } else {
                mods[w.item] &= !bits;
            }
        }
    }
    Ok(())
}

pub fn view(serial: &str, slot: usize, bytes: &[u8], file: &SaveFile) -> Rac2ExtraView {
    let f = fields(file);
    let owned = &file.global.chunks.iter().find(|c| c.id == 10).expect("the layout").data;
    let mut levels: Vec<u32> = RESUME_LEVELS.to_vec();
    if !levels.contains(&f.level) {
        levels.push(f.level);
        levels.sort_unstable();
    }
    Rac2ExtraView {
        serial: serial.to_string(),
        slot,
        fingerprint: crate::save_edit::fingerprint(bytes),
        levels: levels.into_iter().map(|id| Rac2LevelInfo { id, name: level_name(id) }).collect(),
        mods: WEAPON_MODS
            .iter()
            .map(|&(item, slot, price)| Rac2ModInfo {
                item,
                item_name: item_name(item),
                slot,
                name: MOD_NAMES[slot as usize].to_string(),
                price,
                owned: owned[item] != 0,
            })
            .collect(),
        ship: SHIP_PARTS
            .iter()
            .map(|d| Rac2ShipPartInfo {
                name: d.name.to_string(),
                choices: d.choices.iter().map(|&(n, _)| n.to_string()).collect(),
            })
            .collect(),
        platinum_found: platinum_found(file),
        max_gems: MAX_GEMS,
        fields: f,
    }
}

// ---------------------------------------------------------------------------------------------
// The card on disk (the same steps as the main editor's).

fn read_slot(card_dir: &Path, serial: &str, slot: usize) -> Result<(PathBuf, Vec<u8>, SaveFile), String> {
    if !base::edits_serial(serial) {
        return Err("the save editor does not know this game's saves yet".into());
    }
    if slot >= base::SLOTS {
        return Err(format!("there is no slot {}", slot + 1));
    }
    let folder = saves::find_game_folder(card_dir).ok_or("there are no saves on this card yet")?;
    let path = folder.join(format!("save{slot}.bin"));
    let bytes = fs::read(&path).map_err(|e| format!("slot {} cannot be read: {e}", slot + 1))?;
    let file = SaveFile::parse(&bytes)?;
    if file.is_empty() {
        return Err(format!("slot {} is empty", slot + 1));
    }
    Ok((path, bytes, file))
}

/// Reads slot `slot` of the card in `card_dir` for the advanced fields.
pub fn read_save(card_dir: &Path, serial: &str, slot: usize) -> Result<Rac2ExtraView, String> {
    let (_, bytes, file) = read_slot(card_dir, serial, slot)?;
    Ok(view(serial, slot, &bytes, &file))
}

/// Writes `want` into slot `slot` with the main editor's guards: no game using the card, the file
/// unchanged since read, the request checked, the card backed up, the new file read back and
/// checked, then put in place in one rename.
pub fn write_save(
    card_dir: &Path,
    serial: &str,
    slot: usize,
    fingerprint_seen: &str,
    want: &Rac2ExtraFields,
    in_use: impl Fn() -> Result<Option<String>, String>,
) -> Result<Rac2ExtraWriteResult, String> {
    if let Some(game) = in_use()? {
        return Err(format!("{game} is running with this memory card: close it first, so the save is not overwritten"));
    }
    let (path, bytes, mut file) = read_slot(card_dir, serial, slot)?;
    if crate::save_edit::fingerprint(&bytes) != fingerprint_seen {
        return Err("the save changed since it was opened (the game saved?): open it again".into());
    }
    let main_before = file.fields();
    apply(&mut file, want)?;
    let out = file.to_bytes();
    let check = SaveFile::parse(&out)?;
    if fields(&check) != *want || check.fields() != main_before {
        return Err("the edited save did not read back as asked; nothing was written".into());
    }
    let backup = saves::backup_card(card_dir, Some(&format!("before_edit_slot{}", slot + 1)))?;
    crate::save_edit::replace_save(&path, &out)?;
    Ok(Rac2ExtraWriteResult { backup, view: view(serial, slot, &out, &check) })
}

#[cfg(test)]
mod tests {
    use super::*;

    /// The main editor's fixture: a save the game wrote on Oozla (level 1).
    const OOZLA: &[u8] = include_bytes!("../tests/fixtures/rac2/save-oozla.bin");

    fn oozla() -> SaveFile {
        SaveFile::parse(OOZLA).unwrap()
    }

    fn global(file: &SaveFile, cid: i32) -> Vec<u8> {
        chunk(file, cid).to_vec()
    }

    /// Writes `want` and returns the save read back from its bytes.
    fn round_trip(want: &Rac2ExtraFields) -> SaveFile {
        let mut f = oozla();
        apply(&mut f, want).unwrap();
        let back = SaveFile::parse(&f.to_bytes()).unwrap();
        assert_eq!(fields(&back), *want, "reads back as written");
        assert_eq!(back.fields(), oozla().fields(), "the main fields are untouched");
        back
    }

    #[test]
    fn reads_the_fields() {
        let f = fields(&oozla());
        assert_eq!(f.level, 1);
        assert_eq!((f.crystals, f.moonstones, f.platinum_spent), (0, 0, 0));
        assert!(f.ship.iter().all(|&c| c == Some(0)), "the ship's own parts");
        assert!(f.mods.iter().all(|m| !m.bought));
        assert_eq!(f.mods.len(), WEAPON_MODS.len());
    }

    #[test]
    fn an_unchanged_request_changes_nothing() {
        let mut f = oozla();
        let same = fields(&f);
        apply(&mut f, &same).unwrap();
        assert_eq!(f.to_bytes(), OOZLA);
    }

    #[test]
    fn the_level_lands_in_chunk_0() {
        let mut want = fields(&oozla());
        want.level = 3;
        let back = round_trip(&want);
        assert_eq!(global(&back, id::LEVEL), 3u32.to_le_bytes());
        let mut bad = fields(&oozla());
        bad.level = 30;
        assert!(apply(&mut oozla(), &bad).unwrap_err().contains("Museum"), "the port crashed loading it");
        bad.level = 99;
        assert!(apply(&mut oozla(), &bad).is_err());
    }

    #[test]
    fn crystals_and_moonstones_land_where_the_game_keeps_them() {
        let mut want = fields(&oozla());
        want.crystals = 25;
        want.moonstones = 13;
        let back = round_trip(&want);
        let p = global(&back, id::PROGRESS);
        // The pause-menu save after writing them in a running game (e1, format document).
        assert_eq!(&p[0x22..0x28], &[25, 0, 13, 0, 0, 0]);
        assert_eq!(p[0x20], 0, "the armour byte beside them is kept");
        want.crystals = MAX_GEMS;
        want.moonstones = MAX_GEMS;
        round_trip(&want);
        want.crystals = MAX_GEMS + 1;
        assert!(apply(&mut oozla(), &want).is_err());
        want.crystals = 0;
        want.moonstones = MAX_GEMS + 1;
        assert!(apply(&mut oozla(), &want).is_err());
    }

    #[test]
    fn platinum_bolts_spent_stay_under_those_found() {
        let mut file = oozla();
        assert_eq!(platinum_found(&file), 0);
        let mut want = fields(&file);
        want.platinum_spent = 1;
        assert!(apply(&mut file, &want).unwrap_err().contains("platinum"));
        // Two found on Oozla (the main editor's field), then two spent.
        let mut main = file.fields();
        main.planets[1].platinum[0] = true;
        main.planets[1].platinum[1] = true;
        file.apply(&main).unwrap();
        assert_eq!(platinum_found(&file), 2);
        want.platinum_spent = 2;
        apply(&mut file, &want).unwrap();
        let back = SaveFile::parse(&file.to_bytes()).unwrap();
        assert_eq!(global(&back, id::FLAGS)[0x90], 2);
        assert_eq!(fields(&back).platinum_spent, 2);
        want.platinum_spent = 3;
        assert!(apply(&mut file, &want).is_err());
    }

    #[test]
    fn ship_parts_set_only_their_bits() {
        let mut file = oozla();
        // Paint 5 (the main editor's bits) stays as it is.
        let mut main = file.fields();
        main.ship_paint = 5;
        file.apply(&main).unwrap();
        let mut want = fields(&file);
        // Every part at its last choice: the Ship Shack's "all parts" word 0xBB50 plus the nose
        // and wings, which a hidden run loaded (ls1).
        want.ship = SHIP_PARTS.iter().map(|d| Some(d.choices.len() as u32 - 1)).collect();
        apply(&mut file, &want).unwrap();
        let back = SaveFile::parse(&file.to_bytes()).unwrap();
        assert_eq!(word(&global(&back, id::SHIP), 0), 0x5_0000 | 0xBB59);
        assert_eq!(back.fields().ship_paint, 5);
        assert_eq!(fields(&back).ship, want.ship);
        // Back to the ship's own.
        want.ship = vec![Some(0); SHIP_PARTS.len()];
        apply(&mut file, &want).unwrap();
        assert_eq!(word(&global(&file, id::SHIP), 0), 0x5_0000);
        // A choice past the list, or a part list of the wrong length, is refused.
        want.ship[0] = Some(3);
        assert!(apply(&mut file, &want).is_err());
        want.ship = vec![Some(0); 2];
        assert!(apply(&mut file, &want).is_err());
    }

    #[test]
    fn a_ship_value_the_shack_never_sets_reads_as_unknown_and_is_kept() {
        let mut file = oozla();
        // Nose 3: the port crashed loading it (ls2), so it is shown as unknown, never written.
        set_word(chunk_mut(&mut file, id::SHIP), 0, 0x3 | 0x20);
        let f = fields(&file);
        assert_eq!(f.ship[0], None);
        assert_eq!(f.ship[2], Some(1), "the engine's second value is the engine");
        let mut want = f.clone();
        want.ship[3] = Some(1);
        apply(&mut file, &want).unwrap();
        assert_eq!(word(&global(&file, id::SHIP), 0), 0x3 | 0x20 | 0x40);
        want.ship[0] = Some(0);
        apply(&mut file, &want).unwrap();
        assert_eq!(word(&global(&file, id::SHIP), 0), 0x20 | 0x40);
    }

    #[test]
    fn mods_take_their_two_bits() {
        let mut want = fields(&oozla());
        let at = |item, slot| WEAPON_MODS.iter().position(|&(i, s, _)| i == item && s == slot).unwrap();
        want.mods[at(30, 0)].bought = true; // Lancer: Shock
        want.mods[at(30, 2)].bought = true; // Lancer: Lock On
        want.mods[at(22, 1)].bought = true; // Chopper: Acid
        let back = round_trip(&want);
        let m = global(&back, id::MODS);
        assert_eq!(m[30], 0x33);
        assert_eq!(m[22], 0x0C);
        assert_eq!(m.iter().filter(|&&b| b != 0).count(), 2);
        // Taking one away clears both its bits and leaves the other mod.
        let mut file = back;
        want.mods[at(30, 0)].bought = false;
        apply(&mut file, &want).unwrap();
        assert_eq!(global(&file, id::MODS)[30], 0x30);
        // The list must be the game's.
        want.mods.pop();
        assert!(apply(&mut file, &want).is_err());
    }

    #[test]
    fn the_tables_are_consistent() {
        for &(item, slot, price) in WEAPON_MODS {
            assert!(base::ITEMS.iter().any(|d| d.id == item && d.kind == base::Rac2ItemKind::Weapon));
            assert!(slot < 3 && price > 0);
        }
        for d in SHIP_PARTS {
            assert!(d.choices.iter().all(|&(_, v)| v & !d.mask == 0));
            assert_eq!(d.choices[0].1, 0);
            assert_eq!(d.mask & 0x1F_0000, 0, "the paint is the main editor's");
        }
        for l in RESUME_LEVELS {
            assert!(RAC2_PLANETS.iter().any(|&(id, _)| id == *l));
            assert!((*l as usize) < base::LEVEL_SLOTS);
        }
    }

    fn card() -> tempfile::TempDir {
        let dir = tempfile::tempdir().unwrap();
        let game = dir.path().join("BASCUS-97268RATCHET");
        fs::create_dir_all(&game).unwrap();
        fs::write(game.join("save0.bin"), OOZLA).unwrap();
        dir
    }

    #[test]
    fn a_write_backs_up_and_is_refused_when_not_safe() {
        let card = card();
        let v = read_save(card.path(), "SCUS_972.68", 0).unwrap();
        assert_eq!(v.levels.iter().find(|l| l.id == 1).unwrap().name, "Oozla");
        assert_eq!(v.ship.len(), SHIP_PARTS.len());
        let mut want = v.fields.clone();
        want.moonstones = 40;
        want.level = 19;
        let running = write_save(card.path(), "SCUS_972.68", 0, &v.fingerprint, &want, || Ok(Some("The game".into())));
        assert!(running.unwrap_err().contains("running"));
        let stale = write_save(card.path(), "SCUS_972.68", 0, "0000", &want, || Ok(None));
        assert!(stale.unwrap_err().contains("changed"));
        assert_eq!(fs::read(card.path().join("BASCUS-97268RATCHET/save0.bin")).unwrap(), OOZLA);
        let r = write_save(card.path(), "SCUS_972.68", 0, &v.fingerprint, &want, || Ok(None)).unwrap();
        assert_eq!((r.view.fields.moonstones, r.view.fields.level), (40, 19));
        let kept = fs::read(Path::new(&r.backup.path).join("BASCUS-97268RATCHET/save0.bin")).unwrap();
        assert_eq!(kept, OOZLA);
        let again = base::read_save(card.path(), "SCUS_972.68", 0).unwrap();
        assert_eq!(again.planet_name.as_deref(), Some("Grelbin"), "the main editor sees the new level");
        assert!(read_save(card.path(), "SCES_509.16", 0).is_err());
        assert!(read_save(card.path(), "SCUS_972.68", 4).is_err());
    }
}
