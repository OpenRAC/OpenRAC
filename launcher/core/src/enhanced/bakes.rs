// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//! Lighting and water preparation status, logs and cache management.

use super::{BAKE_ACTION, NO_BAKER_HERE, SHIPS_BAKER};
use serde::Serialize;
use serde_json::Value;
use sha2::{Digest, Sha256};
use std::path::{Path, PathBuf};

const STAMP: &str = "openrac-bake.json";

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

/// What `baker` makes of `level` (`--print-profile`):
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

/// Current native bake stamps must match the resolved transport profile exactly.
fn made_by(stamp: &Value, profile: &str) -> bool {
    let digest = format!("{:x}", Sha256::digest(profile.as_bytes()));
    stamp["version"] == 5 && stamp["baker_profile_sha256"].as_str() == Some(digest.as_str())
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
            Some(Value::Array(regions)) => regions.iter().all(|r| r.as_str().is_some_and(present)),
            _ => false,
        }
}

/// Declarative water sources and both outputs must match their current native cache stamp.
fn prepared_water(level: &Path, manifest: &Path) -> bool {
    let read = |path: &Path| -> Value {
        std::fs::read_to_string(path).ok().and_then(|text| serde_json::from_str(&text).ok()).unwrap_or(Value::Null)
    };
    let profile = read(&level.join("probe-bake.json"));
    if profile["water_sources"].is_null() {
        return read(manifest)["bake_inputs"]["water"].is_null();
    }
    let stamp = read(&level.join("water-bake.json"));
    stamp["version"] == 3
        && stamp["inputs"]["algorithm"] == "native-water-sources-v1"
        && ["bake-inputs/overlay.bin", "collision.glb", "probe-bake.json"].iter().all(|name| level.join(name).is_file())
        && ["water.bin", "water.mesh"].iter().enumerate().all(|(i, name)| {
            sha256(&level.join(name)).is_some_and(|digest| stamp["outputs"][i].as_str() == Some(digest.as_str()))
        })
        && [
            ("overlay", "bake-inputs/overlay.bin"),
            ("collision", "collision.glb"),
            ("profile", "probe-bake.json"),
            ("boundaries", "water-boundaries.json"),
        ]
        .iter()
        .all(|(key, name)| stamp["inputs"][key].as_str() == sha256(&level.join(name)).as_deref())
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
                && s["profile_sha256"].as_str() == sha256(&entry.path().join("probe-bake.json")).as_deref()
                && has_baker
                && verified_bake(&baked)
                && prepared_water(&entry.path(), &manifest)
                && profile(&entry.path()).is_some_and(|p| made_by(s, &p))
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
        for text in [r#"{"verified":true}"#, r#"{"verified":false}"#, r#"{"verified":true,"regions":["missing.orpg"]}"#]
        {
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
    fn previous_bake_stamps_require_a_rebake() {
        let profile = "current profile";
        for version in [1, 2, 3, 4] {
            let stamp = serde_json::json!({"version":version,
                "baker_profile_sha256":format!("{:x}",Sha256::digest(profile.as_bytes()))});
            assert!(!made_by(&stamp, profile));
        }
    }

    #[test]
    fn native_water_stamps_check_profiles_sources_and_both_outputs() {
        let root = temp("native-water");
        std::fs::create_dir_all(root.join("bake-inputs")).unwrap();
        let manifest = root.join("manifest.json");
        std::fs::write(&manifest, "{}").unwrap();
        assert!(prepared_water(&root, &manifest));
        for name in ["bake-inputs/overlay.bin", "collision.glb", "water.bin", "water.mesh"] {
            std::fs::write(root.join(name), name).unwrap();
        }
        std::fs::write(root.join("probe-bake.json"), r#"{"water_sources":{"version":1}}"#).unwrap();
        assert!(!prepared_water(&root, &manifest));
        let stamp = serde_json::json!({"version": 3, "inputs": {
            "algorithm":"native-water-sources-v1",
            "overlay":sha256(&root.join("bake-inputs/overlay.bin")),
            "collision":sha256(&root.join("collision.glb")),
            "profile":sha256(&root.join("probe-bake.json")),"boundaries":null},
            "outputs":[sha256(&root.join("water.bin")),sha256(&root.join("water.mesh"))]});
        std::fs::write(root.join("water-bake.json"), stamp.to_string()).unwrap();
        assert!(prepared_water(&root, &manifest));
        for version in [1, 2] {
            let mut old = stamp.clone();
            old["version"] = serde_json::json!(version);
            std::fs::write(root.join("water-bake.json"), old.to_string()).unwrap();
            assert!(!prepared_water(&root, &manifest));
        }
        std::fs::write(root.join("water-bake.json"), stamp.to_string()).unwrap();
        std::fs::write(root.join("probe-bake.json"), r#"{"water_sources":{"version":2}}"#).unwrap();
        assert!(!prepared_water(&root, &manifest));
        let _ = std::fs::remove_dir_all(root);
    }

    #[test]
    fn native_cache_status_tracks_manifest_profile_and_verified_files() {
        let root = temp("native-stamp");
        let levels = root.join("levels");
        let level = levels.join("level_00");
        let probes = root.join("probes");
        let baked = probes.join("level_00");
        let baker = root.join("baker");
        std::fs::create_dir_all(&level).unwrap();
        std::fs::create_dir_all(&baked).unwrap();
        std::fs::write(&baker, "baker").unwrap();
        std::fs::write(level.join("manifest.json"), "{}").unwrap();
        std::fs::write(baked.join("sparse.orpg"), "probes").unwrap();
        std::fs::write(baked.join("bake-result.json"), r#"{"verified":true,"regions":[]}"#).unwrap();
        let stamp = serde_json::json!({"version":5,
            "manifest_sha256": sha256(&level.join("manifest.json")), "profile_sha256":null,
            "baker_profile_sha256":format!("{:x}",Sha256::digest(b"profile"))});
        std::fs::write(baked.join(STAMP), stamp.to_string()).unwrap();
        let status = || bake_status_on(&levels, &probes, &baker, true, &|_| Some("profile".to_owned()));
        assert_eq!(status().ready, 1);
        std::fs::write(level.join("probe-bake.json"), "{}").unwrap();
        assert_eq!(status().ready, 0);
        let _ = std::fs::remove_dir_all(root);
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
            std::fs::write(dir.join("bake-result.json"), r#"{"verified":true,"regions":[]}"#).unwrap();
            let text = serde_json::json!({
                "version": 5,
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
}
