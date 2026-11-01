//! Mod codes: friends match their mods in one step. A host's enabled mods, at their exact versions
//! with the settings a party compares, become a *set*; a player enters its code, sees what would
//! change, and applies it (docs/port/MODS.md, section 12).
//!
//! A set travels two ways, read by one parser: a short code of 8 characters (`K7M2-9QXR`), which
//! names a set stored on the OpenRAC Server's HTTP side-port, and a long code (`ORMS1.` + base64url
//! of the zlib-compressed JSON), which is the set itself, for when there is no server.
//!
//! What keeps this safe. A set never carries an address to download from: `source.url` is only
//! which catalogue repository a mod comes from, on GitHub, and one outside OpenRAC's asks the player
//! first. A mod is pinned to a release by the hash of its `SHA256SUMS` (a retag is refused) or to a
//! commit; it is never replaced by a newer version. Nothing changes before the player's Apply, a
//! way back is kept ([`crate::profile`]), and a failure half way puts everything back. The code is
//! never trusted: every value is checked here, and again by the game's own mod check at join.

use std::collections::{BTreeMap, BTreeSet};
use std::io::{Read, Write};
use std::path::Path;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use base64::Engine;
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::plugins::{self, Manifest};
use crate::profile;
use crate::remote::{self, Context, Pin, PinError, Pinned, Source};

/// The set's format this launcher reads and writes.
pub const FORMAT: u32 = 1;
/// What starts a long code.
pub const LONG_PREFIX: &str = "ORMS1.";
/// What starts a deep link.
pub const LINK_PREFIX: &str = "openrac://modset/";
/// A short code's length, without its dash.
pub const CODE_LENGTH: usize = 8;
/// A short code's letters: the party codes' alphabet, without 0, O, 1 or I, which read alike.
pub const ALPHABET: &str = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
/// The most a set may weigh, compressed and as JSON.
pub const MAX_COMPRESSED: usize = 8 * 1024;
pub const MAX_JSON: usize = 64 * 1024;
/// The most entries in each of a set's lists.
pub const MAX_MODS: usize = 40;
/// Where the game finds the code of the set it plays (the Rules and Refused pages).
pub const GAME_FILE: &str = plugins::CODE_FILE;
/// The sources the player said yes to.
pub const TRUSTED: &str = "trusted_sources.json";
/// The public server's mod sets.
pub const DEFAULT_STORE: &str = "game.openrac.dev:7771";
/// The variable that points the launcher at another store (a test server, a private one).
pub const STORE_ENV: &str = "OPENRAC_MODSET_SERVER";
/// A release younger than this from outside OpenRAC is flagged in the review: compromised accounts
/// are found out in days.
const FRESH_SECONDS: u64 = 48 * 60 * 60;
/// Repositories trusted without asking.
const OWN_REPOSITORIES: &str = "https://github.com/openrac/";

const MAX_TEXT: usize = 64;
const MAX_VALUE: usize = 256;
const MAX_SETTINGS: usize = 64;

// ---- The set ----------------------------------------------------------------------------------

/// A mod's source: which catalogue repository, at which folder and version.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Default)]
#[serde(deny_unknown_fields)]
pub struct SetSource {
    pub url: String,
    #[serde(default)]
    pub path: String,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub tag: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub commit: Option<String>,
}

/// A mod a code can install.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Default)]
#[serde(deny_unknown_fields)]
pub struct SetMod {
    pub id: String,
    pub version: String,
    /// `gameplay`, `input`, `cosmetic` or `solo`.
    pub kind: String,
    pub source: SetSource,
    /// The SHA-256 of its release's `SHA256SUMS`; none for a mod pinned by a commit.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub sums: Option<String>,
    /// The settings a party compares.
    #[serde(default, skip_serializing_if = "BTreeMap::is_empty")]
    pub settings: BTreeMap<String, Value>,
}

/// A mod the host has that no catalogue holds: the code can only name it.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct SetManual {
    pub id: String,
    pub version: String,
    pub name: String,
}

/// A data pack the host plays with, shown as "also needed": a code never downloads one.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct SetPack {
    pub id: String,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub digest: Option<String>,
}

/// The set: what a code means.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct ModSet {
    pub format: u32,
    /// Who shared it, in their words: "Ada's mods".
    pub name: String,
    pub created: String,
    pub game: String,
    /// The discs every mod of it runs on.
    pub discs: Vec<String>,
    /// The OpenRAC it was made with.
    pub openrac: String,
    pub mods: Vec<SetMod>,
    /// Input and cosmetic mods the sharer suggests; each player's choice.
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub extras: Vec<SetMod>,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub manual: Vec<SetManual>,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub packs: Vec<SetPack>,
    /// The game's own short digest of these mods, when a running game made the set.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub digest: Option<String>,
}

const NOT_A_CODE: &str = "That code doesn't look right. Ask for it again.";

fn is_hex(text: &str) -> bool {
    !text.is_empty() && text.bytes().all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
}

fn plain_text(text: &str, most: usize) -> bool {
    text.chars().count() <= most && !text.contains("://") && !text.chars().any(char::is_control)
}

impl SetMod {
    /// The rules of one mod that a code can install.
    fn check(&self) -> Result<(), String> {
        let bad = || NOT_A_CODE.to_string();
        if !plugins::valid_id(&self.id) || plugins::parse_version(&self.version).is_none() {
            return Err(bad());
        }
        if !matches!(self.kind.as_str(), "gameplay" | "input" | "cosmetic" | "solo") {
            return Err(bad());
        }
        match remote::parse_url(&self.source.url) {
            Ok(Source::GitHub { reference: None, tag: None, .. }) if self.source.url.starts_with("https://") => {}
            _ => return Err(bad()),
        }
        if !self.source.path.is_empty() && !plugins::safe_relative(&self.source.path) {
            return Err(bad());
        }
        let tag_ok = |t: &String| {
            !t.is_empty()
                && t.len() <= MAX_TEXT
                && t.bytes().all(|b| b.is_ascii_alphanumeric() || b"._/-".contains(&b))
                && plugins::safe_relative(t)
        };
        if self.source.tag.as_ref().is_some_and(|t| !tag_ok(t)) {
            return Err(bad());
        }
        if self.sums.as_ref().is_some_and(|s| s.len() != 64 || !is_hex(s)) {
            return Err(bad());
        }
        if self.source.commit.as_ref().is_some_and(|c| c.len() != 40 || !is_hex(c)) {
            return Err(bad());
        }
        // Pinned, one way or the other: a tag without its checksums could be moved.
        let pinned = (self.source.tag.is_some() && self.sums.is_some())
            || (self.source.tag.is_none() && self.sums.is_none() && self.source.commit.is_some());
        if !pinned || self.settings.len() > MAX_SETTINGS {
            return Err(bad());
        }
        for (key, value) in &self.settings {
            let scalar = match value {
                Value::Bool(_) | Value::Number(_) => true,
                Value::String(s) => plain_text(s, MAX_VALUE),
                _ => false,
            };
            let key_ok = !key.is_empty()
                && key.len() <= 64
                && key.bytes().all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b == b'_');
            if !scalar || !key_ok {
                return Err(bad());
            }
        }
        Ok(())
    }

    fn pin(&self) -> Pin {
        Pin {
            url: self.source.url.clone(),
            path: self.source.path.clone(),
            id: self.id.clone(),
            version: self.version.clone(),
            tag: self.source.tag.clone(),
            commit: self.source.commit.clone(),
            sums: self.sums.clone(),
        }
    }
}

impl ModSet {
    /// Every rule of the format: nothing else is let past the parser.
    pub fn validate(&self) -> Result<(), String> {
        let bad = || NOT_A_CODE.to_string();
        if self.format != FORMAT {
            return Err("That code was made by a newer launcher. Update OpenRAC, then enter it again.".into());
        }
        if !plain_text(&self.name, MAX_TEXT)
            || !plain_text(&self.created, MAX_TEXT)
            || !plain_text(&self.openrac, MAX_TEXT)
            || !plugins::valid_id(&self.game)
            || self.discs.len() > 8
            || self.discs.iter().any(|d| !plugins::valid_id(d))
        {
            return Err(bad());
        }
        if self.digest.as_ref().is_some_and(|d| d.len() > MAX_TEXT || !is_hex(d)) {
            return Err(bad());
        }
        for list in [self.mods.len(), self.extras.len(), self.manual.len(), self.packs.len()] {
            if list > MAX_MODS {
                return Err(bad());
            }
        }
        for m in self.mods.iter().chain(&self.extras) {
            m.check()?;
        }
        for m in &self.manual {
            if !plugins::valid_id(&m.id)
                || plugins::parse_version(&m.version).is_none()
                || !plain_text(&m.name, MAX_TEXT)
            {
                return Err(bad());
            }
        }
        for p in &self.packs {
            if !plugins::valid_id(&p.id) || p.digest.as_ref().is_some_and(|d| d.len() > MAX_TEXT || !is_hex(d)) {
                return Err(bad());
            }
        }
        let mut seen = BTreeSet::new();
        if !self.mods.iter().chain(&self.extras).all(|m| seen.insert(&m.id)) {
            return Err(bad());
        }
        Ok(())
    }

    /// The set from its JSON, strictly: an unknown key anywhere is refused.
    pub fn from_json(bytes: &[u8]) -> Result<ModSet, String> {
        let probe: Value = serde_json::from_slice(bytes).map_err(|_| NOT_A_CODE.to_string())?;
        // A newer format's keys are not an error of this one: say what it is.
        if probe.get("format").and_then(Value::as_u64).is_some_and(|f| f > u64::from(FORMAT)) {
            return Err("That code was made by a newer launcher. Update OpenRAC, then enter it again.".into());
        }
        let set: ModSet = serde_json::from_slice(bytes).map_err(|_| NOT_A_CODE.to_string())?;
        set.validate()?;
        Ok(set)
    }
}

/// The compressed bytes of a set: what the store keeps and a long code holds.
pub fn compress(set: &ModSet) -> Result<Vec<u8>, String> {
    let json = serde_json::to_vec(set).map_err(|e| e.to_string())?;
    let mut encoder = flate2::write::ZlibEncoder::new(Vec::new(), flate2::Compression::best());
    encoder.write_all(&json).map_err(|e| e.to_string())?;
    let bytes = encoder.finish().map_err(|e| e.to_string())?;
    if bytes.len() > MAX_COMPRESSED {
        return Err("This set is too large to share by code. Turn some mods off and try again.".into());
    }
    Ok(bytes)
}

/// A set from compressed bytes, which are never trusted: the inflated size is capped whatever the
/// data says, and everything is checked.
pub fn decompress(bytes: &[u8]) -> Result<ModSet, String> {
    if bytes.len() > 2 * MAX_COMPRESSED {
        return Err(NOT_A_CODE.into());
    }
    let mut json = Vec::new();
    flate2::read::ZlibDecoder::new(bytes)
        .take(MAX_JSON as u64 + 1)
        .read_to_end(&mut json)
        .map_err(|_| NOT_A_CODE.to_string())?;
    if json.len() > MAX_JSON {
        return Err(NOT_A_CODE.into());
    }
    ModSet::from_json(&json)
}

/// A set as a long code.
pub fn to_long(set: &ModSet) -> Result<String, String> {
    Ok(format!("{LONG_PREFIX}{}", base64::engine::general_purpose::URL_SAFE_NO_PAD.encode(compress(set)?)))
}

