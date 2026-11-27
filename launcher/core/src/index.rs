//! The mod index: one signed `index.json` per mods repository (format 1, openrac-mods
//! `INDEX_FORMAT.md`), the list the Browse view, the change set and the resolver read.
//!
//! Each index is fetched with its minisign signature and checked with the launcher's built-in key
//! ([`INDEX_PUBLIC_KEY`], through `update::verify`) before anything in it is used. The highest
//! `serial` seen is kept, and a lower one is refused (an old index served again). A good copy is
//! kept in the launcher's config folder and used offline, or when a fetch fails; past `expires`
//! it is "stale", which is said quietly and blocks nothing. Several indexes are merged into one
//! (ids are unique across them).
//!
//! Unknown fields are ignored, so a newer index still reads. An index of a newer `format` is not
//! used: the cache is kept and the page asks for a newer launcher.

use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

use base64::Engine;
use serde::{Deserialize, Serialize};
use serde_json::{Map, Value};

use crate::mod_error::{ModAction, ModError, ModErrorKind};
use crate::remote::Fetch;

/// The index format this launcher reads.
pub const FORMAT: u32 = 1;

/// Where the indexes are, in order: the first wins when two list the same id. Settings may name
/// others (`Config::mod_indexes`).
pub const INDEXES: &[&str] = &["https://github.com/OpenRAC/openrac-mods/releases/download/index/index.json", COMMUNITY];

/// New Not checked mods, signed with the same key, with a serial of their own. It is published
/// only while there are such mods, so a failed fetch of it is not an error.
pub const COMMUNITY: &str = "https://github.com/OpenRAC/openrac-mods/releases/download/index/community.json";

/// The public half of the key openrac-mods' index.yml signs index.json with (minisign key id
/// 5466767758D8C8D1): base64 of its .pub file, as the updater's `pubkey` is.
pub const INDEX_PUBLIC_KEY: &str = "dW50cnVzdGVkIGNvbW1lbnQ6IG1pbmlzaWduIHB1YmxpYyBrZXkgNTQ2Njc2Nzc1OEQ4QzhEMQpSV1RSeU5oWWQzWm1WRDZEN2Jwd2d4MGdrYlZ6THJlUDc4dm5LeHRMVkp6UWZ6aGFnM0piWDFaYwo=";

/// The cache folder's name, beside launcher.json.
pub const CACHE_DIR: &str = "mod-index";

