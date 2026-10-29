//! Where a player is with the level editor, for one version of a game: the Mods page's status panel
//! (src/components/EditorFlow.svelte) asks when it opens, when the window comes back into focus
//! and when one of the editor's jobs ends, so that it says what changed without waiting for a tool
//! to fail.
//!
//! It reads, never writes, the folders of [`crate::reset::Layout`]:
//!
//! - the level projects (`level_NN/levels/level_NN/level.json` and its scene, `level_NN.tscn`):
//!   which levels were read, which an older editor made ([`Outdated`]: an older `schema`, a PAL
//!   project read from the 50 Hz gameplay copy, or another version's), which are missing files,
//!   and which have a scene saved after the last pack;
//! - the packed levels (`changes.txt`, written by every pack, and `packed.json`, the version they
//!   were packed for);
//! - the notes the last update left (`carried.json` beside a scene: what was carried over from the
//!   old project and what was not), until the player packs again;
//! - the backups of replaced projects (`<version>-backups/<time>`), newest first;
//! - whether the game's copy of the levels was read at all (`levels-*.json`; the page compares it
//!   with the stamp its actions expect).
//!
//! What a project should be is the editor's `project_format.json` (`schema`, and the gameplay copy
//! per version), the file `editor/godot.py` reads too; without it no project is called outdated.

use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};

use crate::reset::Layout;

/// A scene saved this long after its level.json is an edit: the extractor writes both at once.
const SAME_WRITE_MS: u64 = 2_000;

/// What a level project of this editor is (`editor/project_format.json`).
#[derive(Debug, Clone, Default, PartialEq, Eq, Deserialize)]
pub struct ProjectFormat {
    pub schema: u32,
    /// Version key (`rac1/pal`) -> the gameplay copy its projects are read from.
    #[serde(default)]
    pub gameplay: std::collections::BTreeMap<String, String>,
}

impl ProjectFormat {
    /// The editor's, from the checkout (or an installed launcher's copy) at `root`; None when it has
    /// none or it cannot be read (an older copy), and then nothing is called outdated.
    pub fn load(root: &Path) -> Option<ProjectFormat> {
        let text = std::fs::read_to_string(root.join("editor").join("project_format.json")).ok()?;
        serde_json::from_str(&text).ok()
    }
}

/// Why a level project needs updating.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "OutdatedReason")]
pub enum Reason {
    /// Made by an editor whose projects hold less (an older `schema`).
    Schema,
    /// A PAL project read from the 50 Hz gameplay copy; the game plays the 60 Hz one.
    Gameplay,
    /// Read from another version's disc.
    Version,
}

/// One outdated level.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Outdated {
    pub level: u32,
    pub reason: Reason,
}

/// What the last update carried over for one level (editor/carry.py's `carried.json`).
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Carried {
    pub level: u32,
    /// The edits carried, one line each (`Bolt_0002: moved`).
    #[serde(default)]
    pub carried: Vec<String>,
    /// What could not be, one line each, to redo by hand.
    #[serde(default, rename(deserialize = "not_carried"))]
    pub not_carried: Vec<String>,
    /// Carrying stopped and the fresh level was kept as the disc has it.
    #[serde(default)]
    pub failed: bool,
}

/// A folder of old projects the extractor kept aside.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Backup {
    /// `20261028-101500`: when it was made.
    pub name: String,
    pub path: PathBuf,
    pub levels: Vec<u32>,
}

/// Where the player is with the level editor of one version.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct EditorState {
    /// The levels read into projects.
    pub levels: Vec<u32>,
    /// Those an older editor made, and why.
    pub outdated: Vec<Outdated>,
    /// Those with a scene saved after the last pack (or, never packed, after it was read).
    pub unpacked: Vec<u32>,
    /// Those whose folder lacks its project, scene or level.json.
    pub broken: Vec<u32>,
    /// When the levels were last packed, in ms since 1970; None if never.
    #[ts(type = "number | null")]
    pub packed_at: Option<u64>,
    /// How many levels the last pack changed.
    pub packed_levels: u32,
    /// The packed levels are another version's (packed.json).
    pub packed_other_version: bool,
    /// The last update's notes, while they are newer than the last pack.
    pub carried: Vec<Carried>,
    /// Old projects kept aside, newest first.
    pub backups: Vec<Backup>,
    /// The game's own copy of the levels was read (any format).
    pub game_levels_found: bool,
    /// The editor's project format was found (else `outdated` is empty for want of it).
    pub format_known: bool,
}

