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

use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::fs;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

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

/// The slot whose mark a file name is (`save3.bin.mods` is 3).
fn mark_of(name: &str) -> Option<usize> {
    slot_of(name.strip_suffix(".mods")?)
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
        // The one slot and its mark, and the folder's own files: no other slot.
        files.retain(|path, _| {
            let file = path.split_once('/').map_or("", |(_, f)| f);
            match (slot_of(file), mark_of(file)) {
                (Some(i), _) | (_, Some(i)) => i == index,
                _ => true,
            }
        });
        let used = slots_in(serial, &files).iter().any(|s| s.slot_index == index && !s.empty);
        if !used {
            return Err(format!("slot {} holds no save", index + 1));
        }
    }

    let title = files.iter().find(|(p, _)| p.ends_with("/icon.sys")).and_then(|(_, b)| saves::icon_title(b));
    let manifest = SaveArchiveManifest {
        format: FORMAT.into(),
        format_version: FORMAT_VERSION,
        serial: serial.to_string(),
        title,
        launcher_version: launcher_version.to_string(),
        created: utc_stamp(now_secs()),
        slots: slots_in(serial, &files),
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
    let mut card = BTreeMap::new();
    for folder in card_folders(card_dir) {
        card.extend(folder_files(card_dir, &folder).unwrap_or_default());
    }
    let on_card = slots_in(serial, &card);
    let slots = slots_in(serial, &archive.files)
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
    let m = archive.manifest;
    Ok(SaveImportPreview {
        serial: m.serial,
        title: m.title,
        launcher_version: m.launcher_version,
        created: m.created,
        slots,
        card_has_saves: on_card.iter().any(|s| !s.empty),
    })
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
) -> Result<ImportedSaves, String> {
    let archive = read_archive(source)?;
    check_serial(&archive, serial)?;
    let in_file: Vec<SaveArchiveSlot> = slots_in(serial, &archive.files);
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
    let backup = if holds_saves { Some(saves::backup_card(card_dir, Some("before import"))?) } else { None };

    // The file's folders, each with its files.
    let mut by_folder: BTreeMap<&str, Vec<(&str, &[u8])>> = BTreeMap::new();
    for (path, bytes) in &archive.files {
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
                || match (slot_of(file), mark_of(file)) {
                    (Some(i), _) | (_, Some(i)) => picked(i),
                    // The folder's own files (icon, settings): only where the card has none.
                    _ => !target.exists(),
                };
            if take {
                write_file(&target, bytes)?;
                written += 1;
            }
        }
        if !choice.replace_all {
            // A chosen slot without a mark in the file was played without mods: the card's old
            // mark beside it would say otherwise.
            for pick in choice.slots.iter().filter(|p| p.folder == folder) {
                let mark = format!("save{}.bin.mods", pick.slot_index);
                if !entries.iter().any(|(f, _)| *f == mark) {
                    let _ = fs::remove_file(dest.join(&mark));
                }
            }
        }
        folders.push(folder.to_string());
    }
    Ok(ImportedSaves { folders, files: written, backup })
}

/// [`export_card`] from the game's own card.
pub fn export_saves(
    serial: &str,
    slot: Option<usize>,
    launcher_version: &str,
    dest: &Path,
) -> Result<SaveArchiveManifest, String> {
    export_card(&saves::get_memcard_dir(serial), serial, slot, launcher_version, dest)
}

/// [`preview_into`] the game's own card.
pub fn preview_import(serial: &str, source: &Path) -> Result<SaveImportPreview, String> {
    preview_into(&saves::get_memcard_dir(serial), serial, source)
}

