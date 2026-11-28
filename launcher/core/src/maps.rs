//! Shared maps (`.openrac-map`, docs/port/SHARED_MAPS.md): a level made in OpenRAC Editor, ready to
//! play, without the game's own bytes. Adding one makes its planet's pack from the player's own
//! disc and extracted levels and installs it beside the other packs ([`crate::overrides`]): no
//! editor, no Python, a few seconds.
//!
//! The map's `map.json` lists the pack's files. Each file is made of parts, in order:
//!
//! - `["data", at, length]`: bytes of the map's own (its `data/<file>.bin`);
//! - `["copy", source, at, length]`: bytes of one of the player's files (`sources`);
//! - `["wad", [header parts], [parts]]`: the bytes the parts make, packed as the game's WAD
//!   stream ([`crate::wad`]);
//! - `["point", source, at, count, matrix]` and `["normal", ...]`: `count` positions or normals
//!   (three floats each) of a source, moved by a 4 x 4 matrix (16 doubles as hex, column-major; a
//!   normal's is the inverse, and it is made one long);
//! - `["slot", source, at, count]`: `count` floats of a source, each followed by a zero;
//! - `["index", source, at, count, width, flip]`: `count` indices of `width` bytes as 32-bit ones,
//!   each triangle's last two swapped when `flip` is 1.
//!
//! A source is a file of the player's extracted levels (`{"port": "level_02/ties/tie_586.glb"}`),
//! sectors of the disc (`{"disc": [first, count]}`), or the WAD stream at a place in another source,
//! unpacked (`{"wad": source, "at": offset}`); each has its SHA-256, and so has each file made.

use std::collections::HashMap;
use std::io::Read;
use std::path::{Path, PathBuf};

use serde_json::Value;
use sha2::{Digest, Sha256};

/// The extension of a shared map. A browser may add `.zip` after it.
pub const SUFFIX: &str = ".openrac-map";
const MANIFEST: &str = "map.json";
const FORMAT: &str = "openrac-map";
const VERSION: u64 = 2;
const SECTOR: u64 = 2048;
/// Far above any real map, far below what fills a disk.
const MOST_BYTES: u64 = 2 << 30;
const MOST_FILES: usize = 20_000;

/// Whether `path` names a shared map (`.openrac-map`, or `.openrac-map.zip` as Android saves it).
pub fn is_map(path: &Path) -> bool {
    let name = path.file_name().and_then(|n| n.to_str()).unwrap_or_default().to_ascii_lowercase();
    name.ends_with(SUFFIX) || name.ends_with(".openrac-map.zip")
}

/// Where a map's game files come from on the player's side.
pub struct Game<'a> {
    /// The extracted levels of the disc's version (`assets/port/rac1/<version>`).
    pub levels: &'a Path,
    /// The disc's files, `<data>/iso_data/<game>` (its `sectors.tsv`).
    pub disc: &'a Path,
    /// The version the player set up: "pal", "ntsc", "pal1", "jp".
    pub version: &'a str,
}

/// A map that was added.
#[derive(Debug, Clone, serde::Serialize, PartialEq, Eq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct MapAdded {
    /// Its pack's id in the overrides folder.
    pub id: String,
    pub name: String,
    pub author: String,
    pub credits: Vec<String>,
    /// Whether it took the place of an earlier copy of the same map.
    pub replaced: bool,
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

fn sha(data: &[u8]) -> String {
    hex(&Sha256::digest(data))
}

/// The disc's sectors, read through `sectors.tsv` (first sector, sectors, bytes, file) as the port
/// reads them.
struct Disc {
    folder: PathBuf,
    rows: Vec<(u64, u64, u64, String)>,
}

