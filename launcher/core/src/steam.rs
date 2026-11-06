//! Adds the launcher, or one of its games, to the player's Steam library as a non-Steam game: an
//! entry in `userdata/<id>/config/shortcuts.vdf` and its pictures in `config/grid/`. A game's
//! entry starts the same launcher with `--play <id>` (shortcut.rs) under the game's own name.
//!
//! Steam may be open: it picks up a changed file, and may write its own copy at any moment, so a
//! write that Steam overtakes is made again on top of Steam's copy. Every other entry and every
//! field this file does not know stays as it was: a file that cannot be read is an error, never
//! overwritten, and each write keeps a dated copy (the last three).
//!
//! No controller configuration is written. The entry allows the Desktop Layout in the launcher's
//! window (Steam's "Allow Desktop Configuration in Launcher"), and the game it starts gets Steam's
//! game configuration (the default gamepad layout, or the player's own, which is Steam's to keep).
//!
//! The work takes its folders as arguments, so the tests never touch a real Steam. It is compiled
//! and tested on every platform; Android does not call it.

use std::path::{Path, PathBuf};

use serde::Serialize;

/// The name the entry carries, and how it is found again.
pub const NAME: &str = "OpenRAC";
/// What `add` says when the only Steam is Flathub's.
pub const FLATPAK_STEAM: &str = "Flatpak Steam can't start OpenRAC. Use Steam's normal package to add it.";

/// SteamID64 of account 0: a `loginusers.vdf` id minus this is the `userdata` folder's name.
const STEAM_ID_BASE: u64 = 76_561_197_960_265_728;
/// Deeper than any `shortcuts.vdf` nests; a file past it is not one.
const MAX_DEPTH: usize = 16;

/// Where the launcher stands in the player's Steam.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, ts_rs::TS)]
#[ts(rename = "SteamState")]
#[serde(rename_all = "camelCase")]
pub enum State {
    /// No Steam on this computer.
    NoSteam,
    /// Only Flathub's Steam: its sandbox can't start a program outside it, so nothing is added.
    FlatpakSteam,
    /// Steam, but no account to add it to.
    NoUser,
    NotAdded,
    Added,
    /// Added, but starting a launcher at another path.
    Outdated,
}

/// Which entry: the launcher's own, or one game's. Told apart by name and launch options, so
/// finding the launcher's entry never lands on a game's.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Target {
    pub name: String,
    /// What the entry hands the launcher; empty for the launcher itself.
    pub options: String,
    /// The entry comes with its library pictures (a cover at least): one added without them, by
    /// a launcher from before a game had its own, is shown as to update, which puts them.
    pub pictures: bool,
}

impl Target {
    pub fn launcher() -> Target {
        Target { name: NAME.into(), options: String::new(), pictures: false }
    }

    /// The game `id` (`rac1-pal`) under its `title`.
    pub fn game(title: &str, id: &str) -> Target {
        Target { name: title.into(), options: crate::shortcut::play_options(id), pictures: false }
    }

    /// The same entry, which comes with its library pictures.
    pub fn with_pictures(self) -> Target {
        Target { pictures: true, ..self }
    }

    fn is_launcher(&self) -> bool {
        self.options.is_empty()
    }

    fn matches(&self, entry: &Value) -> bool {
        let Value::Map(fields) = entry else { return false };
        if !matches!(get(fields, "AppName"), Some(Value::Text(name)) if name == self.name.as_bytes()) {
            return false;
        }
        let options = match get(fields, "LaunchOptions") {
            Some(Value::Text(text)) => text.as_slice(),
            _ => b"",
        };
        if self.is_launcher() {
            // The player may have added options of their own to it; a game's entry has this one.
            !options.windows(6).any(|w| w == b"--play")
        } else {
            options == self.options.as_bytes()
        }
    }
}

/// The entry's pictures, as Steam's library shows them. An empty one is not written.
pub struct Art<'a> {
    /// 600 x 900.
    pub cover: &'a [u8],
    /// 920 x 430.
    pub wide: &'a [u8],
    /// 1920 x 620.
    pub hero: &'a [u8],
    /// Transparent, shown over the hero.
    pub logo: &'a [u8],
    pub icon: &'a [u8],
}

/// A value of Steam's binary VDF. Text is bytes: Steam does not promise UTF-8.
#[derive(Debug, Clone, PartialEq)]
enum Value {
    Map(Vec<(Vec<u8>, Value)>),
    Text(Vec<u8>),
    Int(u32),
}

fn read_text(bytes: &[u8], at: &mut usize) -> Result<Vec<u8>, String> {
    let rest = bytes.get(*at..).ok_or("the file ends early")?;
    let end = rest.iter().position(|b| *b == 0).ok_or("a text has no end")?;
    *at += end + 1;
    Ok(rest[..end].to_vec())
}

fn read_map(bytes: &[u8], at: &mut usize, depth: usize) -> Result<Vec<(Vec<u8>, Value)>, String> {
    if depth > MAX_DEPTH {
        return Err("it nests too deep".into());
    }
    let mut map = Vec::new();
    loop {
        let kind = *bytes.get(*at).ok_or("the file ends early")?;
        *at += 1;
        if kind == 0x08 {
            return Ok(map);
        }
        let key = read_text(bytes, at)?;
        let value = match kind {
            0x00 => Value::Map(read_map(bytes, at, depth + 1)?),
            0x01 => Value::Text(read_text(bytes, at)?),
            0x02 => {
                let raw = bytes.get(*at..*at + 4).ok_or("the file ends early")?;
                *at += 4;
                Value::Int(u32::from_le_bytes([raw[0], raw[1], raw[2], raw[3]]))
            }
            other => return Err(format!("it holds an unknown value type {other:#04x}")),
        };
        map.push((key, value));
    }
}

