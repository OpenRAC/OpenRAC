// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//! Lighting and water preparation status, logs and cache management.

use super::{assemble_bake_mods, BAKE_ACTION, NO_BAKER_HERE, SHIPS_BAKER};
use serde::Serialize;
use serde_json::Value;
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
    Some(crate::sha256_hex(&bytes))
}

/// What `baker` makes of `level` (`--print-profile`):
/// its digest names the baker in a level's stamp, the same for every build of the same baker.
/// None when it cannot say. Asked once for each build of the baker and each level's files (a few
/// tens of milliseconds a level).
fn baker_profile(baker: &Path, level: &Path, overrides: Option<&Path>) -> Option<String> {
    let mut command = crate::detect::quiet(baker);
    command.arg("--level-dir").arg(level).arg("--print-profile");
    if let Some(overrides) = overrides {
        command.arg("--level-overrides").arg(overrides);
        return printed(command);
    }
    use std::collections::HashMap;
    use std::sync::Mutex;
    type Key = (PathBuf, PathBuf, String);
    static CACHE: Mutex<Option<HashMap<Key, Option<String>>>> = Mutex::new(None);
    let file = |p: &Path| std::fs::metadata(p).ok().map(|m| (m.len(), m.modified().ok()));
    let key = (
        baker.to_path_buf(),
        level.to_path_buf(),
        format!(
            "{:?} {:?} {:?} {}",
            file(baker),
            file(&level.join("probe-bake.json")),
            file(&level.join("manifest.json")),
            level.join("bake-inputs/gameplay.bin").is_file()
        ),
    );
    if let Some(known) = CACHE.lock().unwrap().get_or_insert_with(HashMap::new).get(&key) {
        return known.clone();
    }
    let said = printed(command);
    CACHE.lock().unwrap().get_or_insert_with(HashMap::new).insert(key, said.clone());
    said
}

fn printed(mut command: std::process::Command) -> Option<String> {
    let output = command.output().ok()?;
    output.status.success().then(|| String::from_utf8_lossy(&output.stdout).trim().to_string())
}

/// Current native bake stamps must match the resolved transport profile exactly.
fn made_by(stamp: &Value, profile: &str) -> bool {
    let digest = crate::sha256_hex(profile.as_bytes());
    let storage = serde_json::from_str::<Value>(profile).ok().and_then(|p| p.get("asset_storage").cloned());
    stamp["version"] == 5
        && stamp.get("asset_storage") == storage.as_ref()
        && stamp["baker_profile_sha256"].as_str() == Some(digest.as_str())
}

