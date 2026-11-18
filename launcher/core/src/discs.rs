//! The player's folder of PlayStation 2 disc images: which of them are games
//! OpenRAC knows. An image is told by the name of its boot program (the
//! `BOOT2` line of its `SYSTEM.CNF`, the disc's serial), read from the image
//! itself, so its file name does not matter. Nothing is changed and nothing
//! leaves the machine; the set-up checks the boot program's bytes afterwards
//! (tools/extractor.py --validate). Images are .iso files or CHDs (crate::chd).

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::{Path, PathBuf};

use serde::Serialize;

use crate::catalog::Catalog;
use crate::chd::{is_chd, ChdImage};

const SECTOR: u64 = 2048;
/// Folders are looked into this deep, and no more images than this are opened.
const DEPTH: usize = 2;
const MOST: usize = 2000;

/// A disc image of a game OpenRAC knows.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct FoundDisc {
    pub path: PathBuf,
    /// As the catalogue writes it: `SCES_509.16`.
    pub serial: String,
    pub game: String,
    /// `rac1/pal`.
    pub version: String,
    pub title: String,
    pub region: String,
}

/// The images under `dir` that are a version in `catalog`, one for each version (the first found).
pub fn scan(dir: &Path, catalog: &Catalog) -> Vec<FoundDisc> {
    let mut images = Vec::new();
    collect(dir, DEPTH, &mut images);
    images.sort();
    let mut found: Vec<FoundDisc> = Vec::new();
    for path in images {
        let Some(id) = identity_of(&path) else { continue };
        let Some(version) = catalog.version_of_disc(None, &id.serial, id.version.as_deref()) else { continue };
        if !found.iter().any(|f| f.version == version.key) {
            found.push(FoundDisc {
                path: path.clone(),
                serial: id.serial.clone(),
                game: version.game.clone(),
                version: version.key.clone(),
                title: version.title.clone(),
                region: version.region.clone(),
            });
        }
    }
    found
}

/// What a disc's `SYSTEM.CNF` says it is: the serial it boots and its version (`VER`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DiscIdentity {
    pub serial: String,
    pub version: Option<String>,
}

fn collect(dir: &Path, depth: usize, out: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else { return };
    for entry in entries.filter_map(Result::ok) {
        if out.len() >= MOST {
            return;
        }
        let path = entry.path();
        let hidden = path.file_name().is_some_and(|n| n.to_string_lossy().starts_with('.'));
        if hidden {
            continue;
        }
        if path.is_dir() {
            if depth > 0 {
                collect(&path, depth - 1, out);
            }
        } else if path.extension().is_some_and(|e| e.eq_ignore_ascii_case("iso")) || is_chd(&path) {
            out.push(path);
        }
    }
}

/// The serial of a PlayStation 2 image: the boot program `SYSTEM.CNF` names, as `SCES_509.16`.
/// `None` when the file is not an ISO 9660 image with one (an .iso, or one in a CHD).
pub fn serial_of(image: &Path) -> Option<String> {
    identity_of(image).map(|id| id.serial)
}

/// The serial and disc version of a PlayStation 2 image (its `SYSTEM.CNF`'s `BOOT2` and `VER`).
pub fn identity_of(image: &Path) -> Option<DiscIdentity> {
    let text = if is_chd(image) {
        system_cnf_in(&mut ChdImage::open(image).ok()?)
    } else {
        system_cnf_in(&mut File::open(image).ok()?)
    }?;
    Some(DiscIdentity { serial: boot_serial(&text)?, version: disc_version(&text) })
}

/// An image read by byte offset.
trait Image {
    fn read_at(&mut self, offset: u64, buffer: &mut [u8]) -> Option<()>;
}

impl Image for File {
    fn read_at(&mut self, offset: u64, buffer: &mut [u8]) -> Option<()> {
        self.seek(SeekFrom::Start(offset)).ok()?;
        self.read_exact(buffer).ok()
    }
}

impl<F: Read + Seek> Image for ChdImage<F> {
    fn read_at(&mut self, offset: u64, buffer: &mut [u8]) -> Option<()> {
        ChdImage::read_at(self, offset, buffer).ok()
    }
}

