//! The way back from a mod code (modset.rs): before a set is applied, what the mods looked like is
//! kept in `<plugins>/profiles`, as `<id>.json` (the list, the sources, every installed folder's
//! id and version) with a folder `<id>/` holding a copy of each mod the set replaces. Restoring puts
//! those folders and both files back. The last [`KEEP`] are kept.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::plugins::{self, LIST};
use crate::remote::SOURCES;

/// Where they are, in the plugins folder.
pub const PROFILES: &str = "profiles";
/// How many are kept.
pub const KEEP: usize = 10;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
struct Saved {
    format: u32,
    /// The code applied after it, as a player reads it, or "" for a full code.
    code: String,
    /// When, in UTC: `2026-10-31T12:00:00Z`.
    created: String,
    /// `plugins.json` and `sources.json` as they were; null where there was none.
    list: Value,
    sources: Value,
    /// Every installed mod's version, by id.
    installed: BTreeMap<String, String>,
    /// Those of them with a copy in the profile's folder.
    copied: Vec<String>,
}

/// A profile as the Mods tab shows it.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[ts(rename = "ModProfile")]
#[serde(rename_all = "camelCase")]
pub struct View {
    pub id: String,
    pub code: String,
    pub created: String,
    /// How many mods were installed then.
    pub mods: usize,
}

fn root(dir: &Path) -> PathBuf {
    dir.join(PROFILES)
}

/// An id this module made: a time and a counter, so that no path comes from anywhere else.
fn valid_id(id: &str) -> bool {
    !id.is_empty() && id.len() <= 32 && id.chars().all(|c| c.is_ascii_alphanumeric() || c == '-')
}

fn read_json(path: &Path) -> Result<Value, String> {
    match std::fs::read_to_string(path) {
        Ok(text) => serde_json::from_str(&text).map_err(|e| format!("{}: {e}", path.display())),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(Value::Null),
        Err(e) => Err(format!("{}: {e}", path.display())),
    }
}

/// Writes `file` beside itself first, so that a failure leaves the old one.
fn write_json(path: &Path, value: &Value) -> Result<(), String> {
    let temporary = path.with_extension("tmp");
    let text = serde_json::to_string_pretty(value).map_err(|e| e.to_string())? + "\n";
    std::fs::write(&temporary, text).map_err(|e| format!("{}: {e}", temporary.display()))?;
    std::fs::rename(&temporary, path).map_err(|e| format!("{}: {e}", path.display()))
}

/// The installed mods' versions, by id.
fn installed_now(dir: &Path) -> BTreeMap<String, String> {
    std::fs::read_dir(dir)
        .into_iter()
        .flatten()
        .flatten()
        .filter_map(|e| {
            let m = plugins::read_manifest(&e.path()).ok()?;
            let id = e.file_name().to_str()?.to_string();
            plugins::valid_id(&id).then_some((id, m.version))
        })
        .collect()
}

/// Keeps what the mods look like now, with a copy of each mod of `replaced` that is installed.
/// `now` is the time in UTC, which names it.
pub fn create(dir: &Path, code: &str, replaced: &[String], now: &str) -> Result<String, String> {
    let base: String = now.chars().filter(char::is_ascii_alphanumeric).collect();
    let folder = root(dir);
    std::fs::create_dir_all(&folder).map_err(|e| format!("{}: {e}", folder.display()))?;
    let id = (0..1000)
        .map(|n| if n == 0 { base.clone() } else { format!("{base}-{n}") })
        .find(|id| !folder.join(format!("{id}.json")).exists())
        .ok_or("too many profiles at once")?;
    let copies = folder.join(&id);
    let installed = installed_now(dir);
    let mut copied = Vec::new();
    for mod_id in replaced.iter().filter(|m| installed.contains_key(*m)) {
        plugins::copy_tree(&dir.join(mod_id), &copies.join(mod_id))?;
        copied.push(mod_id.clone());
    }
    let saved = Saved {
        format: 1,
        code: code.to_string(),
        created: now.to_string(),
        list: read_json(&dir.join(LIST))?,
        sources: read_json(&dir.join(SOURCES))?,
        installed,
        copied,
    };
    write_json(&folder.join(format!("{id}.json")), &serde_json::to_value(saved).map_err(|e| e.to_string())?)?;
    prune(dir);
    Ok(id)
}

