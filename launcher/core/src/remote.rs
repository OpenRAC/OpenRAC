//! Mods from a git repository's address (docs/port/MODS.md, section 10): what the address holds,
//! shown before anything is installed; a native mod's built archive for this computer from the
//! repository's release, checked against GitHub's digest or its `SHA256SUMS`; a script mod's or a data pack's files (the
//! release's joined archive, checked, else the repository's own files: MODS.md, section 11); where each mod came
//! from (`<data>/plugins/sources.json`), and newer releases to update to.
//!
//! GitHub is read through its HTTPS API and raw files, with `curl` (no git, no compiler): the
//! player has neither. Another git host is read with `git`, when it is installed, for data packs
//! only: a native mod comes built, from a release. Everything goes through [`Fetch`], which the
//! tests answer from memory, so no test needs the network.

use std::collections::BTreeMap;
use std::io::Read;
use std::path::Path;

use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::plugins::{self, Installed, Manifest, MANIFEST};

/// What remembers each installed mod's source, in the plugins folder.
pub const SOURCES: &str = "sources.json";
/// A repository's list of its mods' folders (optional).
pub const INDEX: &str = "openrac-mods.json";
/// A release's checksums, in `sha256sum`'s format.
pub const SUMS: &str = "SHA256SUMS";
/// The tag of the release a mods repository publishes its index in (index.rs).
pub const INDEX_TAG: &str = "index";
/// The largest file fetched (a mod's archive, a repository's).
pub const MAX_BYTES: u64 = 256 * 1024 * 1024;
/// The most of a manifest read out of an archive.
const MANIFEST_MAX_BYTES: u64 = 1024 * 1024;

// ---- Fetching -------------------------------------------------------------------------------

/// Gets a URL's bytes. `api`: a GitHub API request (its JSON media type).
pub trait Fetch {
    fn get(&self, url: &str, api: bool) -> Result<Vec<u8>, String>;
}

impl<T: Fetch + ?Sized> Fetch for &T {
    fn get(&self, url: &str, api: bool) -> Result<Vec<u8>, String> {
        (**self).get(url, api)
    }
}

/// The real one for mods: `curl` (on Linux, macOS and Windows 10+), https only, following
/// redirects (a release's asset, a repository's archive), with a time limit and a size cap.
/// GitHub's API is asked with the token in `GH_TOKEN` or `GITHUB_TOKEN` when one is set (5,000
/// requests an hour instead of 60), handed to curl on its input, never on its command line.
pub struct Curl;

impl Fetch for Curl {
    fn get(&self, url: &str, api: bool) -> Result<Vec<u8>, String> {
        if !url.starts_with("https://") {
            return Err(format!("{url} is not an https:// address"));
        }
        let mut cmd = crate::detect::quiet(Path::new("curl"));
        // No -f: the answer's headers (-D -, before its body) say why GitHub refused, and when its
        // limit lets the launcher ask again.
        cmd.args(["-sS", "-L", "-D", "-", "--proto", "=https", "--connect-timeout", "15", "--max-time", "600"])
            .args(["--max-filesize", &MAX_BYTES.to_string()])
            .args(["-H", concat!("User-Agent: openrac-launcher/", env!("CARGO_PKG_VERSION"))]);
        if api {
            cmd.args(["-H", "Accept: application/vnd.github+json", "-H", "X-GitHub-Api-Version: 2022-11-28"]);
        }
        // curl sends a header given this way to the address asked only, not where it redirects to.
        let token = github_token().filter(|_| url.starts_with("https://api.github.com/"));
        let config = token.map(|t| {
            cmd.args(["-K", "-"]);
            format!("header = \"Authorization: Bearer {t}\"\n")
        });
        let out = run_curl(cmd, url, config.as_deref(), "the launcher fetches mods with it")?;
        let (status, headers, body) = split_response(&out);
        match status {
            200..=299 => Ok(body.to_vec()),
            _ => Err(http_error(url, status, &headers, config.is_some(), unix_now())),
        }
    }
}

/// A GitHub token from the environment, if one is set and looks like one.
fn github_token() -> Option<String> {
    ["GH_TOKEN", "GITHUB_TOKEN"]
        .iter()
        .filter_map(|k| std::env::var(k).ok())
        .map(|t| t.trim().to_string())
        .find(|t| !t.is_empty() && t.chars().all(|c| c.is_ascii_alphanumeric() || c == '_'))
}

fn unix_now() -> u64 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_or(0, |d| d.as_secs())
}

/// curl's output with `-D -`: the final answer's status and headers (names in lower case) and its
/// body. With `-L` each redirect's headers come first, and are skipped.
fn split_response(out: &[u8]) -> (u16, Vec<(String, String)>, &[u8]) {
    let mut rest = out;
    let (mut status, mut headers) = (0, Vec::new());
    while rest.starts_with(b"HTTP/") {
        let end = rest.windows(4).position(|w| w == b"\r\n\r\n").map(|i| (i, 4));
        let Some((i, n)) = end.or_else(|| rest.windows(2).position(|w| w == b"\n\n").map(|i| (i, 2))) else { break };
        let block = String::from_utf8_lossy(&rest[..i]);
        let mut lines = block.lines();
        let first = lines.next().unwrap_or_default();
        status = first.split_whitespace().nth(1).and_then(|s| s.parse().ok()).unwrap_or(0);
        headers = lines
            .filter_map(|l| l.split_once(':'))
            .map(|(k, v)| (k.trim().to_ascii_lowercase(), v.trim().to_string()))
            .collect();
        rest = &rest[i + n..];
        // A redirect followed, a 100 Continue, a proxy's tunnel: another answer comes after it.
        let interim = (100..200).contains(&status)
            || (300..400).contains(&status)
            || first.to_ascii_lowercase().contains("connection established");
        if !(interim && rest.starts_with(b"HTTP/")) {
            break;
        }
    }
    (status, headers, rest)
}

/// Why an answer of `status` is no file, in the words the Mods tab shows. `now`: the time in Unix
/// seconds, for how long GitHub's limit lasts.
fn http_error(url: &str, status: u16, headers: &[(String, String)], token: bool, now: u64) -> String {
    let header = |name: &str| headers.iter().find(|(k, _)| k == name).map(|(_, v)| v.as_str());
    let spent = header("x-ratelimit-remaining") == Some("0");
    let retry = header("retry-after").and_then(|s| s.parse::<u64>().ok());
    if matches!(status, 403 | 429) && (spent || retry.is_some()) {
        let wait = retry.or_else(|| header("x-ratelimit-reset")?.parse::<u64>().ok().map(|r| r.saturating_sub(now)));
        let when = match wait.map(|s| s.div_ceil(60).max(1)) {
            None => "try again in an hour".to_string(),
            Some(1) => "try again in a minute".to_string(),
            Some(m) => format!("try again in {m} minutes"),
        };
        if !spent {
            return format!("GitHub asked the launcher to slow down; {when}");
        }
        let limit = header("x-ratelimit-limit").map_or(String::new(), |l| format!(" of {l} requests an hour"));
        let whose = if token { "for your GitHub token" } else { "from this network" };
        return format!("GitHub's limit{limit} {whose} is used up; {when}");
    }
    match status {
        404 => format!("{url}: not found"),
        0 => format!("{url} could not be fetched"),
        s => format!("{url} could not be fetched (HTTP {s})"),
    }
}

/// How long an answer of GitHub's API is reused: long enough for one look and its updates, short
/// enough that a release just published is seen soon.
pub const API_CACHE_SECONDS: u64 = 5 * 60;

/// Answers of GitHub's API kept for a few minutes ([`API_CACHE_SECONDS`]), for the launcher's
/// whole run: checking a repository of eleven mods and then updating each would read it twelve
/// times, and GitHub allows 60 requests an hour without a login. Files and downloads are not kept.
pub struct ApiCache {
    ttl: std::time::Duration,
    answers: std::sync::Mutex<std::collections::HashMap<String, (std::time::Instant, Vec<u8>)>>,
}

impl ApiCache {
    pub fn new(ttl: std::time::Duration) -> Self {
        ApiCache { ttl, answers: Default::default() }
    }

    /// The launcher's one cache.
    pub fn shared() -> &'static ApiCache {
        static SHARED: std::sync::OnceLock<ApiCache> = std::sync::OnceLock::new();
        SHARED.get_or_init(|| ApiCache::new(std::time::Duration::from_secs(API_CACHE_SECONDS)))
    }
}

/// A [`Fetch`] whose API answers come from `cache` while they are fresh. Failures are not kept.
pub struct Cached<'c, F> {
    pub inner: F,
    pub cache: &'c ApiCache,
}

impl<F> Cached<'static, F> {
    /// `inner` through the launcher's one cache.
    pub fn shared(inner: F) -> Self {
        Cached { inner, cache: ApiCache::shared() }
    }
}

impl<F: Fetch> Fetch for Cached<'_, F> {
    fn get(&self, url: &str, api: bool) -> Result<Vec<u8>, String> {
        if !api {
            return self.inner.get(url, api);
        }
        let lock = || self.cache.answers.lock().unwrap_or_else(std::sync::PoisonError::into_inner);
        if let Some((at, bytes)) = lock().get(url) {
            if at.elapsed() < self.cache.ttl {
                return Ok(bytes.clone());
            }
        }
        let bytes = self.inner.get(url, api)?;
        let mut answers = lock();
        answers.retain(|_, (at, _)| at.elapsed() < self.cache.ttl);
        answers.insert(url.to_string(), (std::time::Instant::now(), bytes.clone()));
        Ok(bytes)
    }
}

/// A short fetch that must not hold the launcher's start up: bytematch.dev's progress figures
/// (progress_sync.rs). `curl` with 4 seconds to connect and 10 in all, no redirects and no
/// headers of its own; `api` is ignored.
pub struct QuickCurl;

impl Fetch for QuickCurl {
    fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
        let mut cmd = crate::detect::quiet(Path::new("curl"));
        cmd.args(["-s", "-f", "--connect-timeout", "4", "--max-time", "10"]);
        run_curl(cmd, url, None, "the launcher fetches its progress figures with it")
    }
}

/// Runs `cmd` (a `curl` with its options) on `url`, `input` (a config: `-K -`) on its input, else
/// its input closed: what it printed, or why it failed. `purpose` says, when curl cannot be
/// started, what the launcher needed it for.
fn run_curl(mut cmd: std::process::Command, url: &str, input: Option<&str>, purpose: &str) -> Result<Vec<u8>, String> {
    use std::process::Stdio;
    let started = |e: std::io::Error| format!("curl cannot be started ({e}): {purpose}");
    cmd.arg(url).stdout(Stdio::piped()).stderr(Stdio::piped());
    let out = match input {
        None => cmd.stdin(Stdio::null()).output().map_err(started)?,
        Some(text) => {
            let mut child = cmd.stdin(Stdio::piped()).spawn().map_err(started)?;
            if let Some(mut stdin) = child.stdin.take() {
                use std::io::Write;
                let _ = stdin.write_all(text.as_bytes());
            }
            child.wait_with_output().map_err(started)?
        }
    };
    if out.status.success() {
        return Ok(out.stdout);
    }
    let why = String::from_utf8_lossy(&out.stderr).trim().to_string();
    Err(if why.contains("404") {
        format!("{url}: not found")
    } else if why.is_empty() {
        format!("{url} could not be fetched")
    } else {
        format!("{url} could not be fetched ({why})")
    })
}

/// Where GitHub answers: its API, its raw files and its site, for a repository's archive (the
/// tests point them elsewhere). Raw files and archives do not count against the API's limit.
#[derive(Debug, Clone)]
pub struct Hosts {
    pub api: String,
    pub raw: String,
    pub web: String,
}

impl Default for Hosts {
    fn default() -> Self {
        Hosts {
            api: "https://api.github.com".into(),
            raw: "https://raw.githubusercontent.com".into(),
            web: "https://github.com".into(),
        }
    }
}

// ---- Addresses ------------------------------------------------------------------------------

/// What a repository GitHub does not show is, to someone without a login: private, or not there.
pub const NOT_FOUND: &str =
    "This repository is private or doesn't exist. OpenRAC can only install from public repositories.";

/// A repository's address, as the player pasted it.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", tag = "host")]
pub enum Source {
    /// `https://github.com/<owner>/<repo>`, maybe at a branch or commit (`/tree/<ref>`) or a
    /// release (`/releases/tag/<tag>`).
    #[serde(rename = "github")]
    GitHub { owner: String, repo: String, reference: Option<String>, tag: Option<String> },
    /// Any other git host: the address git clones.
    #[serde(rename = "git")]
    Git { url: String },
}

impl Source {
    /// The repository's address without a version: what sources.json keeps.
    pub fn url(&self) -> String {
        match self {
            Source::GitHub { owner, repo, .. } => format!("https://github.com/{owner}/{repo}"),
            Source::Git { url } => url.clone(),
        }
    }

    /// "owner/repo", or the address.
    pub fn label(&self) -> String {
        match self {
            Source::GitHub { owner, repo, .. } => format!("{owner}/{repo}"),
            Source::Git { url } => url.clone(),
        }
    }
}

fn github_name(s: &str) -> bool {
    !s.is_empty() && s != "." && s != ".." && s.chars().all(|c| c.is_ascii_alphanumeric() || "-_.".contains(c))
}

/// Reads an address: GitHub's forms, or another host's https/ssh git address.
pub fn parse_url(text: &str) -> Result<Source, String> {
    let text = text.trim();
    if text.is_empty() {
        return Err("Paste the address of a repository.".into());
    }
    let bare = text.strip_prefix("https://").or_else(|| text.strip_prefix("http://")).unwrap_or(text);
    let bare = bare.strip_prefix("www.").unwrap_or(bare);
    // git@github.com:<owner>/<repo>(.git), the address GitHub's "Code" button gives for SSH.
    let bare = match bare.strip_prefix("git@github.com:").or_else(|| bare.strip_prefix("ssh://git@github.com/")) {
        Some(path) => format!("github.com/{path}"),
        None => bare.to_string(),
    };
    // <owner>/<repo>: GitHub's own shorthand.
    let bare = match bare.split('/').collect::<Vec<_>>().as_slice() {
        [owner, repo] if !bare.contains(':') && !owner.contains('.') && github_name(owner) && github_name(repo) => {
            format!("github.com/{bare}")
        }
        _ => bare,
    };
    if let Some(path) = bare.strip_prefix("github.com/") {
        let path = path.split(['?', '#']).next().unwrap_or_default().trim_end_matches('/');
        let parts: Vec<&str> = path.split('/').collect();
        if parts.len() < 2 {
            return Err(format!("{text} is not a repository's address (github.com/<owner>/<repository>)"));
        }
        let owner = parts[0].to_string();
        let repo = parts[1].strip_suffix(".git").unwrap_or(parts[1]).to_string();
        if !github_name(&owner) || !github_name(&repo) {
            return Err(format!("{text} is not a repository's address (github.com/<owner>/<repository>)"));
        }
        let rest = &parts[2..];
        let (reference, tag) = match rest {
            [] => (None, None),
            ["tree", r @ ..] if !r.is_empty() => (Some(r.join("/")), None),
            ["releases", "tag", t @ ..] if !t.is_empty() => (None, Some(t.join("/"))),
            ["releases"] | ["releases", "latest"] => (None, None),
            _ => return Err(format!("{text}: open the repository's main page and copy its address")),
        };
        return Ok(Source::GitHub { owner, repo, reference, tag });
    }
    let git = text.starts_with("https://") || text.starts_with("ssh://") || text.starts_with("git@");
    if git && !text.contains(char::is_whitespace) {
        return Ok(Source::Git { url: text.trim_end_matches('/').to_string() });
    }
    Err(format!("{text} is not a repository's address: paste one that starts with https://"))
}