// ---- The format (INDEX_FORMAT.md) ----------------------------------------------------------------

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexPort")]
pub struct Port {
    #[serde(default)]
    pub launcher: String,
    #[serde(default)]
    pub api: String,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexPorts")]
pub struct Ports {
    #[serde(default)]
    pub stable: Port,
    #[serde(default)]
    pub dev: Port,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexImage")]
pub struct Image {
    pub hash: String,
    #[serde(default)]
    pub widths: Vec<u32>,
    #[serde(default)]
    pub width: u32,
    #[serde(default)]
    pub height: u32,
    #[serde(default)]
    pub thumbhash: Option<String>,
    #[serde(default)]
    pub alt: Option<String>,
    /// Screenshots only.
    #[serde(default)]
    pub caption: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexVideo")]
pub struct Video {
    pub youtube: String,
    #[serde(default)]
    pub title: String,
    #[serde(default)]
    pub caption: String,
    #[serde(default)]
    pub uploaded: String,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexMedia")]
pub struct Media {
    #[serde(default)]
    pub icon: Option<Image>,
    #[serde(default)]
    pub banner: Option<Image>,
    #[serde(default)]
    pub screenshots: Vec<Image>,
    #[serde(default)]
    pub videos: Vec<Video>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexRequire")]
pub struct Require {
    pub id: String,
    /// `>=X.Y.Z`, `=X.Y.Z`, or none.
    #[serde(default)]
    pub version: Option<String>,
    /// A pack's values for this mod's settings, by key (`{"camera": "rac2"}`): applied when the
    /// pack is installed or updated, never over a value the player chose (index_apply.rs).
    #[serde(default, skip_serializing_if = "Map::is_empty")]
    #[ts(optional, type = "Record<string, unknown>")]
    pub settings: Map<String, Value>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexConflict")]
pub struct Conflict {
    pub id: String,
    #[serde(default)]
    pub reason: String,
}

/// One archive of a version: for a platform (native) or `any` (script).
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexAsset")]
pub struct Asset {
    pub asset: String,
    pub sha256: String,
    #[serde(default)]
    pub size: u64,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexVersion")]
pub struct Version {
    pub version: String,
    /// The mod interface `MAJOR.MINOR`; none for data and packs.
    #[serde(default)]
    pub api: Option<String>,
    #[serde(default)]
    pub games: BTreeMap<String, Vec<String>>,
    #[serde(default)]
    pub requires: Vec<Require>,
    #[serde(default)]
    pub conflicts: Vec<Conflict>,
    #[serde(default)]
    pub hooks: Vec<String>,
    /// The release's git tag ("" for packs); for a script mod with no archive, the commit.
    #[serde(default)]
    pub tag: String,
    #[serde(default)]
    pub released: String,
    #[serde(default)]
    pub prerelease: bool,
    #[serde(default)]
    pub changelog: String,
    #[serde(default)]
    pub size: u64,
    #[serde(default)]
    pub files: Vec<String>,
    #[serde(default)]
    pub platforms: BTreeMap<String, Asset>,
    /// What the scan of a native version's files found it can reach (none: not scanned).
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub scan: Option<Scan>,
    /// The newest native version only: "ci" when the mod's own CI built it from its tag, else "hand".
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub built: Option<String>,
}

/// The scan of a native version (openrac-mods tools/binary_scan.py): what it can reach, and what
/// of that the version before could not.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexScan")]
pub struct Scan {
    /// Some of: Network, Starts programs, Loads code, Executable memory, Clipboard.
    #[serde(default)]
    pub labels: Vec<String>,
    #[serde(default)]
    pub new: Vec<String>,
}

/// A mod whose newest version the scan held for a person to look at: hidden from new installs.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexHeld")]
pub struct Held {
    pub version: String,
    #[serde(default)]
    pub reasons: Vec<String>,
}

/// The GitHub account a mod's repository belongs to; `id` stays when the account is renamed.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexOwner")]
pub struct Owner {
    pub login: String,
    #[serde(default)]
    #[ts(type = "number | null")]
    pub id: Option<u64>,
}

/// The file with a mod's long texts (readme, changelogs), named with its SHA-256.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexDetailsFile")]
pub struct DetailsFile {
    pub asset: String,
    pub sha256: String,
}

/// The weekly picks: up to three mod ids per game.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexPicks")]
pub struct Picks {
    #[serde(default)]
    pub week: String,
    #[serde(default)]
    pub games: BTreeMap<String, Vec<String>>,
}

/// The labels, in order of how much OpenRAC checked a mod.
pub const TRUST: &[&str] = &["openrac", "verified", "trusted", "unchecked"];

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct IndexMod {
    pub id: String,
    #[serde(default)]
    pub name: String,
    #[serde(default)]
    pub author: String,
    #[serde(default)]
    pub description: String,
    #[serde(default)]
    pub license: Option<String>,
    #[serde(default)]
    pub homepage: Option<String>,
    #[serde(default)]
    pub repo: String,
    #[serde(default)]
    pub path: String,
    /// "script", "native", "data" or "pack" (a set of other mods).
    #[serde(default, rename = "type")]
    #[ts(rename = "type", type = "\"script\" | \"native\" | \"data\" | \"pack\"")]
    pub kind: String,
    #[serde(default)]
    pub games: BTreeMap<String, Vec<String>>,
    #[serde(default)]
    pub tags: Vec<String>,
    /// Some of: cosmetic, gameplay, speedrun-safe, touches-saves, online.
    #[serde(default)]
    pub impact: Vec<String>,
    #[serde(default)]
    pub aliases: Vec<String>,
    #[serde(default)]
    pub links: BTreeMap<String, String>,
    #[serde(default)]
    pub downloads: u64,
    #[serde(default)]
    pub downloads7d: u64,
    /// The author says AI helped make it. Older indexes leave it out.
    #[serde(default)]
    pub ai_assisted: bool,
    #[serde(default)]
    pub created: String,
    #[serde(default)]
    pub updated: String,
    #[serde(default)]
    pub featured: bool,
    #[serde(default)]
    pub recommends: Vec<String>,
    #[serde(default)]
    pub suggests: Vec<String>,
    #[serde(default)]
    pub readme: String,
    /// The GitHub Discussion for this mod in the index repository, once the bot made it.
    #[serde(default)]
    pub discussion: Option<u64>,
    #[serde(default)]
    pub media: Media,
    /// Newest first.
    #[serde(default)]
    pub versions: Vec<Version>,
    /// How much OpenRAC checked it: "openrac", "verified", "trusted" or "unchecked". Older
    /// indexes leave it out ("").
    #[serde(default)]
    #[ts(type = "\"openrac\" | \"verified\" | \"trusted\" | \"unchecked\" | \"\"")]
    pub trust: String,
    /// The owner of an outside mod's repository.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub owner: Option<Owner>,
    /// Its newest version is held for review: not offered to new installs.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub held: Option<Held>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub details: Option<DetailsFile>,
    /// Not in the file: the media base URL of the index this mod came from, set when indexes
    /// are merged, so each mod's pictures load from its own index.
    #[serde(default)]
    pub media_base: String,
    /// Not in the file: the folder URL of the index this mod came from (its details file is there).
    #[serde(default)]
    pub index_base: String,
}

impl IndexMod {
    pub fn version(&self, version: &str) -> Option<&Version> {
        self.versions.iter().find(|v| v.version == version)
    }

    pub fn has_impact(&self, impact: &str) -> bool {
        self.impact.iter().any(|i| i == impact)
    }

    /// Whether OpenRAC checked it in some way (OpenRAC, Verified or Trusted author).
    pub fn checked(&self) -> bool {
        matches!(self.trust.as_str(), "openrac" | "verified" | "trusted")
    }

    /// Who the player trusts when they install a Not checked mod: the owner's login, else the author.
    pub fn trust_key(&self) -> String {
        self.owner.as_ref().map_or_else(|| self.author.clone(), |o| o.login.clone()).to_lowercase()
    }