fn millis(time: SystemTime) -> u64 {
    time.duration_since(UNIX_EPOCH).map(|d| d.as_millis() as u64).unwrap_or(0)
}

fn modified(path: &Path) -> Option<u64> {
    std::fs::metadata(path).ok()?.modified().ok().map(millis)
}

fn json(path: &Path) -> Option<serde_json::Value> {
    serde_json::from_str(&std::fs::read_to_string(path).ok()?).ok()
}

/// `level_NN` -> NN.
fn level_number(name: &str) -> Option<u32> {
    let digits = name.strip_prefix("level_")?;
    (digits.len() == 2 && digits.chars().all(|c| c.is_ascii_digit())).then(|| digits.parse().ok()).flatten()
}

/// The `level_NN` folders in `dir`, by number.
fn level_folders(dir: &Path) -> Vec<(u32, PathBuf)> {
    let Ok(entries) = std::fs::read_dir(dir) else { return Vec::new() };
    let mut found: Vec<(u32, PathBuf)> = entries
        .flatten()
        .filter(|e| e.file_type().map(|t| t.is_dir()).unwrap_or(false))
        .filter_map(|e| level_number(&e.file_name().to_string_lossy()).map(|n| (n, e.path())))
        .collect();
    found.sort();
    found
}

/// Why `stats` (a level.json) is outdated for `version_key` (`rac1/pal`), if it is.
pub fn outdated(stats: &serde_json::Value, format: &ProjectFormat, version_key: &str) -> Option<Reason> {
    let version = version_key.split('/').nth(1).unwrap_or_default();
    if let Some(made_for) = stats.get("version").and_then(|v| v.as_str()) {
        if made_for != version {
            return Some(Reason::Version);
        }
    }
    if stats.get("schema").and_then(|v| v.as_u64()) != Some(u64::from(format.schema)) {
        return Some(Reason::Schema);
    }
    if let Some(wanted) = format.gameplay.get(version_key) {
        if stats.get("gameplay_source").and_then(|v| v.as_str()) != Some(wanted.as_str()) {
            return Some(Reason::Gameplay);
        }
    }
    None
}

