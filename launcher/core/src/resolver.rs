//! What installing some mods from the index takes: the change set the page shows before anything
//! is downloaded.
//!
//! For each mod asked for, and each mod those require, it picks one version: the one installed when
//! it already meets every constraint (an install changes as little as it can), else the newest
//! that is not revoked, runs on this game, disc and platform, meets every `requires` constraint
//! (`plugins::meets`, the port's own rule) and fits this launcher's mod interface (the rule of
//! plugins.rs). When the newest needs a newer interface, an older one that fits is picked and a
//! note says so; when none fits but one would on the dev build, the plan offers `SwitchToDev`.
//! Pre-release versions are offered only when the player allows them, or asks for one by version.
//!
//! A pack (`type: pack`) installs nothing itself: it is its `requires`. Conflicts are checked
//! both ways, against the mods that are on and within the plan; a cycle of requires is refused.
//! `install` comes in dependency order, so applying it in order puts every dependency first.

use std::collections::{BTreeMap, BTreeSet};

use serde::Serialize;

use crate::deeplink::WantedMod;
use crate::index::{Asset, IndexMod, ModIndex, Version};
use crate::mod_error::ModAction;
use crate::plugins::{self, compare_versions, meets, parse_api, parse_version};

/// sources.json's kind for a pack of other mods (index `type: pack`); "pack" there is a data pack.
pub const MODPACK: &str = "modpack";

/// A mod the player has, as the resolver needs it.
#[derive(Debug, Clone, PartialEq, Eq, serde::Deserialize, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct InstalledMod {
    pub id: String,
    pub version: String,
    pub enabled: bool,
}

/// Everything a plan depends on.
pub struct Request<'a> {
    pub index: &'a ModIndex,
    pub installed: &'a [InstalledMod],
    pub game: &'a str,
    pub disc: &'a str,
    /// `plugins::platform_key()`: None where no native mod runs.
    pub platform: Option<&'a str>,
    /// This launcher's mod interface (`plugins::API_MAJOR`, `API_MINOR`).
    pub api: (u32, u32),
    pub allow_prerelease: bool,
    pub wanted: &'a [WantedMod],
}

/// One mod of the change set.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct PlannedMod {
    pub id: String,
    pub name: String,
    pub version: String,
    /// The version installed now, if any (different: an update).
    pub installed: Option<String>,
    #[ts(type = "\"script\" | \"native\" | \"data\" | \"pack\"")]
    pub kind: String,
    /// A dependency the player did not ask for (sources.json `auto`).
    pub auto: bool,
    /// The mods that need it, by id.
    pub required_by: Vec<String>,
    pub repo: String,
    pub path: String,
    pub tag: String,
    /// The archive for this platform (or `any`), with the hash the signed index gives it.
    pub asset: Option<Asset>,
    pub impact: Vec<String>,
    pub hooks: Vec<String>,
    /// Installed but off: turned on.
    pub enable: bool,
}

/// A mod offered beside the change set: recommended (ticked) or suggested (not).
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Offer {
    pub id: String,
    pub name: String,
    pub description: String,
    /// Who offers it (a name).
    pub by: String,
}

/// Two mods that cannot both be on.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct PlanConflict {
    pub id: String,
    pub name: String,
    pub with: String,
    pub with_name: String,
    pub reason: String,
    /// `with` is a mod the player has on (not one this plan brings).
    pub installed: bool,
    pub action: Option<ModAction>,
}

#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Missing {
    pub id: String,
    /// The mod that needs it; None: the player asked for it.
    pub required_by: Option<String>,
}

/// Who wants which version of a mod: `by` "" is the player.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Want {
    pub by: String,
    pub version: Option<String>,
}

/// Why no version of a mod can be installed.
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase", tag = "kind")]
pub enum Why {
    NotForGame {
        game: String,
    },
    NotForDisc {
        disc: String,
    },
    NotForPlatform {
        platform: String,
    },
    /// Every version that fits otherwise needs this interface or newer.
    NeedsNewerApi {
        api: String,
    },
    /// No version meets every constraint at once.
    Clash {
        wants: Vec<Want>,
    },
    Cycle {
        ids: Vec<String>,
    },
    Revoked {
        reason: String,
    },
    /// Only pre-release versions, and they are not allowed.
    OnlyPrerelease,
}

#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Incompatible {
    pub id: String,
    pub name: String,
    pub why: Why,
    pub action: Option<ModAction>,
}

/// What the change set says besides the list (worded by the page).
#[derive(Debug, Clone, PartialEq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase", tag = "kind")]
pub enum PlanNote {
    /// The newest needs the dev build's interface: an older one that fits is installed.
    OlderForApi { id: String, name: String, picked: String, newest: String, api: String },
    /// A pack: these are what it installs.
    Pack { id: String, name: String, mods: Vec<String> },
    /// Friends need the same mods to play together.
    Online { id: String, name: String },
    /// Its saves are backed up before it is first turned on.
    TouchesSaves { id: String, name: String },
    /// A newer version was revoked, so this one is installed.
    RevokedSkipped { id: String, name: String, version: String, reason: String },
}

#[derive(Debug, Clone, PartialEq, Serialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct InstallPlan {
    /// To download and install, dependencies first.
    pub install: Vec<PlannedMod>,
    /// Already installed at a version that fits (maybe off: `enable`).
    pub already_ok: Vec<PlannedMod>,
    /// The ids in `install` that replace an installed version.
    pub updates: Vec<String>,
    /// Packs: nothing to download, only recorded (sources.json `modpack`) once their mods are in.
    pub packs: Vec<PlannedMod>,
    pub recommends: Vec<Offer>,
    pub suggests: Vec<Offer>,
    pub conflicts: Vec<PlanConflict>,
    pub missing: Vec<Missing>,
    pub incompatible: Vec<Incompatible>,
    pub notes: Vec<PlanNote>,
    /// Something above stops the install: conflicts, missing or incompatible mods.
    pub blocked: bool,
    /// The one fix for the whole plan, when there is one (`SwitchToDev`).
    pub action: Option<ModAction>,
}

