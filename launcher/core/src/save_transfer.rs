// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Saves carried between devices: one game's saves exported to a single file, and imported again
//! on another computer or phone.
//!
//! The file is a `.zip` (an "OpenRAC saves file"), so any system can open it and look inside:
//!
//! ```text
//! openrac-saves.json            the manifest (SaveArchiveManifest)
//! BESCES-50916RATCHET/icon.sys  the card's save folders and files, as they sit on the card
//! BESCES-50916RATCHET/save0.bin
//! BESCES-50916RATCHET/save0.bin.mods
//! ```
//!
//! Entry names always use `/` and keep their case; a folder or file name with a separator, a
//! drive or `..` in it is refused, so an import writes only inside the game's own card. The
//! manifest names the disc's serial (`SCES_509.16`), which an import must match, the launcher's
//! version, the date, the slots with their planets and bolts (where the launcher reads them), and
//! every file with its size and SHA-256, checked before anything is written.
//!
//! An import backs the card up first (`saves::backup_card`), then either replaces the game's save
//! folders with the file's, or merges: only the chosen slots are written, every other slot on the
//! card is kept.
//!
//! The mods' records of a save that live in their own folders, not on the card (a mod's
//! `save_data` file, `mod_save_data.rs`: NG++'s weapon tiers, by the save's clock), travel in the
//! manifest (`modRecords`): an export takes each exported save's record from every installed mod
//! that describes one, and an import puts the imported saves' records back into those files, so a
//! save keeps its mods' state on another computer. The card's own files beside a save (`.mods`,
//! `.moddata`) are files of the folder and go with it as they are.
//!
//! A copy between discs works the same way, from another disc's card instead of a file: the discs
//! of a game whose saves are the same file (Ratchet & Clank's PAL, US and Japanese discs, Going
//! Commando's PAL and US: `save_edit.rs`, `save_edit_rac2.rs`) each have their own card, so a
//! game built from one disc does not see the saves of the other. The copy takes the other card's
//! save folder, renamed to this disc's (`BESCES-50916RATCHET` becomes `BASCUS-97199RATCHET`, the
//! folder's own reserved file too), and writes it as an import would.

use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::fs;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

use crate::mod_save_data::{CarriedRecord, RecordFiles};
use crate::pcsx2_card::ImportedSaves;
use crate::saves::{self, SaveMod};

/// The manifest's name inside the file.
pub const MANIFEST: &str = "openrac-saves.json";
/// The manifest's `format`.
pub const FORMAT: &str = "openrac-saves";
/// The newest `formatVersion` this launcher reads and the one it writes.
pub const FORMAT_VERSION: u32 = 1;
/// No save file comes near this: a larger entry is not one.
const MAX_FILE: u64 = 16 << 20;
/// Nor does a whole card.
const MAX_TOTAL: u64 = 64 << 20;

/// One slot in an exported file (and on the card, for an import's comparison).
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveArchiveSlot {
    /// The save folder it is in (`BESCES-50916RATCHET`).
    pub folder: String,
    pub slot_index: usize,
    /// `save0.bin` ...
    pub file: String,
    #[serde(default)]
    pub empty: bool,
    #[serde(default)]
    pub planet_id: Option<u32>,
    #[serde(default)]
    pub planet_name: Option<String>,
    #[serde(default)]
    pub bolts: Option<u32>,
    /// The game's clock when it saved, as the Saves page shows it (`2026-10-10 23:14:14`).
    #[serde(default)]
    pub timestamp: Option<String>,
    /// The mods it was played with (the `.mods` mark beside it).
    #[serde(default)]
    pub mods: Option<Vec<SaveMod>>,
}

/// One file in an exported file: its path (`/`-separated), size and SHA-256.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveArchiveFile {
    pub path: String,
    pub size: u64,
    pub sha256: String,
}

/// `openrac-saves.json`.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveArchiveManifest {
    /// Always `openrac-saves`.
    pub format: String,
    pub format_version: u32,
    /// The disc's serial whose card the saves are from (`SCES_509.16`).
    pub serial: String,
    /// The game's name from its `icon.sys`, when there is one.
    #[serde(default)]
    pub title: Option<String>,
    pub launcher_version: String,
    /// When it was exported, in UTC (`2026-11-02T14:05:00Z`).
    pub created: String,
    /// Every slot in the file; the empty ones too, marked `empty`.
    pub slots: Vec<SaveArchiveSlot>,
    pub files: Vec<SaveArchiveFile>,
    /// The mods' records of the saves, kept in the mods' own folders (`mod_save_data.rs`).
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    #[ts(as = "Option<Vec<SaveModRecord>>", optional)]
    pub mod_records: Vec<SaveModRecord>,
}

/// One mod's record of one save in an exported file: the save's slot, the mod and its file, the
/// save's clock and the record's data, both hex as the mod's file holds them.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveModRecord {
    pub folder: String,
    pub slot_index: usize,
    /// The mod's id (`ng-plus-plus`).
    pub id: String,
    /// Its file in the mod's folder (`tiers.txt`).
    pub file: String,
    /// The save's clock (record 4), 16 hex digits.
    pub clock: String,
    pub data: String,
}

/// The clock a save was stamped with (its record 4, the 8 bytes the game writes when it saves):
/// the key the mods' records go by. None for a file with no such record.
fn save_clock(bytes: &[u8]) -> Option<[u8; 8]> {
    let mut at = 16; // the sizes and the checksum before the records
    while at + 8 <= bytes.len() {
        let id = i32::from_le_bytes(bytes[at..at + 4].try_into().ok()?);
        let size = i32::from_le_bytes(bytes[at + 4..at + 8].try_into().ok()?);
        if id == -1 || size < 0 {
            return None;
        }
        let (start, end) = (at + 8, at + 8 + size as usize);
        if end > bytes.len() {
            return None;
        }
        if id == 4 {
            return bytes.get(start..start + 8)?.try_into().ok();
        }
        at = start + ((size as usize + 3) & !3);
    }
    None
}

fn clock_hex(clock: &[u8; 8]) -> String {
    clock.iter().map(|b| format!("{b:02x}")).collect()
}

fn clock_of_hex(text: &str) -> Option<[u8; 8]> {
    if text.len() != 16 {
        return None;
    }
    let mut out = [0u8; 8];
    for (k, b) in out.iter_mut().enumerate() {
        *b = u8::from_str_radix(text.get(2 * k..2 * k + 2)?, 16).ok()?;
    }
    Some(out)
}

/// The mods' records of the used slots of `files`, from `records`.
fn records_of_slots(
    files: &BTreeMap<String, Vec<u8>>,
    slots: &[SaveArchiveSlot],
    records: &RecordFiles,
) -> Vec<SaveModRecord> {
    let mut out = Vec::new();
    for slot in slots.iter().filter(|s| !s.empty) {
        let Some(clock) = files.get(&format!("{}/{}", slot.folder, slot.file)).and_then(|b| save_clock(b)) else {
            continue;
        };
        for r in records.records_of(&clock) {
            out.push(SaveModRecord {
                folder: slot.folder.clone(),
                slot_index: slot.slot_index,
                id: r.id,
                file: r.file,
                clock: clock_hex(&clock),
                data: r.data,
            });
        }
    }
    out
}

/// One slot of a file to import, with the slot it would replace on the card.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveImportSlot {
    pub slot: SaveArchiveSlot,
    /// The card's own save in that slot, when it holds one: what importing this slot replaces.
    pub replaces: Option<SaveArchiveSlot>,
}

/// What a file to import holds, shown before anything is written.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveImportPreview {
    pub serial: String,
    pub title: Option<String>,
    pub launcher_version: String,
    pub created: String,
    /// The used slots in the file.
    pub slots: Vec<SaveImportSlot>,
    /// Whether the card has saves of its own (they are backed up before the import).
    pub card_has_saves: bool,
}

/// A slot by its folder and number.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveSlotRef {
    pub folder: String,
    pub slot_index: usize,
}

/// How to import: the card's save folders replaced by the file's, or only the chosen slots
/// written (merge), every other slot on the card kept.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveImportChoice {
    pub replace_all: bool,
    /// Merge only: the file's slots to write.
    #[serde(default)]
    pub slots: Vec<SaveSlotRef>,
}

/// An export, for the page to say: where it went (empty when the player backed out of saving it)
/// and the used slots in it.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ExportedSaves {
    pub saved: String,
    pub slots: Vec<SaveArchiveSlot>,
}

