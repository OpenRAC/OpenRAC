// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Enhanced graphics: the experimental probe lighting, modern shadows and ambient occlusion of the
//! native port (port/enhanced), as a built-in entry of the Mods tab. It is not a mod the player
//! installs: the game carries the code, off unless `enhanced_rendering = 1` is in the version's
//! settings file (docs/port/SETTINGS.md), and the entry writes that key and the others it shows
//! there, a line at a time, as the game fixes are (differences.rs).
//!
//! With it on, Play starts the game with the Vulkan renderer, the only one that has it, and with
//! `--probes <data>/probes/<version>`: the probe bakes made on this computer from the player's own
//! extracted levels by the `probe-bake` action (port/enhanced/tools/probes/bake_all.py --update).
//! Each baked level records what it was made from (`openrac-bake.json`); a level whose export or
//! baker changed since counts as not ready and is baked again. The baker is named by what it says
//! it bakes (`--print-profile`: its probe format, bake version and quality), not by its file, which
//! every build of a new commit changes. A level without a bake keeps the
//! original lighting. Intel Macs ship without the baker (the maintainers' decision: Embree's static
//! library cannot be built there, tools/build_port.py), so the entry says that there instead of
//! offering a bake.

use std::path::{Path, PathBuf};

use serde::Serialize;
use serde_json::Value;
use sha2::{Digest, Sha256};

use crate::differences::{read_switch, read_value, settings_file, with_value, write_settings};
use crate::plugins::{check_value, Choice, PluginView, SettingSpec, SettingView};

/// The entry's id on the Mods tab (`?mod=enhanced-graphics`).
pub const ID: &str = "enhanced-graphics";
/// The settings key that switches enhanced rendering on (port/game/common/settings.h).
pub const SWITCH: &str = "enhanced_rendering";
/// The action that bakes the probes (launcher/actions.json).
pub const BAKE_ACTION: &str = "probe-bake";
/// What each baked level records (bake_all.py, STAMP).
const STAMP: &str = "openrac-bake.json";
/// Why an Intel Mac has no bakes, as tools/build_port.py says it when it leaves the baker out.
pub const NO_BAKER_HERE: &str =
    "Intel Macs ship without the probe baker: their enhanced graphics keep the original lighting.";
/// Whether this computer gets the probe baker: every one but an Intel Mac (NO_BAKER_HERE). It is
/// the launcher's own build that counts, as `platform.machine()` does for build_port.py.
pub const SHIPS_BAKER: bool = !cfg!(all(target_os = "macos", target_arch = "x86_64"));

/// Whether a version has the entry: the bake planner reads RAC1's levels (PAL and NTSC-U).
pub fn supported(game: &str, version: &str) -> bool {
    game == "rac1" && matches!(version, "pal" | "ntsc")
}

/// Where a version's probe bakes are kept: `<data>/probes/<version>`.
pub fn probes_dir(data: &Path, version: &str) -> PathBuf {
    data.join("probes").join(version)
}

fn spec(key: &str, name: &str, kind: &str, default: Value, description: &str) -> SettingSpec {
    SettingSpec {
        key: key.into(),
        name: name.into(),
        kind: kind.into(),
        default,
        description: description.into(),
        min: None,
        max: None,
        step: None,
        choices: Vec::new(),
        max_length: None,
        binds: None,
        group: String::new(),
        multiplayer: None,
    }
}

fn range(mut s: SettingSpec, min: f64, max: f64, step: f64) -> SettingSpec {
    (s.min, s.max, s.step) = (Some(min), Some(max), Some(step));
    s
}

fn advanced(mut s: SettingSpec) -> SettingSpec {
    s.group = "advanced".into();
    s
}