fn parse(bytes: &[u8]) -> Result<Vec<(Vec<u8>, Value)>, String> {
    let mut at = 0;
    let map = read_map(bytes, &mut at, 0)?;
    if at != bytes.len() {
        return Err("it has more after its end".into());
    }
    Ok(map)
}

fn write_map(map: &[(Vec<u8>, Value)], out: &mut Vec<u8>) {
    for (key, value) in map {
        let kind = match value {
            Value::Map(_) => 0x00,
            Value::Text(_) => 0x01,
            Value::Int(_) => 0x02,
        };
        out.push(kind);
        out.extend_from_slice(key);
        out.push(0);
        match value {
            Value::Map(inner) => write_map(inner, out),
            Value::Text(text) => {
                out.extend_from_slice(text);
                out.push(0);
            }
            Value::Int(n) => out.extend_from_slice(&n.to_le_bytes()),
        }
    }
    out.push(0x08);
}

fn is_key(key: &[u8], name: &str) -> bool {
    key.eq_ignore_ascii_case(name.as_bytes())
}

fn get<'a>(map: &'a [(Vec<u8>, Value)], name: &str) -> Option<&'a Value> {
    map.iter().find(|(key, _)| is_key(key, name)).map(|(_, value)| value)
}

/// Sets a field, under the spelling the entry already has for it.
fn set(map: &mut Vec<(Vec<u8>, Value)>, name: &str, value: Value) {
    match map.iter_mut().find(|(key, _)| is_key(key, name)) {
        Some(field) => field.1 = value,
        None => map.push((name.as_bytes().to_vec(), value)),
    }
}

fn crc32(bytes: &[u8]) -> u32 {
    let mut crc = u32::MAX;
    for byte in bytes {
        crc ^= u32::from(*byte);
        for _ in 0..8 {
            crc = if crc & 1 == 1 { (crc >> 1) ^ 0xEDB8_8320 } else { crc >> 1 };
        }
    }
    !crc
}

/// The id Steam gives a non-Steam game: it names the pictures too.
fn app_id(exe: &str, name: &str) -> u32 {
    crc32(format!("{exe}{name}").as_bytes()) | 0x8000_0000
}

fn quoted(path: &Path) -> String {
    format!("\"{}\"", path.display())
}

fn start_dir(program: &Path) -> PathBuf {
    program.parent().map(Path::to_path_buf).unwrap_or_default()
}

/// The Steam folders to look in on this computer, best first. `registry` is Windows' own record.
pub fn roots(home: Option<&Path>, registry: Option<PathBuf>) -> Vec<PathBuf> {
    let mut roots: Vec<PathBuf> = registry.into_iter().collect();
    if cfg!(windows) {
        if let Some(programs) = std::env::var_os("ProgramFiles(x86)") {
            roots.push(PathBuf::from(programs).join("Steam"));
        }
    } else if let Some(home) = home {
        if cfg!(target_os = "macos") {
            roots.push(home.join("Library/Application Support/Steam"));
        } else {
            roots.push(home.join(".steam/steam"));
            roots.push(home.join(".local/share/Steam"));
            roots.push(home.join(".var/app/com.valvesoftware.Steam/.local/share/Steam"));
        }
    }
    roots
}

/// The account `loginusers.vdf` (Steam's text form) last signed in with, as a `userdata` folder name.
fn most_recent(text: &str) -> Option<String> {
    let mut current = None;
    for line in text.lines() {
        let parts: Vec<&str> = line.split('"').collect();
        match parts.as_slice() {
            [_, key, _] if key.len() == 17 && key.bytes().all(|b| b.is_ascii_digit()) => {
                current = key.parse::<u64>().ok();
            }
            [_, key, _, "1", _] if key.eq_ignore_ascii_case("MostRecent") => {
                return current.and_then(|id| id.checked_sub(STEAM_ID_BASE)).map(|id| id.to_string());
            }
            _ => {}
        }
    }
    None
}

/// The folder of the account to add to: the last one signed in, the only one, or the one whose
/// settings Steam wrote last.
fn user_dir(root: &Path) -> Option<PathBuf> {
    let userdata = root.join("userdata");
    let recent = std::fs::read_to_string(root.join("config/loginusers.vdf")).ok().and_then(|t| most_recent(&t));
    if let Some(dir) = recent.map(|id| userdata.join(id)).filter(|dir| dir.is_dir()) {
        return Some(dir);
    }
    let mut users = std::fs::read_dir(&userdata)
        .ok()?
        .flatten()
        .filter(|e| e.path().is_dir())
        .filter(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            name != "0" && !name.is_empty() && name.bytes().all(|b| b.is_ascii_digit())
        })
        .map(|e| e.path())
        .collect::<Vec<_>>();
    if users.len() == 1 {
        return users.pop();
    }
    // Several, and none marked last: the one whose settings Steam wrote last.
    let written = |dir: &PathBuf| std::fs::metadata(dir.join("config/localconfig.vdf")).and_then(|m| m.modified()).ok();
    users.sort_by_key(written);
    users.pop().filter(|dir| written(dir).is_some())
}

/// What the file says about an account's entry, for `program`.
fn state_of(user: &Path, program: &Path, target: &Target) -> State {
    let Ok(bytes) = std::fs::read(user.join("config/shortcuts.vdf")) else { return State::NotAdded };
    let Ok(file) = parse(&bytes) else { return State::NotAdded };
    let Some(Value::Map(entries)) = get(&file, "shortcuts") else { return State::NotAdded };
    let Some((_, Value::Map(fields))) = entries.iter().find(|(_, entry)| target.matches(entry)) else {
        return State::NotAdded;
    };
    // An entry that turned the Desktop Layout off in the launcher's window is updated too.
    let desktop = matches!(get(fields, "AllowDesktopConfig"), Some(Value::Int(1)));
    // And one that should have its pictures and has no cover (added before it had them).
    let pictured = !target.pictures
        || matches!(get(fields, "appid"), Some(Value::Int(id)) if has_cover(&user.join("config/grid"), *id));
    match get(fields, "Exe") {
        Some(Value::Text(exe)) if exe == quoted(program).as_bytes() && desktop && pictured => State::Added,
        _ => State::Outdated,
    }
}