fn verified_bake(baked: &Path, reflections: bool) -> bool {
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
    let environment_ready = result.get("environment").is_none()
        && (!reflections || result["environment_unavailable"] == Value::Bool(true))
        || result["environment"].as_str() == Some("environment.orpe")
            && std::fs::metadata(baked.join("environment.orpe")).is_ok_and(|m| m.is_file() && m.len() > 0);
    environment_ready
        && present("sparse.orpg")
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

/// An opted-in ocean needs a current native response even when its lighting can be reused.
fn prepared_ocean(level: &Path) -> bool {
    let profile = level.join("ocean-profile.json");
    if !profile.exists() {
        return true;
    }
    let read =
        |path: &Path| std::fs::read_to_string(path).ok().and_then(|text| serde_json::from_str::<Value>(&text).ok());
    let Some(profile) = read(&profile) else {
        return false;
    };
    if profile["coast"].is_null() {
        return profile.is_object();
    }
    let Some(stamp) = read(&level.join("ocean-bake.json")) else {
        return false;
    };
    stamp["version"] == 1
        && stamp["inputs"]["coast"] == profile["coast"]
        && sha256(&level.join("ocean.wave")).is_some_and(|digest| stamp["output"].as_str() == Some(digest.as_str()))
        && [
            ("manifest", "manifest.json"),
            ("placements", "placements.json"),
            ("collision", "collision.glb"),
            ("gameplay", "bake-inputs/gameplay.bin"),
        ]
        .iter()
        .all(|(key, name)| {
            sha256(&level.join(name)).is_some_and(|digest| stamp["inputs"][key].as_str() == Some(digest.as_str()))
        })
}

/// The bake status of the levels in `levels` against the bakes in `probes`, made by `baker`.
pub fn bake_status(levels: &Path, probes: &Path, baker: &Path, profiles: &Path, version: &str) -> BakeStatus {
    bake_status_with_mods(levels, probes, baker, profiles, version, None)
}

fn bake_status_with_mods(
    levels: &Path,
    probes: &Path,
    baker: &Path,
    profiles: &Path,
    version: &str,
    mods: Option<&Path>,
) -> BakeStatus {
    let game = version.split_once('/').map(|(game, _)| game).unwrap_or(version);
    let version = version.replace('/', "-");
    bake_status_selected(levels, probes, baker, SHIPS_BAKER, mods, &|level, overrides, stamp| {
        if lighting_profile_needs_refresh(profiles, &version, level)
            || !prepared_emission(level, baker, profiles, game, &version, stamp)
            || !matches_modded_source(baker, level, overrides, stamp)
        {
            return None;
        }
        baker_profile(baker, level, overrides)
    })
}

fn matches_modded_source(baker: &Path, level: &Path, overrides: Option<&Path>, stamp: &Value) -> bool {
    let Some(overrides) = overrides else {
        return stamp["modded"] != Value::Bool(true);
    };
    if stamp["modded"] != Value::Bool(true) {
        return false;
    }
    let mut command = crate::detect::quiet(baker);
    command.arg("--print-modded-source").arg(level).arg(overrides);
    printed(command).is_some_and(|source| stamp["source"].as_str() == Some(source.as_str()))
}

// Detect added or removed recipes directly. Only an existing recipe needs a baker query.
fn prepared_emission(level: &Path, baker: &Path, profiles: &Path, game: &str, version: &str, stamp: &Value) -> bool {
    let previous = stamp["emission_textures_sha256"].as_str();
    let Some(name) = level.file_name() else { return false };
    let version = profiles.join(version);
    let has_recipe = [profiles.join(game), version.join(name), version]
        .iter()
        .any(|path| path.join("emission-textures.json").is_file());
    if !has_recipe || previous.is_none() {
        return !has_recipe && previous.is_none();
    }
    crate::detect::quiet(baker).arg("--print-emission-fingerprint").arg(level).arg(profiles).output().ok().is_some_and(
        |output| output.status.success() && previous == Some(String::from_utf8_lossy(&output.stdout).trim()),
    )
}

// Predict the baker's tracked profile refresh without touching the export.
fn lighting_profile_needs_refresh(profiles: &Path, version: &str, level: &Path) -> bool {
    let Some(name) = level.file_name() else { return false };
    let Some(shipped) = sha256(&profiles.join(version).join(name).join("lighting-environments.json")) else {
        return false;
    };
    let local = level.join("lighting-environments.json");
    let ownership = level.join(".lighting-environments.sha256");
    if !local.exists() {
        return !ownership.is_file();
    }
    let installed = sha256(&local);
    let recorded = std::fs::read_to_string(ownership).ok();
    installed.as_ref().is_some_and(|digest| digest != &shipped && recorded.as_ref() == Some(digest))
}

/// [`bake_status`] on a computer that gets the baker or not (`ships_baker`, SHIPS_BAKER), with the
/// baker's profile of a level from `profile`. Without it and without a baker, NO_BAKER_HERE is the
/// reason whatever the levels: preparing them would not make a bake possible.
#[cfg(test)]
fn bake_status_on(
    levels: &Path,
    probes: &Path,
    baker: &Path,
    ships_baker: bool,
    profile: &dyn Fn(&Path, &Value) -> Option<String>,
) -> BakeStatus {
    bake_status_selected(levels, probes, baker, ships_baker, None, &|level, _, stamp| profile(level, stamp))
}

type ReadBakeProfile<'a> = dyn Fn(&Path, Option<&Path>, &Value) -> Option<String> + 'a;

fn bake_status_selected(
    levels: &Path,
    probes: &Path,
    baker: &Path,
    ships_baker: bool,
    mods: Option<&Path>,
    profile: &ReadBakeProfile<'_>,
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
    let numbered = |name: &str| {
        name.strip_prefix("level_").is_some_and(|n| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()))
    };
    let mut targets = Vec::new();
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().into_owned();
        if numbered(&name) && entry.path().join("manifest.json").is_file() {
            targets.push((name.clone(), name, mods.map(Path::to_path_buf)));
        }
    }
    let registry = mods
        .and_then(|root| std::fs::read_to_string(root.join("destinations.json")).ok())
        .and_then(|text| serde_json::from_str::<Value>(&text).ok());
    let destinations = registry.as_ref().and_then(|registry| registry["destinations"].as_array());
    if let Some((root, destinations)) = mods.zip(destinations) {
        for destination in destinations {
            let (Some(id), Some(level)) = (destination["id"].as_str(), destination["level"].as_u64()) else {
                continue;
            };
            if id.is_empty() || !id.bytes().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || c == b'-') {
                continue;
            }
            let name = format!("level_{level:02}");
            targets.push((format!("{id}/{name}"), name, Some(root.join("destinations").join(id))));
        }
    }
    for (label, name, mount) in targets {
        out.levels += 1;
        let level = levels.join(&name);
        let manifest = level.join("manifest.json");
        let overrides = mount.as_ref().map(|root| root.join(&name)).filter(|path| path.is_dir());
        let cache = if let Some(overrides) = &overrides {
            let mut command = crate::detect::quiet(baker);
            command.arg("--print-mod-key").arg(overrides);
            let Some(key) = printed(command).filter(|key| key.parse::<u64>().is_ok()) else {
                continue;
            };
            probes.join("variants").join(key)
        } else if label != name {
            // A registered destination cannot fall back to the campaign's bake.
            continue;
        } else {
            probes.to_path_buf()
        };
        let baked = cache.join(&name);
        let stamp: Option<Value> =
            std::fs::read_to_string(baked.join(STAMP)).ok().and_then(|t| serde_json::from_str(&t).ok());
        let current = stamp.as_ref().is_some_and(|s| {
            s["manifest_sha256"].as_str() == sha256(&manifest).as_deref()
                && s["profile_sha256"].as_str() == sha256(&level.join("probe-bake.json")).as_deref()
                && s["lighting_environments_sha256"].as_str()
                    == sha256(&level.join("lighting-environments.json")).as_deref()
                && has_baker
                && verified_bake(&baked, s["reflections"] == Value::Bool(true))
                && prepared_water(&level, &manifest)
                && prepared_ocean(&level)
                && profile(&level, overrides.as_deref(), s).is_some_and(|p| made_by(s, &p))
        });
        if current {
            out.ready += 1;
        } else if cache.join(format!(".{name}.failed")).is_dir()
            || failures.as_ref().and_then(|r| r["levels"].as_array()).is_some_and(|rows| {
                rows.iter().any(|row| row["level"].as_str() == Some(label.as_str()) && row["error"].is_string())
            })
        {
            out.failed += 1;
        } else if cache.join(format!(".{name}.partial")).is_dir() {
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
    let status = || {
        let (levels, probes, baker) = bake_folders(config, key)?;
        let profiles = config.root()?.join("port/enhanced/lighting/profiles");
        let snapshot = if baker.is_file() && levels.is_dir() { assemble_bake_mods(config, key, false)? } else { None };
        let mods = snapshot.as_ref().map(|snapshot| snapshot.path());
        Ok(bake_status_with_mods(&levels, &probes, &baker, &profiles, key, mods.as_deref()))
    };
    status().unwrap_or_else(|why| BakeStatus { why, ..BakeStatus::default() })
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
        if name == "variants" && valid_variant_cache(&entry.path()) {
            continue;
        }
        if !is_bake_level(&name)
            && !name.starts_with("bake-all-result.json")
            && name != "probe-bake.log"
            && name != "probe-bake.lock"
        {
            return Err(format!("{} holds {name}, which is not a bake: nothing was deleted", probes.display()));
        }
    }
    std::fs::remove_dir_all(probes).map_err(|e| format!("could not delete {}: {e}", probes.display()))
}