/// The settings the entry's page shows, with the port's defaults and ranges (settings.cpp). The
/// diagnostic ones (probe spheres, receiver biases, GTAO directions and steps) stay in the game's
/// own Enhanced menu.
pub fn settings() -> Vec<SettingSpec> {
    let mut quality = spec("gtao_quality", "Occlusion quality", "choice", "performance".into(), "");
    quality.choices = vec![
        Choice { value: "performance".into(), name: "Performance".into() },
        Choice { value: "quality".into(), name: "Quality".into() },
    ];
    let mut shadows = spec(
        "shadow_quality",
        "Shadow quality",
        "choice",
        "high".into(),
        "Low uses the smallest shadow textures. Medium balances detail and cost. High keeps the sharpest shadows.",
    );
    shadows.choices = vec![
        Choice { value: "off".into(), name: "Off".into() },
        Choice { value: "low".into(), name: "Low".into() },
        Choice { value: "medium".into(), name: "Medium".into() },
        Choice { value: "high".into(), name: "High".into() },
    ];
    let mut texture_filter = spec(
        "texture_filter",
        "Texture upscaling",
        "choice",
        "edge-aware".into(),
        "Enlarges world textures while keeping edges sharp.",
    );
    texture_filter.choices = vec![
        Choice { value: "normal".into(), name: "Normal".into() },
        Choice { value: "edge-aware".into(), name: "Edge-aware (2x)".into() },
    ];
    let mut temporal = spec(
        "temporal_quality",
        "FSR temporal upscaling",
        "choice",
        "quality".into(),
        "Reconstructs a lower-resolution world image. The HUD stays at full resolution.",
    );
    temporal.choices = vec![
        Choice { value: "off".into(), name: "Off".into() },
        Choice { value: "native-aa".into(), name: "Native AA".into() },
        Choice { value: "quality".into(), name: "Quality".into() },
        Choice { value: "balanced".into(), name: "Balanced".into() },
        Choice { value: "performance".into(), name: "Performance".into() },
    ];
    vec![
        shadows,
        spec("gtao", "Ambient occlusion", "bool", true.into(), "Soft shade in corners and under objects."),
        quality,
        texture_filter,
        temporal,
        spec(
            "agx_preview",
            "Color Correction",
            "bool",
            true.into(),
            "Balances colour and contrast for a softer, more natural image, and lets you adjust brightness.",
        ),
        range(
            spec("agx_brightness", "Brightness", "float", 0.5.into(), "Makes the image darker or brighter."),
            0.0,
            1.0,
            0.01,
        ),
        spec("motion_blur", "Motion blur", "bool", true.into(), "Subtle motion blur to enhance animations."),
        advanced(range(spec("temporal_sharpness", "FSR sharpening", "float", 0.2.into(), ""), 0.0, 1.0, 0.05)),
    ]
}

/// A setting's value as the settings file holds it, or its default.
fn read_setting(text: &str, spec: &SettingSpec) -> Value {
    if spec.key == "shadow_quality" && read_switch(text, "enhanced_shadows") == Some(false) {
        return "off".into();
    }
    let raw = read_value(text, &spec.key);
    let value = match (spec.kind.as_str(), raw) {
        ("bool", _) => read_switch(text, &spec.key).map(Value::Bool),
        ("int", Some(v)) => v.parse::<i64>().ok().map(Value::from),
        ("float", Some(v)) => v.parse::<f64>().ok().map(Value::from),
        ("choice", Some(v)) => Some(Value::String(v)),
        _ => None,
    };
    value.and_then(|v| check_value(spec, &v)).unwrap_or_else(|| spec.default.clone())
}

/// A value as the port reads it: 1|0, a plain number, a choice's name.
fn written(value: &Value) -> String {
    match value {
        Value::Bool(on) => (if *on { "1" } else { "0" }).to_string(),
        Value::Number(n) => {
            let x = n.as_f64().unwrap_or(0.0);
            let text = format!("{x:.3}");
            text.trim_end_matches('0').trim_end_matches('.').to_string()
        }
        Value::String(s) => s.clone(),
        other => other.to_string(),
    }
}

/// How far the version's probe bakes are: its exported levels, the ones whose bake is current,
/// and the ones whose last bake failed.
#[derive(Debug, Clone, Default, Serialize, PartialEq, Eq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct BakeStatus {
    /// The bake can run: the levels are there and so is the baker; else `why` says what is missing.
    pub available: bool,
    pub why: String,
    /// This computer gets no baker at all (an Intel Mac, NO_BAKER_HERE): no bake is ever offered.
    pub no_baker_here: bool,
    pub levels: usize,
    pub ready: usize,
    pub failed: usize,
    /// Levels a stopped bake left part way: the next bake goes on from what they have.
    pub partial: usize,
}

fn sha256(path: &Path) -> Option<String> {
    let bytes = std::fs::read(path).ok()?;
    Some(format!("{:x}", Sha256::digest(&bytes)))
}

/// What `baker` makes of `level` (`--print-profile`), as bake_all.py's `baker_profile` reads it:
/// its digest names the baker in a level's stamp, the same for every build of the same baker.
/// None when it cannot say. Asked once for each build of the baker and each level's files (a few
/// tens of milliseconds a level).
fn baker_profile(baker: &Path, level: &Path) -> Option<String> {
    use std::collections::HashMap;
    use std::sync::Mutex;
    type Key = (PathBuf, PathBuf, String);
    static CACHE: Mutex<Option<HashMap<Key, Option<String>>>> = Mutex::new(None);
    let file = |p: &Path| std::fs::metadata(p).ok().map(|m| (m.len(), m.modified().ok()));
    let key = (
        baker.to_path_buf(),
        level.to_path_buf(),
        format!(
            "{:?} {:?} {:?}",
            file(baker),
            file(&level.join("probe-bake.json")),
            file(&level.join("manifest.json"))
        ),
    );
    if let Some(known) = CACHE.lock().unwrap().get_or_insert_with(HashMap::new).get(&key) {
        return known.clone();
    }
    let said = crate::detect::quiet(baker)
        .arg("--level-dir")
        .arg(level)
        .arg("--print-profile")
        .output()
        .ok()
        .filter(|o| o.status.success())
        .map(|o| String::from_utf8_lossy(&o.stdout).trim().to_string());
    CACHE.lock().unwrap().get_or_insert_with(HashMap::new).insert(key, said.clone());
    said
}