// ---- What a repository holds ----------------------------------------------------------------

#[derive(Debug, Clone, Deserialize)]
struct Asset {
    name: String,
    browser_download_url: String,
    #[serde(default)]
    digest: Option<String>,
}

impl Asset {
    /// GitHub's SHA-256 of the uploaded file, lower case. Older uploads have none.
    fn sha256(&self) -> Option<String> {
        let hex = self.digest.as_deref()?.strip_prefix("sha256:")?;
        (hex.len() == 64 && hex.chars().all(|c| c.is_ascii_hexdigit())).then(|| hex.to_ascii_lowercase())
    }
}

#[derive(Debug, Clone, Deserialize)]
struct Release {
    tag_name: String,
    #[serde(default)]
    draft: bool,
    #[serde(default)]
    prerelease: bool,
    #[serde(default)]
    assets: Vec<Asset>,
    /// When it was published: what a mod code's review shows.
    #[serde(default)]
    published_at: Option<String>,
    /// The date of the release's commit (GitHub's `created_at`).
    #[serde(default)]
    created_at: Option<String>,
}

impl Release {
    fn asset(&self, name: &str) -> Option<&Asset> {
        self.assets.iter().find(|a| a.name == name)
    }

    /// When it was made, for newest-first: its commit's date, else when it was published.
    fn when(&self) -> Option<&str> {
        self.created_at.as_deref().or(self.published_at.as_deref())
    }

    /// Whether it holds a build of mod `id` for `platform` (`<id>-<version>-<platform>.zip`).
    fn builds(&self, id: &str, platform: &str) -> bool {
        let (prefix, suffix) = (format!("{id}-"), format!("-{platform}.zip"));
        self.assets.iter().any(|a| {
            a.name
                .strip_prefix(&prefix)
                .and_then(|r| r.strip_suffix(&suffix))
                .is_some_and(|v| plugins::parse_version(v).is_some())
        })
    }

    /// Whether asset `name` can be checked: by GitHub's digest or the release's SHA256SUMS.
    fn checkable(&self, name: &str) -> bool {
        self.asset(SUMS).is_some() || self.asset(name).is_some_and(|a| a.sha256().is_some())
    }
}

/// One mod the repository holds, as the Mods tab shows it before it is installed.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct RemoteMod {
    pub id: String,
    pub name: String,
    pub version: String,
    pub author: String,
    pub description: String,
    pub homepage: String,
    /// Its folder in the repository, "" for the top.
    pub path: String,
    /// "native" (a library the game loads: code), "script" (Luau the game runs sandboxed) or "pack"
    /// (textures, models, levels).
    #[ts(type = "\"native\" | \"script\" | \"pack\"")]
    pub kind: String,
    pub games: BTreeMap<String, Vec<String>>,
    pub api: Option<String>,
    /// Whether this computer's game can take it; if not, why.
    pub compatible: bool,
    pub why: Option<String>,
    /// The release asset that would be installed, and the release's tag.
    pub asset: Option<String>,
    pub tag: Option<String>,
    /// The version installed now, if it is.
    pub installed: Option<String>,
}

/// What an address holds.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[ts(rename = "RemotePreview")]
#[serde(rename_all = "camelCase")]
pub struct Preview {
    /// The repository's address, without a version.
    pub url: String,
    /// "owner/repo", or the address.
    pub label: String,
    /// "github" or "git".
    #[ts(type = "\"github\" | \"git\"")]
    pub host: String,
    /// The owner, who publishes it (GitHub).
    pub owner: String,
    /// What the manifests were read at: a release's tag, a branch or a commit.
    pub reference: String,
    /// The commit read, where known (a data pack with no release).
    pub commit: Option<String>,
    pub mods: Vec<RemoteMod>,
    /// Said about the whole repository ("no release yet").
    pub note: Option<String>,
}

fn json<T: for<'a> Deserialize<'a>>(bytes: &[u8], what: &str) -> Result<T, String> {
    serde_json::from_slice(bytes).map_err(|e| format!("{what} cannot be read ({e})"))
}

/// The manifests' paths in a repository's file list: the index's folders, the top's manifest, or
/// one in each top folder.
pub fn discover(paths: &[String], index: Option<&Value>) -> Result<Vec<String>, String> {
    if let Some(index) = index {
        let folders =
            index.get("mods").and_then(Value::as_array).ok_or_else(|| format!("{INDEX} has no \"mods\" list"))?;
        let mut out = Vec::new();
        for f in folders {
            let f = f.as_str().unwrap_or_default().trim_matches('/');
            if f.is_empty() || !plugins::safe_relative(f) {
                return Err(format!("{INDEX} names {f:?}, which is not a folder of the repository"));
            }
            let path = format!("{f}/{MANIFEST}");
            if !paths.contains(&path) {
                return Err(format!("{INDEX} names {f}, which has no {MANIFEST}"));
            }
            out.push(path);
        }
        return Ok(out);
    }
    if paths.iter().any(|p| p == MANIFEST) {
        return Ok(vec![MANIFEST.to_string()]);
    }
    let mut out: Vec<String> = paths
        .iter()
        .filter(|p| p.split('/').count() == 2 && p.ends_with(&format!("/{MANIFEST}")) && !p.starts_with('.'))
        .cloned()
        .collect();
    out.sort();
    if out.is_empty() {
        return Err(format!(
            "No OpenRAC mod in this repository: it has no {MANIFEST} at its top or in a folder at its top."
        ));
    }
    Ok(out)
}

/// The folder a manifest's path is in, "" for the top.
fn folder_of(path: &str) -> String {
    path.rsplit_once('/').map(|(f, _)| f.to_string()).unwrap_or_default()
}

/// A release's asset for one mod on one platform: `<id>-<version>-<platform>.zip`.
pub fn asset_name(id: &str, version: &str, platform: &str) -> String {
    format!("{id}-{version}-{platform}.zip")
}

/// `SHA256SUMS`: file name → hash (lower case). A `*` before the name (binary mode) is allowed.
pub fn parse_sums(text: &str) -> BTreeMap<String, String> {
    let mut out = BTreeMap::new();
    for line in text.lines() {
        let line = line.trim();
        let Some((hash, name)) = line.split_once(char::is_whitespace) else { continue };
        let name = name.trim_start().trim_start_matches('*').trim();
        if hash.len() == 64 && hash.chars().all(|c| c.is_ascii_hexdigit()) && !name.is_empty() {
            out.insert(name.to_string(), hash.to_ascii_lowercase());
        }
    }
    out
}

/// Whether `bytes` are the file `name`, by GitHub's `digest` and what `sums` lists: why not. One
/// of the two is needed; when both are there, both must match.
pub fn verify(bytes: &[u8], name: &str, digest: Option<&str>, sums: Option<&str>) -> Result<(), String> {
    let listed = sums.and_then(|s| parse_sums(s).remove(name));
    if digest.is_none() && listed.is_none() {
        return Err(match sums {
            None => format!("The release has no {SUMS}: {name} cannot be checked, so it is not installed."),
            Some(_) => {
                format!("The release's {SUMS} does not list {name}: it cannot be checked, so it is not installed.")
            }
        });
    }
    let got = crate::sha256_hex(bytes);
    if [digest, listed.as_deref()].into_iter().flatten().any(|want| want != got) {
        return Err(format!(
            "{name} is not the file the release lists (its SHA-256 differs): it was damaged or changed, and is not installed."
        ));
    }
    Ok(())
}

/// The manifest in a mod's .zip (at its top or in its one folder), read without unpacking it.
fn manifest_in_zip(bytes: &[u8]) -> Result<Value, String> {
    let mut archive =
        zip::ZipArchive::new(std::io::Cursor::new(bytes)).map_err(|e| format!("the download is not a .zip ({e})"))?;
    let mut best: Option<String> = None;
    for i in 0..archive.len() {
        let name = archive.by_index(i).map_err(|e| e.to_string())?.name().map_err(|e| e.to_string())?.to_string();
        let depth = name.split('/').count();
        if name.ends_with(MANIFEST)
            && (name == MANIFEST || depth == 2)
            && best.as_ref().is_none_or(|b| b.len() > name.len())
        {
            best = Some(name);
        }
    }
    let name = best.ok_or_else(|| format!("the download has no {MANIFEST}: it is not an OpenRAC mod"))?;
    let mut text = String::new();
    // A manifest is a few kilobytes: one that claims more is read no further than the limit.
    archive
        .by_name(&name)
        .map_err(|e| e.to_string())?
        .take(MANIFEST_MAX_BYTES + 1)
        .read_to_string(&mut text)
        .map_err(|e| e.to_string())?;
    if text.len() as u64 > MANIFEST_MAX_BYTES {
        return Err(format!("its {MANIFEST} is too large to be a mod's"));
    }
    serde_json::from_str(&text).map_err(|e| format!("its {MANIFEST} cannot be read ({e})"))
}

/// A manifest's text as a mod to show: what it is and whether this computer's game can take it.
#[allow(clippy::too_many_arguments)]
fn describe(
    raw: &Value,
    path: &str,
    repo: &str,
    game: &str,
    version: &str,
    platform: Option<&str>,
    installed: &BTreeMap<String, String>,
) -> Result<RemoteMod, String> {
    let text = |k: &str| raw.get(k).and_then(Value::as_str).unwrap_or_default().to_string();
    let format = raw.get("format").and_then(Value::as_u64).unwrap_or(0);
    let folder = folder_of(path);
    if format == 1 {
        // A data pack (editor/MODDING.md): named after its folder, or the repository.
        let id = if folder.is_empty() { repo.to_ascii_lowercase() } else { folder.clone() };
        let pack_game = text("game");
        let (compatible, why) = if pack_game == game {
            (true, None)
        } else {
            (false, Some(format!("It is a pack for {pack_game}, not this game.")))
        };
        let mut games = BTreeMap::new();
        games.insert(pack_game, vec![text("version")]);
        return Ok(RemoteMod {
            installed: installed.get(&id).cloned(),
            id,
            name: text("name"),
            version: text("pack_version"),
            author: text("author"),
            description: text("description"),
            homepage: String::new(),
            path: folder,
            kind: "pack".into(),
            games,
            api: None,
            compatible,
            why,
            asset: None,
            tag: None,
        });
    }
    let m: Manifest = serde_json::from_value(raw.clone()).map_err(|e| format!("{path} cannot be read ({e})"))?;
    let check = m.validate().map_err(|e| format!("Its {MANIFEST} breaks a rule: {e}.")).and_then(|()| {
        if m.plugin.is_empty() && !m.is_script() {
            m.games
                .get(game)
                .filter(|d| d.iter().any(|d| d == version))
                .map(|_| ())
                .ok_or_else(|| format!("Not for this game's disc ({game}, {version})."))
        } else {
            m.runs_on(game, version, platform)
        }
    });
    Ok(RemoteMod {
        installed: installed.get(&m.id).cloned(),
        id: m.id.clone(),
        name: m.name.clone(),
        version: m.version.clone(),
        author: m.author.clone(),
        description: m.description.clone(),
        homepage: m.homepage.clone(),
        path: folder,
        // A repository's manifest is a mod's source: a native mod's names no library yet (its
        // release's archive adds it), so format 2 that is not a script is native. A data pack
        // is format 1 (handled above).
        kind: if m.is_script() { "script".into() } else { "native".into() },
        games: m.games.clone(),
        api: m.api.clone(),
        compatible: check.is_ok(),
        why: check.err(),
        asset: None,
        tag: None,
    })
}

/// The installed mods' versions, by id (native mods' manifests, and sources.json for packs).
fn installed_versions(dir: &Path) -> BTreeMap<String, String> {
    let mut out: BTreeMap<String, String> = read_sources(dir).mods.into_iter().map(|(id, s)| (id, s.version)).collect();
    if let Ok(entries) = std::fs::read_dir(dir) {
        for e in entries.flatten() {
            if let Ok(m) = plugins::read_manifest(&e.path()) {
                out.insert(m.id.clone(), m.version.clone());
            }
        }
    }
    out
}

/// A GitHub repository read through its API: the releases, the file list and the manifests at a
/// version, and per mod the release whose asset is for this computer.
struct GitHubRead {
    preview: Preview,
    releases: Vec<Release>,
}

