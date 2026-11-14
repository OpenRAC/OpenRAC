// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Interface texture runs (docs/port/TEXTURE_PACKS.md, "Export without playing"): what the Models
//! and textures panel says of a version's runs before its export.
//!
//! A run file (port/game/common/program/interface_runs.h) lists the interface textures a run of the
//! game made and where their bytes are on the disc. The project ships an official one for some
//! versions (`<root>/port/game/<id>/interface-run.json`); a player can make their own
//! (`<data>/interface_runs/<id>.json`, port/tools/interface_runs/run.py). The export reads both.
//! This module reads only what the panel shows: for each kind of texture, whether a run has it and
//! which; and whether the player's own run is worth offering to the project.

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
    /// Every route played to its end.
    pub clean: bool,
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
    /// The project's run, if this copy carries one.
    pub official: Option<Run>,
    /// The player's own, if they made one.
    pub local: Option<Run>,
    /// A run file that is there but cannot be read (cut short, another format, another game):
    /// which, in words.
    pub unreadable: Vec<String>,
    /// For each kind (CATEGORIES' order): "official", "local" or "none": the run the export takes it
    /// from.
    pub sources: [String; 3],
    /// Whether this version's routes can be played here (a run can be made).
    pub runnable: bool,
    /// The player's own run is clean and current and has textures the official one lacks: the panel
    /// suggests offering it to the project.
    pub suggest_sharing: bool,
    /// The player's own run file.
    pub local_file: String,
    /// Its copy named as the project keeps it (`share/<id>/interface-run.json` beside it), which
    /// run.py writes after a clean run: the file a pull request adds.
    pub share_file: String,
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
}

#[derive(Deserialize)]
struct Texture {
    category: Option<String>,
}

/// The project's run for version `id` in the copy at `root`.
pub fn official_file(root: &Path, id: &str) -> PathBuf {
    root.join("port").join("game").join(id).join("interface-run.json")
}

/// The player's own run for version `id`: `<data>/interface_runs/<id>.json`.
pub fn local_file(data: &Path, id: &str) -> PathBuf {
    data.join("interface_runs").join(format!("{id}.json"))
}

/// The copy of the player's clean run that a pull request adds: `<data>/interface_runs/share/<id>/interface-run.json`.
pub fn share_file(data: &Path, id: &str) -> PathBuf {
    data.join("interface_runs").join("share").join(id).join("interface-run.json")
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
    let made = file.made.filter(|m| m.len() == 10 && m.chars().all(|c| c.is_ascii_digit() || c == '-'));
    Ok(Some(Run { made, counts, clean: file.clean.unwrap_or(false), current: file.converter == Some(CONVERTER) }))
}

/// What the panel shows for `game` `version`: `root` is the OpenRAC copy (a checkout or the
/// carried one), `data` the game's data folder.
pub fn view(root: Option<&Path>, data: &Path, game: &str, version: &str) -> Result<View, String> {
    let id = texture_packs::id(game, version)?;
    let mut unreadable = Vec::new();
    let mut take = |path: Option<PathBuf>| match path.map(|p| read(&p, &id)) {
        Some(Ok(run)) => run,
        Some(Err(why)) => {
            unreadable.push(why);
            None
        }
        None => None,
    };
    let official = take(root.map(|r| official_file(r, &id)));
    let local_path = local_file(data, &id);
    let share = share_file(data, &id);
    let local = take(Some(local_path.clone()));
    let usable = |run: &Option<Run>, at: usize| run.as_ref().is_some_and(|r| r.current && r.counts[at] > 0);
    let sources = std::array::from_fn(|at| {
        if usable(&official, at) {
            "official"
        } else if usable(&local, at) {
            "local"
        } else {
            "none"
        }
        .to_string()
    });
    let suggest_sharing = local.as_ref().is_some_and(|l| {
        l.clean
            && l.current
            && (0..3).any(|at| l.counts[at] > official.as_ref().filter(|o| o.current).map_or(0, |o| o.counts[at]))
    });
    Ok(View {
        runnable: RUNNABLE.contains(&id.as_str()),
        id,
        official,
        local,
        unreadable,
        sources,
        suggest_sharing,
        local_file: local_path.display().to_string(),
        share_file: share.display().to_string(),
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
        let v = view(Some(dir.path()), dir.path(), "rac1", "ntsc").unwrap();
        assert_eq!((v.official, v.local), (None, None));
        assert_eq!(v.sources, ["none", "none", "none"].map(String::from));
        assert!(v.runnable && !v.suggest_sharing && v.unreadable.is_empty());
        assert!(!view(None, dir.path(), "rac3", "ntsc").unwrap().runnable);
    }

    #[test]
    fn each_kind_from_the_official_run_first_then_the_players() {
        let dir = tempfile::tempdir().unwrap();
        let (root, data) = (dir.path().join("root"), dir.path().join("data"));
        write(&official_file(&root, "rac1-ntsc"), &run("rac1-ntsc", CONVERTER, true, &["font", "interface"]));
        write(&local_file(&data, "rac1-ntsc"), &run("rac1-ntsc", CONVERTER, true, &["interface", "effects"]));
        let v = view(Some(&root), &data, "rac1", "ntsc").unwrap();
        assert_eq!(v.sources, ["official", "official", "local"].map(String::from));
        assert_eq!(v.official.as_ref().unwrap().made.as_deref(), Some("2026-11-14"));
        // The player's run has effects the official one lacks: worth offering.
        assert!(v.suggest_sharing);
    }

    #[test]
    fn no_suggestion_for_a_run_with_failures_or_nothing_new() {
        let dir = tempfile::tempdir().unwrap();
        let (root, data) = (dir.path().join("root"), dir.path().join("data"));
        write(&official_file(&root, "rac1-pal"), &run("rac1-pal", CONVERTER, true, &["font", "effects"]));
        write(&local_file(&data, "rac1-pal"), &run("rac1-pal", CONVERTER, true, &["font"]));
        assert!(!view(Some(&root), &data, "rac1", "pal").unwrap().suggest_sharing);
        write(&local_file(&data, "rac1-pal"), &run("rac1-pal", CONVERTER, false, &["interface"]));
        assert!(!view(Some(&root), &data, "rac1", "pal").unwrap().suggest_sharing);
    }

    #[test]
    fn an_out_of_date_or_broken_run_is_not_used() {
        let dir = tempfile::tempdir().unwrap();
        let data = dir.path();
        write(&local_file(data, "rac2-ntsc"), &run("rac2-ntsc", CONVERTER + 1, true, &["font"]));
        let v = view(None, data, "rac2", "ntsc").unwrap();
        assert!(!v.local.as_ref().unwrap().current);
        assert_eq!(v.sources[0], "none");
        assert!(!v.suggest_sharing);
        // Cut short, another game's, another format: said, not used.
        for text in [&run("rac2-ntsc", CONVERTER, true, &["font"])[..40], &run("rac1-pal", CONVERTER, true, &["font"]),
            r#"{"format": 2, "game": "rac2-ntsc"}"#]
        {
            write(&local_file(data, "rac2-ntsc"), text);
            let v = view(None, data, "rac2", "ntsc").unwrap();
            assert_eq!((v.local, v.unreadable.len()), (None, 1));
        }
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
            for a in list.as_array().unwrap().iter().filter(|a| a["id"] == "interface-export") {
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
        // Each official run in the checkout is one of its version's, current, and clean.
        for id in RUNNABLE {
            if let Some(run) = read(&official_file(&root, id), id).unwrap() {
                assert!(run.current && run.clean, "{id}'s official run");
            }
        }
    }
}
