//! Mods kept current and tidy without the player managing them (the plan's Phase 5).
//!
//! - **Updates** come from one fetch of the signed index. Patch and minor versions go in by
//!   themselves; a major version, or a native mod's version that takes new hooks, waits for a
//!   click with its changelog. A mod kept at its version (a pin) is left alone. Every update is
//!   planned as one set through the resolver: an update that would break another mod's `requires`
//!   is held back ("A needs B 1.2.0"), and the API and pre-release rules of resolver.rs apply.
//!   Nothing is updated while the game runs. A set is all or nothing (index_apply.rs).
//! - **Rollback after a crash**: the last update is remembered (`plugins/.last-update.json`), so a
//!   crash after it can offer "Roll back B to 1.2.0", which pins B until a version newer than the
//!   bad one comes out.
//! - **Unused dependencies**: removing a mod finds the ones it alone needed; those the launcher
//!   installed (`auto`) can go by themselves, those the player installed are asked about. A mod
//!   pack is its mods: removing it removes the ones it brought.

use std::collections::BTreeSet;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::deeplink::WantedMod;
use crate::index::ModIndex;
use crate::index_apply::{self, ApplyResult};
use crate::mod_error::{ModAction, ModError, ModErrorKind};
use crate::plugins::{self, compare_versions, meets, parse_version};
use crate::remote::{self, Sources};
use crate::resolver::{self, api_fits, InstalledMod, Why};

/// Why an update does not go in by itself.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase", rename_all_fields = "camelCase", tag = "kind")]
pub enum Hold {
    /// A new major version: it may change gameplay or saves.
    Major,
    /// A native mod's new version takes hooks it did not before.
    NewHooks { hooks: Vec<String> },
    /// Another mod needs the installed version (`by` wants `wants`).
    HeldBack { by: String, by_name: String, wants: String },
    /// The new version needs `id` at a version the set cannot give.
    Needs { id: String, wants: Option<String> },
    /// The newest version needs a newer launcher.
    NeedsLauncher { api: String },
    /// Something else stops it (a conflict, a missing mod).
    Blocked { detail: String },
}

#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct UpdateItem {
    pub id: String,
    pub name: String,
    pub from: String,
    pub to: String,
    pub native: bool,
    /// The release notes of the new version, from the index.
    pub changelog: String,
    pub hold: Option<Hold>,
}

/// What the look for updates found, sorted.
#[derive(Debug, Clone, PartialEq, Serialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct UpdatePlan {
    /// Go in by themselves.
    pub silent: Vec<UpdateItem>,
    /// "Update available": wait for a click (major, new hooks).
    pub waiting: Vec<UpdateItem>,
    /// Cannot go in now, and why.
    pub held: Vec<UpdateItem>,
}

/// What a game's mods are checked against.
pub struct Here<'a> {
    pub game: &'a str,
    pub disc: &'a str,
    pub platform: Option<&'a str>,
    pub api: (u32, u32),
    pub prerelease: bool,
}

fn newer(a: &str, b: &str) -> bool {
    match (parse_version(a), parse_version(b)) {
        (Some(a), Some(b)) => compare_versions(a, b).is_gt(),
        _ => false,
    }
}

fn major(v: &str) -> Option<u64> {
    parse_version(v).map(|p| p.0)
}

/// Whether a pin still holds: a rollback's pin lifts once a version newer than the bad one is out.
fn pinned(sources: &Sources, id: &str, newest: Option<&str>) -> bool {
    let Some(s) = sources.mods.get(id).filter(|s| s.pin) else { return false };
    match (s.pin_reason.as_deref(), s.skip.as_deref(), newest) {
        (Some("rollback"), Some(bad), Some(n)) => !newer(n, bad),
        _ => true,
    }
}

/// The exact set a run of updates ends at, as the resolver's wanted list: every mod that is on
/// at its version, with `targets` at theirs.
fn wanted_for(installed: &[InstalledMod], targets: &[UpdateItem]) -> Vec<WantedMod> {
    installed
        .iter()
        .filter(|m| m.enabled || targets.iter().any(|t| t.id == m.id))
        .map(|m| WantedMod {
            id: m.id.clone(),
            version: Some(targets.iter().find(|t| t.id == m.id).map_or(m.version.clone(), |t| t.to.clone())),
        })
        .collect()
}