/// The version picked for a mod: one of the index's, or the installed one (maybe not in the index).
#[derive(Debug, Clone, PartialEq, Eq)]
enum Pick {
    Index(usize),
    Installed(String),
}

/// Whether `api` (`MAJOR.MINOR`, or none) loads on a launcher with `have`.
pub fn api_fits(api: Option<&str>, have: (u32, u32)) -> bool {
    match api {
        None => true,
        Some(text) => parse_api(text).is_some_and(|(major, minor)| major == have.0 && minor <= have.1),
    }
}

fn newer(a: &str, b: &str) -> std::cmp::Ordering {
    match (parse_version(a), parse_version(b)) {
        (Some(a), Some(b)) => compare_versions(a, b),
        _ => a.cmp(b),
    }
}

struct Resolver<'a> {
    req: &'a Request<'a>,
    /// The dev build's interface, from the index (`ports.dev.api`).
    dev_api: Option<(u32, u32)>,
    /// The versions asked for by exact version: allowed even when pre-release.
    pinned: BTreeMap<&'a str, &'a str>,
}

impl<'a> Resolver<'a> {
    fn installed(&self, id: &str) -> Option<&'a InstalledMod> {
        self.req.installed.iter().find(|m| m.id == id)
    }

    /// Whether a version runs on this game, disc and platform: why not.
    fn runs_here(&self, m: &IndexMod, v: &Version) -> Result<(), Why> {
        let discs = v.games.get(self.req.game).ok_or(Why::NotForGame { game: self.req.game.into() })?;
        if !discs.iter().any(|d| d == self.req.disc) {
            return Err(Why::NotForDisc { disc: self.req.disc.into() });
        }
        if m.kind == "native" {
            let platform = self.req.platform.unwrap_or_default();
            if !v.platforms.contains_key(platform) {
                return Err(Why::NotForPlatform { platform: platform.into() });
            }
        }
        Ok(())
    }

    fn meets_all(version: &str, wants: &[Want]) -> bool {
        wants.iter().all(|w| meets(version, w.version.as_deref()).unwrap_or(false))
    }

    /// The version to use for `m` under `wants`, or why there is none.
    fn pick(
        &self,
        m: &'a IndexMod,
        wants: &[Want],
        notes: &mut Vec<PlanNote>,
    ) -> Result<Pick, (Why, Option<ModAction>)> {
        let index = self.req.index;
        // The installed version, when it already fits: nothing changes.
        if let Some(have) = self.installed(&m.id) {
            let entry = m.version(&have.version);
            let fits = entry.is_none_or(|v| self.runs_here(m, v).is_ok() && api_fits(v.api.as_deref(), self.req.api));
            if fits && index.revoked(&m.id, &have.version).is_none() && Self::meets_all(&have.version, wants) {
                return Ok(Pick::Installed(have.version.clone()));
            }
        }
        let mut order: Vec<usize> = (0..m.versions.len()).collect();
        order.sort_by(|&a, &b| newer(&m.versions[b].version, &m.versions[a].version));
        let pinned = self.pinned.get(m.id.as_str()).copied();
        // Each filter in turn, so a failure says the first thing that stopped every version.
        let live: Vec<usize> =
            order.iter().copied().filter(|&i| index.revoked(&m.id, &m.versions[i].version).is_none()).collect();
        if live.is_empty() {
            let reason = order.first().and_then(|&i| index.revoked(&m.id, &m.versions[i].version)).unwrap_or_default();
            return Err((Why::Revoked { reason: reason.into() }, None));
        }
        let released: Vec<usize> = live
            .iter()
            .copied()
            .filter(|&i| {
                let v = &m.versions[i];
                !v.prerelease || self.req.allow_prerelease || pinned == Some(v.version.as_str())
            })
            .collect();
        if released.is_empty() {
            return Err((Why::OnlyPrerelease, None));
        }
        let here: Vec<usize> =
            released.iter().copied().filter(|&i| self.runs_here(m, &m.versions[i]).is_ok()).collect();
        if here.is_empty() {
            let why = self.runs_here(m, &m.versions[released[0]]).unwrap_err();
            return Err((why, None));
        }
        let wanted: Vec<usize> =
            here.iter().copied().filter(|&i| Self::meets_all(&m.versions[i].version, wants)).collect();
        if wanted.is_empty() {
            return Err((Why::Clash { wants: wants.to_vec() }, None));
        }
        let Some(&best) = wanted.iter().find(|&&i| api_fits(m.versions[i].api.as_deref(), self.req.api)) else {
            let need = m.versions[wanted[0]].api.clone().unwrap_or_default();
            let on_dev = self.dev_api.is_some_and(|dev| {
                dev > self.req.api && wanted.iter().any(|&i| api_fits(m.versions[i].api.as_deref(), dev))
            });
            return Err((Why::NeedsNewerApi { api: need }, on_dev.then_some(ModAction::SwitchToDev)));
        };
        let newest = &m.versions[wanted[0]];
        if best != wanted[0] {
            notes.push(PlanNote::OlderForApi {
                id: m.id.clone(),
                name: m.name.clone(),
                picked: m.versions[best].version.clone(),
                newest: newest.version.clone(),
                api: newest.api.clone().unwrap_or_default(),
            });
        }
        // A newer version that was revoked: say why this one is installed instead.
        if let Some(&skipped) =
            order.iter().find(|&&i| newer(&m.versions[i].version, &m.versions[best].version).is_gt())
        {
            if let Some(reason) = index.revoked(&m.id, &m.versions[skipped].version) {
                notes.push(PlanNote::RevokedSkipped {
                    id: m.id.clone(),
                    name: m.name.clone(),
                    version: m.versions[skipped].version.clone(),
                    reason: reason.into(),
                });
            }
        }
        Ok(Pick::Index(best))
    }

    fn version_of(&self, id: &str, pick: &Pick) -> Option<&'a Version> {
        let m = self.req.index.get(id)?;
        match pick {
            Pick::Index(i) => m.versions.get(*i),
            Pick::Installed(v) => m.version(v),
        }
    }

    fn picked_version(&self, id: &str, pick: &Pick) -> String {
        match pick {
            Pick::Index(i) => self.req.index.get(id).map(|m| m.versions[*i].version.clone()).unwrap_or_default(),
            Pick::Installed(v) => v.clone(),
        }
    }
}