fn read_github(
    fetch: &dyn Fetch,
    hosts: &Hosts,
    owner: &str,
    repo: &str,
    reference: Option<&str>,
    tag: Option<&str>,
    ctx: (&str, &str, Option<&str>, &Path),
) -> Result<GitHubRead, String> {
    let (game, version, platform, dir) = ctx;
    let base = format!("{}/repos/{owner}/{repo}", hosts.api);
    let releases: Vec<Release> = json(
        &fetch.get(&format!("{base}/releases?per_page=100"), true).map_err(|e| {
            // GitHub answers a private repository and one that is not there alike (404) without a
            // login, so the message cannot tell them apart.
            if e.ends_with("not found") {
                NOT_FOUND.to_string()
            } else {
                e
            }
        })?,
        "the repository's releases",
    )?;
    let mut eligible: Vec<Release> = match tag {
        Some(t) => {
            let r = releases
                .iter()
                .find(|r| r.tag_name == t)
                .ok_or_else(|| format!("github.com/{owner}/{repo} has no release {t}"))?;
            vec![r.clone()]
        }
        // The mod index's own release (openrac-mods' `index` tag) holds no mod: never a mod's release.
        None => releases.iter().filter(|r| !r.draft && !r.prerelease && r.tag_name != INDEX_TAG).cloned().collect(),
    };
    // Newest first. GitHub's list is not in date order: a repository of many mods, each released
    // on its own tag, can list an older mod's release first, and every other mod's manifest would
    // then be read as it was back then (openrac-mods listed xbox-icons-v1.1.0 first, from before
    // the other mods took Going Commando, so they could not be installed for it). The dates are
    // GitHub's ISO 8601 in UTC, which sort as text; a release without one goes last.
    eligible.sort_by(|a, b| b.when().cmp(&a.when()));
    // What the manifests are read at: the release named, a branch or commit named, the newest
    // release, or the default branch.
    // A link to a folder, /tree/<branch>/<folder>: the branch is the shortest start of the path the
    // repository knows (a branch's name may hold slashes), the rest the folder whose mods are shown.
    let (at, commit, subdir) = match (tag, reference, eligible.first()) {
        (Some(t), _, _) => (t.to_string(), commit_of(fetch, &base, t)?, None),
        (None, Some(r), _) => {
            let parts: Vec<&str> = r.split('/').collect();
            let mut found = None;
            for n in 1..=parts.len() {
                let branch = parts[..n].join("/");
                match commit_of(fetch, &base, &branch) {
                    Ok(c) => {
                        let rest = parts[n..].join("/");
                        found = Some((branch, c, (!rest.is_empty()).then_some(rest)));
                        break;
                    }
                    Err(e) if n == parts.len() || !e.ends_with("not found") => return Err(e),
                    Err(_) => {}
                }
            }
            found.ok_or_else(|| format!("github.com/{owner}/{repo} has no branch {r}"))?
        }
        (None, None, Some(r)) => (r.tag_name.clone(), commit_of(fetch, &base, &r.tag_name)?, None),
        (None, None, None) => ("HEAD".to_string(), commit_of(fetch, &base, "HEAD")?, None),
    };
    let tree_at = commit.clone().unwrap_or_else(|| at.clone());
    let tree: Value =
        json(&fetch.get(&format!("{base}/git/trees/{tree_at}?recursive=1"), true)?, "the repository's files")?;
    let paths: Vec<String> = tree
        .get("tree")
        .and_then(Value::as_array)
        .map(|t| {
            t.iter()
                .filter(|e| e.get("type").and_then(Value::as_str) == Some("blob"))
                .filter_map(|e| e.get("path").and_then(Value::as_str).map(str::to_string))
                .collect()
        })
        .unwrap_or_default();
    let raw_at = |path: &str| format!("{}/{owner}/{repo}/{tree_at}/{path}", hosts.raw);
    let index = if paths.iter().any(|p| p == INDEX) {
        Some(json::<Value>(&fetch.get(&raw_at(INDEX), false)?, INDEX)?)
    } else {
        None
    };
    let installed = installed_versions(dir);
    let mut mods = Vec::new();
    // A folder named in the link: only the mods in it.
    let found = discover(&paths, index.as_ref())?;
    let shown: Vec<String> = match &subdir {
        None => found.clone(),
        Some(d) => found.iter().filter(|p| p.starts_with(&format!("{d}/"))).cloned().collect(),
    };
    if !found.is_empty() && shown.is_empty() {
        return Err(format!("github.com/{owner}/{repo} has no mod in {}", subdir.unwrap_or_default()));
    }
    for path in shown {
        let raw: Value = json(&fetch.get(&raw_at(&path), false)?, &path)?;
        let mut m = describe(&raw, &path, repo, game, version, platform, &installed)?;
        // A mod whose version at `at` has no release yet (its bump came before another mod's
        // release): its manifest at its own newest release, which is what would be installed.
        if let (Some(p), None, None, "native") = (platform, tag, reference, m.kind.as_str()) {
            let name = asset_name(&m.id, &m.version, p);
            if !eligible.iter().any(|r| r.asset(&name).is_some()) {
                if let Some(r) = eligible.iter().find(|r| r.tag_name != at && r.builds(&m.id, p)) {
                    let url = format!("{}/{owner}/{repo}/{}/{path}", hosts.raw, r.tag_name);
                    let own = fetch
                        .get(&url, false)
                        .and_then(|bytes| json::<Value>(&bytes, &path))
                        .and_then(|raw| describe(&raw, &path, repo, game, version, platform, &installed));
                    if let Some(own) = own.ok().filter(|o| o.id == m.id) {
                        m = own;
                    }
                }
            }
        }
        if m.kind == "native" {
            match platform {
                None if m.compatible => {
                    m.compatible = false;
                    m.why = Some("Native mods are not loaded on this system.".to_string());
                }
                None => {}
                Some(p) => {
                    let name = asset_name(&m.id, &m.version, p);
                    match eligible.iter().find(|r| r.asset(&name).is_some()) {
                        Some(r) => {
                            m.asset = Some(name);
                            m.tag = Some(r.tag_name.clone());
                            if !r.checkable(m.asset.as_deref().unwrap_or_default()) && m.compatible {
                                m.compatible = false;
                                m.why = Some(format!(
                                    "Its release has no {SUMS} and no digest from GitHub, so its download cannot be checked."
                                ));
                            }
                        }
                        None if m.compatible => {
                            m.compatible = false;
                            // The usual slip: the tag's "v" in the file's name.
                            let with_v = asset_name(&m.id, &format!("v{}", m.version), p);
                            m.why = Some(if eligible.is_empty() {
                                "No release yet: a native mod is installed built, from a release.".to_string()
                            } else if eligible.iter().any(|r| r.asset(&with_v).is_some()) {
                                format!("Its release has {with_v}, which must be named {name}: no \"v\" before the version.")
                            } else {
                                format!("No release has it built for this computer ({name}).")
                            });
                        }
                        None => {}
                    }
                }
            }
        } else {
            // A script mod or a data pack: its joined archive from a release when there is one
            // (checked against GitHub's digest or SHA256SUMS), else the repository's own files.
            let name = format!("{}-{}.zip", m.id, m.version);
            if let Some(r) = eligible.iter().find(|r| r.asset(&name).is_some() && r.checkable(&name)) {
                m.asset = Some(name);
                m.tag = Some(r.tag_name.clone());
            }
        }
        mods.push(m);
    }
    let note = releases.is_empty().then(|| "This repository has no release yet.".to_string());
    Ok(GitHubRead {
        preview: Preview {
            url: format!("https://github.com/{owner}/{repo}"),
            label: format!("{owner}/{repo}"),
            host: "github".into(),
            owner: owner.into(),
            reference: at,
            commit,
            mods,
            note,
        },
        releases,
    })
}

/// The commit `at` names (a tag, a branch, HEAD), or none when the answer has no sha.
fn commit_of(fetch: &dyn Fetch, base: &str, at: &str) -> Result<Option<String>, String> {
    Ok(json::<Value>(&fetch.get(&format!("{base}/commits/{at}"), true)?, "the repository's commit")?
        .get("sha")
        .and_then(Value::as_str)
        .map(str::to_string))
}

// ---- Other git hosts --------------------------------------------------------------------------

fn git_found() -> bool {
    crate::detect::quiet(Path::new("git"))
        .arg("--version")
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .status()
        .is_ok_and(|s| s.success())
}

fn git(args: &[&str], cwd: Option<&Path>) -> Result<String, String> {
    let mut cmd = crate::detect::quiet(Path::new("git"));
    cmd.args(args).env("GIT_TERMINAL_PROMPT", "0").stdin(std::process::Stdio::null());
    if let Some(cwd) = cwd {
        cmd.current_dir(cwd);
    }
    let out = cmd.output().map_err(|e| format!("git cannot be started ({e})"))?;
    if !out.status.success() {
        return Err(format!("git {}: {}", args.first().unwrap_or(&""), String::from_utf8_lossy(&out.stderr).trim()));
    }
    Ok(String::from_utf8_lossy(&out.stdout).trim().to_string())
}

/// A shallow clone of another host's repository into a folder of `dir`'s: its files and commit.
fn clone_git(url: &str, dir: &Path) -> Result<(plugins::Scratch, String), String> {
    if !git_found() {
        return Err(format!(
            "{url} is not on GitHub, and reading another host needs git, which is not installed. Install git, or ask for the mod's .zip."
        ));
    }
    std::fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    let scratch = plugins::Scratch::new(dir, "clone")?;
    let into = scratch.0.join("repo");
    git(&["clone", "--depth", "1", "--quiet", url, &into.to_string_lossy()], None)?;
    let commit = git(&["rev-parse", "HEAD"], Some(&into))?;
    Ok((scratch, commit))
}

fn files_under(root: &Path) -> Vec<String> {
    fn walk(at: &Path, root: &Path, out: &mut Vec<String>) {
        for e in std::fs::read_dir(at).into_iter().flatten().flatten() {
            let p = e.path();
            if e.file_name() == ".git" {
                continue;
            }
            if p.is_dir() {
                walk(&p, root, out);
            } else if let Ok(rel) = p.strip_prefix(root) {
                out.push(rel.to_string_lossy().replace('\\', "/"));
            }
        }
    }
    let mut out = Vec::new();
    walk(root, root, &mut out);
    out
}

fn read_git(url: &str, ctx: (&str, &str, Option<&str>, &Path)) -> Result<(Preview, plugins::Scratch), String> {
    let (game, version, platform, dir) = ctx;
    let (scratch, commit) = clone_git(url, dir)?;
    let root = scratch.0.join("repo");
    let paths = files_under(&root);
    let index = if root.join(INDEX).is_file() {
        Some(json::<Value>(&std::fs::read(root.join(INDEX)).map_err(|e| e.to_string())?, INDEX)?)
    } else {
        None
    };
    let name = url.trim_end_matches(".git").rsplit(['/', ':']).next().unwrap_or("mod").to_string();
    let installed = installed_versions(dir);
    let mut mods = Vec::new();
    for path in discover(&paths, index.as_ref())? {
        let raw: Value = json(&std::fs::read(root.join(&path)).map_err(|e| e.to_string())?, &path)?;
        let mut m = describe(&raw, &path, &name, game, version, platform, &installed)?;
        if m.kind == "native" && m.compatible {
            m.compatible = false;
            m.why =
                Some("A native mod is installed built, from a release, which only GitHub's are read for yet.".into());
        }
        mods.push(m);
    }
    Ok((
        Preview {
            url: url.into(),
            label: url.into(),
            host: "git".into(),
            owner: String::new(),
            reference: "HEAD".into(),
            commit: Some(commit),
            mods,
            note: None,
        },
        scratch,
    ))
}

// ---- sources.json ---------------------------------------------------------------------------

/// Where one installed mod came from.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModSource {
    pub url: String,
    /// Its folder in the repository, "" for the top.
    #[serde(default)]
    pub path: String,
    pub version: String,
    #[serde(default)]
    pub tag: Option<String>,
    #[serde(default)]
    pub commit: Option<String>,
    /// "native", "script" or "pack".
    #[serde(default)]
    pub kind: String,
    /// Kept at this version: no update is offered.
    #[serde(default)]
    pub pin: bool,
    /// The SHA-256 of its release's `SHA256SUMS`, when a mod code installed it.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub sums: Option<String>,
    /// Installed as another mod's dependency, not asked for (index_apply.rs).
    #[serde(default, skip_serializing_if = "std::ops::Not::not")]
    #[ts(as = "Option<bool>", optional)]
    pub auto: bool,
    /// Why it is kept at its version: `shared` (installed from a shared setup's exact versions) or
    /// `rollback` (rolled back after a crash, until a version newer than `skip` comes out).
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional, type = "\"shared\" | \"rollback\"")]
    pub pin_reason: Option<String>,
    /// A rolled-back version: not updated to again.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    #[ts(optional)]
    pub skip: Option<String>,
    /// A mod pack's mods (index `type: pack`): removing the pack removes the ones it brought.
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    #[ts(as = "Option<Vec<String>>", optional)]
    pub members: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, ts_rs::TS)]
#[ts(rename = "ModSources")]
pub struct Sources {
    pub format: u32,
    #[serde(default)]
    pub mods: BTreeMap<String, ModSource>,
}

impl Default for Sources {
    fn default() -> Self {
        Sources { format: 1, mods: BTreeMap::new() }
    }
}

pub fn read_sources(dir: &Path) -> Sources {
    std::fs::read(dir.join(SOURCES))
        .ok()
        .and_then(|b| serde_json::from_slice::<Sources>(&b).ok())
        .filter(|s| s.format == 1)
        .unwrap_or_default()
}

pub fn write_sources(dir: &Path, sources: &Sources) -> Result<(), String> {
    std::fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    let path = dir.join(SOURCES);
    let tmp = dir.join(format!("{SOURCES}.tmp"));
    let text = serde_json::to_string_pretty(sources).map_err(|e| e.to_string())? + "\n";
    std::fs::write(&tmp, text).map_err(|e| format!("{}: {e}", tmp.display()))?;
    std::fs::rename(&tmp, &path).map_err(|e| format!("{}: {e}", path.display()))
}

/// Forgets a removed mod's source.
pub fn forget(dir: &Path, id: &str) -> Result<(), String> {
    let mut s = read_sources(dir);
    if s.mods.remove(id).is_some() {
        write_sources(dir, &s)?;
    }
    Ok(())
}

/// Keeps a mod at its version (or lets it update again).
pub fn set_pin(dir: &Path, id: &str, pin: bool) -> Result<Sources, String> {
    let mut s = read_sources(dir);
    let entry = s.mods.get_mut(id).ok_or_else(|| format!("{id} was not installed from a repository"))?;
    entry.pin = pin;
    if !pin {
        entry.pin_reason = None;
        entry.skip = None;
    }
    write_sources(dir, &s)?;
    Ok(s)
}

// ---- Preview, install, update ---------------------------------------------------------------

/// Where a game's install comes from: the plugins folder, the game and disc, this computer's
/// platform (None: no native mods here), the fetcher and GitHub's addresses.
pub struct Context<'a> {
    pub dir: &'a Path,
    pub game: &'a str,
    pub version: &'a str,
    pub platform: Option<&'a str>,
    pub fetch: &'a dyn Fetch,
    pub hosts: Hosts,
}

impl Context<'_> {
    fn parts(&self) -> (&str, &str, Option<&str>, &Path) {
        (self.game, self.version, self.platform, self.dir)
    }
}

/// What an address holds, for the Mods tab to show before anything is installed.
pub fn preview(ctx: &Context, url: &str) -> Result<Preview, String> {
    match parse_url(url)? {
        Source::GitHub { owner, repo, reference, tag } => {
            Ok(read_github(ctx.fetch, &ctx.hosts, &owner, &repo, reference.as_deref(), tag.as_deref(), ctx.parts())?
                .preview)
        }
        Source::Git { url } => Ok(read_git(&url, ctx.parts())?.0),
    }
}

/// What installing from an address came to.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase", tag = "kind")]
pub enum RemoteInstalled {
    /// A native or script mod, now in the store.
    Plugin { id: String, name: String, version: String, replaced: bool, enabled: bool },
    /// A data pack, its files in `path`: the overrides' to add (actions.json `overrides-add`).
    Pack { id: String, version: String, path: String },
}

/// Installs mod `id` of the repository at `url`: a native mod's built archive for this computer,
/// checked against the release's SHA256SUMS; a script mod into the store and a data pack's files for
/// the overrides, each from its release's joined archive (checked) or the repository's files. Remembers the
/// source, keeping its pin.
pub fn install(ctx: &Context, url: &str, id: &str) -> Result<RemoteInstalled, String> {
    install_checked(ctx, url, id, None)
}

