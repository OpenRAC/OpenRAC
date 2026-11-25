//! What a mod keeps beside each save, for the save editor (docs/port/MODS.md, "A mod's save data").
//!
//! A native mod may keep its own state per save in a file of its data folder
//! (`<data home>/mod-data/<id>/`, openrac_api.data_dir), never on the memory card. When its
//! manifest describes that file under `save_data`, the save editor shows the fields of the record
//! of the save opened, while the mod is on, and writes them back:
//!
//! ```json
//! "save_data": {
//!   "file": "tiers.txt",
//!   "records": "clock-hex",
//!   "size": 56,
//!   "title": "Weapon tiers",
//!   "fields": [
//!     {"key": "lancer", "name": "Lancer", "offset": 30, "mask": 127, "max": 8,
//!      "choices": ["None", "Alpha", "Beta"]}
//!   ]
//! }
//! ```
//!
//! `records` is the file's layout; the one there is, `clock-hex`, has a line per save: the save's
//! clock (the 8 bytes the game stamps when it saves; Going Commando's chunk 4) as 16 hex digits, a
//! space, and the record's `size` bytes as hex. The last line of a clock is its record. A field is
//! one byte at `offset`; `mask` (255 when absent) the bits it owns, the others kept as they are;
//! its value goes from `min` to `max`, named by `choices` from 0 when it has them.
//!
//! A field may sit beside one of the editor's own elements instead (`attach`, [`Attach`]):
//! `{"to": "item", "id": 30, "as": "version"}` continues item 30's version picker with the
//! field's values above `min`, `"as": "row"` adds a control in its row; `"requires": {"version":
//! "last"}` ([`Requires`]) ties those values to the item's last game version.

use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

/// The most bytes a record may have.
pub const SIZE_MAX: usize = 4096;
/// The largest file read.
const FILE_MAX: u64 = 4 << 20;

fn default_mask() -> u8 {
    0xFF
}
fn default_max() -> u8 {
    0xFF
}

/// One field of a record, as the manifest describes it.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "ModSaveFieldSpec")]
pub struct Field {
    pub key: String,
    pub name: String,
    pub offset: usize,
    #[serde(default = "default_mask")]
    pub mask: u8,
    #[serde(default)]
    pub min: u8,
    #[serde(default = "default_max")]
    pub max: u8,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    #[ts(as = "Option<Vec<String>>", optional)]
    pub choices: Vec<String>,
    /// Where the editor shows the field: beside one of its own elements instead of the mod's
    /// section. A target the launcher does not know leaves the field in the mod's section.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub attach: Option<Attach>,
    /// What the field's values above `min` need of the native element it is attached to.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub requires: Option<Requires>,
}

/// A field's place among the editor's own elements (`"attach"`).
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, ts_rs::TS)]
#[ts(rename = "ModSaveAttach")]
pub struct Attach {
    /// The kind of element: `item` (an item of the game's list, by the game's item number).
    /// Other kinds are for later; the editor shows a field with one in the mod's section.
    pub to: String,
    /// The element's number (for `item`, the item id).
    pub id: i64,
    /// How it is shown: `version` continues the item's version picker with the field's values
    /// above `min` (named by `choices`); `row` adds a control in the item's row.
    #[serde(rename = "as")]
    #[ts(rename = "as")]
    pub role: String,
}

/// What a field's values above `min` need (`"requires"`).
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, ts_rs::TS)]
#[ts(rename = "ModSaveRequires")]
pub struct Requires {
    /// `last`: the item is at the game's last version. Picking such a value moves the item there;
    /// picking a game version below it puts the field back to `min`.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub version: Option<String>,
}

/// The kinds of element and ways of showing the launcher knows (see [`Attach`]).
pub const ATTACH_ITEM_AS: &[&str] = &["version", "row"];

/// The manifest's `save_data`.
#[derive(Debug, Clone, Deserialize, PartialEq, Default)]
pub struct SaveData {
    pub file: String,
    #[serde(default)]
    pub records: String,
    pub size: usize,
    #[serde(default)]
    pub title: String,
    #[serde(default)]
    pub fields: Vec<Field>,
}

