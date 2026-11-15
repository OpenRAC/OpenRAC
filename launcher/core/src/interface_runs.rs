// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Interface texture runs (docs/port/TEXTURE_PACKS.md, "Export without playing"): what the Models
//! and textures panel says of a version's runs before its export.
//!
//! A run file (port/game/common/program/interface_runs.h) lists the interface textures a run of the
//! game made and where their bytes are on the disc. The player makes it on their own computer
//! (`<data>/interface_runs/<id>.json`, port/tools/interface_runs/run.py, a few minutes) and the export
//! reads it. This module reads only what the panel shows: for each kind of texture, whether the run
//! has it.

use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::texture_packs;

/// The run file's format (interface_runs.h kInterfaceRunFormat).
pub const FORMAT: u32 = 1;

/// The port's conversion the names are made with (renderer/textures/interface_run.h kConverter).
/// A run of another one is out of date: the export would find none of its textures.
pub const CONVERTER: u32 = 1;

/// The kinds of texture, in the panel's order (interface_run.h's categories).
pub const CATEGORIES: [&str; 3] = ["font", "interface", "effects"];

/// The versions whose routes the run plays (port/tools/interface_runs/run.py ROUTE_SETS).
pub const RUNNABLE: [&str; 3] = ["rac1-pal", "rac1-ntsc", "rac2-ntsc"];

/// The largest run file read: a real one is a few MB.
const MAX_FILE: u64 = 64 * 1024 * 1024;

/// One run file, as the panel shows it.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[ts(rename = "InterfaceRun")]
#[serde(rename_all = "camelCase")]
pub struct Run {
    /// When it was made, `2026-11-14` (UTC), if it says.
    pub made: Option<String>,
    /// The textures it holds, by kind (CATEGORIES' order).
    pub counts: [u32; 3],
    /// No route it played failed.
    pub clean: bool,
    /// For each kind (CATEGORIES' order): every route that kind needs has played. A run can be
    /// made a kind at a time (run.py --kinds); a file made before that is complete when clean.
    pub complete: [bool; 3],
    /// For each kind: when it was last made, if it says.
    pub kind_made: [Option<String>; 3],
    /// Made by this launcher's port (CONVERTER); one that is not cannot be exported from.
    pub current: bool,
}

/// What a version's interface textures can be exported from.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[ts(rename = "InterfaceRunsView")]
#[serde(rename_all = "camelCase")]
pub struct View {
    /// The port's id of the version: `rac1-ntsc`.
    pub id: String,
    /// The player's run, if they made one.
    pub local: Option<Run>,
    /// A run file that is there but cannot be read (cut short, another format, another game):
    /// which, in words.
    pub unreadable: Vec<String>,
    /// For each kind (CATEGORIES' order): the run is current and has textures of it to export.
    pub available: [bool; 3],
    /// Whether this version's routes can be played here (a run can be made).
    pub runnable: bool,
    /// The player's run file.
    pub local_file: String,
}

#[derive(Deserialize)]
struct File {
    format: Option<u32>,
    game: Option<String>,
    converter: Option<u32>,
    made: Option<String>,
    clean: Option<bool>,
    #[serde(default)]
    textures: Vec<Texture>,
    kinds: Option<std::collections::BTreeMap<String, Kind>>,
    routes: Option<Routes>,
}

#[derive(Deserialize)]
struct Kind {
    complete: Option<bool>,
    made: Option<String>,
}

#[derive(Deserialize)]
struct Routes {
    #[serde(default)]
    skipped: Vec<serde_json::Value>,
}

/// A date as run.py writes it (`2026-11-14`), or None.
fn date(text: Option<String>) -> Option<String> {
    text.filter(|m| m.len() == 10 && m.chars().all(|c| c.is_ascii_digit() || c == '-'))
}

#[derive(Deserialize)]
struct Texture {
    category: Option<String>,
}

/// The player's own run for version `id`: `<data>/interface_runs/<id>.json`.
pub fn local_file(data: &Path, id: &str) -> PathBuf {
    data.join("interface_runs").join(format!("{id}.json"))
}