/// The archive the signed mod index names, which must be the one downloaded: a second hash beside
/// the release's own SHA256SUMS, so an archive swapped on a release after it was indexed is refused.
fn check_expected(bytes: &[u8], name: &str, expected: Option<&crate::index::Asset>) -> Result<(), String> {
    let Some(want) = expected else { return Ok(()) };
    if want.asset != name {
        return Err(format!("{name} is not the archive the mod list names ({}): it is not installed.", want.asset));
    }
    if crate::sha256_hex(bytes) != want.sha256.to_ascii_lowercase() {
        return Err(format!(
            "{name} is not the file the mod list lists (its SHA-256 differs): it was changed after it was listed, and is not installed."
        ));
    }
    Ok(())
}

/// [`install`], with the archive the mod index names for it (`expected`) checked as well.
pub fn install_checked(
    ctx: &Context,
    url: &str,
    id: &str,
    expected: Option<&crate::index::Asset>,
) -> Result<RemoteInstalled, String> {
    let source = parse_url(url)?;
    std::fs::create_dir_all(ctx.dir).map_err(|e| format!("{}: {e}", ctx.dir.display()))?;
    let (preview, releases, clone) = match &source {
        Source::GitHub { owner, repo, reference, tag } => {
            let r = read_github(ctx.fetch, &ctx.hosts, owner, repo, reference.as_deref(), tag.as_deref(), ctx.parts())?;
            (r.preview, r.releases, None)
        }
        Source::Git { url } => {
            let (p, scratch) = read_git(url, ctx.parts())?;
            (p, Vec::new(), Some(scratch))
        }
    };
    let m =
        preview.mods.iter().find(|m| m.id == id).ok_or_else(|| format!("{} holds no mod {id}", preview.label))?.clone();
    if !m.compatible {
        return Err(m.why.clone().unwrap_or_else(|| format!("{} cannot be installed here", m.name)));
    }
    let release = |tag: &Option<String>| releases.iter().find(|r| Some(&r.tag_name) == tag.as_ref()).cloned();
    let sums_of = |r: &Release| -> Result<Option<String>, String> {
        match r.asset(SUMS) {
            Some(a) => Ok(Some(String::from_utf8_lossy(&ctx.fetch.get(&a.browser_download_url, false)?).into_owned())),
            None => Ok(None),
        }
    };
    let result = if m.kind == "native" {
        let r = release(&m.tag).ok_or("the release went away while it was read")?;
        let name = m.asset.clone().ok_or("no asset")?;
        let asset = r.asset(&name).ok_or("no asset")?;
        let bytes = ctx.fetch.get(&asset.browser_download_url, false)?;
        verify(&bytes, &name, asset.sha256().as_deref(), sums_of(&r)?.as_deref())?;
        check_expected(&bytes, &name, expected)?;
        let inside = manifest_in_zip(&bytes)?;
        let (got_id, got_version) = (
            inside.get("id").and_then(Value::as_str).unwrap_or_default(),
            inside.get("version").and_then(Value::as_str).unwrap_or_default(),
        );
        if got_id != m.id || got_version != m.version {
            return Err(format!(
                "{name} holds {got_id} {got_version}, not the {} {} the repository describes: it is not installed.",
                m.id, m.version
            ));
        }
        let scratch = plugins::Scratch::new(ctx.dir, "download")?;
        let file = scratch.0.join(&name);
        std::fs::write(&file, &bytes).map_err(|e| format!("{}: {e}", file.display()))?;
        match plugins::install(ctx.dir, ctx.game, ctx.version, &file)? {
            Installed::Plugin { id, name, version, replaced, enabled } => {
                RemoteInstalled::Plugin { id, name, version, replaced, enabled }
            }
            Installed::Pack => return Err(format!("{name} is a data pack, not the native mod described")),
        }
    } else {
        // A script mod goes to the store; a pack's files wait in .downloads/<id>-<version>/ for
        // overrides-add.
        let target = ctx.dir.join(".downloads").join(format!("{}-{}", m.id, m.version.replace('/', "_")));
        let _ = std::fs::remove_dir_all(&target);
        let staging = plugins::Scratch::new(ctx.dir, "pack")?;
        let folder = if let (Some(name), Some(r)) = (m.asset.clone(), release(&m.tag)) {
            let asset = r.asset(&name).ok_or("no asset")?;
            let bytes = ctx.fetch.get(&asset.browser_download_url, false)?;
            verify(&bytes, &name, asset.sha256().as_deref(), sums_of(&r)?.as_deref())?;
            check_expected(&bytes, &name, expected)?;
            let zip = staging.0.join(&name);
            std::fs::write(&zip, &bytes).map_err(|e| e.to_string())?;
            let out = staging.0.join("unpacked");
            plugins::unzip(&zip, &out)?;
            // package_mod.py's archives hold one top folder, <id>-<version>/.
            plugins::manifest_root(&out)?
        } else if let Some(want) = expected {
            return Err(format!("The release has no {}, which the mod list names: it is not installed.", want.asset));
        } else if let Some(scratch) = &clone {
            scratch.0.join("repo").join(&m.path)
        } else {
            // The repository's archive at the commit shown: one top folder, then the mod's.
            let Source::GitHub { owner, repo, .. } = &source else { unreachable!() };
            let at = preview.commit.clone().unwrap_or_else(|| preview.reference.clone());
            let bytes = ctx.fetch.get(&archive_url(&ctx.hosts, owner, repo, &at), false)?;
            let zip = staging.0.join("repo.zip");
            std::fs::write(&zip, &bytes).map_err(|e| e.to_string())?;
            let out = staging.0.join("repo");
            plugins::unzip(&zip, &out)?;
            let top = std::fs::read_dir(&out)
                .map_err(|e| e.to_string())?
                .flatten()
                .find(|e| e.path().is_dir())
                .ok_or("the repository's archive is empty")?
                .path();
            top.join(&m.path)
        };
        if !folder.join(MANIFEST).is_file() {
            return Err(format!("the download of {} has no {MANIFEST} where the repository has it", m.name));
        }
        if m.kind == "script" {
            let inside: Value = json(&std::fs::read(folder.join(MANIFEST)).map_err(|e| e.to_string())?, MANIFEST)?;
            let (got_id, got_version) = (
                inside.get("id").and_then(Value::as_str).unwrap_or_default(),
                inside.get("version").and_then(Value::as_str).unwrap_or_default(),
            );
            if got_id != m.id || got_version != m.version {
                return Err(format!(
                    "The download holds {got_id} {got_version}, not the {} {} the repository describes: it is not installed.",
                    m.id, m.version
                ));
            }
            match plugins::install(ctx.dir, ctx.game, ctx.version, &folder)? {
                Installed::Plugin { id, name, version, replaced, enabled } => {
                    RemoteInstalled::Plugin { id, name, version, replaced, enabled }
                }
                Installed::Pack => return Err(format!("{} is a data pack, not the script mod described", m.name)),
            }
        } else {
            std::fs::create_dir_all(target.parent().unwrap_or(ctx.dir)).map_err(|e| e.to_string())?;
            plugins::copy_tree(&folder, &target)?;
            RemoteInstalled::Pack {
                id: m.id.clone(),
                version: m.version.clone(),
                path: target.to_string_lossy().into_owned(),
            }
        }
    };
    let mut sources = read_sources(ctx.dir);
    let pin = sources.mods.get(&m.id).is_some_and(|s| s.pin);
    let auto = sources.mods.get(&m.id).is_some_and(|s| s.auto);
    let (pin_reason, skip) =
        sources.mods.get(&m.id).filter(|s| s.pin).map_or((None, None), |s| (s.pin_reason.clone(), s.skip.clone()));
    sources.mods.insert(
        m.id.clone(),
        ModSource {
            url: source.url(),
            path: m.path.clone(),
            version: m.version.clone(),
            tag: m.tag.clone(),
            commit: preview.commit.clone(),
            kind: m.kind.clone(),
            pin,
            sums: None,
            auto,
            pin_reason,
            skip,
            members: Vec::new(),
        },
    );
    write_sources(ctx.dir, &sources)?;
    Ok(result)
}

/// A newer version of an installed mod.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[ts(rename = "ModUpdate")]
#[serde(rename_all = "camelCase")]
pub struct Update {
    pub id: String,
    pub installed: String,
    pub available: String,
    /// The release it is in, or the commit (a data pack with no release).
    pub tag: Option<String>,
    pub commit: Option<String>,
}

/// What each source not pinned offers that is newer: a release with a newer version built for
/// this computer, or, for a script mod or a data pack with no release, a newer commit. A source that cannot be read
/// now is left out (`errors` says which, once per address with the mods it holds).
///
/// Each address is read once, however many of its mods are installed: a repository of several
/// mods would otherwise cost GitHub's anonymous allowance (60 requests an hour) a read per mod.
pub fn check_updates(ctx: &Context) -> (Vec<Update>, Vec<String>) {
    let mut updates = Vec::new();
    let mut read: BTreeMap<String, Result<Preview, String>> = BTreeMap::new();
    let mut failed: BTreeMap<String, Vec<String>> = BTreeMap::new();
    for (id, s) in read_sources(ctx.dir).mods {
        if s.pin {
            continue;
        }
        let got = read.entry(s.url.clone()).or_insert_with(|| preview(ctx, &s.url));
        match got {
            Err(_) => failed.entry(s.url.clone()).or_default().push(id),
            Ok(p) => {
                let Some(m) = p.mods.iter().find(|m| m.id == id && m.compatible) else { continue };
                let newer = match (plugins::parse_version(&m.version), plugins::parse_version(&s.version)) {
                    (Some(a), Some(b)) => plugins::compare_versions(a, b).is_gt(),
                    _ => false,
                };
                let moved = m.tag.is_none() && m.kind != "native" && p.commit.is_some() && p.commit != s.commit;
                if newer || (moved && m.version == s.version) {
                    updates.push(Update {
                        id,
                        installed: s.version.clone(),
                        available: m.version.clone(),
                        tag: m.tag.clone(),
                        commit: if moved { p.commit.clone() } else { None },
                    });
                }
            }
        }
    }
    let errors = failed
        .into_iter()
        .map(|(url, ids)| match &read[&url] {
            Err(e) => format!("{}: {e}", ids.join(", ")),
            Ok(_) => unreachable!("only unread addresses fail"),
        })
        .collect();
    (updates, errors)
}

/// Installs the newest version of mod `id` from where it came from (its place, on/off and settings
/// kept). A pinned mod is refused.
pub fn update(ctx: &Context, id: &str) -> Result<RemoteInstalled, String> {
    let s = read_sources(ctx.dir).mods.remove(id).ok_or_else(|| format!("{id} was not installed from a repository"))?;
    if s.pin {
        return Err(format!("{id} is kept at {}: let it update first", s.version));
    }
    install(ctx, &s.url, id)
}

// ---- A mod at an exact version ----------------------------------------------------------------

/// The newest archives kept in the cache, in bytes, before the oldest go.
const CACHE_BYTES: u64 = 512 * 1024 * 1024;

/// One mod a mod code names (modset.rs): never "the newest", always this version of it, from a
/// release's `tag` whose `SHA256SUMS` hashes to `sums`, or from a `commit`.
#[derive(Debug, Clone, PartialEq)]
pub struct Pin {
    pub url: String,
    pub path: String,
    pub id: String,
    pub version: String,
    pub tag: Option<String>,
    pub commit: Option<String>,
    pub sums: Option<String>,
}

/// Why a pinned mod cannot be had.
#[derive(Debug, Clone, PartialEq)]
pub enum PinError {
    /// The release or the commit is not there any more.
    Gone,
    /// It is there, and is not what the code was made from: a retag, a rebuilt release.
    Changed,
    /// It is what the code names, and cannot run here: the words say why.
    Unfit(String),
    /// It could not be read now (no network, GitHub's limit).
    Failed(String),
}

/// A pinned mod as read, nothing downloaded yet: what the review shows and the install needs.
#[derive(Debug, Clone)]
pub struct Pinned {
    pub pin: Pin,
    pub name: String,
    /// "native" or "script".
    pub kind: String,
    pub manifest: Manifest,
    /// When its release was published.
    pub published: Option<String>,
    /// The archive to fetch: its name, its address and, from the release's `SHA256SUMS`, its hash.
    asset: Option<(String, String, String)>,
    /// The address of the repository's archive, when there is no release to fetch from.
    zipball: Option<String>,
}

/// What the download of a pinned mod left: checked, not installed yet.
pub struct Staged {
    _scratch: plugins::Scratch,
    path: std::path::PathBuf,
}

fn github_parts(url: &str) -> Result<(String, String), String> {
    match parse_url(url)? {
        Source::GitHub { owner, repo, reference: None, tag: None } => Ok((owner, repo)),
        _ => Err(format!("{url} is not a repository on GitHub")),
    }
}

/// A repository's archive at `at` (a commit, a tag), from GitHub's site rather than its API, whose
/// limit it does not count against.
fn archive_url(hosts: &Hosts, owner: &str, repo: &str, at: &str) -> String {
    format!("{}/{owner}/{repo}/archive/{at}.zip", hosts.web)
}

/// The release tagged `tag`: from the list of the newest releases, which every mod of a repository
/// shares (one request for a code of ten mods, through [`Cached`]), else asked for by its tag.
fn release_by_tag(ctx: &Context, owner: &str, repo: &str, tag: &str) -> Result<Release, String> {
    let base = format!("{}/repos/{owner}/{repo}", ctx.hosts.api);
    let listed = ctx.fetch.get(&format!("{base}/releases?per_page=100"), true).ok();
    if let Some(r) = listed
        .and_then(|bytes| serde_json::from_slice::<Vec<Release>>(&bytes).ok())
        .and_then(|list| list.into_iter().find(|r| r.tag_name == tag))
    {
        return Ok(r);
    }
    json(&ctx.fetch.get(&format!("{base}/releases/tags/{tag}"), true)?, "the release")
}

/// An error of the fetcher: "not found" and every other failure are different stories to tell.
fn pin_fetch_error(e: String) -> PinError {
    if e.ends_with("not found") {
        PinError::Gone
    } else {
        PinError::Failed(e)
    }
}