/// A set from a long code.
pub fn from_long(text: &str) -> Result<ModSet, String> {
    let body = text.strip_prefix(LONG_PREFIX).ok_or(NOT_A_CODE)?;
    let bytes = base64::engine::general_purpose::URL_SAFE_NO_PAD.decode(body).map_err(|_| NOT_A_CODE.to_string())?;
    decompress(&bytes)
}

/// A short code as it is shown: `K7M2-9QXR`.
pub fn show_code(code: &str) -> String {
    if code.len() == CODE_LENGTH {
        format!("{}-{}", &code[..4], &code[4..])
    } else {
        code.to_string()
    }
}

/// What the player entered.
#[derive(Debug, Clone, PartialEq)]
pub enum Entered {
    /// A short code, its 8 characters without the dash.
    Short(String),
    /// A long code, whole.
    Long(String),
}

/// Reads what was typed, pasted or followed: a short code in any case, with or without its dash, a
/// long code, or a deep link to either. A wrong character is said at once, without asking the server.
pub fn parse_entry(text: &str) -> Result<Entered, String> {
    let text = text.trim();
    let text = text.strip_prefix(LINK_PREFIX).unwrap_or(text).trim_end_matches('/');
    if text.starts_with(LONG_PREFIX) {
        return Ok(Entered::Long(text.to_string()));
    }
    let code: String =
        text.chars().filter(|c| !matches!(c, '-' | ' ' | '\u{2013}' | '\u{2014}')).collect::<String>().to_uppercase();
    if code.is_empty() {
        return Err("Enter a mod code.".into());
    }
    if let Some(c) = code.chars().find(|c| !ALPHABET.contains(*c)) {
        return Err(match c {
            '0' | 'O' | '1' | 'I' => format!("Mod codes have no {c}: check that character."),
            _ => NOT_A_CODE.to_string(),
        });
    }
    if code.len() != CODE_LENGTH {
        return Err(format!("A mod code has {CODE_LENGTH} characters, like K7M2-9QXR."));
    }
    Ok(Entered::Short(code))
}

// ---- The server's store -----------------------------------------------------------------------

/// Why the store did not answer as asked.
#[derive(Debug, Clone, PartialEq)]
pub enum StoreError {
    /// The code is unknown or has expired.
    NotFound,
    /// The server said no, or too many requests.
    Refused,
    /// It could not be reached.
    Unreachable,
}

/// The store on the server, which the tests answer from memory.
pub trait Store {
    fn put(&self, body: &[u8]) -> Result<String, StoreError>;
    fn get(&self, code: &str) -> Result<Vec<u8>, StoreError>;
}

/// The real one: HTTP over a plain socket, on every platform (Android has no curl). A set is public
/// metadata and nothing in it is believed; the review and the hashes do the protecting.
pub struct HttpStore {
    /// `host:port`.
    pub address: String,
}

impl Default for HttpStore {
    fn default() -> Self {
        let address = std::env::var(STORE_ENV).ok().filter(|a| !a.is_empty());
        HttpStore { address: address.unwrap_or_else(|| DEFAULT_STORE.to_string()) }
    }
}

impl HttpStore {
    /// One request, one connection: the status and the body.
    fn request(&self, method: &str, path: &str, body: &[u8]) -> Result<(u16, Vec<u8>), StoreError> {
        use std::net::{TcpStream, ToSocketAddrs};
        let wait = Duration::from_secs(8);
        let unreachable = |_| StoreError::Unreachable;
        let target = self.address.to_socket_addrs().map_err(unreachable)?.next().ok_or(StoreError::Unreachable)?;
        let mut stream = TcpStream::connect_timeout(&target, wait).map_err(unreachable)?;
        stream.set_read_timeout(Some(wait)).map_err(unreachable)?;
        stream.set_write_timeout(Some(wait)).map_err(unreachable)?;
        let host = self.address.as_str();
        let head = format!(
            "{method} {path} HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\nContent-Length: {}\r\nContent-Type: application/octet-stream\r\nUser-Agent: openrac-launcher/{}\r\n\r\n",
            body.len(),
            env!("CARGO_PKG_VERSION"),
        );
        // In one write: a short request needs no two packets.
        stream.write_all(&[head.as_bytes(), body].concat()).map_err(unreachable)?;
        let mut answer = Vec::new();
        stream.take(1 << 20).read_to_end(&mut answer).map_err(unreachable)?;
        parse_response(&answer).ok_or(StoreError::Unreachable)
    }
}

/// The status and body of a whole HTTP response.
fn parse_response(answer: &[u8]) -> Option<(u16, Vec<u8>)> {
    let end = answer.windows(4).position(|w| w == b"\r\n\r\n")?;
    let head = std::str::from_utf8(&answer[..end]).ok()?;
    let status = head.split_whitespace().nth(1)?.parse().ok()?;
    let body = &answer[end + 4..];
    let length = head.lines().find_map(|l| {
        let (name, value) = l.split_once(':')?;
        name.eq_ignore_ascii_case("content-length").then(|| value.trim().parse::<usize>().ok())?
    });
    // A response cut short must not read as a short body.
    match length {
        Some(n) if body.len() < n => None,
        Some(n) => Some((status, body[..n].to_vec())),
        None => Some((status, body.to_vec())),
    }
}

impl Store for HttpStore {
    fn put(&self, body: &[u8]) -> Result<String, StoreError> {
        let (status, answer) = self.request("PUT", "/v1/modsets", body)?;
        if status != 200 {
            return Err(StoreError::Refused);
        }
        let value: Value = serde_json::from_slice(&answer).map_err(|_| StoreError::Refused)?;
        let code = value.get("code").and_then(Value::as_str).ok_or(StoreError::Refused)?;
        // The answer is checked like anything a player would type.
        match parse_entry(code) {
            Ok(Entered::Short(code)) => Ok(code),
            _ => Err(StoreError::Refused),
        }
    }

    fn get(&self, code: &str) -> Result<Vec<u8>, StoreError> {
        match self.request("GET", &format!("/v1/modsets/{code}"), &[])? {
            (200, body) => Ok(body),
            (404, _) => Err(StoreError::NotFound),
            _ => Err(StoreError::Refused),
        }
    }
}

/// The set a code means, from the server for a short code and from itself for a long one. The
/// display code comes back with it, none for a long code.
pub fn resolve(store: &dyn Store, text: &str) -> Result<(ModSet, Option<String>), String> {
    match parse_entry(text)? {
        Entered::Long(long) => Ok((from_long(&long)?, None)),
        Entered::Short(code) => {
            let bytes = store.get(&code).map_err(|e| match e {
                StoreError::NotFound => "That code isn't known, or it expired. Ask for a new one.".to_string(),
                StoreError::Refused => "The server is busy. Try again in a minute.".to_string(),
                StoreError::Unreachable => {
                    "The OpenRAC server can't be reached. Ask your friend for the full code, which works offline."
                        .to_string()
                }
            })?;
            Ok((decompress(&bytes)?, Some(show_code(&code))))
        }
    }
}

// ---- Sharing ----------------------------------------------------------------------------------

/// A mod of the set being shared, in the Share window.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ShareMod {
    pub id: String,
    pub name: String,
    pub version: String,
}

/// What sharing would hold, before anything is sent.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct SharePreview {
    /// The gameplay mods the code installs.
    pub mods: Vec<ShareMod>,
    /// Input and cosmetic mods the sharer may add.
    pub optional: Vec<ShareMod>,
    /// Mods no catalogue holds: named by the code, sent by hand.
    pub manual: Vec<ShareMod>,
    /// Mods that cannot be played in a party.
    pub solo: Vec<ShareMod>,
}

/// A code, made.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Shared {
    /// `K7M2-9QXR`, or none when the server could not be reached.
    pub code: Option<String>,
    /// The whole set, which works without a server.
    pub long: String,
    /// The deep link: to the short code when there is one.
    pub link: String,
}

/// An enabled mod, its manifest and its place in the host's load order.
struct Enabled {
    manifest: Manifest,
    values: serde_json::Map<String, Value>,
}

fn enabled_mods(dir: &Path) -> Result<Vec<Enabled>, String> {
    let stored = plugins::stored(dir)?;
    let mut out = Vec::new();
    for (id, enabled) in plugins::load_order(dir)? {
        if !enabled {
            continue;
        }
        let manifest = plugins::read_manifest(&dir.join(&id))?;
        let values = stored.get(&id).map(|s| s.1.clone()).unwrap_or_default();
        out.push(Enabled { manifest, values });
    }
    Ok(out)
}

fn shown(m: &Manifest) -> ShareMod {
    ShareMod { id: m.id.clone(), name: m.name.clone(), version: m.version.clone() }
}

/// The source a code can name for an installed mod: on GitHub, at the version installed, pinned.
fn shareable(sources: &remote::Sources, m: &Manifest) -> Option<remote::ModSource> {
    let s = sources.mods.get(&m.id)?;
    let on_github = matches!(remote::parse_url(&s.url), Ok(Source::GitHub { reference: None, tag: None, .. }));
    (on_github && s.version == m.version && (s.tag.is_some() || s.commit.as_ref().is_some_and(|c| c.len() == 40)))
        .then(|| s.clone())
}

/// What a code of this game's enabled mods would hold.
pub fn share_preview(dir: &Path) -> Result<SharePreview, String> {
    let sources = remote::read_sources(dir);
    let mut out = SharePreview { mods: Vec::new(), optional: Vec::new(), manual: Vec::new(), solo: Vec::new() };
    for e in enabled_mods(dir)? {
        let kind = plugins::party_kind(&e.manifest);
        let can = shareable(&sources, &e.manifest).is_some();
        if kind == "solo" {
            out.solo.push(shown(&e.manifest));
        }
        match (kind, can) {
            ("input" | "cosmetic", true) => out.optional.push(shown(&e.manifest)),
            ("input" | "cosmetic", false) => {}
            (_, true) => out.mods.push(shown(&e.manifest)),
            (_, false) => out.manual.push(shown(&e.manifest)),
        }
    }
    Ok(out)
}

/// The settings of a mod that a party compares, as they stand. The multiplayer mod's own are the
/// player's (a party does not compare that mod), so it brings none.
fn shared_settings(e: &Enabled) -> BTreeMap<String, Value> {
    if e.manifest.id == "multiplayer" {
        return BTreeMap::new();
    }
    e.manifest
        .settings
        .iter()
        .filter(|s| plugins::shared_setting(s))
        .filter_map(|s| {
            let value = e.values.get(&s.key).and_then(|v| plugins::check_value(s, v)).unwrap_or(s.default.clone());
            plain_settable(&value).then(|| (s.key.clone(), value))
        })
        .collect()
}

/// A value a set can carry; a string setting holding an address cannot travel in one.
fn plain_settable(value: &Value) -> bool {
    match value {
        Value::Bool(_) | Value::Number(_) => true,
        Value::String(s) => plain_text(s, MAX_VALUE),
        _ => false,
    }
}

/// A name as a set carries it: short, one line, no address.
fn carried_name(name: &str) -> String {
    name.replace("://", " ").chars().filter(|c| !c.is_control()).take(MAX_TEXT).collect()
}

/// The OpenRAC that last ran these mods, from the port's `status.json`; "" before its first start.
pub fn port_version(dir: &Path) -> String {
    std::fs::read(dir.join(plugins::STATUS))
        .ok()
        .and_then(|b| serde_json::from_slice::<Value>(&b).ok())
        .and_then(|v| v.get("openrac").and_then(Value::as_str).map(str::to_string))
        .filter(|v| v.len() <= MAX_TEXT && plain_text(v, MAX_TEXT))
        .unwrap_or_default()
}