fn request<'a>(
    index: &'a ModIndex,
    installed: &'a [InstalledMod],
    here: &'a Here,
    wanted: &'a [WantedMod],
) -> resolver::Request<'a> {
    resolver::Request {
        index,
        installed,
        game: here.game,
        disc: here.disc,
        platform: here.platform,
        api: here.api,
        allow_prerelease: here.prerelease,
        wanted,
    }
}

/// The updates the index offers for the installed mods, sorted into those that go in by
/// themselves, those that wait for a click and those held back.
pub fn plan(index: &ModIndex, installed: &[InstalledMod], sources: &Sources, here: &Here) -> UpdatePlan {
    let mut out = UpdatePlan::default();
    let mut candidates = Vec::new();
    for have in installed {
        let Some(m) = index.get(&have.id) else { continue };
        if m.kind == "pack" {
            continue;
        }
        let runs = |v: &crate::index::Version| {
            v.games.get(here.game).is_some_and(|d| d.iter().any(|d| d == here.disc))
                && (m.kind != "native" || here.platform.is_some_and(|p| v.platforms.contains_key(p)))
        };
        let offered: Vec<&crate::index::Version> = m
            .versions
            .iter()
            .filter(|v| newer(&v.version, &have.version))
            .filter(|v| index.revoked(&m.id, &v.version).is_none() && (here.prerelease || !v.prerelease) && runs(v))
            .collect();
        let best = offered.iter().copied().filter(|v| api_fits(v.api.as_deref(), here.api)).max_by(|a, b| {
            compare_versions(
                parse_version(&a.version).unwrap_or_default(),
                parse_version(&b.version).unwrap_or_default(),
            )
        });
        let skip = sources.mods.get(&m.id).and_then(|s| s.skip.clone());
        if pinned(sources, &m.id, best.map(|v| v.version.as_str())) {
            continue;
        }
        let item = |v: &crate::index::Version, hold| UpdateItem {
            id: m.id.clone(),
            name: m.name.clone(),
            from: have.version.clone(),
            to: v.version.clone(),
            native: m.kind == "native",
            changelog: v.changelog.clone(),
            hold,
        };
        let Some(v) = best.filter(|v| skip.as_deref() != Some(v.version.as_str())) else {
            // Only a version for a newer launcher: it waits until the launcher updates.
            if let Some(v) = offered.first().filter(|_| best.is_none()) {
                out.held.push(item(v, Some(Hold::NeedsLauncher { api: v.api.clone().unwrap_or_default() })));
            }
            continue;
        };
        let old_hooks: BTreeSet<&String> =
            m.version(&have.version).map(|o| o.hooks.iter().collect()).unwrap_or_default();
        let new_hooks: Vec<String> = v.hooks.iter().filter(|h| !old_hooks.contains(h)).cloned().collect();
        if major(&v.version) != major(&have.version) {
            out.waiting.push(item(v, Some(Hold::Major)));
        } else if m.kind == "native" && m.version(&have.version).is_some() && !new_hooks.is_empty() {
            out.waiting.push(item(v, Some(Hold::NewHooks { hooks: new_hooks })));
        } else {
            candidates.push(item(v, None));
        }
    }
    // One set: what breaks another mod is held back, until what is left resolves.
    let name_of = |id: &str| index.get(id).map_or(id.to_string(), |m| m.name.clone());
    for _ in 0..=candidates.len() {
        if candidates.is_empty() {
            break;
        }
        let wanted = wanted_for(installed, &candidates);
        let set = resolver::resolve(&request(index, installed, here, &wanted));
        if !set.blocked {
            break;
        }
        let target = |id: &str| candidates.iter().find(|c| c.id == id).map(|c| c.to.clone());
        let mut holds: Vec<(String, Hold)> = Vec::new();
        for i in &set.incompatible {
            match &i.why {
                Why::Clash { wants } => {
                    let at =
                        target(&i.id).or_else(|| installed.iter().find(|m| m.id == i.id).map(|m| m.version.clone()));
                    let broken = wants.iter().filter(|w| {
                        !w.by.is_empty()
                            && !meets(at.as_deref().unwrap_or_default(), w.version.as_deref()).unwrap_or(false)
                    });
                    for w in broken {
                        if target(&i.id).is_some() {
                            let wants = w.version.clone().unwrap_or_default();
                            holds.push((
                                i.id.clone(),
                                Hold::HeldBack { by: w.by.clone(), by_name: name_of(&w.by), wants },
                            ));
                        } else if target(&w.by).is_some() {
                            holds.push((w.by.clone(), Hold::Needs { id: i.id.clone(), wants: w.version.clone() }));
                        }
                    }
                }
                _ if target(&i.id).is_some() => {
                    holds.push((i.id.clone(), Hold::Blocked { detail: format!("{:?}", i.why) }));
                }
                _ => {}
            }
        }
        for m in &set.missing {
            if let Some(by) = m.required_by.as_ref().filter(|by| target(by).is_some()) {
                holds.push((by.clone(), Hold::Needs { id: m.id.clone(), wants: None }));
            }
        }
        for c in &set.conflicts {
            for id in [&c.id, &c.with] {
                if target(id).is_some() {
                    holds.push((
                        id.clone(),
                        Hold::Blocked { detail: format!("{} conflicts with {}", c.name, c.with_name) },
                    ));
                }
            }
        }
        if holds.is_empty() {
            // Nothing to blame: none goes in this time.
            for c in candidates.drain(..) {
                out.held
                    .push(UpdateItem { hold: Some(Hold::Blocked { detail: "the set does not resolve".into() }), ..c });
            }
            break;
        }
        for (id, hold) in holds {
            if let Some(at) = candidates.iter().position(|c| c.id == id) {
                let c = candidates.remove(at);
                out.held.push(UpdateItem { hold: Some(hold), ..c });
            }
        }
    }
    out.silent = candidates;
    out
}