/// An OpenRAC saves file, read and checked: its manifest and its files by path.
#[derive(Debug, Clone)]
pub struct SaveArchive {
    pub manifest: SaveArchiveManifest,
    pub files: BTreeMap<String, Vec<u8>>,
}

/// The slot a file name is (`save3.bin` is 3), or None for the folder's other files.
fn slot_of(name: &str) -> Option<usize> {
    name.strip_prefix("save")?.strip_suffix(".bin")?.parse().ok().filter(|_| {
        // `save01.bin` or `save+1.bin` are not the game's names.
        let digits = &name[4..name.len() - 4];
        !digits.is_empty() && digits.chars().all(|c| c.is_ascii_digit()) && (digits == "0" || !digits.starts_with('0'))
    })
}

/// What the computer keeps beside a save: the port's mark of the mods it was played with
/// (`.mods`) and the mods' own data for it (`.moddata`, port/game/common/mods.cpp).
const COMPANIONS: [&str; 2] = [".mods", ".moddata"];

/// The slot a file beside a save belongs to (`save3.bin.mods`, `save3.bin.moddata`: 3).
fn companion_of(name: &str) -> Option<usize> {
    COMPANIONS.iter().find_map(|end| slot_of(name.strip_suffix(end)?))
}

/// The slot a file of a save folder goes with: its save, or a file beside it. None for the
/// folder's own files (its icon, its reserved file).
fn slot_file(name: &str) -> Option<usize> {
    slot_of(name).or_else(|| companion_of(name))
}

/// Whether `name` is one file or folder name on every system: not empty, not `.` or `..`, no
/// separator, drive, or character Windows refuses, and not ending in a dot or a space.
fn plain_name(name: &str) -> bool {
    !name.is_empty()
        && name != "."
        && name != ".."
        && !name.ends_with(['.', ' '])
        && !name.chars().any(|c| c.is_control() || r#"/\:*?"<>|"#.contains(c))
}

/// An entry path `folder/file`, split; None for anything else (a path that would leave the card).
fn split_entry(path: &str) -> Option<(&str, &str)> {
    let (folder, file) = path.split_once('/')?;
    (plain_name(folder) && plain_name(file) && !folder.starts_with('.') && !file.starts_with('.'))
        .then_some((folder, file))
}

/// `YYYY-MM-DDTHH:MM:SSZ` for seconds since 1970 (Howard Hinnant's civil_from_days).
fn utc_stamp(secs: u64) -> String {
    let days = (secs / 86_400) as i64;
    let rem = secs % 86_400;
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + i64::from(m <= 2);
    format!("{y:04}-{m:02}-{d:02}T{:02}:{:02}:{:02}Z", rem / 3600, rem % 3600 / 60, rem % 60)
}

fn now_secs() -> u64 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0)
}

/// The name an export is offered under: `OpenRAC saves SCES_509.16 2026-11-02.zip`, with the
/// slot when it is one (`... slot 2 ...`).
pub fn export_name(serial: &str, slot: Option<usize>) -> String {
    let day = &utc_stamp(now_secs())[..10];
    let serial: String = serial.chars().filter(|c| c.is_ascii_alphanumeric() || "._-".contains(*c)).collect();
    match slot {
        Some(i) => format!("OpenRAC saves {serial} slot {} {day}.zip", i + 1),
        None => format!("OpenRAC saves {serial} {day}.zip"),
    }
}

/// The slot `index` of `folder` from its file and mark.
fn describe(serial: &str, folder: &str, index: usize, save: &[u8], mark: Option<&[u8]>) -> SaveArchiveSlot {
    let s = saves::describe_save(serial, save);
    let mods = if s.empty {
        None
    } else {
        mark.and_then(|m| serde_json::from_slice::<serde_json::Value>(m).ok())
            .and_then(|v| serde_json::from_value::<Vec<SaveMod>>(v.get("mods")?.clone()).ok())
            .filter(|m| !m.is_empty())
    };
    SaveArchiveSlot {
        folder: folder.to_string(),
        slot_index: index,
        file: format!("save{index}.bin"),
        empty: s.empty,
        planet_id: s.planet_id,
        planet_name: s.planet_name,
        bolts: s.bolts,
        timestamp: s.timestamp,
        mods,
    }
}

/// Every slot in a set of files by path, in folder and slot order.
fn slots_in(serial: &str, files: &BTreeMap<String, Vec<u8>>) -> Vec<SaveArchiveSlot> {
    let mut out: Vec<SaveArchiveSlot> = files
        .iter()
        .filter_map(|(path, bytes)| {
            let (folder, file) = split_entry(path)?;
            let index = slot_of(file)?;
            let mark = files.get(&format!("{path}.mods")).map(Vec::as_slice);
            Some(describe(serial, folder, index, bytes, mark))
        })
        .collect();
    out.sort_by(|a, b| (a.folder.as_str(), a.slot_index).cmp(&(b.folder.as_str(), b.slot_index)));
    out
}

/// The card's save folders (every folder but the backups and hidden ones), sorted.
fn card_folders(card_dir: &Path) -> Vec<String> {
    let mut out: Vec<String> = fs::read_dir(card_dir)
        .into_iter()
        .flatten()
        .flatten()
        .filter(|e| e.path().is_dir())
        .map(|e| e.file_name().to_string_lossy().to_string())
        .filter(|n| !n.starts_with('.'))
        .collect();
    out.sort();
    out
}

/// The files of a save folder on the card, by `folder/file` (hidden ones and subfolders left out).
fn folder_files(card_dir: &Path, folder: &str) -> Result<BTreeMap<String, Vec<u8>>, String> {
    let dir = card_dir.join(folder);
    let mut out = BTreeMap::new();
    for entry in fs::read_dir(&dir).map_err(|e| format!("{}: {e}", dir.display()))?.flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        if name.starts_with('.') || !entry.path().is_file() {
            continue;
        }
        if !plain_name(&name) {
            return Err(format!("{name} in {folder} cannot be carried to every system"));
        }
        out.insert(format!("{folder}/{name}"), fs::read(entry.path()).map_err(|e| format!("{name}: {e}"))?);
    }
    Ok(out)
}

/// Exports the saves on the card in `card_dir` to `dest`: all of them, or one slot (`slot`) of the
/// game's folder with the folder's other files (its icon, its settings), so it imports onto an
/// empty card too. Written beside `dest` first and moved into place, so a failed export leaves no
/// half a file. Returns the manifest written.
pub fn export_card(
    card_dir: &Path,
    serial: &str,
    slot: Option<usize>,
    launcher_version: &str,
    dest: &Path,
    records: &RecordFiles,
) -> Result<SaveArchiveManifest, String> {
    let mut files = BTreeMap::new();
    let folders = match slot {
        None => card_folders(card_dir),
        Some(_) => saves::find_game_folder(card_dir)
            .and_then(|p| p.file_name().map(|n| n.to_string_lossy().to_string()))
            .into_iter()
            .collect(),
    };
    if folders.is_empty() {
        return Err("there are no saves on this game's memory card yet".into());
    }
    for folder in &folders {
        if !plain_name(folder) {
            return Err(format!("the save folder {folder} cannot be carried to every system"));
        }
        files.extend(folder_files(card_dir, folder)?);
    }
    if let Some(index) = slot {
        // The one slot and the files beside it, and the folder's own files: no other slot.
        files.retain(|path, _| {
            let file = path.split_once('/').map_or("", |(_, f)| f);
            slot_file(file).is_none_or(|i| i == index)
        });
        let used = slots_in(serial, &files).iter().any(|s| s.slot_index == index && !s.empty);
        if !used {
            return Err(format!("slot {} holds no save", index + 1));
        }
    }

    let title = files.iter().find(|(p, _)| p.ends_with("/icon.sys")).and_then(|(_, b)| saves::icon_title(b));
    let slots = slots_in(serial, &files);
    let mod_records = records_of_slots(&files, &slots, records);
    let manifest = SaveArchiveManifest {
        format: FORMAT.into(),
        format_version: FORMAT_VERSION,
        serial: serial.to_string(),
        title,
        launcher_version: launcher_version.to_string(),
        created: utc_stamp(now_secs()),
        slots,
        mod_records,
        files: files
            .iter()
            .map(|(path, bytes)| SaveArchiveFile {
                path: path.clone(),
                size: bytes.len() as u64,
                sha256: crate::sha256_hex(bytes),
            })
            .collect(),
    };

    let json = serde_json::to_vec_pretty(&manifest).map_err(|e| e.to_string())?;
    let partial = PathBuf::from(format!("{}.partial", dest.to_string_lossy()));
    let written = (|| -> Result<(), String> {
        let out = fs::File::create(&partial).map_err(|e| format!("{}: {e}", partial.display()))?;
        let mut zip = zip::ZipWriter::new(out);
        let options = zip::write::SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
        zip.start_file(MANIFEST, options).map_err(|e| e.to_string())?;
        zip.write_all(&json).map_err(|e| e.to_string())?;
        for (path, bytes) in &files {
            zip.start_file(path.as_str(), options).map_err(|e| e.to_string())?;
            zip.write_all(bytes).map_err(|e| e.to_string())?;
        }
        zip.finish().map_err(|e| e.to_string())?.sync_all().map_err(|e| e.to_string())?;
        fs::rename(&partial, dest).map_err(|e| format!("{}: {e}", dest.display()))
    })();
    if written.is_err() {
        let _ = fs::remove_file(&partial);
    }
    written.map(|()| manifest)
}