/// Whether the bake in `baked`, whose stamp is `stamp`, was made by the baker whose profile is
/// `profile`. A stamp of version 2 named the baker by its file's digest: it counts when its bake
/// has the profile's probe format and bake version (bake_all.py, up_to_date).
fn made_by(stamp: &Value, baked: &Path, profile: &str) -> bool {
    let digest = format!("{:x}", Sha256::digest(profile.as_bytes()));
    if stamp["baker_profile_sha256"].as_str() == Some(digest.as_str()) {
        return true;
    }
    if stamp["version"].as_u64() != Some(2) {
        return false;
    }
    let (Ok(profile), Some(result)) = (
        serde_json::from_str::<Value>(profile),
        std::fs::read_to_string(baked.join("bake-result.json"))
            .ok()
            .and_then(|t| serde_json::from_str::<Value>(&t).ok()),
    ) else {
        return false;
    };
    ["bake_version", "probe_format_version"].iter().all(|k| !result[k].is_null() && result[k] == profile[k])
}

fn verified_bake(baked: &Path) -> bool {
    let Some(result) = std::fs::read_to_string(baked.join("bake-result.json"))
        .ok()
        .and_then(|text| serde_json::from_str::<Value>(&text).ok())
    else {
        return false;
    };
    if result["verified"] != Value::Bool(true) {
        return false;
    }
    if result["empty"] == Value::Bool(true) {
        return true;
    }
    let present = |name: &str| {
        Path::new(name).file_name().and_then(|n| n.to_str()) == Some(name)
            && name.ends_with(".orpg")
            && std::fs::metadata(baked.join(name)).is_ok_and(|m| m.is_file() && m.len() > 0)
    };
    present("sparse.orpg")
        && match result.get("regions") {
            None => true,
            Some(Value::Array(regions)) => regions.iter().all(|r| r.as_str().is_some_and(present)),
            _ => false,
        }
}

/// The bake status of the levels in `levels` against the bakes in `probes`, made by `baker`.
pub fn bake_status(levels: &Path, probes: &Path, baker: &Path) -> BakeStatus {
    bake_status_on(levels, probes, baker, SHIPS_BAKER, &|level| baker_profile(baker, level))
}

/// [`bake_status`] on a computer that gets the baker or not (`ships_baker`, SHIPS_BAKER), with the
/// baker's profile of a level from `profile`. Without it and without a baker, NO_BAKER_HERE is the
/// reason whatever the levels: preparing them would not make a bake possible.
fn bake_status_on(
    levels: &Path,
    probes: &Path,
    baker: &Path,
    ships_baker: bool,
    profile: &dyn Fn(&Path) -> Option<String>,
) -> BakeStatus {
    let mut out = BakeStatus::default();
    let has_baker = baker.is_file();
    if !has_baker && !ships_baker {
        out.no_baker_here = true;
        out.why = NO_BAKER_HERE.into();
    }
    let failures: Option<Value> = std::fs::read_to_string(probes.join("bake-all-result.json"))
        .ok()
        .and_then(|text| serde_json::from_str(&text).ok());
    let Ok(entries) = std::fs::read_dir(levels) else {
        if !out.no_baker_here {
            out.why = "the levels are not ready yet".into();
        }
        return out;
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().into_owned();
        let numbered =
            name.strip_prefix("level_").is_some_and(|n| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()));
        let manifest = entry.path().join("manifest.json");
        if !numbered || !manifest.is_file() {
            continue;
        }
        out.levels += 1;
        let baked = probes.join(&name);
        let stamp: Option<Value> =
            std::fs::read_to_string(baked.join(STAMP)).ok().and_then(|t| serde_json::from_str(&t).ok());
        let current = stamp.as_ref().is_some_and(|s| {
            s["manifest_sha256"].as_str() == sha256(&manifest).as_deref()
                && has_baker
                && verified_bake(&baked)
                && profile(&entry.path()).is_some_and(|p| made_by(s, &baked, &p))
        });
        if current {
            out.ready += 1;
        } else if probes.join(format!(".{name}.failed")).is_dir()
            || failures.as_ref().and_then(|r| r["levels"].as_array()).is_some_and(|rows| {
                rows.iter().any(|row| row["level"].as_str() == Some(name.as_str()) && row["error"].is_string())
            })
        {
            out.failed += 1;
        } else if probes.join(format!(".{name}.partial")).is_dir() {
            out.partial += 1;
        }
    }
    if out.no_baker_here {
        return out;
    }
    if out.levels == 0 {
        out.why = "the levels are not ready yet".into();
    } else if !has_baker {
        out.why = "this build of the game has no probe baker".into();
    } else {
        out.available = true;
    }
    out
}