/// The last run of updates, for the crash notice's "Roll back".
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct LastUpdate {
    /// Seconds since 1970.
    pub at: u64,
    pub mods: Vec<UpdatedMod>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct UpdatedMod {
    pub id: String,
    pub name: String,
    pub from: String,
    pub to: String,
}

const LAST_UPDATE: &str = ".last-update.json";

pub fn last_update(dir: &Path) -> Option<LastUpdate> {
    serde_json::from_slice(&std::fs::read(dir.join(LAST_UPDATE)).ok()?).ok()
}

fn write_last_update(dir: &Path, last: &LastUpdate) -> Result<(), String> {
    let text = serde_json::to_string_pretty(last).map_err(|e| e.to_string())?;
    std::fs::write(dir.join(LAST_UPDATE), text).map_err(|e| e.to_string())
}

/// What a run of updates came to.
#[derive(Debug, Clone, PartialEq, Serialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct UpdateRun {
    pub updated: Vec<UpdateItem>,
    pub plan: UpdatePlan,
    pub result: Option<ApplyResult>,
}

/// Installs `items` (the plan's silent ones, or the waiting ones the player clicked) as one set:
/// one failure puts every one back. Refused while the game runs.
pub fn apply(
    ctx: &remote::Context,
    index: &ModIndex,
    installed: &[InstalledMod],
    here: &Here,
    items: &[UpdateItem],
    game_running: bool,
    data_home: &Path,
) -> Result<(Vec<UpdateItem>, ApplyResult), ModError> {
    if game_running {
        return Err(ModError::new(ModErrorKind::UserFixable, "Mods are not updated while the game runs.")
            .with(ModAction::CloseGame));
    }
    if items.is_empty() {
        return Ok((Vec::new(), ApplyResult::default()));
    }
    let wanted = wanted_for(installed, items);
    let set = resolver::resolve(&request(index, installed, here, &wanted));
    let options =
        index_apply::Options { serial: None, data_home, turn_off: &[], pin_shared: &[], all_or_nothing: true };
    let result = index_apply::apply(ctx, &set, &options, |_| {})?;
    if result.failed.is_some() {
        return Ok((Vec::new(), result));
    }
    let done: Vec<UpdateItem> = items.iter().filter(|i| result.done.contains(&i.id)).cloned().collect();
    // A rollback's pin is lifted by the newer version that just went in.
    let mut sources = remote::read_sources(ctx.dir);
    for i in &done {
        if let Some(s) = sources.mods.get_mut(&i.id).filter(|s| s.pin_reason.as_deref() == Some("rollback")) {
            s.pin = false;
            s.pin_reason = None;
            s.skip = None;
        }
    }
    remote::write_sources(ctx.dir, &sources).map_err(|e| ModError::new(ModErrorKind::Bug, e))?;
    let last = LastUpdate {
        at: crate::index::now(),
        mods: done
            .iter()
            .map(|i| UpdatedMod { id: i.id.clone(), name: i.name.clone(), from: i.from.clone(), to: i.to.clone() })
            .collect(),
    };
    if !done.is_empty() {
        write_last_update(ctx.dir, &last).map_err(|e| ModError::new(ModErrorKind::Bug, e))?;
    }
    Ok((done, result))
}

