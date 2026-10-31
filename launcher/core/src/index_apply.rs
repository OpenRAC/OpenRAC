//! Applying a change set (resolver.rs): each mod installed in order through `remote::install`, so
//! the release's SHA256SUMS and the manifest are checked as for a link, and the archive's hash
//! from the signed index is checked as well. Dependencies are marked `auto` in sources.json; every
//! mod ends up on and in its place (plugins.rs places a mod after what it needs).
//!
//! It stops at the first failure and keeps what was installed before it: the result says which
//! were done and why the next one failed, for "Installed 2 of 3".
//!
//! Before a mod that `touches-saves` is first turned on, the game's memory card is copied to
//! `<data>/saves.backup/<date>/<serial>/` (saves.rs's data folder), so a bad save can be undone.

use std::path::{Path, PathBuf};

use serde::Serialize;

use crate::mod_error::{ModAction, ModError, ModErrorKind};
use crate::plugins;
use crate::remote::{self, RemoteInstalled};
use crate::resolver::{InstallPlan, PlannedMod};

/// What applying needs besides the plan.
pub struct Options<'a> {
    /// The disc's serial (`SCES-50916`): whose saves to back up. None: no backup.
    pub serial: Option<&'a str>,
    /// OpenRAC's data folder (`saves::data_home()`), where memory cards and backups are.
    pub data_home: &'a Path,
    /// Mods to turn off first: the ones a conflict's "Replace" chose.
    pub turn_off: &'a [String],
    /// Mods asked for at an exact version by a shared setup's link: kept at it ("pinned by a
    /// shared setup") until the player lets them update.
    pub pin_shared: &'a [String],
    /// An update set: one failure puts back every mod this run changed (auto-update).
    pub all_or_nothing: bool,
}

/// A data pack installed: the page adds it to the overrides (actions.json `overrides-add`), as for
/// a link (`RemoteInstalled::Pack`).
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct AppliedPack {
    pub id: String,
    pub version: String,
    pub path: String,
}

/// How far applying got, mod by mod, for the progress line.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ApplyProgress {
    pub id: String,
    /// `installing`, `done` or `failed`.
    #[ts(type = "\"installing\" | \"done\" | \"failed\"")]
    pub step: String,
    /// Which one this is (from 0) of `total`.
    pub at: usize,
    pub total: usize,
}

#[derive(Debug, Clone, PartialEq, Serialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ApplyResult {
    /// The ids installed (or turned on), in order.
    pub done: Vec<String>,
    pub packs: Vec<AppliedPack>,
    /// The mod that failed and why; the rest were not tried.
    pub failed: Option<ModError>,
    /// Not tried after the failure.
    pub skipped: Vec<String>,
    /// Installed but could not be turned on: id and why.
    pub not_on: Vec<(String, String)>,
    /// Where the saves were backed up, if they were.
    pub backup: Option<String>,
    /// `all_or_nothing` and one failed: the mods done before it were put back as they were.
    pub rolled_back: bool,
}

/// The address `remote::install` reads a planned mod from: its release by tag when the index lists
/// an archive, else the repository's files at the tag or commit (a script or data mod without one).
pub fn source_url(m: &PlannedMod) -> String {
    let repo = m.repo.trim_end_matches('/');
    let has_archive = m.asset.is_some() || m.kind == "native";
    match (has_archive, m.path.is_empty()) {
        (true, _) => format!("{repo}/releases/tag/{}", m.tag),
        (false, true) => format!("{repo}/tree/{}", m.tag),
        (false, false) => format!("{repo}/tree/{}/{}", m.tag, m.path),
    }
}

/// `YYYY-MM-DD-HHMMSS` in UTC, for the backup's folder.
fn stamp(secs: u64) -> String {
    let days = (secs / 86_400) as i64;
    let rem = secs % 86_400;
    // Howard Hinnant's civil_from_days.
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + i64::from(m <= 2);
    format!("{y:04}-{m:02}-{d:02}-{:02}{:02}{:02}", rem / 3600, rem % 3600 / 60, rem % 60)
}

fn copy_tree(from: &Path, to: &Path) -> Result<(), String> {
    std::fs::create_dir_all(to).map_err(|e| format!("{}: {e}", to.display()))?;
    for entry in std::fs::read_dir(from).map_err(|e| format!("{}: {e}", from.display()))?.flatten() {
        let (path, target) = (entry.path(), to.join(entry.file_name()));
        let kind = entry.file_type().map_err(|e| e.to_string())?;
        if kind.is_dir() {
            copy_tree(&path, &target)?;
        } else if kind.is_file() {
            std::fs::copy(&path, &target).map_err(|e| format!("{}: {e}", path.display()))?;
        }
    }
    Ok(())
}