fn read_saved(dir: &Path, id: &str) -> Result<Saved, String> {
    if !valid_id(id) {
        return Err(format!("no saved set {id}"));
    }
    let path = root(dir).join(format!("{id}.json"));
    let value = read_json(&path)?;
    if value.is_null() {
        return Err(format!("no saved set {id}"));
    }
    serde_json::from_value(value).map_err(|e| format!("{}: {e}", path.display()))
}

/// The saved profiles, the newest first.
pub fn list(dir: &Path) -> Vec<View> {
    let mut ids: Vec<String> = std::fs::read_dir(root(dir))
        .into_iter()
        .flatten()
        .flatten()
        .filter_map(|e| e.file_name().to_str()?.strip_suffix(".json").map(str::to_string))
        .filter(|id| valid_id(id))
        .collect();
    ids.sort();
    ids.reverse();
    ids.into_iter()
        .filter_map(|id| {
            let s = read_saved(dir, &id).ok()?;
            Some(View { id, code: s.code, created: s.created, mods: s.installed.len() })
        })
        .collect()
}

fn delete(dir: &Path, id: &str) {
    let _ = std::fs::remove_file(root(dir).join(format!("{id}.json")));
    let _ = std::fs::remove_dir_all(root(dir).join(id));
}

/// Forgets every profile past the newest [`KEEP`].
fn prune(dir: &Path) {
    for old in list(dir).into_iter().skip(KEEP) {
        delete(dir, &old.id);
    }
}

/// Forgets a profile: an apply that never started needs no way back.
pub fn discard(dir: &Path, id: &str) {
    if valid_id(id) {
        delete(dir, id);
    }
}

/// Puts each copied mod back in place, then the list and the sources. With `remove_new`, a mod
/// that was not installed then goes too: that is undoing a failed apply. Otherwise (the player's
/// restore) it stays, off, for it may be theirs since.
pub fn restore(dir: &Path, id: &str, remove_new: bool) -> Result<(), String> {
    let saved = read_saved(dir, id)?;
    let copies = root(dir).join(id);
    for mod_id in &saved.copied {
        if !plugins::valid_id(mod_id) {
            return Err(format!("no mod {mod_id}"));
        }
        let scratch = plugins::Scratch::new(dir, "restore")?;
        let copy = scratch.0.join(mod_id);
        plugins::copy_tree(&copies.join(mod_id), &copy)?;
        let target = dir.join(mod_id);
        let old = scratch.0.join(".old");
        let replaced = target.exists();
        if replaced {
            std::fs::rename(&target, &old).map_err(|e| format!("{}: {e}", target.display()))?;
        }
        if let Err(e) = std::fs::rename(&copy, &target) {
            if replaced {
                let _ = std::fs::rename(&old, &target);
            }
            return Err(format!("{}: {e}", target.display()));
        }
    }
    if remove_new {
        for mod_id in installed_now(dir).keys().filter(|m| !saved.installed.contains_key(*m)) {
            let folder = dir.join(mod_id);
            std::fs::remove_dir_all(&folder).map_err(|e| format!("{}: {e}", folder.display()))?;
        }
    }
    restore_file(&dir.join(LIST), &saved.list)?;
    restore_file(&dir.join(SOURCES), &saved.sources)?;
    Ok(())
}