impl SaveData {
    pub fn validate(&self) -> Result<(), String> {
        let plain = !self.file.is_empty()
            && self.file.len() <= 64
            && !self.file.starts_with('.')
            && self.file.chars().all(|c| c.is_ascii_alphanumeric() || matches!(c, '.' | '_' | '-'));
        if !plain {
            return Err(format!("save_data: {:?} is not a file name in the mod's data folder", self.file));
        }
        if self.records != "clock-hex" {
            return Err(format!(
                "save_data: records {:?} is not a layout the launcher knows (clock-hex)",
                self.records
            ));
        }
        if self.size == 0 || self.size > SIZE_MAX {
            return Err(format!("save_data: size {} is not 1 to {SIZE_MAX}", self.size));
        }
        let mut keys = std::collections::BTreeSet::new();
        for f in &self.fields {
            if f.key.is_empty() || !keys.insert(f.key.as_str()) {
                return Err(format!("save_data: field {:?} has no key, or the key of another", f.name));
            }
            if f.offset >= self.size {
                return Err(format!("save_data: field {} is past the record's {} bytes", f.key, self.size));
            }
            if f.min > f.max || f.max & !f.mask != 0 {
                return Err(format!("save_data: field {}'s values do not fit its mask", f.key));
            }
            if let Some(a) = &f.attach {
                if a.to.is_empty() {
                    return Err(format!("save_data: field {}'s attach names no element", f.key));
                }
                if a.to == "item" && !ATTACH_ITEM_AS.contains(&a.role.as_str()) {
                    return Err(format!(
                        "save_data: field {} attaches to an item as {:?}, not version or row",
                        f.key, a.role
                    ));
                }
            }
            if let Some(r) = &f.requires {
                let on_version = f.attach.as_ref().is_some_and(|a| a.to == "item" && a.role == "version");
                match r.version.as_deref() {
                    Some("last") if on_version => {}
                    Some("last") => {
                        return Err(format!(
                            "save_data: field {} requires a version but is not on a version picker",
                            f.key
                        ))
                    }
                    Some(v) => return Err(format!("save_data: field {} requires version {v:?}, not last", f.key)),
                    None => {}
                }
            }
        }
        Ok(())
    }
}

/// A field with its value in the save's record.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModSaveField {
    #[serde(flatten)]
    #[ts(flatten)]
    pub spec: Field,
    pub value: u8,
}

/// One mod's record of one save.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModSaveData {
    pub id: String,
    pub name: String,
    pub title: String,
    /// Whether the file has a record of this save; without one the fields read 0 and a write adds
    /// it.
    pub found: bool,
    pub fields: Vec<ModSaveField>,
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

fn unhex(text: &str) -> Option<Vec<u8>> {
    if !text.len().is_multiple_of(2) {
        return None;
    }
    (0..text.len()).step_by(2).map(|i| u8::from_str_radix(text.get(i..i + 2)?, 16).ok()).collect()
}

/// The line of `clock` in `text` (the last), and its record.
fn find(text: &str, clock: &[u8; 8], size: usize) -> Option<(usize, Vec<u8>)> {
    let key = hex(clock);
    let mut out = None;
    for (n, line) in text.lines().enumerate() {
        let Some((k, data)) = line.trim_end().split_once(' ') else { continue };
        if !k.eq_ignore_ascii_case(&key) {
            continue;
        }
        if let Some(mut bytes) = unhex(data) {
            if bytes.len() >= size {
                bytes.truncate(size);
                out = Some((n, bytes));
            }
        }
    }
    out
}