/// Reads and checks an OpenRAC saves file: its manifest, then every file it names, by size and
/// SHA-256. Nothing in a file that fails is used.
pub fn read_archive(path: &Path) -> Result<SaveArchive, String> {
    const NOT_OURS: &str = "is not an OpenRAC saves file, or it is damaged";
    let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
    let file = fs::File::open(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let mut zip = zip::ZipArchive::new(file).map_err(|_| format!("{name} {NOT_OURS}"))?;

    let mut read_entry = |entry: &str, limit: u64| -> Result<Vec<u8>, String> {
        let f = zip.by_name(entry).map_err(|_| format!("{name} {NOT_OURS} ({entry} is missing)"))?;
        if f.size() > limit {
            return Err(format!("{name} {NOT_OURS} ({entry} is too large)"));
        }
        let mut bytes = Vec::with_capacity(f.size() as usize);
        f.take(limit + 1).read_to_end(&mut bytes).map_err(|_| format!("{name} {NOT_OURS} ({entry} cannot be read)"))?;
        if bytes.len() as u64 > limit {
            return Err(format!("{name} {NOT_OURS} ({entry} is too large)"));
        }
        Ok(bytes)
    };

    let manifest: SaveArchiveManifest = serde_json::from_slice(&read_entry(MANIFEST, 1 << 20)?)
        .map_err(|e| format!("{name} {NOT_OURS} (its manifest: {e})"))?;
    if manifest.format != FORMAT {
        return Err(format!("{name} {NOT_OURS}"));
    }
    if manifest.format_version > FORMAT_VERSION {
        return Err(format!("{name} was made by a newer launcher: update this one to import it"));
    }

    let mut files = BTreeMap::new();
    let mut total = 0u64;
    for f in &manifest.files {
        if split_entry(&f.path).is_none() {
            return Err(format!("{name} names a file outside a save folder ({}): it is not imported", f.path));
        }
        total += f.size;
        if f.size > MAX_FILE || total > MAX_TOTAL {
            return Err(format!("{name} {NOT_OURS} (it is far larger than a memory card)"));
        }
        let bytes = read_entry(&f.path, MAX_FILE)?;
        if bytes.len() as u64 != f.size || crate::sha256_hex(&bytes) != f.sha256.to_ascii_lowercase() {
            return Err(format!("{name} is damaged: {} is not the file that was exported", f.path));
        }
        if files.insert(f.path.clone(), bytes).is_some() {
            return Err(format!("{name} {NOT_OURS} ({} is in it twice)", f.path));
        }
    }
    Ok(SaveArchive { manifest, files })
}

/// The file's game against this one: an error naming both when they differ.
fn check_serial(archive: &SaveArchive, serial: &str) -> Result<(), String> {
    let theirs = &archive.manifest.serial;
    if theirs.eq_ignore_ascii_case(serial) {
        return Ok(());
    }
    let game = archive.manifest.title.as_deref().map(|t| format!("{t}, ")).unwrap_or_default();
    Err(format!(
        "These saves are from another game or disc ({game}{theirs}), not this one ({serial}): they can only be imported there."
    ))
}

/// The card's own folder with this name, whatever its case (a card copied from a system that
/// changed it).
fn card_folder_named(card_dir: &Path, name: &str) -> Option<String> {
    card_folders(card_dir).into_iter().find(|f| f.eq_ignore_ascii_case(name))
}

/// What importing the file at `source` onto the card in `card_dir` would do, before anything is
/// written: refused for another game's file.
pub fn preview_into(card_dir: &Path, serial: &str, source: &Path) -> Result<SaveImportPreview, String> {
    let archive = read_archive(source)?;
    check_serial(&archive, serial)?;
    let (slots, card_has_saves) = preview_files(card_dir, serial, &archive.files);
    let m = archive.manifest;
    Ok(SaveImportPreview {
        serial: m.serial,
        title: m.title,
        launcher_version: m.launcher_version,
        created: m.created,
        slots,
        card_has_saves,
    })
}

/// The used slots of `files` with the card's save each would replace, and whether the card in
/// `card_dir` has saves of its own.
fn preview_files(card_dir: &Path, serial: &str, files: &BTreeMap<String, Vec<u8>>) -> (Vec<SaveImportSlot>, bool) {
    let mut card = BTreeMap::new();
    for folder in card_folders(card_dir) {
        card.extend(folder_files(card_dir, &folder).unwrap_or_default());
    }
    let on_card = slots_in(serial, &card);
    let slots = slots_in(serial, files)
        .into_iter()
        .filter(|s| !s.empty)
        .map(|slot| {
            let replaces = on_card
                .iter()
                .find(|c| c.folder.eq_ignore_ascii_case(&slot.folder) && c.slot_index == slot.slot_index && !c.empty)
                .cloned();
            SaveImportSlot { slot, replaces }
        })
        .collect();
    (slots, on_card.iter().any(|s| !s.empty))
}

/// Writes `bytes` to `path` through a file beside it and a rename, so a save is never half written.
fn write_file(path: &Path, bytes: &[u8]) -> Result<(), String> {
    let tmp = PathBuf::from(format!("{}.importing", path.to_string_lossy()));
    fs::write(&tmp, bytes).map_err(|e| format!("{}: {e}", tmp.display()))?;
    fs::rename(&tmp, path).map_err(|e| {
        let _ = fs::remove_file(&tmp);
        format!("{}: {e}", path.display())
    })
}

/// Imports the saves file at `source` onto the card in `card_dir`, after a backup of the card:
/// refused for another game's file, a damaged one, or a merge that chose no slot.
pub fn import_into(
    card_dir: &Path,
    serial: &str,
    source: &Path,
    choice: &SaveImportChoice,
    records: &RecordFiles,
) -> Result<ImportedSaves, String> {
    let archive = read_archive(source)?;
    check_serial(&archive, serial)?;
    let done = write_files(card_dir, serial, &archive.files, choice, "before import")?;
    // The mods' records of the saves written, back into the mods' files (a mod that reads none
    // here still finds its record when it is installed).
    for r in &archive.manifest.mod_records {
        let picked = choice.replace_all
            || choice.slots.iter().any(|p| p.folder.eq_ignore_ascii_case(&r.folder) && p.slot_index == r.slot_index);
        let Some(clock) = clock_of_hex(&r.clock).filter(|_| picked) else { continue };
        records.put(&clock, &CarriedRecord { id: r.id.clone(), file: r.file.clone(), data: r.data.clone() })?;
    }
    Ok(done)
}

/// Writes save folders' files (by `folder/file`) onto the card in `card_dir` as `choice` says,
/// after a backup of the card noted `note`: refused for a merge that chose no slot, or a slot
/// `files` does not hold.
fn write_files(
    card_dir: &Path,
    serial: &str,
    files: &BTreeMap<String, Vec<u8>>,
    choice: &SaveImportChoice,
    note: &str,
) -> Result<ImportedSaves, String> {
    let in_file: Vec<SaveArchiveSlot> = slots_in(serial, files);
    if !choice.replace_all {
        if choice.slots.is_empty() {
            return Err("choose at least one slot to import".into());
        }
        for pick in &choice.slots {
            if !in_file.iter().any(|s| s.folder == pick.folder && s.slot_index == pick.slot_index && !s.empty) {
                return Err(format!("the file has no save in slot {}", pick.slot_index + 1));
            }
        }
    }

    fs::create_dir_all(card_dir).map_err(|e| format!("{}: {e}", card_dir.display()))?;
    // Anything on the card is backed up first, a card of co-op saves alone included.
    let holds_saves = fs::read_dir(card_dir)
        .map(|entries| {
            entries.flatten().any(|e| e.path().is_dir() && !e.file_name().to_string_lossy().starts_with('.'))
        })
        .unwrap_or(false);
    let backup = if holds_saves { Some(saves::backup_card(card_dir, Some(note))?) } else { None };

    // The file's folders, each with its files.
    let mut by_folder: BTreeMap<&str, Vec<(&str, &[u8])>> = BTreeMap::new();
    for (path, bytes) in files {
        if let Some((folder, file)) = split_entry(path) {
            by_folder.entry(folder).or_default().push((file, bytes));
        }
    }

    let mut folders = Vec::new();
    let mut written = 0u32;
    for (folder, entries) in by_folder {
        let picked = |i: usize| choice.slots.iter().any(|p| p.folder == folder && p.slot_index == i);
        if !choice.replace_all && !entries.iter().any(|(f, _)| slot_of(f).is_some_and(picked)) {
            continue;
        }
        // The card's folder takes the file's name, case and all.
        let dest = card_dir.join(folder);
        if let Some(existing) = card_folder_named(card_dir, folder).filter(|e| e != folder) {
            let step = card_dir.join(format!(".renaming_{folder}"));
            fs::rename(card_dir.join(&existing), &step).map_err(|e| e.to_string())?;
            fs::rename(&step, &dest).map_err(|e| e.to_string())?;
        }
        if choice.replace_all && dest.exists() {
            fs::remove_dir_all(&dest).map_err(|e| format!("{}: {e}", dest.display()))?;
        }
        fs::create_dir_all(&dest).map_err(|e| format!("{}: {e}", dest.display()))?;
        for (file, bytes) in &entries {
            let target = dest.join(file);
            let take = choice.replace_all
                || match slot_file(file) {
                    Some(i) => picked(i),
                    // The folder's own files (icon, settings): only where the card has none.
                    None => !target.exists(),
                };
            if take {
                write_file(&target, bytes)?;
                written += 1;
            }
        }
        if !choice.replace_all {
            // A chosen slot without a mark in the file was played without mods, and one without
            // mod data has none: the card's old files beside it would belong to the save replaced.
            for pick in choice.slots.iter().filter(|p| p.folder == folder) {
                for end in COMPANIONS {
                    let beside = format!("save{}.bin{end}", pick.slot_index);
                    if !entries.iter().any(|(f, _)| *f == beside) {
                        let _ = fs::remove_file(dest.join(&beside));
                    }
                }
            }
        }
        written += fill_empty_slots(serial, folder, &dest)?;
        folders.push(folder.to_string());
    }
    Ok(ImportedSaves { folders, files: written, backup })
}

/// Going Commando opens its save folder only when it lists its four slot files (`func_00298F00`,
/// state 8, counts nine entries), so a one-slot file imported where the folder had no others
/// would read as no save at all. Each slot file missing from the game's own folder is written as
/// an empty slot, made from one that is there; returns how many.
fn fill_empty_slots(serial: &str, folder: &str, dest: &Path) -> Result<u32, String> {
    if !crate::save_edit_rac2::edits_serial(serial)
        || !card_folder_name(serial).is_some_and(|f| f.eq_ignore_ascii_case(folder))
    {
        return Ok(0);
    }
    let slot = |i: usize| dest.join(format!("save{i}.bin"));
    let Some(template) = (0..4).map(slot).find(|p| p.is_file()) else {
        return Ok(0);
    };
    let bytes = fs::read(&template).map_err(|e| format!("{}: {e}", template.display()))?;
    // A save the editor cannot read is left as the file had it: the import itself is done.
    let Ok(empty) = crate::save_edit_rac2::empty_slot(&bytes) else {
        return Ok(0);
    };
    let mut made = 0;
    for target in (0..4).map(slot).filter(|p| !p.exists()) {
        write_file(&target, &empty)?;
        made += 1;
    }
    Ok(made)
}

// ---------------------------------------------------------------------------------------------
// Copies between the discs of one game.

/// The discs whose games write the same save file, a game a line, each with the name a player
/// knows it by. Ratchet & Clank: every disc's descriptor tables list the same chunks
/// (`save_edit.rs`); Going Commando: the US discs (v1.01 and v2.00 share `SCUS_972.68`, and so
/// a card) and the PAL disc (`save_edit_rac2.rs`).
const SAME_SAVES: &[&[(&str, &str)]] = &[
    &[("SCES_509.16", "PAL"), ("SCUS_971.99", "NTSC-U"), ("SCPS_150.37", "NTSC-J")],
    &[("SCUS_972.68", "NTSC-U"), ("SCES_516.07", "PAL")],
];

/// Another disc of the same game, as the Saves panel offers it to copy from.
#[derive(Debug, Clone, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SaveCopySource {
    /// Its serial (`SCES_509.16`).
    pub serial: String,
    /// Its region as players name it (`PAL`, `NTSC-U`, `NTSC-J`).
    pub region: String,
    /// How many saves its card holds.
    pub saves: usize,
}