/// After a crash: mod `id` goes back to the version its last update replaced, and is kept there
/// until a version newer than the one that crashed comes out.
pub fn crash_rollback(dir: &Path, game: &str, disc: &str, id: &str) -> Result<Vec<plugins::PluginView>, String> {
    let bad = plugins::read_manifest(&dir.join(id))?.version;
    let views = plugins::roll_back(dir, game, disc, id)?;
    let back = plugins::read_manifest(&dir.join(id))?.version;
    let mut sources = remote::read_sources(dir);
    if let Some(s) = sources.mods.get_mut(id) {
        s.version = back;
        s.pin = true;
        s.pin_reason = Some("rollback".into());
        s.skip = Some(bad);
        remote::write_sources(dir, &sources)?;
    }
    if let Some(mut last) = last_update(dir) {
        last.mods.retain(|m| m.id != id);
        let _ = if last.mods.is_empty() {
            std::fs::remove_file(dir.join(LAST_UPDATE)).map_err(|e| e.to_string())
        } else {
            write_last_update(dir, &last)
        };
    }
    Ok(views)
}

// ---- Unused dependencies ----------------------------------------------------------------------

/// A dependency nothing else needs once a mod is removed.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Orphan {
    pub id: String,
    pub name: String,
    /// The launcher installed it (sources.json `auto`): it goes without asking when cleaning up is on.
    pub auto: bool,
}

/// The mods that `id` needed (directly or through another, or a pack's members) that nothing left
/// installed needs once `id` is gone.
pub fn orphans(dir: &Path, id: &str) -> Vec<Orphan> {
    let sources = remote::read_sources(dir);
    let installed: Vec<String> =
        plugins::list(dir, "", "").map(|v| v.into_iter().map(|v| v.id).collect()).unwrap_or_default();
    let needs = |of: &str| -> Vec<String> {
        let mut out: Vec<String> = plugins::read_manifest(&dir.join(of))
            .map(|m| m.requires.into_iter().map(|r| r.id).collect())
            .unwrap_or_default();
        if let Some(s) = sources.mods.get(of) {
            out.extend(s.members.iter().cloned());
        }
        out
    };
    let mut nodes: Vec<String> = installed.clone();
    nodes.extend(sources.mods.iter().filter(|(_, s)| s.kind == resolver::MODPACK).map(|(k, _)| k.clone()));
    // Everything still needed by what stays, from the mods (and packs) that are not `id`.
    let mut kept: BTreeSet<String> = BTreeSet::new();
    let mut stack: Vec<String> = Vec::new();
    // The roots are what nothing needs (`id`'s own needs count): the player's mods and packs.
    let needed: BTreeSet<String> = nodes.iter().flat_map(|n| needs(n)).collect();
    for n in nodes.iter().filter(|n| *n != id && !needed.contains(*n)) {
        stack.push(n.clone());
    }
    while let Some(n) = stack.pop() {
        if n == id || !kept.insert(n.clone()) {
            continue;
        }
        stack.extend(needs(&n));
    }
    // What `id` needed, transitively, that is not kept.
    let mut gone = Vec::new();
    let mut seen = BTreeSet::new();
    let mut stack = needs(id);
    while let Some(n) = stack.pop() {
        if !seen.insert(n.clone()) || kept.contains(&n) || !nodes.contains(&n) {
            continue;
        }
        stack.extend(needs(&n));
        let name = plugins::read_manifest(&dir.join(&n)).map_or(n.clone(), |m| m.name);
        gone.push(Orphan { auto: sources.mods.get(&n).is_some_and(|s| s.auto), id: n, name });
    }
    gone
}