/// Reads what `pin` names with as few of GitHub's requests as can be: one to the API for its
/// release (the list of releases, which a code's other mods from the repository share), which the
/// anonymous limit (60 an hour) counts, and raw files, which it does not. No tree: a code of ten
/// mods from one repository must not read it ten times.
pub fn read_pinned(ctx: &Context, pin: &Pin) -> Result<Pinned, PinError> {
    let (owner, repo) = github_parts(&pin.url).map_err(PinError::Failed)?;
    let (at, release) = match (&pin.tag, &pin.commit) {
        (Some(tag), _) => (tag.clone(), Some(release_by_tag(ctx, &owner, &repo, tag).map_err(pin_fetch_error)?)),
        (None, Some(commit)) => (commit.clone(), None),
        (None, None) => return Err(PinError::Failed(format!("{} names no version to fetch", pin.id))),
    };
    let manifest_path = if pin.path.is_empty() { MANIFEST.to_string() } else { format!("{}/{MANIFEST}", pin.path) };
    let raw_url = format!("{}/{owner}/{repo}/{at}/{manifest_path}", ctx.hosts.raw);
    let raw: Value =
        json(&ctx.fetch.get(&raw_url, false).map_err(pin_fetch_error)?, MANIFEST).map_err(PinError::Failed)?;
    let described = describe(&raw, &manifest_path, &repo, ctx.game, ctx.version, ctx.platform, &BTreeMap::new())
        .map_err(|_| PinError::Changed)?;
    if described.id != pin.id || described.version != pin.version || described.kind == "pack" {
        return Err(PinError::Changed);
    }
    let manifest: Manifest = serde_json::from_value(raw).map_err(|_| PinError::Changed)?;
    if !described.compatible {
        return Err(PinError::Unfit(described.why.unwrap_or_default()));
    }
    let mut asset = None;
    let mut published = None;
    // What is installed is the release's archive, not the repository's manifest: with a release,
    // the kind is the archive's. A native build is `<id>-<version>-<platform>.zip` (for any
    // platform), a script `<id>-<version>.zip`; stage_pinned checks the archive's manifest agrees.
    let mut kind = described.kind.clone();
    if let Some(release) = &release {
        let built = plugins::PLATFORMS.iter().any(|p| release.asset(&asset_name(&pin.id, &pin.version, p)).is_some());
        kind = if built { "native".into() } else { "script".into() };
    }
    if let Some(release) = &release {
        published = release.published_at.clone();
        let sums_asset = release.asset(SUMS).ok_or(PinError::Changed)?;
        let text = ctx.fetch.get(&sums_asset.browser_download_url, false).map_err(pin_fetch_error)?;
        if pin.sums.as_deref() != Some(crate::sha256_hex(&text).as_str()) {
            return Err(PinError::Changed);
        }
        let listed = parse_sums(&String::from_utf8_lossy(&text));
        let wanted = if kind == "native" {
            let Some(platform) = ctx.platform else {
                return Err(PinError::Unfit("Native mods are not loaded on this system.".into()));
            };
            asset_name(&pin.id, &pin.version, platform)
        } else {
            format!("{}-{}.zip", pin.id, pin.version)
        };
        match (release.asset(&wanted), listed.get(&wanted)) {
            (Some(a), Some(hash)) => asset = Some((wanted, a.browser_download_url.clone(), hash.clone())),
            (Some(_), None) => return Err(PinError::Changed),
            (None, _) if kind == "native" => return Err(PinError::Unfit("Not built for this computer.".into())),
            (None, _) => {}
        }
    } else if kind == "native" {
        return Err(PinError::Unfit("A native mod is installed built, from a release.".into()));
    }
    let zipball = asset.is_none().then(|| archive_url(&ctx.hosts, &owner, &repo, &at));
    Ok(Pinned { pin: pin.clone(), name: described.name, kind, manifest, published, asset, zipball })
}

/// The newest version of mod `id` a repository's releases have an archive of, for "ask the host to
/// update": a release's assets are `<id>-<version>[-<platform>].zip`.
pub fn newest_release(ctx: &Context, url: &str, id: &str) -> Option<String> {
    let (owner, repo) = github_parts(url).ok()?;
    let releases: Vec<Release> =
        json(&ctx.fetch.get(&format!("{}/repos/{owner}/{repo}/releases?per_page=100", ctx.hosts.api), true).ok()?, "")
            .ok()?;
    let prefix = format!("{id}-");
    let mut best: Option<(String, (u64, u64, u64, bool))> = None;
    for release in releases.iter().filter(|r| !r.draft && !r.prerelease) {
        for asset in &release.assets {
            let Some(rest) = asset.name.strip_prefix(&prefix).and_then(|r| r.strip_suffix(".zip")) else { continue };
            let version = plugins::PLATFORMS.iter().find_map(|p| rest.strip_suffix(&format!("-{p}"))).unwrap_or(rest);
            let Some(parsed) = plugins::parse_version(version) else { continue };
            if best.as_ref().is_none_or(|(_, b)| plugins::compare_versions(parsed, *b).is_gt()) {
                best = Some((version.to_string(), parsed));
            }
        }
    }
    best.map(|(version, _)| version)
}

/// The SHA-256 of a release's `SHA256SUMS`: what a mod code pins a release by. It holds every
/// platform's hash, so one code serves them all, and a rebuilt release changes it.
pub fn sums_hash(ctx: &Context, url: &str, tag: &str) -> Result<String, String> {
    let (owner, repo) = github_parts(url)?;
    let release = release_by_tag(ctx, &owner, &repo, tag)?;
    let asset = release.asset(SUMS).ok_or_else(|| format!("{tag} has no {SUMS}"))?;
    Ok(crate::sha256_hex(&ctx.fetch.get(&asset.browser_download_url, false)?))
}

/// `<plugins>/.cache/<sha256>.zip`: an archive a code fetched, kept under its own hash. A file
/// there is read back only if it still hashes to its name.
fn cache_file(dir: &Path, hash: &str) -> std::path::PathBuf {
    dir.join(".cache").join(format!("{hash}.zip"))
}

fn from_cache(dir: &Path, hash: &str) -> Option<Vec<u8>> {
    let bytes = std::fs::read(cache_file(dir, hash)).ok()?;
    (crate::sha256_hex(&bytes) == hash).then_some(bytes)
}

/// Keeps an archive by its hash, then takes the least recently written ones away past the limit.
/// The cache only saves downloads: failing to keep one is no failure.
fn to_cache(dir: &Path, hash: &str, bytes: &[u8]) {
    let file = cache_file(dir, hash);
    let Some(parent) = file.parent() else { return };
    if std::fs::create_dir_all(parent).is_err() || std::fs::write(&file, bytes).is_err() {
        return;
    }
    let mut kept: Vec<(std::time::SystemTime, u64, std::path::PathBuf)> = std::fs::read_dir(parent)
        .into_iter()
        .flatten()
        .flatten()
        .filter_map(|e| {
            let meta = e.metadata().ok()?;
            Some((meta.modified().ok()?, meta.len(), e.path()))
        })
        .collect();
    kept.sort();
    let mut total: u64 = kept.iter().map(|k| k.1).sum();
    for (_, size, path) in kept {
        if total <= CACHE_BYTES || path == file {
            break;
        }
        if std::fs::remove_file(&path).is_ok() {
            total -= size;
        }
    }
}

/// Downloads a pinned mod (or takes it from the cache) and checks it, without touching the store:
/// a code's mods are all fetched before the first is installed, so that a failure changes nothing.
pub fn stage_pinned(ctx: &Context, pinned: &Pinned) -> Result<Staged, String> {
    std::fs::create_dir_all(ctx.dir).map_err(|e| format!("{}: {e}", ctx.dir.display()))?;
    let scratch = plugins::Scratch::new(ctx.dir, "code")?;
    let path = if let Some((name, url, hash)) = &pinned.asset {
        let bytes = match from_cache(ctx.dir, hash) {
            Some(bytes) => bytes,
            None => {
                let bytes = ctx.fetch.get(url, false)?;
                if crate::sha256_hex(&bytes) != *hash {
                    return Err(format!(
                        "{name} is not the file the release lists (its SHA-256 differs): it was damaged or changed, and is not installed."
                    ));
                }
                to_cache(ctx.dir, hash, &bytes);
                bytes
            }
        };
        let inside = manifest_in_zip(&bytes)?;
        check_inside(&inside, pinned, ctx.platform)?;
        let file = scratch.0.join(name);
        std::fs::write(&file, &bytes).map_err(|e| format!("{}: {e}", file.display()))?;
        file
    } else {
        let url = pinned.zipball.as_deref().ok_or("nothing to download")?;
        let zip = scratch.0.join("repo.zip");
        std::fs::write(&zip, ctx.fetch.get(url, false)?).map_err(|e| e.to_string())?;
        let out = scratch.0.join("repo");
        plugins::unzip(&zip, &out)?;
        let top = std::fs::read_dir(&out)
            .map_err(|e| e.to_string())?
            .flatten()
            .find(|e| e.path().is_dir())
            .ok_or("the repository's archive is empty")?
            .path();
        let folder = top.join(&pinned.pin.path);
        let inside: Value =
            json(&std::fs::read(folder.join(MANIFEST)).map_err(|e| format!("{}: {e}", pinned.name))?, MANIFEST)?;
        check_inside(&inside, pinned, ctx.platform)?;
        folder
    };
    Ok(Staged { _scratch: scratch, path })
}

/// A download must hold the mod the review showed: its id and version, and its kind, a native
/// mod's library for this computer among them.
fn check_inside(manifest: &Value, pinned: &Pinned, platform: Option<&str>) -> Result<(), String> {
    let pin = &pinned.pin;
    let text = |k: &str| manifest.get(k).and_then(Value::as_str).unwrap_or_default();
    if text("id") != pin.id || text("version") != pin.version {
        return Err(format!(
            "The download holds {} {}, not {} {}: it is not installed.",
            text("id"),
            text("version"),
            pin.id,
            pin.version
        ));
    }
    let inside: Manifest = serde_json::from_value(manifest.clone())
        .map_err(|e| format!("The download's {MANIFEST} cannot be read ({e}): it is not installed."))?;
    let kind = if inside.is_script() { "script" } else { "native" };
    if kind != pinned.kind {
        return Err(format!(
            "The download holds a {kind} mod, not the {} mod shown: {} is not installed.",
            pinned.kind, pinned.name
        ));
    }
    if kind == "native" && platform.is_none_or(|p| !inside.plugin.contains_key(p)) {
        return Err(format!("The download has no library for this computer: {} is not installed.", pinned.name));
    }
    if kind == "script" && !inside.plugin.is_empty() {
        return Err(format!(
            "The download names a library, not the script mod shown: {} is not installed.",
            pinned.name
        ));
    }
    Ok(())
}