    /// Where to report a problem: its bug tracker, else its repository's issues.
    pub fn bugs_url(&self) -> String {
        self.links.get("bugs").cloned().unwrap_or_else(|| format!("{}/issues", self.repo.trim_end_matches('/')))
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexRevoked")]
pub struct Revoked {
    pub id: String,
    /// `["*"]`: every version.
    #[serde(default)]
    pub versions: Vec<String>,
    #[serde(default)]
    pub reason: String,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModIndex {
    pub format: u32,
    #[serde(default)]
    pub serial: u64,
    #[serde(default)]
    pub generated: String,
    #[serde(default)]
    pub expires: String,
    #[serde(default)]
    pub source: String,
    #[serde(default)]
    pub media: String,
    #[serde(default)]
    pub ports: Ports,
    #[serde(default)]
    pub tags: Vec<String>,
    #[serde(default)]
    pub mods: Vec<IndexMod>,
    #[serde(default)]
    pub revoked: Vec<Revoked>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub picks: Option<Picks>,
}

impl ModIndex {
    pub fn get(&self, id: &str) -> Option<&IndexMod> {
        self.mods.iter().find(|m| m.id == id)
    }

    /// Why `id` `version` was revoked, if it was.
    pub fn revoked(&self, id: &str, version: &str) -> Option<&str> {
        self.revoked
            .iter()
            .find(|r| r.id == id && r.versions.iter().any(|v| v == "*" || v == version))
            .map(|r| r.reason.as_str())
    }
}

/// Several indexes as one: the first listing an id keeps it; revocations from every index count.
pub fn merge(indexes: Vec<ModIndex>) -> ModIndex {
    let mut out = ModIndex { format: FORMAT, ..Default::default() };
    let mut seen = BTreeSet::new();
    let mut tags = BTreeSet::new();
    for (i, index) in indexes.into_iter().enumerate() {
        if i == 0 {
            out.serial = index.serial;
            out.generated = index.generated.clone();
            out.expires = index.expires.clone();
            out.source = index.source.clone();
            out.media = index.media.clone();
            out.ports = index.ports.clone();
            out.tags = index.tags.clone();
            out.picks = index.picks.clone();
        } else if index.expires < out.expires {
            // The merged list is as fresh as its stalest part.
            out.expires = index.expires.clone();
        }
        tags.extend(out.tags.iter().cloned());
        for mut m in index.mods {
            if seen.insert(m.id.clone()) {
                if m.media_base.is_empty() {
                    m.media_base = index.media.clone();
                }
                out.mods.push(m);
            }
        }
        for t in index.tags {
            if tags.insert(t.clone()) {
                out.tags.push(t);
            }
        }
        out.revoked.extend(index.revoked);
    }
    out.mods.sort_by(|a, b| a.id.cmp(&b.id));
    out
}

// ---- Time -------------------------------------------------------------------------------------

/// Seconds since 1970 of an RFC 3339 time in UTC (`2026-11-07T10:00:00Z`); None if it is not one.
pub fn parse_time(text: &str) -> Option<u64> {
    let text = text.trim().strip_suffix('Z')?;
    let (date, time) = text.split_once('T')?;
    let num = |s: &str| (!s.is_empty() && s.chars().all(|c| c.is_ascii_digit())).then(|| s.parse::<i64>().ok())?;
    let d: Vec<&str> = date.split('-').collect();
    let t: Vec<&str> = time.split('.').next()?.split(':').collect();
    let ([y, m, day], [hh, mm, ss]) = (d.as_slice(), t.as_slice()) else { return None };
    let (y, m, day, hh, mm, ss) = (num(y)?, num(m)?, num(day)?, num(hh)?, num(mm)?, num(ss)?);
    if !(1..=12).contains(&m) || !(1..=31).contains(&day) || hh > 23 || mm > 59 || ss > 60 {
        return None;
    }
    // Days from 1970-01-01 (Howard Hinnant's days_from_civil).
    let y = if m <= 2 { y - 1 } else { y };
    let era = y.div_euclid(400);
    let yoe = y - era * 400;
    let mp = (m + 9) % 12;
    let doy = (153 * mp + 2) / 5 + day - 1;
    let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    let days = era * 146_097 + doe - 719_468;
    u64::try_from(days * 86_400 + hh * 3600 + mm * 60 + ss).ok()
}

pub fn now() -> u64 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_or(0, |d| d.as_secs())
}

// ---- Signature --------------------------------------------------------------------------------

/// The signature as `update::verify` takes it: base64 of the minisign text. A `.minisig` may hold
/// the text itself (minisign's own output) or the base64 (`tauri signer sign`); both are read.
fn signature_base64(text: &str) -> String {
    let text = text.trim();
    if text.starts_with("untrusted comment:") {
        base64::engine::general_purpose::STANDARD.encode(format!("{text}\n"))
    } else {
        text.to_string()
    }
}

/// Checks `file` against its signature with `key`; the index is never used without this.
pub fn verify(file: &Path, signature: &str, key: &str) -> Result<(), ModError> {
    crate::update::verify(file, &signature_base64(signature), key).map_err(|e| {
        ModError::new(ModErrorKind::UpstreamBad, format!("The mod list couldn't be verified ({e})."))
            .with(ModAction::UseCached)
    })
}

// ---- Fetching and the cache ---------------------------------------------------------------------

/// What the index commands send the page.
#[derive(Debug, Clone, PartialEq, Serialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct IndexView {
    pub index: ModIndex,
    /// Past `expires`: shown with a quiet "out of date" line, still used.
    pub stale: bool,
    /// At least one index came from the cache because the fetch failed.
    pub offline: bool,
    /// When the oldest copy in use was fetched (seconds since 1970), for "from 3 hours ago".
    pub fetched_at: u64,
    /// An index is of a newer format: the launcher needs an update to read it.
    pub newer_format: bool,
    /// What went wrong per index, while a cache was used instead.
    pub errors: Vec<ModError>,
    /// Installed versions that were revoked and turned off now (index_fetch with a game).
    pub turned_off: Vec<RevokedOff>,
}

/// An installed mod turned off because its version was revoked.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct RevokedOff {
    pub id: String,
    pub name: String,
    pub version: String,
    pub reason: String,
}

/// The highest serial seen per index, kept beside the copies.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
struct State {
    #[serde(default)]
    serials: BTreeMap<String, u64>,
    #[serde(default)]
    fetched: BTreeMap<String, u64>,
}

/// A file name for an index's URL in the cache.
fn cache_name(url: &str) -> String {
    let hash = crate::sha256_hex(url.as_bytes());
    hash[..16].to_string()
}

fn read_state(dir: &Path) -> State {
    std::fs::read(dir.join("state.json")).ok().and_then(|b| serde_json::from_slice(&b).ok()).unwrap_or_default()
}

fn write_state(dir: &Path, state: &State) -> Result<(), String> {
    let text = serde_json::to_string_pretty(state).map_err(|e| e.to_string())?;
    let tmp = dir.join("state.json.tmp");
    std::fs::write(&tmp, text).map_err(|e| format!("{}: {e}", tmp.display()))?;
    std::fs::rename(&tmp, dir.join("state.json")).map_err(|e| e.to_string())
}

/// Reads an index's bytes once its signature is checked: the format and the serial.
fn accept(file: &Path, signature: &str, key: &str, highest: u64) -> Result<ModIndex, ModError> {
    verify(file, signature, key)?;
    let bytes =
        std::fs::read(file).map_err(|e| ModError::new(ModErrorKind::Bug, format!("{}: {e}", file.display())))?;
    // The format first, from a loose read: a newer one may not fit the types at all.
    let format = serde_json::from_slice::<serde_json::Value>(&bytes)
        .ok()
        .and_then(|v| v.get("format").and_then(serde_json::Value::as_u64))
        .unwrap_or(0);
    if format > u64::from(FORMAT) {
        return Err(ModError::new(
            ModErrorKind::UserFixable,
            format!("The mod list is format {format}: update the launcher."),
        )
        .with(ModAction::UseCached));
    }
    let index: ModIndex = serde_json::from_slice(&bytes).map_err(|e| {
        ModError::new(ModErrorKind::UpstreamBad, format!("The mod list cannot be read ({e})."))
            .with(ModAction::UseCached)
    })?;
    if index.serial < highest {
        return Err(ModError::new(
            ModErrorKind::UpstreamBad,
            format!("The mod list is older than one already seen (serial {} < {highest}).", index.serial),
        )
        .with(ModAction::UseCached));
    }
    Ok(index)
}

/// Where the indexes come from, and where their copies go.
pub struct Sources<'a> {
    pub urls: Vec<String>,
    pub fetch: &'a dyn Fetch,
    /// The cache folder (`<config>/mod-index`).
    pub cache: PathBuf,
    /// The public key (base64): [`INDEX_PUBLIC_KEY`], or a test's.
    pub key: &'a str,
    /// Seconds since 1970.
    pub now: u64,
}

/// The URLs to read: the player's own list if there is one, else [`INDEXES`].
pub fn urls(configured: &[String]) -> Vec<String> {
    let mine: Vec<String> =
        configured.iter().map(|u| u.trim().to_string()).filter(|u| u.starts_with("https://")).collect();
    if mine.is_empty() {
        INDEXES.iter().map(|u| u.to_string()).collect()
    } else {
        mine
    }
}

/// One index: fetched and checked, or its cached copy when that fails. Err with no copy.
fn one(src: &Sources, url: &str, state: &mut State) -> Result<(ModIndex, bool, u64, Option<ModError>), ModError> {
    let name = cache_name(url);
    let (json, sig) = (src.cache.join(format!("{name}.json")), src.cache.join(format!("{name}.json.minisig")));
    let highest = state.serials.get(url).copied().unwrap_or(0);
    let fresh = (|| -> Result<ModIndex, ModError> {
        let bytes = src.fetch.get(url, false).map_err(|e| {
            ModError::new(ModErrorKind::Transient, format!("The mod list could not be fetched ({e})."))
                .with(ModAction::UseCached)
        })?;
        let signature = src.fetch.get(&format!("{url}.minisig"), false).map_err(|e| {
            ModError::new(ModErrorKind::UpstreamBad, format!("The mod list has no signature ({e})."))
                .with(ModAction::UseCached)
        })?;
        let signature = String::from_utf8_lossy(&signature).into_owned();
        let tmp = src.cache.join(format!("{name}.json.new"));
        std::fs::write(&tmp, &bytes)
            .map_err(|e| ModError::new(ModErrorKind::Bug, format!("{}: {e}", tmp.display())))?;
        let index = accept(&tmp, &signature, src.key, highest);
        if index.is_err() {
            let _ = std::fs::remove_file(&tmp);
        }
        let index = index?;
        // Kept only once checked: the signature first, so a copy never lacks one.
        std::fs::write(&sig, &signature).map_err(|e| ModError::new(ModErrorKind::Bug, e.to_string()))?;
        std::fs::rename(&tmp, &json).map_err(|e| ModError::new(ModErrorKind::Bug, e.to_string()))?;
        Ok(index)
    })();
    match fresh {
        Ok(index) => {
            state.serials.insert(url.to_string(), index.serial);
            state.fetched.insert(url.to_string(), src.now);
            Ok((index, false, src.now, None))
        }
        Err(error) => {
            // The copy is checked again: a file on disk is not trusted more than one from the net.
            let signature = std::fs::read_to_string(&sig).map_err(|_| error.clone())?;
            let cached = accept(&json, &signature, src.key, 0).map_err(|_| error.clone())?;
            let fetched = state.fetched.get(url).copied().unwrap_or(0);
            Ok((cached, true, fetched, Some(error)))
        }
    }
}

/// Every index, fetched and checked (or cached), merged. Err only when none can be used.
pub fn fetch(src: &Sources) -> Result<IndexView, ModError> {
    std::fs::create_dir_all(&src.cache)
        .map_err(|e| ModError::new(ModErrorKind::Bug, format!("{}: {e}", src.cache.display())))?;
    let mut state = read_state(&src.cache);
    let mut view = IndexView::default();
    let mut got = Vec::new();
    let mut oldest = u64::MAX;
    let mut last_error = None;
    let mut main_ok = false;
    for url in &src.urls {
        let optional = url.ends_with("/community.json");
        match one(src, url, &mut state) {
            Ok((mut index, offline, fetched, error)) => {
                if let Some(e) = error {
                    crate::launcher_log::line(&format!("mod index {url}: {}", e.detail));
                    if !optional {
                        view.newer_format |= e.detail.contains("update the launcher");
                        view.errors.push(e);
                    }
                }
                if !optional {
                    view.offline |= offline;
                    oldest = oldest.min(fetched);
                }
                let base = url.rsplit_once('/').map_or(String::new(), |(dir, _)| format!("{dir}/"));
                for m in &mut index.mods {
                    m.index_base.clone_from(&base);
                }
                main_ok |= !optional;
                got.push(index);
            }
            Err(e) => {
                crate::launcher_log::line(&format!("mod index {url}: {}", e.detail));
                if !optional {
                    view.newer_format |= e.detail.contains("update the launcher");
                    last_error = Some(e);
                }
            }
        }
    }
    let _ = write_state(&src.cache, &state);
    if !main_ok {
        return Err(last_error.unwrap_or_else(|| ModError::new(ModErrorKind::Bug, "No mod list is set.")));
    }
    view.index = merge(got);
    view.fetched_at = if oldest == u64::MAX { 0 } else { oldest };
    view.stale = parse_time(&view.index.expires).is_none_or(|t| src.now > t);
    Ok(view)
}

/// A mod's long texts (`details-<id>.json`): its readme and each version's changelog.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexDetails")]
pub struct Details {
    #[serde(default)]
    pub id: String,
    #[serde(default)]
    pub readme: String,
    #[serde(default)]
    pub versions: Vec<VersionNotes>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "IndexVersionNotes")]
pub struct VersionNotes {
    pub version: String,
    #[serde(default)]
    pub changelog: String,
}

/// A mod's details file, fetched when first needed and kept by its hash. The signed index names
/// its SHA-256, so a file that does not match is refused. A mod with no details file (an older
/// index) gets the texts the index itself carries.
pub fn details(fetch: &dyn Fetch, cache: &Path, m: &IndexMod) -> Result<Details, ModError> {
    let from_index = || Details {
        id: m.id.clone(),
        readme: m.readme.clone(),
        versions: m
            .versions
            .iter()
            .map(|v| VersionNotes { version: v.version.clone(), changelog: v.changelog.clone() })
            .collect(),
    };
    let Some(file) = m.details.as_ref().filter(|_| !m.index_base.is_empty()) else { return Ok(from_index()) };
    let bad = |what: String| ModError::new(ModErrorKind::UpstreamBad, what);
    if file.asset.contains('/') || file.asset.contains("..") || file.sha256.len() != 64 {
        return Err(bad(format!("{}: the details file's name is not valid.", m.id)));
    }
    let dir = cache.join("details");
    let kept = dir.join(format!("{}.json", file.sha256));
    let bytes = match std::fs::read(&kept) {
        Ok(b) if crate::sha256_hex(&b) == file.sha256 => b,
        _ => {
            let b = fetch.get(&format!("{}{}", m.index_base, file.asset), false).map_err(|e| {
                ModError::new(ModErrorKind::Transient, format!("{}'s details could not be fetched ({e}).", m.id))
                    .with(ModAction::Retry)
            })?;
            if crate::sha256_hex(&b) != file.sha256 {
                return Err(bad(format!("{}'s details do not match the signed mod list.", m.id)));
            }
            let _ = std::fs::create_dir_all(&dir).and_then(|()| std::fs::write(&kept, &b));
            b
        }
    };
    let d: Details =
        serde_json::from_slice(&bytes).map_err(|e| bad(format!("{}'s details cannot be read ({e}).", m.id)))?;
    if d.id != m.id {
        return Err(bad(format!("{}'s details are for another mod.", m.id)));
    }
    Ok(d)
}

/// The installed mods whose version the index revoked: `(id, version)` of each installed one.
pub fn revoked_installed(index: &ModIndex, installed: &[(String, String)]) -> Vec<RevokedOff> {
    installed
        .iter()
        .filter_map(|(id, version)| {
            index.revoked(id, version).map(|reason| RevokedOff {
                id: id.clone(),
                name: index.get(id).map_or(id.clone(), |m| m.name.clone()),
                version: version.clone(),
                reason: reason.to_string(),
            })
        })
        .collect()
}

/// Turns off the installed mods of the plugins folder `dir` whose version was revoked; the ones
/// turned off (with the reason).
pub fn turn_off_revoked(dir: &Path, game: &str, disc: &str, index: &ModIndex) -> Vec<RevokedOff> {
    let Ok(views) = crate::plugins::list(dir, game, disc) else { return Vec::new() };
    let on: Vec<(String, String)> =
        views.iter().filter(|v| v.enabled).map(|v| (v.id.clone(), v.version.clone())).collect();
    let found = revoked_installed(index, &on);
    for r in &found {
        if let Err(e) = crate::plugins::set_enabled(dir, game, disc, &r.id, false) {
            crate::launcher_log::line(&format!("revoked {} {}: could not turn it off: {e}", r.id, r.version));
        }
    }
    found
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use std::cell::RefCell;
    use std::collections::HashMap;

    pub const FIXTURE: &str = include_str!("../tests/fixtures/index.json");
    pub const FIXTURE_SIG: &str = include_str!("../tests/fixtures/index.json.minisig");
    /// A key made for these tests only (`minisign -G -W`), its private half thrown away;
    /// FIXTURE_SIG is its signature of the fixture (`minisign -S`, the format index.yml publishes).
    pub const TEST_KEY: &str = "dW50cnVzdGVkIGNvbW1lbnQ6IG1pbmlzaWduIHB1YmxpYyBrZXkgMzk2RkM2MzFGRDkyM0NGMgpSV1R5UEpMOU1jWnZPUU8zNE1OTytWWmdjN3hwMjRXOFBKRUpSSkNvVGlwMlIxU1pYV0FIVGpsSAo=";
    const URL: &str = "https://example.test/index.json";

    pub fn fixture() -> ModIndex {
        serde_json::from_str(FIXTURE).unwrap()
    }

    /// The network, answered from memory; None: that URL fails.
    #[derive(Default)]
    struct Net {
        files: RefCell<HashMap<String, Vec<u8>>>,
    }

    impl Net {
        fn serving(json: &[u8], sig: &str) -> Net {
            let net = Net::default();
            net.files.borrow_mut().insert(URL.into(), json.to_vec());
            net.files.borrow_mut().insert(format!("{URL}.minisig"), sig.as_bytes().to_vec());
            net
        }
    }

    impl Fetch for Net {
        fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
            self.files.borrow().get(url).cloned().ok_or_else(|| format!("{url} could not be fetched (offline)"))
        }
    }