/// Copies the memory card of `serial` into `<data>/saves.backup/<date>/<serial>/`. None: there
/// are no saves yet.
pub fn backup_saves(data_home: &Path, serial: &str, now: u64) -> Result<Option<PathBuf>, String> {
    if serial.is_empty() || serial.contains(['/', '\\']) || serial.contains("..") {
        return Err(format!("{serial:?} is not a disc's serial"));
    }
    let card = data_home.join("memcard").join(serial);
    if !card.is_dir() {
        return Ok(None);
    }
    let base = data_home.join("saves.backup").join(stamp(now));
    let mut target = base.join(serial);
    let mut n = 2;
    while target.exists() {
        target = base.join(format!("{serial}-{n}"));
        n += 1;
    }
    copy_tree(&card, &target)?;
    Ok(Some(target))
}

/// Applies `plan` to the plugins folder `ctx.dir`. `progress` hears each step.
pub fn apply(
    ctx: &remote::Context,
    plan: &InstallPlan,
    options: &Options,
    mut progress: impl FnMut(ApplyProgress),
) -> Result<ApplyResult, ModError> {
    if plan.blocked {
        return Err(ModError::new(ModErrorKind::UserFixable, "This change set has something to fix first."));
    }
    let (game, disc) = (ctx.game, ctx.version);
    let mut result = ApplyResult::default();
    for id in options.turn_off {
        plugins::set_enabled(ctx.dir, game, disc, id, false)
            .map_err(|e| ModError::new(ModErrorKind::UserFixable, e).for_mod(id).with(ModAction::Retry))?;
    }
    // Installed ones that only need turning on go first: they are dependencies of the rest.
    for m in plan.already_ok.iter().filter(|m| m.enable) {
        match plugins::set_enabled(ctx.dir, game, disc, &m.id, true) {
            Ok(_) => result.done.push(m.id.clone()),
            Err(e) => result.not_on.push((m.id.clone(), e)),
        }
    }
    let total = plan.install.len();
    let had: Vec<String> = plugins::list(ctx.dir, game, disc).unwrap_or_default().into_iter().map(|v| v.id).collect();
    for (at, m) in plan.install.iter().enumerate() {
        progress(ApplyProgress { id: m.id.clone(), step: "installing".into(), at, total });
        let step = (|| -> Result<RemoteInstalled, ModError> {
            let first_time = !had.contains(&m.id);
            if first_time && result.backup.is_none() && m.impact.iter().any(|i| i == "touches-saves") {
                if let Some(serial) = options.serial {
                    let saved = backup_saves(options.data_home, serial, crate::index::now()).map_err(|e| {
                        ModError::new(ModErrorKind::UserFixable, format!("The saves could not be backed up: {e}"))
                            .with(ModAction::Retry)
                    })?;
                    result.backup = saved.map(|p| p.display().to_string());
                }
            }
            let installed =
                remote::install_checked(ctx, &source_url(m), &m.id, m.asset.as_ref()).map_err(ModError::from)?;
            let version = match &installed {
                RemoteInstalled::Plugin { version, .. } | RemoteInstalled::Pack { version, .. } => version,
            };
            if *version != m.version {
                return Err(ModError::new(
                    ModErrorKind::UpstreamBad,
                    format!("The release holds {} {version}, not {} as the mod list says.", m.id, m.version),
                ));
            }
            Ok(installed)
        })();
        let installed = match step {
            Ok(i) => i,
            Err(e) => {
                progress(ApplyProgress { id: m.id.clone(), step: "failed".into(), at, total });
                let e = e.for_mod(&m.id);
                // A bad hash or signature is never retried: it is reported.
                let e = if e.kind == ModErrorKind::UpstreamBad || e.action.is_some() {
                    e
                } else {
                    e.with(ModAction::Retry)
                };
                crate::launcher_log::line(&format!("mod install {}: {:?} {}", m.id, e.kind, e.detail));
                result.failed = Some(e);
                result.skipped = plan.install[at + 1..].iter().map(|m| m.id.clone()).collect();
                if options.all_or_nothing {
                    undo(ctx, &had, &mut result);
                }
                return Ok(result);
            }
        };
        // A dependency stays `auto` only while the player never asked for it themselves.
        let mut sources = remote::read_sources(ctx.dir);
        if let Some(s) = sources.mods.get_mut(&m.id) {
            let was_auto = s.auto || !had.contains(&m.id);
            s.auto = m.auto && was_auto;
            if options.pin_shared.contains(&m.id) {
                s.pin = true;
                s.pin_reason = Some("shared".into());
            }
            let _ = remote::write_sources(ctx.dir, &sources);
        }
        match installed {
            // A new mod goes on; an update keeps the switch the player left it at.
            RemoteInstalled::Plugin { enabled: false, .. } if !had.contains(&m.id) => {
                if let Err(e) = plugins::set_enabled(ctx.dir, game, disc, &m.id, true) {
                    result.not_on.push((m.id.clone(), e));
                }
            }
            RemoteInstalled::Plugin { .. } => {}
            RemoteInstalled::Pack { id, version, path } => result.packs.push(AppliedPack { id, version, path }),
        }
        result.done.push(m.id.clone());
        progress(ApplyProgress { id: m.id.clone(), step: "done".into(), at, total });
    }
    // Every mod of a pack is in: the pack is recorded, so a later removal knows what it brought.
    if !plan.packs.is_empty() {
        let mut sources = remote::read_sources(ctx.dir);
        for p in &plan.packs {
            sources.mods.insert(
                p.id.clone(),
                remote::ModSource {
                    url: p.repo.clone(),
                    path: p.path.clone(),
                    version: p.version.clone(),
                    kind: crate::resolver::MODPACK.into(),
                    auto: p.auto,
                    // What it brought: the planned mods that are there because of it.
                    members: plan
                        .install
                        .iter()
                        .chain(&plan.already_ok)
                        .filter(|m| m.required_by.contains(&p.id))
                        .map(|m| m.id.clone())
                        .collect(),
                    ..Default::default()
                },
            );
            result.done.push(p.id.clone());
        }
        remote::write_sources(ctx.dir, &sources).map_err(|e| ModError::new(ModErrorKind::Bug, e))?;
    }
    Ok(result)
}

