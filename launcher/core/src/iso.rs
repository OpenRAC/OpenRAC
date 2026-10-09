//! ISO 9660 inspection, serial detection, and asset extraction for OpenRAC.
//!
//! PS2 discs are ISO 9660 images whose root directory contains a `SYSTEM.CNF`
//! file. The line `BOOT2 = cdrom0:\<SERIAL>;1` tells which executable the
//! PlayStation 2 boots (e.g. `SCES_509.16`, `SCUS_971.99`, `SCUS_972.68`).
//!
//! This module reads the Primary Volume Descriptor and directory records
//! directly (standard library only), verifies the serial and image size
//! against OpenRAC's game manifests (`games/*/game.json`), extracts files
//! to `assets/` or `OpenRAC/<game>`, and places inputs.

use std::fs::{self, File};
use std::io::{Read, Seek, SeekFrom};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::catalog::{Catalog, Version};

const SECTOR_SIZE: u64 = 2048;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum IsoMatchStatus {
    /// Correct game, correct version, exact size matches game.json.
    ExactMatch,
    /// Correct game & serial, but size or revision differs (e.g. v2.00 vs v1.01).
    RevisionMismatch,
    /// A valid PS2 disc, but for a different game or version.
    WrongGame,
    /// An ISO image, but missing SYSTEM.CNF or not a recognised PS2 disc.
    NotPs2Disc,
    /// Not a valid ISO 9660 filesystem.
    InvalidIso,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct IsoInspection {
    pub path: PathBuf,
    pub filename: String,
    pub size: u64,
    pub is_valid_iso: bool,
    pub serial: Option<String>,
    pub detected_game_id: Option<String>,
    pub detected_game_title: Option<String>,
    pub detected_version_name: Option<String>,
    pub detected_region: Option<String>,
    pub target_game_id: String,
    pub target_version_key: String,
    pub target_serial: Option<String>,
    pub target_expected_size: Option<u64>,
    pub matches_target_game: bool,
    pub matches_target_version: bool,
    pub status: IsoMatchStatus,
    pub message: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ImportResult {
    pub target_key: String,
    pub baserom_path: PathBuf,
    pub extracted_assets_dir: PathBuf,
    pub extracted_files: Vec<String>,
    pub setup_message: String,
}

#[derive(Debug, Clone)]
pub struct IsoFileEntry {
    pub name: String,
    pub lba: u32,
    pub size: u32,
    pub is_dir: bool,
}

/// Locates a file in the ISO 9660 filesystem by relative path, e.g. "SYSTEM.CNF".
pub fn iso_find_file<R: Read + Seek>(reader: &mut R, path: &str) -> Option<(u32, u32)> {
    // Sector 16: Primary Volume Descriptor
    reader.seek(SeekFrom::Start(16 * SECTOR_SIZE)).ok()?;
    let mut pvd = [0u8; 2048];
    reader.read_exact(&mut pvd).ok()?;
    if &pvd[1..6] != b"CD001" {
        return None;
    }

    // Root directory record at offset 156 of PVD
    let mut root_lba = u32::from_le_bytes(pvd[158..162].try_into().ok()?);
    let mut root_size = u32::from_le_bytes(pvd[166..170].try_into().ok()?);

    let parts: Vec<&str> = path.split('/').filter(|p| !p.is_empty()).collect();
    for (idx, part) in parts.iter().enumerate() {
        let is_last = idx == parts.len() - 1;
        let entries = read_dir_entries(reader, root_lba, root_size)?;
        let found = entries.into_iter().find(|e| e.name.eq_ignore_ascii_case(part))?;
        if is_last {
            return Some((found.lba, found.size));
        } else if found.is_dir {
            root_lba = found.lba;
            root_size = found.size;
        } else {
            return None;
        }
    }
    None
}

/// Reads directory entries from an ISO extent.
pub fn read_dir_entries<R: Read + Seek>(reader: &mut R, lba: u32, size: u32) -> Option<Vec<IsoFileEntry>> {
    reader.seek(SeekFrom::Start(lba as u64 * SECTOR_SIZE)).ok()?;
    let mut data = vec![0u8; size as usize];
    reader.read_exact(&mut data).ok()?;

    let mut entries = Vec::new();
    let mut i = 0;
    while i < data.len() {
        let len = data[i] as usize;
        if len == 0 {
            // ISO 9660 directory records do not cross sector boundaries
            i = ((i / SECTOR_SIZE as usize) + 1) * SECTOR_SIZE as usize;
            continue;
        }
        if i + len > data.len() || len < 33 {
            break;
        }

        let entry_lba = u32::from_le_bytes(data[i + 2..i + 6].try_into().ok()?);
        let entry_size = u32::from_le_bytes(data[i + 10..i + 14].try_into().ok()?);
        let flags = data[i + 25];
        let name_len = data[i + 32] as usize;

        if i + 33 + name_len <= data.len() {
            let raw_name = &data[i + 33..i + 33 + name_len];
            let name_str = String::from_utf8_lossy(raw_name);
            let clean_name = name_str.split(';').next().unwrap_or(&name_str).trim();
            // Skip '.' (\x00) and '..' (\x01)
            if !clean_name.is_empty() && clean_name != "\x00" && clean_name != "\x01" {
                entries.push(IsoFileEntry {
                    name: clean_name.to_string(),
                    lba: entry_lba,
                    size: entry_size,
                    is_dir: (flags & 2) != 0,
                });
            }
        }
        i += len;
    }
    Some(entries)
}

/// Reads `SYSTEM.CNF` and returns the boot executable serial (e.g. `SCES_509.16`).
pub fn read_boot_serial(iso_path: &Path) -> Result<String, String> {
    let mut file = File::open(iso_path).map_err(|e| format!("{}: {e}", iso_path.display()))?;
    let (lba, size) = iso_find_file(&mut file, "SYSTEM.CNF")
        .ok_or_else(|| "Not a valid PS2 disc image: SYSTEM.CNF not found".to_string())?;

    file.seek(SeekFrom::Start(lba as u64 * SECTOR_SIZE)).map_err(|e| e.to_string())?;
    let mut buf = vec![0u8; size as usize];
    file.read_exact(&mut buf).map_err(|e| e.to_string())?;

    let content = String::from_utf8_lossy(&buf);
    for line in content.lines() {
        if let Some((k, v)) = line.split_once('=') {
            if k.trim() == "BOOT2" {
                let val = v.trim();
                let filename = val.split('\\').last().unwrap_or(val);
                let serial = filename.split(';').next().unwrap_or(filename).trim();
                return Ok(serial.to_string());
            }
        }
    }
    Err("SYSTEM.CNF does not contain a BOOT2 directive".to_string())
}

/// Inspects an ISO file against a target game and version in the catalog.
pub fn inspect_iso(catalog: &Catalog, target_key: &str, iso_path: &Path) -> IsoInspection {
    let filename = iso_path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
    let size = fs::metadata(iso_path).map(|m| m.len()).unwrap_or(0);

    let (target_game_id, target_ver_name) = match target_key.split_once('/') {
        Some((g, v)) => (g.to_string(), v.to_string()),
        None => (target_key.to_string(), String::new()),
    };

    let target_version: Option<&Version> = catalog
        .games
        .iter()
        .find(|g| g.id == target_game_id)
        .and_then(|g| g.versions.iter().find(|v| v.name == target_ver_name));

    let target_serial = target_version.map(|v| v.serial.clone());
    let target_expected_size = target_version.and_then(|v| v.disc.as_ref()).and_then(|d| d.size);

    let serial_res = read_boot_serial(iso_path);
    let (is_valid_iso, serial, not_ps2) = match serial_res {
        Ok(s) => (true, Some(s), false),
        Err(err) => {
            let is_iso = File::open(iso_path)
                .ok()
                .and_then(|mut f| {
                    f.seek(SeekFrom::Start(16 * SECTOR_SIZE)).ok()?;
                    let mut pvd = [0u8; 6];
                    f.read_exact(&mut pvd).ok()?;
                    Some(&pvd[1..6] == b"CD001")
                })
                .unwrap_or(false);
            (is_iso, None, is_iso && err.contains("SYSTEM.CNF"))
        }
    };

    // Match against any game in catalog
    let mut detected_game_id = None;
    let mut detected_game_title = None;
    let mut detected_version_name = None;
    let mut detected_region = None;

    if let Some(ser) = &serial {
        for game in &catalog.games {
            for v in &game.versions {
                if v.serial == *ser {
                    detected_game_id = Some(game.id.clone());
                    detected_game_title = Some(game.title.clone());
                    detected_version_name = Some(v.name.clone());
                    detected_region = Some(v.region.clone());
                    break;
                }
            }
        }
    }

    let matches_target_game = detected_game_id.as_deref() == Some(&target_game_id);
    let matches_target_version = matches_target_game && target_serial.as_deref() == serial.as_deref();

    let (status, message) = if !is_valid_iso {
        (IsoMatchStatus::InvalidIso, format!("{filename} is not a valid ISO 9660 image."))
    } else if not_ps2 || serial.is_none() {
        (
            IsoMatchStatus::NotPs2Disc,
            format!("{filename} is an ISO image, but lacks a valid PS2 SYSTEM.CNF boot file."),
        )
    } else {
        let ser = serial.as_ref().unwrap();
        if !matches_target_version {
            let found_desc = match (&detected_game_title, &detected_region) {
                (Some(title), Some(reg)) => format!("{title} ({reg}, {ser})"),
                (Some(title), None) => format!("{title} ({ser})"),
                _ => format!("unknown title ({ser})"),
            };
            (
                IsoMatchStatus::WrongGame,
                format!(
                    "Game mismatch: detected {found_desc}, but this slot expects {} (serial {}).",
                    target_version.map(|v| v.title.as_str()).unwrap_or(&target_game_id),
                    target_serial.as_deref().unwrap_or("unknown")
                ),
            )
        } else {
            // Serial matches! Check size
            match target_expected_size {
                Some(expected_size) if size == expected_size => (
                    IsoMatchStatus::ExactMatch,
                    format!(
                        "Exact match! Detected {} ({}, {ser}) with expected size ({size} bytes).",
                        detected_game_title.as_deref().unwrap_or(&target_game_id),
                        detected_region.as_deref().unwrap_or(""),
                    ),
                ),
                Some(expected_size) => (
                    IsoMatchStatus::RevisionMismatch,
                    format!(
                        "Version mismatch: detected correct serial ({ser}), but size differs (got {size} bytes, expected {expected_size} bytes). You may have another revision (e.g. v2.00 vs v1.01) or a modified dump.",
                    ),
                ),
                None => (
                    IsoMatchStatus::ExactMatch,
                    format!("Serial matches ({ser}); game.json does not pin a specific size."),
                ),
            }
        }
    };

    IsoInspection {
        path: iso_path.to_path_buf(),
        filename,
        size,
        is_valid_iso,
        serial,
        detected_game_id,
        detected_game_title,
        detected_version_name,
        detected_region,
        target_game_id,
        target_version_key: target_key.to_string(),
        target_serial,
        target_expected_size,
        matches_target_game,
        matches_target_version,
        status,
        message,
    }
}

/// Extracts all files from an ISO image into `out_dir` using standard Linux utilities (7z, bsdtar)
/// or built-in ISO reader, completely independent of Docker or Podman.
pub fn extract_iso_files(iso_path: &Path, out_dir: &Path) -> Result<Vec<String>, String> {
    fs::create_dir_all(out_dir).map_err(|e| format!("{}: {e}", out_dir.display()))?;

    // 1. Try 7z (standard utility available across all popular Linux distributions via 7zip / p7zip)
    if let Ok(output) = std::process::Command::new("7z")
        .arg("x")
        .arg("-y")
        .arg(format!("-o{}", out_dir.display()))
        .arg(iso_path)
        .output()
    {
        if output.status.success() {
            let mut extracted = Vec::new();
            if let Ok(entries) = fs::read_dir(out_dir) {
                for entry in entries.flatten() {
                    extracted.push(entry.file_name().to_string_lossy().to_string());
                }
            }
            if !extracted.is_empty() {
                extracted.sort();
                return Ok(extracted);
            }
        }
    }

    // 2. Try bsdtar (standard libarchive utility present in many distros)
    if let Ok(output) = std::process::Command::new("bsdtar")
        .arg("-xf")
        .arg(iso_path)
        .arg("-C")
        .arg(out_dir)
        .output()
    {
        if output.status.success() {
            let mut extracted = Vec::new();
            if let Ok(entries) = fs::read_dir(out_dir) {
                for entry in entries.flatten() {
                    extracted.push(entry.file_name().to_string_lossy().to_string());
                }
            }
            if !extracted.is_empty() {
                extracted.sort();
                return Ok(extracted);
            }
        }
    }

    // 3. Fallback to built-in pure Rust ISO 9660 reader (no external tools required)
    let mut file = File::open(iso_path).map_err(|e| format!("{}: {e}", iso_path.display()))?;

    // Read PVD
    file.seek(SeekFrom::Start(16 * SECTOR_SIZE)).map_err(|e| e.to_string())?;
    let mut pvd = [0u8; 2048];
    file.read_exact(&mut pvd).map_err(|e| e.to_string())?;
    if &pvd[1..6] != b"CD001" {
        return Err("Not an ISO 9660 image".to_string());
    }

    let root_lba = u32::from_le_bytes(pvd[158..162].try_into().unwrap());
    let root_size = u32::from_le_bytes(pvd[166..170].try_into().unwrap());

    let entries = read_dir_entries(&mut file, root_lba, root_size)
        .ok_or_else(|| "Failed to read root directory".to_string())?;

    let mut extracted = Vec::new();
    for entry in entries {
        if entry.is_dir {
            continue;
        }
        let dest = out_dir.join(&entry.name);
        file.seek(SeekFrom::Start(entry.lba as u64 * SECTOR_SIZE)).map_err(|e| e.to_string())?;
        let mut buf = vec![0u8; entry.size as usize];
        file.read_exact(&mut buf).map_err(|e| e.to_string())?;
        fs::write(&dest, &buf).map_err(|e| format!("{}: {e}", dest.display()))?;
        extracted.push(entry.name);
    }

    Ok(extracted)
}

/// Imports an ISO file into OpenRAC:
/// 1. Verifies inspection
/// 2. Links/copies into `baserom/<spec.file>`
/// 3. Extracts files into `<root>/<game_id>` (and creates a `~/<game_id>` symlink if possible)
/// 4. Executes `python3 tools/openrac.py setup <target_key>` to place game inputs
pub fn import_iso(
    root: &Path,
    catalog: &Catalog,
    target_key: &str,
    source_iso: &Path,
) -> Result<ImportResult, String> {
    let inspection = inspect_iso(catalog, target_key, source_iso);
    if !inspection.is_valid_iso {
        return Err(format!("Cannot import: {}", inspection.message));
    }
    if inspection.status == IsoMatchStatus::WrongGame {
        return Err(format!("Cannot import: {}", inspection.message));
    }

    let (game_id, ver_name) = target_key
        .split_once('/')
        .ok_or_else(|| format!("Invalid target key: {target_key}"))?;

    let game = catalog
        .games
        .iter()
        .find(|g| g.id == game_id)
        .ok_or_else(|| format!("Game not found: {game_id}"))?;
    let version = game
        .versions
        .iter()
        .find(|v| v.name == ver_name)
        .ok_or_else(|| format!("Version not found: {ver_name}"))?;

    let spec = version
        .disc
        .as_ref()
        .ok_or_else(|| format!("Version {target_key} has no disc spec in game.json"))?;

    // 1. Link or copy into baserom/
    let baserom_dir = root.join("baserom");
    fs::create_dir_all(&baserom_dir).map_err(|e| e.to_string())?;
    let baserom_dest = baserom_dir.join(&spec.file);

    if baserom_dest.exists() {
        let same = fs::metadata(&baserom_dest)
            .ok()
            .and_then(|m1| fs::metadata(source_iso).ok().map(|m2| m1.len() == m2.len()))
            .unwrap_or(false);
        if !same {
            let _ = fs::remove_file(&baserom_dest);
        }
    }

    if !baserom_dest.exists() {
        if let Err(_) = fs::hard_link(source_iso, &baserom_dest) {
            fs::copy(source_iso, &baserom_dest)
                .map_err(|e| format!("Failed to copy to {}: {e}", baserom_dest.display()))?;
        }
    }

    // 2. Extract game assets into <root>/<game_id>
    let assets_dir = root.join(game_id);
    let extracted_files = extract_iso_files(source_iso, &assets_dir)?;

    // Also link /home/<user>/<game_id> if outside root and not yet existing
    if let Ok(home) = std::env::var("HOME") {
        let home_target = PathBuf::from(home).join(game_id);
        if !home_target.exists() && home_target != assets_dir {
            #[cfg(unix)]
            let _ = std::os::unix::fs::symlink(&assets_dir, &home_target);
        }
    }

    // 3. Place inputs via `tools/openrac.py setup <target_key>`
    let py_script = root.join("tools").join("openrac.py");
    let mut setup_message = String::new();
    if py_script.is_file() {
        let output = std::process::Command::new("python3")
            .arg(&py_script)
            .arg("setup")
            .arg(target_key)
            .current_dir(root)
            .output();
        match output {
            Ok(out) => {
                let msg = String::from_utf8_lossy(&out.stdout).to_string();
                setup_message = msg;
            }
            Err(e) => {
                setup_message = format!("Setup execution error: {e}");
            }
        }
    }

    Ok(ImportResult {
        target_key: target_key.to_string(),
        baserom_path: baserom_dest,
        extracted_assets_dir: assets_dir,
        extracted_files,
        setup_message,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::catalog::load;
    use crate::testing::checkout;

    #[test]
    fn inspects_existing_disc_or_reports_missing() {
        let root = checkout();
        let cat = load(&root).unwrap();

        let disc_path = root
            .join("baserom")
            .join("Ratchet & Clank (Europe) (En,Fr,De,Es,It) (v2.00).iso");
        if disc_path.is_file() {
            let res = inspect_iso(&cat, "rac1/pal", &disc_path);
            assert_eq!(res.status, IsoMatchStatus::ExactMatch);
            assert_eq!(res.serial.as_deref(), Some("SCES_509.16"));
            assert!(res.matches_target_version);

            // Test mismatch on another game slot
            let mismatch_res = inspect_iso(&cat, "rac2/ntsc", &disc_path);
            assert_eq!(mismatch_res.status, IsoMatchStatus::WrongGame);
            assert!(!mismatch_res.matches_target_version);
        }
    }

    #[test]
    fn detects_non_iso_cleanly() {
        let root = checkout();
        let cat = load(&root).unwrap();
        let readme = root.join("README.md");
        let res = inspect_iso(&cat, "rac1/pal", &readme);
        assert_eq!(res.status, IsoMatchStatus::InvalidIso);
    }
}