/// The other discs whose saves `serial`'s game reads as its own, with their regions.
fn same_save_discs(serial: &str) -> Vec<(&'static str, &'static str)> {
    SAME_SAVES
        .iter()
        .find(|game| game.iter().any(|(s, _)| s.eq_ignore_ascii_case(serial)))
        .map(|game| game.iter().copied().filter(|(s, _)| !s.eq_ignore_ascii_case(serial)).collect())
        .unwrap_or_default()
}

/// The save folder a disc's game makes on its card (func_00298BB8 in Going Commando, the same
/// template in Ratchet & Clank): `B`, the region (`E` for Europe, `I` for Japan, `A` otherwise),
/// the serial's letters, a dash, its digits and `RATCHET`: `SCUS_971.99` is
/// `BASCUS-97199RATCHET`.
pub fn card_folder_name(serial: &str) -> Option<String> {
    let (letters, number) = serial.split_once('_')?;
    let digits: String = number.chars().filter(char::is_ascii_digit).collect();
    if letters.len() != 4 || !letters.chars().all(|c| c.is_ascii_uppercase()) || digits.len() != 5 {
        return None;
    }
    let region = match letters.as_bytes()[2] {
        b'E' => 'E',
        b'P' => 'I',
        _ => 'A',
    };
    Some(format!("B{region}{letters}-{digits}RATCHET"))
}

/// The game's folder name on a card, when it has one.
fn game_folder(card_dir: &Path) -> Option<String> {
    saves::find_game_folder(card_dir).and_then(|p| p.file_name().map(|n| n.to_string_lossy().to_string()))
}

/// The save folder of the card in `from_card`, by `folder/file` under the name the card in
/// `to_card` uses (its own folder's, or the one `to_serial`'s game would make), its reserved file
/// (named as the folder) renamed with it. Also whether `to_card` has that folder already.
fn carried_files(
    from_card: &Path,
    to_card: &Path,
    to_serial: &str,
) -> Result<(BTreeMap<String, Vec<u8>>, bool), String> {
    let from = game_folder(from_card).ok_or("the other disc's memory card holds no saves")?;
    let theirs = game_folder(to_card);
    let to = match &theirs {
        Some(name) => name.clone(),
        None => card_folder_name(to_serial).ok_or(format!("{to_serial} has no known save folder"))?,
    };
    let mut out = BTreeMap::new();
    for (path, bytes) in folder_files(from_card, &from)? {
        let file = path.split_once('/').map_or("", |(_, f)| f);
        let file = if file.eq_ignore_ascii_case(&from) { to.as_str() } else { file };
        out.insert(format!("{to}/{file}"), bytes);
    }
    Ok((out, theirs.is_some()))
}

/// The pair's check: two discs of one game whose saves are the same file.
fn check_pair(from_serial: &str, to_serial: &str) -> Result<(), String> {
    if same_save_discs(to_serial).iter().any(|(s, _)| s.eq_ignore_ascii_case(from_serial)) {
        Ok(())
    } else {
        Err(format!("saves from {from_serial} cannot be copied to {to_serial}: they are not the same game's"))
    }
}

/// The other discs of the game whose cards hold saves, for the game on `serial`; `card_of` gives
/// a serial's card.
pub fn copy_sources_with(serial: &str, card_of: impl Fn(&str) -> PathBuf) -> Vec<SaveCopySource> {
    same_save_discs(serial)
        .into_iter()
        .filter_map(|(other, region)| {
            let card = card_of(other);
            let folder = game_folder(&card)?;
            let files = folder_files(&card, &folder).ok()?;
            let saves = slots_in(serial, &files).iter().filter(|s| !s.empty).count();
            (saves > 0).then(|| SaveCopySource { serial: other.to_string(), region: region.to_string(), saves })
        })
        .collect()
}