/// The set of this game's enabled mods. `extras`: the optional mods the sharer ticked. `name`: who
/// shares it. `openrac`: the version of OpenRAC.
pub fn build(
    ctx: &Context,
    extras: &[String],
    name: &str,
    openrac: &str,
    packs: Vec<SetPack>,
    now: &str,
) -> Result<ModSet, String> {
    let sources = remote::read_sources(ctx.dir);
    let mut set = ModSet {
        format: FORMAT,
        name: carried_name(name),
        created: now.to_string(),
        game: ctx.game.to_string(),
        discs: Vec::new(),
        openrac: openrac.to_string(),
        mods: Vec::new(),
        extras: Vec::new(),
        manual: Vec::new(),
        packs,
        digest: None,
    };
    let mut discs: Option<BTreeSet<String>> = None;
    for e in enabled_mods(ctx.dir)? {
        let kind = plugins::party_kind(&e.manifest).to_string();
        let optional = matches!(kind.as_str(), "input" | "cosmetic");
        if optional && !extras.contains(&e.manifest.id) {
            continue;
        }
        let Some(source) = shareable(&sources, &e.manifest) else {
            if !optional {
                let name = carried_name(&e.manifest.name);
                set.manual.push(SetManual { id: e.manifest.id.clone(), version: e.manifest.version.clone(), name });
            }
            continue;
        };
        let here: BTreeSet<String> = e.manifest.games.get(ctx.game).into_iter().flatten().cloned().collect();
        discs = Some(match discs {
            Some(d) => d.intersection(&here).cloned().collect(),
            None => here,
        });
        let sums = match &source.tag {
            Some(tag) => {
                Some(source.sums.clone().map_or_else(|| remote::sums_hash(ctx, &source.url, tag), Ok).map_err(
                    |_| format!("The release of {} can't be read now. Check your connection.", e.manifest.name),
                )?)
            }
            None => None,
        };
        let entry = SetMod {
            id: e.manifest.id.clone(),
            version: e.manifest.version.clone(),
            kind,
            settings: if optional { BTreeMap::new() } else { shared_settings(&e) },
            source: SetSource {
                url: source.url,
                path: source.path,
                commit: if source.tag.is_none() { source.commit } else { None },
                tag: source.tag,
            },
            sums,
        };
        if optional {
            set.extras.push(entry);
        } else {
            set.mods.push(entry);
        }
    }
    set.discs = discs.map_or_else(|| vec![ctx.version.to_string()], |d| d.into_iter().collect());
    if set.mods.len().max(set.extras.len()).max(set.manual.len()) > MAX_MODS {
        return Err(format!("A code holds at most {MAX_MODS} mods. Turn some off and try again."));
    }
    set.validate()?;
    Ok(set)
}

/// What the game reads to show the code on its Rules and Refused pages (`<plugins>/modset.json`).
/// It shows the code only while its running mods match `mods`. A change to what a party compares
/// takes the file away ([`plugins::forget_code`]).
pub fn write_game_file(dir: &Path, set: &ModSet, code: Option<&str>, long: &str) -> Result<(), String> {
    // As the game lists its mods for a party: Multiplayer's own left out.
    let mods: Vec<Value> = set
        .mods
        .iter()
        .filter(|m| m.id != "multiplayer")
        .map(|m| serde_json::json!({"id": m.id, "version": m.version}))
        .chain(set.manual.iter().map(|m| serde_json::json!({"id": m.id, "version": m.version})))
        .collect();
    let value = serde_json::json!({
        "format": 1, "code": code, "link": link_for(code, long), "created": set.created, "mods": mods,
    });
    let text = serde_json::to_string_pretty(&value).map_err(|e| e.to_string())? + "\n";
    let temporary = dir.join(format!("{GAME_FILE}.tmp"));
    std::fs::write(&temporary, text).map_err(|e| format!("{}: {e}", temporary.display()))?;
    std::fs::rename(&temporary, dir.join(GAME_FILE)).map_err(|e| format!("{GAME_FILE}: {e}"))
}

fn link_for(code: Option<&str>, long: &str) -> String {
    match code {
        Some(code) => format!("{LINK_PREFIX}{code}"),
        None => format!("{LINK_PREFIX}{long}"),
    }
}

/// Makes the codes of a set: the long one always, the short one when the server answers. A server
/// that cannot be reached is no failure: the long code works without it.
pub fn share(dir: &Path, store: &dyn Store, set: &ModSet) -> Result<Shared, String> {
    let long = to_long(set)?;
    let code = store.put(&compress(set)?).ok().map(|c| show_code(&c));
    write_game_file(dir, set, code.as_deref(), &long)?;
    let link = link_for(code.as_deref(), &long);
    Ok(Shared { code, long, link })
}

// ---- Review -----------------------------------------------------------------------------------

/// What a set would do to one mod.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ReviewMod {
    pub id: String,
    pub name: String,
    pub version: String,
    /// What Apply does to it.
    #[ts(type = "\"install\" | \"update\" | \"downgrade\" | \"reinstall\" | \"enable\" | \"keep\" | \"unavailable\"")]
    pub action: String,
    /// The version installed, if it is.
    pub from: Option<String>,
    /// `owner/repository`.
    pub source: String,
    /// It carries a library: it runs code on this computer.
    pub native: bool,
    /// A suggested input or cosmetic mod: the player's choice, off until ticked.
    pub optional: bool,
    /// An unavailable mod: why.
    pub why: Option<String>,
    /// An unavailable mod whose version is gone: the newest there is.
    pub nearest: Option<String>,
    /// A release from outside OpenRAC, younger than two days.
    pub fresh: bool,
    pub published: Option<String>,
}

/// One setting a set changes.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ReviewSetting {
    pub mod_name: String,
    pub name: String,
    #[ts(type = "unknown")]
    pub from: Value,
    #[ts(type = "unknown")]
    pub to: Value,
}

/// A repository a set installs from that the player has not said yes to.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ReviewSource {
    pub url: String,
    /// `owner/repository`.
    pub label: String,
}

/// Everything Apply would do, computed with nothing changed.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Review {
    /// The set again, to give back to Apply: what was reviewed is what is applied.
    pub set: String,
    /// `K7M2-9QXR`, or none for a long code.
    pub code: Option<String>,
    pub name: String,
    pub created: String,
    /// The game it is for.
    pub game: String,
    /// That is not the game it was reviewed for.
    pub wrong_game: bool,
    pub mods: Vec<ReviewMod>,
    /// Gameplay mods that are on and that the set does not list: Apply turns them off.
    pub disable: Vec<ShareMod>,
    pub settings: Vec<ReviewSetting>,
    /// Mods to get by hand.
    pub manual: Vec<ShareMod>,
    /// Data packs that are also needed.
    pub packs: Vec<String>,
    /// Sources to say yes to first.
    pub ask: Vec<ReviewSource>,
    /// The OpenRAC of the set, when it is another one than this.
    pub other_openrac: Option<String>,
    /// Some of the set cannot be applied: Apply does the rest.
    pub partial: bool,
    /// Nothing would change.
    pub nothing: bool,
}

fn label_of(url: &str) -> String {
    url.trim_start_matches("https://github.com/").to_string()
}

fn is_own(url: &str) -> bool {
    url.to_ascii_lowercase().starts_with(OWN_REPOSITORIES)
}

#[derive(Debug, Default, Serialize, Deserialize)]
struct TrustedFile {
    format: u32,
    urls: Vec<String>,
}

fn read_trusted(dir: &Path) -> Vec<String> {
    std::fs::read(dir.join(TRUSTED))
        .ok()
        .and_then(|b| serde_json::from_slice::<TrustedFile>(&b).ok())
        .map_or_else(Vec::new, |f| f.urls)
}

fn write_trusted(dir: &Path, urls: Vec<String>) -> Result<(), String> {
    let text = serde_json::to_string_pretty(&TrustedFile { format: 1, urls }).map_err(|e| e.to_string())? + "\n";
    std::fs::write(dir.join(TRUSTED), text).map_err(|e| format!("{TRUSTED}: {e}"))
}

/// Whether a repository needs asking: not OpenRAC's, nothing installed from it, never said yes to.
fn needs_asking(url: &str, sources: &remote::Sources, trusted: &[String]) -> bool {
    let same = |a: &str| a.eq_ignore_ascii_case(url);
    !is_own(url) && !sources.mods.values().any(|s| same(&s.url)) && !trusted.iter().any(|t| same(t))
}

/// Seconds since 1970 of `2026-10-31T12:00:00Z`.
fn parse_time(text: &str) -> Option<u64> {
    let (date, time) = text.strip_suffix('Z')?.split_once('T')?;
    let mut d = date.split('-').map(|p| p.parse::<i64>().ok());
    let (y, m, day) = (d.next()??, d.next()??, d.next()??);
    let mut t = time.split('.').next()?.split(':').map(|p| p.parse::<i64>().ok());
    let (hour, minute, second) = (t.next()??, t.next()??, t.next()??);
    // Days from the civil calendar, by the usual era arithmetic.
    let y = if m <= 2 { y - 1 } else { y };
    let era = y.div_euclid(400);
    let yoe = y - era * 400;
    let doy = (153 * (m + if m > 2 { -3 } else { 9 }) + 2) / 5 + day - 1;
    let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    let days = era * 146_097 + doe - 719_468;
    u64::try_from(days * 86_400 + hour * 3600 + minute * 60 + second).ok()
}

/// The time as `2026-10-31T12:00:00Z`.
pub fn now_utc() -> String {
    format_time(SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |d| d.as_secs()))
}

fn format_time(seconds: u64) -> String {
    let days = (seconds / 86_400) as i64 + 719_468;
    let era = days.div_euclid(146_097);
    let doe = days - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = if mp < 10 { mp + 3 } else { mp - 9 };
    let year = yoe + era * 400 + i64::from(month <= 2);
    let rest = seconds % 86_400;
    format!("{year:04}-{month:02}-{day:02}T{:02}:{:02}:{:02}Z", rest / 3600, rest % 3600 / 60, rest % 60)
}

/// What one mod comes to, and what its review line is built from.
struct Planned {
    entry: SetMod,
    line: ReviewMod,
    /// What the review read of it; none when it needs no download.
    pinned: Option<Pinned>,
}

/// Whether the mod installed is exactly the one a set names.
fn same_install(m: &SetMod, source: Option<&remote::ModSource>) -> bool {
    source.is_some_and(|s| {
        s.url.eq_ignore_ascii_case(&m.source.url)
            && s.version == m.version
            && s.tag == m.source.tag
            && match (&s.sums, &m.sums) {
                (Some(a), Some(b)) => a == b,
                _ => m.sums.is_some() || s.commit == m.source.commit,
            }
    })
}