impl Disc {
    fn open(folder: &Path) -> Result<Disc, String> {
        let path = folder.join(crate::install::SECTOR_MAP);
        let text = std::fs::read_to_string(&path)
            .map_err(|_| "the disc is not set up: set it up first (Setup)".to_string())?;
        let mut rows = Vec::new();
        for line in text.lines().filter(|l| !l.starts_with('#') && !l.trim().is_empty()) {
            let cols: Vec<&str> = line.split('\t').collect();
            let num = |i: usize| cols.get(i).and_then(|c| c.trim().parse::<u64>().ok());
            match (num(0), num(1), num(2), cols.get(3)) {
                (Some(first), Some(count), Some(bytes), Some(file)) => {
                    rows.push((first, count, bytes, file.to_string()))
                }
                _ => return Err(format!("{} cannot be read", path.display())),
            }
        }
        rows.sort_by_key(|r| r.0);
        Ok(Disc { folder: folder.to_path_buf(), rows })
    }

    fn read(&self, first: u64, count: u64) -> Result<Vec<u8>, String> {
        let mut out = vec![0u8; (count * SECTOR) as usize];
        let end = first + count;
        let mut covered = 0u64;
        for (start, sectors, bytes, file) in &self.rows {
            let (a, b) = ((*start).max(first), (start + sectors).min(end));
            if a >= b {
                continue;
            }
            let path = self.folder.join(file);
            let mut f = std::fs::File::open(&path).map_err(|e| format!("{}: {e}", path.display()))?;
            let skip = (a - start) * SECTOR;
            let want = ((b - a) * SECTOR).min(bytes.saturating_sub(skip));
            use std::io::Seek;
            f.seek(std::io::SeekFrom::Start(skip)).map_err(|e| e.to_string())?;
            let at = ((a - first) * SECTOR) as usize;
            f.read_exact(&mut out[at..at + want as usize]).map_err(|e| format!("{}: {e}", path.display()))?;
            covered += b - a;
        }
        if covered != count {
            return Err(format!("the disc has no sectors {first}..{end}: set the disc up again"));
        }
        Ok(out)
    }
}

/// The map's sources, read when a part first needs them, each checked against its SHA-256.
struct Sources<'a> {
    list: &'a [Value],
    game: &'a Game<'a>,
    disc: Option<Disc>,
    read: HashMap<usize, std::rc::Rc<Vec<u8>>>,
}

impl Sources<'_> {
    fn get(&mut self, index: u64) -> Result<std::rc::Rc<Vec<u8>>, String> {
        let i = index as usize;
        if let Some(data) = self.read.get(&i) {
            return Ok(data.clone());
        }
        let source = self.list.get(i).ok_or("the map names a source it does not list")?;
        let data = if let Some(rel) = source.get("port").and_then(Value::as_str) {
            let path = safe_join(self.game.levels, rel)?;
            std::fs::read(&path).map_err(|_| {
                format!("your extracted levels have no {rel}: extract the levels again (Setup), or update OpenRAC")
            })?
        } else if let Some(range) = source.get("disc").and_then(Value::as_array) {
            let num = |k: usize| range.get(k).and_then(Value::as_u64).ok_or("the map names disc sectors oddly");
            let (first, count) = (num(0)?, num(1)?);
            if count * SECTOR > MOST_BYTES {
                return Err("the map names too much of the disc".into());
            }
            if self.disc.is_none() {
                self.disc = Some(Disc::open(self.game.disc)?);
            }
            self.disc.as_ref().unwrap().read(first, count)?
        } else if let Some(of) = source.get("wad").and_then(Value::as_u64) {
            if of as usize >= i {
                return Err("the map's sources go round in a circle".into());
            }
            let at = source.get("at").and_then(Value::as_u64).unwrap_or(0) as usize;
            let outer = self.get(of)?;
            if at >= outer.len() {
                return Err("the map names a WAD stream past the end of its file".into());
            }
            crate::wad::decode(&outer[at..], MOST_BYTES as usize)?
        } else {
            return Err("the map has a kind of source this OpenRAC does not know: update OpenRAC".into());
        };
        let wanted = source.get("sha256").and_then(Value::as_str).unwrap_or_default();
        if sha(&data) != wanted {
            return Err(match source.get("port").and_then(Value::as_str) {
                Some(rel) => format!(
                    "your extracted levels' {rel} is not the one this map was made with: update OpenRAC and extract the levels again"
                ),
                None => "your disc differs from the one this map was made with".to_string(),
            });
        }
        let data = std::rc::Rc::new(data);
        self.read.insert(i, data.clone());
        Ok(data)
    }
}