/// What copying the saves on `from_serial`'s card (in `from_card`) onto `to_serial`'s (in
/// `to_card`) would do, before anything is written: the slots, under this card's folder name, with
/// what each replaces. `serial` is the other disc's, `created` now; a card without the game's
/// folder yet has no saves of its own, and takes the other card's folder whole.
pub fn preview_copy_between(
    from_card: &Path,
    from_serial: &str,
    to_card: &Path,
    to_serial: &str,
) -> Result<SaveImportPreview, String> {
    check_pair(from_serial, to_serial)?;
    let (files, _) = carried_files(from_card, to_card, to_serial)?;
    let (slots, card_has_saves) = preview_files(to_card, to_serial, &files);
    let title = files.iter().find(|(p, _)| p.ends_with("/icon.sys")).and_then(|(_, b)| saves::icon_title(b));
    Ok(SaveImportPreview {
        serial: from_serial.to_string(),
        title,
        launcher_version: String::new(),
        created: utc_stamp(now_secs()),
        slots,
        card_has_saves,
    })
}

/// Copies the saves on `from_serial`'s card onto `to_serial`'s, after a backup of this card, as
/// an import would: every save replaced, or the chosen slots merged. A card without the game's
/// folder yet takes the other card's whole: Going Commando counts its folder's files and takes a
/// folder with fewer for no save, then offers to write every slot empty.
pub fn copy_between(
    from_card: &Path,
    from_serial: &str,
    to_card: &Path,
    to_serial: &str,
    choice: &SaveImportChoice,
) -> Result<ImportedSaves, String> {
    check_pair(from_serial, to_serial)?;
    let (files, has_folder) = carried_files(from_card, to_card, to_serial)?;
    let whole = SaveImportChoice { replace_all: true, slots: Vec::new() };
    let choice = if has_folder { choice } else { &whole };
    let region = same_save_discs(to_serial)
        .into_iter()
        .find(|(s, _)| s.eq_ignore_ascii_case(from_serial))
        .map_or(from_serial, |(_, r)| r);
    write_files(to_card, to_serial, &files, choice, &format!("before copy from {region}"))
}

/// [`copy_sources_with`] the cards of the launcher's save folder.
pub fn copy_sources(serial: &str) -> Vec<SaveCopySource> {
    copy_sources_with(serial, saves::get_memcard_dir)
}

/// [`preview_copy_between`] the games' own cards.
pub fn preview_copy(from_serial: &str, to_serial: &str) -> Result<SaveImportPreview, String> {
    preview_copy_between(
        &saves::get_memcard_dir(from_serial),
        from_serial,
        &saves::get_memcard_dir(to_serial),
        to_serial,
    )
}

/// [`copy_between`] the games' own cards.
pub fn copy_saves(from_serial: &str, to_serial: &str, choice: &SaveImportChoice) -> Result<ImportedSaves, String> {
    copy_between(
        &saves::get_memcard_dir(from_serial),
        from_serial,
        &saves::get_memcard_dir(to_serial),
        to_serial,
        choice,
    )
}

/// [`export_card`] from the game's own card.
pub fn export_saves(
    serial: &str,
    slot: Option<usize>,
    launcher_version: &str,
    dest: &Path,
    records: &RecordFiles,
) -> Result<SaveArchiveManifest, String> {
    export_card(&saves::get_memcard_dir(serial), serial, slot, launcher_version, dest, records)
}

/// [`preview_into`] the game's own card.
pub fn preview_import(serial: &str, source: &Path) -> Result<SaveImportPreview, String> {
    preview_into(&saves::get_memcard_dir(serial), serial, source)
}

/// [`import_into`] the game's own card.
pub fn import_saves(
    serial: &str,
    source: &Path,
    choice: &SaveImportChoice,
    records: &RecordFiles,
) -> Result<ImportedSaves, String> {
    import_into(&saves::get_memcard_dir(serial), serial, source, choice, records)
}

#[cfg(test)]
mod tests {
    use super::*;

    const PAL: &str = "SCES_509.16";
    const FOLDER: &str = "BESCES-50916RATCHET";

    /// A save as the game writes it: planet in record 0, bolts in record 1, the clock in record 4.
    fn save(planet: u32, bolts: u32) -> Vec<u8> {
        let mut out = vec![0u8; 16];
        let clock = [0x00u8, 0x14, 0x14, 0x23, 0x00, 0x10, 0x10, 0x26];
        for (id, data) in
            [(0i32, planet.to_le_bytes().to_vec()), (1, bolts.to_le_bytes().to_vec()), (4, clock.to_vec())]
        {
            out.extend_from_slice(&id.to_le_bytes());
            out.extend_from_slice(&(data.len() as i32).to_le_bytes());
            out.extend_from_slice(&data);
        }
        out.extend_from_slice(&(-1i32).to_le_bytes());
        out
    }

    /// A slot never saved to.
    fn unused() -> Vec<u8> {
        let mut out = vec![0u8; 16];
        out.extend_from_slice(&0i32.to_le_bytes());
        out.extend_from_slice(&4i32.to_le_bytes());
        out.extend_from_slice(&u32::MAX.to_le_bytes());
        out.extend_from_slice(&(-1i32).to_le_bytes());
        out
    }

    /// A card as the port leaves it: the folder's own files, two used slots, the rest unused, and
    /// a mod mark beside slot 1.
    fn card(dir: &Path) {
        let f = dir.join(FOLDER);
        fs::create_dir_all(&f).unwrap();
        fs::write(f.join("icon.sys"), b"icon").unwrap();
        fs::write(f.join(FOLDER), b"settings").unwrap();
        fs::write(f.join("save0.bin"), save(3, 1234)).unwrap();
        fs::write(f.join("save1.bin"), save(5, 99)).unwrap();
        fs::write(f.join("save1.bin.mods"), br#"{"format": 1, "mods": [{"id": "lock-strafe"}]}"#).unwrap();
        for i in 2..5 {
            fs::write(f.join(format!("save{i}.bin")), unused()).unwrap();
        }
        fs::write(f.join(".DS_Store"), b"finder").unwrap();
    }

    fn read(dir: &Path, file: &str) -> Vec<u8> {
        fs::read(dir.join(FOLDER).join(file)).unwrap()
    }

    #[test]
    fn slot_names_are_the_games() {
        assert_eq!(slot_of("save0.bin"), Some(0));
        assert_eq!(slot_of("save4.bin"), Some(4));
        assert_eq!(slot_of("save01.bin"), None);
        assert_eq!(slot_of("save.bin"), None);
        assert_eq!(slot_of("icon.sys"), None);
        assert_eq!(companion_of("save1.bin.mods"), Some(1));
        assert_eq!(companion_of("save1.bin"), None);
    }

    #[test]
    fn an_entry_cannot_leave_its_folder() {
        assert_eq!(split_entry("BESCES-50916RATCHET/save0.bin"), Some((FOLDER, "save0.bin")));
        for bad in ["save0.bin", "../x/save0.bin", "a/../b", "a/b/c", "a\\b", "C:/x", "/abs", "a/.hidden", "a/b.", ""] {
            assert_eq!(split_entry(bad), None, "{bad}");
        }
    }

    #[test]
    fn stamps_are_utc() {
        assert_eq!(utc_stamp(0), "1970-01-01T00:00:00Z");
        assert_eq!(utc_stamp(1_793_628_300), "2026-11-02T14:05:00Z");
    }

    /// All the saves out of one card and into an empty one: the same files, case and all, and the
    /// manifest names the game, its slots, planets and bolts.
    #[test]
    fn exports_and_imports_every_save() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("saves.zip"));
        card(&from);

        let m = export_card(&from, PAL, None, "0.10.1", &file, &RecordFiles::default()).unwrap();
        assert_eq!((m.format.as_str(), m.format_version, m.serial.as_str()), (FORMAT, 1, PAL));
        assert_eq!(m.launcher_version, "0.10.1");
        assert!(m.created.ends_with('Z'));
        assert!(!m.files.iter().any(|f| f.path.contains(".DS_Store")), "hidden files stay behind");
        let used: Vec<_> = m.slots.iter().filter(|s| !s.empty).collect();
        assert_eq!(used.len(), 2);
        assert_eq!((used[0].planet_name.as_deref(), used[0].bolts), (Some("Kerwan"), Some(1234)));
        assert_eq!((used[1].planet_name.as_deref(), used[1].bolts), (Some("Rilgar"), Some(99)));
        assert_eq!(used[1].mods.as_ref().unwrap()[0].id, "lock-strafe");
        assert!(!file.with_extension("zip.partial").exists());