fn plan_mod(ctx: &Context, m: &SetMod, optional: bool, enabled: Option<bool>, sources: &remote::Sources) -> Planned {
    let installed = plugins::read_manifest(&ctx.dir.join(&m.id)).ok();
    let from = installed.as_ref().map(|i| i.version.clone());
    let source = sources.mods.get(&m.id);
    let mut line = ReviewMod {
        id: m.id.clone(),
        name: installed.as_ref().map_or_else(|| m.id.clone(), |i| i.name.clone()),
        version: m.version.clone(),
        action: "keep".into(),
        from: from.clone(),
        source: label_of(&m.source.url),
        native: installed.as_ref().is_some_and(|i| !i.plugin.is_empty()),
        optional,
        why: None,
        nearest: None,
        fresh: false,
        published: None,
    };
    if from.as_deref() == Some(m.version.as_str()) && same_install(m, source) {
        if enabled != Some(true) {
            line.action = "enable".into();
        }
        return Planned { entry: m.clone(), line, pinned: None };
    }
    match remote::read_pinned(ctx, &m.pin()) {
        Ok(pinned) => {
            line.name = pinned.name.clone();
            line.native = pinned.kind == "native";
            line.published = pinned.published.clone();
            let now = SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |d| d.as_secs());
            line.fresh = !is_own(&m.source.url)
                && pinned
                    .published
                    .as_deref()
                    .and_then(parse_time)
                    .is_some_and(|t| now.saturating_sub(t) < FRESH_SECONDS);
            line.action = match from.as_deref().and_then(plugins::parse_version) {
                None if from.is_some() => "reinstall",
                None => "install",
                Some(have) => match plugins::parse_version(&m.version).map(|w| plugins::compare_versions(w, have)) {
                    Some(std::cmp::Ordering::Greater) => "update",
                    Some(std::cmp::Ordering::Less) => "downgrade",
                    _ => "reinstall",
                },
            }
            .into();
            Planned { entry: m.clone(), line, pinned: Some(pinned) }
        }
        Err(e) => {
            line.action = "unavailable".into();
            match e {
                PinError::Gone => {
                    line.why = Some(format!("Version {} is gone from its repository.", m.version));
                    line.nearest = remote::newest_release(ctx, &m.source.url, &m.id);
                }
                PinError::Changed => {
                    line.why = Some(format!("Version {} changed after this code was made.", m.version));
                }
                // Players never read which disc, only that theirs is another.
                PinError::Unfit(why) => {
                    let wrong_disc = why.contains("disc");
                    line.why = Some(if wrong_disc { "It doesn't run on your disc.".to_string() } else { why });
                }
                PinError::Failed(_) => {
                    line.why = Some("It couldn't be read just now. Check your connection and try again.".into());
                }
            }
            Planned { entry: m.clone(), line, pinned: None }
        }
    }
}

/// The changes to a mod's settings a set brings, read against its manifest. A key the mod does not
/// have, or a value it does not take, is ignored: the same check applies them.
fn plan_settings(planned: &Planned, stored: &BTreeMap<String, plugins::Stored>, dir: &Path) -> Vec<ReviewSetting> {
    let manifest = planned
        .pinned
        .as_ref()
        .map(|p| p.manifest.clone())
        .or_else(|| plugins::read_manifest(&dir.join(&planned.entry.id)).ok());
    let Some(manifest) = manifest else { return Vec::new() };
    let empty = serde_json::Map::new();
    let values = stored.get(&planned.entry.id).map_or(&empty, |s| &s.1);
    let mut out = Vec::new();
    for spec in manifest.settings.iter().filter(|s| plugins::shared_setting(s)) {
        let Some(to) = planned.entry.settings.get(&spec.key).and_then(|v| plugins::check_value(spec, v)) else {
            continue;
        };
        let from = values.get(&spec.key).and_then(|v| plugins::check_value(spec, v)).unwrap_or(spec.default.clone());
        if from != to {
            out.push(ReviewSetting { mod_name: planned.line.name.clone(), name: spec.name.clone(), from, to });
        }
    }
    out
}

struct Plan {
    review: Review,
    planned: Vec<Planned>,
    /// The enabled gameplay mods the set does not list.
    disable: Vec<String>,
}

fn make_plan(ctx: &Context, set: &ModSet, code: Option<String>, openrac: &str) -> Result<Plan, String> {
    let sources = remote::read_sources(ctx.dir);
    let trusted = read_trusted(ctx.dir);
    let stored = plugins::stored(ctx.dir)?;
    let enabled = |id: &str| stored.get(id).map(|s| s.0);
    let mut planned = Vec::new();
    let wrong_game = set.game != ctx.game;
    if !wrong_game {
        for m in &set.mods {
            planned.push(plan_mod(ctx, m, false, enabled(&m.id), &sources));
        }
        for m in &set.extras {
            planned.push(plan_mod(ctx, m, true, enabled(&m.id), &sources));
        }
    }
    let settings: Vec<ReviewSetting> = planned
        .iter()
        .filter(|p| !p.line.optional && p.line.action != "unavailable")
        .flat_map(|p| plan_settings(p, &stored, ctx.dir))
        .collect();
    // Gameplay mods off the list go. Input and cosmetic ones are the player's own. Multiplayer is
    // what the party itself runs on: a party's check leaves it out, and so does a code.
    let listed: BTreeSet<&str> =
        set.mods.iter().map(|m| m.id.as_str()).chain(set.manual.iter().map(|m| m.id.as_str())).collect();
    let mut disable = Vec::new();
    let mut disable_views = Vec::new();
    if !wrong_game {
        for e in enabled_mods(ctx.dir)? {
            let kind = plugins::party_kind(&e.manifest);
            if matches!(kind, "gameplay" | "solo")
                && e.manifest.id != "multiplayer"
                && !listed.contains(e.manifest.id.as_str())
            {
                disable.push(e.manifest.id.clone());
                disable_views.push(shown(&e.manifest));
            }
        }
    }
    let mut ask: BTreeMap<String, ReviewSource> = BTreeMap::new();
    for p in planned.iter().filter(|p| p.pinned.is_some() && needs_asking(&p.entry.source.url, &sources, &trusted)) {
        let url = p.entry.source.url.clone();
        ask.entry(url.clone()).or_insert(ReviewSource { label: label_of(&url), url });
    }
    let mods: Vec<ReviewMod> = planned.iter().map(|p| p.line.clone()).collect();
    let manual: Vec<ShareMod> = set
        .manual
        .iter()
        .filter(|m| {
            // Already here at that version: there is nothing to fetch by hand.
            !plugins::read_manifest(&ctx.dir.join(&m.id)).is_ok_and(|i| i.version == m.version)
        })
        .map(|m| ShareMod { id: m.id.clone(), name: m.name.clone(), version: m.version.clone() })
        .collect();
    let required_gone = mods.iter().any(|m| m.action == "unavailable" && !m.optional);
    let partial = wrong_game || required_gone || !manual.is_empty();
    let nothing = mods.iter().all(|m| matches!(m.action.as_str(), "keep" | "unavailable"))
        && disable.is_empty()
        && settings.is_empty();
    let other_openrac =
        (!set.openrac.is_empty() && !openrac.is_empty() && set.openrac != openrac).then(|| set.openrac.clone());
    let review = Review {
        set: serde_json::to_string(set).map_err(|e| e.to_string())?,
        code,
        name: set.name.clone(),
        created: set.created.clone(),
        game: set.game.clone(),
        wrong_game,
        mods,
        disable: disable_views,
        settings,
        manual,
        packs: set.packs.iter().map(|p| p.id.clone()).collect(),
        ask: ask.into_values().collect(),
        other_openrac,
        partial,
        nothing,
    };
    Ok(Plan { review, planned, disable })
}

/// What a set would do here, with nothing changed. `openrac`: this OpenRAC's version, "" unknown.
pub fn review(ctx: &Context, set: &ModSet, code: Option<String>, openrac: &str) -> Result<Review, String> {
    Ok(make_plan(ctx, set, code, openrac)?.review)
}

// ---- Apply ------------------------------------------------------------------------------------

/// What the player chose in the review.
#[derive(Debug, Clone, Deserialize, Default, ts_rs::TS)]
#[serde(rename_all = "camelCase", default)]
pub struct Choices {
    /// The optional mods ticked.
    pub extras: Vec<String>,
    /// The sources said yes to.
    pub trusted: Vec<String>,
}

/// What Apply came to.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Applied {
    /// The way back ([`crate::profile`]).
    pub profile: String,
    /// Mods installed or replaced.
    pub installed: usize,
    /// Mods switched on and off.
    pub enabled: usize,
    pub disabled: usize,
    /// Mods the set names that are still missing.
    pub skipped: Vec<String>,
    /// Settings the mods did not take.
    pub ignored: Vec<String>,
}

/// Applies a set the player reviewed. All its mods are fetched and checked first; only then does
/// anything change, after a way back is kept. A failure from there puts everything back.
pub fn apply(
    ctx: &Context,
    set_json: &str,
    code: Option<&str>,
    choices: &Choices,
    openrac: &str,
    now: &str,
) -> Result<Applied, String> {
    let set = ModSet::from_json(set_json.as_bytes())?;
    if set.game != ctx.game {
        return Err("This code is for another game.".into());
    }
    let plan = make_plan(ctx, &set, code.map(str::to_string), openrac)?;
    let chosen = |p: &Planned| !p.line.optional || choices.extras.contains(&p.entry.id);
    let still_asking: Vec<&ReviewSource> = plan
        .review
        .ask
        .iter()
        .filter(|s| !choices.trusted.iter().any(|t| t.eq_ignore_ascii_case(&s.url)))
        .filter(|s| plan.planned.iter().any(|p| chosen(p) && p.entry.source.url == s.url))
        .collect();
    if let Some(source) = still_asking.first() {
        return Err(format!("{} hasn't been confirmed.", source.label));
    }
    let wanted: Vec<&Planned> = plan.planned.iter().filter(|p| chosen(p)).collect();
    let to_install: Vec<&Planned> = wanted.iter().copied().filter(|p| p.pinned.is_some()).collect();
    // Everything is fetched and checked while nothing has changed: a bad download costs nothing.
    let mut staged = Vec::new();
    for p in &to_install {
        let pinned = p.pinned.as_ref().expect("filtered above");
        let stage = remote::stage_pinned(ctx, pinned).map_err(|e| {
            // A bad file is said as it is; anything else is the network's, in a player's words.
            if e.contains("SHA-256") || e.contains("The download holds") {
                e
            } else {
                format!("{} couldn't be downloaded. Check your connection and try again.", p.line.name)
            }
        })?;
        staged.push((pinned, stage));
    }
    let replaced: Vec<String> = to_install.iter().map(|p| p.entry.id.clone()).collect();
    let label = code.unwrap_or_default();
    let profile = profile::create(ctx.dir, label, &replaced, now)?;
    let done = (|| -> Result<Applied, String> {
        for (pinned, stage) in &staged {
            remote::install_staged(ctx, pinned, stage)?;
        }
        for id in &plan.disable {
            plugins::set_enabled(ctx.dir, ctx.game, ctx.version, id, false)?;
        }
        let mut enabled = 0;
        let mut ignored = Vec::new();
        for p in wanted.iter().filter(|p| p.line.action != "unavailable") {
            if p.line.action != "keep" {
                enabled += 1;
            }
            plugins::set_enabled(ctx.dir, ctx.game, ctx.version, &p.entry.id, true)?;
            ignored.extend(plugins::set_values(ctx.dir, &p.entry.id, &p.entry.settings)?);
        }
        let order: Vec<String> = set.mods.iter().map(|m| m.id.clone()).filter(|id| ctx.dir.join(id).is_dir()).collect();
        plugins::arrange(ctx.dir, &order)?;
        let mut urls = read_trusted(ctx.dir);
        for url in &choices.trusted {
            if !urls.contains(url) {
                urls.push(url.clone());
            }
        }
        if !choices.trusted.is_empty() {
            write_trusted(ctx.dir, urls)?;
        }
        let skipped = plan
            .review
            .mods
            .iter()
            .filter(|m| m.action == "unavailable" && !m.optional)
            .map(|m| m.id.clone())
            .chain(plan.review.manual.iter().map(|m| m.id.clone()))
            .collect();
        Ok(Applied {
            profile: profile.clone(),
            installed: staged.len(),
            enabled,
            disabled: plan.disable.len(),
            skipped,
            ignored,
        })
    })();
    match done {
        Ok(applied) => {
            // The game shows the code of what it plays; a player who applied one can host with it.
            if let Ok(long) = to_long(&set) {
                let _ = write_game_file(ctx.dir, &set, code, &long);
            }
            Ok(applied)
        }
        Err(e) => {
            profile::restore(ctx.dir, &profile, true).map_err(|undo| {
                format!("{e} Putting your mods back failed too ({undo}): restore them in the Mods tab.")
            })?;
            profile::discard(ctx.dir, &profile);
            Err(format!("{e} Your mods are as they were."))
        }
    }
}