/// `root/rel`, refusing a path out of `root`.
fn safe_join(root: &Path, rel: &str) -> Result<PathBuf, String> {
    let ok = !rel.is_empty()
        && !rel.starts_with('/')
        && !rel.contains('\\')
        && !rel.contains(':')
        && rel.split('/').all(|p| !p.is_empty() && p != "." && p != "..");
    if !ok {
        return Err(format!("the map names a file outside its place ({rel})"));
    }
    Ok(root.join(rel))
}

fn matrix(value: &Value) -> Result<[f64; 16], String> {
    let text = value.as_str().filter(|t| t.len() == 16 * 16).ok_or("the map has a damaged matrix")?;
    let mut m = [0f64; 16];
    for (i, item) in m.iter_mut().enumerate() {
        let bits = u64::from_str_radix(&text[i * 16..i * 16 + 16], 16).map_err(|_| "the map has a damaged matrix")?;
        *item = f64::from_bits(bits);
    }
    Ok(m)
}

fn slice(data: &[u8], at: u64, length: u64) -> Result<&[u8], String> {
    let (a, n) = (at as usize, length as usize);
    data.get(a..a.checked_add(n).ok_or("a part of the map is too long")?)
        .ok_or_else(|| "a part of the map reads past the end of its file".to_string())
}

fn f32s(data: &[u8], at: u64, count: u64) -> Result<Vec<f32>, String> {
    Ok(slice(data, at, count * 4)?.chunks_exact(4).map(|c| f32::from_le_bytes([c[0], c[1], c[2], c[3]])).collect())
}

/// What `parts` make, appended to `out`.
fn build(parts: &[Value], own: &[u8], sources: &mut Sources, out: &mut Vec<u8>) -> Result<(), String> {
    for part in parts {
        let items = part.as_array().ok_or("the map has a damaged part")?;
        let kind = items.first().and_then(Value::as_str).unwrap_or_default();
        let num = |k: usize| items.get(k).and_then(Value::as_u64).ok_or("the map has a damaged part");
        if out.len() as u64 > MOST_BYTES {
            return Err("the map makes a file too big".into());
        }
        match kind {
            "data" => out.extend_from_slice(slice(own, num(1)?, num(2)?)?),
            "copy" => {
                let data = sources.get(num(1)?)?;
                out.extend_from_slice(slice(&data, num(2)?, num(3)?)?);
            }
            "wad" => {
                let list = |k: usize| items.get(k).and_then(Value::as_array).ok_or("the map has a damaged part");
                let mut header = Vec::new();
                build(list(1)?, own, sources, &mut header)?;
                let mut inner = Vec::new();
                build(list(2)?, own, sources, &mut inner)?;
                out.extend(crate::wad::compress(&inner, Some(&header))?);
            }
            "point" | "normal" => {
                let data = sources.get(num(1)?)?;
                let m = matrix(items.get(4).unwrap_or(&Value::Null))?;
                let values = f32s(&data, num(2)?, num(3)? * 3)?;
                for p in values.chunks_exact(3) {
                    let (x, y, z) = (p[0] as f64, p[1] as f64, p[2] as f64);
                    let v: [f64; 3] = if kind == "point" {
                        // OpenRAC Editor's gltf_read.point, in its order.
                        std::array::from_fn(|r| m[r] * x + m[4 + r] * y + m[8 + r] * z + m[12 + r])
                    } else {
                        // static_assets.triangles' normal: the inverse's rows, made one long.
                        let v: [f64; 3] =
                            std::array::from_fn(|r| 0.0 + m[r * 4] * x + m[r * 4 + 1] * y + m[r * 4 + 2] * z);
                        let length = (0.0 + v[0] * v[0] + v[1] * v[1] + v[2] * v[2]).sqrt();
                        if length != 0.0 {
                            [v[0] / length, v[1] / length, v[2] / length]
                        } else {
                            [0.0, 0.0, 1.0]
                        }
                    };
                    for c in v {
                        out.extend_from_slice(&(c as f32).to_le_bytes());
                    }
                }
            }
            "slot" => {
                let data = sources.get(num(1)?)?;
                for v in f32s(&data, num(2)?, num(3)?)? {
                    out.extend_from_slice(&v.to_le_bytes());
                    out.extend_from_slice(&0f32.to_le_bytes());
                }
            }
            "index" => {
                let data = sources.get(num(1)?)?;
                let (at, count, width, flip) = (num(2)?, num(3)?, num(4)?, num(5)? == 1);
                if !matches!(width, 1 | 2 | 4) || (flip && count % 3 != 0) {
                    return Err("the map has a damaged part".into());
                }
                let bytes = slice(&data, at, count * width)?;
                let mut values: Vec<u32> = bytes
                    .chunks_exact(width as usize)
                    .map(|c| match width {
                        1 => c[0] as u32,
                        2 => u16::from_le_bytes([c[0], c[1]]) as u32,
                        _ => u32::from_le_bytes([c[0], c[1], c[2], c[3]]),
                    })
                    .collect();
                if flip {
                    for t in values.chunks_exact_mut(3) {
                        t.swap(1, 2);
                    }
                }
                for v in values {
                    out.extend_from_slice(&v.to_le_bytes());
                }
            }
            _ => return Err("the map has a kind of part this OpenRAC does not know: update OpenRAC".into()),
        }
    }
    Ok(())
}