fn is_bake_level(name: &str) -> bool {
    let level = name.trim_start_matches('.');
    let level = level
        .strip_suffix(".partial")
        .or_else(|| level.strip_suffix(".failed"))
        .or_else(|| level.strip_suffix(".previous"))
        .unwrap_or(level);
    level.strip_prefix("level_").is_some_and(|n| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()))
}

fn valid_variant_cache(path: &Path) -> bool {
    path.symlink_metadata().is_ok_and(|info| info.file_type().is_dir())
        && std::fs::read_dir(path).is_ok_and(|entries| {
            entries.into_iter().all(|entry| {
                entry.is_ok_and(|entry| {
                    entry.file_type().is_ok_and(|ty| ty.is_dir())
                        && entry.file_name().to_str().is_some_and(|name| name.parse::<u64>().is_ok())
                        && std::fs::read_dir(entry.path()).is_ok_and(|levels| {
                            levels.into_iter().all(|level| {
                                level.is_ok_and(|level| level.file_name().to_str().is_some_and(is_bake_level))
                            })
                        })
                })
            })
        })
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
            assert!(!verified_bake(root, false));
        }
        std::fs::write(root.join("bake-result.json"), r#"{"verified":true,"regions":[]}"#).unwrap();
        assert!(verified_bake(root, false));
        std::fs::write(root.join("sparse.orpg"), "").unwrap();
        assert!(!verified_bake(root, false));
    }

    #[test]
    fn added_or_removed_emission_recipes_make_a_bake_stale() {
        let temp = tempfile::tempdir().unwrap();
        let profiles = temp.path();
        let level = profiles.join("level_02");
        let shipped = profiles.join("rac1-ntsc/level_02");
        std::fs::create_dir_all(&shipped).unwrap();
        let recipe = shipped.join("emission-textures.json");
        let ready =
            |stamp| prepared_emission(&level, &profiles.join("unused-baker"), profiles, "rac1", "rac1-ntsc", stamp);
        assert!(ready(&Value::Null));
        std::fs::write(&recipe, "{}").unwrap();
        assert!(!ready(&Value::Null));
        std::fs::remove_file(recipe).unwrap();
        assert!(!ready(&serde_json::json!({"emission_textures_sha256": "previous"})));
    }

    #[test]
    fn missing_or_empty_reflections_are_not_ready() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path();
        std::fs::write(root.join("sparse.orpg"), "probe bytes").unwrap();
        std::fs::write(root.join("bake-result.json"), r#"{"verified":true,"regions":[]}"#).unwrap();
        assert!(verified_bake(root, false));
        assert!(!verified_bake(root, true));
        std::fs::write(
            root.join("bake-result.json"),
            r#"{"verified":true,"regions":[],"environment_unavailable":true}"#,
        )
        .unwrap();
        assert!(verified_bake(root, true));
        std::fs::write(
            root.join("bake-result.json"),
            r#"{"verified":true,"regions":[],"environment":"environment.orpe"}"#,
        )
        .unwrap();
        assert!(!verified_bake(root, false));
        assert!(!verified_bake(root, true));
        std::fs::write(root.join("environment.orpe"), "reflections").unwrap();
        assert!(verified_bake(root, true));
        std::fs::write(root.join("environment.orpe"), "").unwrap();
        assert!(!verified_bake(root, true));
        std::fs::write(
            root.join("bake-result.json"),
            r#"{"verified":true,"regions":[],"environment":"../other.orpe"}"#,
        )
        .unwrap();
        assert!(!verified_bake(root, false));
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
                "baker_profile_sha256":crate::sha256_hex(profile.as_bytes())});
            assert!(!made_by(&stamp, profile));
        }
    }

    #[test]
    fn native_ocean_reuses_transport_after_material_edits() {
        let root = temp("native-ocean");
        std::fs::create_dir_all(root.join("bake-inputs")).unwrap();
        let profile = root.join("ocean-profile.json");
        assert!(prepared_ocean(&root));
        let coast = serde_json::json!({"origin": [608, 481], "offshore_depth": 12});
        std::fs::write(&profile, serde_json::json!({"coast": coast}).to_string()).unwrap();
        assert!(!prepared_ocean(&root));
        let mut inputs = serde_json::json!({"coast": coast});
        for (key, name) in [
            ("manifest", "manifest.json"),
            ("placements", "placements.json"),
            ("collision", "collision.glb"),
            ("gameplay", "bake-inputs/gameplay.bin"),
        ] {
            std::fs::write(root.join(name), name).unwrap();
            inputs[key] = sha256(&root.join(name)).unwrap().into();
        }
        let wave = root.join("ocean.wave");
        std::fs::write(&wave, "baked response").unwrap();
        let stamp = serde_json::json!({"version": 1, "inputs": inputs, "output": sha256(&wave)});
        std::fs::write(root.join("ocean-bake.json"), stamp.to_string()).unwrap();
        assert!(prepared_ocean(&root));
        std::fs::write(&profile, serde_json::json!({"coast": coast, "material": {"roughness": 0.14}}).to_string())
            .unwrap();
        assert!(prepared_ocean(&root));
        std::fs::write(root.join("collision.glb"), "changed collision").unwrap();
        assert!(!prepared_ocean(&root));
        std::fs::write(root.join("collision.glb"), "collision.glb").unwrap();
        std::fs::remove_file(&wave).unwrap();
        assert!(!prepared_ocean(&root));
        std::fs::remove_dir_all(root).unwrap();
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
            "baker_profile_sha256":crate::sha256_hex(b"profile")});
        std::fs::write(baked.join(STAMP), stamp.to_string()).unwrap();
        let status = || bake_status_on(&levels, &probes, &baker, true, &|_, _| Some("profile".to_owned()));
        assert_eq!(status().ready, 1);
        let mut wrapped_stamp = stamp.clone();
        wrapped_stamp["asset_storage"] = serde_json::json!("lz4-v1");
        std::fs::write(baked.join(STAMP), wrapped_stamp.to_string()).unwrap();
        assert_eq!(status().ready, 0);
        let compressed_profile = r#"{"asset_storage":"lz4-v1"}"#;
        wrapped_stamp["baker_profile_sha256"] = serde_json::json!(crate::sha256_hex(compressed_profile.as_bytes()));
        std::fs::write(baked.join(STAMP), wrapped_stamp.to_string()).unwrap();
        let compressed_status =
            || bake_status_on(&levels, &probes, &baker, true, &|_, _| Some(compressed_profile.to_owned()));
        assert_eq!(compressed_status().ready, 1);
        wrapped_stamp.as_object_mut().unwrap().remove("asset_storage");
        std::fs::write(baked.join(STAMP), wrapped_stamp.to_string()).unwrap();
        assert_eq!(compressed_status().ready, 0);
        std::fs::write(baked.join(STAMP), stamp.to_string()).unwrap();
        let authoring = level.join("lighting-environments.json");
        std::fs::write(&authoring, "{}").unwrap();
        assert_eq!(status().ready, 0);
        let mut authored_stamp = stamp.clone();
        authored_stamp["lighting_environments_sha256"] = serde_json::json!(sha256(&authoring));
        std::fs::write(baked.join(STAMP), authored_stamp.to_string()).unwrap();
        assert_eq!(status().ready, 1);
        std::fs::write(&authoring, "{ }").unwrap();
        assert_eq!(status().ready, 0);
        std::fs::remove_file(&authoring).unwrap();
        assert_eq!(status().ready, 0);
        std::fs::write(baked.join(STAMP), stamp.to_string()).unwrap();
        std::fs::write(level.join("probe-bake.json"), "{}").unwrap();
        assert_eq!(status().ready, 0);
        let _ = std::fs::remove_dir_all(root);
    }

    #[test]
    fn a_ship_destination_counts_separately_from_its_campaign_level() {
        let root = tempfile::tempdir().unwrap();
        let levels = root.path().join("levels");
        let level = levels.join("level_01");
        let probes = root.path().join("probes");
        let baked = probes.join("level_01");
        let mods = root.path().join("mods");
        let baker = root.path().join("baker");
        std::fs::create_dir_all(&level).unwrap();
        std::fs::create_dir_all(&baked).unwrap();
        std::fs::create_dir_all(mods.join("destinations/test-ship/level_01")).unwrap();
        std::fs::write(&baker, "baker").unwrap();
        std::fs::write(level.join("manifest.json"), "{}").unwrap();
        std::fs::write(baked.join("bake-result.json"), r#"{"verified":true,"empty":true}"#).unwrap();
        let stamp = serde_json::json!({
            "version": 5,
            "manifest_sha256": sha256(&level.join("manifest.json")),
            "baker_profile_sha256": crate::sha256_hex(b"profile"),
        });
        std::fs::write(baked.join(STAMP), stamp.to_string()).unwrap();
        std::fs::write(mods.join("destinations.json"), r#"{"destinations":[{"id":"test-ship","level":1}]}"#).unwrap();
        let status =
            bake_status_selected(&levels, &probes, &baker, true, Some(&mods), &|_, _, _| Some("profile".to_owned()));
        assert_eq!((status.levels, status.ready), (2, 1));
    }

    #[test]
    fn lighting_refresh_preserves_edits_and_local_removal() {
        let root = temp("lighting-refresh");
        let level = root.join("level_06");
        let shipped = root.join("rac1-ntsc").join("level_06");
        std::fs::create_dir_all(&level).unwrap();
        std::fs::create_dir_all(&shipped).unwrap();
        let name = "lighting-environments.json";
        let ownership = level.join(".lighting-environments.sha256");
        std::fs::write(shipped.join(name), "first profile").unwrap();
        let pending = || lighting_profile_needs_refresh(&root, "rac1-ntsc", &level);
        assert!(pending());
        std::fs::copy(shipped.join(name), level.join(name)).unwrap();
        std::fs::write(&ownership, sha256(&level.join(name)).unwrap()).unwrap();
        assert!(!pending());
        std::fs::write(shipped.join(name), "updated profile").unwrap();
        assert!(pending());
        std::fs::write(level.join(name), "local edit").unwrap();
        assert!(!pending());
        std::fs::remove_file(level.join(name)).unwrap();
        assert!(!pending());
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
        let profile = |_: &Path, _: &Value| std::fs::read_to_string(root.join("baker")).ok();
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
                "manifest_sha256": crate::sha256_hex(manifest.as_bytes()),
                "baker_profile_sha256": crate::sha256_hex(b"baker"),
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
        let same = |_: &Path, _: &Value| Some("baker".to_string());
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
        let variants = probes.join("variants/123");
        std::fs::create_dir_all(variants.join("level_00")).unwrap();
        std::fs::write(variants.join("notes.txt"), "keep me").unwrap();
        assert!(delete_bake_folder(&probes).is_err());
        std::fs::remove_file(variants.join("notes.txt")).unwrap();
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
            let here = bake_status_on(&levels, &probes, &baker, false, &|_, _| None);
            assert!(!here.available && here.no_baker_here, "{here:?}");
            assert_eq!(here.why, NO_BAKER_HERE);
            assert_eq!(here.levels, usize::from(ready));
            // Elsewhere the existing reasons are kept.
            let elsewhere = bake_status_on(&levels, &probes, &baker, true, &|_, _| None);
            assert!(!elsewhere.available && !elsewhere.no_baker_here);
            let why = if ready { "this build of the game has no probe baker" } else { "the levels are not ready yet" };
            assert_eq!(elsewhere.why, why);
        }
        // A baker that is there anyway (built by hand) is used.
        std::fs::write(&baker, "baker").unwrap();
        let built = bake_status_on(&levels, &probes, &baker, false, &|_, _| None);
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