/// Puts a profile back.
pub fn restore(dir: &Path, id: &str) -> Result<(), String> {
    profile::restore(dir, id, false)?;
    let _ = std::fs::remove_file(dir.join(GAME_FILE));
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;
    use sha2::{Digest, Sha256};
    use std::cell::RefCell;
    use std::collections::HashMap;

    const API: &str = "https://api.test";
    const RAW: &str = "https://raw.test";
    const DL: &str = "https://dl.test";
    const OWN: &str = "https://github.com/OpenRAC/rac1-mods";
    const NOW: &str = "2026-10-31T12:00:00Z";

    fn here() -> &'static str {
        plugins::platform_key().unwrap_or("linux-x86_64")
    }

    #[derive(Default)]
    struct Fixture {
        files: HashMap<String, Vec<u8>>,
        asked: RefCell<Vec<String>>,
    }

    impl remote::Fetch for Fixture {
        fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
            self.asked.borrow_mut().push(url.to_string());
            self.files.get(url).cloned().ok_or_else(|| format!("{url}: not found"))
        }
    }

    impl Fixture {
        fn put(&mut self, url: &str, bytes: impl Into<Vec<u8>>) {
            self.files.insert(url.to_string(), bytes.into());
        }

        fn requests(&self) -> usize {
            self.asked.borrow().len()
        }
    }

    fn manifest(id: &str, version: &str, extra: &Value) -> Value {
        let mut m = json!({
            "format": 2, "id": id, "name": format!("Mod {id}"), "version": version, "author": "Someone",
            "api": "1.0", "games": {"rac1": ["pal", "ntsc"]},
            "plugin": {(here()): format!("lib/{}/m.so", here())},
        });
        for (k, v) in extra.as_object().unwrap() {
            m[k] = v.clone();
        }
        m
    }

    fn mod_zip(id: &str, version: &str, extra: &Value) -> Vec<u8> {
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file(format!("{id}/openrac-mod.json"), o).unwrap();
        w.write_all(manifest(id, version, extra).to_string().as_bytes()).unwrap();
        w.start_file(format!("{id}/lib/{}/m.so", here()), o).unwrap();
        w.write_all(b"library").unwrap();
        w.finish().unwrap().into_inner()
    }

    fn sha(bytes: &[u8]) -> String {
        format!("{:x}", Sha256::digest(bytes))
    }

    /// Publishes mod `id` in OpenRAC/rac1-mods under its own tag, as the repository's workflow does,
    /// and answers the entry a code holds for it.
    fn publish_at(fx: &mut Fixture, repo: &str, id: &str, version: &str, extra: &Value) -> SetMod {
        let tag = format!("{id}-v{version}");
        let name = remote::asset_name(id, version, here());
        let zip = mod_zip(id, version, extra);
        let sums = format!("{}  {name}\n{}  {id}-{version}-other.zip\n", sha(&zip), "0".repeat(64));
        fx.put(&format!("{DL}/{tag}/{name}"), zip);
        fx.put(&format!("{DL}/{tag}/SHA256SUMS"), sums.clone());
        fx.put(
            &format!("{API}/repos/{repo}/releases/tags/{tag}"),
            json!({"tag_name": tag, "published_at": "2020-01-01T00:00:00Z", "assets": [
                {"name": name, "browser_download_url": format!("{DL}/{tag}/{name}")},
                {"name": "SHA256SUMS", "browser_download_url": format!("{DL}/{tag}/SHA256SUMS")},
            ]})
            .to_string(),
        );
        fx.put(&format!("{RAW}/{repo}/{tag}/{id}/openrac-mod.json"), manifest(id, version, extra).to_string());
        SetMod {
            id: id.into(),
            version: version.into(),
            kind: extra.get("multiplayer").and_then(Value::as_str).unwrap_or("gameplay").into(),
            source: SetSource {
                url: format!("https://github.com/{repo}"),
                path: id.into(),
                tag: Some(tag),
                commit: None,
            },
            sums: Some(sha(sums.as_bytes())),
            settings: BTreeMap::new(),
        }
    }

    fn publish(fx: &mut Fixture, id: &str, version: &str, extra: &Value) -> SetMod {
        publish_at(fx, "OpenRAC/rac1-mods", id, version, extra)
    }

    fn ctx<'a>(dir: &'a Path, fetch: &'a dyn remote::Fetch) -> Context<'a> {
        Context {
            dir,
            game: "rac1",
            version: "pal",
            platform: Some(here()),
            fetch,
            hosts: remote::Hosts { api: API.into(), raw: RAW.into() },
        }
    }

    fn set_of(mods: Vec<SetMod>) -> ModSet {
        ModSet {
            format: FORMAT,
            name: "Ada's mods".into(),
            created: NOW.into(),
            game: "rac1".into(),
            discs: vec!["pal".into(), "ntsc".into()],
            openrac: "0.10.1".into(),
            mods,
            extras: Vec::new(),
            manual: Vec::new(),
            packs: Vec::new(),
            digest: None,
        }
    }

    fn json_of(set: &ModSet) -> String {
        serde_json::to_string(set).unwrap()
    }

    /// Installs a mod through the code path, as if from an earlier code.
    fn have(dir: &Path, fx: &Fixture, m: &SetMod) {
        let c = ctx(dir, fx);
        let pinned = remote::read_pinned(&c, &m.pin()).unwrap();
        let staged = remote::stage_pinned(&c, &pinned).unwrap();
        remote::install_staged(&c, &pinned, &staged).unwrap();
    }

    /// Installs a mod from a folder: it has no source.
    fn have_local(dir: &Path, id: &str, version: &str, extra: &Value) {
        let t = tempfile::tempdir().unwrap();
        let folder = t.path().join(id);
        std::fs::create_dir_all(folder.join(format!("lib/{}", here()))).unwrap();
        std::fs::write(folder.join(format!("lib/{}/m.so", here())), "library").unwrap();
        std::fs::write(folder.join("openrac-mod.json"), manifest(id, version, extra).to_string()).unwrap();
        plugins::install(dir, "rac1", "pal", &folder).unwrap();
    }

    fn on(dir: &Path, id: &str) -> bool {
        plugins::stored(dir).unwrap().get(id).is_some_and(|s| s.0)
    }

    fn version_of(dir: &Path, id: &str) -> String {
        plugins::read_manifest(&dir.join(id)).unwrap().version
    }

    fn snapshot(dir: &Path) -> BTreeMap<String, Vec<u8>> {
        fn walk(at: &Path, root: &Path, out: &mut BTreeMap<String, Vec<u8>>) {
            for e in std::fs::read_dir(at).into_iter().flatten().flatten() {
                let rel = e.path().strip_prefix(root).unwrap().to_string_lossy().into_owned();
                if rel.starts_with(".cache") || rel.starts_with("profiles") {
                    continue;
                }
                if e.path().is_dir() {
                    walk(&e.path(), root, out);
                } else {
                    out.insert(rel, std::fs::read(e.path()).unwrap());
                }
            }
        }
        let mut out = BTreeMap::new();
        walk(dir, dir, &mut out);
        out
    }

    #[test]
    fn a_long_code_comes_back_as_the_same_set() {
        let mut m = SetMod {
            id: "difficulty-mod".into(),
            version: "1.4.0".into(),
            kind: "gameplay".into(),
            source: SetSource {
                url: OWN.into(),
                path: "difficulty-mod".into(),
                tag: Some("difficulty-mod-v1.4.0".into()),
                commit: None,
            },
            sums: Some("a".repeat(64)),
            settings: BTreeMap::new(),
        };
        m.settings.insert("enemy_count".into(), json!(2));
        m.settings.insert("hard".into(), json!(true));
        let mut set = set_of(vec![m]);
        set.manual.push(SetManual { id: "thing".into(), version: "1.0.0".into(), name: "A thing".into() });
        set.packs.push(SetPack { id: "hd-pack".into(), digest: None });
        let long = to_long(&set).unwrap();
        assert!(long.starts_with(LONG_PREFIX));
        assert!(long.bytes().all(|b| b.is_ascii_alphanumeric() || b"._-".contains(&b)), "{long}");
        assert_eq!(from_long(&long).unwrap(), set);
    }

    #[test]
    fn a_damaged_long_code_is_refused() {
        let long = to_long(&set_of(vec![])).unwrap();
        let mut flipped = long.clone().into_bytes();
        let at = flipped.len() - 6;
        flipped[at] = if flipped[at] == b'A' { b'B' } else { b'A' };
        assert!(from_long(std::str::from_utf8(&flipped).unwrap()).is_err());
        assert!(from_long(&long[..long.len() - 8]).is_err());
        assert!(from_long("ORMS1.!!!").is_err());
        assert!(from_long("ORMS2.abcd").is_err());
    }

    #[test]
    fn a_bomb_stops_at_the_cap() {
        let json = format!(r#"{{"name":"{}"}}"#, " ".repeat(10 * MAX_JSON));
        let mut e = flate2::write::ZlibEncoder::new(Vec::new(), flate2::Compression::best());
        e.write_all(json.as_bytes()).unwrap();
        let bytes = e.finish().unwrap();
        assert!(bytes.len() < MAX_COMPRESSED);
        assert!(decompress(&bytes).is_err());
    }

    #[test]
    fn a_large_set_is_refused_when_shared() {
        let mut set = set_of(vec![]);
        for i in 0..MAX_MODS {
            set.manual.push(SetManual {
                id: format!("mod-{i}"),
                version: "1.0.0".into(),
                name: (0..40).map(|n| char::from(b'a' + ((i * 7 + n * n * 13) % 26) as u8)).collect(),
            });
        }
        let mut more = set.clone();
        for n in 0..8u8 {
            more.packs
                .extend((0..MAX_MODS).map(|i| SetPack { id: format!("p-{i}"), digest: Some(sha(&[i as u8, n])) }));
        }
        assert!(compress(&set).is_ok());
        assert!(compress(&more).is_err());
    }

    #[test]
    fn unknown_keys_are_refused_at_every_level() {
        let m = SetMod {
            id: "a".into(),
            version: "1.0.0".into(),
            kind: "gameplay".into(),
            source: SetSource { url: OWN.into(), path: "a".into(), tag: Some("a-v1.0.0".into()), commit: None },
            sums: Some("a".repeat(64)),
            settings: BTreeMap::new(),
        };
        let mut set = serde_json::to_value(set_of(vec![m])).unwrap();
        assert!(ModSet::from_json(set.to_string().as_bytes()).is_ok());
        set["blob"] = json!("x");
        assert!(ModSet::from_json(set.to_string().as_bytes()).is_err());
        set.as_object_mut().unwrap().remove("blob");
        set["mods"][0]["url"] = json!("https://evil.example/a.zip");
        assert!(ModSet::from_json(set.to_string().as_bytes()).is_err());
        set["mods"][0].as_object_mut().unwrap().remove("url");
        set["mods"][0]["source"]["asset"] = json!("x");
        assert!(ModSet::from_json(set.to_string().as_bytes()).is_err());
    }

    #[test]
    fn a_newer_format_says_so() {
        let mut set = serde_json::to_value(set_of(vec![])).unwrap();
        set["format"] = json!(2);
        set["future"] = json!(1);
        assert!(ModSet::from_json(set.to_string().as_bytes()).unwrap_err().contains("newer launcher"));
    }

    #[test]
    fn what_is_in_a_set_follows_the_rules() {
        let good = SetMod {
            id: "a".into(),
            version: "1.0.0".into(),
            kind: "gameplay".into(),
            source: SetSource { url: OWN.into(), path: "a".into(), tag: Some("a-v1.0.0".into()), commit: None },
            sums: Some("a".repeat(64)),
            settings: BTreeMap::new(),
        };
        assert!(good.check().is_ok());
        let breaks = |change: &dyn Fn(&mut SetMod)| {
            let mut m = good.clone();
            change(&mut m);
            m.check().is_err()
        };
        assert!(breaks(&|m| m.id = "../evil".into()));
        assert!(breaks(&|m| m.id = "A".into()));
        assert!(breaks(&|m| m.version = "latest".into()));
        assert!(breaks(&|m| m.kind = "native".into()));
        assert!(breaks(&|m| m.source.path = "../x".into()));
        assert!(breaks(&|m| m.source.path = "/etc".into()));
        assert!(breaks(&|m| m.source.path = "a\\b".into()));
        assert!(breaks(&|m| m.source.tag = Some("../x".into())));
        assert!(breaks(&|m| m.source.url = "https://gitlab.com/x/y".into()));
        assert!(breaks(&|m| m.source.url = "http://github.com/x/y".into()));
        assert!(breaks(&|m| m.source.url = "https://github.com/x/y/releases/tag/v1".into()));
        assert!(breaks(&|m| m.source.url = "git@github.com:x/y.git".into()));
        assert!(breaks(&|m| m.sums = Some("xyz".into())));
        // Unpinned, or half pinned, is not a code's.
        assert!(breaks(&|m| m.sums = None));
        assert!(breaks(&|m| {
            m.sums = None;
            m.source.tag = None;
        }));
        assert!(!breaks(&|m| {
            m.sums = None;
            m.source.tag = None;
            m.source.commit = Some("b".repeat(40));
        }));
        assert!(breaks(&|m| {
            m.settings.insert("a".into(), json!({"nested": 1}));
        }));
        assert!(breaks(&|m| {
            m.settings.insert("a".into(), json!("https://evil.example"));
        }));
        assert!(breaks(&|m| {
            m.settings.insert("A-B".into(), json!(1));
        }));
    }

    #[test]
    fn the_same_mod_twice_is_refused() {
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "a", "1.0.0", &json!({}));
        assert!(set_of(vec![m.clone(), m]).validate().is_err());
    }

    #[test]
    fn entries_read_every_way() {
        for text in [
            "K7M2-9QXR",
            "k7m29qxr",
            "  K7M2 9QXR ",
            "k7m2\u{2013}9qxr",
            "openrac://modset/K7M2-9QXR",
            "openrac://modset/K7M29QXR/",
        ] {
            assert_eq!(parse_entry(text).unwrap(), Entered::Short("K7M29QXR".into()), "{text}");
        }
        assert_eq!(parse_entry("openrac://modset/ORMS1.abc").unwrap(), Entered::Long("ORMS1.abc".into()));
        assert_eq!(parse_entry("ORMS1.aB-_").unwrap(), Entered::Long("ORMS1.aB-_".into()));
        assert!(parse_entry("K7M0-9QXR").unwrap_err().contains("no 0"));
        assert!(parse_entry("K7MO-9QXR").unwrap_err().contains("no O"));
        assert!(parse_entry("K7M1-9QXR").unwrap_err().contains("no 1"));
        assert!(parse_entry("K7MI-9QXR").unwrap_err().contains("no I"));
        assert!(parse_entry("K7M2").unwrap_err().contains("8 characters"));
        assert!(parse_entry("").is_err());
        assert!(parse_entry("https://evil.example/x").is_err());
        assert_eq!(show_code("K7M29QXR"), "K7M2-9QXR");
    }

    #[derive(Default)]
    struct MemoryStore {
        sets: RefCell<HashMap<String, Vec<u8>>>,
        down: bool,
    }

    impl Store for MemoryStore {
        fn put(&self, body: &[u8]) -> Result<String, StoreError> {
            if self.down {
                return Err(StoreError::Unreachable);
            }
            self.sets.borrow_mut().insert("K7M29QXR".into(), body.to_vec());
            Ok("K7M29QXR".into())
        }

        fn get(&self, code: &str) -> Result<Vec<u8>, StoreError> {
            if self.down {
                return Err(StoreError::Unreachable);
            }
            self.sets.borrow().get(code).cloned().ok_or(StoreError::NotFound)
        }
    }

    #[test]
    fn a_short_and_a_long_code_mean_the_same_set() {
        let t = tempfile::tempdir().unwrap();
        let store = MemoryStore::default();
        let set = set_of(vec![]);
        let shared = share(t.path(), &store, &set).unwrap();
        assert_eq!(shared.code.as_deref(), Some("K7M2-9QXR"));
        assert_eq!(shared.link, "openrac://modset/K7M2-9QXR");
        let (a, code) = resolve(&store, "k7m2-9qxr").unwrap();
        let (b, none) = resolve(&store, &shared.long).unwrap();
        assert_eq!((a, b, code.as_deref(), none), (set.clone(), set, Some("K7M2-9QXR"), None));
    }

    #[test]
    fn with_no_server_the_long_code_still_works() {
        let t = tempfile::tempdir().unwrap();
        let store = MemoryStore { down: true, ..Default::default() };
        let set = set_of(vec![]);
        let shared = share(t.path(), &store, &set).unwrap();
        assert_eq!(shared.code, None);
        assert!(shared.link.starts_with("openrac://modset/ORMS1."));
        assert_eq!(resolve(&store, &shared.long).unwrap().0, set);
        assert!(resolve(&store, "K7M2-9QXR").unwrap_err().contains("full code"));
    }

    #[test]
    fn an_unknown_code_asks_for_a_new_one() {
        assert!(resolve(&MemoryStore::default(), "K7M2-9QXR").unwrap_err().contains("Ask for a new one"));
    }

    #[test]
    fn the_real_client_speaks_to_a_socket() {
        use std::net::TcpListener;
        let listener = TcpListener::bind("127.0.0.1:0").unwrap();
        let address = listener.local_addr().unwrap().to_string();
        let server = std::thread::spawn(move || {
            let mut seen = Vec::new();
            for answer in [
                "HTTP/1.1 200 OK\r\nContent-Length: 19\r\n\r\n{\"code\":\"K7M29QXR\"}".as_bytes().to_vec(),
                b"HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabc".to_vec(),
                b"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n".to_vec(),
                b"HTTP/1.1 429 Too Many Requests\r\nContent-Length: 0\r\n\r\n".to_vec(),
                b"HTTP/1.1 200 OK\r\nContent-Length: 30\r\n\r\ncut".to_vec(),
            ] {
                let (mut stream, _) = listener.accept().unwrap();
                let mut buffer = [0u8; 1024];
                let got = stream.read(&mut buffer).unwrap();
                seen.push(String::from_utf8_lossy(&buffer[..got]).into_owned());
                stream.write_all(&answer).unwrap();
            }
            seen
        });
        let store = HttpStore { address };
        assert_eq!(store.put(b"xyz"), Ok("K7M29QXR".to_string()));
        assert_eq!(store.get("K7M29QXR"), Ok(b"abc".to_vec()));
        assert_eq!(store.get("AAAAAAAA"), Err(StoreError::NotFound));
        assert_eq!(store.get("AAAAAAAA"), Err(StoreError::Refused));
        assert_eq!(store.get("AAAAAAAA"), Err(StoreError::Unreachable));
        let seen = server.join().unwrap();
        assert!(seen[0].starts_with("PUT /v1/modsets HTTP/1.1\r\n") && seen[0].contains("Content-Length: 3"));
        assert!(seen[1].starts_with("GET /v1/modsets/K7M29QXR HTTP/1.1\r\n"));
    }

    #[test]
    fn a_review_changes_nothing_and_says_everything() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let spec = json!({"settings": [
            {"key": "enemies", "name": "Enemies", "type": "int", "default": 1},
            {"key": "seed", "name": "Seed", "type": "int", "default": 0, "multiplayer": "host"},
        ]});
        let old = publish(&mut fx, "difficulty-mod", "1.3.0", &spec);
        let mut new = publish(&mut fx, "difficulty-mod", "1.4.0", &spec);
        new.settings.insert("enemies".into(), json!(3));
        new.settings.insert("seed".into(), json!(9));
        new.settings.insert("gone".into(), json!(1));
        let fresh = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        have(dir, &fx, &old);
        have_local(dir, "other-gameplay", "1.0.0", &json!({}));
        have_local(dir, "mouse", "1.0.0", &json!({"multiplayer": "input"}));
        let mut set = set_of(vec![new, fresh]);
        set.manual.push(SetManual { id: "by-hand".into(), version: "1.0.0".into(), name: "By hand".into() });
        set.packs.push(SetPack { id: "hd-pack".into(), digest: None });
        let before = snapshot(dir);
        let c = ctx(dir, &fx);
        let r = review(&c, &set, Some("K7M2-9QXR".into()), "0.9.0").unwrap();
        assert_eq!(snapshot(dir), before, "a review changes nothing");
        let actions: Vec<(&str, &str)> = r.mods.iter().map(|m| (m.id.as_str(), m.action.as_str())).collect();
        assert_eq!(actions, [("difficulty-mod", "update"), ("ammo-max", "install")]);
        assert_eq!(r.mods[0].from.as_deref(), Some("1.3.0"));
        assert!(r.mods[1].native);
        assert_eq!(r.disable.iter().map(|m| m.id.as_str()).collect::<Vec<_>>(), ["other-gameplay"]);
        assert_eq!(r.settings.len(), 1, "a host setting and an unknown key are not the code's to change");
        assert_eq!(
            (r.settings[0].name.as_str(), &r.settings[0].from, &r.settings[0].to),
            ("Enemies", &json!(1), &json!(3))
        );
        assert_eq!(r.manual.len(), 1);
        assert_eq!(r.packs, ["hd-pack"]);
        assert_eq!(r.other_openrac.as_deref(), Some("0.10.1"));
        assert!(r.partial && !r.nothing);
        assert!(r.ask.is_empty(), "OpenRAC's own repositories are not asked about");
    }

    #[test]
    fn a_retagged_release_is_refused() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let mut bad = m.clone();
        bad.sums = Some("f".repeat(64));
        let c = ctx(t.path(), &fx);
        let r = review(&c, &set_of(vec![bad.clone()]), None, "").unwrap();
        assert_eq!(r.mods[0].action, "unavailable");
        assert!(r.mods[0].why.as_ref().unwrap().contains("changed"));
        assert!(r.partial);
    }

    #[test]
    fn a_download_that_differs_from_its_checksums_never_installs() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let name = remote::asset_name("ammo-max", "1.0.0", here());
        fx.put(&format!("{DL}/ammo-max-v1.0.0/{name}"), mod_zip("evil", "1.0.0", &json!({})));
        let c = ctx(t.path(), &fx);
        let err = apply(&c, &json_of(&set_of(vec![m])), None, &Choices::default(), "", NOW).unwrap_err();
        assert!(err.contains("SHA-256"), "{err}");
        assert!(!t.path().join("ammo-max").exists());
        assert!(!t.path().join("evil").exists());
    }

    #[test]
    fn a_version_that_is_gone_is_never_replaced() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        fx.files.remove(&format!("{API}/repos/OpenRAC/rac1-mods/releases/tags/ammo-max-v1.0.0"));
        let zip = |v: &str, p: &str| json!({"name": format!("ammo-max-{v}-{p}.zip"), "browser_download_url": "x"});
        fx.put(
            &format!("{API}/repos/OpenRAC/rac1-mods/releases?per_page=100"),
            json!([
                {"tag_name": "ammo-max-v1.2.0", "assets": [zip("1.2.0", here()), zip("1.2.0", "windows-x86_64")]},
                {"tag_name": "ammo-max-v1.1.0", "assets": [zip("1.1.0", here())]},
                {"tag_name": "ammo-max-v9.0.0", "prerelease": true, "assets": [zip("9.0.0", here())]},
                {"tag_name": "ammo-maximum-v5.0.0", "assets": [{"name": "ammo-maximum-5.0.0.zip", "browser_download_url": "x"}]},
            ])
            .to_string(),
        );
        let c = ctx(t.path(), &fx);
        let set = set_of(vec![m]);
        let r = review(&c, &set, None, "").unwrap();
        assert_eq!(r.mods[0].action, "unavailable");
        assert_eq!(r.mods[0].nearest.as_deref(), Some("1.2.0"));
        let done = apply(&c, &json_of(&set), None, &Choices::default(), "", NOW).unwrap();
        assert_eq!(done.skipped, ["ammo-max"]);
        assert!(!t.path().join("ammo-max").exists());
    }

    #[test]
    fn no_build_for_this_computer_is_said() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let mut c = ctx(t.path(), &fx);
        // The manifest does not list that platform.
        c.platform = Some("android-arm64");
        let r = review(&c, &set_of(vec![m]), None, "").unwrap();
        assert_eq!(r.mods[0].action, "unavailable");
        assert!(r.mods[0].why.as_ref().unwrap().contains("this computer"), "{:?}", r.mods[0].why);
    }

    #[test]
    fn another_disc_never_names_a_region() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({"games": {"rac1": ["ntsc"]}}));
        let c = ctx(t.path(), &fx);
        let r = review(&c, &set_of(vec![m]), None, "").unwrap();
        let why = r.mods[0].why.as_ref().unwrap();
        assert_eq!(why, "It doesn't run on your disc.");
    }

    #[test]
    fn one_code_serves_both_discs() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let set = set_of(vec![m]);
        let mut c = ctx(t.path(), &fx);
        c.version = "ntsc";
        assert_eq!(review(&c, &set, None, "").unwrap().mods[0].action, "install");
        apply(&c, &json_of(&set), None, &Choices::default(), "", NOW).unwrap();
        assert!(on(t.path(), "ammo-max"));
    }

    #[test]
    fn a_code_for_another_game_applies_nothing() {
        let t = tempfile::tempdir().unwrap();
        let fx = Fixture::default();
        let mut set = set_of(vec![]);
        set.game = "rac2".into();
        let c = ctx(t.path(), &fx);
        assert!(review(&c, &set, None, "").unwrap().wrong_game);
        assert!(apply(&c, &json_of(&set), None, &Choices::default(), "", NOW).is_err());
    }

    #[test]
    fn apply_gives_exactly_the_reviewed_set() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let spec = json!({"settings": [
            {"key": "enemies", "name": "Enemies", "type": "int", "default": 1, "min": 0, "max": 9},
            {"key": "mine", "name": "Mine", "type": "int", "default": 1, "multiplayer": "own"},
        ]});
        let old = publish(&mut fx, "difficulty-mod", "1.5.0", &spec);
        let mut wanted = publish(&mut fx, "difficulty-mod", "1.4.0", &spec);
        wanted.settings.insert("enemies".into(), json!(3));
        wanted.settings.insert("mine".into(), json!(7));
        let added = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        have(dir, &fx, &old);
        have_local(dir, "other-gameplay", "1.0.0", &json!({}));
        have_local(dir, "solo-thing", "1.0.0", &json!({"multiplayer": "solo"}));
        have_local(dir, "mouse", "1.0.0", &json!({"multiplayer": "input"}));
        have_local(dir, "hud", "1.0.0", &json!({"multiplayer": "cosmetic"}));
        have_local(dir, "multiplayer", "0.9.0", &json!({}));
        let c = ctx(dir, &fx);
        let set = set_of(vec![added.clone(), wanted]);
        let done = apply(&c, &json_of(&set), Some("K7M2-9QXR"), &Choices::default(), "", NOW).unwrap();
        assert_eq!(version_of(dir, "difficulty-mod"), "1.4.0", "a code can go down as well as up");
        assert_eq!(version_of(dir, "ammo-max"), "1.0.0");
        assert!(on(dir, "difficulty-mod") && on(dir, "ammo-max"));
        assert!(!on(dir, "other-gameplay") && !on(dir, "solo-thing"));
        assert!(on(dir, "mouse") && on(dir, "hud"), "input and cosmetic mods are the player's");
        assert!(on(dir, "multiplayer"), "a code leaves Multiplayer itself alone");
        let values = plugins::stored(dir).unwrap()["difficulty-mod"].1.clone();
        assert_eq!(values.get("enemies"), Some(&json!(3)));
        assert_eq!(values.get("mine"), None, "a player's own setting is never set by a code");
        assert_eq!(done.ignored, ["mine"]);
        assert_eq!(done.installed, 2);
        assert_eq!(done.disabled, 2);
        // The set's order among its own mods, the host's load order.
        let order: Vec<String> = plugins::load_order(dir).unwrap().into_iter().map(|e| e.0).collect();
        let at = |id: &str| order.iter().position(|x| x == id).unwrap();
        assert!(at("ammo-max") < at("difficulty-mod"), "{order:?}");
        // What was installed is remembered exactly.
        let source = remote::read_sources(dir).mods["ammo-max"].clone();
        assert_eq!((source.tag, source.sums), (added.source.tag, added.sums));
        // The game can show the code of what it plays.
        let file: Value = serde_json::from_slice(&std::fs::read(dir.join(GAME_FILE)).unwrap()).unwrap();
        assert_eq!(file["code"], "K7M2-9QXR");
        assert_eq!(file["mods"][0], json!({"id": "ammo-max", "version": "1.0.0"}));
        assert!(!file["mods"].to_string().contains("multiplayer"));
    }

    #[test]
    fn applying_a_second_time_has_nothing_to_do_and_no_network() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let c = ctx(dir, &fx);
        let set = set_of(vec![m]);
        apply(&c, &json_of(&set), None, &Choices::default(), "", NOW).unwrap();
        let asked = fx.requests();
        let r = review(&c, &set, None, "").unwrap();
        assert!(r.nothing);
        assert_eq!(r.mods[0].action, "keep");
        assert_eq!(fx.requests(), asked, "what is in place needs no request");
    }

    #[test]
    fn a_failed_download_changes_nothing() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let old = publish(&mut fx, "ammo-max", "0.9.0", &json!({}));
        let first = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let second = publish(&mut fx, "lock-strafe", "1.0.0", &json!({}));
        have(dir, &fx, &old);
        let name = remote::asset_name("lock-strafe", "1.0.0", here());
        fx.files.remove(&format!("{DL}/lock-strafe-v1.0.0/{name}"));
        let before = snapshot(dir);
        let c = ctx(dir, &fx);
        let err = apply(&c, &json_of(&set_of(vec![first, second])), None, &Choices::default(), "", NOW).unwrap_err();
        assert!(err.contains("couldn't be downloaded"), "{err}");
        assert_eq!(snapshot(dir), before);
        assert!(profile::list(dir).is_empty(), "nothing started, so nothing to go back to");
    }

    #[test]
    fn a_failure_half_way_puts_everything_back() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let old = publish(&mut fx, "ammo-max", "0.9.0", &json!({}));
        let first = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        // This archive passes every check of the download and fails at its install: its manifest
        // names a library it does not hold.
        let second = publish(&mut fx, "lock-strafe", "1.0.0", &json!({}));
        let name = remote::asset_name("lock-strafe", "1.0.0", here());
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file("lock-strafe/openrac-mod.json", o).unwrap();
        w.write_all(manifest("lock-strafe", "1.0.0", &json!({})).to_string().as_bytes()).unwrap();
        let broken = w.finish().unwrap().into_inner();
        let sums = format!("{}  {name}\n", sha(&broken));
        fx.put(&format!("{DL}/lock-strafe-v1.0.0/{name}"), broken);
        fx.put(&format!("{DL}/lock-strafe-v1.0.0/SHA256SUMS"), sums.clone());
        let mut second = second;
        second.sums = Some(sha(sums.as_bytes()));
        have(dir, &fx, &old);
        have_local(dir, "other-gameplay", "1.0.0", &json!({}));
        let before = snapshot(dir);
        let c = ctx(dir, &fx);
        let err = apply(&c, &json_of(&set_of(vec![first, second])), None, &Choices::default(), "", NOW).unwrap_err();
        assert!(err.contains("as they were"), "{err}");
        assert_eq!(snapshot(dir), before, "the first mod's update and the list went back");
        assert!(profile::list(dir).is_empty());
    }

    #[test]
    fn restore_brings_the_old_set_back() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let old = publish(&mut fx, "ammo-max", "0.9.0", &json!({}));
        let new = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        have(dir, &fx, &old);
        have_local(dir, "other-gameplay", "1.0.0", &json!({}));
        let before = snapshot(dir);
        let c = ctx(dir, &fx);
        let done = apply(&c, &json_of(&set_of(vec![new])), Some("K7M2-9QXR"), &Choices::default(), "", NOW).unwrap();
        assert_eq!(version_of(dir, "ammo-max"), "1.0.0");
        assert_eq!(profile::list(dir)[0].code, "K7M2-9QXR");
        restore(dir, &done.profile).unwrap();
        assert_eq!(snapshot(dir), before);
    }

    #[test]
    fn a_source_outside_openrac_asks_first_and_is_remembered() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish_at(&mut fx, "someone/mods", "ammo-max", "1.0.0", &json!({}));
        let url = m.source.url.clone();
        let set = set_of(vec![m.clone()]);
        let c = ctx(dir, &fx);
        let r = review(&c, &set, None, "").unwrap();
        assert_eq!(r.ask, [ReviewSource { url: url.clone(), label: "someone/mods".into() }]);
        let err = apply(&c, &json_of(&set), None, &Choices::default(), "", NOW).unwrap_err();
        assert!(err.contains("someone/mods"), "{err}");
        assert!(!dir.join("ammo-max").exists());
        let yes = Choices { trusted: vec![url.clone()], ..Default::default() };
        apply(&c, &json_of(&set), None, &yes, "", NOW).unwrap();
        assert!(dir.join("ammo-max").is_dir());
        assert_eq!(read_trusted(dir), [url]);
        // Its next version is from a known source.
        let mut fx2 = Fixture::default();
        let next = publish_at(&mut fx2, "someone/mods", "ammo-max", "1.1.0", &json!({}));
        assert!(review(&ctx(dir, &fx2), &set_of(vec![next]), None, "").unwrap().ask.is_empty());
    }

    #[test]
    fn a_source_the_player_installed_from_needs_no_asking() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish_at(&mut fx, "someone/mods", "ammo-max", "1.0.0", &json!({}));
        let mut sources = remote::read_sources(dir);
        sources.mods.insert(
            "x".into(),
            remote::ModSource { url: "https://github.com/someone/mods".into(), ..Default::default() },
        );
        remote::write_sources(dir, &sources).unwrap();
        assert!(review(&ctx(dir, &fx), &set_of(vec![m]), None, "").unwrap().ask.is_empty());
    }

    #[test]
    fn a_new_release_from_outside_openrac_is_flagged() {
        let t = tempfile::tempdir().unwrap();
        let mut fx = Fixture::default();
        let young = format_time(SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs() - 3600);
        let m = publish_at(&mut fx, "someone/mods", "ammo-max", "1.0.0", &json!({}));
        let url = format!("{API}/repos/someone/mods/releases/tags/ammo-max-v1.0.0");
        let text = String::from_utf8(fx.files[&url].clone()).unwrap().replace("2020-01-01T00:00:00Z", &young);
        fx.put(&url, text.clone());
        let own = publish(&mut fx, "lock-strafe", "1.0.0", &json!({}));
        let own_url = format!("{API}/repos/OpenRAC/rac1-mods/releases/tags/lock-strafe-v1.0.0");
        let own_text = String::from_utf8(fx.files[&own_url].clone()).unwrap().replace("2020-01-01T00:00:00Z", &young);
        fx.put(&own_url, own_text);
        let r = review(&ctx(t.path(), &fx), &set_of(vec![m, own]), None, "").unwrap();
        assert!(r.mods[0].fresh, "from outside OpenRAC");
        assert!(!r.mods[1].fresh, "OpenRAC's own");
    }

    #[test]
    fn optional_mods_wait_for_a_tick() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let gameplay = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let hud = publish(&mut fx, "hud", "1.0.0", &json!({"multiplayer": "cosmetic"}));
        let pad = publish(&mut fx, "pad", "1.0.0", &json!({"multiplayer": "input"}));
        let mut set = set_of(vec![gameplay]);
        set.extras = vec![hud, pad];
        let c = ctx(dir, &fx);
        assert!(review(&c, &set, None, "").unwrap().mods[1].optional);
        let choices = Choices { extras: vec!["pad".into()], ..Default::default() };
        apply(&c, &json_of(&set), None, &choices, "", NOW).unwrap();
        assert!(dir.join("pad").is_dir());
        assert!(!dir.join("hud").exists());
    }

    #[test]
    fn a_fetched_archive_is_kept_by_its_hash_and_checked_when_reused() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        let name = remote::asset_name("ammo-max", "1.0.0", here());
        let asset = format!("{DL}/ammo-max-v1.0.0/{name}");
        let hash = sha(&fx.files[&asset]);
        let c = ctx(dir, &fx);
        let count = || fx.asked.borrow().iter().filter(|u| **u == asset).count();
        let pinned = remote::read_pinned(&c, &m.pin()).unwrap();
        remote::stage_pinned(&c, &pinned).unwrap();
        assert_eq!(count(), 1);
        remote::stage_pinned(&c, &pinned).unwrap();
        assert_eq!(count(), 1, "the second comes from the cache");
        // A cached file that was changed is not believed.
        let file = dir.join(".cache").join(format!("{hash}.zip"));
        std::fs::write(&file, b"changed").unwrap();
        remote::stage_pinned(&c, &pinned).unwrap();
        assert_eq!(count(), 2);
        assert_eq!(sha(&std::fs::read(file).unwrap()), hash);
    }

    #[test]
    fn a_commit_pinned_script_installs_from_its_commit() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let commit = "c".repeat(40);
        let script = json!({"format": 2, "id": "swap", "name": "Swap", "version": "1.0.0", "type": "script", "api": "1.1",
            "games": {"rac1": ["pal", "ntsc"]}, "scripts": ["main.luau"], "multiplayer": "gameplay"});
        fx.put(&format!("{RAW}/OpenRAC/rac1-mods/{commit}/swap/openrac-mod.json"), script.to_string());
        let mut w = zip::ZipWriter::new(std::io::Cursor::new(Vec::new()));
        let o = zip::write::SimpleFileOptions::default();
        w.start_file("repo-abc/swap/openrac-mod.json", o).unwrap();
        w.write_all(script.to_string().as_bytes()).unwrap();
        w.start_file("repo-abc/swap/main.luau", o).unwrap();
        w.write_all(b"-- hello").unwrap();
        fx.put(&format!("{API}/repos/OpenRAC/rac1-mods/zipball/{commit}"), w.finish().unwrap().into_inner());
        let m = SetMod {
            id: "swap".into(),
            version: "1.0.0".into(),
            kind: "gameplay".into(),
            source: SetSource { url: OWN.into(), path: "swap".into(), tag: None, commit: Some(commit) },
            sums: None,
            settings: BTreeMap::new(),
        };
        let c = ctx(dir, &fx);
        apply(&c, &json_of(&set_of(vec![m])), None, &Choices::default(), "", NOW).unwrap();
        assert!(dir.join("swap/main.luau").is_file());
        assert!(on(dir, "swap"));
    }

    #[test]
    fn a_share_holds_what_a_party_compares_and_nothing_personal() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let spec = json!({"settings": [
            {"key": "enemies", "name": "Enemies", "type": "int", "default": 1},
            {"key": "seed", "name": "Seed", "type": "int", "default": 0, "multiplayer": "host"},
            {"key": "trace", "name": "Trace", "type": "bool", "default": false, "multiplayer": "own"},
            {"key": "key", "name": "Key", "type": "keybind", "default": "KeyA"},
            {"key": "who", "name": "Who", "type": "string", "default": "${player_name}"},
        ]});
        let difficulty = publish(&mut fx, "difficulty-mod", "1.4.0", &spec);
        let pad = publish(&mut fx, "pad", "1.0.0", &json!({"multiplayer": "input"}));
        let hud = publish(&mut fx, "hud", "1.0.0", &json!({"multiplayer": "cosmetic"}));
        let multiplayer = publish(
            &mut fx,
            "multiplayer",
            "0.9.0",
            &json!({"settings": [
                {"key": "server", "name": "Server", "type": "string", "default": "game.openrac.dev:7770"},
            ]}),
        );
        let solo = publish(&mut fx, "archipelago", "1.0.0", &json!({"multiplayer": "solo"}));
        for m in [&multiplayer, &difficulty, &pad, &hud, &solo] {
            have(dir, &fx, m);
        }
        have_local(dir, "my-own", "2.0.0", &json!({}));
        plugins::set_setting(dir, "rac1", "pal", "difficulty-mod", "enemies", json!(3), "Ada").unwrap();
        let preview = share_preview(dir).unwrap();
        let ids = |v: &[ShareMod]| v.iter().map(|m| m.id.clone()).collect::<Vec<_>>();
        assert_eq!(ids(&preview.mods), ["multiplayer", "difficulty-mod", "archipelago"]);
        assert_eq!(ids(&preview.optional), ["pad", "hud"]);
        assert_eq!(ids(&preview.manual), ["my-own"]);
        assert_eq!(ids(&preview.solo), ["archipelago"]);
        let c = ctx(dir, &fx);
        let set = build(&c, &["hud".to_string()], "Ada's mods", "0.10.1", Vec::new(), NOW).unwrap();
        assert_eq!(
            set.mods.iter().map(|m| m.id.as_str()).collect::<Vec<_>>(),
            ["multiplayer", "difficulty-mod", "archipelago"]
        );
        assert_eq!(set.extras.iter().map(|m| m.id.as_str()).collect::<Vec<_>>(), ["hud"]);
        assert_eq!(set.manual[0].id, "my-own");
        assert!(set.mods[0].settings.is_empty(), "Multiplayer's settings are the player's");
        let wanted: BTreeMap<String, Value> = [("enemies".to_string(), json!(3))].into_iter().collect();
        assert_eq!(set.mods[1].settings, wanted);
        assert_eq!(set.discs, ["ntsc", "pal"]);
        assert_eq!(set.mods[1].sums, difficulty.sums);
        // Round trip: a friend gets the same.
        assert_eq!(from_long(&to_long(&set).unwrap()).unwrap(), set);
        assert!(!serde_json::to_string(&set).unwrap().contains(dir.to_string_lossy().as_ref()));
    }

    #[test]
    fn the_same_mods_make_the_same_set_whenever() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        have(dir, &fx, &m);
        let c = ctx(dir, &fx);
        let a = build(&c, &[], "Ada", "0.10.1", Vec::new(), NOW).unwrap();
        let b = build(&c, &[], "Ada", "0.10.1", Vec::new(), "2026-11-02T09:30:00Z").unwrap();
        assert_eq!(a, ModSet { created: NOW.into(), ..b });
    }

    #[test]
    fn a_mod_installed_from_a_link_is_shared_at_its_release() {
        // remote::install remembers a tag and no sums: the code reads them from the release.
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let mut fx = Fixture::default();
        let m = publish(&mut fx, "ammo-max", "1.0.0", &json!({}));
        have(dir, &fx, &m);
        let mut sources = remote::read_sources(dir);
        sources.mods.get_mut("ammo-max").unwrap().sums = None;
        remote::write_sources(dir, &sources).unwrap();
        let set = build(&ctx(dir, &fx), &[], "Ada", "", Vec::new(), NOW).unwrap();
        assert_eq!(set.mods[0].sums, m.sums);
    }

    #[test]
    fn a_code_goes_stale_when_what_a_party_compares_changes() {
        let t = tempfile::tempdir().unwrap();
        let dir = t.path();
        let spec = json!({"settings": [
            {"key": "enemies", "name": "Enemies", "type": "int", "default": 1},
            {"key": "mine", "name": "Mine", "type": "int", "default": 1, "multiplayer": "own"},
        ]});
        have_local(dir, "difficulty-mod", "1.0.0", &spec);
        have_local(dir, "mouse", "1.0.0", &json!({"multiplayer": "input"}));
        let code = || std::fs::write(dir.join(GAME_FILE), "{}").unwrap();
        let there = || dir.join(GAME_FILE).exists();
        code();
        plugins::set_setting(dir, "rac1", "pal", "difficulty-mod", "mine", json!(2), "Ada").unwrap();
        plugins::set_enabled(dir, "rac1", "pal", "mouse", false).unwrap();
        assert!(there(), "a player's own setting and an input mod change nothing a party compares");
        plugins::set_setting(dir, "rac1", "pal", "difficulty-mod", "enemies", json!(2), "Ada").unwrap();
        assert!(!there(), "a compared setting does");
        code();
        plugins::set_enabled(dir, "rac1", "pal", "difficulty-mod", false).unwrap();
        assert!(!there(), "so does turning a gameplay mod off");
        code();
        plugins::remove(dir, "rac1", "pal", "difficulty-mod").unwrap();
        assert!(!there(), "and removing it");
    }

    #[test]
    fn a_strange_name_never_stops_a_share() {
        assert_eq!(carried_name("see https://x\n"), "see https x");
        assert_eq!(carried_name(&"a".repeat(200)).len(), MAX_TEXT);
    }

    #[test]
    fn times_read_and_print_back() {
        for text in ["2026-10-31T12:00:00Z", "1970-01-01T00:00:00Z", "2024-02-29T23:59:59Z", "2000-03-01T00:00:00Z"] {
            assert_eq!(format_time(parse_time(text).unwrap()), text);
        }
        assert_eq!(parse_time("1970-01-02T00:00:01Z"), Some(86_401));
        assert_eq!(parse_time("2026-10-31T12:00:00.123Z"), parse_time("2026-10-31T12:00:00Z"));
        assert_eq!(parse_time("yesterday"), None);
    }
}