fn restore_file(path: &Path, value: &Value) -> Result<(), String> {
    if value.is_null() {
        return match std::fs::remove_file(path) {
            Err(e) if e.kind() != std::io::ErrorKind::NotFound => Err(format!("{}: {e}", path.display())),
            _ => Ok(()),
        };
    }
    write_json(path, value)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    /// A mod folder of the store, with its manifest and one file.
    fn add(dir: &Path, id: &str, version: &str, body: &str) {
        let folder = dir.join(id);
        std::fs::create_dir_all(&folder).unwrap();
        std::fs::write(
            folder.join(plugins::MANIFEST),
            json!({"format": 2, "id": id, "name": id, "version": version, "games": {"rac1": ["pal"]}}).to_string(),
        )
        .unwrap();
        std::fs::write(folder.join("file"), body).unwrap();
    }

    fn list_file(dir: &Path, ids: &[(&str, bool)]) {
        let mods: Vec<Value> = ids.iter().map(|(id, on)| json!({"id": id, "enabled": on, "settings": {}})).collect();
        std::fs::write(dir.join(LIST), json!({"format": 1, "mods": mods}).to_string()).unwrap();
    }

    #[test]
    fn a_restore_puts_back_folders_list_and_sources() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        add(dir, "alpha", "1.0.0", "old");
        add(dir, "beta", "1.0.0", "untouched");
        list_file(dir, &[("alpha", true), ("beta", false)]);
        std::fs::write(dir.join(SOURCES), r#"{"format": 1, "mods": {}}"#).unwrap();

        let id = create(dir, "K7M2-9QXR", &["alpha".into(), "not-installed".into()], "2026-10-31T12:00:00Z").unwrap();

        add(dir, "alpha", "2.0.0", "new");
        add(dir, "gamma", "1.0.0", "the set's");
        list_file(dir, &[("gamma", true), ("alpha", false)]);
        std::fs::remove_file(dir.join(SOURCES)).unwrap();

        restore(dir, &id, false).unwrap();

        assert_eq!(std::fs::read_to_string(dir.join("alpha/file")).unwrap(), "old");
        assert_eq!(plugins::read_manifest(&dir.join("alpha")).unwrap().version, "1.0.0");
        assert!(std::fs::read_to_string(dir.join(LIST)).unwrap().contains("beta"));
        assert!(dir.join(SOURCES).is_file());
        // The player's restore keeps a mod that came since: its line is gone, so it reads as off.
        assert!(dir.join("gamma").is_dir());
    }

    #[test]
    fn undoing_a_failed_apply_removes_what_it_added() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        add(dir, "alpha", "1.0.0", "old");
        let id = create(dir, "", &[], "2026-10-31T12:00:00Z").unwrap();
        add(dir, "gamma", "1.0.0", "the set's");
        std::fs::write(dir.join(LIST), "{}").unwrap();

        restore(dir, &id, true).unwrap();

        assert!(!dir.join("gamma").exists());
        assert!(dir.join("alpha").is_dir());
        // There was no list before: there is none after.
        assert!(!dir.join(LIST).exists());
    }

    #[test]
    fn the_last_ten_are_kept_newest_first() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        add(dir, "alpha", "1.0.0", "old");
        for hour in 0..13 {
            create(dir, "", &["alpha".into()], &format!("2026-10-31T{hour:02}:00:00Z")).unwrap();
        }
        let kept = list(dir);
        assert_eq!(kept.len(), KEEP);
        assert_eq!(kept[0].created, "2026-10-31T12:00:00Z");
        assert_eq!(kept[KEEP - 1].created, "2026-10-31T03:00:00Z");
        // A pruned profile's copies go with it.
        assert!(!root(dir).join("20261031T000000Z").exists());
    }

    #[test]
    fn two_in_the_same_second_stay_apart() {
        let t = tempfile::tempdir().unwrap();
        let a = create(t.path(), "", &[], "2026-10-31T12:00:00Z").unwrap();
        let b = create(t.path(), "", &[], "2026-10-31T12:00:00Z").unwrap();
        assert_ne!(a, b);
    }

    #[test]
    fn an_id_never_names_a_path() {
        let t = tempfile::tempdir().unwrap();
        assert!(restore(t.path(), "../../etc", false).is_err());
        assert!(restore(t.path(), "a/b", false).is_err());
        assert!(restore(t.path(), "missing", false).is_err());
        std::fs::write(t.path().join("victim"), "x").unwrap();
        discard(t.path(), "../victim");
        assert!(t.path().join("victim").exists());
    }
}