fn system_cnf_in(file: &mut impl Image) -> Option<String> {
    // The primary volume descriptor, sector 16: "\x01CD001", the root directory's record at 156.
    let mut pvd = [0u8; SECTOR as usize];
    file.read_at(16 * SECTOR, &mut pvd)?;
    if &pvd[..6] != b"\x01CD001" {
        return None;
    }
    let root = &pvd[156..190];
    let extent = u64::from(u32::from_le_bytes(root[2..6].try_into().ok()?));
    let size = (u32::from_le_bytes(root[10..14].try_into().ok()?) as usize).min(64 * SECTOR as usize);
    let mut dir = vec![0u8; size];
    file.read_at(extent * SECTOR, &mut dir)?;

    let mut at = 0;
    while at + 33 < dir.len() {
        let len = dir[at] as usize;
        if len == 0 {
            at = (at / SECTOR as usize + 1) * SECTOR as usize; // records do not cross sectors
            continue;
        }
        if at + len > dir.len() {
            break;
        }
        let record = &dir[at..at + len];
        let name_len = record[32] as usize;
        let name = record.get(33..33 + name_len)?;
        if name.eq_ignore_ascii_case(b"SYSTEM.CNF;1") || name.eq_ignore_ascii_case(b"SYSTEM.CNF") {
            let extent = u64::from(u32::from_le_bytes(record[2..6].try_into().ok()?));
            let size = (u32::from_le_bytes(record[10..14].try_into().ok()?) as usize).min(4096);
            let mut text = vec![0u8; size];
            file.read_at(extent * SECTOR, &mut text)?;
            return Some(String::from_utf8_lossy(&text).into_owned());
        }
        at += len;
    }
    None
}

/// `VER = 2.00` gives `2.00`.
fn disc_version(system_cnf: &str) -> Option<String> {
    let line = system_cnf.lines().find(|l| l.trim_start().to_ascii_uppercase().starts_with("VER"))?;
    let value = line.split_once('=')?.1.trim();
    (!value.is_empty()).then(|| value.to_string())
}