/// NG++'s tiers file (`ng-plus-plus`, `tiers.txt`): it has layouts of its own, marked by a first line
/// `ngpp-tiers N` (openrac-mods, ng-plus-plus/src/save_tiers.h). Layout 1 (no marker) had the RYNO II
/// at Heavy, Ultima, Alpha to Omega, Omega Prime; layout 2 the Zodiac at Alpha to Omega; layout 3 the
/// RYNO II at Heavy, Alpha to Omega, Ultima and the Zodiac at Heavy, Mega, Ultra, Alpha to Omega;
/// layout 4, the one now, has both at Heavy, Mega, Ultra, Alpha to Omega, Ultima. The editor reads
/// layouts 1 to 4 and always writes layout 4.
const NGPP_ID: &str = "ng-plus-plus";
const NGPP_MARKER: &str = "ngpp-tiers";
const NGPP_LAYOUT: u32 = 4;
const NGPP_ZODIAC: usize = 43;
const NGPP_RYNO: usize = 44;
/// The Zodiac's steps before Alpha in layouts 3 and 4.
const NGPP_ZODIAC_OWN: u8 = 3;
/// The steps the RYNO II's Mega and Ultra put before its Alpha in layout 4.
const NGPP_RYNO_NEW: u8 = 2;
/// The Zodiac's and the RYNO II's steps in layout 4 (level 0 is none).
const NGPP_CHOICES: [&str; 13] =
    ["None", "Heavy", "Mega", "Ultra", "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Omega", "Ultima"];

fn is_ngpp(id: &str, spec: &SaveData) -> bool {
    id == NGPP_ID && spec.file == "tiers.txt" && spec.records == "clock-hex" && spec.size > NGPP_RYNO
}

/// The spec with the Zodiac's and the RYNO II's path of layout 4, whatever an older manifest lists.
fn effective(id: &str, spec: &SaveData) -> SaveData {
    let mut out = spec.clone();
    if is_ngpp(id, spec) {
        for f in out.fields.iter_mut().filter(|f| (f.offset == NGPP_ZODIAC || f.offset == NGPP_RYNO) && f.mask == 0x7F)
        {
            f.choices = NGPP_CHOICES.iter().map(|c| c.to_string()).collect();
            f.max = (NGPP_CHOICES.len() - 1) as u8;
        }
    }
    out
}

fn marker_of(line: &str) -> Option<u32> {
    line.trim().strip_prefix(NGPP_MARKER)?.trim().parse().ok()
}

fn is_record(line: &str, size: usize) -> bool {
    line.trim_end()
        .split_once(' ')
        .is_some_and(|(k, d)| k.len() == 16 && unhex(k).is_some() && unhex(d.trim()).is_some_and(|b| b.len() >= size))
}

/// The layout of a tiers file: its marker's, or 1 with records and none; the newest when empty.
fn ngpp_layout(text: &str, size: usize) -> u32 {
    if let Some(n) = text.lines().find_map(marker_of) {
        return n.clamp(1, NGPP_LAYOUT);
    }
    if text.lines().any(|l| is_record(l, size)) {
        1
    } else {
        NGPP_LAYOUT
    }
}

/// `text` in layout 4: the RYNO II's levels and (before layout 3) the Zodiac's moved over, the marker first and
/// alone. Lines that are no record stay in their order.
fn ngpp_to_current(text: &str, size: usize) -> String {
    let layout = ngpp_layout(text, size);
    let mut out = vec![format!("{NGPP_MARKER} {NGPP_LAYOUT}")];
    for line in text.lines() {
        if marker_of(line).is_some() {
            continue;
        }
        if layout < NGPP_LAYOUT && is_record(line, size) {
            let (k, d) = line.trim_end().split_once(' ').expect("a record has a space");
            let mut b = unhex(d.trim()).expect("a record is hex");
            let started = |v: u8| v & 0x80;
            let level = |v: u8| v & 0x7F;
            if layout < 2 {
                let l = level(b[NGPP_RYNO]);
                // Old: 1 Heavy, 2 Ultima, 3..10 Alpha..Omega, 11 Omega Prime. Layout 3's Ultima is 10.
                let now = if l < 2 {
                    l
                } else if l == 2 {
                    2
                } else {
                    l - 1
                };
                b[NGPP_RYNO] = started(b[NGPP_RYNO]) | now;
            }
            // Layouts 1 to 3: the Mega and Ultra come after Heavy, so Alpha (2) and up move up two.
            let l = level(b[NGPP_RYNO]);
            b[NGPP_RYNO] = started(b[NGPP_RYNO]) | if l < 2 { l } else { l + NGPP_RYNO_NEW };
            // Layouts 1 and 2: the Zodiac's Mega and Ultra come before Alpha (layout 3 has them).
            if layout < 3 {
                let z = level(b[NGPP_ZODIAC]);
                b[NGPP_ZODIAC] = started(b[NGPP_ZODIAC]) | if z == 0 { 0 } else { z + NGPP_ZODIAC_OWN };
            }
            out.push(format!("{k} {}", hex(&b)));
        } else {
            out.push(line.to_string());
        }
    }
    let mut s = out.join("\n");
    s.push('\n');
    s
}

