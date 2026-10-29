//! Read-only view of existing packed-edit and prepared-stack records for RAC1.
use std::collections::BTreeSet;
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};

use serde::Serialize;
use serde_json::Value;

use crate::overrides;

#[derive(Debug, Clone, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SummaryPack {
    pub id: String,
    pub name: String,
    pub applied_files: usize,
}

#[derive(Debug, Clone, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModLoadSummary {
    pub version: String,
    pub packed_version: Option<String>,
    pub packed_changes: Vec<String>,
    pub changes_truncated: bool,
    pub prepared_at: Option<u64>,
    pub prepared_version: Option<String>,
    pub stale: bool,
    pub packs: Vec<SummaryPack>,
    pub packed_files_applied: usize,
    pub gameplay_levels: Vec<String>,
    pub scenery_levels: Vec<String>,
    pub terrain_levels: Vec<String>,
    pub collision_levels: Vec<String>,
    pub notes: Vec<String>,
}

fn text(path: &Path) -> Result<Option<String>, String> {
    if !path.exists() {
        return Ok(None);
    }
    if std::fs::metadata(path).map_err(|e| e.to_string())?.len() > 4 * 1024 * 1024 {
        return Err("The mod summary record is too large to display".into());
    }
    std::fs::read_to_string(path).map(Some).map_err(|e| e.to_string())
}

fn json(path: &Path) -> Result<Option<Value>, String> {
    text(path)?.map(|t| serde_json::from_str(&t).map_err(|e| format!("Cannot read mod summary: {e}"))).transpose()
}

fn modified(path: &Path) -> Option<SystemTime> {
    std::fs::metadata(path).ok()?.modified().ok()
}