/// `BOOT2 = cdrom0:\SCES_509.16;1` gives `SCES_509.16`.
fn boot_serial(system_cnf: &str) -> Option<String> {
    let line = system_cnf.lines().find(|l| l.trim_start().to_ascii_uppercase().starts_with("BOOT2"))?;
    let value = line.split_once('=')?.1.trim();
    let name = value.rsplit(['\\', ':', '/']).next()?;
    let name = name.split(';').next()?.trim();
    (!name.is_empty()).then(|| name.to_ascii_uppercase())
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::testing::checkout;

    /// A small ISO 9660 image: a volume descriptor, a root directory with SYSTEM.CNF, its text.
    pub(crate) fn image(path: &Path, system_cnf: &str) {
        std::fs::write(path, image_data(system_cnf)).unwrap();
    }

    pub(crate) fn image_data(system_cnf: &str) -> Vec<u8> {
        let mut data = vec![0u8; 20 * SECTOR as usize];
        let pvd = 16 * SECTOR as usize;
        data[pvd..pvd + 6].copy_from_slice(b"\x01CD001");
        let root = pvd + 156;
        data[root] = 34;
        data[root + 2..root + 6].copy_from_slice(&18u32.to_le_bytes());
        data[root + 10..root + 14].copy_from_slice(&(SECTOR as u32).to_le_bytes());
        let dir = 18 * SECTOR as usize;
        let name = b"SYSTEM.CNF;1";
        data[dir] = (33 + name.len()) as u8;
        data[dir + 2..dir + 6].copy_from_slice(&19u32.to_le_bytes());
        data[dir + 10..dir + 14].copy_from_slice(&(system_cnf.len() as u32).to_le_bytes());
        data[dir + 32] = name.len() as u8;
        data[dir + 33..dir + 33 + name.len()].copy_from_slice(name);
        let text = 19 * SECTOR as usize;
        data[text..text + system_cnf.len()].copy_from_slice(system_cnf.as_bytes());
        data
    }

    #[test]
    fn reads_the_serial_the_disc_boots() {
        assert_eq!(boot_serial("BOOT2 = cdrom0:\\SCES_509.16;1\r\nVER = 2.00\r\n"), Some("SCES_509.16".into()));
        assert_eq!(boot_serial("VER = 1.00\nBOOT2=cdrom0:\\scus_971.99;1"), Some("SCUS_971.99".into()));
        assert_eq!(boot_serial("BOOT = cdrom:\\PSX.EXE;1"), None);
        assert_eq!(
            disc_version("BOOT2 = cdrom0:\\SCUS_972.68;1 \r\nVER = 2.00 \r\nVMODE = NTSC \r\n"),
            Some("2.00".into())
        );
        assert_eq!(disc_version("BOOT2 = cdrom0:\\SCUS_972.68;1"), None);
    }

    #[test]
    fn tells_two_releases_of_one_serial_apart_by_the_disc_version() {
        let tmp = std::env::temp_dir().join(format!("openrac-discs-ver-{}", std::process::id()));
        std::fs::create_dir_all(&tmp).unwrap();
        image(&tmp.join("a.iso"), "BOOT2 = cdrom0:\\SCUS_972.68;1 \r\nVER = 2.00 \r\nVMODE = NTSC \r\n");
        image(&tmp.join("b.iso"), "BOOT2 = cdrom0:\\SCUS_972.68;1 \r\nVER = 1.01 \r\nVMODE = NTSC \r\n");
        image(&tmp.join("c.iso"), "BOOT2 = cdrom0:\\SCES_516.07;1 \r\nVER = 1.00 \r\nVMODE = PAL \r\n");
        image(&tmp.join("d.iso"), "BOOT2 = cdrom0:\\SCUS_972.68;1 \r\nVER = 9.99 \r\n");
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let found: Vec<(String, String)> = scan(&tmp, &catalog)
            .into_iter()
            .map(|f| (f.version, f.path.file_name().unwrap().to_string_lossy().into_owned()))
            .collect();
        let want = [("rac2/ntsc2", "a.iso"), ("rac2/ntsc", "b.iso"), ("rac2/pal", "c.iso")];
        assert_eq!(found, want.map(|(v, p)| (v.to_string(), p.to_string())));
        std::fs::remove_dir_all(&tmp).unwrap();
    }

    #[test]
    fn finds_ratchet_and_clanks_pal_v1_and_japanese_discs_as_versions_of_their_own() {
        let tmp = std::env::temp_dir().join(format!("openrac-discs-rac1-{}", std::process::id()));
        std::fs::create_dir_all(&tmp).unwrap();
        // Both PAL releases boot SCES_509.16; v1.00 says so in its VER. The Japanese disc has its own serial.
        image(&tmp.join("a.iso"), "BOOT2 = cdrom0:\\SCES_509.16;1 \r\nVER = 1.00 \r\nVMODE = PAL \r\n");
        image(&tmp.join("b.iso"), "BOOT2 = cdrom0:\\SCES_509.16;1 \r\nVER = 2.00 \r\nVMODE = PAL \r\n");
        image(&tmp.join("c.iso"), "BOOT2 = cdrom0:\\SCPS_150.37;1 \r\nVER = 1.02 \r\nVMODE = NTSC \r\n");
        image(&tmp.join("d.iso"), "BOOT2 = cdrom0:\\SCUS_971.99;1 \r\nVER = 1.00 \r\nVMODE = NTSC \r\n");
        // A release of the serial that no version here is.
        image(&tmp.join("e.iso"), "BOOT2 = cdrom0:\\SCES_509.16;1 \r\nVER = 3.00 \r\n");
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let found: Vec<(String, String)> = scan(&tmp, &catalog)
            .into_iter()
            .map(|f| (f.version, f.path.file_name().unwrap().to_string_lossy().into_owned()))
            .collect();
        let want = [("rac1/pal1", "a.iso"), ("rac1/pal", "b.iso"), ("rac1/jp", "c.iso"), ("rac1/ntsc", "d.iso")];
        assert_eq!(found, want.map(|(v, p)| (v.to_string(), p.to_string())));
        std::fs::remove_dir_all(&tmp).unwrap();
    }

    /// A real image, where this checkout has one (baserom/ is the contributor's own, never in git).
    #[test]
    fn reads_a_real_image() {
        let image = checkout().join("baserom").join("SCES_509.16.iso");
        if image.is_file() {
            assert_eq!(serial_of(&image), Some("SCES_509.16".into()));
        }
    }

    #[test]
    fn finds_the_games_it_knows_whatever_their_files_are_called() {
        let tmp = std::env::temp_dir().join(format!("openrac-discs-{}", std::process::id()));
        std::fs::create_dir_all(tmp.join("PS2").join("Ratchet")).unwrap();
        image(
            &tmp.join("PS2").join("Ratchet").join("my backup (1).iso"),
            "BOOT2 = cdrom0:\\SCES_509.16;1\r\nVER = 2.00\r\n",
        );
        image(&tmp.join("another game.ISO"), "BOOT2 = cdrom0:\\SLES_123.45;1\r\n");
        let chd = crate::chd::tests::chd_of(&image_data("BOOT2 = cdrom0:\\SCUS_971.99;1\r\n"), 8 * SECTOR as u32);
        std::fs::write(tmp.join("Ratchet & Clank (USA).chd"), chd).unwrap();
        std::fs::write(tmp.join("notes.iso"), b"not an image").unwrap();
        std::fs::write(tmp.join("notes.chd"), b"not an image").unwrap();
        std::fs::write(tmp.join("readme.txt"), b"").unwrap();

        let catalog = crate::catalog::load(&checkout()).unwrap();
        let found = scan(&tmp, &catalog);
        assert_eq!(found.len(), 2, "{found:?}");
        assert_eq!((found[0].version.as_str(), found[0].serial.as_str()), ("rac1/pal", "SCES_509.16"));
        assert!(found[0].path.ends_with("my backup (1).iso"));
        assert_eq!((found[1].version.as_str(), found[1].serial.as_str()), ("rac1/ntsc", "SCUS_971.99"));
        assert!(found[1].path.ends_with("Ratchet & Clank (USA).chd"));
        assert_eq!(serial_of(&tmp.join("notes.iso")), None);
        assert_eq!(serial_of(&tmp.join("notes.chd")), None);
        std::fs::remove_dir_all(&tmp).unwrap();
    }
}
