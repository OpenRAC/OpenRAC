// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//! A version's external PCSX2 replacement folder, used directly at the next Play.

use std::path::{Path, PathBuf};

pub const ENV: &str = "OPENRAC_PCSX2_TEXTURE_PACK";

fn selection_file(home: &Path, game: &str, version: &str) -> Result<PathBuf, String> {
    Ok(home.join("pcsx2_texture_packs").join(format!("{}.json", super::id(game, version)?)))
}

pub fn selected(home: &Path, game: &str, version: &str) -> Result<Option<PathBuf>, String> {
    let file = selection_file(home, game, version)?;
    match std::fs::read_to_string(&file) {
        Ok(text) => serde_json::from_str(&text).map_err(|e| format!("{}: {e}", file.display())),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(e) => Err(format!("{}: {e}", file.display())),
    }
}

/// Accept the textures root, the matching serial folder, or its replacements folder.
pub fn select(home: &Path, game: &str, version: &str, serial: &str, source: &str) -> Result<(), String> {
    let file = selection_file(home, game, version)?;
    let selected = if source.is_empty() {
        None
    } else {
        let source = Path::new(source);
        let serial = serial.replace('_', "-").replace('.', "");
        let folder = if source.join(&serial).join("replacements").is_dir() {
            source.join(&serial).join("replacements")
        } else if source.file_name().is_some_and(|name| name == serial.as_str()) {
            source.join("replacements")
        } else if source.file_name().is_some_and(|name| name == "replacements")
            && source.parent().and_then(Path::file_name).is_some_and(|name| name == serial.as_str())
        {
            source.to_path_buf()
        } else {
            return Err(format!("Choose the textures folder, {serial}, or its replacements folder."));
        };
        if !folder.is_absolute() || !contains_replacements(&folder)? {
            return Err(format!("No supported PCSX2 PNG or DDS replacements found for {serial}."));
        }
        Some(folder)
    };
    let text = serde_json::to_string(&selected).map_err(|e| e.to_string())?;
    crate::differences::write_settings(&file, &text)
}

fn contains_replacements(folder: &Path) -> Result<bool, String> {
    let mut pending = vec![folder.to_path_buf()];
    while let Some(dir) = pending.pop() {
        for entry in std::fs::read_dir(&dir).map_err(|e| format!("{}: {e}", dir.display()))? {
            let entry = entry.map_err(|e| e.to_string())?;
            let kind = entry.file_type().map_err(|e| e.to_string())?;
            if kind.is_dir() {
                pending.push(entry.path());
            } else if kind.is_file() && replacement_name(&entry.file_name().to_string_lossy()) {
                return Ok(true);
            }
        }
    }
    Ok(false)
}

fn replacement_name(name: &str) -> bool {
    let Some((stem, extension)) = name.rsplit_once('.') else { return false };
    if !extension.eq_ignore_ascii_case("png") && !extension.eq_ignore_ascii_case("dds") {
        return false;
    }
    let parts: Vec<_> = stem.split('-').collect();
    let (texture, palette, bits) = match parts.as_slice() {
        [texture, palette, bits] => (*texture, *palette, *bits),
        [texture, palette, region, bits]
            if region.strip_prefix('r').is_some_and(|r| u64::from_str_radix(r, 16).is_ok()) =>
        {
            (*texture, *palette, *bits)
        }
        _ => return false,
    };
    if u64::from_str_radix(texture, 16).is_err() || u64::from_str_radix(palette, 16).is_err() || bits.len() != 8 {
        return false;
    }
    u32::from_str_radix(bits, 16)
        .is_ok_and(|bits| matches!(bits & 63, 0x13 | 0x1b) && (bits >> 6) & 15 <= 10 && (bits >> 10) & 15 <= 10)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn selection_accepts_each_folder_level_and_persists_per_version() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().join("HD pack with spaces");
        let serial = root.join("SCUS-97199");
        let folder = serial.join("replacements");
        std::fs::create_dir_all(folder.join("Interface")).unwrap();
        std::fs::write(folder.join("Interface/abc-def-r1000000100000-00005153.DDS"), b"DDS ").unwrap();
        let home = temp.path().join("home");
        for source in [&root, &serial, &folder] {
            select(&home, "rac1", "ntsc", "SCUS_971.99", source.to_str().unwrap()).unwrap();
            assert_eq!(selected(&home, "rac1", "ntsc").unwrap(), Some(folder.clone()));
        }
        assert_eq!(selected(&home, "rac1", "pal").unwrap(), None);
        // Invalid selections preserve the last working pack.
        assert!(select(&home, "rac1", "ntsc", "SCUS_971.99", home.to_str().unwrap()).is_err());
        assert_eq!(selected(&home, "rac1", "ntsc").unwrap(), Some(folder));
        select(&home, "rac1", "ntsc", "SCUS_971.99", "").unwrap();
        assert_eq!(selected(&home, "rac1", "ntsc").unwrap(), None);
    }

    #[test]
    fn selection_rejects_other_discs_and_folders_without_replacements() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().join("textures");
        let folder = root.join("SCES-50916/replacements");
        std::fs::create_dir_all(&folder).unwrap();
        let home = temp.path().join("home");
        assert!(select(&home, "rac1", "pal", "SCES_509.16", root.to_str().unwrap()).is_err());
        std::fs::write(folder.join("abc-def-00001153.png"), b"PNG").unwrap();
        select(&home, "rac1", "pal", "SCES_509.16", root.to_str().unwrap()).unwrap();
        for source in [&root, &folder] {
            assert!(select(&home, "rac1", "ntsc", "SCUS_971.99", source.to_str().unwrap()).is_err());
        }
        assert_eq!(selected(&home, "rac1", "ntsc").unwrap(), None);
    }
}