fn current_text(id: &str, spec: &SaveData, text: String) -> String {
    if is_ngpp(id, spec) {
        ngpp_to_current(&text, spec.size)
    } else {
        text
    }
}

fn file_of(mod_data: &Path, id: &str, spec: &SaveData) -> Result<PathBuf, String> {
    if !crate::plugins::valid_id(id) {
        return Err(format!("{id:?} is not a mod's id"));
    }
    spec.validate()?;
    Ok(mod_data.join(id).join(&spec.file))
}

fn read_text(path: &Path) -> Result<String, String> {
    match fs::metadata(path) {
        Err(_) => Ok(String::new()),
        Ok(m) if m.len() > FILE_MAX => Err(format!("{} is larger than {} MB", path.display(), FILE_MAX >> 20)),
        Ok(_) => fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display())),
    }
}

/// Mod `id`'s record of the save stamped `clock`, from `<mod_data>/<id>/<file>`.
pub fn read(mod_data: &Path, id: &str, name: &str, spec: &SaveData, clock: &[u8; 8]) -> Result<ModSaveData, String> {
    let text = read_text(&file_of(mod_data, id, spec)?)?;
    let text = current_text(id, spec, text);
    let spec = &effective(id, spec);
    let found = find(&text, clock, spec.size);
    let record = found.as_ref().map(|(_, r)| r.clone()).unwrap_or_else(|| vec![0; spec.size]);
    let fields =
        spec.fields.iter().map(|f| ModSaveField { spec: f.clone(), value: record[f.offset] & f.mask }).collect();
    Ok(ModSaveData { id: id.into(), name: name.into(), title: spec.title.clone(), found: found.is_some(), fields })
}

/// Writes `values` (by field key) into mod `id`'s record of the save stamped `clock`: its line
/// replaced, or added at the end; every other line as it was; the file replaced in one rename.
pub fn write(
    mod_data: &Path,
    id: &str,
    spec: &SaveData,
    clock: &[u8; 8],
    values: &BTreeMap<String, u8>,
) -> Result<(), String> {
    let path = file_of(mod_data, id, spec)?;
    let text = current_text(id, spec, read_text(&path)?);
    let spec = &effective(id, spec);
    let found = find(&text, clock, spec.size);
    let mut record = found.as_ref().map(|(_, r)| r.clone()).unwrap_or_else(|| vec![0; spec.size]);
    for (key, &value) in values {
        let f = spec.fields.iter().find(|f| &f.key == key).ok_or_else(|| format!("{id} has no field {key}"))?;
        if value < f.min || value > f.max {
            return Err(format!("{}: {value} is not {} to {}", f.name, f.min, f.max));
        }
        record[f.offset] = (record[f.offset] & !f.mask) | (value & f.mask);
    }
    let line = format!("{} {}", hex(clock), hex(&record));
    let mut lines: Vec<String> = text.lines().map(str::to_string).collect();
    match found {
        Some((n, _)) => lines[n] = line,
        None => lines.push(line),
    }
    let mut out = lines.join("\n");
    out.push('\n');
    let dir = path.parent().ok_or("no folder")?;
    fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    let temp = path.with_extension("launcher-new");
    fs::write(&temp, out).map_err(|e| format!("{}: {e}", temp.display()))?;
    fs::rename(&temp, &path).map_err(|e| {
        let _ = fs::remove_file(&temp);
        format!("{}: {e}", path.display())
    })
}