/// Resolve only a real .log file inside this version's lighting cache. Canonical paths reject
/// traversal and symlinks pointing outside the cache; tool output alone grants no file access.
pub fn bake_log(config: &crate::config::Config, key: &str, path: &Path) -> Result<PathBuf, String> {
    let (_, probes, _) = bake_folders(config, key)?;
    checked_bake_log(&probes, path)
}

fn checked_bake_log(probes: &Path, path: &Path) -> Result<PathBuf, String> {
    let root = probes.canonicalize().map_err(|e| format!("Lighting folder: {e}"))?;
    let target = path.canonicalize().map_err(|e| format!("Lighting log: {e}"))?;
    if !target.starts_with(&root)
        || !target.is_file()
        || !target.extension().and_then(|ext| ext.to_str()).is_some_and(|ext| ext.eq_ignore_ascii_case("log"))
    {
        return Err("Only log files inside this game's lighting folder can be opened".into());
    }
    Ok(target)
}

/// A version's (`rac1/pal`) exported levels, its bakes' folder and its baker, from its probe-bake
/// action.
fn bake_folders(config: &crate::config::Config, key: &str) -> Result<(PathBuf, PathBuf, PathBuf), String> {
    let value_after = |args: &[String], flag: &str| {
        args.iter().position(|a| a == flag).and_then(|i| args.get(i + 1)).map(PathBuf::from)
    };
    let (args, artifact) = crate::library::filled(config, key, BAKE_ACTION)?;
    match (value_after(&args, "--levels-dir"), value_after(&args, "--output-dir")) {
        (Some(levels), Some(probes)) => {
            let levels =
                if levels.is_absolute() { levels } else { config.root().map(|r| r.join(&levels)).unwrap_or(levels) };
            Ok((levels, probes, artifact.unwrap_or_default()))
        }
        _ => Err("the probe bake names no folders".into()),
    }
}

/// The bake status of a version (`rac1/pal`) from its probe-bake action's folders and baker.
pub fn version_bake_status(config: &crate::config::Config, key: &str) -> BakeStatus {
    match bake_folders(config, key) {
        Ok((levels, probes, baker)) => bake_status(&levels, &probes, &baker),
        Err(e) => BakeStatus { why: e, ..BakeStatus::default() },
    }
}

/// Deletes every bake of a version (`rac1/pal`): the finished levels, the ones a stopped bake left
/// part way and the failed ones' logs. The game keeps the original lighting until they are baked
/// again; the levels and the settings are not touched. A folder that is not the bakes' (one that
/// holds anything but level bakes and the bake's report) is refused rather than emptied.
pub fn delete_bakes(config: &crate::config::Config, key: &str) -> Result<(), String> {
    let (_, probes, _) = bake_folders(config, key)?;
    delete_bake_folder(&probes)
}

fn delete_bake_folder(probes: &Path) -> Result<(), String> {
    let Ok(entries) = std::fs::read_dir(probes) else {
        return Ok(()); // nothing baked yet
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().into_owned();
        let level = name.trim_start_matches('.');
        let level = level
            .strip_suffix(".partial")
            .or_else(|| level.strip_suffix(".failed"))
            .or_else(|| level.strip_suffix(".previous"))
            .unwrap_or(level);
        let bake = level.strip_prefix("level_").is_some_and(|n| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()));
        if !bake && !name.starts_with("bake-all-result.json") && name != "probe-bake.log" && name != "probe-bake.lock" {
            return Err(format!("{} holds {name}, which is not a bake: nothing was deleted", probes.display()));
        }
    }
    std::fs::remove_dir_all(probes).map_err(|e| format!("could not delete {}: {e}", probes.display()))
}

/// The entry as the Mods tab shows it: a mod's row and page, with the bake's status beside it.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct EnhancedView {
    #[serde(rename = "mod")]
    pub entry: PluginView,
    pub bake: BakeStatus,
}