    fn sources<'a>(net: &'a Net, cache: &Path, now: u64) -> Sources<'a> {
        Sources { urls: vec![URL.into()], fetch: net, cache: cache.to_path_buf(), key: TEST_KEY, now }
    }

    const BEFORE_EXPIRY: u64 = 1_762_000_000; // 2025-11-01, before 2026-11-07

    #[test]
    fn the_fixture_reads_with_unknown_fields_ignored() {
        let mut raw: serde_json::Value = serde_json::from_str(FIXTURE).unwrap();
        raw["somethingNew"] = serde_json::json!({"x": 1});
        raw["mods"][0]["alsoNew"] = serde_json::json!(true);
        let index: ModIndex = serde_json::from_value(raw).unwrap();
        assert_eq!(index.format, 1);
        let lock = index.get("lock-strafe").unwrap();
        assert_eq!(lock.kind, "native");
        assert_eq!(lock.versions[0].platforms["linux-x86_64"].asset, "lock-strafe-1.2.0-linux-x86_64.zip");
        assert_eq!(index.revoked("ammo-max", "1.0.0"), Some("1.0.0 could corrupt save files. Use 1.0.1."));
        assert_eq!(index.revoked("ammo-max", "1.0.1"), None);
    }

    #[test]
    fn a_valid_signature_is_accepted_and_kept_for_offline() {
        let cache = tempfile::tempdir().unwrap();
        let net = Net::serving(FIXTURE.as_bytes(), FIXTURE_SIG);
        let view = fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).unwrap();
        assert_eq!(view.index.mods.len(), fixture().mods.len());
        assert!(!view.offline && !view.stale && view.errors.is_empty());
        assert!(view.index.mods.iter().all(|m| m.media_base == view.index.media));