/// The mods' records kept beside the saves, for carrying them with the saves (an export, an
/// import: save_transfer.rs): the mods' folder and every mod that describes a `clock-hex` file.
#[derive(Debug, Clone, Default)]
pub struct RecordFiles {
    /// `<data home>/mod-data`.
    pub mod_data: PathBuf,
    /// Each mod with its `save_data`, by id.
    pub mods: Vec<(String, SaveData)>,
}

/// One mod's record of one save as it travels: the mod, its file and the line's data (hex) for
/// the save's clock.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CarriedRecord {
    pub id: String,
    pub file: String,
    pub data: String,
}

impl RecordFiles {
    /// The installed mods of every game in `install` (`active/<game>/data/plugins/<id>`) that
    /// describe their save data, whether on or off: a record travels with its save even while its
    /// mod is off. The mods' folder is the port's ([`mod_data_home`]).
    pub fn installed(install: &Path) -> Self {
        let mut mods: Vec<(String, SaveData)> = Vec::new();
        let games = fs::read_dir(install.join("active")).into_iter().flatten().flatten();
        for game in games {
            let plugins = crate::plugins::dir(install, &game.file_name().to_string_lossy());
            for entry in fs::read_dir(&plugins).into_iter().flatten().flatten() {
                let id = entry.file_name().to_string_lossy().to_string();
                if !crate::plugins::valid_id(&id) || mods.iter().any(|(m, _)| *m == id) {
                    continue;
                }
                let Ok(manifest) = crate::plugins::read_manifest(&entry.path()) else { continue };
                if let Some(spec) = manifest.save_data.filter(|s| s.validate().is_ok()) {
                    mods.push((id, spec));
                }
            }
        }
        RecordFiles { mod_data: mod_data_home(), mods }
    }

    /// Every mod's record of the save stamped `clock`, as its file holds it (the last line of the
    /// clock, its data whole). None for a clock of zeros: a slot the game never stamped.
    pub fn records_of(&self, clock: &[u8; 8]) -> Vec<CarriedRecord> {
        if clock.iter().all(|&b| b == 0) {
            return Vec::new();
        }
        let key = hex(clock);
        let mut out = Vec::new();
        for (id, spec) in &self.mods {
            let Ok(path) = file_of(&self.mod_data, id, spec) else { continue };
            let Ok(text) = read_text(&path) else { continue };
            let data = text
                .lines()
                .filter_map(|line| line.trim_end().split_once(' '))
                .rfind(|(k, d)| k.eq_ignore_ascii_case(&key) && unhex(d).is_some())
                .map(|(_, d)| d.to_ascii_lowercase());
            if let Some(data) = data {
                out.push(CarriedRecord { id: id.clone(), file: spec.file.clone(), data });
            }
        }
        out
    }