/// Puts a staged mod in the store and remembers exactly what it is.
pub fn install_staged(ctx: &Context, pinned: &Pinned, staged: &Staged) -> Result<(), String> {
    match plugins::install(ctx.dir, ctx.game, ctx.version, &staged.path)? {
        Installed::Plugin { .. } => {}
        Installed::Pack => return Err(format!("{} is a data pack, not the mod described", pinned.name)),
    }
    let mut sources = read_sources(ctx.dir);
    sources.mods.insert(
        pinned.pin.id.clone(),
        ModSource {
            url: pinned.pin.url.clone(),
            path: pinned.pin.path.clone(),
            version: pinned.pin.version.clone(),
            tag: pinned.pin.tag.clone(),
            commit: pinned.pin.commit.clone(),
            kind: pinned.kind.clone(),
            // A code's versions are exact: updating one would leave the party's set.
            pin: true,
            pin_reason: Some("shared".into()),
            sums: pinned.pin.sums.clone(),
            ..ModSource::default()
        },
    );
    write_sources(ctx.dir, &sources)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::RefCell;
    use std::collections::HashMap;
    use std::io::Write;
    use std::path::PathBuf;

    /// GitHub, answered from memory; every request is recorded.
    #[derive(Default)]
    struct Fixture {
        files: HashMap<String, Vec<u8>>,
        asked: RefCell<Vec<String>>,
    }

    impl Fixture {
        fn put(&mut self, url: &str, bytes: impl Into<Vec<u8>>) {
            self.files.insert(url.to_string(), bytes.into());
        }
    }

    impl Fetch for Fixture {
        fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
            self.asked.borrow_mut().push(url.to_string());
            self.files.get(url).cloned().ok_or_else(|| format!("{url}: not found"))
        }
    }

    const API: &str = "https://api.test";
    const RAW: &str = "https://raw.test";
    const DL: &str = "https://dl.test";
    const PLATFORM: &str = "linux-x86_64";

    const WEB: &str = "https://web.test";

    fn hosts() -> Hosts {
        Hosts { api: API.into(), raw: RAW.into(), web: WEB.into() }
    }

    fn manifest(id: &str, version: &str) -> String {
        format!(
            r#"{{"format": 2, "id": "{id}", "name": "Mod {id}", "version": "{version}", "author": "Someone",
                "api": "1.0", "games": {{"rac1": ["pal", "ntsc"]}},
                "plugin": {{"{PLATFORM}": "lib/{PLATFORM}/m.so", "windows-x86_64": "lib/windows-x86_64/m.dll"}}}}"#
        )
    }

    fn mod_zip(id: &str, version: &str) -> Vec<u8> {
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file(format!("{id}/openrac-mod.json"), o).unwrap();
        // As package_mod.py writes it: the manifest names this platform's library only.
        let one = manifest(id, version).replace(r#", "windows-x86_64": "lib/windows-x86_64/m.dll""#, "");
        w.write_all(one.as_bytes()).unwrap();
        w.start_file(format!("{id}/lib/{PLATFORM}/m.so"), o).unwrap();
        w.write_all(b"library").unwrap();
        w.finish().unwrap().into_inner()
    }

    fn sha(bytes: &[u8]) -> String {
        crate::sha256_hex(bytes)
    }

    /// A repository o/r with the given files at commit c1, releases as given.
    fn repo(fx: &mut Fixture, files: &[(&str, String)], releases: Value, commit: &str) {
        fx.put(&format!("{API}/repos/o/r/releases?per_page=100"), releases.to_string());
        let tree: Vec<Value> = files.iter().map(|(p, _)| serde_json::json!({"path": p, "type": "blob"})).collect();
        for at in ["HEAD", "v1.0.0", "v1.1.0", "v2.0.0", "main"] {
            fx.put(&format!("{API}/repos/o/r/commits/{at}"), serde_json::json!({"sha": commit}).to_string());
        }
        fx.put(
            &format!("{API}/repos/o/r/git/trees/{commit}?recursive=1"),
            serde_json::json!({"tree": tree}).to_string(),
        );
        for (p, text) in files {
            fx.put(&format!("{RAW}/o/r/{commit}/{p}"), text.clone());
        }
    }

    /// A release with a mod's asset for this platform and, unless `sums` is None, SHA256SUMS.
    fn release(fx: &mut Fixture, tag: &str, id: &str, version: &str, sums: Option<String>, pre: bool) -> Value {
        let name = asset_name(id, version, PLATFORM);
        let bytes = mod_zip(id, version);
        fx.put(&format!("{DL}/{tag}/{name}"), bytes.clone());
        let mut assets = vec![serde_json::json!({"name": name, "browser_download_url": format!("{DL}/{tag}/{name}")})];
        if let Some(sums) = sums {
            let text = if sums.is_empty() { format!("{}  {name}\n", sha(&bytes)) } else { sums };
            fx.put(&format!("{DL}/{tag}/SHA256SUMS"), text);
            assets.push(
                serde_json::json!({"name": "SHA256SUMS", "browser_download_url": format!("{DL}/{tag}/SHA256SUMS")}),
            );
        }
        serde_json::json!({"tag_name": tag, "draft": false, "prerelease": pre, "assets": assets})
    }

    fn ctx<'a>(dir: &'a Path, fetch: &'a dyn Fetch) -> Context<'a> {
        Context { dir, game: "rac1", version: "pal", platform: Some(PLATFORM), fetch, hosts: hosts() }
    }

    #[test]
    fn the_index_release_is_never_a_mods_release() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        // `index` comes first and is not marked pre-release here: skipped by its name all the same.
        let mut index = release(&mut fx, "index", "m", "9.9.9", Some(String::new()), false);
        index["assets"]
            .as_array_mut()
            .unwrap()
            .push(serde_json::json!({"name": "index.json", "browser_download_url": "x"}));
        let real = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        let files = [("openrac-mod.json", manifest("m", "1.0.0"))];
        repo(&mut fx, &files, serde_json::json!([index, real]), "c1");
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        assert_eq!(p.reference, "v1.0.0");
        assert_eq!(p.mods[0].tag.as_deref(), Some("v1.0.0"));
    }

    #[test]
    fn addresses() {
        let gh = |o: &str, r: &str, reference: Option<&str>, tag: Option<&str>| Source::GitHub {
            owner: o.into(),
            repo: r.into(),
            reference: reference.map(Into::into),
            tag: tag.map(Into::into),
        };
        assert_eq!(
            parse_url("https://github.com/OpenRAC/openrac-mods").unwrap(),
            gh("OpenRAC", "openrac-mods", None, None)
        );
        assert_eq!(
            parse_url("github.com/OpenRAC/openrac-mods.git/").unwrap(),
            gh("OpenRAC", "openrac-mods", None, None)
        );
        assert_eq!(parse_url("https://www.github.com/a/b/tree/dev/x").unwrap(), gh("a", "b", Some("dev/x"), None));
        assert_eq!(
            parse_url("https://github.com/a/b/releases/tag/v1.2.0").unwrap(),
            gh("a", "b", None, Some("v1.2.0"))
        );
        assert_eq!(parse_url("https://github.com/a/b/releases/latest").unwrap(), gh("a", "b", None, None));
        assert!(parse_url("https://github.com/a").is_err());
        assert!(parse_url("https://github.com/a/b/issues/3").is_err());
        assert!(parse_url("https://github.com/a/../b").is_err());
        assert_eq!(
            parse_url("https://gitlab.com/x/y.git").unwrap(),
            Source::Git { url: "https://gitlab.com/x/y.git".into() }
        );
        assert_eq!(
            parse_url("git@codeberg.org:x/y.git").unwrap(),
            Source::Git { url: "git@codeberg.org:x/y.git".into() }
        );
        assert!(parse_url("not a url").is_err());
        assert!(parse_url("").is_err());
    }

    #[test]
    fn every_common_link_form() {
        let plain = Source::GitHub { owner: "OpenRAC".into(), repo: "openrac-mods".into(), reference: None, tag: None };
        for text in [
            "https://github.com/OpenRAC/openrac-mods",
            "http://github.com/OpenRAC/openrac-mods",
            "github.com/OpenRAC/openrac-mods",
            "  https://github.com/OpenRAC/openrac-mods/  ",
            "https://github.com/OpenRAC/openrac-mods.git",
            "https://github.com/OpenRAC/openrac-mods?tab=readme",
            "git@github.com:OpenRAC/openrac-mods.git",
            "git@github.com:OpenRAC/openrac-mods",
            "ssh://git@github.com/OpenRAC/openrac-mods.git",
            "OpenRAC/openrac-mods",
        ] {
            assert_eq!(parse_url(text).unwrap(), plain, "{text}");
        }
        assert_eq!(
            parse_url("https://github.com/OpenRAC/openrac-mods/tree/master/lock-strafe").unwrap(),
            Source::GitHub {
                owner: "OpenRAC".into(),
                repo: "openrac-mods".into(),
                reference: Some("master/lock-strafe".into()),
                tag: None
            }
        );
        assert_eq!(
            parse_url("https://github.com/OpenRAC/openrac-mods/releases/tag/v1.0.0").unwrap(),
            Source::GitHub {
                owner: "OpenRAC".into(),
                repo: "openrac-mods".into(),
                reference: None,
                tag: Some("v1.0.0".into())
            }
        );
        // Not a shorthand: a host's name, or a path deeper than owner/repo.
        assert!(matches!(parse_url("gitlab.com/x").unwrap_err(), e if e.contains("not a repository")));
        assert!(parse_url("a/b/c").is_err());
    }

    #[test]
    fn a_link_to_a_folder_shows_its_mods_only() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let files = [
            ("lock-strafe/openrac-mod.json", manifest("lock-strafe", "1.0.0")),
            ("button-swap/openrac-mod.json", script_manifest("button-swap", "1.0.0")),
        ];
        repo(&mut fx, &files, serde_json::json!([]), "c1");
        // The branch is the first part of the path the repository knows; the rest is the folder.
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r/tree/main/button-swap").unwrap();
        assert_eq!(p.reference, "main");
        assert_eq!(p.mods.iter().map(|m| m.id.as_str()).collect::<Vec<_>>(), ["button-swap"]);
        // A branch whose name holds a slash: the longer start is tried when the shorter is unknown.
        fx.put(&format!("{API}/repos/o/r/commits/feature/x"), serde_json::json!({"sha": "c1"}).to_string());
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r/tree/feature/x/lock-strafe").unwrap();
        assert_eq!(p.reference, "feature/x");
        assert_eq!(p.mods.iter().map(|m| m.id.as_str()).collect::<Vec<_>>(), ["lock-strafe"]);
        let err = preview(&ctx(dir.path(), &fx), "https://github.com/o/r/tree/main/nothing-here").unwrap_err();
        assert!(err.contains("no mod in nothing-here"), "{err}");
    }

    #[test]
    fn a_repository_github_does_not_show_is_said_plainly() {
        let dir = tempfile::tempdir().unwrap();
        let fx = Fixture::default();
        let err = preview(&ctx(dir.path(), &fx), "https://github.com/someone/private-thing").unwrap_err();
        assert_eq!(err, NOT_FOUND);
    }

    #[test]
    fn discovery() {
        let p = |v: &[&str]| v.iter().map(|s| s.to_string()).collect::<Vec<_>>();
        assert_eq!(discover(&p(&["openrac-mod.json", "a/openrac-mod.json"]), None).unwrap(), ["openrac-mod.json"]);
        assert_eq!(
            discover(&p(&["b/openrac-mod.json", "a/openrac-mod.json", "a/x/openrac-mod.json", "README.md"]), None)
                .unwrap(),
            ["a/openrac-mod.json", "b/openrac-mod.json"]
        );
        let index = serde_json::json!({"format": 1, "mods": ["b"]});
        assert_eq!(
            discover(&p(&["b/openrac-mod.json", "a/openrac-mod.json"]), Some(&index)).unwrap(),
            ["b/openrac-mod.json"]
        );
        assert!(discover(&p(&["a/openrac-mod.json"]), Some(&index)).unwrap_err().contains("names b"));
        assert!(discover(&p(&["README.md"]), None).unwrap_err().contains("No OpenRAC mod"));
        let bad = serde_json::json!({"mods": ["../x"]});
        assert!(discover(&p(&["../x/openrac-mod.json"]), Some(&bad)).is_err());
    }

    #[test]
    fn a_manifest_in_its_sources_is_a_native_mod() {
        // As a repository holds it: no `plugin` yet (the release's archive adds it).
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let rel = release(&mut fx, "v1.0.0", "lock-strafe", "1.0.0", Some(String::new()), false);
        let source = r#"{"format": 2, "id": "lock-strafe", "name": "Lock-strafe", "version": "1.0.0",
            "author": "A", "api": "1.1", "games": {"rac1": ["pal", "ntsc"]}}"#;
        repo(&mut fx, &[("lock-strafe/openrac-mod.json", source.to_string())], serde_json::json!([rel]), "c1");
        let m = &preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap().mods[0];
        assert_eq!(m.kind, "native");
        assert!(m.compatible, "{:?}", m.why);
        assert_eq!(m.asset.as_deref(), Some("lock-strafe-1.0.0-linux-x86_64.zip"));
        // No platform for native mods: refused, with the reason.
        let none = Context { platform: None, ..ctx(dir.path(), &fx) };
        let m = &preview(&none, "https://github.com/o/r").unwrap().mods[0];
        assert!(!m.compatible && m.why.as_deref().unwrap().contains("not loaded"), "{:?}", m.why);
    }

    #[test]
    fn asset_names_per_platform() {
        assert_eq!(asset_name("lock-strafe", "1.0.0", "macos-arm64"), "lock-strafe-1.0.0-macos-arm64.zip");
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let rel = release(&mut fx, "v1.0.0", "lock-strafe", "1.0.0", Some(String::new()), false);
        repo(
            &mut fx,
            &[("lock-strafe/openrac-mod.json", manifest("lock-strafe", "1.0.0"))],
            serde_json::json!([rel]),
            "c1",
        );
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        let m = &p.mods[0];
        assert!(m.compatible, "{:?}", m.why);
        assert_eq!(m.asset.as_deref(), Some("lock-strafe-1.0.0-linux-x86_64.zip"));
        assert_eq!(m.tag.as_deref(), Some("v1.0.0"));
        assert_eq!((p.label.as_str(), p.owner.as_str(), m.kind.as_str()), ("o/r", "o", "native"));
        // On Windows the manifest has a library but the release has no asset for it.
        let win = Context { platform: Some("windows-x86_64"), ..ctx(dir.path(), &fx) };
        let m = &preview(&win, "https://github.com/o/r").unwrap().mods[0];
        assert!(!m.compatible);
        assert!(m.why.as_deref().unwrap().contains("lock-strafe-1.0.0-windows-x86_64.zip"), "{:?}", m.why);
        // macOS: the manifest names no library for it.
        let mac = Context { platform: Some("macos-arm64"), ..ctx(dir.path(), &fx) };
        assert!(preview(&mac, "https://github.com/o/r").unwrap().mods[0].why.as_deref().unwrap().contains("Not built"));
        // No native mods at all (a system with no platform key).
        let none = Context { platform: None, ..ctx(dir.path(), &fx) };
        assert!(!preview(&none, "https://github.com/o/r").unwrap().mods[0].compatible);
    }

    #[test]
    fn no_release_yet() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "0.1.0"))], serde_json::json!([]), "c1");
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        assert_eq!(p.note.as_deref(), Some("This repository has no release yet."));
        assert_eq!(p.reference, "HEAD");
        assert!(p.mods[0].why.as_deref().unwrap().starts_with("No release yet"));
        assert!(install(&ctx(dir.path(), &fx), "https://github.com/o/r", "m").is_err());
    }

    #[test]
    fn a_v_in_the_archives_name_is_named() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let with_v = asset_name("m", "v1.0.0", PLATFORM);
        let rel = serde_json::json!({"tag_name": "v1.0.0", "draft": false, "prerelease": false,
            "assets": [{"name": with_v, "browser_download_url": format!("{DL}/v1.0.0/{with_v}")}]});
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.0.0"))], serde_json::json!([rel]), "c1");
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        let why = p.mods[0].why.as_deref().unwrap();
        assert!(why.contains(&with_v), "{why}");
        assert!(why.contains(&asset_name("m", "1.0.0", PLATFORM)), "{why}");
        assert!(why.contains("no \"v\" before the version"), "{why}");
        assert!(!p.mods[0].compatible);
    }

    #[test]
    fn installs_and_remembers_the_source() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let rel = release(&mut fx, "v1.0.0", "lock-strafe", "1.0.0", Some(String::new()), false);
        repo(
            &mut fx,
            &[("lock-strafe/openrac-mod.json", manifest("lock-strafe", "1.0.0"))],
            serde_json::json!([rel]),
            "c1",
        );
        let c = ctx(dir.path(), &fx);
        let got = install(&c, "https://github.com/o/r/", "lock-strafe").unwrap();
        assert!(
            matches!(got, RemoteInstalled::Plugin { ref id, ref version, .. } if id == "lock-strafe" && version == "1.0.0")
        );
        assert!(dir.path().join("lock-strafe/openrac-mod.json").is_file());
        let s = read_sources(dir.path());
        let entry = &s.mods["lock-strafe"];
        assert_eq!(
            (entry.url.as_str(), entry.path.as_str(), entry.version.as_str(), entry.tag.as_deref(), entry.pin),
            ("https://github.com/o/r", "lock-strafe", "1.0.0", Some("v1.0.0"), false)
        );
        // The preview now says it is installed.
        assert_eq!(preview(&c, "https://github.com/o/r").unwrap().mods[0].installed.as_deref(), Some("1.0.0"));
        // sources.json round trip, pin, forget.
        set_pin(dir.path(), "lock-strafe", true).unwrap();
        assert!(read_sources(dir.path()).mods["lock-strafe"].pin);
        forget(dir.path(), "lock-strafe").unwrap();
        assert!(read_sources(dir.path()).mods.is_empty());
        assert!(set_pin(dir.path(), "lock-strafe", true).is_err());
    }

    #[test]
    fn github_digest_checks_the_download() {
        let (id, zero) = ("m", "0".repeat(64));
        let name = asset_name(id, "1.0.0", PLATFORM);
        let bytes = mod_zip(id, "1.0.0");
        let good = sha(&bytes);
        let other_line = format!("{zero}  other.zip\n");
        let wrong_line = format!("{zero}  {name}\n");
        for (digest, sums, want) in [
            (good.as_str(), None, None),                                // no SHA256SUMS needed
            (good.as_str(), Some(other_line), None),                    // not listed: the digest is enough
            (zero.as_str(), None, Some("SHA-256 differs")),             // the digest is checked
            (good.as_str(), Some(wrong_line), Some("SHA-256 differs")), // both must match
        ] {
            let dir = tempfile::tempdir().unwrap();
            let mut fx = Fixture::default();
            let mut rel = release(&mut fx, "v1.0.0", id, "1.0.0", sums, false);
            fx.put(&format!("{DL}/v1.0.0/{name}"), bytes.clone());
            rel["assets"][0]["digest"] = format!("sha256:{}", digest.to_ascii_uppercase()).into();
            repo(&mut fx, &[("openrac-mod.json", manifest(id, "1.0.0"))], serde_json::json!([rel]), "c1");
            let c = ctx(dir.path(), &fx);
            let got = install(&c, "https://github.com/o/r", id);
            match want {
                None => {
                    got.unwrap();
                }
                Some(want) => assert!(got.unwrap_err().contains(want)),
            }
        }
    }

    #[test]
    fn checksums_refuse() {
        let id = "m";
        for (sums, want) in [
            (None, "has no SHA256SUMS"),
            (Some(format!("{}  other.zip\n", "0".repeat(64))), "does not list"),
            (Some(format!("{} *{}\n", "0".repeat(64), asset_name(id, "1.0.0", PLATFORM))), "SHA-256 differs"),
        ] {
            let dir = tempfile::tempdir().unwrap();
            let mut fx = Fixture::default();
            let missing = sums.is_none();
            let rel = release(&mut fx, "v1.0.0", id, "1.0.0", sums, false);
            repo(&mut fx, &[("openrac-mod.json", manifest(id, "1.0.0"))], serde_json::json!([rel]), "c1");
            let c = ctx(dir.path(), &fx);
            let err = install(&c, "https://github.com/o/r", id).unwrap_err();
            if missing {
                // Shown as not installable before anything is downloaded.
                assert!(err.contains("no SHA256SUMS"), "{err}");
            } else {
                assert!(err.contains(want), "{err}");
            }
            assert!(!dir.path().join(id).exists());
        }
        // The parser: binary marker, upper case, junk lines.
        let s = parse_sums(&format!("{}  a.zip\n{} *b.zip\nnonsense\n", "A".repeat(64), "b".repeat(64)));
        assert_eq!(s["a.zip"], "a".repeat(64));
        assert_eq!(s["b.zip"], "b".repeat(64));
        assert_eq!(s.len(), 2);
    }

    #[test]
    fn a_download_that_is_another_mod_is_refused() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let name = asset_name("m", "1.0.0", PLATFORM);
        let bytes = mod_zip("other", "9.9.9");
        fx.put(&format!("{DL}/v1.0.0/{name}"), bytes.clone());
        fx.put(&format!("{DL}/v1.0.0/SHA256SUMS"), format!("{}  {name}\n", sha(&bytes)));
        let rel = serde_json::json!({"tag_name": "v1.0.0", "assets": [
            {"name": name, "browser_download_url": format!("{DL}/v1.0.0/{name}")},
            {"name": "SHA256SUMS", "browser_download_url": format!("{DL}/v1.0.0/SHA256SUMS")}]});
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.0.0"))], serde_json::json!([rel]), "c1");
        let err = install(&ctx(dir.path(), &fx), "https://github.com/o/r", "m").unwrap_err();
        assert!(err.contains("holds other 9.9.9"), "{err}");
        assert!(!dir.path().join("m").exists() && !dir.path().join("other").exists());
    }

    #[test]
    fn a_data_pack_from_the_repository_archive() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let pack = r#"{"format": 1, "game": "rac1", "version": "pal", "name": "Shiny bolts", "author": "A"}"#;
        repo(
            &mut fx,
            &[("shiny/openrac-mod.json", pack.to_string()), ("shiny/level_00/textures/x.png", "png".into())],
            serde_json::json!([]),
            "c1",
        );
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file("o-r-c1/shiny/openrac-mod.json", o).unwrap();
        w.write_all(pack.as_bytes()).unwrap();
        w.start_file("o-r-c1/shiny/level_00/textures/x.png", o).unwrap();
        w.write_all(b"png").unwrap();
        w.start_file("o-r-c1/README.md", o).unwrap();
        fx.put(&format!("{WEB}/o/r/archive/c1.zip"), w.finish().unwrap().into_inner());
        let c = ctx(dir.path(), &fx);
        let p = preview(&c, "https://github.com/o/r").unwrap();
        assert_eq!((p.mods[0].id.as_str(), p.mods[0].kind.as_str(), p.mods[0].compatible), ("shiny", "pack", true));
        let RemoteInstalled::Pack { path, .. } = install(&c, "https://github.com/o/r", "shiny").unwrap() else {
            panic!("not a pack")
        };
        let path = PathBuf::from(path);
        assert!(path.join("openrac-mod.json").is_file() && path.join("level_00/textures/x.png").is_file());
        assert!(!path.join("README.md").exists(), "only the mod's folder");
        assert_eq!(read_sources(dir.path()).mods["shiny"].commit.as_deref(), Some("c1"));
        // A newer commit on the branch is an update for a pack with no release.
        let mut fx2 = Fixture::default();
        repo(&mut fx2, &[("shiny/openrac-mod.json", pack.to_string())], serde_json::json!([]), "c2");
        let (updates, errors) = check_updates(&ctx(dir.path(), &fx2));
        assert!(errors.is_empty(), "{errors:?}");
        assert_eq!(updates.len(), 1);
        assert_eq!(updates[0].commit.as_deref(), Some("c2"));
        // A pack for another game is not offered.
        let mut fx3 = Fixture::default();
        repo(&mut fx3, &[("openrac-mod.json", pack.replace("rac1", "rac2"))], serde_json::json!([]), "c1");
        assert!(!preview(&ctx(dir.path(), &fx3), "https://github.com/o/r").unwrap().mods[0].compatible);
    }

    fn script_manifest(id: &str, version: &str) -> String {
        format!(
            r#"{{"format": 2, "id": "{id}", "name": "Mod {id}", "version": "{version}", "author": "Someone",
                "type": "script", "scripts": ["main.luau"], "api": "1.1", "games": {{"rac1": ["pal", "ntsc"]}}}}"#
        )
    }

    /// The repository's archive at `commit`, holding a script mod in `folder`.
    fn script_zipball(fx: &mut Fixture, commit: &str, folder: &str, id: &str, version: &str) {
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file(format!("o-r-{commit}/{folder}/openrac-mod.json"), o).unwrap();
        w.write_all(script_manifest(id, version).as_bytes()).unwrap();
        w.start_file(format!("o-r-{commit}/{folder}/main.luau"), o).unwrap();
        w.write_all(b"openrac.log('hi')\n").unwrap();
        w.start_file(format!("o-r-{commit}/README.md"), o).unwrap();
        fx.put(&format!("{WEB}/o/r/archive/{commit}.zip"), w.finish().unwrap().into_inner());
    }

    #[test]
    fn a_script_mod_from_the_repository_itself() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        // No release: the repository's own files are the mod.
        repo(
            &mut fx,
            &[("swap/openrac-mod.json", script_manifest("swap", "1.0.0")), ("swap/main.luau", "-- x".into())],
            serde_json::json!([]),
            "c1",
        );
        script_zipball(&mut fx, "c1", "swap", "swap", "1.0.0");
        // On a system with no native mods (no platform key) it is offered all the same.
        let android = Context { platform: None, ..ctx(dir.path(), &fx) };
        let p = preview(&android, "https://github.com/o/r").unwrap();
        let m = &p.mods[0];
        assert_eq!((m.id.as_str(), m.kind.as_str(), m.compatible, m.asset.as_deref()), ("swap", "script", true, None));
        let got = install(&android, "https://github.com/o/r", "swap").unwrap();
        assert!(matches!(got, RemoteInstalled::Plugin { ref id, enabled: true, .. } if id == "swap"));
        assert!(dir.path().join("swap/main.luau").is_file());
        assert!(!dir.path().join("swap/README.md").exists(), "only the mod's folder");
        let s = &read_sources(dir.path()).mods["swap"];
        assert_eq!((s.kind.as_str(), s.commit.as_deref(), s.tag.as_deref()), ("script", Some("c1"), None));
        // A newer commit on the branch is an update, and installs as one.
        let mut fx2 = Fixture::default();
        repo(&mut fx2, &[("swap/openrac-mod.json", script_manifest("swap", "1.0.0"))], serde_json::json!([]), "c2");
        script_zipball(&mut fx2, "c2", "swap", "swap", "1.0.0");
        let c2 = ctx(dir.path(), &fx2);
        let (u, errors) = check_updates(&c2);
        assert!(errors.is_empty(), "{errors:?}");
        assert_eq!((u.len(), u[0].commit.as_deref()), (1, Some("c2")));
        assert!(matches!(update(&c2, "swap").unwrap(), RemoteInstalled::Plugin { replaced: true, .. }));
        assert_eq!(read_sources(dir.path()).mods["swap"].commit.as_deref(), Some("c2"));
        assert!(check_updates(&c2).0.is_empty());
        // A script mod for an interface this launcher's port does not have is not offered.
        let mut fx3 = Fixture::default();
        repo(
            &mut fx3,
            &[("openrac-mod.json", script_manifest("swap", "1.0.0").replace("\"1.1\"", "\"1.99\""))],
            serde_json::json!([]),
            "c1",
        );
        let p = preview(&ctx(dir.path(), &fx3), "https://github.com/o/r").unwrap();
        assert!(!p.mods[0].compatible && p.mods[0].why.as_deref().unwrap().contains("interface 1.99"));
    }

    #[test]
    fn a_script_mod_from_its_release() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        // The joined archive package_mod.py writes for a script mod, listed in SHA256SUMS.
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file("swap-1.0.0/openrac-mod.json", o).unwrap();
        w.write_all(script_manifest("swap", "1.0.0").as_bytes()).unwrap();
        w.start_file("swap-1.0.0/main.luau", o).unwrap();
        w.write_all(b"-- released").unwrap();
        let bytes = w.finish().unwrap().into_inner();
        let name = "swap-1.0.0.zip";
        fx.put(&format!("{DL}/v1.0.0/{name}"), bytes.clone());
        let release = serde_json::json!({"tag_name": "v1.0.0", "draft": false, "prerelease": false, "assets": [
            {"name": name, "browser_download_url": format!("{DL}/v1.0.0/{name}")},
            {"name": "SHA256SUMS", "browser_download_url": format!("{DL}/v1.0.0/SHA256SUMS")}]});
        fx.put(&format!("{DL}/v1.0.0/SHA256SUMS"), format!("{}  {name}\n", sha(&bytes)));
        repo(&mut fx, &[("openrac-mod.json", script_manifest("swap", "1.0.0"))], serde_json::json!([release]), "c1");
        let c = ctx(dir.path(), &fx);
        let p = preview(&c, "https://github.com/o/r").unwrap();
        assert_eq!((p.mods[0].asset.as_deref(), p.mods[0].tag.as_deref()), (Some(name), Some("v1.0.0")));
        assert!(matches!(install(&c, "https://github.com/o/r", "swap").unwrap(), RemoteInstalled::Plugin { .. }));
        assert_eq!(std::fs::read_to_string(dir.path().join("swap/main.luau")).unwrap(), "-- released");
        assert_eq!(read_sources(dir.path()).mods["swap"].tag.as_deref(), Some("v1.0.0"));
        // A hash that differs is refused.
        fx.put(&format!("{DL}/v1.0.0/SHA256SUMS"), format!("{}  {name}\n", "0".repeat(64)));
        let refused = install(&ctx(dir.path(), &fx), "https://github.com/o/r", "swap").unwrap_err();
        assert!(refused.contains("SHA-256") || refused.contains("differs"), "{refused}");
    }

    #[test]
    fn updates() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let r1 = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.0.0"))], serde_json::json!([r1.clone()]), "c1");
        install(&ctx(dir.path(), &fx), "https://github.com/o/r", "m").unwrap();
        assert!(check_updates(&ctx(dir.path(), &fx)).0.is_empty(), "nothing newer yet");

        // 1.1.0 released; 2.0.0 only as a prerelease, which is not offered.
        let mut fx = Fixture::default();
        let r1 = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        let r2 = release(&mut fx, "v1.1.0", "m", "1.1.0", Some(String::new()), false);
        let pre = release(&mut fx, "v2.0.0", "m", "2.0.0", Some(String::new()), true);
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.1.0"))], serde_json::json!([pre, r2, r1]), "c2");
        let c = ctx(dir.path(), &fx);
        let (u, errors) = check_updates(&c);
        assert!(errors.is_empty(), "{errors:?}");
        assert_eq!(
            u,
            [Update {
                id: "m".into(),
                installed: "1.0.0".into(),
                available: "1.1.0".into(),
                tag: Some("v1.1.0".into()),
                commit: None
            }]
        );
        // Pinned: nothing offered, and update refused.
        set_pin(dir.path(), "m", true).unwrap();
        assert!(check_updates(&c).0.is_empty());
        assert!(update(&c, "m").is_err());
        set_pin(dir.path(), "m", false).unwrap();
        let got = update(&c, "m").unwrap();
        assert!(matches!(got, RemoteInstalled::Plugin { ref version, replaced: true, .. } if version == "1.1.0"));
        assert_eq!(read_sources(dir.path()).mods["m"].version, "1.1.0");
        assert!(check_updates(&c).0.is_empty());
        // An older release is not an update.
        let mut fx = Fixture::default();
        let r1 = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.0.0"))], serde_json::json!([r1]), "c1");
        assert!(check_updates(&ctx(dir.path(), &fx)).0.is_empty());
        // Two mods from one repository: it is read once, and its failure said once for both.
        let mut sources = read_sources(dir.path());
        let same = sources.mods["m"].clone();
        sources.mods.insert("n".into(), same);
        write_sources(dir.path(), &sources).unwrap();
        fx.asked.borrow_mut().clear();
        assert!(check_updates(&ctx(dir.path(), &fx)).0.is_empty());
        assert_eq!(fx.asked.borrow().iter().filter(|u| u.contains("/releases?")).count(), 1, "one read per address");
        // A source that cannot be read is said, not fatal.
        let empty = Fixture::default();
        let (u, errors) = check_updates(&ctx(dir.path(), &empty));
        assert!(u.is_empty());
        assert_eq!(errors.len(), 1, "{errors:?}");
        assert!(errors[0].starts_with("m, n: "), "{errors:?}");
    }

    /// A repository o/r of `ids` mods, each in its folder with a release `<id>-v<version>` (as
    /// openrac-mods is), all at commit `commit`.
    fn many(fx: &mut Fixture, ids: &[&str], version: &str, commit: &str) {
        let mut releases = Vec::new();
        let mut files = Vec::new();
        for id in ids {
            let tag = format!("{id}-v{version}");
            releases.push(release(fx, &tag, id, version, Some(String::new()), false));
            fx.put(&format!("{API}/repos/o/r/commits/{tag}"), serde_json::json!({"sha": commit}).to_string());
            files.push((format!("{id}/openrac-mod.json"), manifest(id, version)));
        }
        let files: Vec<(&str, String)> = files.iter().map(|(p, t)| (p.as_str(), t.clone())).collect();
        repo(fx, &files, Value::Array(releases), commit);
    }

    /// Checking a repository of several mods and updating each reads GitHub's API once, through
    /// the launcher's cache: not once per mod, which spent the anonymous 60 an hour.
    #[test]
    fn updating_every_mod_of_a_repository_reads_it_once() {
        let ids = ["a", "b", "c", "d", "e"];
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        many(&mut fx, &ids, "1.0.0", "c1");
        for id in ids {
            install(&ctx(dir.path(), &fx), "https://github.com/o/r", id).unwrap();
        }
        let api = |fx: &Fixture| fx.asked.borrow().iter().filter(|u| u.starts_with(API)).count();
        let update_all = |fetch: &dyn Fetch| {
            let c = ctx(dir.path(), fetch);
            let (found, errors) = check_updates(&c);
            assert!(errors.is_empty(), "{errors:?}");
            assert_eq!(found.len(), ids.len());
            for u in found {
                update(&c, &u.id).unwrap();
            }
        };
        // Without the cache: the look reads the repository (its releases, the commit, the tree),
        // and every update reads it again.
        let mut fx = Fixture::default();
        many(&mut fx, &ids, "1.1.0", "c2");
        update_all(&fx);
        assert_eq!(api(&fx), 3 * (1 + ids.len()));
        // With it: three requests in all.
        for id in ids {
            let mut sources = read_sources(dir.path());
            sources.mods.get_mut(id).unwrap().version = "1.0.0".into();
            write_sources(dir.path(), &sources).unwrap();
        }
        fx.asked.borrow_mut().clear();
        let cache = ApiCache::new(std::time::Duration::from_secs(60));
        update_all(&Cached { inner: &fx, cache: &cache });
        assert_eq!(api(&fx), 3, "{:#?}", fx.asked.borrow());
        assert_eq!(read_sources(dir.path()).mods["e"].version, "1.1.0");
        // An answer past its time is asked again; a failure is never kept.
        let stale = ApiCache::new(std::time::Duration::ZERO);
        let once = Cached { inner: &fx, cache: &stale };
        let url = format!("{API}/repos/o/r/releases?per_page=100");
        fx.asked.borrow_mut().clear();
        once.get(&url, true).unwrap();
        once.get(&url, true).unwrap();
        assert_eq!(api(&fx), 2);
        let missing = format!("{API}/repos/o/missing");
        let cached = Cached { inner: &fx, cache: &cache };
        assert!(cached.get(&missing, true).is_err() && cached.get(&missing, true).is_err());
        assert_eq!(fx.asked.borrow().iter().filter(|u| **u == missing).count(), 2);
        // Downloads go around it.
        fx.asked.borrow_mut().clear();
        let dl = format!("{DL}/a-v1.1.0/SHA256SUMS");
        cached.get(&dl, false).unwrap();
        cached.get(&dl, false).unwrap();
        assert_eq!(fx.asked.borrow().len(), 2);
    }

    /// curl's `-D -` output: the redirects' headers skipped, the last answer's kept.
    #[test]
    fn the_answer_after_its_redirects() {
        let out = b"HTTP/2 302\r\nlocation: https://x\r\n\r\nHTTP/1.1 200 Connection established\r\n\r\nHTTP/2 200\r\nETag: \"e\"\r\n\r\nHTTP/ body";
        let (status, headers, body) = split_response(out);
        assert_eq!((status, body), (200, &b"HTTP/ body"[..]));
        assert_eq!(headers, [("etag".to_string(), "\"e\"".to_string())]);
        let (status, _, body) = split_response(b"HTTP/1.1 404 Not Found\r\n\r\n{}");
        assert_eq!((status, body), (404, &b"{}"[..]));
        assert_eq!(split_response(b"no headers").0, 0);
    }

    /// GitHub's limit said plainly, with when it lets the launcher ask again.
    #[test]
    fn the_rate_limit_is_said_plainly() {
        let h = |pairs: &[(&str, &str)]| -> Vec<(String, String)> {
            pairs.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect()
        };
        let url = "https://api.github.com/repos/OpenRAC/openrac-mods/releases?per_page=100";
        let spent = h(&[("x-ratelimit-limit", "60"), ("x-ratelimit-remaining", "0"), ("x-ratelimit-reset", "10000")]);
        assert_eq!(
            http_error(url, 403, &spent, false, 10000 - 23 * 60 + 5),
            "GitHub's limit of 60 requests an hour from this network is used up; try again in 23 minutes"
        );
        assert_eq!(
            http_error(url, 403, &spent, true, 9990),
            "GitHub's limit of 60 requests an hour for your GitHub token is used up; try again in a minute"
        );
        assert_eq!(
            http_error(url, 429, &h(&[("retry-after", "120")]), false, 0),
            "GitHub asked the launcher to slow down; try again in 2 minutes"
        );
        // Another refusal is not the limit; a missing file is "not found", as before.
        assert_eq!(
            http_error(url, 403, &h(&[("x-ratelimit-remaining", "59")]), false, 0),
            format!("{url} could not be fetched (HTTP 403)")
        );
        assert_eq!(http_error(url, 404, &[], false, 0), format!("{url}: not found"));
    }

    #[test]
    fn a_named_release() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let r1 = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        repo(&mut fx, &[("openrac-mod.json", manifest("m", "1.0.0"))], serde_json::json!([r1]), "c1");
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r/releases/tag/v1.0.0").unwrap();
        assert_eq!(p.reference, "v1.0.0");
        assert!(preview(&ctx(dir.path(), &fx), "https://github.com/o/r/releases/tag/v9")
            .unwrap_err()
            .contains("no release v9"));
    }

    /// A code's review and install read the release's archive, not the repository's manifest: one
    /// that says script while its release ships a library is shown native, and an archive named
    /// as a script's that holds a library is refused.
    #[test]
    fn a_pinned_mod_is_what_its_release_holds() {
        let script = r#"{"format": 2, "id": "m", "name": "Mod m", "version": "1.0.0", "author": "Someone",
            "type": "script", "scripts": ["main.luau"], "api": "1.1", "games": {"rac1": ["pal", "ntsc"]}}"#;
        let pin = |sums: &[u8]| Pin {
            url: "https://github.com/o/r".into(),
            path: String::new(),
            id: "m".into(),
            version: "1.0.0".into(),
            tag: Some("v1.0.0".into()),
            commit: None,
            sums: Some(sha(sums)),
        };
        // A native build in the release: shown native, and it installs.
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let rel = release(&mut fx, "v1.0.0", "m", "1.0.0", Some(String::new()), false);
        fx.put(&format!("{API}/repos/o/r/releases/tags/v1.0.0"), rel.to_string());
        fx.put(&format!("{RAW}/o/r/v1.0.0/openrac-mod.json"), script);
        let sums = fx.files[&format!("{DL}/v1.0.0/SHA256SUMS")].clone();
        let c = ctx(dir.path(), &fx);
        let pinned = read_pinned(&c, &pin(&sums)).unwrap();
        assert_eq!(pinned.kind, "native");
        assert!(stage_pinned(&c, &pinned).is_ok());
        // The script's archive name, a library inside: refused before anything is installed.
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let bytes = mod_zip("m", "1.0.0");
        let name = "m-1.0.0.zip";
        let text = format!("{}  {name}\n", sha(&bytes));
        fx.put(&format!("{DL}/v1.0.0/{name}"), bytes);
        fx.put(&format!("{DL}/v1.0.0/SHA256SUMS"), text.clone());
        let rel = serde_json::json!({"tag_name": "v1.0.0", "draft": false, "prerelease": false, "assets": [
            {"name": name, "browser_download_url": format!("{DL}/v1.0.0/{name}")},
            {"name": "SHA256SUMS", "browser_download_url": format!("{DL}/v1.0.0/SHA256SUMS")}]});
        fx.put(&format!("{API}/repos/o/r/releases/tags/v1.0.0"), rel.to_string());
        fx.put(&format!("{RAW}/o/r/v1.0.0/openrac-mod.json"), script);
        let c = ctx(dir.path(), &fx);
        let pinned = read_pinned(&c, &pin(text.as_bytes())).unwrap();
        assert_eq!(pinned.kind, "script");
        let err = stage_pinned(&c, &pinned).err().unwrap();
        assert!(err.contains("native mod, not the script"), "{err}");
    }

    /// Another host, through git: a local repository stands for it. Skipped without git.
    #[test]
    fn another_host_through_git() {
        if !git_found() {
            eprintln!("git is not installed: skipped");
            return;
        }
        let dir = tempfile::tempdir().unwrap();
        let src = dir.path().join("src");
        std::fs::create_dir_all(src.join("shiny/level_00")).unwrap();
        let pack = r#"{"format": 1, "game": "rac1", "version": "pal", "name": "Shiny", "author": "A"}"#;
        std::fs::write(src.join("shiny/openrac-mod.json"), pack).unwrap();
        std::fs::write(src.join("shiny/level_00/x.png"), "png").unwrap();
        std::fs::create_dir_all(src.join("native")).unwrap();
        std::fs::write(src.join("native/openrac-mod.json"), manifest("native", "1.0.0")).unwrap();
        let run = |args: &[&str]| git(args, Some(&src)).unwrap();
        run(&["init", "-q"]);
        run(&["-c", "user.name=t", "-c", "user.email=t@t", "add", "."]);
        run(&["-c", "user.name=t", "-c", "user.email=t@t", "commit", "-q", "-m", "x"]);
        // A file:// address stands for another host's https one.
        let url = format!("file://{}", src.display());
        let plugins = dir.path().join("plugins");
        let fx = Fixture::default();
        let c = ctx(&plugins, &fx);
        let (p, _scratch) = read_git(&url, c.parts()).unwrap();
        let shiny = p.mods.iter().find(|m| m.id == "shiny").unwrap();
        assert!(shiny.compatible);
        let native = p.mods.iter().find(|m| m.id == "native").unwrap();
        assert!(!native.compatible && native.why.as_deref().unwrap().contains("release"));
        assert!(fx.asked.borrow().is_empty(), "nothing fetched over https");
    }

    /// The real GitHub, once by hand: `cargo test -p openrac-launcher-core live_rac1_mods -- --ignored`.
    #[test]
    #[ignore = "network"]
    fn live_rac1_mods() {
        let dir = tempfile::tempdir().unwrap();
        let c = Context {
            dir: dir.path(),
            game: "rac1",
            version: "pal",
            platform: plugins::platform_key(),
            fetch: &Curl,
            hosts: Hosts::default(),
        };
        // A public repository with releases (the SDK's) and no mod at its top: the API's shapes
        // (releases, commits, trees) read, and discovery says there is none.
        let e = preview(&c, "https://github.com/OpenRAC/OpenRAC").unwrap_err();
        eprintln!("OpenRAC/OpenRAC: {e}");
        assert!(e.starts_with("No OpenRAC mod"), "{e}");
        // openrac-mods, once it is public, holds the lock-strafe.
        match preview(&c, "https://github.com/OpenRAC/openrac-mods") {
            Ok(p) => assert!(p.mods.iter().any(|m| m.id == "lock-strafe"), "{p:#?}"),
            Err(e) => eprintln!("OpenRAC/openrac-mods: {e}"),
        }
    }

    /// Two mods in folders a/ and b/, each released on its own tag: `releases` as listed, and the
    /// manifests at each commit.
    fn two_mods(fx: &mut Fixture, releases: Value, at: &[(&str, &str, [String; 2])]) {
        fx.put(&format!("{API}/repos/o/r/releases?per_page=100"), releases.to_string());
        let tree = serde_json::json!({"tree": [
            {"path": "a/openrac-mod.json", "type": "blob"}, {"path": "b/openrac-mod.json", "type": "blob"}]});
        for (tag, commit, [a, b]) in at {
            fx.put(&format!("{API}/repos/o/r/commits/{tag}"), serde_json::json!({"sha": commit}).to_string());
            fx.put(&format!("{API}/repos/o/r/git/trees/{commit}?recursive=1"), tree.to_string());
            for (at, (folder, text)) in [(*commit, ("a", a)), (*commit, ("b", b)), (*tag, ("a", a)), (*tag, ("b", b))] {
                fx.put(&format!("{RAW}/o/r/{at}/{folder}/openrac-mod.json"), text.clone());
            }
        }
    }

    fn dated(mut release: Value, created: &str) -> Value {
        release["created_at"] = created.into();
        release["published_at"] = created.into();
        release
    }

    #[test]
    fn manifests_are_read_at_the_newest_release_whatever_githubs_order() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        // b took PAL after a-v1.0.0 was made; GitHub lists the older release first.
        let old = dated(release(&mut fx, "a-v1.0.0", "a", "1.0.0", Some(String::new()), false), "2026-11-08T04:52:00Z");
        let new = dated(release(&mut fx, "b-v1.1.0", "b", "1.1.0", Some(String::new()), false), "2026-11-08T07:37:49Z");
        let ntsc_only = manifest("b", "1.0.0").replace(r#""pal", "#, "");
        two_mods(
            &mut fx,
            serde_json::json!([old, new]),
            &[
                ("a-v1.0.0", "c0", [manifest("a", "1.0.0"), ntsc_only]),
                ("b-v1.1.0", "c1", [manifest("a", "1.0.0"), manifest("b", "1.1.0")]),
            ],
        );
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        assert_eq!(p.reference, "b-v1.1.0");
        let b = p.mods.iter().find(|m| m.id == "b").unwrap();
        assert!(b.compatible, "{b:?}");
        assert_eq!((b.version.as_str(), b.tag.as_deref()), ("1.1.0", Some("b-v1.1.0")));
        let a = p.mods.iter().find(|m| m.id == "a").unwrap();
        assert_eq!((a.compatible, a.tag.as_deref()), (true, Some("a-v1.0.0")));
    }

    #[test]
    fn a_mod_bumped_but_not_released_yet_offers_its_own_newest_release() {
        let dir = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        // a-v2.0.0, the newest, has b at 1.2.0, whose release is not out: b's own b-v1.1.0 is.
        let b_rel =
            dated(release(&mut fx, "b-v1.1.0", "b", "1.1.0", Some(String::new()), false), "2026-11-08T05:00:00Z");
        let a_rel =
            dated(release(&mut fx, "a-v2.0.0", "a", "2.0.0", Some(String::new()), false), "2026-11-08T06:00:00Z");
        two_mods(
            &mut fx,
            serde_json::json!([b_rel, a_rel]),
            &[
                ("b-v1.1.0", "c1", [manifest("a", "1.0.0"), manifest("b", "1.1.0")]),
                ("a-v2.0.0", "c2", [manifest("a", "2.0.0"), manifest("b", "1.2.0")]),
            ],
        );
        let p = preview(&ctx(dir.path(), &fx), "https://github.com/o/r").unwrap();
        assert_eq!(p.reference, "a-v2.0.0");
        let b = p.mods.iter().find(|m| m.id == "b").unwrap();
        assert!(b.compatible, "{b:?}");
        assert_eq!((b.version.as_str(), b.tag.as_deref()), ("1.1.0", Some("b-v1.1.0")));
        let got = install(&ctx(dir.path(), &fx), "https://github.com/o/r", "b").unwrap();
        assert!(matches!(got, RemoteInstalled::Plugin { ref version, .. } if version == "1.1.0"), "{got:?}");
    }
}