        // Offline: the copy, checked again, with when it was fetched.
        let offline = Net::default();
        let view = fetch(&sources(&offline, cache.path(), BEFORE_EXPIRY + 3600)).unwrap();
        assert!(view.offline);
        assert_eq!(view.fetched_at, BEFORE_EXPIRY);
        assert_eq!(view.errors[0].kind, ModErrorKind::Transient);
        assert_eq!(view.index.mods.len(), fixture().mods.len());
    }

    #[test]
    fn the_base64_signature_form_is_read_too() {
        let cache = tempfile::tempdir().unwrap();
        let b64 = base64::engine::general_purpose::STANDARD.encode(FIXTURE_SIG);
        let net = Net::serving(FIXTURE.as_bytes(), &b64);
        assert!(fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).is_ok());
    }

    #[test]
    fn ai_assisted_is_read_and_off_when_left_out() {
        let on: IndexMod = serde_json::from_str(r#"{"id":"a","aiAssisted":true}"#).unwrap();
        let old: IndexMod = serde_json::from_str(r#"{"id":"a"}"#).unwrap();
        assert!(on.ai_assisted);
        assert!(!old.ai_assisted);
    }

    #[test]
    fn the_built_in_key_is_a_minisign_public_key() {
        let text = base64::engine::general_purpose::STANDARD.decode(INDEX_PUBLIC_KEY).unwrap();
        let text = String::from_utf8(text).unwrap();
        assert!(text.starts_with("untrusted comment: minisign public key 5466767758D8C8D1\n"), "{text}");
        minisign_verify::PublicKey::decode(&text).unwrap();
        // And the line after the comment alone, as minisign's own -P takes it.
        minisign_verify::PublicKey::from_base64(text.lines().nth(1).unwrap()).unwrap();
    }

    #[test]
    fn a_raw_minisig_is_base64_encoded_as_update_verify_takes_it() {
        // The builder publishes minisign's own text; update::verify takes it as one base64 line.
        let b64 = signature_base64(FIXTURE_SIG);
        assert!(!b64.contains('\n') && !b64.starts_with("untrusted"));
        let back = String::from_utf8(base64::engine::general_purpose::STANDARD.decode(&b64).unwrap()).unwrap();
        assert_eq!(back.trim_end(), FIXTURE_SIG.trim_end());
        let file = tempfile::NamedTempFile::new().unwrap();
        std::fs::write(file.path(), FIXTURE).unwrap();
        crate::update::verify(file.path(), &b64, TEST_KEY).unwrap();
        // Already base64: passed as it is.
        assert_eq!(signature_base64(&b64), b64);
    }

    #[test]
    fn a_bad_or_missing_signature_is_refused_and_the_cache_kept() {
        let cache = tempfile::tempdir().unwrap();
        // Nothing cached: an error, never the unchecked list.
        let changed = FIXTURE.replace("Strafe as in", "Steal as in");
        let net = Net::serving(changed.as_bytes(), FIXTURE_SIG);
        let e = fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).unwrap_err();
        assert_eq!(e.kind, ModErrorKind::UpstreamBad);
        assert!(!cache.path().join(format!("{}.json", cache_name(URL))).exists());

        // A good copy first, then a bad one: the copy stays in use, and on disk.
        fetch(&sources(&Net::serving(FIXTURE.as_bytes(), FIXTURE_SIG), cache.path(), BEFORE_EXPIRY)).unwrap();
        let view = fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).unwrap();
        assert!(view.offline);
        assert_eq!(view.errors[0].kind, ModErrorKind::UpstreamBad);
        assert!(!view.index.mods.iter().any(|m| m.description.contains("Steal")));

        let unsigned = Net::default();
        unsigned.files.borrow_mut().insert(URL.into(), FIXTURE.as_bytes().to_vec());
        assert_eq!(
            fetch(&sources(&unsigned, cache.path(), BEFORE_EXPIRY)).unwrap().errors[0].kind,
            ModErrorKind::UpstreamBad
        );
        // The real key does not take the test key's signature.
        let mut src = sources(&net, cache.path(), BEFORE_EXPIRY);
        src.key = INDEX_PUBLIC_KEY;
        src.cache = tempfile::tempdir().unwrap().keep();
        assert_eq!(fetch(&src).unwrap_err().kind, ModErrorKind::UpstreamBad);
    }