    /// Puts `record` back as the record of the save stamped `clock`: its clock's line in the mod's
    /// file replaced (or added at the end), every other line as it was, the file replaced in one
    /// rename. The mod need not be installed here: its record waits in its folder.
    pub fn put(&self, clock: &[u8; 8], record: &CarriedRecord) -> Result<(), String> {
        // No mods' folder (RecordFiles::default()): nowhere to keep it.
        if self.mod_data.as_os_str().is_empty() {
            return Ok(());
        }
        let plain = |n: &str| {
            !n.is_empty() && !n.starts_with('.') && !n.chars().any(|c| c.is_control() || r#"/\:*?"<>|"#.contains(c))
        };
        if !crate::plugins::valid_id(&record.id) || !plain(&record.file) {
            return Err(format!("{}/{} is not a mod's record file", record.id, record.file));
        }
        if record.data.len() > 2 * SIZE_MAX || unhex(&record.data).is_none() {
            return Err(format!("{}'s record is not one", record.id));
        }
        let path = self.mod_data.join(&record.id).join(&record.file);
        let text = read_text(&path)?;
        let key = hex(clock);
        let line = format!("{key} {}", record.data);
        let mut lines: Vec<String> = text.lines().map(str::to_string).collect();
        let at =
            lines.iter().rposition(|l| l.trim_end().split_once(' ').is_some_and(|(k, _)| k.eq_ignore_ascii_case(&key)));
        match at {
            Some(n) => lines[n] = line,
            None => lines.push(line),
        }
        let mut out = lines.join("\n");
        out.push('\n');
        let dir = path.parent().ok_or("no folder")?;
        fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
        let temp = path.with_extension("launcher-new");
        fs::write(&temp, out).map_err(|e| format!("{}: {e}", temp.display()))?;
        fs::rename(&temp, &path).map_err(|e| {
            let _ = fs::remove_file(&temp);
            format!("{}: {e}", path.display())
        })
    }
}

/// The mods' own folders: `<data home>/mod-data`, as the port's (options.cpp).
pub fn mod_data_home() -> PathBuf {
    crate::saves::data_home().join("mod-data")
}

/// The records of the save stamped `clock` of every mod of `game` that is on and describes its
/// save data, in load order.
pub fn read_all(
    plugins_dir: &Path,
    game: &str,
    version: &str,
    mod_data: &Path,
    clock: &[u8; 8],
) -> Result<Vec<ModSaveData>, String> {
    let mut out = Vec::new();
    for view in crate::plugins::list(plugins_dir, game, version)? {
        if !view.enabled {
            continue;
        }
        let Ok(manifest) = crate::plugins::read_manifest(Path::new(&view.path)) else { continue };
        if let Some(spec) = manifest.save_data.as_ref() {
            out.push(read(mod_data, &view.id, &view.name, spec, clock)?);
        }
    }
    Ok(out)
}

/// Writes `values` into the record of mod `id` (on, and describing its save data).
pub fn write_one(
    plugins_dir: &Path,
    game: &str,
    version: &str,
    mod_data: &Path,
    clock: &[u8; 8],
    id: &str,
    values: &BTreeMap<String, u8>,
) -> Result<ModSaveData, String> {
    let view = crate::plugins::list(plugins_dir, game, version)?
        .into_iter()
        .find(|v| v.id == id && v.enabled)
        .ok_or_else(|| format!("{id} is not on"))?;
    let manifest = crate::plugins::read_manifest(Path::new(&view.path))?;
    let spec = manifest.save_data.as_ref().ok_or_else(|| format!("{id} keeps no save data"))?;
    write(mod_data, id, spec, clock, values)?;
    read(mod_data, id, &view.name, spec, clock)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn spec() -> SaveData {
        SaveData {
            file: "tiers.txt".into(),
            records: "clock-hex".into(),
            size: 4,
            title: "Tiers".into(),
            fields: vec![Field {
                key: "a".into(),
                name: "A".into(),
                offset: 2,
                mask: 0x7F,
                min: 0,
                max: 8,
                choices: vec![],
                attach: None,
                requires: None,
            }],
        }
    }

    #[test]
    fn reads_and_writes_a_record() {
        let dir = tempfile::tempdir().unwrap();
        let clock = [0, 0x28, 0x38, 0, 0, 0x12, 0x11, 0x26];
        let other = [1u8; 8];
        let file = dir.path().join("m").join("tiers.txt");
        fs::create_dir_all(file.parent().unwrap()).unwrap();
        fs::write(&file, format!("{} 01020304\n{} 00008300\nbad line\n", hex(&clock), hex(&other))).unwrap();

        let r = read(dir.path(), "m", "M", &spec(), &clock).unwrap();
        assert!(r.found);
        assert_eq!(r.fields[0].value, 3);
        let r = read(dir.path(), "m", "M", &spec(), &other).unwrap();
        assert_eq!(r.fields[0].value, 3); // 0x83: the top bit is not the field's

        let mut values = BTreeMap::new();
        values.insert("a".to_string(), 7u8);
        write(dir.path(), "m", &spec(), &other, &values).unwrap();
        let text = fs::read_to_string(&file).unwrap();
        assert!(text.contains(&format!("{} 00008700", hex(&other))));
        assert!(text.contains(&format!("{} 01020304", hex(&clock))));
        assert!(text.contains("bad line"));

        // A save the file has no record of: added.
        let new = [9u8; 8];
        assert!(!read(dir.path(), "m", "M", &spec(), &new).unwrap().found);
        write(dir.path(), "m", &spec(), &new, &values).unwrap();
        assert!(read(dir.path(), "m", "M", &spec(), &new).unwrap().found);

        values.insert("a".to_string(), 9u8);
        assert!(write(dir.path(), "m", &spec(), &new, &values).is_err());
    }

    #[test]
    fn ngpp_tiers_layouts() {
        let dir = tempfile::tempdir().unwrap();
        let mut spec = spec();
        spec.size = 56;
        spec.fields = [("item_43", 43usize), ("item_44", 44)]
            .into_iter()
            .map(|(k, o)| Field {
                key: k.to_string(),
                name: k.to_string(),
                offset: o,
                mask: 0x7F,
                min: 0,
                max: 8,
                choices: vec![],
                attach: None,
                requires: None,
            })
            .collect();
        let clock = [1u8; 8];
        let id = "ng-plus-plus";
        let file = dir.path().join(id).join("tiers.txt");
        fs::create_dir_all(file.parent().unwrap()).unwrap();
        let rec = |zodiac: u8, ryno: u8| {
            let mut b = [0u8; 56];
            b[43] = zodiac;
            b[44] = ryno;
            format!("{} {}", hex(&clock), hex(&b))
        };
        let byte_of = |text: &str, at: usize| {
            unhex(text.lines().find(|l| is_record(l, 56)).unwrap().split_once(' ').unwrap().1).unwrap()[at]
        };
        let zodiac_of = |text: &str| byte_of(text, 43);
        let ryno_of = |text: &str| byte_of(text, 44);

        // Layout 2: the Zodiac at Alpha (1, started) is the fourth step now; the RYNO II's Alpha (2)
        // is the fourth too.
        fs::write(&file, format!("ngpp-tiers 2\n{}\nnote\n", rec(0x81, 2))).unwrap();
        let r = read(dir.path(), id, "N", &spec, &clock).unwrap();
        assert_eq!((r.fields[0].value, r.fields[1].value), (4, 4));
        for f in &r.fields {
            assert_eq!(f.spec.choices[..5], ["None", "Heavy", "Mega", "Ultra", "Alpha"]);
            assert_eq!(f.spec.choices[12], "Ultima");
            assert_eq!(f.spec.max, 12);
        }
        // Reading changes nothing on disk.
        assert!(fs::read_to_string(&file).unwrap().starts_with("ngpp-tiers 2\n"));

        // Writing converts the whole file to layout 4, keeping other lines and the started bit.
        let mut v = BTreeMap::new();
        v.insert("item_44".to_string(), 5u8);
        write(dir.path(), id, &spec, &clock, &v).unwrap();
        let text = fs::read_to_string(&file).unwrap();
        assert!(text.starts_with("ngpp-tiers 4\n") && text.contains("note\n"));
        assert_eq!(text.matches("ngpp-tiers").count(), 1);
        assert_eq!(zodiac_of(&text), 0x84);

        // Layout 4 is not moved again; both go up to Ultima (12), and no further.
        v.clear();
        v.insert("item_43".to_string(), 12u8);
        v.insert("item_44".to_string(), 12u8);
        write(dir.path(), id, &spec, &clock, &v).unwrap();
        let text = fs::read_to_string(&file).unwrap();
        assert_eq!(zodiac_of(&text), 0x8C);
        assert_eq!(ryno_of(&text), 12);
        v.insert("item_43".to_string(), 13u8);
        assert!(write(dir.path(), id, &spec, &clock, &v).is_err());

        // Layout 3: the Zodiac stays (Ultra is 3), the RYNO II's Alpha (2) and Ultima (10) move up two.
        fs::write(&file, format!("ngpp-tiers 3\n{}\n", rec(3, 10))).unwrap();
        let r = read(dir.path(), id, "N", &spec, &clock).unwrap();
        assert_eq!((r.fields[0].value, r.fields[1].value), (3, 12));
        fs::write(&file, format!("ngpp-tiers 3\n{}\n", rec(1, 2))).unwrap();
        let r = read(dir.path(), id, "N", &spec, &clock).unwrap();
        assert_eq!((r.fields[0].value, r.fields[1].value), (1, 4));
        fs::write(&file, format!("ngpp-tiers 3\n{}\n", rec(0x80, 1))).unwrap();
        let r = read(dir.path(), id, "N", &spec, &clock).unwrap();
        assert_eq!((r.fields[0].value, r.fields[1].value), (0, 1));

        // Layout 1 (no marker): the RYNO II's Omega Prime (11) is Ultima (12) now.
        fs::write(&file, format!("{}\n", rec(1, 11))).unwrap();
        let r = read(dir.path(), id, "N", &spec, &clock).unwrap();
        assert_eq!((r.fields[0].value, r.fields[1].value), (4, 12));

        // A new file is layout 4.
        fs::remove_file(&file).unwrap();
        v.clear();
        v.insert("item_43".to_string(), 1u8);
        write(dir.path(), id, &spec, &clock, &v).unwrap();
        let text = fs::read_to_string(&file).unwrap();
        assert!(text.starts_with("ngpp-tiers 4\n"));
        assert_eq!(zodiac_of(&text), 1);
    }

    #[test]
    fn reads_an_attached_field() {
        let json = r#"{"file": "t.txt", "records": "clock-hex", "size": 56, "fields": [
            {"key": "item_30", "name": "Lancer", "offset": 30, "mask": 127, "max": 8,
             "choices": ["None", "Alpha"], "attach": {"to": "item", "id": 30, "as": "version"},
             "requires": {"version": "last"}},
            {"key": "later", "name": "L", "offset": 1, "attach": {"to": "planet", "id": 3, "as": "row"}}]}"#;
        let s: SaveData = serde_json::from_str(json).unwrap();
        s.validate().unwrap();
        let a = s.fields[0].attach.as_ref().unwrap();
        assert_eq!((a.to.as_str(), a.id, a.role.as_str()), ("item", 30, "version"));
        assert_eq!(s.fields[0].requires.as_ref().unwrap().version.as_deref(), Some("last"));
        let back = serde_json::to_value(&s.fields[0]).unwrap();
        assert_eq!(back["attach"]["as"], "version");
        assert!(serde_json::to_value(&s.fields[1]).unwrap().get("requires").is_none());

        let mut bad = s.clone();
        bad.fields[0].attach.as_mut().unwrap().role = "side".into();
        assert!(bad.validate().is_err());
        let mut bad = s.clone();
        bad.fields[0].attach.as_mut().unwrap().role = "row".into();
        assert!(bad.validate().is_err(), "requires a version off a version picker");
        let mut bad = s;
        bad.fields[0].requires.as_mut().unwrap().version = Some("first".into());
        assert!(bad.validate().is_err());
    }

    #[test]
    fn refuses_a_bad_spec() {
        let mut s = spec();
        s.file = "../x".into();
        assert!(s.validate().is_err());
        let mut s = spec();
        s.fields[0].offset = 4;
        assert!(s.validate().is_err());
        let mut s = spec();
        s.fields[0].max = 200;
        assert!(s.validate().is_err());
    }
}