/// Removes mod `id` (or a mod pack) and the dependencies in `also` (from [`orphans`]); the list.
pub fn remove_with(
    dir: &Path,
    game: &str,
    disc: &str,
    id: &str,
    also: &[String],
) -> Result<Vec<plugins::PluginView>, String> {
    let is_pack = remote::read_sources(dir).mods.get(id).is_some_and(|s| s.kind == resolver::MODPACK);
    if !is_pack {
        plugins::remove(dir, game, disc, id)?;
    }
    remote::forget(dir, id)?;
    for other in also {
        if plugins::list(dir, game, disc)?.iter().any(|v| v.id == *other) {
            plugins::remove(dir, game, disc, other)?;
        }
        remote::forget(dir, other)?;
    }
    plugins::list(dir, game, disc)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::index::tests::fixture;
    use crate::index::{IndexMod, Require, Version};

    const HERE: Here =
        Here { game: "rac1", disc: "pal", platform: Some("linux-x86_64"), api: (1, 10), prerelease: false };

    fn have(id: &str, version: &str) -> InstalledMod {
        InstalledMod { id: id.into(), version: version.into(), enabled: true }
    }

    fn script(id: &str, versions: &[(&str, &[(&str, &str)])]) -> IndexMod {
        IndexMod {
            id: id.into(),
            name: id.into(),
            kind: "script".into(),
            versions: versions
                .iter()
                .map(|(v, req)| Version {
                    version: (*v).into(),
                    api: Some("1.1".into()),
                    games: [("rac1".to_string(), vec!["pal".to_string()])].into(),
                    requires: req
                        .iter()
                        .map(|(id, w)| Require { id: (*id).into(), version: Some((*w).into()) })
                        .collect(),
                    changelog: format!("notes {v}"),
                    tag: format!("{id}-v{v}"),
                    ..Default::default()
                })
                .collect(),
            ..Default::default()
        }
    }

    fn ids(items: &[UpdateItem]) -> Vec<(&str, &str)> {
        items.iter().map(|i| (i.id.as_str(), i.to.as_str())).collect()
    }

    #[test]
    fn patch_and_minor_go_in_by_themselves_and_majors_wait() {
        let index = fixture();
        let installed = [have("camera-base", "1.1.0"), have("lock-strafe", "1.1.0")];
        let plan = super::plan(&index, &installed, &Sources::default(), &HERE);
        assert_eq!(ids(&plan.silent), [("camera-base", "1.2.0")]);
        // lock-strafe 1.2.0 is a native minor update that takes hooks 1.1.0 did not: it waits.
        let lock = plan.waiting.iter().find(|w| w.id == "lock-strafe").expect("waits");
        assert!(matches!(&lock.hold, Some(Hold::NewHooks { hooks }) if hooks.len() == 2));

        let mut index = ModIndex { format: 1, ..Default::default() };
        index.mods.push(script("big", &[("2.0.0", &[]), ("1.0.0", &[])]));
        let plan = super::plan(&index, &[have("big", "1.0.0")], &Sources::default(), &HERE);
        assert!(plan.silent.is_empty());
        assert_eq!(plan.waiting[0].hold, Some(Hold::Major));
        assert_eq!(plan.waiting[0].changelog, "notes 2.0.0");
    }

    #[test]
    fn a_native_version_with_new_hooks_waits() {
        let mut index = fixture();
        let lock = index.mods.iter_mut().find(|m| m.id == "lock-strafe").unwrap();
        lock.versions[0].requires.clear();
        lock.versions[1].hooks = vec!["rac1.pad.process_report".into()];
        let plan = super::plan(&index, &[have("lock-strafe", "1.1.0")], &Sources::default(), &HERE);
        assert_eq!(plan.waiting[0].hold, Some(Hold::NewHooks { hooks: vec!["rac1.player.turn_to_heading".into()] }));
        // The same hooks: in by itself.
        let lock = index.mods.iter_mut().find(|m| m.id == "lock-strafe").unwrap();
        lock.versions[1].hooks = lock.versions[0].hooks.clone();
        assert_eq!(ids(&plan_of(&index, &[have("lock-strafe", "1.1.0")])), [("lock-strafe", "1.2.0")]);
    }

    fn plan_of(index: &ModIndex, installed: &[InstalledMod]) -> Vec<UpdateItem> {
        super::plan(index, installed, &Sources::default(), &HERE).silent
    }

    #[test]
    fn an_update_that_breaks_another_mod_is_held_back() {
        let mut index = ModIndex { format: 1, ..Default::default() };
        index.mods.push(script("base", &[("1.3.0", &[]), ("1.2.0", &[])]));
        index.mods.push(script("user", &[("1.0.0", &[("base", "=1.2.0")])]));
        let plan = super::plan(&index, &[have("base", "1.2.0"), have("user", "1.0.0")], &Sources::default(), &HERE);
        assert!(plan.silent.is_empty());
        assert_eq!(
            plan.held[0].hold,
            Some(Hold::HeldBack { by: "user".into(), by_name: "user".into(), wants: "=1.2.0".into() })
        );
        // Both have newer versions that fit together: they update together.
        index.mods[1] = script("user", &[("1.1.0", &[("base", ">=1.3.0")]), ("1.0.0", &[("base", "=1.2.0")])]);
        let plan2 = plan_of(&index, &[have("base", "1.2.0"), have("user", "1.0.0")]);
        assert_eq!(ids(&plan2), [("base", "1.3.0"), ("user", "1.1.0")]);
    }

    #[test]
    fn pins_api_and_prerelease_are_respected() {
        let index = fixture();
        let mut sources = Sources::default();
        sources.mods.insert("camera-base".into(), remote::ModSource { pin: true, ..Default::default() });
        assert!(super::plan(&index, &[have("camera-base", "1.1.0")], &sources, &HERE).silent.is_empty());
        // item-randomizer 1.4.0 needs the dev build: it waits for the launcher.
        let plan = super::plan(&index, &[have("item-randomizer", "1.3.0")], &Sources::default(), &HERE);
        assert!(plan.silent.is_empty());
        assert_eq!(plan.held[0].hold, Some(Hold::NeedsLauncher { api: "1.11".into() }));
        // On the dev build with pre-releases, the beta is a major: it waits.
        let dev = Here { api: (1, 11), prerelease: true, ..HERE };
        let p = super::plan(&index, &[have("item-randomizer", "1.3.0")], &Sources::default(), &dev);
        assert_eq!(ids(&p.waiting), [("item-randomizer", "2.0.0-beta.1")]);
        // A rollback's pin lifts only for a version newer than the bad one.
        let mut back = Sources::default();
        back.mods.insert(
            "camera-base".into(),
            remote::ModSource {
                pin: true,
                pin_reason: Some("rollback".into()),
                skip: Some("1.2.0".into()),
                ..Default::default()
            },
        );
        assert!(super::plan(&index, &[have("camera-base", "1.1.0")], &back, &HERE).silent.is_empty());
        back.mods.get_mut("camera-base").unwrap().skip = Some("1.1.5".into());
        assert_eq!(
            ids(&super::plan(&index, &[have("camera-base", "1.1.0")], &back, &HERE).silent),
            [("camera-base", "1.2.0")]
        );
    }

    #[test]
    fn nothing_updates_while_the_game_runs() {
        let dir = tempfile::tempdir().unwrap();
        struct Never;
        impl remote::Fetch for Never {
            fn get(&self, url: &str, _: bool) -> Result<Vec<u8>, String> {
                panic!("fetched {url} while the game runs")
            }
        }
        let ctx = remote::Context {
            dir: dir.path(),
            game: "rac1",
            version: "pal",
            platform: None,
            fetch: &Never,
            hosts: remote::Hosts::default(),
        };
        let index = fixture();
        let installed = [have("camera-base", "1.1.0")];
        let items = plan_of(&index, &installed);
        let e = apply(&ctx, &index, &installed, &HERE, &items, true, dir.path()).unwrap_err();
        assert_eq!(e.action, Some(ModAction::CloseGame));
    }

    fn mod_folder(dir: &Path, id: &str, version: &str, requires: &[&str]) {
        let req: Vec<serde_json::Value> = requires.iter().map(|r| serde_json::json!({"id": r})).collect();
        let m = serde_json::json!({"format": 2, "id": id, "name": format!("Mod {id}"), "version": version, "author": "a",
            "type": "script", "api": "1.1", "games": {"rac1": ["pal"]}, "scripts": ["main.luau"], "requires": req});
        let src = dir.join(".src").join(format!("{id}-{version}"));
        std::fs::create_dir_all(&src).unwrap();
        std::fs::write(src.join("openrac-mod.json"), m.to_string()).unwrap();
        std::fs::write(src.join("main.luau"), "print(1)").unwrap();
        plugins::install(dir, "rac1", "pal", &src).unwrap();
        let _ = std::fs::remove_dir_all(dir.join(".src"));
    }

    #[test]
    fn a_crash_rollback_puts_the_old_version_back_and_pins_it() {
        let tmp = tempfile::tempdir().unwrap();
        let dir = tmp.path();
        mod_folder(dir, "camera-base", "1.1.0", &[]);
        mod_folder(dir, "camera-base", "1.2.0", &[]);
        let mut sources = Sources::default();
        sources.mods.insert("camera-base".into(), remote::ModSource { version: "1.2.0".into(), ..Default::default() });
        remote::write_sources(dir, &sources).unwrap();
        let last = LastUpdate {
            at: 1,
            mods: vec![UpdatedMod {
                id: "camera-base".into(),
                name: "Camera".into(),
                from: "1.1.0".into(),
                to: "1.2.0".into(),
            }],
        };
        write_last_update(dir, &last).unwrap();
        let views = crash_rollback(dir, "rac1", "pal", "camera-base").unwrap();
        assert_eq!(views[0].version, "1.1.0");
        let s = &remote::read_sources(dir).mods["camera-base"];
        assert!(s.pin);
        assert_eq!(
            (s.pin_reason.as_deref(), s.skip.as_deref(), s.version.as_str()),
            (Some("rollback"), Some("1.2.0"), "1.1.0")
        );
        assert!(last_update(dir).is_none());
        // The index's 1.2.0 is the bad one: not offered again.
        let installed = [have("camera-base", "1.1.0")];
        assert!(super::plan(&fixture(), &installed, &remote::read_sources(dir), &HERE).silent.is_empty());
    }

    #[test]
    fn removing_a_mod_finds_the_dependencies_it_alone_needed() {
        let tmp = tempfile::tempdir().unwrap();
        let dir = tmp.path();
        mod_folder(dir, "shared-dep", "1.0.0", &[]);
        mod_folder(dir, "only-dep", "1.0.0", &[]);
        mod_folder(dir, "mine-dep", "1.0.0", &[]);
        mod_folder(dir, "app-a", "1.0.0", &["shared-dep", "only-dep", "mine-dep"]);
        mod_folder(dir, "app-b", "1.0.0", &["shared-dep"]);
        let mut sources = Sources::default();
        for (id, auto) in
            [("shared-dep", true), ("only-dep", true), ("mine-dep", false), ("app-a", false), ("app-b", false)]
        {
            sources.mods.insert(id.into(), remote::ModSource { version: "1.0.0".into(), auto, ..Default::default() });
        }
        remote::write_sources(dir, &sources).unwrap();
        let mut found = orphans(dir, "app-a");
        found.sort_by(|a, b| a.id.cmp(&b.id));
        // shared-dep stays (app-b needs it); mine-dep was the player's own: it is asked about.
        assert_eq!(
            found.iter().map(|o| (o.id.as_str(), o.auto)).collect::<Vec<_>>(),
            [("mine-dep", false), ("only-dep", true)]
        );
        let left = remove_with(dir, "rac1", "pal", "app-a", &["only-dep".into()]).unwrap();
        let ids: BTreeSet<&str> = left.iter().map(|v| v.id.as_str()).collect();
        assert_eq!(ids, ["app-b", "mine-dep", "shared-dep"].into());
        assert!(!remote::read_sources(dir).mods.contains_key("only-dep"));
    }

    #[test]
    fn removing_a_pack_removes_the_mods_it_brought() {
        let tmp = tempfile::tempdir().unwrap();
        let dir = tmp.path();
        mod_folder(dir, "in-pack", "1.0.0", &[]);
        mod_folder(dir, "kept", "1.0.0", &[]);
        let mut sources = Sources::default();
        sources
            .mods
            .insert("in-pack".into(), remote::ModSource { version: "1.0.0".into(), auto: true, ..Default::default() });
        sources
            .mods
            .insert("kept".into(), remote::ModSource { version: "1.0.0".into(), auto: false, ..Default::default() });
        sources.mods.insert(
            "the-pack".into(),
            remote::ModSource {
                kind: resolver::MODPACK.into(),
                version: "1.0.0".into(),
                members: vec!["in-pack".into(), "kept".into()],
                ..Default::default()
            },
        );
        remote::write_sources(dir, &sources).unwrap();
        let found: Vec<(String, bool)> = orphans(dir, "the-pack").into_iter().map(|o| (o.id, o.auto)).collect();
        assert!(found.contains(&("in-pack".into(), true)));
        assert!(found.contains(&("kept".into(), false)), "the player's own is asked about");
        let left = remove_with(dir, "rac1", "pal", "the-pack", &["in-pack".into()]).unwrap();
        assert_eq!(left.iter().map(|v| v.id.as_str()).collect::<Vec<_>>(), ["kept"]);
        assert!(!remote::read_sources(dir).mods.contains_key("the-pack"));
    }
}