/// Whether Steam has a cover for the entry `id`: ours, or one the player chose, in any format.
fn has_cover(grid: &Path, id: u32) -> bool {
    ["png", "jpg", "jpeg", "webp"].iter().any(|ext| grid.join(format!("{id}p.{ext}")).exists())
}

/// The first Steam of `roots` that is there.
fn found(roots: &[PathBuf]) -> Option<&PathBuf> {
    roots.iter().find(|root| root.join("userdata").is_dir() || root.join("config").is_dir())
}

/// Whether a Steam folder is Flathub's, through a link too (`~/.steam/steam` may point there).
fn is_flatpak(root: &Path) -> bool {
    let real = root.canonicalize().unwrap_or_else(|_| root.to_path_buf());
    real.components().any(|part| part.as_os_str() == "com.valvesoftware.Steam")
}

/// Where the launcher at `program` stands in the first Steam of `roots` that is there.
pub fn state(roots: &[PathBuf], program: &Path) -> State {
    state_for(roots, program, &Target::launcher())
}

/// Where the entry `target`, started through `program`, stands in the first Steam of `roots`.
pub fn state_for(roots: &[PathBuf], program: &Path, target: &Target) -> State {
    let Some(root) = found(roots) else {
        return State::NoSteam;
    };
    if is_flatpak(root) {
        return State::FlatpakSteam;
    }
    match user_dir(root) {
        Some(user) => state_of(&user, program, target),
        None => State::NoUser,
    }
}

/// `shortcuts.vdf` with the entry for `program` added, or updated where it is. And the entry's id.
/// `icon` is the path to the icon, given the id.
fn with_entry(
    bytes: Option<&[u8]>,
    program: &Path,
    target: &Target,
    icon: impl Fn(u32) -> PathBuf,
) -> Result<(Vec<u8>, u32), String> {
    let mut file = match bytes {
        Some(bytes) => parse(bytes).map_err(|e| format!("Steam's list of shortcuts can't be read: {e}."))?,
        None => Vec::new(),
    };
    if get(&file, "shortcuts").is_none() {
        file.push((b"shortcuts".to_vec(), Value::Map(Vec::new())));
    }
    let Some((_, Value::Map(entries))) = file.iter_mut().find(|(key, _)| is_key(key, "shortcuts")) else {
        return Err("Steam's list of shortcuts can't be read: it has no list.".into());
    };

    if !entries.iter().any(|(_, entry)| target.matches(entry)) {
        let next = entries
            .iter()
            .filter_map(|(key, _)| std::str::from_utf8(key).ok()?.parse::<u64>().ok())
            .max()
            .map_or(0, |n| n + 1);
        // What Steam writes for a new entry: it adds any that are missing, and drops none.
        let text = |text: &str| Value::Text(text.as_bytes().to_vec());
        let fields = vec![
            (b"AppName".to_vec(), text(&target.name)),
            (b"ShortcutPath".to_vec(), text("")),
            (b"LaunchOptions".to_vec(), text(&target.options)),
            (b"IsHidden".to_vec(), Value::Int(0)),
            (b"AllowDesktopConfig".to_vec(), Value::Int(1)),
            (b"AllowOverlay".to_vec(), Value::Int(1)),
            (b"OpenVR".to_vec(), Value::Int(0)),
            (b"Devkit".to_vec(), Value::Int(0)),
            (b"DevkitGameID".to_vec(), text("")),
            (b"DevkitOverrideAppID".to_vec(), Value::Int(0)),
            (b"LastPlayTime".to_vec(), Value::Int(0)),
            (b"FlatpakAppID".to_vec(), text("")),
            (b"tags".to_vec(), Value::Map(Vec::new())),
        ];
        entries.push((next.to_string().into_bytes(), Value::Map(fields)));
    }
    let Some((_, Value::Map(fields))) = entries.iter_mut().find(|(_, entry)| target.matches(entry)) else {
        return Err("Steam's list of shortcuts can't be read: the new entry is not in it.".into());
    };

    let exe = quoted(program);
    // An entry keeps its id across a moved launcher: the player's collections hang on it.
    let id = match get(fields, "appid") {
        Some(Value::Int(id)) => *id,
        _ => app_id(&exe, &target.name),
    };
    let text = |path: String| Value::Text(path.into_bytes());
    set(fields, "appid", Value::Int(id));
    set(fields, "Exe", text(exe));
    set(fields, "StartDir", text(quoted(&start_dir(program))));
    set(fields, "icon", text(icon(id).display().to_string()));
    // The launcher's window keeps Steam's Desktop Layout (mouse and keys); only the game gets the
    // game configuration.
    set(fields, "AllowDesktopConfig", Value::Int(1));

    let mut out = Vec::new();
    write_map(&file, &mut out);
    Ok((out, id))
}

/// How many dated copies of `shortcuts.vdf` are kept; older ones go.
const BACKUPS: usize = 3;

/// The size and change time of a file, to tell whether someone wrote it since it was read.
fn mark(path: &Path) -> Option<(u64, std::time::SystemTime)> {
    let meta = std::fs::metadata(path).ok()?;
    Some((meta.len(), meta.modified().ok()?))
}

/// Whether a list about to replace Steam's reads back, with the entry in it.
fn valid(bytes: &[u8], target: &Target) -> bool {
    let Ok(file) = parse(bytes) else { return false };
    matches!(get(&file, "shortcuts"), Some(Value::Map(entries)) if entries.iter().any(|(_, e)| target.matches(e)))
}