/// [`import_into`] the game's own card.
pub fn import_saves(serial: &str, source: &Path, choice: &SaveImportChoice) -> Result<ImportedSaves, String> {
    import_into(&saves::get_memcard_dir(serial), serial, source, choice)
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
        assert_eq!(mark_of("save1.bin.mods"), Some(1));
        assert_eq!(mark_of("save1.bin"), None);
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

        let m = export_card(&from, PAL, None, "0.10.1", &file).unwrap();
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

        let done = import_into(&to, PAL, &file, &SaveImportChoice { replace_all: true, slots: vec![] }).unwrap();
        assert_eq!(done.folders, vec![FOLDER.to_string()]);
        assert!(done.backup.is_none(), "an empty card has nothing to back up");
        for name in ["icon.sys", FOLDER, "save0.bin", "save1.bin", "save1.bin.mods", "save4.bin"] {
            assert_eq!(read(&to, name), read(&from, name), "{name}");
        }
        let names: Vec<String> =
            fs::read_dir(to.join(FOLDER)).unwrap().flatten().map(|e| e.file_name().to_string_lossy().into()).collect();
        assert!(names.contains(&FOLDER.to_string()), "the folder's case is kept: {names:?}");
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

        let m = export_card(&from, PAL, Some(1), "0.10.1", &file).unwrap();
        assert!(m.files.iter().all(|f| !f.path.ends_with("save0.bin")), "no other slot");
        assert!(m.files.iter().any(|f| f.path.ends_with("icon.sys")), "the folder's own files");

        let preview = preview_into(&to, PAL, &file).unwrap();
        assert_eq!(preview.slots.len(), 1);
        assert_eq!(preview.slots[0].replaces.as_ref().and_then(|r| r.bolts), Some(8));
        assert!(preview.card_has_saves);

        let choice =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 1 }] };
        let done = import_into(&to, PAL, &file, &choice).unwrap();
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
        export_card(&from, PAL, None, "0.10.1", &file).unwrap();

        let done = import_into(&to, PAL, &file, &SaveImportChoice { replace_all: true, slots: vec![] }).unwrap();
        assert_eq!(read(&to, "save0.bin"), read(&from, "save0.bin"));
        assert!(!to.join(FOLDER).join("stray").exists());
        let backup = done.backup.unwrap();
        assert_eq!(fs::read(Path::new(&backup.path).join(FOLDER).join("stray")).unwrap(), b"x");
    }

    /// Another disc's file (NTSC's on a PAL card) is refused, naming both, and nothing is written.
    #[test]
    fn refuses_another_games_saves() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("ntsc.zip"));
        card(&from);
        export_card(&from, "SCUS_971.99", None, "0.10.1", &file).unwrap();
        fs::create_dir_all(to.join(FOLDER)).unwrap();
        fs::write(to.join(FOLDER).join("save0.bin"), b"mine").unwrap();

        let err = preview_into(&to, PAL, &file).unwrap_err();
        assert!(err.contains("SCUS_971.99") && err.contains(PAL), "{err}");
        let err = import_into(&to, PAL, &file, &SaveImportChoice { replace_all: true, slots: vec![] }).unwrap_err();
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
        export_card(&from, PAL, None, "0.10.1", &file).unwrap();
        let good = fs::read(&file).unwrap();
        let all = SaveImportChoice { replace_all: true, slots: vec![] };

        let junk = tmp.path().join("junk.zip");
        fs::write(&junk, b"not a zip at all").unwrap();
        assert!(import_into(&to, PAL, &junk, &all).unwrap_err().contains("not an OpenRAC saves file"));

        let cut = tmp.path().join("cut.zip");
        fs::write(&cut, &good[..good.len() / 2]).unwrap();
        assert!(import_into(&to, PAL, &cut, &all).is_err());

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
        assert!(import_into(&to, PAL, &changed, &all).unwrap_err().contains("damaged"));

        let mut escaping = archive.manifest.clone();
        escaping.files[0].path = "../outside".into();
        let out = tmp.path().join("escape.zip");
        rewrite(&out, &escaping, &|_, _| {});
        assert!(import_into(&to, PAL, &out, &all).unwrap_err().contains("outside a save folder"));

        let mut newer = archive.manifest.clone();
        newer.format_version = FORMAT_VERSION + 1;
        let future = tmp.path().join("future.zip");
        rewrite(&future, &newer, &|_, _| {});
        assert!(import_into(&to, PAL, &future, &all).unwrap_err().contains("newer launcher"));

        assert!(!to.exists(), "nothing was written");
    }

    /// A merge must choose a slot the file holds.
    #[test]
    fn a_merge_needs_a_slot_in_the_file() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("s.zip"));
        card(&from);
        export_card(&from, PAL, None, "0.10.1", &file).unwrap();
        let none = SaveImportChoice { replace_all: false, slots: vec![] };
        assert!(import_into(&to, PAL, &file, &none).is_err());
        let unused =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 3 }] };
        assert!(import_into(&to, PAL, &file, &unused).unwrap_err().contains("slot 4"));
        assert!(!to.exists());
    }

    /// An empty slot or an empty card has nothing to export.
    #[test]
    fn nothing_to_export() {
        let tmp = tempfile::tempdir().unwrap();
        let file = tmp.path().join("x.zip");
        assert!(export_card(&tmp.path().join("none"), PAL, None, "0", &file).is_err());
        card(tmp.path());
        assert!(export_card(tmp.path(), PAL, Some(3), "0", &file).unwrap_err().contains("slot 4"));
        assert!(!file.exists());
    }

    /// A card folder in another case (a card copied through a system that changed it) takes the
    /// file's name.
    #[test]
    fn the_folder_takes_the_files_case() {
        let tmp = tempfile::tempdir().unwrap();
        let (from, to, file) = (tmp.path().join("a"), tmp.path().join("b"), tmp.path().join("s.zip"));
        card(&from);
        export_card(&from, PAL, None, "0", &file).unwrap();
        fs::create_dir_all(to.join("besces-50916ratchet")).unwrap();
        fs::write(to.join("besces-50916ratchet").join("save0.bin"), save(1, 1)).unwrap();
        let choice =
            SaveImportChoice { replace_all: false, slots: vec![SaveSlotRef { folder: FOLDER.into(), slot_index: 0 }] };
        import_into(&to, PAL, &file, &choice).unwrap();
        assert_eq!(card_folders(&to), vec![FOLDER.to_string()]);
        assert_eq!(read(&to, "save0.bin"), read(&from, "save0.bin"));
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