        let preview = preview_into(&to, PAL, &file).unwrap();
        assert_eq!(preview.slots.len(), 2);
        assert!(preview.slots.iter().all(|s| s.replaces.is_none()));
        assert!(!preview.card_has_saves);

        let done = import_into(
            &to,
            PAL,
            &file,
            &SaveImportChoice { replace_all: true, slots: vec![] },
            &RecordFiles::default(),
        )
        .unwrap();
        assert_eq!(done.folders, vec![FOLDER.to_string()]);
        assert!(done.backup.is_none(), "an empty card has nothing to back up");
        for name in ["icon.sys", FOLDER, "save0.bin", "save1.bin", "save1.bin.mods", "save4.bin"] {
            assert_eq!(read(&to, name), read(&from, name), "{name}");
        }
        let names: Vec<String> =
            fs::read_dir(to.join(FOLDER)).unwrap().flatten().map(|e| e.file_name().to_string_lossy().into()).collect();
        assert!(names.contains(&FOLDER.to_string()), "the folder's case is kept: {names:?}");
    }

    /// The mods' records of the saves on this computer: NG++'s weapon tiers (`tiers.txt`, a line
    /// per save clock) in a mods' folder of the test's own.
    fn ngpp_records(mod_data: &Path) -> RecordFiles {
        let spec: crate::mod_save_data::SaveData = serde_json::from_str(
            r#"{"file": "tiers.txt", "records": "clock-hex", "size": 56, "title": "Weapon tiers",
                "fields": [{"key": "item_44", "name": "RYNO II", "offset": 44, "mask": 127, "max": 10,
                            "attach": {"to": "item", "id": 44, "as": "version"},
                            "requires": {"version": "last"}}]}"#,
        )
        .unwrap();
        RecordFiles { mod_data: mod_data.to_path_buf(), mods: vec![("ng-plus-plus".into(), spec)] }
    }

    /// A save played with NG++, exported and imported on another computer: the card's files beside
    /// it (`.mods`, `.moddata`, the SDK's per-save data) and NG++'s tiers record kept in its own
    /// folder all come back as they were, and export again unchanged.
    #[test]
    fn a_modded_save_keeps_its_mods_data_through_an_export_and_an_import() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("saves.zip"));
        card(&from);
        fs::write(from.join(FOLDER).join("save1.bin.moddata"), b"{\"ng-plus-plus\": \"AAEC\"}").unwrap();
        // The tiers: the save's clock (save()'s record 4) with the RYNO II at Ultima (2) and the
        // started mark, another save's line before it.
        let clock = "0014142300101026";
        let mut tiers = [0u8; 56];
        tiers[44] = 0x82;
        tiers[30] = 0x83;
        let record: String = tiers.iter().map(|b| format!("{b:02x}")).collect();
        let here = ngpp_records(&tmp.path().join("mod-data-a"));
        fs::create_dir_all(here.mod_data.join("ng-plus-plus")).unwrap();
        let text = format!("1111111111111111 {}\n{clock} {record}\n", "00".repeat(56));
        fs::write(here.mod_data.join("ng-plus-plus/tiers.txt"), &text).unwrap();

        let m = export_card(&from, PAL, None, "0.10.1", &file, &here).unwrap();
        assert!(m.files.iter().any(|f| f.path.ends_with("save1.bin.moddata")));
        let carried: Vec<_> = m.mod_records.iter().filter(|r| r.slot_index == 1).collect();
        assert_eq!(carried.len(), 1);
        assert_eq!((carried[0].id.as_str(), carried[0].file.as_str()), ("ng-plus-plus", "tiers.txt"));
        assert_eq!((carried[0].clock.as_str(), carried[0].data.as_str()), (clock, record.as_str()));
        assert!(!m.mod_records.iter().any(|r| r.clock == "1111111111111111"), "only the exported saves'");

        // Another computer: no tiers yet, and NG++ not even installed.
        let there = ngpp_records(&tmp.path().join("mod-data-b"));
        let all = SaveImportChoice { replace_all: true, slots: vec![] };
        import_into(&to, PAL, &file, &all, &there).unwrap();
        assert_eq!(read(&to, "save1.bin.moddata"), read(&from, "save1.bin.moddata"));
        assert_eq!(read(&to, "save1.bin"), read(&from, "save1.bin"));
        let back = fs::read_to_string(there.mod_data.join("ng-plus-plus/tiers.txt")).unwrap();
        assert_eq!(back, format!("{clock} {record}\n"));
        let read_back = crate::mod_save_data::read(
            &there.mod_data,
            "ng-plus-plus",
            "NG++",
            &there.mods[0].1,
            &clock_of_hex(clock).unwrap(),
        )
        .unwrap();
        assert!(read_back.found);
        assert_eq!(read_back.fields[0].value, 2, "the RYNO II at Ultima");

        // Exported again from there: the same records.
        let again = tmp.path().join("again.zip");
        let m2 = export_card(&to, PAL, None, "0.10.1", &again, &there).unwrap();
        assert_eq!(m2.mod_records, m.mod_records);

        // An edit there (the save editor writes the record) replaces the line, and a merge of one
        // slot brings the exported record back over it; the other save's line stays.
        crate::mod_save_data::write(
            &there.mod_data,
            "ng-plus-plus",
            &there.mods[0].1,
            &clock_of_hex(clock).unwrap(),
            &BTreeMap::from([("item_44".to_string(), 5u8)]),
        )
        .unwrap();
        fs::write(
            there.mod_data.join("ng-plus-plus/tiers.txt"),
            format!(
                "2222222222222222 {}\n{}",
                "01".repeat(56),
                fs::read_to_string(there.mod_data.join("ng-plus-plus/tiers.txt")).unwrap()
            ),
        )
        .unwrap();
        let one =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 1 }] };
        import_into(&to, PAL, &file, &one, &there).unwrap();
        let merged = fs::read_to_string(there.mod_data.join("ng-plus-plus/tiers.txt")).unwrap();
        assert_eq!(merged, format!("2222222222222222 {}\n{clock} {record}\n", "01".repeat(56)));
    }

    /// A record in a file is data, never a path: a mod id or file name that would leave the mods'
    /// folder is refused.
    #[test]
    fn a_carried_record_stays_in_its_mods_folder() {
        let tmp = tempfile::tempdir().unwrap();
        let there = ngpp_records(tmp.path());
        let clock = [1u8; 8];
        for (id, file) in [("../x", "tiers.txt"), ("ng-plus-plus", "../tiers.txt"), ("ng-plus-plus", "a/b")] {
            let r = CarriedRecord { id: id.into(), file: file.into(), data: "00".into() };
            assert!(there.put(&clock, &r).is_err(), "{id}/{file}");
        }
        let r = CarriedRecord { id: "ng-plus-plus".into(), file: "tiers.txt".into(), data: "zz".into() };
        assert!(there.put(&clock, &r).is_err(), "not hex");
    }

    /// One slot exported, and merged onto a card that has its own saves: only that slot changes,
    /// after a backup of the card.
    #[test]
    fn merges_one_slot_after_a_backup() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("one.zip"));
        card(&from);
        let f = to.join(FOLDER);
        fs::create_dir_all(&f).unwrap();
        fs::write(f.join("icon.sys"), b"their icon").unwrap();
        fs::write(f.join("save0.bin"), save(1, 7)).unwrap();
        fs::write(f.join("save1.bin"), save(2, 8)).unwrap();

        let m = export_card(&from, PAL, Some(1), "0.10.1", &file, &RecordFiles::default()).unwrap();
        assert!(m.files.iter().all(|f| !f.path.ends_with("save0.bin")), "no other slot");
        assert!(m.files.iter().any(|f| f.path.ends_with("icon.sys")), "the folder's own files");

        let preview = preview_into(&to, PAL, &file).unwrap();
        assert_eq!(preview.slots.len(), 1);
        assert_eq!(preview.slots[0].replaces.as_ref().and_then(|r| r.bolts), Some(8));
        assert!(preview.card_has_saves);

        let choice =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 1 }] };
        let done = import_into(&to, PAL, &file, &choice, &RecordFiles::default()).unwrap();
        assert_eq!(read(&to, "save1.bin"), read(&from, "save1.bin"));
        assert_eq!(read(&to, "save1.bin.mods"), read(&from, "save1.bin.mods"));
        assert_eq!(read(&to, "save0.bin"), save(1, 7), "the card's other slot is kept");
        assert_eq!(read(&to, "icon.sys"), b"their icon", "the card's own icon is kept");
        assert_eq!(read(&to, FOLDER), b"settings", "a file the card lacked is brought in");

        let backup = done.backup.expect("a backup first");
        assert_eq!(backup.note.as_deref(), Some("before import"));
        assert_eq!(fs::read(Path::new(&backup.path).join(FOLDER).join("save1.bin")).unwrap(), save(2, 8));
    }

    /// Replace all: the card's folder becomes the file's, after a backup.
    #[test]
    fn replaces_the_card_after_a_backup() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("all.zip"));
        card(&from);
        fs::create_dir_all(to.join(FOLDER)).unwrap();
        fs::write(to.join(FOLDER).join("save0.bin"), save(1, 7)).unwrap();
        fs::write(to.join(FOLDER).join("stray"), b"x").unwrap();
        export_card(&from, PAL, None, "0.10.1", &file, &RecordFiles::default()).unwrap();

        let done = import_into(
            &to,
            PAL,
            &file,
            &SaveImportChoice { replace_all: true, slots: vec![] },
            &RecordFiles::default(),
        )
        .unwrap();
        assert_eq!(read(&to, "save0.bin"), read(&from, "save0.bin"));
        assert!(!to.join(FOLDER).join("stray").exists());
        let backup = done.backup.unwrap();
        assert_eq!(fs::read(Path::new(&backup.path).join(FOLDER).join("stray")).unwrap(), b"x");
    }

    /// Going Commando: one slot exported and imported onto a card without the others gets them
    /// as empty slots, so the game still finds its folder (it needs all four slot files).
    #[test]
    fn a_going_commando_slot_alone_gets_empty_slots_beside_it() {
        const RAC2: &str = "SCES_516.07";
        let oozla = include_bytes!("../tests/fixtures/rac2/save-oozla.bin");
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("one.zip"));
        let folder = card_folder_name(RAC2).unwrap();
        fs::create_dir_all(from.join(&folder)).unwrap();
        fs::write(from.join(&folder).join("icon.sys"), b"icon").unwrap();
        fs::write(from.join(&folder).join("save0.bin"), oozla).unwrap();
        export_card(&from, RAC2, Some(0), "1.1.0", &file, &RecordFiles::default()).unwrap();

        let all = SaveImportChoice { replace_all: true, slots: vec![] };
        let done = import_into(&to, RAC2, &file, &all, &RecordFiles::default()).unwrap();
        let dir = to.join(&folder);
        assert_eq!(fs::read(dir.join("save0.bin")).unwrap(), oozla);
        assert_eq!(done.files, 5, "icon.sys, save0.bin and three empty slots");
        for i in 1..4 {
            let bytes = fs::read(dir.join(format!("save{i}.bin"))).unwrap();
            let slot = crate::save_edit_rac2::SaveFile::parse(&bytes).expect("a save the game loads");
            let level = slot.global.chunks.iter().find(|c| c.id == 0).unwrap();
            assert_eq!(level.data, (-1i32).to_le_bytes(), "the game's mark of an empty slot");
            assert!(saves::describe_save(RAC2, &bytes).empty, "the launcher lists it empty");
        }

        // A card that has the other slots keeps them.
        fs::write(dir.join("save2.bin"), oozla).unwrap();
        let one = SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder, slot_index: 0 }] };
        import_into(&to, RAC2, &file, &one, &RecordFiles::default()).unwrap();
        assert_eq!(fs::read(dir.join("save2.bin")).unwrap(), oozla);
    }

    /// Another disc's file (NTSC's on a PAL card) is refused, naming both, and nothing is written.
    #[test]
    fn refuses_another_games_saves() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("ntsc.zip"));
        card(&from);
        export_card(&from, "SCUS_971.99", None, "0.10.1", &file, &RecordFiles::default()).unwrap();
        fs::create_dir_all(to.join(FOLDER)).unwrap();
        fs::write(to.join(FOLDER).join("save0.bin"), b"mine").unwrap();

        let err = preview_into(&to, PAL, &file).unwrap_err();
        assert!(err.contains("SCUS_971.99") && err.contains(PAL), "{err}");
        let err = import_into(
            &to,
            PAL,
            &file,
            &SaveImportChoice { replace_all: true, slots: vec![] },
            &RecordFiles::default(),
        )
        .unwrap_err();
        assert!(err.contains("another game"), "{err}");
        assert_eq!(read(&to, "save0.bin"), b"mine");
        assert_eq!(fs::read_dir(&to).unwrap().count(), 1, "no backup either: nothing was done");
    }

    /// A file that is no zip, one cut short, one with a changed save, one with a path out of the
    /// card: each refused before the card is touched.
    #[test]
    fn refuses_a_damaged_file() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("ok.zip"));
        card(&from);
        export_card(&from, PAL, None, "0.10.1", &file, &RecordFiles::default()).unwrap();
        let good = fs::read(&file).unwrap();
        let all = SaveImportChoice { replace_all: true, slots: vec![] };

        let junk = tmp.path().join("junk.zip");
        fs::write(&junk, b"not a zip at all").unwrap();
        assert!(import_into(&to, PAL, &junk, &all, &RecordFiles::default())
            .unwrap_err()
            .contains("not an OpenRAC saves file"));

        let cut = tmp.path().join("cut.zip");
        fs::write(&cut, &good[..good.len() / 2]).unwrap();
        assert!(import_into(&to, PAL, &cut, &all, &RecordFiles::default()).is_err());

        // The same manifest over a changed save0.bin.
        let archive = read_archive(&file).unwrap();
        let rewrite = |path: &Path, manifest: &SaveArchiveManifest, change: &dyn Fn(&str, &mut Vec<u8>)| {
            let mut zip = zip::ZipWriter::new(fs::File::create(path).unwrap());
            let o = zip::write::SimpleFileOptions::default();
            zip.start_file(MANIFEST, o).unwrap();
            zip.write_all(&serde_json::to_vec(manifest).unwrap()).unwrap();
            for (p, b) in &archive.files {
                let mut b = b.clone();
                change(p, &mut b);
                zip.start_file(p.as_str(), o).unwrap();
                zip.write_all(&b).unwrap();
            }
            zip.finish().unwrap();
        };
        let changed = tmp.path().join("changed.zip");
        rewrite(&changed, &archive.manifest, &|p, b| {
            if p.ends_with("save0.bin") {
                b[20] ^= 0xff;
            }
        });
        assert!(import_into(&to, PAL, &changed, &all, &RecordFiles::default()).unwrap_err().contains("damaged"));

        let mut escaping = archive.manifest.clone();
        escaping.files[0].path = "../outside".into();
        let out = tmp.path().join("escape.zip");
        rewrite(&out, &escaping, &|_, _| {});
        assert!(import_into(&to, PAL, &out, &all, &RecordFiles::default())
            .unwrap_err()
            .contains("outside a save folder"));

        let mut newer = archive.manifest.clone();
        newer.format_version = FORMAT_VERSION + 1;
        let future = tmp.path().join("future.zip");
        rewrite(&future, &newer, &|_, _| {});
        assert!(import_into(&to, PAL, &future, &all, &RecordFiles::default()).unwrap_err().contains("newer launcher"));

        assert!(!to.exists(), "nothing was written");
    }

    /// A merge must choose a slot the file holds.
    #[test]
    fn a_merge_needs_a_slot_in_the_file() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("s.zip"));
        card(&from);
        export_card(&from, PAL, None, "0.10.1", &file, &RecordFiles::default()).unwrap();
        let none = SaveImportChoice { replace_all: false, slots: vec![] };
        assert!(import_into(&to, PAL, &file, &none, &RecordFiles::default()).is_err());
        let unused =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 3 }] };
        assert!(import_into(&to, PAL, &file, &unused, &RecordFiles::default()).unwrap_err().contains("slot 4"));
        assert!(!to.exists());
    }

    /// An empty slot or an empty card has nothing to export.
    #[test]
    fn nothing_to_export() {
        let tmp = tempfile::tempdir().unwrap();
        let file = tmp.path().join("x.zip");
        assert!(export_card(&tmp.path().join("none"), PAL, None, "0", &file, &RecordFiles::default()).is_err());
        card(tmp.path());
        assert!(export_card(tmp.path(), PAL, Some(3), "0", &file, &RecordFiles::default())
            .unwrap_err()
            .contains("slot 4"));
        assert!(!file.exists());
    }

    /// A card folder in another case (a card copied through a system that changed it) takes the
    /// file's name.
    #[test]
    fn the_folder_takes_the_files_case() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("s.zip"));
        card(&from);
        export_card(&from, PAL, None, "0", &file, &RecordFiles::default()).unwrap();
        fs::create_dir_all(to.join("besces-50916ratchet")).unwrap();
        fs::write(to.join("besces-50916ratchet").join("save0.bin"), save(1, 1)).unwrap();
        let choice =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 0 }] };
        import_into(&to, PAL, &file, &choice, &RecordFiles::default()).unwrap();
        assert_eq!(card_folders(&to), vec![FOLDER.to_string()]);
        assert_eq!(read(&to, "save0.bin"), read(&from, "save0.bin"));
    }

    const NTSC: &str = "SCUS_971.99";
    const NTSC_FOLDER: &str = "BASCUS-97199RATCHET";

    #[test]
    fn files_beside_a_save_go_with_its_slot() {
        assert_eq!(slot_file("save2.bin"), Some(2));
        assert_eq!(slot_file("save2.bin.mods"), Some(2));
        assert_eq!(slot_file("save2.bin.moddata"), Some(2));
        assert_eq!(slot_file("icon.sys"), None);
        assert_eq!(slot_file(FOLDER), None);
    }

    /// One slot exported: the mods' data of another slot stays behind with it.
    #[test]
    fn one_slot_leaves_the_other_slots_mod_data() {
        let tmp = tempfile::tempdir().unwrap();
        let file = tmp.path().join("one.zip");
        card(tmp.path());
        fs::write(tmp.path().join(FOLDER).join("save0.bin.moddata"), b"zero").unwrap();
        fs::write(tmp.path().join(FOLDER).join("save1.bin.moddata"), b"one").unwrap();
        let m = export_card(tmp.path(), PAL, Some(1), "0", &file, &RecordFiles::default()).unwrap();
        assert!(m.files.iter().any(|f| f.path.ends_with("save1.bin.moddata")));
        assert!(!m.files.iter().any(|f| f.path.ends_with("save0.bin.moddata")));
    }

    #[test]
    fn save_folder_names() {
        assert_eq!(card_folder_name(NTSC).as_deref(), Some(NTSC_FOLDER));
        assert_eq!(card_folder_name(PAL).as_deref(), Some(FOLDER));
        assert_eq!(card_folder_name("SCPS_150.37").as_deref(), Some("BISCPS-15037RATCHET"));
        assert_eq!(card_folder_name("SCUS_972.68").as_deref(), Some("BASCUS-97268RATCHET"));
        assert_eq!(card_folder_name("SCES_516.07").as_deref(), Some("BESCES-51607RATCHET"));
        for bad in ["", "SCUS", "scus_971.99", "SCUS_97.1", "SCUS_971.999"] {
            assert_eq!(card_folder_name(bad), None, "{bad}");
        }
    }

    /// The PAL disc's saves onto the US disc's card, which has none yet: the whole folder comes,
    /// one slot chosen or not, renamed to the US disc's with its reserved file.
    #[test]
    fn copies_the_pal_saves_to_an_empty_us_card() {
        let tmp = tempfile::tempdir().unwrap();
        let (pal, us) = (tmp.path().join("pal"), tmp.path().join("us"));
        card(&pal);

        let preview = preview_copy_between(&pal, PAL, &us, NTSC).unwrap();
        assert_eq!(preview.serial, PAL);
        assert!(!preview.card_has_saves);
        assert_eq!(preview.slots.len(), 2);
        assert!(preview.slots.iter().all(|s| s.slot.folder == NTSC_FOLDER && s.replaces.is_none()));

        let one = SaveImportChoice {
            replace_all: false,
            slots: vec![SaveSlotRef { folder: NTSC_FOLDER.into(), slot_index: 1 }],
        };
        let done = copy_between(&pal, PAL, &us, NTSC, &one).unwrap();
        assert_eq!(done.folders, vec![NTSC_FOLDER.to_string()]);
        assert!(done.backup.is_none(), "an empty card has nothing to back up");
        assert_eq!(card_folders(&us), vec![NTSC_FOLDER.to_string()]);
        let mine = us.join(NTSC_FOLDER);
        for name in ["icon.sys", "save0.bin", "save1.bin", "save1.bin.mods", "save4.bin"] {
            assert_eq!(fs::read(mine.join(name)).unwrap(), read(&pal, name), "{name}");
        }
        assert_eq!(fs::read(mine.join(NTSC_FOLDER)).unwrap(), b"settings", "the reserved file takes the name");
        assert!(!mine.join(FOLDER).exists());
        assert!(pal.join(FOLDER).join("save0.bin").exists(), "the PAL card is left as it was");
    }

    /// One slot onto a US card with saves of its own: that slot and its mod mark replaced, the old
    /// mod data beside it gone, the rest kept, after a backup named for the copy.
    #[test]
    fn merges_one_pal_slot_onto_a_us_card() {
        let tmp = tempfile::tempdir().unwrap();
        let (pal, us) = (tmp.path().join("pal"), tmp.path().join("us"));
        card(&pal);
        let mine = us.join(NTSC_FOLDER);
        fs::create_dir_all(&mine).unwrap();
        fs::write(mine.join("icon.sys"), b"us icon").unwrap();
        fs::write(mine.join("save0.bin"), save(1, 7)).unwrap();
        fs::write(mine.join("save1.bin"), save(2, 8)).unwrap();
        fs::write(mine.join("save1.bin.moddata"), b"old").unwrap();

        let preview = preview_copy_between(&pal, PAL, &us, NTSC).unwrap();
        assert!(preview.card_has_saves);
        let slot1 = preview.slots.iter().find(|s| s.slot.slot_index == 1).unwrap();
        assert_eq!(slot1.replaces.as_ref().and_then(|r| r.bolts), Some(8));

        let choice = SaveImportChoice {
            replace_all: false,
            slots: vec![SaveSlotRef { folder: NTSC_FOLDER.into(), slot_index: 1 }],
        };
        let done = copy_between(&pal, PAL, &us, NTSC, &choice).unwrap();
        assert_eq!(fs::read(mine.join("save1.bin")).unwrap(), read(&pal, "save1.bin"));
        assert_eq!(fs::read(mine.join("save1.bin.mods")).unwrap(), read(&pal, "save1.bin.mods"));
        assert!(!mine.join("save1.bin.moddata").exists(), "the replaced save's mod data goes with it");
        assert_eq!(fs::read(mine.join("save0.bin")).unwrap(), save(1, 7));
        assert_eq!(fs::read(mine.join("icon.sys")).unwrap(), b"us icon");
        assert_eq!(fs::read(mine.join(NTSC_FOLDER)).unwrap(), b"settings", "a file the card lacked");
        assert_eq!(done.backup.and_then(|b| b.note).as_deref(), Some("before copy from PAL"));
    }

    /// Another game's disc, the same disc, or a card without saves: refused, nothing written.
    #[test]
    fn copies_only_between_one_games_discs() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to) = (tmp.path().join("from"), tmp.path().join("to"));
        card(&from);
        let all = SaveImportChoice { replace_all: true, slots: vec![] };
        assert!(copy_between(&from, "SCES_516.07", &to, NTSC, &all).unwrap_err().contains("not the same game"));
        assert!(copy_between(&from, NTSC, &to, NTSC, &all).is_err());
        assert!(preview_copy_between(&to, PAL, &from, NTSC).unwrap_err().contains("holds no saves"));
        assert!(!to.exists());
    }

    /// The discs offered to copy from: the game's others whose cards hold saves.
    #[test]
    fn offers_the_other_discs_with_saves() {
        let tmp = tempfile::tempdir().unwrap();
        card(&tmp.path().join(PAL));
        fs::create_dir_all(tmp.path().join("SCPS_150.37").join("BISCPS-15037RATCHET")).unwrap();
        let sources = copy_sources_with(NTSC, |s| tmp.path().join(s));
        assert_eq!(sources.len(), 1);
        assert_eq!((sources[0].serial.as_str(), sources[0].region.as_str(), sources[0].saves), (PAL, "PAL", 2));
        assert!(copy_sources_with("SCUS_973.53", |s| tmp.path().join(s)).is_empty(), "a game without copies");
        assert!(copy_sources_with(PAL, |s| tmp.path().join(s)).is_empty(), "no other disc has saves");
    }

    /// The name offered for the file keeps only characters every system takes.
    #[test]
    fn export_names() {
        let all = export_name(PAL, None);
        assert!(all.starts_with("OpenRAC saves SCES_509.16 ") && all.ends_with(".zip"), "{all}");
        assert!(export_name(PAL, Some(1)).contains(" slot 2 "));
        assert!(!export_name("a/b\\c", None).contains(['/', '\\']));
    }
}