/// Puts back what an update set changed before it failed: each replaced mod rolled back to the
/// version kept in `.previous`, each new one removed.
fn undo(ctx: &remote::Context, had: &[String], result: &mut ApplyResult) {
    for id in result.done.iter().rev() {
        let back = if had.contains(id) {
            plugins::roll_back(ctx.dir, ctx.game, ctx.version, id).map(|_| ())
        } else {
            plugins::remove(ctx.dir, ctx.game, ctx.version, id).and_then(|_| remote::forget(ctx.dir, id))
        };
        if let Err(e) = back {
            crate::launcher_log::line(&format!("mod update {id}: could not be put back: {e}"));
        }
    }
    result.done.clear();
    result.rolled_back = true;
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::index::Asset;
    use std::cell::RefCell;
    use std::collections::HashMap;
    use std::io::Write;

    /// GitHub, answered from memory.
    #[derive(Default)]
    struct Hub {
        files: HashMap<String, Vec<u8>>,
        asked: RefCell<Vec<String>>,
    }

    impl remote::Fetch for Hub {
        fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
            self.asked.borrow_mut().push(url.to_string());
            self.files.get(url).cloned().ok_or_else(|| format!("{url}: not found"))
        }
    }

    const API: &str = "https://api.test";
    const RAW: &str = "https://raw.test";
    const OWNER_REPO: &str = "OpenRAC/openrac-mods";

    fn zip_of(files: &[(&str, &str)]) -> Vec<u8> {
        let mut buf = std::io::Cursor::new(Vec::new());
        let mut w = zip::ZipWriter::new(&mut buf);
        for (name, text) in files {
            w.start_file(*name, zip::write::SimpleFileOptions::default()).unwrap();
            w.write_all(text.as_bytes()).unwrap();
        }
        w.finish().unwrap();
        buf.into_inner()
    }

    fn sha(bytes: &[u8]) -> String {
        use sha2::Digest;
        format!("{:x}", sha2::Sha256::digest(bytes))
    }

    fn manifest(id: &str, version: &str, requires: &[&str]) -> String {
        let req: Vec<String> = requires.iter().map(|r| format!(r#"{{"id":"{r}"}}"#)).collect();
        format!(
            r#"{{"format":2,"id":"{id}","name":"{id}","version":"{version}","author":"a","type":"script","api":"1.1",
            "games":{{"rac1":["pal"]}},"scripts":["main.luau"],"requires":[{}]}}"#,
            req.join(",")
        )
    }

    /// A script mod `id` released as `<id>-v<version>` with its joined archive and SHA256SUMS; the
    /// planned mod pointing at it.
    fn release(hub: &mut Hub, id: &str, version: &str, requires: &[&str], swap: bool) -> PlannedMod {
        let tag = format!("{id}-v{version}");
        let folder = format!("{id}-{version}");
        let man = manifest(id, version, requires);
        let zip =
            zip_of(&[(&format!("{folder}/openrac-mod.json"), &man), (&format!("{folder}/main.luau"), "print(1)")]);
        let asset = format!("{id}-{version}.zip");
        let base = format!("{API}/repos/{OWNER_REPO}");
        let dl = |name: &str| format!("https://dl.test/{tag}/{name}");
        let served = if swap {
            zip_of(&[(&format!("{folder}/openrac-mod.json"), &man), (&format!("{folder}/main.luau"), "evil()")])
        } else {
            zip.clone()
        };
        let mut releases: Vec<serde_json::Value> = hub
            .files
            .get(&format!("{base}/releases?per_page=100"))
            .map(|b| serde_json::from_slice(b).unwrap())
            .unwrap_or_default();
        releases.push(serde_json::json!({
            "tag_name": tag,
            "assets": [{"name": asset, "browser_download_url": dl(&asset)}, {"name": "SHA256SUMS", "browser_download_url": dl("SHA256SUMS")}]
        }));
        hub.files.insert(format!("{base}/releases?per_page=100"), serde_json::to_vec(&releases).unwrap());
        hub.files.insert(format!("{base}/commits/{tag}"), format!(r#"{{"sha":"c-{tag}"}}"#).into_bytes());
        hub.files.insert(
            format!("{base}/git/trees/c-{tag}?recursive=1"),
            format!(r#"{{"tree":[{{"type":"blob","path":"{id}/openrac-mod.json"}}]}}"#).into_bytes(),
        );
        hub.files.insert(format!("{RAW}/{OWNER_REPO}/c-{tag}/{id}/openrac-mod.json"), man.into_bytes());
        hub.files.insert(dl(&asset), served.clone());
        // The release's own sums match what is served: only the index can tell it was swapped.
        hub.files.insert(dl("SHA256SUMS"), format!("{}  {asset}\n", sha(&served)).into_bytes());
        PlannedMod {
            id: id.into(),
            name: id.into(),
            version: version.into(),
            installed: None,
            kind: "script".into(),
            auto: false,
            required_by: vec![],
            repo: format!("https://github.com/{OWNER_REPO}"),
            path: id.into(),
            tag,
            asset: Some(Asset { asset, sha256: sha(&zip), size: zip.len() as u64 }),
            impact: vec![],
            hooks: vec![],
            enable: false,
        }
    }

    fn ctx<'a>(dir: &'a Path, hub: &'a Hub) -> remote::Context<'a> {
        remote::Context {
            dir,
            game: "rac1",
            version: "pal",
            platform: Some("linux-x86_64"),
            fetch: hub,
            hosts: remote::Hosts { api: API.into(), raw: RAW.into() },
        }
    }

    fn opts<'a>(data: &'a Path, serial: Option<&'a str>) -> Options<'a> {
        Options { serial, data_home: data, turn_off: &[], pin_shared: &[], all_or_nothing: false }
    }

    #[test]
    fn the_address_is_the_release_or_the_repository_at_the_tag() {
        let mut m = PlannedMod {
            id: "x".into(),
            name: "x".into(),
            version: "1.0.0".into(),
            installed: None,
            kind: "native".into(),
            auto: false,
            required_by: vec![],
            repo: "https://github.com/o/r/".into(),
            path: "x".into(),
            tag: "x-v1.0.0".into(),
            asset: None,
            impact: vec![],
            hooks: vec![],
            enable: false,
        };
        assert_eq!(source_url(&m), "https://github.com/o/r/releases/tag/x-v1.0.0");
        m.kind = "data".into();
        assert_eq!(source_url(&m), "https://github.com/o/r/tree/x-v1.0.0/x");
        m.path = String::new();
        assert_eq!(source_url(&m), "https://github.com/o/r/tree/x-v1.0.0");
        // The address remote.rs reads a release's tag from.
        m.asset = Some(Asset::default());
        assert!(
            matches!(remote::parse_url(&source_url(&m)), Ok(remote::Source::GitHub { tag: Some(t), .. }) if t == "x-v1.0.0")
        );
    }

    #[test]
    fn installs_in_order_marks_dependencies_auto_and_turns_them_on() {
        let dir = tempfile::tempdir().unwrap();
        let data = tempfile::tempdir().unwrap();
        let mut hub = Hub::default();
        let mut base = release(&mut hub, "base-mod", "1.0.0", &[], false);
        base.auto = true;
        let top = release(&mut hub, "top-mod", "1.0.0", &["base-mod"], false);
        let plan = InstallPlan { install: vec![base, top], ..Default::default() };
        let mut steps = Vec::new();
        let result =
            apply(&ctx(dir.path(), &hub), &plan, &opts(data.path(), None), |p| steps.push((p.id, p.step))).unwrap();
        assert_eq!(result.done, ["base-mod", "top-mod"]);
        assert!(result.failed.is_none());
        let sources = remote::read_sources(dir.path());
        assert!(sources.mods["base-mod"].auto && !sources.mods["top-mod"].auto);
        let views = plugins::list(dir.path(), "rac1", "pal").unwrap();
        let order: Vec<(&str, bool)> = views.iter().map(|v| (v.id.as_str(), v.enabled)).collect();
        assert_eq!(order, [("base-mod", true), ("top-mod", true)]);
        assert_eq!(steps.len(), 4);
        // Asked for itself later: no longer auto.
        let mut again = release(&mut Hub::default(), "base-mod", "1.0.0", &[], false);
        again.auto = false;
        let result = apply(
            &ctx(dir.path(), &hub),
            &InstallPlan { install: vec![again], ..Default::default() },
            &opts(data.path(), None),
            |_| {},
        )
        .unwrap();
        assert!(result.failed.is_none());
        assert!(!remote::read_sources(dir.path()).mods["base-mod"].auto);
    }

    #[test]
    fn stops_at_the_first_failure_and_keeps_what_was_installed() {
        let dir = tempfile::tempdir().unwrap();
        let data = tempfile::tempdir().unwrap();
        let mut hub = Hub::default();
        let a = release(&mut hub, "aaa", "1.0.0", &[], false);
        // Swapped after it was indexed: SHA256SUMS matches, the index does not.
        let b = release(&mut hub, "bbb", "1.0.0", &[], true);
        let c = release(&mut hub, "ccc", "1.0.0", &[], false);
        let plan = InstallPlan { install: vec![a, b, c], ..Default::default() };
        let result = apply(&ctx(dir.path(), &hub), &plan, &opts(data.path(), None), |_| {}).unwrap();
        assert_eq!(result.done, ["aaa"]);
        assert_eq!(result.skipped, ["ccc"]);
        let failed = result.failed.unwrap();
        assert_eq!((failed.kind, failed.mod_id.as_deref()), (ModErrorKind::UpstreamBad, Some("bbb")));
        assert_eq!(failed.action, None, "a bad hash is not retried");
        let ids: Vec<String> = plugins::list(dir.path(), "rac1", "pal").unwrap().into_iter().map(|v| v.id).collect();
        assert_eq!(ids, ["aaa"]);
    }

    #[test]
    fn a_pack_is_recorded_once_its_mods_are_in() {
        let dir = tempfile::tempdir().unwrap();
        let mut hub = Hub::default();
        let inner = release(&mut hub, "inner-mod", "1.0.0", &[], false);
        let mut pack = inner.clone();
        (pack.id, pack.kind, pack.asset, pack.tag) = ("the-pack".into(), "pack".into(), None, String::new());
        let plan = InstallPlan { install: vec![inner], packs: vec![pack], ..Default::default() };
        let result = apply(&ctx(dir.path(), &hub), &plan, &opts(dir.path(), None), |_| {}).unwrap();
        assert_eq!(result.done, ["inner-mod", "the-pack"]);
        let s = remote::read_sources(dir.path());
        assert_eq!((s.mods["the-pack"].kind.as_str(), s.mods["the-pack"].version.as_str()), ("modpack", "1.0.0"));
        let installed = crate::resolver::installed_mods(dir.path(), "rac1", "pal");
        assert!(installed.iter().any(|m| m.id == "the-pack"));
    }

    #[test]
    fn an_update_set_is_all_or_nothing() {
        let dir = tempfile::tempdir().unwrap();
        let data = tempfile::tempdir().unwrap();
        let mut hub = Hub::default();
        let a1 = release(&mut hub, "aaa", "1.0.0", &[], false);
        let b1 = release(&mut hub, "bbb", "1.0.0", &[], false);
        let first = InstallPlan { install: vec![a1, b1], ..Default::default() };
        assert!(apply(&ctx(dir.path(), &hub), &first, &opts(data.path(), None), |_| {}).unwrap().failed.is_none());
        // Both update; bbb's archive was swapped after it was listed.
        let a2 = release(&mut hub, "aaa", "1.1.0", &[], false);
        let b2 = release(&mut hub, "bbb", "1.1.0", &[], true);
        let set = InstallPlan { install: vec![a2, b2], ..Default::default() };
        let options = Options { all_or_nothing: true, ..opts(data.path(), None) };
        let result = apply(&ctx(dir.path(), &hub), &set, &options, |_| {}).unwrap();
        assert!(result.failed.is_some() && result.rolled_back && result.done.is_empty());
        let versions: Vec<(String, String)> =
            plugins::list(dir.path(), "rac1", "pal").unwrap().into_iter().map(|v| (v.id, v.version)).collect();
        assert_eq!(versions, [("aaa".into(), "1.0.0".into()), ("bbb".into(), "1.0.0".into())]);
    }

    #[test]
    fn exact_versions_from_a_shared_setup_are_pinned() {
        let dir = tempfile::tempdir().unwrap();
        let mut hub = Hub::default();
        let a = release(&mut hub, "aaa", "1.0.0", &[], false);
        let plan = InstallPlan { install: vec![a], ..Default::default() };
        let pin = ["aaa".to_string()];
        let options = Options { pin_shared: &pin, ..opts(dir.path(), None) };
        apply(&ctx(dir.path(), &hub), &plan, &options, |_| {}).unwrap();
        let s = &remote::read_sources(dir.path()).mods["aaa"];
        assert!(s.pin && s.pin_reason.as_deref() == Some("shared"));
        // Let it update again: the reason goes with the pin.
        let s = remote::set_pin(dir.path(), "aaa", false).unwrap();
        assert_eq!(s.mods["aaa"].pin_reason, None);
    }

    #[test]
    fn a_blocked_plan_is_refused() {
        let dir = tempfile::tempdir().unwrap();
        let hub = Hub::default();
        let plan = InstallPlan { blocked: true, ..Default::default() };
        assert_eq!(
            apply(&ctx(dir.path(), &hub), &plan, &opts(dir.path(), None), |_| {}).unwrap_err().kind,
            ModErrorKind::UserFixable
        );
        assert!(hub.asked.borrow().is_empty());
    }

    #[test]
    fn saves_are_backed_up_before_a_touches_saves_mod_is_first_on() {
        let dir = tempfile::tempdir().unwrap();
        let data = tempfile::tempdir().unwrap();
        let card = data.path().join("memcard/SCES-50916/BESCES-50916RATCHET");
        std::fs::create_dir_all(&card).unwrap();
        std::fs::write(card.join("save0.bin"), b"my game").unwrap();
        let mut hub = Hub::default();
        let mut m = release(&mut hub, "ammo-max", "1.0.1", &[], false);
        m.impact = vec!["touches-saves".into()];
        let plan = InstallPlan { install: vec![m.clone()], ..Default::default() };
        let result = apply(&ctx(dir.path(), &hub), &plan, &opts(data.path(), Some("SCES-50916")), |_| {}).unwrap();
        let backup = PathBuf::from(result.backup.expect("backed up"));
        assert!(backup.starts_with(data.path().join("saves.backup")));
        assert_eq!(std::fs::read(backup.join("BESCES-50916RATCHET/save0.bin")).unwrap(), b"my game");
        // Installed already: no second backup.
        let result = apply(&ctx(dir.path(), &hub), &plan, &opts(data.path(), Some("SCES-50916")), |_| {}).unwrap();
        assert!(result.backup.is_none());
        assert!(backup_saves(data.path(), "../x", 0).is_err());
    }

    #[test]
    fn backup_folders_are_named_by_date() {
        assert_eq!(stamp(0), "1970-01-01-000000");
        assert_eq!(stamp(1_793_440_800), "2026-10-31-100000");
        assert_eq!(stamp(951_827_415), "2000-02-29-123015");
    }
}