/// All paths come from the selected RAC1 data directory and an allowed version,
/// never from a filename supplied by a pack manifest.
pub fn read(data: &Path, packed: &Path, version: &str) -> Result<ModLoadSummary, String> {
    if !matches!(version, "pal" | "ntsc") {
        return Err("Choose a supported RAC1 disc version".into());
    }
    let stack_path = data.join("mods").join(version).join("stack.json");
    let note = json(&packed.join("packed.json"))?;
    let stack = json(&stack_path)?;
    let timestamp = modified(&stack_path);
    let mut result = ModLoadSummary {
        version: version.into(),
        packed_version: note.as_ref().and_then(|v| v["version"].as_str()).map(str::to_owned),
        packed_changes: Vec::new(),
        changes_truncated: false,
        prepared_at: timestamp.and_then(|t| t.duration_since(UNIX_EPOCH).ok()).map(|d| d.as_secs()),
        prepared_version: stack.as_ref().and_then(|v| v["version"].as_str()).map(str::to_owned),
        stale: stack.is_none(),
        packs: Vec::new(),
        packed_files_applied: 0,
        gameplay_levels: Vec::new(),
        scenery_levels: Vec::new(),
        terrain_levels: Vec::new(),
        collision_levels: Vec::new(),
        notes: Vec::new(),
    };
    if result.packed_version.as_deref().is_some_and(|v| v != version) {
        result.notes.push(
            "The latest packed edits belong to another disc edition; check the packing report before playing.".into(),
        );
    }
    if let Some(changes) = text(&packed.join("changes.txt"))? {
        let lines: Vec<_> = changes.lines().filter(|s| !s.trim().is_empty()).collect();
        result.changes_truncated = lines.len() > 200;
        result.packed_changes = lines.into_iter().take(200).map(str::to_owned).collect();
    }
    let selected: Vec<_> = overrides::list(&data.join("overrides"))?.into_iter().filter(|p| p.enabled).collect();
    let selected_ids: Vec<_> = selected.iter().map(|p| p.id.as_str()).collect();
    if let Some(stack) = &stack {
        let layers = stack["layers"].as_array().ok_or("Prepared mod summary has no layers")?;
        if layers.iter().any(|v| v["id"].as_str().is_none()) {
            return Err("Prepared mod summary has an invalid layer".into());
        }
        let ids: Vec<_> = layers.iter().filter_map(|v| v["id"].as_str()).filter(|id| *id != "packed").collect();
        result.stale = ids != selected_ids || result.prepared_version.as_deref() != Some(version);
        let files = stack["files"].as_object().ok_or("Prepared mod summary has no file list")?;
        if files.values().any(|v| v.as_str().is_none()) {
            return Err("Prepared mod summary has an invalid file owner".into());
        }
        let (mut gameplay, mut scenery, mut terrain, mut collision) =
            (BTreeSet::new(), BTreeSet::new(), BTreeSet::new(), BTreeSet::new());
        for (file, owner) in files {
            if owner.as_str() == Some("packed") {
                result.packed_files_applied += 1;
            }
            let Some((level, name)) = file.split_once('/') else { continue };
            if level.len() != 8 || !level.starts_with("level_") || !level[6..].bytes().all(|b| b.is_ascii_digit()) {
                continue;
            }
            if name.starts_with("gameplay_") && name.ends_with(".wad") {
                gameplay.insert(level.to_owned());
            }
            if name == "placements.json" {
                scenery.insert(level.to_owned());
            }
            if name == "terrain.glb" {
                terrain.insert(level.to_owned());
            }
            if name.starts_with("core_") && name.ends_with(".bin") {
                collision.insert(level.to_owned());
            }
        }
        result.gameplay_levels = gameplay.into_iter().collect();
        result.scenery_levels = scenery.into_iter().collect();
        result.terrain_levels = terrain.into_iter().collect();
        result.collision_levels = collision.into_iter().collect();
        if let Some(notes) = stack["notes"].as_array() {
            result.notes.extend(notes.iter().filter_map(Value::as_str).map(str::to_owned));
        }
        if let Some(skipped) = stack["skipped"].as_object() {
            for (name, why) in skipped {
                result.notes.push(format!("{name}: {why}"));
            }
        }
        for pack in &selected {
            let applied_files = if pack.is_destination {
                let child =
                    json(&data.join("mods").join(version).join("destinations").join(&pack.id).join("stack.json"))?
                        .ok_or("Prepared destination is missing; prepare the mods again")?;
                child["files"]
                    .as_object()
                    .ok_or("Prepared destination has no file list")?
                    .values()
                    .filter(|v| v.as_str() == Some(&pack.id))
                    .count()
            } else {
                files.values().filter(|v| v.as_str() == Some(&pack.id)).count()
            };
            result.packs.push(SummaryPack { id: pack.id.clone(), name: pack.name.clone(), applied_files });
        }
    } else {
        result.packs =
            selected.iter().map(|p| SummaryPack { id: p.id.clone(), name: p.name.clone(), applied_files: 0 }).collect();
    }
    if let Some(at) = timestamp {
        let mut inputs =
            vec![packed.join("packed.json"), packed.join("changes.txt"), data.join("overrides/overrides.json")];
        inputs.extend(selected.iter().map(|p| data.join("overrides/packs").join(&p.id).join(overrides::MANIFEST)));
        result.stale |= inputs.iter().any(|p| modified(p).is_some_and(|t| t > at));
    }
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn write(root: &Path, path: &str, value: Value) {
        let p = root.join(path);
        std::fs::create_dir_all(p.parent().unwrap()).unwrap();
        std::fs::write(p, value.to_string()).unwrap();
    }
    #[test]
    fn destination_counts_are_read_without_claiming_the_campaign_was_replaced() {
        let d = tempfile::tempdir().unwrap();
        let p = d.path();
        write(p, "overrides/packs/harbor/openrac-mod.json", serde_json::json!({"name":"Harbor","format":4}));
        write(p, "overrides/overrides.json", serde_json::json!({"format":1,"packs":[{"id":"harbor","enabled":true}]}));
        write(
            p,
            "mods/pal/destinations/harbor/stack.json",
            serde_json::json!({"files":{"level_02/core_pal.bin":"harbor"}}),
        );
        write(
            p,
            "mods/pal/stack.json",
            serde_json::json!({"version":"pal","layers":[{"id":"harbor"}],"files":{},"notes":["1 added ship destination prepared separately from the campaign"]}),
        );
        let s = read(p, &p.join("packed/pal"), "pal").unwrap();
        assert!(!s.stale);
        assert_eq!(s.packs[0].applied_files, 1);
        assert!(s.gameplay_levels.is_empty() && s.collision_levels.is_empty());
    }
    #[test]
    fn missing_records_are_not_reported_as_a_successful_empty_load() {
        let d = tempfile::tempdir().unwrap();
        let s = read(d.path(), &d.path().join("packed/pal"), "pal").unwrap();
        assert!(s.stale && s.prepared_at.is_none());
        assert!(s.packs.is_empty());
        assert!(read(d.path(), d.path(), "../pal").is_err());
    }
    #[test]
    fn winning_files_and_region_are_read_from_the_prepared_stack() {
        let d = tempfile::tempdir().unwrap();
        let p = d.path();
        write(p, "overrides/packs/map/openrac-mod.json", serde_json::json!({"name":"My map"}));
        write(p, "overrides/overrides.json", serde_json::json!({"format":1,"packs":[{"id":"map","enabled":true}]}));
        write(
            p,
            "mods/pal/stack.json",
            serde_json::json!({"version":"pal","layers":[{"id":"packed"},{"id":"map"}],"files":{"level_00/terrain.glb":"map","level_00/gameplay_pal.wad":"packed","level_00/core_pal.bin":"map"},"notes":["One replacement wins"],"skipped":{}}),
        );
        let s = read(p, &p.join("packed/pal"), "pal").unwrap();
        assert!(!s.stale);
        assert_eq!(s.packs[0].applied_files, 2);
        assert_eq!(s.packed_files_applied, 1);
        assert_eq!(s.gameplay_levels, vec!["level_00"]);
        assert_eq!(s.terrain_levels, vec!["level_00"]);
        assert_eq!(s.collision_levels, vec!["level_00"]);
        assert_eq!(s.notes, vec!["One replacement wins"]);
        write(p, "overrides/overrides.json", serde_json::json!({"format":1,"packs":[{"id":"map","enabled":false}]}));
        assert!(read(p, &p.join("packed/pal"), "pal").unwrap().stale);
        assert!(read(p, &p.join("packed/ntsc"), "ntsc").unwrap().prepared_at.is_none());
    }
    #[test]
    fn malformed_records_fail_instead_of_inventing_zero_changes() {
        let d = tempfile::tempdir().unwrap();
        write(d.path(), "mods/pal/stack.json", serde_json::json!({"version":"pal"}));
        assert!(read(d.path(), d.path(), "pal").is_err());
        write(d.path(), "mods/pal/stack.json", serde_json::json!({"version":"pal","layers":[{}],"files":{}}));
        assert!(read(d.path(), d.path(), "pal").is_err());
        write(
            d.path(),
            "mods/pal/stack.json",
            serde_json::json!({"version":"pal","layers":[],"files":{"level_00/terrain.glb":null}}),
        );
        assert!(read(d.path(), d.path(), "pal").is_err());
    }

    #[test]
    fn a_checkout_pack_from_another_edition_is_reported_explicitly() {
        let d = tempfile::tempdir().unwrap();
        let data = d.path().join("data");
        let packed = d.path().join("checkout-packed");
        write(&packed, "packed.json", serde_json::json!({"game":"rac1","version":"ntsc"}));
        write(&data, "mods/pal/stack.json", serde_json::json!({"version":"pal","layers":[],"files":{}}));
        let s = read(&data, &packed, "pal").unwrap();
        assert_eq!(s.packed_version.as_deref(), Some("ntsc"));
        assert!(s.notes.iter().any(|n| n.contains("another disc edition")));
        assert_eq!(s.packed_files_applied, 0);
    }
}