/// Keeps a dated copy of the list and drops the oldest past `BACKUPS`.
fn back_up(config: &Path, old: &[u8]) -> std::io::Result<()> {
    let stamp = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default().as_nanos();
    std::fs::write(config.join(format!("shortcuts.vdf.openrac-bak-{stamp:024}")), old)?;
    let mut copies: Vec<PathBuf> = std::fs::read_dir(config)?
        .flatten()
        .map(|e| e.path())
        .filter(|p| p.file_name().is_some_and(|n| n.to_string_lossy().starts_with("shortcuts.vdf.openrac-bak-")))
        .collect();
    copies.sort();
    let extra = copies.len().saturating_sub(BACKUPS);
    for stale in &copies[..extra] {
        let _ = std::fs::remove_file(stale);
    }
    Ok(())
}

/// Writes `bytes` next to `list` and moves it over, so Steam never sees half a file.
fn replace(list: &Path, bytes: &[u8]) -> std::io::Result<()> {
    let partial = list.with_extension("vdf.openrac-new");
    let done = std::fs::write(&partial, bytes).and_then(|()| std::fs::rename(&partial, list));
    if done.is_err() {
        let _ = std::fs::remove_file(&partial);
    }
    done
}

/// Reads the list, or nothing when there is none yet.
fn read_list(list: &Path) -> std::io::Result<Option<Vec<u8>>> {
    match std::fs::read(list) {
        Ok(bytes) => Ok(Some(bytes)),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(e) => Err(e),
    }
}