/// The entry for `game`/`version`, its values from the version's settings file in `data_home`.
pub fn view(data_home: &Path, game: &str, version: &str, bake: BakeStatus) -> EnhancedView {
    let text = std::fs::read_to_string(settings_file(data_home, game, version)).unwrap_or_default();
    let enabled = read_switch(&text, SWITCH).unwrap_or(false);
    let settings = settings()
        .into_iter()
        .map(|spec| {
            let value = read_setting(&text, &spec);
            SettingView { spec, value, conflicts: Vec::new() }
        })
        .collect();
    EnhancedView {
        entry: PluginView {
            id: ID.into(),
            name: "Enhanced graphics".into(),
            version: "Experimental".into(),
            author: "OpenRAC".into(),
            description:
                "Improved lighting and effects whilst preserving the charm of the original visuals we all love.".into(),
            homepage: String::new(),
            license: "GPL-3.0-or-later".into(),
            enabled,
            state: if enabled { "ready" } else { "off" }.into(),
            reason: String::new(),
            blocked: String::new(),
            settings,
            presets: Vec::new(),
            hooks: Vec::new(),
            shared_hooks: Vec::new(),
            native: false,
            kind: "built-in".into(),
            path: String::new(),
        },
        bake,
    }
}

/// Switches enhanced rendering on or off for the version.
pub fn set_enabled(data_home: &Path, game: &str, version: &str, on: bool) -> Result<(), String> {
    let file = settings_file(data_home, game, version);
    let text = std::fs::read_to_string(&file).unwrap_or_default();
    write_settings(&file, &with_value(&text, SWITCH, if on { "1" } else { "0" }))
}

/// Sets one of the entry's settings, checked as the port would read it.
pub fn set_setting(data_home: &Path, game: &str, version: &str, key: &str, value: &Value) -> Result<(), String> {
    let all = settings();
    let spec = all.iter().find(|s| s.key == key).ok_or_else(|| format!("{key} is not an enhanced graphics setting"))?;
    let value = check_value(spec, value).ok_or_else(|| format!("{} cannot be {value}", spec.name))?;
    let file = settings_file(data_home, game, version);
    let text = std::fs::read_to_string(&file).unwrap_or_default();
    let text = if key == "shadow_quality" {
        // Keep the renderer's quality enum and legacy enable switch compatible.
        let enabled = value != "off";
        let text = with_value(&text, "enhanced_shadows", if enabled { "1" } else { "0" });
        if enabled {
            with_value(&text, key, &written(&value))
        } else {
            text
        }
    } else {
        with_value(&text, key, &written(&value))
    };
    write_settings(&file, &text)
}

/// What Play adds: environment variables, then arguments.
pub type PlayAdditions = (Vec<(String, String)>, Vec<String>);

