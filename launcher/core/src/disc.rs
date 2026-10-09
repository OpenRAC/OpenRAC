//! Adding the user's own disc image to the checkout.
//!
//! A player picks the image of their disc; the launcher reads which game it
//! is from the image itself (the serial in its `SYSTEM.CNF`) and puts it
//! where the tools look, `baserom/<name>.iso`: a link on Unix, so that a
//! 4 GB file is not copied, a copy elsewhere. Nothing is downloaded and the
//! image is never changed.

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::{Path, PathBuf};

use crate::catalog::Version;

/// An ISO 9660 sector.
const SECTOR: u64 = 2048;

/// Reads `count` bytes of `file` from byte `at`.
fn read_at(file: &mut File, at: u64, count: usize) -> Result<Vec<u8>, String> {
    let mut bytes = vec![0; count];
    file.seek(SeekFrom::Start(at)).map_err(|e| e.to_string())?;
    file.read_exact(&mut bytes).map_err(|_| "it is too short to be a disc image".to_string())?;
    Ok(bytes)
}

fn u32_le(bytes: &[u8], at: usize) -> u64 {
    u64::from(u32::from_le_bytes([bytes[at], bytes[at + 1], bytes[at + 2], bytes[at + 3]]))
}

/// The serial of a PlayStation 2 disc image, as game.json writes it
/// (`SCES_509.16`): the program its `SYSTEM.CNF` boots.
pub fn serial(image: &Path) -> Result<String, String> {
    let mut file = File::open(image).map_err(|e| format!("cannot open it: {e}"))?;
    // The primary volume descriptor is sector 16; its root directory record is at byte 156.
    let volume = read_at(&mut file, 16 * SECTOR, SECTOR as usize)?;
    if &volume[1..6] != b"CD001" {
        return Err("it is not a disc image (no ISO 9660 volume)".into());
    }
    let (root_sector, root_size) = (u32_le(&volume, 156 + 2), u32_le(&volume, 156 + 10));
    // A root directory is a few sectors; a huge size means a damaged image.
    if root_size > 64 * SECTOR {
        return Err("its root directory is damaged".into());
    }
    let records = read_at(&mut file, root_sector * SECTOR, root_size as usize)?;
    let mut at = 0;
    while at + 34 <= records.len() {
        let length = records[at] as usize;
        if length == 0 {
            // Records do not cross sectors: go on at the next one.
            at = (at / SECTOR as usize + 1) * SECTOR as usize;
            continue;
        }
        let name_length = records[at + 32] as usize;
        let name = records.get(at + 33..at + 33 + name_length).unwrap_or_default();
        if name.split(|b| *b == b';').next() == Some(b"SYSTEM.CNF".as_slice()) {
            let (sector, size) = (u32_le(&records, at + 2), u32_le(&records, at + 10));
            let text = read_at(&mut file, sector * SECTOR, size.min(SECTOR) as usize)?;
            return boot_serial(&String::from_utf8_lossy(&text));
        }
        at += length;
    }
    Err("it has no SYSTEM.CNF: not a PlayStation 2 game disc".into())
}

/// The serial in the text of a `SYSTEM.CNF`: `BOOT2 = cdrom0:\SCES_509.16;1`.
fn boot_serial(text: &str) -> Result<String, String> {
    for line in text.lines() {
        let Some((key, value)) = line.split_once('=') else { continue };
        if key.trim() != "BOOT2" {
            continue;
        }
        let program = value.trim().rsplit(['\\', ':']).next().unwrap_or_default();
        let serial = program.split(';').next().unwrap_or_default().trim();
        if !serial.is_empty() {
            return Ok(serial.to_string());
        }
    }
    Err("its SYSTEM.CNF names no program".into())
}

/// Puts `image` where the tools look for `version`'s disc. Returns the place.
///
/// Refused when the image is another game's or another version's, or when a
/// disc for the version is already there.
pub fn add(root: &Path, version: &Version, image: &Path) -> Result<PathBuf, String> {
    let name = image.file_name().map(|n| n.to_string_lossy().into_owned()).unwrap_or_default();
    let found = serial(image).map_err(|why| format!("{name}: {why}"))?;
    if found != version.serial {
        return Err(format!(
            "{name} is {found}, not {} ({}, {}). Pick that game in the launcher, or another image.",
            version.serial, version.title, version.region
        ));
    }
    let spec = version.disc.as_ref().ok_or_else(|| format!("{} names no disc image", version.key))?;
    let place = root.join("baserom").join(&spec.file);
    if place.exists() || place.is_symlink() {
        return Err(format!("baserom/{} is already there", spec.file));
    }
    std::fs::create_dir_all(root.join("baserom")).map_err(|e| format!("cannot create baserom/: {e}"))?;
    let image = std::fs::canonicalize(image).map_err(|e| e.to_string())?;
    link_or_copy(&image, &place).map_err(|e| format!("cannot add it as baserom/{}: {e}", spec.file))?;
    Ok(place)
}