/// Where the player is, from `layout`'s folders, `format` saying what a project should be.
pub fn inspect(layout: &Layout, format: Option<&ProjectFormat>) -> EditorState {
    let key = layout.key();
    let mut state = EditorState { format_known: format.is_some(), ..EditorState::default() };

    // The last pack: every pack writes changes.txt; packed.json says for which version.
    let packed = layout.packed();
    state.packed_at = modified(&packed.join("changes.txt"));
    if state.packed_at.is_some() {
        state.packed_levels = level_folders(&packed).len() as u32;
        if let Some(made) = json(&packed.join("packed.json")) {
            state.packed_other_version =
                made.get("version").and_then(|v| v.as_str()).is_some_and(|v| v != layout.version());
        }
    }

    for (n, project) in level_folders(&layout.projects()) {
        state.levels.push(n);
        let name = format!("level_{n:02}");
        let level = project.join("levels").join(&name);
        let (stats_file, scene) = (level.join("level.json"), level.join(format!("{name}.tscn")));
        let stats = json(&stats_file);
        if !project.join("project.godot").is_file() || !scene.is_file() || stats.is_none() {
            state.broken.push(n);
            continue;
        }
        if let (Some(format), Some(stats)) = (format, &stats) {
            if let Some(reason) = outdated(stats, format, &key) {
                state.outdated.push(Outdated { level: n, reason });
            }
        }
        let (Some(saved), Some(read)) = (modified(&scene), modified(&stats_file)) else { continue };
        let edited = saved > read + SAME_WRITE_MS;
        if edited && state.packed_at.is_none_or(|packed| saved > packed) {
            state.unpacked.push(n);
        }
        // The last update's notes, until the player packs again.
        let notes = level.join("carried.json");
        if let (Some(at), Some(text)) = (modified(&notes), std::fs::read_to_string(&notes).ok()) {
            if state.packed_at.is_none_or(|packed| at > packed) {
                if let Ok(carried) = serde_json::from_str::<Carried>(&text) {
                    state.carried.push(carried);
                }
            }
        }
        // A level the update carried edits onto is one to pack, though its scene was written
        // straight after its level.json.
        if state.carried.last().is_some_and(|c| c.level == n && !c.carried.is_empty()) && !state.unpacked.contains(&n) {
            state.unpacked.push(n);
        }
    }

    if let Ok(entries) = std::fs::read_dir(layout.backups()) {
        let mut backups: Vec<Backup> = entries
            .flatten()
            .filter(|e| e.file_type().map(|t| t.is_dir()).unwrap_or(false))
            .map(|e| Backup {
                name: e.file_name().to_string_lossy().into_owned(),
                levels: level_folders(&e.path()).into_iter().map(|(n, _)| n).collect(),
                path: e.path(),
            })
            .collect();
        backups.sort_by(|a, b| b.name.cmp(&a.name));
        state.backups = backups;
    }

    state.game_levels_found = std::fs::read_dir(layout.port()).is_ok_and(|entries| {
        entries.flatten().any(|e| {
            let name = e.file_name().to_string_lossy().into_owned();
            name.starts_with("levels-") && name.ends_with(".json")
        })
    });
    state
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::Duration;

    struct World {
        _tmp: tempfile::TempDir,
        root: PathBuf,
        install: PathBuf,
    }

    fn format() -> ProjectFormat {
        ProjectFormat { schema: 3, gameplay: [("rac1/pal".to_string(), "gameplay_ntsc".to_string())].into() }
    }

    fn set_time(path: &Path, ms: u64) {
        let file = std::fs::File::options().write(true).open(path).unwrap();
        file.set_modified(UNIX_EPOCH + Duration::from_millis(ms)).unwrap();
    }

    impl World {
        fn new() -> World {
            let tmp = tempfile::tempdir().unwrap();
            let root = tmp.path().join("OpenRAC");
            let install = tmp.path().join("install");
            std::fs::create_dir_all(&root).unwrap();
            World { _tmp: tmp, root, install }
        }

        fn layout(&self) -> Layout {
            Layout::new(&self.root, &self.install, "rac1", "pal", false).unwrap()
        }

        fn write(&self, rel: &str, text: &str, ms: u64) -> PathBuf {
            let path = self.root.join("assets").join(rel);
            std::fs::create_dir_all(path.parent().unwrap()).unwrap();
            std::fs::write(&path, text).unwrap();
            set_time(&path, ms);
            path
        }

        /// A level project as the extractor writes it at `ms`, with level.json `stats`.
        fn level(&self, n: u32, stats: serde_json::Value, ms: u64) {
            let dir = format!("levels/rac1/pal/level_{n:02}");
            self.write(&format!("{dir}/project.godot"), "config_version=5", ms);
            self.write(&format!("{dir}/levels/level_{n:02}/level.json"), &stats.to_string(), ms);
            self.write(&format!("{dir}/levels/level_{n:02}/level_{n:02}.tscn"), "[gd_scene format=3]", ms);
        }

        fn save_scene(&self, n: u32, ms: u64) {
            let path =
                self.root.join(format!("assets/levels/rac1/pal/level_{n:02}/levels/level_{n:02}/level_{n:02}.tscn"));
            set_time(&path, ms);
        }

        fn pack(&self, ms: u64, version: &str) {
            self.write("packed/rac1/pal/changes.txt", "level 00\n", ms);
            self.write(
                "packed/rac1/pal/packed.json",
                &format!(r#"{{"format":1,"game":"rac1","version":"{version}"}}"#),
                ms,
            );
            self.write("packed/rac1/pal/level_00/gameplay_pal.wad", "x", ms);
        }
    }

    fn current() -> serde_json::Value {
        serde_json::json!({"schema": 3, "gameplay_source": "gameplay_ntsc", "version": "pal"})
    }

    #[test]
    fn nothing_read_yet() {
        let w = World::new();
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(state, EditorState { format_known: true, ..EditorState::default() });
    }

    #[test]
    fn fresh_levels_are_ready_and_nothing_is_to_pack() {
        let w = World::new();
        w.level(0, current(), 1_000_000);
        w.level(3, current(), 1_000_000);
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(state.levels, vec![0, 3]);
        assert!(state.outdated.is_empty() && state.unpacked.is_empty() && state.broken.is_empty());
        assert_eq!(state.packed_at, None);
    }

    #[test]
    fn outdated_projects_say_why() {
        let w = World::new();
        w.level(0, serde_json::json!({"schema": 3}), 1_000_000); // PAL before the 60 Hz fix
        w.level(1, serde_json::json!({"schema": 1}), 1_000_000);
        w.level(2, serde_json::json!({"schema": 3, "gameplay_source": "gameplay_ntsc", "version": "ntsc"}), 1_000_000);
        w.level(4, current(), 1_000_000);
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(
            state.outdated,
            vec![
                Outdated { level: 0, reason: Reason::Gameplay },
                Outdated { level: 1, reason: Reason::Schema },
                Outdated { level: 2, reason: Reason::Version },
            ]
        );
        // Without the editor's format, nothing is called outdated.
        assert!(inspect(&w.layout(), None).outdated.is_empty());
    }

    #[test]
    fn the_schema_and_gameplay_rule_follow_project_format() {
        let ntsc = "rac1/ntsc";
        let format = format();
        // NTSC has no gameplay rule: a project without gameplay_source is current.
        assert_eq!(outdated(&serde_json::json!({"schema": 3}), &format, ntsc), None);
        assert_eq!(outdated(&serde_json::json!({"schema": 2}), &format, ntsc), Some(Reason::Schema));
    }

    #[test]
    fn a_scene_saved_after_the_last_pack_is_unpacked() {
        let w = World::new();
        w.level(0, current(), 1_000_000);
        w.level(1, current(), 1_000_000);
        w.level(2, current(), 1_000_000);
        // Never packed: an edit is unpacked; a scene written with its level.json is not an edit.
        w.save_scene(0, 1_100_000);
        w.save_scene(2, 1_000_500);
        assert_eq!(inspect(&w.layout(), Some(&format())).unpacked, vec![0]);
        // Packed after that edit: nothing to pack; then level 1 saved again.
        w.pack(1_200_000, "pal");
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(state.unpacked, Vec::<u32>::new());
        assert_eq!((state.packed_at, state.packed_levels, state.packed_other_version), (Some(1_200_000), 1, false));
        w.save_scene(1, 1_300_000);
        assert_eq!(inspect(&w.layout(), Some(&format())).unpacked, vec![1]);
    }

    #[test]
    fn packed_for_another_version() {
        let w = World::new();
        w.pack(1_000_000, "ntsc");
        assert!(inspect(&w.layout(), Some(&format())).packed_other_version);
    }

    #[test]
    fn missing_files_are_broken() {
        let w = World::new();
        w.level(0, current(), 1_000_000);
        w.level(1, current(), 1_000_000);
        std::fs::remove_file(w.root.join("assets/levels/rac1/pal/level_01/levels/level_01/level_01.tscn")).unwrap();
        w.write("levels/rac1/pal/level_02/levels/level_02/level.json", "{}", 1_000_000);
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(state.levels, vec![0, 1, 2]);
        assert_eq!(state.broken, vec![1, 2]);
    }

    #[test]
    fn the_last_update_names_its_backup_and_what_it_carried_until_the_next_pack() {
        let w = World::new();
        w.level(0, current(), 2_000_000);
        w.write(
            "levels/rac1/pal/level_00/levels/level_00/carried.json",
            r#"{"level":0,"carried":["Bolt_0002: moved"],"not_carried":["Camera_0001: moved"],"mobys":1,"backup":"/x"}"#,
            2_000_100,
        );
        w.write("levels/rac1/pal-backups/20261001-000000/level_00/project.godot", "", 1_000_000);
        w.write("levels/rac1/pal-backups/20261028-101500/level_00/project.godot", "", 1_900_000);
        w.write("levels/rac1/pal-backups/20261028-101500/level_05/project.godot", "", 1_900_000);
        let state = inspect(&w.layout(), Some(&format()));
        assert_eq!(
            state.carried,
            vec![Carried {
                level: 0,
                carried: vec!["Bolt_0002: moved".into()],
                not_carried: vec!["Camera_0001: moved".into()],
                failed: false,
            }]
        );
        // The carried level is to pack though its scene was written with its level.json.
        assert_eq!(state.unpacked, vec![0]);
        assert_eq!(
            state.backups.iter().map(|b| b.name.as_str()).collect::<Vec<_>>(),
            ["20261028-101500", "20261001-000000"]
        );
        assert_eq!(state.backups[0].levels, vec![0, 5]);
        w.pack(2_100_000, "pal");
        let state = inspect(&w.layout(), Some(&format()));
        assert!(state.carried.is_empty());
        assert!(state.unpacked.is_empty());
    }

    #[test]
    fn the_games_levels_are_found_by_any_stamp() {
        let w = World::new();
        assert!(!inspect(&w.layout(), None).game_levels_found);
        w.write("port/rac1/pal/levels-3.json", "{}", 1);
        assert!(inspect(&w.layout(), None).game_levels_found);
    }

    #[test]
    fn the_editors_own_project_format_reads() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
        let format = ProjectFormat::load(&root).expect("editor/project_format.json");
        assert!(format.schema >= 3);
        assert_eq!(format.gameplay.get("rac1/pal").map(String::as_str), Some("gameplay_ntsc"));
    }
}