/// One round: every needed mod picked under the constraints the last round's picks set.
struct Round {
    picks: BTreeMap<String, Pick>,
    failed: BTreeMap<String, (Why, Option<ModAction>)>,
    missing: BTreeMap<String, Option<String>>,
    required_by: BTreeMap<String, BTreeSet<String>>,
    notes: Vec<PlanNote>,
}

pub fn resolve(req: &Request) -> InstallPlan {
    let r = Resolver {
        req,
        dev_api: parse_api(&req.index.ports.dev.api),
        pinned: req.wanted.iter().filter_map(|w| Some((w.id.as_str(), w.version.as_deref()?))).collect(),
    };
    let roots: Vec<&str> = req.wanted.iter().map(|w| w.id.as_str()).collect();
    let base: BTreeMap<String, Vec<Want>> = req
        .wanted
        .iter()
        .map(|w| (w.id.clone(), vec![Want { by: String::new(), version: w.version.as_ref().map(|v| format!("={v}")) }]))
        .collect();

    // Picks set constraints, which may change picks: rounds until nothing changes.
    let mut last: BTreeMap<String, Pick> = BTreeMap::new();
    let mut round = None;
    for _ in 0..64 {
        let mut wants = base.clone();
        for (id, pick) in &last {
            for q in r.version_of(id, pick).map(|v| v.requires.as_slice()).unwrap_or_default() {
                wants.entry(q.id.clone()).or_default().push(Want { by: id.clone(), version: q.version.clone() });
            }
        }
        let mut now = Round {
            picks: BTreeMap::new(),
            failed: BTreeMap::new(),
            missing: BTreeMap::new(),
            required_by: BTreeMap::new(),
            notes: Vec::new(),
        };
        let mut stack: Vec<(String, Option<String>)> = roots.iter().rev().map(|id| (id.to_string(), None)).collect();
        let mut seen = BTreeSet::new();
        while let Some((id, by)) = stack.pop() {
            if let Some(by) = &by {
                now.required_by.entry(id.clone()).or_default().insert(by.clone());
            }
            if !seen.insert(id.clone()) {
                continue;
            }
            let my_wants = wants.get(&id).cloned().unwrap_or_default();
            let Some(m) = req.index.get(&id) else {
                // Not in the index: an installed copy that meets the constraints still does.
                match r.installed(&id) {
                    Some(have) if Resolver::meets_all(&have.version, &my_wants) => {
                        now.picks.insert(id, Pick::Installed(have.version.clone()));
                    }
                    _ => {
                        now.missing.insert(id, by);
                    }
                }
                continue;
            };
            match r.pick(m, &my_wants, &mut now.notes) {
                Ok(pick) => {
                    if let Some(v) = r.version_of(&id, &pick) {
                        for q in v.requires.iter().rev() {
                            stack.push((q.id.clone(), Some(id.clone())));
                        }
                    }
                    now.picks.insert(id, pick);
                }
                Err(why) => {
                    now.failed.insert(id, why);
                }
            }
        }
        let done = now.picks == last;
        last = now.picks.clone();
        round = Some(now);
        if done {
            break;
        }
    }
    let mut round = round.expect("at least one round");
    // The last word: every constraint the final picks set, checked against them. Rounds that never
    // settle (picks that keep changing each other) end here as a clash, never as a wrong plan.
    let mut wants = base;
    for (id, pick) in &round.picks {
        for q in r.version_of(id, pick).map(|v| v.requires.as_slice()).unwrap_or_default() {
            wants.entry(q.id.clone()).or_default().push(Want { by: id.clone(), version: q.version.clone() });
        }
    }
    let broken: Vec<String> = round
        .picks
        .iter()
        .filter(|(id, pick)| {
            !Resolver::meets_all(&r.picked_version(id, pick), wants.get(*id).map(Vec::as_slice).unwrap_or_default())
        })
        .map(|(id, _)| id.clone())
        .collect();
    for id in broken {
        round.picks.remove(&id);
        let mine = wants.remove(&id).unwrap_or_default();
        round.failed.entry(id).or_insert((Why::Clash { wants: mine }, None));
    }
    build(&r, round)
}

/// The cycle through `start` in the requires among `picks`, if there is one.
fn cycle(r: &Resolver, picks: &BTreeMap<String, Pick>) -> Option<Vec<String>> {
    fn visit(
        r: &Resolver,
        picks: &BTreeMap<String, Pick>,
        id: &str,
        path: &mut Vec<String>,
        done: &mut BTreeSet<String>,
    ) -> Option<Vec<String>> {
        if let Some(at) = path.iter().position(|p| p == id) {
            return Some(path[at..].to_vec());
        }
        if done.contains(id) {
            return None;
        }
        path.push(id.to_string());
        if let Some(v) = picks.get(id).and_then(|p| r.version_of(id, p)) {
            for q in &v.requires {
                if picks.contains_key(&q.id) {
                    if let Some(c) = visit(r, picks, &q.id, path, done) {
                        return Some(c);
                    }
                }
            }
        }
        path.pop();
        done.insert(id.to_string());
        None
    }
    let mut done = BTreeSet::new();
    picks.keys().find_map(|id| visit(r, picks, id, &mut Vec::new(), &mut done))
}

/// Dependencies first, then the mods that need them; ties by the order they were asked for.
fn dependency_order(r: &Resolver, picks: &BTreeMap<String, Pick>, roots: &[&str]) -> Vec<String> {
    fn visit(
        r: &Resolver,
        picks: &BTreeMap<String, Pick>,
        id: &str,
        out: &mut Vec<String>,
        seen: &mut BTreeSet<String>,
    ) {
        if !seen.insert(id.to_string()) {
            return;
        }
        if let Some(v) = picks.get(id).and_then(|p| r.version_of(id, p)) {
            for q in &v.requires {
                if picks.contains_key(&q.id) {
                    visit(r, picks, &q.id, out, seen);
                }
            }
        }
        out.push(id.to_string());
    }
    let mut out = Vec::new();
    let mut seen = BTreeSet::new();
    for id in roots.iter().copied().chain(picks.keys().map(String::as_str)) {
        if picks.contains_key(id) {
            visit(r, picks, id, &mut out, &mut seen);
        }
    }
    out
}