fn read_member(archive: &mut zip::ZipArchive<std::fs::File>, name: &str) -> Result<Vec<u8>, String> {
    let mut entry = archive.by_name(name).map_err(|_| format!("the map has no {name}"))?;
    if entry.size() > MOST_BYTES {
        return Err("the map is too big".into());
    }
    let mut data = Vec::with_capacity(entry.size() as usize);
    entry.by_ref().take(MOST_BYTES).read_to_end(&mut data).map_err(|e| format!("the map is damaged ({e})"))?;
    Ok(data)
}

/// What the map says about itself (`map.json` without its files), for the page before adding it.
pub fn about(map: &Path) -> Result<Value, String> {
    let (manifest, _) = open(map)?;
    let mut about = manifest;
    if let Some(o) = about.as_object_mut() {
        o.remove("files");
        o.remove("sources");
    }
    Ok(about)
}

fn open(map: &Path) -> Result<(Value, zip::ZipArchive<std::fs::File>), String> {
    let name = map.file_name().and_then(|n| n.to_str()).unwrap_or("the map").to_string();
    let file = std::fs::File::open(map).map_err(|e| format!("{}: {e}", map.display()))?;
    let mut archive = zip::ZipArchive::new(file).map_err(|_| format!("{name} is not an OpenRAC map"))?;
    let text = read_member(&mut archive, MANIFEST).map_err(|_| format!("{name} is not an OpenRAC map"))?;
    let manifest: Value = serde_json::from_slice(&text).map_err(|_| format!("{name} is not an OpenRAC map"))?;
    if manifest.get("format").and_then(Value::as_str) != Some(FORMAT) {
        return Err(format!("{name} is not an OpenRAC map"));
    }
    match manifest.get("version").and_then(Value::as_u64) {
        Some(VERSION) => {}
        Some(v) if v > VERSION => return Err(format!("{name} was made for a newer OpenRAC: update OpenRAC")),
        _ => return Err(format!("{name} was made by an older OpenRAC Editor: ask its maker to export it again")),
    }
    Ok((manifest, archive))
}