/// What Play adds when the version's settings switch enhanced rendering on: the Vulkan renderer
/// and the probe bakes' folder. `settings` is the version's settings file's text.
pub fn play_additions(settings: &str, data: &Path, version: &str) -> Option<PlayAdditions> {
    if !read_switch(settings, SWITCH).unwrap_or(false) {
        return None;
    }
    let env = vec![("OPENRAC_GL_API".to_string(), "vulkan".to_string())];
    let args = vec!["--probes".to_string(), probes_dir(data, version).display().to_string()];
    Some((env, args))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temp(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("openrac-enhanced-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    fn unverified_or_incomplete_lighting_is_not_ready() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path();
        std::fs::write(root.join("sparse.orpg"), "probe bytes").unwrap();
        for text in [r#"{"verified":false}"#, r#"{"verified":true,"regions":["missing.orpg"]}"#] {
            std::fs::write(root.join("bake-result.json"), text).unwrap();
            assert!(!verified_bake(root));
        }
        std::fs::write(root.join("bake-result.json"), r#"{"verified":true,"regions":[]}"#).unwrap();
        assert!(verified_bake(root));
        std::fs::write(root.join("sparse.orpg"), "").unwrap();
        assert!(!verified_bake(root));
    }

    #[test]
    fn lighting_logs_stay_inside_the_versions_cache() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().join("lighting");
        std::fs::create_dir(&root).unwrap();
        let log = root.join("probe-bake.log");
        std::fs::write(&log, "test diagnostic").unwrap();
        assert_eq!(checked_bake_log(&root, &log).unwrap(), log.canonicalize().unwrap());
        let outside = temp.path().join("other.log");
        std::fs::write(&outside, "outside").unwrap();
        assert!(checked_bake_log(&root, &outside).is_err());
        assert!(checked_bake_log(&root, &root.join("../other.log")).is_err());
        let executable = root.join("tool.exe");
        std::fs::write(&executable, "not a log").unwrap();
        assert!(checked_bake_log(&root, &executable).is_err());
        assert!(checked_bake_log(&root, &root).is_err());
        assert!(checked_bake_log(&root, &root.join("missing.log")).is_err());
        #[cfg(unix)]
        {
            let link = root.join("outside.log");
            std::os::unix::fs::symlink(&outside, &link).unwrap();
            assert!(checked_bake_log(&root, &link).is_err());
        }
    }

    #[test]
    fn simplified_settings_preserve_shadow_compatibility() {
        let home = temp("shadow-options");
        let all = settings();
        assert_eq!(
            all.iter().filter(|s| s.group == "advanced").map(|s| s.key.as_str()).collect::<Vec<_>>(),
            vec!["temporal_sharpness"]
        );
        assert!(all
            .iter()
            .all(|s| s.key != "enhanced_shadows" && (s.key == "motion_blur" || !s.key.starts_with("motion_blur_"))));
        for version in ["pal", "ntsc"] {
            let file = settings_file(&home, "rac1", version);
            std::fs::create_dir_all(file.parent().unwrap()).unwrap();
            std::fs::write(&file, "enhanced_shadows = 0\nshadow_quality = low\nmotion_blur = 0\n").unwrap();
            let shadow = all.iter().find(|s| s.key == "shadow_quality").unwrap();
            let text = std::fs::read_to_string(&file).unwrap();
            assert_eq!(read_setting(&text, shadow), Value::from("off"));
            for quality in ["low", "medium", "high", "off"] {
                set_setting(&home, "rac1", version, "shadow_quality", &Value::from(quality)).unwrap();
                let text = std::fs::read_to_string(&file).unwrap();
                assert_eq!(read_setting(&text, shadow), Value::from(quality));
                assert_eq!(read_switch(&text, "enhanced_shadows"), Some(quality != "off"));
                assert_eq!(read_switch(&text, "motion_blur"), Some(false));
                assert_ne!(read_value(&text, "shadow_quality"), Some("off".into()));
            }
        }
        let _ = std::fs::remove_dir_all(home);
    }

    #[test]
    fn off_until_switched_on_and_settings_land_in_the_ports_file() {
        let home = temp("settings");
        let file = settings_file(&home, "rac1", "pal");
        std::fs::create_dir_all(file.parent().unwrap()).unwrap();
        std::fs::write(&file, "msaa = 4\ngtao = 1\nprobe_lighting = 0\nagx_exposure = 1.5\n").unwrap();
        let v = view(&home, "rac1", "pal", BakeStatus::default());
        assert!(!v.entry.enabled);
        let value =
            |v: &EnhancedView, key: &str| v.entry.settings.iter().find(|s| s.spec.key == key).unwrap().value.clone();
        assert_eq!(value(&v, "gtao"), Value::Bool(true));
        assert!(v.entry.settings.iter().all(|s| s.spec.key != "probe_lighting"));
        assert!(set_setting(&home, "rac1", "pal", "probe_lighting", &Value::Bool(false)).is_err());
        assert_eq!(value(&v, "texture_filter"), Value::from("edge-aware"));
        assert_eq!(value(&v, "temporal_quality"), Value::from("quality"));
        assert_eq!(value(&v, "shadow_quality"), Value::from("high"));
        assert_eq!(value(&v, "temporal_sharpness"), Value::from(0.2));
        assert_eq!(value(&v, "agx_preview"), Value::Bool(true));
        assert_eq!(value(&v, "motion_blur"), Value::Bool(true));

        assert_eq!(value(&v, "agx_brightness"), Value::from(0.5));
        for brightness in [0.0, 0.5, 1.0] {
            set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(brightness)).unwrap();
            assert_eq!(
                value(&view(&home, "rac1", "pal", BakeStatus::default()), "agx_brightness"),
                Value::from(brightness)
            );
        }
        assert!(set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(1.01)).is_err());
        assert!(set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(-0.01)).is_err());
        set_setting(&home, "rac1", "pal", "agx_preview", &Value::Bool(false)).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "agx_preview"), Value::Bool(false));
        set_enabled(&home, "rac1", "pal", true).unwrap();
        set_setting(&home, "rac1", "pal", "motion_blur", &Value::Bool(true)).unwrap();
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("low")).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "shadow_quality"), Value::from("low"));
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("medium")).unwrap();
        set_setting(&home, "rac1", "pal", "gtao_quality", &Value::from("quality")).unwrap();
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("off")).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "shadow_quality"), Value::from("off"));
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("medium")).unwrap();
        set_setting(&home, "rac1", "pal", "texture_filter", &Value::from("normal")).unwrap();
        let text = std::fs::read_to_string(&file).unwrap();
        assert!(text.starts_with("msaa = 4\ngtao = 1\n"), "{text}");
        for line in [
            "enhanced_rendering = 1",
            "gtao_quality = quality",
            "enhanced_shadows = 1",
            "shadow_quality = medium",
            "texture_filter = normal",
            "agx_exposure = 1.5",
            "agx_preview = 0",
            "motion_blur = 1",
        ] {
            assert!(text.lines().any(|l| l == line), "{line} in {text}");
        }
        let v = view(&home, "rac1", "pal", BakeStatus::default());
        assert!(v.entry.enabled);
        assert_eq!(value(&v, "texture_filter"), Value::from("normal"));
        assert_eq!(value(&v, "shadow_quality"), Value::from("medium"));
        assert_eq!(value(&v, "motion_blur"), Value::Bool(true));
        // Out of the port's range, another type, or not one of the entry's: refused, file kept.
        assert!(set_setting(&home, "rac1", "pal", "gtao", &Value::from("yes")).is_err());
        assert!(set_setting(&home, "rac1", "pal", "probe_spheres", &Value::Bool(true)).is_err());
        assert!(set_setting(&home, "rac1", "pal", "texture_filter", &Value::from("unknown")).is_err());
        assert!(set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("unknown")).is_err());
        assert_eq!(std::fs::read_to_string(&file).unwrap(), text);
        let _ = std::fs::remove_dir_all(&home);
    }

    #[test]
    fn play_takes_vulkan_and_the_bakes_only_when_on() {
        let data = Path::new("/data/rac1");
        assert_eq!(play_additions("", data, "pal"), None);
        assert_eq!(play_additions("enhanced_rendering = 0\n", data, "pal"), None);
        let (env, args) = play_additions("enhanced_rendering = 1\n", data, "pal").unwrap();
        assert_eq!(env, vec![("OPENRAC_GL_API".to_string(), "vulkan".to_string())]);
        // Joined, not spelled out: Windows joins with a backslash.
        let probes = data.join("probes").join("pal");
        assert_eq!(args, vec!["--probes".to_string(), probes.display().to_string()]);
    }

    #[test]
    fn a_level_counts_as_ready_while_its_export_and_baker_are_the_bakes() {
        let root = temp("bake");
        let (levels, probes, baker) = (root.join("levels"), root.join("probes"), root.join("baker"));
        for name in ["level_00", "level_01", "level_99"] {
            std::fs::create_dir_all(levels.join(name)).unwrap();
            std::fs::write(levels.join(name).join("manifest.json"), name).unwrap();
        }
        std::fs::create_dir_all(levels.join("textures")).unwrap();
        // The fake baker's profile is its file's text.
        let profile = |_: &Path| std::fs::read_to_string(root.join("baker")).ok();
        let status = || bake_status_on(&levels, &probes, &baker, true, &profile);
        let none = status();
        assert_eq!((none.available, none.levels, none.ready), (false, 3, 0));
        std::fs::write(&baker, "baker").unwrap();
        let stamp = |name: &str, manifest: &str| {
            let dir = probes.join(name);
            std::fs::create_dir_all(&dir).unwrap();
            std::fs::write(dir.join("sparse.orpg"), "probe bytes").unwrap();
            std::fs::write(dir.join("bake-result.json"), r#"{"verified":true}"#).unwrap();
            let text = serde_json::json!({
                "version": 3,
                "manifest_sha256": format!("{:x}", Sha256::digest(manifest.as_bytes())),
                "baker_profile_sha256": format!("{:x}", Sha256::digest(b"baker")),
            });
            std::fs::write(dir.join(STAMP), text.to_string()).unwrap();
        };
        stamp("level_00", "level_00");
        stamp("level_01", "an older export");
        std::fs::create_dir_all(probes.join(".level_99.failed")).unwrap();
        let some = status();
        assert_eq!((some.available, some.levels, some.ready, some.failed), (true, 3, 1, 1));
        // A level with nothing to bake (no sparse.orpg, an empty result) counts as ready.
        std::fs::remove_dir_all(probes.join(".level_99.failed")).unwrap();
        stamp("level_99", "level_99");
        std::fs::remove_file(probes.join("level_99").join("sparse.orpg")).unwrap();
        std::fs::write(probes.join("level_99").join("bake-result.json"), r#"{"verified": true, "empty": true}"#)
            .unwrap();
        assert_eq!(status().ready, 2);
        std::fs::remove_dir_all(probes.join("level_99")).unwrap();
        std::fs::create_dir_all(probes.join(".level_99.failed")).unwrap();
        // A baker that bakes otherwise (a new probe format, other quality): every bake is made again.
        std::fs::write(&baker, "another baker").unwrap();
        assert_eq!(status().ready, 0);
        // Another build of the same baker (its file differs, its profile does not): kept.
        let same = |_: &Path| Some("baker".to_string());
        assert_eq!(bake_status_on(&levels, &probes, &baker, true, &same).ready, 1);
        let _ = std::fs::remove_dir_all(&root);
    }

    #[test]
    fn a_bake_stamped_with_the_bakers_file_counts_while_its_versions_are_the_bakers() {
        let root = temp("legacy");
        let (levels, probes, baker) = (root.join("levels"), root.join("probes"), root.join("baker"));
        std::fs::create_dir_all(levels.join("level_00")).unwrap();
        std::fs::write(levels.join("level_00").join("manifest.json"), "level_00").unwrap();
        std::fs::write(&baker, "a build").unwrap();
        let dir = probes.join("level_00");
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(dir.join("sparse.orpg"), "probe bytes").unwrap();
        let old = serde_json::json!({
            "version": 2,
            "manifest_sha256": format!("{:x}", Sha256::digest(b"level_00")),
            "baker_sha256": "an older build",
        });
        std::fs::write(dir.join(STAMP), old.to_string()).unwrap();
        std::fs::write(
            dir.join("bake-result.json"),
            r#"{"verified":true, "bake_version": 1, "probe_format_version": 3}"#,
        )
        .unwrap();
        let profile = |_: &Path| Some(r#"{"bake_version":1,"probe_format_version":3,"rays":1024}"#.to_string());
        assert_eq!(bake_status_on(&levels, &probes, &baker, true, &profile).ready, 1);
        let newer = |_: &Path| Some(r#"{"bake_version":2,"probe_format_version":3}"#.to_string());
        assert_eq!(bake_status_on(&levels, &probes, &baker, true, &newer).ready, 0);
        let _ = std::fs::remove_dir_all(&root);
    }

    #[test]
    fn deleting_the_bakes_empties_only_a_folder_of_bakes() {
        let root = temp("delete");
        let probes = root.join("probes");
        for dir in ["level_00", ".level_01.partial", ".level_02.failed", ".level_03.previous"] {
            std::fs::create_dir_all(probes.join(dir)).unwrap();
        }
        std::fs::write(probes.join("bake-all-result.json"), "{}").unwrap();
        std::fs::write(probes.join("probe-bake.log"), "diagnostic").unwrap();
        std::fs::write(probes.join("probe-bake.lock"), "0").unwrap();
        delete_bake_folder(&probes).unwrap();
        assert!(!probes.exists());
        // Nothing baked: nothing to do.
        delete_bake_folder(&probes).unwrap();
        // A folder with something else in it (a wrong setting) is left whole.
        std::fs::create_dir_all(probes.join("level_00")).unwrap();
        std::fs::write(probes.join("my notes.txt"), "mine").unwrap();
        assert!(delete_bake_folder(&probes).is_err());
        assert!(probes.join("my notes.txt").is_file() && probes.join("level_00").is_dir());
        let _ = std::fs::remove_dir_all(&root);
    }

    #[test]
    fn an_intel_mac_says_it_ships_without_the_baker_instead_of_offering_a_bake() {
        let root = temp("intel");
        let (levels, probes, baker) = (root.join("levels"), root.join("probes"), root.join("baker"));
        // Before the levels are ready, and after: the reason stays the missing baker.
        for ready in [false, true] {
            if ready {
                std::fs::create_dir_all(levels.join("level_00")).unwrap();
                std::fs::write(levels.join("level_00").join("manifest.json"), "level_00").unwrap();
            }
            let here = bake_status_on(&levels, &probes, &baker, false, &|_| None);
            assert!(!here.available && here.no_baker_here, "{here:?}");
            assert_eq!(here.why, NO_BAKER_HERE);
            assert_eq!(here.levels, usize::from(ready));
            // Elsewhere the existing reasons are kept.
            let elsewhere = bake_status_on(&levels, &probes, &baker, true, &|_| None);
            assert!(!elsewhere.available && !elsewhere.no_baker_here);
            let why = if ready { "this build of the game has no probe baker" } else { "the levels are not ready yet" };
            assert_eq!(elsewhere.why, why);
        }
        // A baker that is there anyway (built by hand) is used.
        std::fs::write(&baker, "baker").unwrap();
        let built = bake_status_on(&levels, &probes, &baker, false, &|_| None);
        assert!(built.available && !built.no_baker_here && built.why.is_empty(), "{built:?}");
        let _ = std::fs::remove_dir_all(&root);
    }

    #[test]
    fn the_frontend_reads_whether_this_computer_gets_a_baker() {
        // launcher/src/lib/api.ts, BakeStatus.noBakerHere.
        let json = serde_json::to_value(BakeStatus { no_baker_here: true, ..BakeStatus::default() }).unwrap();
        assert_eq!(json["noBakerHere"], Value::Bool(true));
    }

    #[test]
    fn written_values_read_back_as_the_port_parses_them() {
        assert_eq!(written(&Value::from(1.0)), "1");
        assert_eq!(written(&Value::from(1.25)), "1.25");
        assert_eq!(written(&Value::from(0.05)), "0.05");
        assert_eq!(written(&Value::Bool(false)), "0");
    }
}