/// Reads one run file: None when it is not there, Err (in words) when it cannot be used.
fn read(path: &Path, id: &str) -> Result<Option<Run>, String> {
    let meta = match std::fs::metadata(path) {
        Ok(meta) => meta,
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(None),
        Err(e) => return Err(format!("{} cannot be read: {e}", path.display())),
    };
    if !meta.is_file() || meta.len() > MAX_FILE {
        return Err(format!("{} is not a run file", path.display()));
    }
    let text = std::fs::read_to_string(path).map_err(|e| format!("{} cannot be read: {e}", path.display()))?;
    let file: File =
        serde_json::from_str(&text).map_err(|_| format!("{} is cut short or not a run file", path.display()))?;
    if file.format != Some(FORMAT) {
        return Err(format!("{} is a run file of another format", path.display()));
    }
    if file.game.as_deref() != Some(id) {
        return Err(format!("{} is a run of {}, not {id}", path.display(), file.game.unwrap_or_default()));
    }
    let mut counts = [0u32; 3];
    for t in &file.textures {
        if let Some(at) = CATEGORIES.iter().position(|c| Some(*c) == t.category.as_deref()) {
            counts[at] += 1;
        }
    }
    let made = date(file.made);
    let clean = file.clean.unwrap_or(false);
    let (complete, kind_made) = match file.kinds {
        Some(mut kinds) => {
            let mut complete = [false; 3];
            let mut kind_made: [Option<String>; 3] = Default::default();
            for (at, c) in CATEGORIES.iter().enumerate() {
                if let Some(k) = kinds.remove(*c) {
                    complete[at] = k.complete.unwrap_or(false);
                    kind_made[at] = date(k.made);
                }
            }
            (complete, kind_made)
        }
        // Made before kinds: all of it, when no route failed or was left out.
        None => {
            let whole = clean && file.routes.is_none_or(|r| r.skipped.is_empty());
            ([whole; 3], [made.clone(), made.clone(), made.clone()])
        }
    };
    Ok(Some(Run { made, counts, clean, complete, kind_made, current: file.converter == Some(CONVERTER) }))
}

/// What the panel shows for `game` `version`: `data` is the game's data folder.
pub fn view(data: &Path, game: &str, version: &str) -> Result<View, String> {
    let id = texture_packs::id(game, version)?;
    let local_path = local_file(data, &id);
    let (local, unreadable) = match read(&local_path, &id) {
        Ok(run) => (run, Vec::new()),
        Err(why) => (None, vec![why]),
    };
    let available = std::array::from_fn(|at| local.as_ref().is_some_and(|r| r.current && r.counts[at] > 0));
    Ok(View {
        runnable: RUNNABLE.contains(&id.as_str()),
        id,
        local,
        unreadable,
        available,
        local_file: local_path.display().to_string(),
    })
}