fn build(r: &Resolver, round: Round) -> InstallPlan {
    let req = r.req;
    let index = req.index;
    let roots: Vec<&str> = req.wanted.iter().map(|w| w.id.as_str()).collect();
    let mut plan = InstallPlan { notes: round.notes, ..Default::default() };
    let name_of =
        |id: &str| index.get(id).map_or(id.to_string(), |m| if m.name.is_empty() { id.into() } else { m.name.clone() });

    for (id, (why, action)) in round.failed {
        plan.incompatible.push(Incompatible { name: name_of(&id), id, why, action });
    }
    for (id, required_by) in round.missing {
        plan.missing.push(Missing { id, required_by });
    }
    if let Some(ids) = cycle(r, &round.picks) {
        plan.incompatible.push(Incompatible {
            id: ids[0].clone(),
            name: name_of(&ids[0]),
            why: Why::Cycle { ids },
            action: None,
        });
    }

    let order = dependency_order(r, &round.picks, &roots);
    for id in &order {
        let pick = &round.picks[id];
        let version = r.picked_version(id, pick);
        let have = r.installed(id);
        let m = index.get(id);
        let v = r.version_of(id, pick);
        let kind = m.map_or("script".to_string(), |m| m.kind.clone());
        let asset = v.and_then(|v| match kind.as_str() {
            "native" => req.platform.and_then(|p| v.platforms.get(p)).cloned(),
            _ => v.platforms.get("any").cloned(),
        });
        let planned = PlannedMod {
            id: id.clone(),
            name: name_of(id),
            version: version.clone(),
            installed: have.map(|h| h.version.clone()),
            kind,
            // A dependency, or a mod a pack the player asked for brings.
            auto: !roots.contains(&id.as_str()),
            required_by: round.required_by.get(id).map(|s| s.iter().cloned().collect()).unwrap_or_default(),
            repo: m.map(|m| m.repo.clone()).unwrap_or_default(),
            path: m.map(|m| m.path.clone()).unwrap_or_default(),
            tag: v.map(|v| v.tag.clone()).unwrap_or_default(),
            asset,
            impact: m.map(|m| m.impact.clone()).unwrap_or_default(),
            hooks: v.map(|v| v.hooks.clone()).unwrap_or_default(),
            enable: have.is_some_and(|h| !h.enabled),
        };
        if planned.kind == "pack" {
            let mods: Vec<String> = v.map(|v| v.requires.iter().map(|q| q.id.clone()).collect()).unwrap_or_default();
            plan.notes.push(PlanNote::Pack { id: id.clone(), name: name_of(id), mods });
            if have.is_none_or(|h| h.version != version) {
                plan.packs.push(planned);
            }
            continue;
        }
        if have.is_some_and(|h| h.version == version) {
            plan.already_ok.push(planned);
        } else {
            if have.is_some() {
                plan.updates.push(id.clone());
            }
            if let Some(m) = m {
                if m.has_impact("online") {
                    plan.notes.push(PlanNote::Online { id: id.clone(), name: m.name.clone() });
                }
                if m.has_impact("touches-saves") && have.is_none() {
                    plan.notes.push(PlanNote::TouchesSaves { id: id.clone(), name: m.name.clone() });
                }
            }
            plan.install.push(planned);
        }
    }

    conflicts(r, &round.picks, &mut plan);
    offers(r, &round.picks, &mut plan);

    plan.blocked = !plan.conflicts.is_empty() || !plan.missing.is_empty() || !plan.incompatible.is_empty();
    plan.action = plan.incompatible.iter().find_map(|i| i.action.clone().filter(|a| *a == ModAction::SwitchToDev));
    plan
}

/// Conflicts both ways: what the plan brings against the mods that are on, and within the plan.
/// Two mods that were both on before are the player's business, not this change set's.
fn conflicts(r: &Resolver, picks: &BTreeMap<String, Pick>, plan: &mut InstallPlan) {
    let index = r.req.index;
    let changed: BTreeSet<&str> =
        plan.install.iter().chain(plan.already_ok.iter().filter(|m| m.enable)).map(|m| m.id.as_str()).collect();
    // Every mod that would be on afterwards, with the conflicts its version declares.
    let mut on: BTreeMap<String, Vec<crate::index::Conflict>> = BTreeMap::new();
    for (id, pick) in picks {
        on.insert(id.clone(), r.version_of(id, pick).map(|v| v.conflicts.clone()).unwrap_or_default());
    }
    for m in r.req.installed.iter().filter(|m| m.enabled && !picks.contains_key(&m.id)) {
        let declared = index.get(&m.id).and_then(|e| e.version(&m.version)).map(|v| v.conflicts.clone());
        on.insert(m.id.clone(), declared.unwrap_or_default());
    }
    let name_of = |id: &str| index.get(id).map_or(id.to_string(), |m| m.name.clone());
    let mut seen = BTreeSet::new();
    for (a, declared) in &on {
        for c in declared {
            let b = &c.id;
            if !on.contains_key(b) || a == b {
                continue;
            }
            if !changed.contains(a.as_str()) && !changed.contains(b.as_str()) {
                continue;
            }
            let pair = if a < b { (a.clone(), b.clone()) } else { (b.clone(), a.clone()) };
            if !seen.insert(pair) {
                continue;
            }
            // Said from the side of the mod the plan brings.
            let (id, with) = if changed.contains(a.as_str()) { (a, b) } else { (b, a) };
            let installed = !picks.contains_key(with);
            plan.conflicts.push(PlanConflict {
                id: id.clone(),
                name: name_of(id),
                with: with.clone(),
                with_name: name_of(with),
                reason: c.reason.clone(),
                installed,
                action: installed.then(|| ModAction::ReplaceConflict { replace: with.clone(), with: id.clone() }),
            });
        }
    }
}