    #[test]
    fn a_lower_serial_is_refused() {
        let cache = tempfile::tempdir().unwrap();
        let net = Net::serving(FIXTURE.as_bytes(), FIXTURE_SIG);
        let serial = fixture().serial;
        // A newer one was seen before: this older (signed) copy is a rollback.
        write_state(cache.path(), &State { serials: [(URL.to_string(), serial + 1)].into(), fetched: BTreeMap::new() })
            .unwrap();
        let e = fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).unwrap_err();
        assert_eq!(e.kind, ModErrorKind::UpstreamBad);
        assert!(e.detail.contains("older"), "{}", e.detail);
        // The same serial again is fine.
        write_state(cache.path(), &State { serials: [(URL.to_string(), serial)].into(), fetched: BTreeMap::new() })
            .unwrap();
        assert!(fetch(&sources(&net, cache.path(), BEFORE_EXPIRY)).is_ok());
    }

    #[test]
    fn an_expired_index_is_stale_but_used() {
        let cache = tempfile::tempdir().unwrap();
        let net = Net::serving(FIXTURE.as_bytes(), FIXTURE_SIG);
        let expires = parse_time(&fixture().expires).unwrap();
        let view = fetch(&sources(&net, cache.path(), expires + 1)).unwrap();
        assert!(view.stale);
        assert!(!view.index.mods.is_empty());
    }

    #[test]
    fn times_are_read_as_utc() {
        assert_eq!(parse_time("1970-01-01T00:00:00Z"), Some(0));
        assert_eq!(parse_time("2026-10-31T10:00:00Z"), Some(1_793_440_800));
        assert_eq!(parse_time("2000-02-29T12:30:15.250Z"), Some(951_827_415));
        assert_eq!(parse_time("2026-10-31 10:00:00"), None);
        assert_eq!(parse_time("2026-13-01T00:00:00Z"), None);
    }

    #[test]
    fn indexes_merge_with_the_first_winning() {
        let a = fixture();
        let mut b = ModIndex { format: 1, media: "https://other/".into(), ..Default::default() };
        let mut dup = a.get("lock-strafe").unwrap().clone();
        dup.name = "Impostor".into();
        b.mods.push(dup);
        b.mods.push(IndexMod { id: "zzz-new".into(), ..Default::default() });
        b.revoked.push(Revoked { id: "zzz-new".into(), versions: vec!["*".into()], reason: "bad".into() });
        let merged = merge(vec![a.clone(), b]);
        assert_eq!(merged.get("lock-strafe").unwrap().name, "Lock-strafe");
        assert_eq!(merged.get("zzz-new").unwrap().media_base, "https://other/");
        assert_eq!(merged.revoked("zzz-new", "9.9.9"), Some("bad"));
        assert_eq!(merged.mods.len(), a.mods.len() + 1);
    }

    #[test]
    fn revoked_installed_versions_are_found() {
        let index = fixture();
        let found =
            revoked_installed(&index, &[("ammo-max".into(), "1.0.0".into()), ("lock-strafe".into(), "1.2.0".into())]);
        assert_eq!(found.len(), 1);
        assert_eq!((found[0].id.as_str(), found[0].name.as_str()), ("ammo-max", "Ammo max"));
    }

    #[test]
    fn the_community_list_is_optional() {
        let cache = tempfile::tempdir().unwrap();
        let net = Net::serving(FIXTURE.as_bytes(), FIXTURE_SIG);
        let mut src = sources(&net, cache.path(), BEFORE_EXPIRY);
        src.urls.push("https://example.test/community.json".into());
        let view = fetch(&src).unwrap();
        assert!(!view.offline && view.errors.is_empty(), "{:?}", view.errors);
        assert!(view.index.mods.iter().all(|m| m.index_base == "https://example.test/"));
        // Only the community list: not enough.
        let (offline, empty) = (Net::default(), tempfile::tempdir().unwrap());
        let mut src = sources(&offline, empty.path(), BEFORE_EXPIRY);
        src.urls = vec!["https://example.test/community.json".into()];
        assert!(fetch(&src).is_err());
    }

    #[test]
    fn trust_fields_read_and_older_indexes_have_none() {
        let m: IndexMod = serde_json::from_str(
            r#"{"id":"a","author":"X","trust":"unchecked","owner":{"login":"Some-One","id":5},
                "held":{"version":"1.1.0","reasons":["mentions cmd.exe"]},
                "details":{"asset":"details-a.json","sha256":"00"},
                "versions":[{"version":"1.0.0","scan":{"labels":["Network"],"new":["Network"]},"built":"ci"}]}"#,
        )
        .unwrap();
        assert!(!m.checked());
        assert_eq!(m.trust_key(), "some-one");
        assert_eq!(m.held.as_ref().unwrap().version, "1.1.0");
        assert_eq!(m.versions[0].scan.as_ref().unwrap().labels, ["Network"]);
        assert_eq!(m.versions[0].built.as_deref(), Some("ci"));
        let old: IndexMod = serde_json::from_str(r#"{"id":"a","author":"X"}"#).unwrap();
        assert_eq!((old.trust.as_str(), old.trust_key()), ("", "x".to_string()));
        assert!(old.held.is_none() && old.details.is_none());
    }

    #[test]
    fn details_are_checked_against_the_signed_hash() {
        let cache = tempfile::tempdir().unwrap();
        let body = br#"{"id":"a","readme":"Long text","versions":[{"version":"1.0.0","changelog":"Fixed"}]}"#;
        let mut m: IndexMod = serde_json::from_str(r#"{"id":"a","readme":"short"}"#).unwrap();
        // No details file: the index's own texts.
        assert_eq!(details(&Net::default(), cache.path(), &m).unwrap().readme, "short");
        m.index_base = "https://example.test/".into();
        m.details = Some(DetailsFile { asset: "details-a.json".into(), sha256: crate::sha256_hex(body) });
        let net = Net::default();
        net.files.borrow_mut().insert("https://example.test/details-a.json".into(), body.to_vec());
        let d = details(&net, cache.path(), &m).unwrap();
        assert_eq!((d.readme.as_str(), d.versions[0].changelog.as_str()), ("Long text", "Fixed"));
        // Kept: read again offline.
        assert_eq!(details(&Net::default(), cache.path(), &m).unwrap().readme, "Long text");
        // Changed on the server: refused.
        let other = tempfile::tempdir().unwrap();
        net.files
            .borrow_mut()
            .insert("https://example.test/details-a.json".into(), b"{\"id\":\"a\",\"readme\":\"evil\"}".to_vec());
        assert_eq!(details(&net, other.path(), &m).unwrap_err().kind, ModErrorKind::UpstreamBad);
    }

    #[test]
    fn the_configured_list_replaces_the_default() {
        assert_eq!(urls(&[]), vec![INDEXES[0].to_string(), COMMUNITY.to_string()]);
        assert_eq!(
            urls(&["http://insecure".into(), " https://mine/index.json ".into()]),
            vec!["https://mine/index.json"]
        );
    }
}