/// The `--only` value (the export action's option) for the kinds chosen: in CATEGORIES' order,
/// joined by commas; None for no kind or one that is not known.
pub fn option(chosen: &[String]) -> Option<String> {
    if chosen.is_empty() || chosen.iter().any(|c| !CATEGORIES.contains(&c.as_str())) {
        return None;
    }
    Some(CATEGORIES.iter().filter(|c| chosen.iter().any(|x| x == *c)).copied().collect::<Vec<_>>().join(","))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::checkout;

    fn write(path: &Path, text: &str) {
        std::fs::create_dir_all(path.parent().unwrap()).unwrap();
        std::fs::write(path, text).unwrap();
    }

    fn run(game: &str, converter: u32, clean: bool, cats: &[&str]) -> String {
        let textures: Vec<String> =
            cats.iter().enumerate().map(|(i, c)| format!(r#"{{"name": "{i:016x}", "category": "{c}"}}"#)).collect();
        format!(
            r#"{{"format": 1, "game": "{game}", "converter": {converter}, "made": "2026-11-14", "clean": {clean},
                "blobs": {{}}, "textures": [{}]}}"#,
            textures.join(",")
        )
    }

    #[test]
    fn nothing_when_no_run_is_there() {
        let dir = tempfile::tempdir().unwrap();
        let v = view(dir.path(), "rac1", "ntsc").unwrap();
        assert_eq!(v.local, None);
        assert_eq!(v.available, [false; 3]);
        assert!(v.runnable && v.unreadable.is_empty());
        assert!(!view(dir.path(), "rac3", "ntsc").unwrap().runnable);
    }

    #[test]
    fn each_kind_the_players_run_has() {
        let dir = tempfile::tempdir().unwrap();
        let data = dir.path();
        write(&local_file(data, "rac1-ntsc"), &run("rac1-ntsc", CONVERTER, true, &["interface", "effects"]));
        let v = view(data, "rac1", "ntsc").unwrap();
        assert_eq!(v.available, [false, true, true]);
        assert_eq!(v.local.as_ref().unwrap().made.as_deref(), Some("2026-11-14"));
        assert_eq!(v.local.unwrap().counts, [0, 1, 1]);
    }

    #[test]
    fn an_out_of_date_or_broken_run_is_not_used() {
        let dir = tempfile::tempdir().unwrap();
        let data = dir.path();
        write(&local_file(data, "rac2-ntsc"), &run("rac2-ntsc", CONVERTER + 1, true, &["font"]));
        let v = view(data, "rac2", "ntsc").unwrap();
        assert!(!v.local.as_ref().unwrap().current);
        assert!(!v.available[0]);
        // Cut short, another game's, another format: said, not used.
        for text in [&run("rac2-ntsc", CONVERTER, true, &["font"])[..40], &run("rac1-pal", CONVERTER, true, &["font"]),
            r#"{"format": 2, "game": "rac2-ntsc"}"#]
        {
            write(&local_file(data, "rac2-ntsc"), text);
            let v = view(data, "rac2", "ntsc").unwrap();
            assert_eq!((v.local, v.unreadable.len()), (None, 1));
        }
    }

    #[test]
    fn a_run_made_a_kind_at_a_time() {
        let dir = tempfile::tempdir().unwrap();
        let data = dir.path();
        let text = run("rac1-pal", CONVERTER, true, &["font", "interface"]).replacen(
            r#""blobs""#,
            r#""kinds": {"font": {"complete": true, "made": "2026-11-13"}, "interface": {"complete": false, "made": "2026-11-14"},
                "effects": {"complete": false, "made": null}}, "blobs""#,
            1,
        );
        write(&local_file(data, "rac1-pal"), &text);
        let local = view(data, "rac1", "pal").unwrap().local.unwrap();
        assert_eq!(local.complete, [true, false, false]);
        assert_eq!(local.kind_made, [Some("2026-11-13".into()), Some("2026-11-14".into()), None]);
    }

    #[test]
    fn a_run_from_before_kinds_is_whole_when_clean() {
        let dir = tempfile::tempdir().unwrap();
        let data = dir.path();
        write(&local_file(data, "rac1-pal"), &run("rac1-pal", CONVERTER, true, &["font"]));
        let local = view(data, "rac1", "pal").unwrap().local.unwrap();
        assert_eq!(local.complete, [true; 3]);
        assert_eq!(local.kind_made[2].as_deref(), Some("2026-11-14"));
        let skipped = run("rac1-pal", CONVERTER, true, &["font"])
            .replacen(r#""blobs""#, r#""routes": {"done": [], "skipped": ["title-load-0"]}, "blobs""#, 1);
        write(&local_file(data, "rac1-pal"), &skipped);
        assert_eq!(view(data, "rac1", "pal").unwrap().local.unwrap().complete, [false; 3]);
    }

    #[test]
    fn the_export_option_is_in_the_order_the_actions_list_it() {
        let s = |v: &[&str]| v.iter().map(|x| x.to_string()).collect::<Vec<_>>();
        assert_eq!(option(&s(&["effects", "font"])).as_deref(), Some("font,effects"));
        assert_eq!(option(&s(&["interface"])).as_deref(), Some("interface"));
        assert_eq!(option(&s(&[])), None);
        assert_eq!(option(&s(&["hud"])), None);
    }

    #[test]
    fn constants_match_the_port_and_the_actions() {
        let root = checkout();
        let header = std::fs::read_to_string(root.join("port/renderer/textures/interface_run.h")).unwrap();
        assert!(header.contains(&format!("inline constexpr int kConverter = {CONVERTER};")));
        let program = std::fs::read_to_string(root.join("port/game/common/program/interface_runs.h")).unwrap();
        assert!(program.contains(&format!("inline constexpr int kInterfaceRunFormat = {FORMAT};")));
        let actions: serde_json::Value =
            serde_json::from_str(&std::fs::read_to_string(root.join("launcher/actions.json")).unwrap()).unwrap();
        for (key, list) in actions["versions"].as_object().unwrap() {
            let id = key.replace('/', "-");
            let has = |name: &str| list.as_array().unwrap().iter().any(|a| a["id"] == name);
            // A run can be made exactly where its routes are, and is exported wherever it can be made.
            assert_eq!(has("interface-run"), RUNNABLE.contains(&id.as_str()), "{key}");
            if has("interface-run") {
                assert!(has("interface-export"), "{key}");
            }
            for a in list.as_array().unwrap().iter().filter(|a| a["id"] == "interface-export" || a["id"] == "interface-run") {
                let options: Vec<String> =
                    a["options"].as_array().unwrap().iter().map(|o| o.as_str().unwrap().to_string()).collect();
                // Every choice of kinds the panel can make is one of the action's options.
                for mask in 1..8u32 {
                    let chosen: Vec<String> =
                        (0..3).filter(|i| mask & (1 << i) != 0).map(|i| CATEGORIES[i].to_string()).collect();
                    assert!(options.contains(&option(&chosen).unwrap()), "{key}: {chosen:?}");
                }
            }
        }
        // Runs are the player's own: the checkout carries none.
        for id in RUNNABLE {
            assert!(!root.join("port/game").join(id).join("interface-run.json").exists(), "{id}");
        }
    }
}