/// Adds the map at `map` to the overrides folder `overrides` (`<data>/overrides`), its files made
/// from the player's `game`. Adding the same map again replaces the earlier copy, in its place.
pub fn add(map: &Path, overrides: &Path, game: &Game) -> Result<MapAdded, String> {
    let (manifest, mut archive) = open(map)?;
    let text = |key: &str| manifest.get(key).and_then(Value::as_str).unwrap_or_default().to_string();
    let id = text("id");
    if !crate::overrides::valid_pack_id(&id) {
        return Err("the map has no usable id".into());
    }
    if text("game") != "rac1" {
        return Err("this map is for another game".into());
    }
    let disc = text("disc");
    if disc != game.version {
        return Err(format!(
            "this map was made from the {} disc; yours is {}: set that disc up to play it",
            disc.to_uppercase(),
            game.version.to_uppercase()
        ));
    }
    let files = manifest.get("files").and_then(Value::as_array).ok_or("the map lists no files")?;
    if files.len() > MOST_FILES {
        return Err("the map is too big".into());
    }
    let empty = Vec::new();
    let list = manifest.get("sources").and_then(Value::as_array).unwrap_or(&empty);
    let mut sources = Sources { list, game, disc: None, read: HashMap::new() };
    let packs = overrides.join("packs");
    std::fs::create_dir_all(&packs).map_err(|e| format!("{}: {e}", packs.display()))?;
    let staged = packs.join(format!(".{id}.adding"));
    let _ = std::fs::remove_dir_all(&staged);
    let made = (|| -> Result<(), String> {
        let mut total: u64 = 0;
        for (i, file) in files.iter().enumerate() {
            let rel = file.get("path").and_then(Value::as_str).unwrap_or_default();
            let target = safe_join(&staged, rel)?;
            let parts = file.get("parts").and_then(Value::as_array).ok_or("the map has a damaged file")?;
            let member = format!("data/{i}.bin");
            let own = if archive.index_for_name(&member).is_some() {
                read_member(&mut archive, &member)?
            } else {
                Vec::new()
            };
            let mut out = Vec::new();
            build(parts, &own, &mut sources, &mut out).map_err(|e| format!("{rel}: {e}"))?;
            if sha(&out) != file.get("sha256").and_then(Value::as_str).unwrap_or_default() {
                return Err(format!("{rel} did not come out as the map's maker made it: update OpenRAC"));
            }
            total += out.len() as u64;
            if total > MOST_BYTES {
                return Err("the map makes too many bytes".into());
            }
            if let Some(parent) = target.parent() {
                std::fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
            }
            std::fs::write(&target, &out).map_err(|e| format!("{}: {e}", target.display()))?;
        }
        if !staged.join(crate::overrides::MANIFEST).is_file() {
            return Err("the map has no pack manifest (openrac-mod.json)".into());
        }
        Ok(())
    })();
    if let Err(e) = made {
        let _ = std::fs::remove_dir_all(&staged);
        return Err(e);
    }
    let replaced = crate::overrides::install(overrides, &id, &staged)?;
    Ok(MapAdded {
        id,
        name: text("name"),
        author: text("author"),
        credits: manifest
            .get("credits")
            .and_then(Value::as_array)
            .map(|c| c.iter().filter_map(|v| v.as_str().map(str::to_string)).collect())
            .unwrap_or_default(),
        replaced,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    struct Fixture {
        _dir: tempfile::TempDir,
        root: PathBuf,
    }

    impl Fixture {
        fn new() -> Fixture {
            let dir = tempfile::tempdir().unwrap();
            let root = dir.path().to_path_buf();
            // The extracted levels and a disc of two sectors in one file.
            std::fs::create_dir_all(root.join("levels/level_02/ties")).unwrap();
            let floats: Vec<u8> = [1f32, 2.0, 3.0, 0.0, 0.0, 1.0].iter().flat_map(|v| v.to_le_bytes()).collect();
            std::fs::write(root.join("levels/level_02/ties/tie.bin"), &floats).unwrap();
            std::fs::create_dir_all(root.join("iso")).unwrap();
            let mut disc = vec![0u8; 2 * 2048];
            disc[..11].copy_from_slice(b"hello disc!");
            let stream = crate::wad::compress(b"the level's core, the level's core", None).unwrap();
            disc[2048..2048 + stream.len()].copy_from_slice(&stream);
            std::fs::write(root.join("iso/image.bin"), &disc).unwrap();
            std::fs::write(root.join("iso/sectors.tsv"), "# openrac sector map 1\n0\t2\t4096\timage.bin\n").unwrap();
            Fixture { _dir: dir, root }
        }

        fn game(&self) -> Game<'_> {
            Game {
                levels: Box::leak(self.root.join("levels").into_boxed_path()),
                disc: Box::leak(self.root.join("iso").into_boxed_path()),
                version: "pal",
            }
        }

        fn map(&self, manifest: &Value, data: &[(&str, &[u8])]) -> PathBuf {
            let path = self.root.join("test.openrac-map");
            let mut zip = zip::ZipWriter::new(std::fs::File::create(&path).unwrap());
            let options = zip::write::SimpleFileOptions::default();
            zip.start_file(MANIFEST, options).unwrap();
            zip.write_all(manifest.to_string().as_bytes()).unwrap();
            for (name, bytes) in data {
                zip.start_file(*name, options).unwrap();
                zip.write_all(bytes).unwrap();
            }
            zip.finish().unwrap();
            path
        }
    }

    fn mhex(m: &[f64; 16]) -> String {
        m.iter().map(|v| format!("{:016x}", v.to_bits())).collect()
    }

    #[test]
    fn a_map_is_made_from_the_players_files() {
        let f = Fixture::new();
        let disc = f.game();
        let tie = std::fs::read(f.root.join("levels/level_02/ties/tie.bin")).unwrap();
        let sector0 = {
            let mut s = vec![0u8; 2048];
            s[..11].copy_from_slice(b"hello disc!");
            s
        };
        let sector1 = std::fs::read(f.root.join("iso/image.bin")).unwrap()[2048..].to_vec();
        let core = crate::wad::decode(&sector1, 1 << 20).unwrap();
        let mut grown = core.clone();
        grown.extend_from_slice(b" and more");
        let moved = {
            let mut m = [0f64; 16];
            m[0] = 2.0;
            m[5] = 2.0;
            m[10] = 2.0;
            m[12] = 10.0;
            m[15] = 1.0;
            m
        };
        let mut expected_points = Vec::new();
        for v in [12f32, 4.0, 6.0, 10.0, 0.0, 2.0] {
            expected_points.extend_from_slice(&v.to_le_bytes());
        }
        let pack = b"{\"format\": 4, \"name\": \"Test\"}".to_vec();
        let header = sector1[..16].to_vec();
        let stream = crate::wad::compress(&grown, Some(&header)).unwrap();
        let data_core = b" and more".to_vec();
        let manifest = serde_json::json!({
            "format": "openrac-map", "version": 2, "id": "test-map", "name": "Test", "author": "Me",
            "credits": ["a palm"], "game": "rac1", "disc": "pal",
            "sources": [
                {"port": "level_02/ties/tie.bin", "sha256": sha(&tie)},
                {"disc": [0, 1], "sha256": sha(&sector0)},
                {"disc": [1, 1], "sha256": sha(&sector1)},
                {"wad": 2, "at": 0, "sha256": sha(&core)}
            ],
            "files": [
                {"path": "openrac-mod.json", "sha256": sha(&pack), "parts": [["data", 0, pack.len()]]},
                {"path": "level_11/hello.bin", "sha256": sha(b"hello"), "parts": [["copy", 1, 0, 5]]},
                {"path": "level_11/core_pal.bin", "sha256": sha(&stream),
                 "parts": [["wad", [["copy", 2, 0, 16]], [["copy", 3, 0, core.len()], ["data", 0, data_core.len()]]]]},
                {"path": "level_11/points.bin", "sha256": sha(&expected_points),
                 "parts": [["point", 0, 0, 2, mhex(&moved)]]}
            ]
        });
        let map = f.map(&manifest, &[("data/0.bin", &pack), ("data/2.bin", &data_core)]);
        let overrides = f.root.join("overrides");
        let added = add(&map, &overrides, &disc).unwrap();
        assert_eq!(added.id, "test-map");
        assert!(!added.replaced);
        let pack_dir = overrides.join("packs/test-map");
        assert_eq!(std::fs::read(pack_dir.join("level_11/hello.bin")).unwrap(), b"hello");
        assert_eq!(std::fs::read(pack_dir.join("level_11/core_pal.bin")).unwrap(), stream);
        assert_eq!(std::fs::read(pack_dir.join("level_11/points.bin")).unwrap(), expected_points);
        let views = crate::overrides::list(&overrides).unwrap();
        assert_eq!(views.len(), 1);
        assert!(views[0].enabled);
        // Again: the same pack, in its place.
        assert!(add(&map, &overrides, &disc).unwrap().replaced);
        assert_eq!(crate::overrides::list(&overrides).unwrap().len(), 1);
    }

    #[test]
    fn a_map_of_another_disc_or_changed_files_is_refused() {
        let f = Fixture::new();
        let disc = f.game();
        let base = |disc_key: &str, source_sha: &str| {
            serde_json::json!({
                "format": "openrac-map", "version": 2, "id": "m", "name": "M", "game": "rac1", "disc": disc_key,
                "sources": [{"port": "level_02/ties/tie.bin", "sha256": source_sha}],
                "files": [{"path": "openrac-mod.json", "sha256": sha(b"x"), "parts": [["copy", 0, 0, 1]]}]
            })
        };
        let map = f.map(&base("ntsc", ""), &[]);
        assert!(add(&map, &f.root.join("o"), &disc).unwrap_err().contains("NTSC disc"));
        let map = f.map(&base("pal", "00"), &[]);
        assert!(add(&map, &f.root.join("o"), &disc).unwrap_err().contains("extract the levels again"));
        assert!(!f.root.join("o/packs/m").exists());
        let mut escape = base("pal", "");
        escape["files"][0]["path"] = "../x".into();
        let map = f.map(&escape, &[]);
        assert!(add(&map, &f.root.join("o"), &disc).unwrap_err().contains("outside"));
        let mut old = base("pal", "");
        old["version"] = 1.into();
        let map = f.map(&old, &[]);
        assert!(add(&map, &f.root.join("o"), &disc).unwrap_err().contains("export it again"));
    }

    #[test]
    fn normals_and_indices_follow_the_editors_sums() {
        let f = Fixture::new();
        let tie = std::fs::read(f.root.join("levels/level_02/ties/tie.bin")).unwrap();
        let game = f.game();
        let list = vec![serde_json::json!({"port": "level_02/ties/tie.bin", "sha256": sha(&tie)})];
        let mut sources = Sources { list: &list, game: &game, disc: None, read: HashMap::new() };
        let mirror = {
            let mut m = [0f64; 16];
            m[0] = -1.0;
            m[5] = 1.0;
            m[10] = 1.0;
            m[15] = 1.0;
            m
        };
        let mut out = Vec::new();
        let parts =
            serde_json::json!([["normal", 0, 12, 1, mhex(&mirror)], ["slot", 0, 0, 1], ["index", 0, 0, 3, 1, 1]]);
        build(parts.as_array().unwrap(), &[], &mut sources, &mut out).unwrap();
        let floats: Vec<f32> = out[..20].chunks_exact(4).map(|c| f32::from_le_bytes(c.try_into().unwrap())).collect();
        assert_eq!(floats, vec![0.0, 0.0, 1.0, 1.0, 0.0]);
        let indices: Vec<u32> = out[20..].chunks_exact(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())).collect();
        assert_eq!(indices, vec![tie[0] as u32, tie[2] as u32, tie[1] as u32]);
    }

    #[test]
    fn a_map_file_is_known_by_its_name() {
        assert!(is_map(Path::new("/x/Museum.openrac-map")));
        assert!(is_map(Path::new("/x/museum.OPENRAC-MAP.zip")));
        assert!(!is_map(Path::new("/x/pack.zip")));
    }
}