/// What the new mods recommend and suggest that is not in the plan or installed, and could be.
fn offers(r: &Resolver, picks: &BTreeMap<String, Pick>, plan: &mut InstallPlan) {
    let index = r.req.index;
    let mut offered = BTreeSet::new();
    let new: Vec<&PlannedMod> = plan.install.iter().collect();
    let can = |id: &str| {
        index.get(id).is_some_and(|m| m.kind != "pack" && r.pick(m, &[], &mut Vec::new()).is_ok())
            && !picks.contains_key(id)
            && r.installed(id).is_none()
    };
    for pm in new {
        let Some(m) = index.get(&pm.id) else { continue };
        for (ids, into_recommends) in [(&m.recommends, true), (&m.suggests, false)] {
            for id in ids {
                if !can(id) || !offered.insert(id.clone()) {
                    continue;
                }
                let o = index.get(id).expect("checked");
                let offer = Offer {
                    id: id.clone(),
                    name: o.name.clone(),
                    description: o.description.clone(),
                    by: m.name.clone(),
                };
                if into_recommends {
                    plan.recommends.push(offer);
                } else {
                    plan.suggests.push(offer);
                }
            }
        }
    }
}

/// The resolver's inputs from a game's plugins folder: its mods and the packs sources.json lists.
pub fn installed_mods(dir: &std::path::Path, game: &str, disc: &str) -> Vec<InstalledMod> {
    let mut out: Vec<InstalledMod> = plugins::list(dir, game, disc)
        .map(|views| {
            views
                .into_iter()
                .filter(|v| v.kind != "built-in")
                .map(|v| InstalledMod { id: v.id, version: v.version, enabled: v.enabled })
                .collect()
        })
        .unwrap_or_default();
    for (id, s) in crate::remote::read_sources(dir).mods {
        // Data packs (overrides) and mod packs live outside the plugins' list.
        if (s.kind == "pack" || s.kind == MODPACK) && !out.iter().any(|m| m.id == id) {
            out.push(InstalledMod { id, version: s.version, enabled: true });
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::index::tests::fixture;
    use crate::index::{Conflict, Require};

    const STABLE: (u32, u32) = (1, 10);
    const DEV: (u32, u32) = (1, 11);

    fn want(id: &str) -> WantedMod {
        WantedMod { id: id.into(), version: None }
    }

    fn have(id: &str, version: &str, enabled: bool) -> InstalledMod {
        InstalledMod { id: id.into(), version: version.into(), enabled }
    }

    fn plan_with(
        index: &ModIndex,
        installed: &[InstalledMod],
        api: (u32, u32),
        pre: bool,
        wanted: &[WantedMod],
    ) -> InstallPlan {
        resolve(&Request {
            index,
            installed,
            game: "rac1",
            disc: "pal",
            platform: Some("linux-x86_64"),
            api,
            allow_prerelease: pre,
            wanted,
        })
    }

    fn ids(mods: &[PlannedMod]) -> Vec<(&str, &str)> {
        mods.iter().map(|m| (m.id.as_str(), m.version.as_str())).collect()
    }

    #[test]
    fn a_chain_installs_dependencies_first() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("lock-strafe")]);
        assert!(!plan.blocked, "{plan:#?}");
        assert_eq!(
            ids(&plan.install),
            [("camera-base", "1.2.0"), ("camera-tweaks", "2.0.0"), ("lock-strafe", "1.2.0")]
        );
        assert!(plan.install[0].auto && plan.install[1].auto && !plan.install[2].auto);
        assert_eq!(plan.install[1].required_by, ["lock-strafe"]);
        // Its archive for this platform, with the signed hash.
        let asset = plan.install[2].asset.as_ref().unwrap();
        assert_eq!(asset.asset, "lock-strafe-1.2.0-linux-x86_64.zip");
        assert_eq!(asset.sha256.len(), 64);
    }

    #[test]
    fn recommends_are_offered_ticked_and_suggests_unticked() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("lock-strafe")]);
        // lock-strafe recommends camera-tweaks (already in the plan) and suggests xbox-icons;
        // camera-tweaks recommends xbox-icons.
        let rec: Vec<&str> = plan.recommends.iter().map(|o| o.id.as_str()).collect();
        assert_eq!(rec, ["xbox-icons"]);
        assert!(plan.suggests.is_empty(), "offered once, as recommended");
        let plan = plan_with(&index, &[], STABLE, false, &[want("lock-strafe"), want("xbox-icons")]);
        assert!(plan.recommends.is_empty() && plan.suggests.is_empty());
    }

    #[test]
    fn installed_versions_that_fit_are_kept() {
        let index = fixture();
        let installed = [have("camera-base", "1.1.0", true)];
        let plan = plan_with(&index, &installed, STABLE, false, &[want("camera-tweaks")]);
        assert_eq!(ids(&plan.install), [("camera-tweaks", "2.0.0")]);
        assert_eq!(ids(&plan.already_ok), [("camera-base", "1.1.0")]);
        assert!(plan.updates.is_empty());
        // Off: kept, and turned on.
        let plan = plan_with(&index, &[have("camera-base", "1.1.0", false)], STABLE, false, &[want("camera-tweaks")]);
        assert!(plan.already_ok[0].enable);
    }

    #[test]
    fn an_update_is_planned_when_the_installed_one_is_too_old() {
        let index = fixture();
        let installed = [have("camera-tweaks", "1.0.0", true)];
        let plan = plan_with(&index, &installed, STABLE, false, &[want("lock-strafe")]);
        assert_eq!(plan.updates, ["camera-tweaks"]);
        let tweaks = plan.install.iter().find(|m| m.id == "camera-tweaks").unwrap();
        assert_eq!((tweaks.version.as_str(), tweaks.installed.as_deref()), ("2.0.0", Some("1.0.0")));
    }

    #[test]
    fn a_diamond_meets_both_sides() {
        let mut index = fixture();
        // top needs left and right; left wants base >=1.1.0, right base =1.2.0.
        let base = index.get("camera-base").unwrap().clone();
        let mk = |id: &str, requires: Vec<Require>| IndexMod {
            id: id.into(),
            name: id.into(),
            kind: "script".into(),
            versions: vec![Version {
                version: "1.0.0".into(),
                api: Some("1.1".into()),
                games: base.versions[0].games.clone(),
                requires,
                ..Default::default()
            }],
            ..Default::default()
        };
        let req = |id: &str, v: &str| Require { id: id.into(), version: Some(v.into()) };
        index.mods.push(mk("left", vec![req("camera-base", ">=1.1.0")]));
        index.mods.push(mk("right", vec![req("camera-base", "=1.1.0")]));
        index.mods.push(mk("top", vec![req("left", ">=1.0.0"), req("right", ">=1.0.0")]));
        let plan = plan_with(&index, &[], STABLE, false, &[want("top")]);
        assert!(!plan.blocked, "{plan:#?}");
        assert_eq!(plan.install.iter().find(|m| m.id == "camera-base").unwrap().version, "1.1.0");
        let pos = |id: &str| plan.install.iter().position(|m| m.id == id).unwrap();
        assert!(pos("camera-base") < pos("left") && pos("left") < pos("top") && pos("right") < pos("top"));

        // Two constraints that cannot both be met.
        index.mods.push(mk("other", vec![req("camera-base", "=1.2.0")]));
        let plan = plan_with(&index, &[], STABLE, false, &[want("right"), want("other")]);
        assert!(plan.blocked);
        let clash = plan.incompatible.iter().find(|i| i.id == "camera-base").unwrap();
        let Why::Clash { wants } = &clash.why else { panic!("{clash:?}") };
        let by: BTreeSet<&str> = wants.iter().map(|w| w.by.as_str()).collect();
        assert_eq!(by, ["other", "right"].into());
    }

    #[test]
    fn conflicts_are_found_both_ways() {
        let index = fixture();
        // lock-strafe declares the conflict; classic-controls is on.
        let on = [have("classic-controls", "1.0.0", true)];
        let plan = plan_with(&index, &on, STABLE, false, &[want("lock-strafe")]);
        assert!(plan.blocked);
        let c = &plan.conflicts[0];
        assert_eq!((c.id.as_str(), c.with.as_str(), c.installed), ("lock-strafe", "classic-controls", true));
        assert_eq!(
            c.action,
            Some(ModAction::ReplaceConflict { replace: "classic-controls".into(), with: "lock-strafe".into() })
        );
        // The other way: classic-controls asked for while lock-strafe is on.
        let on = [
            have("lock-strafe", "1.2.0", true),
            have("camera-tweaks", "2.0.0", true),
            have("camera-base", "1.2.0", true),
        ];
        let plan = plan_with(&index, &on, STABLE, false, &[want("classic-controls")]);
        assert_eq!(plan.conflicts.len(), 1);
        assert_eq!(plan.conflicts[0].with, "lock-strafe");
        // Off, it is no conflict.
        let off = [have("classic-controls", "1.0.0", false)];
        assert!(!plan_with(&index, &off, STABLE, false, &[want("lock-strafe")]).blocked);
        // Within the plan.
        let plan = plan_with(&index, &[], STABLE, false, &[want("lock-strafe"), want("classic-controls")]);
        assert_eq!(plan.conflicts.len(), 1);
        assert!(!plan.conflicts[0].installed && plan.conflicts[0].action.is_none());
    }

    #[test]
    fn missing_mods_and_wrong_game_disc_or_platform_are_reported() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("no-such-mod")]);
        assert_eq!(plan.missing, [Missing { id: "no-such-mod".into(), required_by: None }]);
        assert!(plan.blocked);

        let mut index = fixture();
        index.mods.iter_mut().find(|m| m.id == "camera-base").unwrap().versions.iter_mut().for_each(|v| {
            v.games.insert("rac1".into(), vec!["ntsc".into()]);
        });
        let plan = plan_with(&index, &[], STABLE, false, &[want("camera-tweaks")]);
        assert_eq!(plan.incompatible[0].why, Why::NotForDisc { disc: "pal".into() });

        let index = fixture();
        let plan = resolve(&Request {
            index: &index,
            installed: &[],
            game: "rac2",
            disc: "pal",
            platform: Some("linux-x86_64"),
            api: STABLE,
            allow_prerelease: false,
            wanted: &[want("camera-base")],
        });
        assert_eq!(plan.incompatible[0].why, Why::NotForGame { game: "rac2".into() });

        let plan = resolve(&Request {
            index: &index,
            installed: &[],
            game: "rac1",
            disc: "pal",
            platform: Some("android-arm64"),
            api: STABLE,
            allow_prerelease: false,
            wanted: &[want("lock-strafe")],
        });
        assert_eq!(plan.incompatible[0].why, Why::NotForPlatform { platform: "android-arm64".into() });
        // A script mod runs everywhere.
        assert!(!resolve(&Request { platform: None, wanted: &[want("camera-base")], ..req_like(&index) }).blocked);
    }

    fn req_like(index: &ModIndex) -> Request<'_> {
        Request {
            index,
            installed: &[],
            game: "rac1",
            disc: "pal",
            platform: Some("linux-x86_64"),
            api: STABLE,
            allow_prerelease: false,
            wanted: &[],
        }
    }

    #[test]
    fn a_cycle_is_refused() {
        let mut index = fixture();
        let base = index.get("camera-base").unwrap().clone();
        let mut a = base.clone();
        a.id = "cyc-a".into();
        a.versions.truncate(1);
        a.versions[0].requires = vec![Require { id: "cyc-b".into(), version: None }];
        let mut b = a.clone();
        b.id = "cyc-b".into();
        b.versions[0].requires = vec![Require { id: "cyc-a".into(), version: None }];
        index.mods.extend([a, b]);
        let plan = plan_with(&index, &[], STABLE, false, &[want("cyc-a")]);
        assert!(plan.blocked);
        assert!(plan.incompatible.iter().any(|i| matches!(&i.why, Why::Cycle { ids } if ids.len() == 2)));
    }

    #[test]
    fn stable_picks_the_older_version_that_fits_and_says_so() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("item-randomizer")]);
        assert_eq!(ids(&plan.install), [("item-randomizer", "1.3.0")]);
        assert!(plan
            .notes
            .iter()
            .any(|n| matches!(n, PlanNote::OlderForApi { newest, api, .. } if newest == "1.4.0" && api == "1.11")));
        // The dev build gets the newest; pre-release only with the setting on.
        assert_eq!(
            ids(&plan_with(&index, &[], DEV, false, &[want("item-randomizer")]).install),
            [("item-randomizer", "1.4.0")]
        );
        assert_eq!(
            ids(&plan_with(&index, &[], DEV, true, &[want("item-randomizer")]).install),
            [("item-randomizer", "2.0.0-beta.1")]
        );
        // Asked for by version, a pre-release is allowed.
        let exact = [WantedMod { id: "item-randomizer".into(), version: Some("2.0.0-beta.1".into()) }];
        assert_eq!(ids(&plan_with(&index, &[], DEV, false, &exact).install), [("item-randomizer", "2.0.0-beta.1")]);
    }

    #[test]
    fn nothing_fitting_offers_the_dev_build() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("archipelago")]);
        assert!(plan.blocked);
        assert_eq!(plan.action, Some(ModAction::SwitchToDev));
        assert!(matches!(&plan.incompatible[0].why, Why::NeedsNewerApi { api } if api == "1.11"));
        // On the dev build it resolves, with its dependency at the version it needs.
        let plan = plan_with(&index, &[], DEV, false, &[want("archipelago")]);
        assert!(!plan.blocked, "{plan:#?}");
        assert_eq!(ids(&plan.install), [("item-randomizer", "1.4.0"), ("archipelago", "0.3.0")]);
        // Already on the dev build and still too new: no switch offered.
        let mut newer = index.clone();
        newer.ports.dev.api = "1.10".into();
        assert_eq!(plan_with(&newer, &[], STABLE, false, &[want("archipelago")]).action, None);
    }

    #[test]
    fn a_pack_is_its_requires() {
        let index = fixture();
        let plan = plan_with(&index, &[], DEV, false, &[want("randomizer-pack")]);
        assert!(!plan.blocked, "{plan:#?}");
        let got: BTreeSet<&str> = plan.install.iter().map(|m| m.id.as_str()).collect();
        assert_eq!(got, ["camera-base", "camera-tweaks", "item-randomizer", "xbox-icons"].into());
        assert!(plan.install.iter().all(|m| m.auto && m.kind != "pack"));
        assert!(plan
            .notes
            .iter()
            .any(|n| matches!(n, PlanNote::Pack { id, mods, .. } if id == "randomizer-pack" && mods.len() == 3)));
        assert_eq!(ids(&plan.packs), [("randomizer-pack", "1.0.0")]);
        // Recorded: next time it is already there, and nothing is to do.
        let have_all: Vec<InstalledMod> =
            plan.install.iter().chain(&plan.packs).map(|m| have(&m.id, &m.version, true)).collect();
        let again = plan_with(&index, &have_all, DEV, false, &[want("randomizer-pack")]);
        assert!(again.install.is_empty() && again.packs.is_empty());
        // On stable its item-randomizer >=1.4.0 does not fit: switch to dev.
        assert_eq!(
            plan_with(&index, &[], STABLE, false, &[want("randomizer-pack")]).action,
            Some(ModAction::SwitchToDev)
        );
    }

    #[test]
    fn a_revoked_version_is_never_chosen() {
        let index = fixture();
        let exact = [WantedMod { id: "ammo-max".into(), version: Some("1.0.0".into()) }];
        let plan = plan_with(&index, &[], STABLE, false, &exact);
        assert!(plan.blocked);
        assert!(matches!(&plan.incompatible[0].why, Why::Clash { .. }));
        let plan = plan_with(&index, &[], STABLE, false, &[want("ammo-max")]);
        assert_eq!(ids(&plan.install), [("ammo-max", "1.0.1")]);
        // Installed at the revoked version: replaced, not kept.
        let plan = plan_with(&index, &[have("ammo-max", "1.0.0", true)], STABLE, false, &[want("ammo-max")]);
        assert_eq!(plan.updates, ["ammo-max"]);
        // Its impact is said.
        assert!(plan_with(&index, &[], STABLE, false, &[want("ammo-max")])
            .notes
            .iter()
            .any(|n| matches!(n, PlanNote::TouchesSaves { id, .. } if id == "ammo-max")));
        // Every version revoked.
        let mut all = index.clone();
        all.revoked[0].versions = vec!["*".into()];
        assert!(matches!(
            plan_with(&all, &[], STABLE, false, &[want("ammo-max")]).incompatible[0].why,
            Why::Revoked { .. }
        ));
    }

    #[test]
    fn pinned_exact_versions_hold_for_a_shared_set() {
        let index = fixture();
        let set = [
            WantedMod { id: "camera-base".into(), version: Some("1.1.0".into()) },
            WantedMod { id: "camera-tweaks".into(), version: Some("2.0.0".into()) },
        ];
        let plan = plan_with(&index, &[have("camera-base", "1.2.0", true)], STABLE, false, &set);
        assert!(!plan.blocked);
        assert_eq!(ids(&plan.install), [("camera-base", "1.1.0"), ("camera-tweaks", "2.0.0")]);
    }

    #[test]
    fn data_mods_and_online_mods_are_planned() {
        let index = fixture();
        let plan = plan_with(&index, &[], STABLE, false, &[want("xbox-icons")]);
        assert_eq!(ids(&plan.install), [("xbox-icons", "1.0.3")]);
        assert!(plan.install[0].asset.is_none(), "no archive: read from the repository");
        let plan = plan_with(&index, &[], DEV, false, &[want("archipelago")]);
        assert!(plan.notes.iter().any(|n| matches!(n, PlanNote::Online { .. })));
    }

    // ---- The property: for random small graphs, a plan that is not blocked is in dependency
    // order, meets every constraint and holds no conflict.

    use proptest::prelude::*;

    #[derive(Debug, Clone)]
    struct GenVersion {
        major: u64,
        api_minor: u32,
        requires: Vec<(usize, u8, u64)>,
        conflicts: Vec<usize>,
    }

    /// Versions per mod, the installed mods (index, major, on) and the picks.
    type GenIndex = (Vec<Vec<GenVersion>>, Vec<(usize, u64, bool)>, Vec<usize>);

    fn gen_index() -> impl Strategy<Value = GenIndex> {
        let version = (
            1u64..4,
            0u32..13,
            prop::collection::vec((0usize..6, 0u8..3, 1u64..4), 0..3),
            prop::collection::vec(0usize..6, 0..2),
        )
            .prop_map(|(major, api_minor, requires, conflicts)| GenVersion {
                major,
                api_minor,
                requires,
                conflicts,
            });
        let mods = prop::collection::vec(prop::collection::vec(version, 1..4), 2..7);
        mods.prop_flat_map(|mods| {
            let n = mods.len();
            let installed = prop::collection::vec((0..n, 1u64..4, any::<bool>()), 0..3);
            let wanted = prop::collection::vec(0..n, 1..3);
            (Just(mods), installed, wanted)
        })
    }

    fn build_index(mods: &[Vec<GenVersion>]) -> ModIndex {
        let n = mods.len();
        let id = |i: usize| format!("m{}", i % n);
        let mut index = ModIndex { format: 1, ..Default::default() };
        for (i, versions) in mods.iter().enumerate() {
            let mut list: Vec<Version> = Vec::new();
            for v in versions {
                let text = format!("{}.0.0", v.major);
                if list.iter().any(|o| o.version == text) {
                    continue;
                }
                list.push(Version {
                    version: text,
                    api: Some(format!("1.{}", v.api_minor)),
                    games: [("rac1".to_string(), vec!["pal".to_string()])].into(),
                    requires: v
                        .requires
                        .iter()
                        .filter(|(j, _, _)| j % n != i)
                        .map(|(j, op, at)| Require {
                            id: id(*j),
                            version: match op {
                                0 => None,
                                1 => Some(format!(">={at}.0.0")),
                                _ => Some(format!("={at}.0.0")),
                            },
                        })
                        .collect(),
                    conflicts: v
                        .conflicts
                        .iter()
                        .filter(|j| *j % n != i)
                        .map(|j| Conflict { id: id(*j), reason: String::new() })
                        .collect(),
                    tag: "t".into(),
                    ..Default::default()
                });
            }
            list.sort_by(|a, b| newer(&b.version, &a.version));
            index.mods.push(IndexMod {
                id: id(i),
                name: id(i),
                kind: "script".into(),
                versions: list,
                ..Default::default()
            });
        }
        index
    }

    proptest! {
        #![proptest_config(ProptestConfig { cases: 400, ..ProptestConfig::default() })]
        #[test]
        fn plans_are_ordered_satisfied_and_conflict_free((mods, installed, wanted) in gen_index()) {
            let index = build_index(&mods);
            let n = mods.len();
            let mut inst: Vec<InstalledMod> = Vec::new();
            for (i, major, on) in installed {
                if !inst.iter().any(|m| m.id == format!("m{}", i % n)) {
                    inst.push(have(&format!("m{}", i % n), &format!("{major}.0.0"), on));
                }
            }
            let mut want_list: Vec<WantedMod> = Vec::new();
            for i in wanted {
                if !want_list.iter().any(|w| w.id == format!("m{}", i % n)) {
                    want_list.push(want(&format!("m{}", i % n)));
                }
            }
            let plan = plan_with(&index, &inst, STABLE, false, &want_list);
            if plan.blocked {
                return Ok(());
            }
            // The version each mod ends at.
            let mut at: BTreeMap<String, String> = inst.iter().map(|m| (m.id.clone(), m.version.clone())).collect();
            for m in plan.install.iter().chain(&plan.already_ok) {
                at.insert(m.id.clone(), m.version.clone());
            }
            let pos: BTreeMap<&str, usize> = plan.install.iter().enumerate().map(|(i, m)| (m.id.as_str(), i)).collect();
            for (i, m) in plan.install.iter().chain(&plan.already_ok).enumerate() {
                let Some(v) = index.get(&m.id).and_then(|e| e.version(&m.version)) else { continue };
                prop_assert!(api_fits(v.api.as_deref(), STABLE));
                for q in &v.requires {
                    let got = at.get(&q.id);
                    prop_assert!(got.is_some(), "{} needs {} which is not there", m.id, q.id);
                    prop_assert!(meets(got.unwrap(), q.version.as_deref()).unwrap(), "{} needs {} {:?}, has {:?}", m.id, q.id, q.version, got);
                    if i < plan.install.len() {
                        if let Some(&j) = pos.get(q.id.as_str()) {
                            prop_assert!(j < i, "{} is installed before its dependency {}", m.id, q.id);
                        }
                    }
                }
            }
            // No conflict touching anything the plan installs or turns on.
            let on: BTreeSet<&str> = inst.iter().filter(|m| m.enabled).map(|m| m.id.as_str())
                .chain(plan.install.iter().chain(&plan.already_ok).map(|m| m.id.as_str())).collect();
            for m in &plan.install {
                let v = index.get(&m.id).and_then(|e| e.version(&m.version)).unwrap();
                for c in &v.conflicts {
                    prop_assert!(!on.contains(c.id.as_str()), "{} conflicts with {}, which is on", m.id, c.id);
                }
                for other in &on {
                    let Some(ov) = at.get(*other).and_then(|ver| index.get(other).and_then(|e| e.version(ver))) else { continue };
                    prop_assert!(!ov.conflicts.iter().any(|c| c.id == m.id), "{} conflicts with {}", other, m.id);
                }
            }
        }
    }
}