/// The pictures' file names for an id. Steam names them by the id as an unsigned number.
fn art_names(id: u32) -> [(String, &'static str); 5] {
    [
        (format!("{id}p"), "jpg"),
        (format!("{id}"), "jpg"),
        (format!("{id}_hero"), "jpg"),
        (format!("{id}_logo"), "png"),
        (format!("{id}_icon"), "png"),
    ]
}

/// Puts each picture Steam does not have yet. A picture the player chose, in any format, stays.
fn write_art(grid: &Path, id: u32, art: &Art) -> std::io::Result<()> {
    std::fs::create_dir_all(grid)?;
    let bytes = [art.cover, art.wide, art.hero, art.logo, art.icon];
    for ((stem, ext), bytes) in art_names(id).into_iter().zip(bytes) {
        if bytes.is_empty() || ["png", "jpg", "jpeg", "webp"].iter().any(|e| grid.join(format!("{stem}.{e}")).exists())
        {
            continue;
        }
        let target = grid.join(format!("{stem}.{ext}"));
        let partial = grid.join(format!("{stem}.{ext}.openrac-new"));
        let done = std::fs::write(&partial, bytes).and_then(|()| std::fs::rename(&partial, &target));
        if done.is_err() {
            let _ = std::fs::remove_file(&partial);
        }
        done?;
    }
    Ok(())
}

/// Adds the launcher at `program` to the account folder `user`, or updates its entry.
///
/// Steam may be open: it reads the list again when it changes (seen in DualTouch, which adds its
/// shortcut this way), and it may write its own copy at any moment. So the list is read, changed
/// and checked, and if Steam wrote it in between, read and changed again on top of Steam's copy.
/// `before_write` runs between the read and the write; the tests use it to play Steam.
fn add_to(user: &Path, program: &Path, target: &Target, art: &Art, before_write: impl Fn(&Path)) -> Result<(), String> {
    let failed = |e: std::io::Error| format!("Steam's folder can't be written: {e}.");
    let config = user.join("config");
    let grid = config.join("grid");
    let list = config.join("shortcuts.vdf");
    let icon = |id: u32| grid.join(format!("{id}_icon.png"));

    let mut old = read_list(&list).map_err(failed)?;
    let mut seen = mark(&list);
    let (mut new, mut id) = with_entry(old.as_deref(), program, target, icon)?;
    before_write(&list);
    if mark(&list) != seen {
        old = read_list(&list).map_err(failed)?;
        seen = mark(&list);
        (new, id) = with_entry(old.as_deref(), program, target, icon)?;
    }
    if !valid(&new, target) {
        return Err("Steam's list of shortcuts can't be written: the new list does not read back.".into());
    }
    std::fs::create_dir_all(&config).map_err(failed)?;
    if let Some(old) = &old {
        back_up(&config, old).map_err(failed)?;
    }
    if mark(&list) != seen {
        return Err("Steam changed its list of shortcuts just now. Try again.".into());
    }
    replace(&list, &new).map_err(failed)?;
    write_art(&grid, id, art).map_err(failed)
}

/// Adds the launcher to the first Steam of `roots`, or updates its entry. What it stands at now.
/// Steam may be open; if it does not show the entry, it shows it after a restart.
pub fn add(roots: &[PathBuf], program: &Path, art: &Art) -> Result<State, String> {
    add_for(roots, program, &Target::launcher(), art)
}

/// The same for the entry `target`: a game's own, which starts `program` with `--play`.
pub fn add_for(roots: &[PathBuf], program: &Path, target: &Target, art: &Art) -> Result<State, String> {
    let Some(root) = found(roots) else {
        return Err("Steam isn't installed here.".into());
    };
    if is_flatpak(root) {
        return Err(FLATPAK_STEAM.into());
    }
    let user = user_dir(root).ok_or("Sign in to Steam once, then try again.")?;
    add_to(&user, program, target, art, |_| ())?;
    Ok(state_of(&user, program, target))
}

#[cfg(test)]
mod tests {
    use super::*;

    const ART: Art = Art { cover: b"cover", wide: b"wide", hero: b"hero", logo: b"logo", icon: b"icon" };

    /// A list as Steam writes it: another game, with fields and a value this file does not name.
    fn steams_list() -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(b"\x00shortcuts\x00\x00" as &[u8]);
        out.extend_from_slice(b"7\x00");
        out.extend_from_slice(b"\x02appid\x00\x01\x02\x03\x84");
        out.extend_from_slice(b"\x01appname\x00Other game \xff\x00");
        out.extend_from_slice(b"\x01Exe\x00\"/games/other\"\x00");
        out.extend_from_slice(b"\x02Unknown\x00\x09\x00\x00\x00");
        out.extend_from_slice(b"\x00tags\x00\x010\x00Favourite\x00\x08");
        out.extend_from_slice(b"\x08\x08\x08");
        out
    }

    fn steam(dir: &Path) -> PathBuf {
        let user = dir.join("userdata/42");
        std::fs::create_dir_all(user.join("config")).unwrap();
        user
    }

    fn backups(user: &Path) -> Vec<Vec<u8>> {
        let mut names: Vec<PathBuf> = std::fs::read_dir(user.join("config"))
            .unwrap()
            .flatten()
            .map(|e| e.path())
            .filter(|p| p.to_string_lossy().contains("openrac-bak-"))
            .collect();
        names.sort();
        names.iter().map(|p| std::fs::read(p).unwrap()).collect()
    }

    fn entries_of(list: &Path) -> Vec<(Vec<u8>, Value)> {
        let file = parse(&std::fs::read(list).unwrap()).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        entries.clone()
    }

    #[test]
    fn steam_writing_the_list_mid_add_is_merged_not_lost() {
        let dir = tempfile::tempdir().unwrap();
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        // Steam's copy, written between the read and the write: its own new game too.
        let mut theirs = steams_list();
        theirs.truncate(theirs.len() - 2);
        theirs.extend_from_slice(b"\x009\x00\x01AppName\x00New one\x00\x08\x08\x08");
        assert!(parse(&theirs).is_ok());
        let program = Path::new("/opt/openrac");
        add_to(&user, program, &Target::launcher(), &ART, |list| std::fs::write(list, &theirs).unwrap()).unwrap();
        let entries = entries_of(&list);
        assert_eq!(entries.len(), 3);
        assert_eq!(entries[1].0, b"9");
        assert_eq!(entries[2].0, b"10");
        assert!(Target::launcher().matches(&entries[2].1));
        assert_eq!(backups(&user), vec![theirs]);
    }

    #[test]
    fn backups_are_kept_to_three() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        std::fs::write(user.join("config/shortcuts.vdf"), steams_list()).unwrap();
        for n in 0..6 {
            add(&roots, Path::new(&format!("/opt/{n}/launcher")), &ART).unwrap();
        }
        assert_eq!(backups(&user).len(), BACKUPS);
    }

    #[test]
    fn adding_twice_changes_nothing_but_a_backup() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        add(&roots, Path::new("/opt/openrac"), &ART).unwrap();
        let first = std::fs::read(&list).unwrap();
        assert_eq!(add(&roots, Path::new("/opt/openrac"), &ART), Ok(State::Added));
        assert_eq!(std::fs::read(&list).unwrap(), first);
    }

    #[test]
    fn the_players_own_pictures_stay() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let id = app_id("\"/opt/openrac\"", NAME);
        let grid = user.join("config/grid");
        std::fs::create_dir_all(&grid).unwrap();
        std::fs::write(grid.join(format!("{id}p.png")), b"mine").unwrap();
        std::fs::write(grid.join(format!("{id}_hero.jpg")), b"mine too").unwrap();
        add(&roots, Path::new("/opt/openrac"), &ART).unwrap();
        assert!(!grid.join(format!("{id}p.jpg")).exists());
        assert_eq!(std::fs::read(grid.join(format!("{id}_hero.jpg"))).unwrap(), b"mine too");
        assert_eq!(std::fs::read(grid.join(format!("{id}.jpg"))).unwrap(), b"wide");
        assert!(std::fs::read_dir(&grid)
            .unwrap()
            .flatten()
            .all(|e| !e.path().to_string_lossy().ends_with("openrac-new")));
    }

    #[test]
    fn an_id_steam_gave_is_kept_and_names_the_pictures_unsigned() {
        // Steam's own ids have the top bit set; read as signed they are negative.
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let mut list = Vec::new();
        list.extend_from_slice(b"\x00shortcuts\x00\x000\x00\x02appid\x00\xfe\xff\xff\xff");
        list.extend_from_slice(b"\x01AppName\x00OpenRAC\x00\x01Exe\x00\"/opt/openrac\"\x00\x08\x08\x08");
        std::fs::write(user.join("config/shortcuts.vdf"), &list).unwrap();
        assert_eq!(add(&roots, Path::new("/opt/openrac"), &ART), Ok(State::Added));
        assert!(user.join("config/grid/4294967294_icon.png").exists());
        let entries = entries_of(&user.join("config/shortcuts.vdf"));
        assert_eq!(entries.len(), 1);
        let Value::Map(fields) = &entries[0].1 else { panic!("not a map") };
        assert_eq!(get(fields, "appid"), Some(&Value::Int(0xFFFF_FFFE)));
    }

    #[test]
    fn a_second_entry_of_ours_is_left_and_the_first_updated() {
        let mut list = Vec::new();
        list.extend_from_slice(b"\x00shortcuts\x00");
        for key in [&b"0"[..], b"1"] {
            list.push(0);
            list.extend_from_slice(key);
            list.extend_from_slice(b"\x00\x01AppName\x00OpenRAC\x00\x01Exe\x00\"/old\"\x00\x08");
        }
        list.extend_from_slice(b"\x08\x08");
        let (out, _) = with_entry(Some(&list), Path::new("/new"), &Target::launcher(), |_| PathBuf::new()).unwrap();
        let file = parse(&out).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        assert_eq!(entries.len(), 2);
        let exe = |e: &Value| match e {
            Value::Map(f) => get(f, "Exe").cloned(),
            _ => None,
        };
        assert_eq!(exe(&entries[0].1), Some(Value::Text(b"\"/new\"".to_vec())));
        assert_eq!(exe(&entries[1].1), Some(Value::Text(b"\"/old\"".to_vec())));
    }

    #[test]
    fn paths_with_spaces_unicode_and_backslashes_are_kept_as_written() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        for program in ["/home/zółw/My Games/openrac launcher", r"C:\Program Files\OpenRAC\launcher.exe"] {
            let program = Path::new(program);
            assert_eq!(add(&roots, program, &ART), Ok(State::Added));
            let entries = entries_of(&user.join("config/shortcuts.vdf"));
            let Value::Map(fields) = &entries[0].1 else { panic!("not a map") };
            assert_eq!(get(fields, "Exe"), Some(&Value::Text(quoted(program).into_bytes())));
        }
    }

    #[test]
    fn truncated_empty_and_deep_lists_are_refused() {
        let full = steams_list();
        for cut in 1..full.len() {
            assert!(parse(&full[..cut]).is_err(), "cut at {cut}");
        }
        assert!(parse(b"").is_err());
        let mut deep = Vec::new();
        for _ in 0..100 {
            deep.extend_from_slice(b"\x00a\x00");
        }
        deep.extend(std::iter::repeat_n(0x08, 101));
        assert!(parse(&deep).is_err());
    }

    #[test]
    fn an_empty_file_is_refused_and_kept() {
        let dir = tempfile::tempdir().unwrap();
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, b"").unwrap();
        assert!(add(&[dir.path().to_path_buf()], Path::new("/a"), &ART).is_err());
        assert_eq!(std::fs::read(&list).unwrap(), b"");
    }

    #[test]
    fn several_users_without_a_last_go_by_settings_written_last() {
        let dir = tempfile::tempdir().unwrap();
        for (id, age) in [("41", 100), ("42", 10)] {
            let config = dir.path().join(format!("userdata/{id}/config"));
            std::fs::create_dir_all(&config).unwrap();
            let f = std::fs::File::create(config.join("localconfig.vdf")).unwrap();
            f.set_modified(std::time::SystemTime::now() - std::time::Duration::from_secs(age)).unwrap();
        }
        assert_eq!(user_dir(dir.path()), Some(dir.path().join("userdata/42")));
    }

    #[test]
    fn the_first_steam_that_is_there_is_used() {
        let dir = tempfile::tempdir().unwrap();
        let second = dir.path().join("b");
        let user = steam(&second);
        let roots = [dir.path().join("a"), second];
        assert_eq!(add(&roots, Path::new("/x"), &ART), Ok(State::Added));
        assert!(user.join("config/shortcuts.vdf").exists());
    }

    #[cfg(unix)]
    #[test]
    fn a_folder_it_cannot_write_leaves_the_list_whole() {
        use std::os::unix::fs::PermissionsExt;
        let dir = tempfile::tempdir().unwrap();
        let user = steam(dir.path());
        let config = user.join("config");
        let list = config.join("shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        std::fs::set_permissions(&config, std::fs::Permissions::from_mode(0o555)).unwrap();
        if std::fs::write(config.join("probe"), b"").is_ok() {
            return; // running as root: nothing is read-only
        }
        assert!(add(&[dir.path().to_path_buf()], Path::new("/a"), &ART).is_err());
        std::fs::set_permissions(&config, std::fs::Permissions::from_mode(0o755)).unwrap();
        assert_eq!(std::fs::read(&list).unwrap(), steams_list());
        assert_eq!(std::fs::read_dir(&config).unwrap().count(), 1);
    }

    #[test]
    fn a_list_written_back_is_the_same_bytes() {
        let bytes = steams_list();
        let mut out = Vec::new();
        write_map(&parse(&bytes).unwrap(), &mut out);
        assert_eq!(out, bytes);
    }

    #[test]
    fn a_list_it_cannot_read_is_refused_and_kept() {
        for bad in [&b"nonsense"[..], b"\x00shortcuts\x00", b"\x07big\x00\x00\x00\x00\x00\x08", b"\x08more"] {
            assert!(parse(bad).is_err(), "{bad:?}");
        }
        let dir = tempfile::tempdir().unwrap();
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, b"nonsense").unwrap();
        let err = add(&[dir.path().to_path_buf()], Path::new("/opt/openrac"), &ART).unwrap_err();
        assert!(err.contains("can't be read"), "{err}");
        assert_eq!(std::fs::read(&list).unwrap(), b"nonsense");
        assert!(!user.join("config/grid").exists());
    }

    #[test]
    fn the_id_is_steams_checksum_with_the_top_bit_set() {
        assert_eq!(crc32(b"123456789"), 0xCBF4_3926);
        assert_eq!(app_id("1234", "56789"), 0xCBF4_3926);
        assert_eq!(app_id("a", "") & 0x8000_0000, 0x8000_0000);
    }

    #[test]
    fn adding_keeps_the_rest_and_writes_the_pictures() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        let program = Path::new("/opt/openrac/launcher");
        assert_eq!(state(&roots, program), State::NotAdded);

        assert_eq!(add(&roots, program, &ART), Ok(State::Added));

        let file = parse(&std::fs::read(&list).unwrap()).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        assert_eq!(entries.len(), 2);
        let before = parse(&steams_list()).unwrap();
        let Some(Value::Map(old)) = get(&before, "shortcuts") else { panic!("no list") };
        assert_eq!(entries[0], old[0], "the other game is untouched");
        assert_eq!(entries[1].0, b"8", "numbered after the highest");
        let Value::Map(fields) = &entries[1].1 else { panic!("not a map") };
        let id = app_id("\"/opt/openrac/launcher\"", NAME);
        assert_eq!(get(fields, "appid"), Some(&Value::Int(id)));
        assert_eq!(get(fields, "Exe"), Some(&Value::Text(b"\"/opt/openrac/launcher\"".to_vec())));
        assert_eq!(get(fields, "StartDir"), Some(&Value::Text(b"\"/opt/openrac\"".to_vec())));

        let grid = user.join("config/grid");
        for (name, bytes) in [
            (format!("{id}p.jpg"), "cover"),
            (format!("{id}.jpg"), "wide"),
            (format!("{id}_hero.jpg"), "hero"),
            (format!("{id}_logo.png"), "logo"),
            (format!("{id}_icon.png"), "icon"),
        ] {
            assert_eq!(std::fs::read_to_string(grid.join(&name)).unwrap(), bytes, "{name}");
        }
        assert_eq!(backups(&user)[0], steams_list());
    }

    #[test]
    fn the_launcher_keeps_the_desktop_layout() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        let program = Path::new("/opt/openrac");
        assert_eq!(add(&roots, program, &ART), Ok(State::Added));
        let ours = |list: &Path| {
            let entries = entries_of(list);
            let Some((_, Value::Map(fields))) = entries.into_iter().find(|(_, e)| Target::launcher().matches(e)) else {
                panic!("no entry")
            };
            fields
        };
        assert_eq!(get(&ours(&list), "AllowDesktopConfig"), Some(&Value::Int(1)));

        // An entry an older launcher wrote, with the Desktop Layout turned off: shown as to update,
        // and updated in place, every other field kept.
        let mut file = parse(&std::fs::read(&list).unwrap()).unwrap();
        let Some((_, Value::Map(entries))) = file.iter_mut().find(|(k, _)| is_key(k, "shortcuts")) else {
            panic!("no list")
        };
        let Some((_, Value::Map(fields))) = entries.iter_mut().find(|(_, e)| Target::launcher().matches(e)) else {
            panic!("no entry")
        };
        set(fields, "AllowDesktopConfig", Value::Int(0));
        set(fields, "LaunchOptions", Value::Text(b"--keep".to_vec()));
        let mut bytes = Vec::new();
        write_map(&file, &mut bytes);
        std::fs::write(&list, &bytes).unwrap();
        assert_eq!(state(&roots, program), State::Outdated);
        assert_eq!(add(&roots, program, &ART), Ok(State::Added));
        let fields = ours(&list);
        assert_eq!(get(&fields, "AllowDesktopConfig"), Some(&Value::Int(1)));
        assert_eq!(get(&fields, "LaunchOptions"), Some(&Value::Text(b"--keep".to_vec())));
        assert_eq!(entries_of(&list).len(), 2);
    }

    #[test]
    fn adding_again_updates_the_entry_and_keeps_its_id_and_the_first_copy() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        add(&roots, Path::new("/old/launcher"), &ART).unwrap();
        assert_eq!(state(&roots, Path::new("/new/launcher")), State::Outdated);

        assert_eq!(add(&roots, Path::new("/new/launcher"), &ART), Ok(State::Added));

        let file = parse(&std::fs::read(&list).unwrap()).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        assert_eq!(entries.len(), 2);
        let Value::Map(fields) = &entries[1].1 else { panic!("not a map") };
        assert_eq!(get(fields, "appid"), Some(&Value::Int(app_id("\"/old/launcher\"", NAME))));
        assert_eq!(get(fields, "Exe"), Some(&Value::Text(b"\"/new/launcher\"".to_vec())));
        assert_eq!(backups(&user)[0], steams_list());
    }

    #[test]
    fn a_first_shortcut_makes_the_list() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        assert_eq!(add(&roots, Path::new("/opt/openrac"), &ART), Ok(State::Added));
        assert!(backups(&user).is_empty());
        assert!(!user.join("config/shortcuts.vdf.openrac-new").exists());
    }

    #[test]
    fn the_account_is_the_last_signed_in_or_the_only_one() {
        let text = "\"users\"\n{\n\t\"76561197960265770\"\n\t{\n\t\t\"MostRecent\"\t\t\"0\"\n\t}\n\t\"76561197960265771\"\n\t{\n\t\t\"mostrecent\"\t\t\"1\"\n\t}\n}\n";
        assert_eq!(most_recent(text).as_deref(), Some("43"));
        assert_eq!(most_recent("\"users\"\n{\n}\n"), None);

        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        std::fs::create_dir_all(dir.path().join("userdata/0")).unwrap();
        assert_eq!(state(&roots, Path::new("/a")), State::NoUser, "0 is no account");
        let only = dir.path().join("userdata/42");
        std::fs::create_dir_all(&only).unwrap();
        assert_eq!(user_dir(dir.path()), Some(only));
        let recent = dir.path().join("userdata/43");
        std::fs::create_dir_all(&recent).unwrap();
        assert_eq!(state(&roots, Path::new("/a")), State::NoUser, "two, and none is the last");
        std::fs::create_dir_all(dir.path().join("config")).unwrap();
        std::fs::write(dir.path().join("config/loginusers.vdf"), text).unwrap();
        assert_eq!(user_dir(dir.path()), Some(recent));
    }

    #[test]
    fn flatpak_steam_is_told_apart_and_nothing_is_written() {
        let dir = tempfile::tempdir().unwrap();
        let native = dir.path().join("native");
        let flatpak = dir.path().join(".var/app/com.valvesoftware.Steam/.local/share/Steam");
        let user = steam(&flatpak);
        let program = Path::new("/opt/openrac");

        let only = [native.clone(), flatpak.clone()];
        assert_eq!(state(&only, program), State::FlatpakSteam);
        assert_eq!(add(&only, program, &ART), Err(FLATPAK_STEAM.to_string()));
        assert_eq!(std::fs::read_dir(user.join("config")).unwrap().count(), 0);

        steam(&native);
        assert_eq!(state(&only, program), State::NotAdded, "the normal Steam comes first");
        assert_eq!(add(&only, program, &ART), Ok(State::Added));
        assert_eq!(std::fs::read_dir(user.join("config")).unwrap().count(), 0);
    }

    #[cfg(unix)]
    #[test]
    fn a_link_to_flatpak_steam_is_flatpak_steam() {
        let dir = tempfile::tempdir().unwrap();
        let flatpak = dir.path().join(".var/app/com.valvesoftware.Steam/.local/share/Steam");
        steam(&flatpak);
        let link = dir.path().join("steam");
        std::os::unix::fs::symlink(&flatpak, &link).unwrap();
        assert_eq!(state(&[link], Path::new("/opt/openrac")), State::FlatpakSteam);
    }

    #[test]
    fn no_steam_folder_is_no_steam() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().join("nowhere")];
        assert_eq!(state(&roots, Path::new("/a")), State::NoSteam);
        assert!(add(&roots, Path::new("/a"), &ART).is_err());
    }

    const NO_ART: Art = Art { cover: b"", wide: b"", hero: b"", logo: b"", icon: b"icon" };

    #[test]
    fn a_game_gets_its_own_entry_beside_the_launchers() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        let program = Path::new("/opt/OpenRAC.AppImage");
        let game = Target::game("Ratchet & Clank (PAL)", "rac1-pal");
        let other = Target::game("Ratchet & Clank (NTSC)", "rac1-ntsc");

        assert_eq!(add_for(&roots, program, &game, &NO_ART), Ok(State::Added));
        assert_eq!(state(&roots, program), State::NotAdded, "a game's entry is not the launcher's");
        assert_eq!(state_for(&roots, program, &other), State::NotAdded);
        assert_eq!(add(&roots, program, &ART), Ok(State::Added));
        assert_eq!(add_for(&roots, program, &other, &NO_ART), Ok(State::Added));

        let entries = entries_of(&list);
        assert_eq!(entries.len(), 4, "the other game's entry stayed");
        let fields = |n: usize| match &entries[n].1 {
            Value::Map(f) => f.clone(),
            _ => panic!("not a map"),
        };
        assert_eq!(get(&fields(1), "AppName"), Some(&Value::Text(b"Ratchet & Clank (PAL)".to_vec())));
        assert_eq!(get(&fields(1), "LaunchOptions"), Some(&Value::Text(b"--play rac1-pal".to_vec())));
        assert_eq!(get(&fields(1), "Exe"), Some(&Value::Text(b"\"/opt/OpenRAC.AppImage\"".to_vec())));
        assert_eq!(get(&fields(2), "LaunchOptions"), Some(&Value::Text(Vec::new())));
        assert_ne!(get(&fields(1), "appid"), get(&fields(2), "appid"));
        // Adding again changes no entry's count.
        assert_eq!(add_for(&roots, program, &game, &NO_ART), Ok(State::Added));
        assert_eq!(entries_of(&list).len(), 4);
    }

    #[test]
    fn a_launcher_entry_with_the_players_own_options_is_still_found() {
        let mut list = Vec::new();
        list.extend_from_slice(
            b"\x00shortcuts\x00\x000\x00\x01AppName\x00OpenRAC\x00\x01LaunchOptions\x00-x\x00\x08\x08\x08",
        );
        let (out, _) = with_entry(Some(&list), Path::new("/p"), &Target::launcher(), |_| PathBuf::new()).unwrap();
        let file = parse(&out).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        assert_eq!(entries.len(), 1);
    }

    #[test]
    fn a_game_added_without_its_pictures_is_to_update_and_gets_them() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let program = Path::new("/opt/OpenRAC.AppImage");
        let before = Target::game("Ratchet & Clank (PAL)", "rac1-pal");
        let now = before.clone().with_pictures();

        // A launcher from before the games had pictures: the icon alone.
        assert_eq!(add_for(&roots, program, &before, &NO_ART), Ok(State::Added));
        assert_eq!(state_for(&roots, program, &now), State::Outdated);
        assert_eq!(add_for(&roots, program, &now, &ART), Ok(State::Added));
        let entries = entries_of(&user.join("config/shortcuts.vdf"));
        assert_eq!(entries.len(), 1, "updated in place");
        let Value::Map(fields) = &entries[0].1 else { panic!("not a map") };
        let Some(Value::Int(id)) = get(fields, "appid") else { panic!("no id") };
        let grid = user.join("config/grid");
        for (name, bytes) in [(format!("{id}p.jpg"), "cover"), (format!("{id}_hero.jpg"), "hero")] {
            assert_eq!(std::fs::read_to_string(grid.join(&name)).unwrap(), bytes, "{name}");
        }
        assert_eq!(std::fs::read_to_string(grid.join(format!("{id}_icon.png"))).unwrap(), "icon", "the first kept");

        // A cover the player chose counts as one.
        std::fs::remove_file(grid.join(format!("{id}p.jpg"))).unwrap();
        assert_eq!(state_for(&roots, program, &now), State::Outdated);
        std::fs::write(grid.join(format!("{id}p.png")), b"mine").unwrap();
        assert_eq!(state_for(&roots, program, &now), State::Added);
        // The launcher's own entry asks for no pictures to be Added.
        assert_eq!(add(&roots, program, &NO_ART), Ok(State::Added));
    }

    #[test]
    fn a_game_art_is_only_what_exists() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        add_for(&roots, Path::new("/p"), &Target::game("G", "g-pal"), &NO_ART).unwrap();
        let grid: Vec<String> = std::fs::read_dir(user.join("config/grid"))
            .unwrap()
            .flatten()
            .map(|e| e.file_name().to_string_lossy().to_string())
            .collect();
        assert_eq!(grid.len(), 1, "{grid:?}");
        assert!(grid[0].ends_with("_icon.png"));
    }
}