#[cfg(unix)]
fn link_or_copy(image: &Path, place: &Path) -> std::io::Result<()> {
    std::os::unix::fs::symlink(image, place)
}

/// Windows needs a privilege for links to files; a hard link works on the
/// same drive, and a copy anywhere.
#[cfg(not(unix))]
fn link_or_copy(image: &Path, place: &Path) -> std::io::Result<()> {
    std::fs::hard_link(image, place).or_else(|_| std::fs::copy(image, place).map(|_| ()))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::catalog::FileSpec;

    /// A disc image with nothing in it but a volume descriptor, a root
    /// directory and a SYSTEM.CNF that boots `serial`.
    fn image(dir: &Path, name: &str, serial: &str) -> PathBuf {
        let mut bytes = vec![0u8; 20 * SECTOR as usize];
        let volume = 16 * SECTOR as usize;
        bytes[volume + 1..volume + 6].copy_from_slice(b"CD001");
        // The root directory: sector 18, one sector long.
        bytes[volume + 156 + 2..volume + 156 + 6].copy_from_slice(&18u32.to_le_bytes());
        bytes[volume + 156 + 10..volume + 156 + 14].copy_from_slice(&2048u32.to_le_bytes());
        let text = format!("BOOT2 = cdrom0:\\{serial};1\r\nVER = 1.00\r\n");
        let record = 18 * SECTOR as usize;
        let file_name = b"SYSTEM.CNF;1";
        bytes[record] = (33 + file_name.len()) as u8;
        bytes[record + 2..record + 6].copy_from_slice(&19u32.to_le_bytes());
        bytes[record + 10..record + 14].copy_from_slice(&(text.len() as u32).to_le_bytes());
        bytes[record + 32] = file_name.len() as u8;
        bytes[record + 33..record + 33 + file_name.len()].copy_from_slice(file_name);
        let content = 19 * SECTOR as usize;
        bytes[content..content + text.len()].copy_from_slice(text.as_bytes());
        let path = dir.join(name);
        std::fs::write(&path, bytes).unwrap();
        path
    }

    fn version() -> Version {
        Version {
            key: "rac9/pal".into(),
            game: "rac9".into(),
            name: "pal".into(),
            title: "Test".into(),
            region: "PAL".into(),
            serial: "SCES_000.00".into(),
            dir: "games/rac9/pal".into(),
            disc: Some(FileSpec { file: "SCES_000.00.iso".into(), size: None, sha1: None }),
            boot: None,
            inputs: vec![],
            target: None,
            setup: None,
            build_host: None,
            readme: None,
            source: None,
            progress: None,
        }
    }

    #[test]
    fn reads_the_serial_of_an_image() {
        let dir = tempfile::tempdir().unwrap();
        assert_eq!(serial(&image(dir.path(), "a.iso", "SCES_000.00")).unwrap(), "SCES_000.00");
        let not_a_disc = dir.path().join("b.iso");
        std::fs::write(&not_a_disc, vec![0u8; 20 * SECTOR as usize]).unwrap();
        assert!(serial(&not_a_disc).is_err());
        std::fs::write(&not_a_disc, b"short").unwrap();
        assert!(serial(&not_a_disc).is_err());
    }

    #[test]
    fn adds_the_right_disc_once_and_refuses_another_game() {
        let dir = tempfile::tempdir().unwrap();
        let root = dir.path().join("checkout");
        std::fs::create_dir_all(&root).unwrap();
        let other = image(dir.path(), "other.iso", "SCUS_111.11");
        let why = add(&root, &version(), &other).unwrap_err();
        assert!(why.contains("SCUS_111.11") && why.contains("SCES_000.00"), "{why}");

        let mine = image(dir.path(), "My Game.iso", "SCES_000.00");
        let place = add(&root, &version(), &mine).unwrap();
        assert_eq!(place, root.join("baserom").join("SCES_000.00.iso"));
        assert_eq!(std::fs::read(&place).unwrap(), std::fs::read(&mine).unwrap());
        assert!(add(&root, &version(), &mine).unwrap_err().contains("already there"));
    }
}
