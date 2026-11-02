//! Adds the launcher to the player's Steam library as a non-Steam game: an entry in
//! `userdata/<id>/config/shortcuts.vdf` and its pictures in `config/grid/`.
//!
//! Steam reads that file when it starts and may write its own copy over it, so nothing is written
//! while Steam runs. Every other entry and every field this file does not know stays as it was: a
//! file that cannot be read is an error, never overwritten, and the first write keeps a copy.
//!
//! The work takes its folders as arguments, so the tests never touch a real Steam. It is compiled
//! and tested on every platform; Android does not call it.

use std::path::{Path, PathBuf};

use serde::Serialize;

use crate::detect;

/// The name the entry carries, and how it is found again.
pub const NAME: &str = "OpenRAC";
/// What `add` says when Steam runs.
pub const STEAM_OPEN: &str = "Close Steam, then try again.";
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

/// The entry's pictures, as Steam's library shows them.
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

fn is_ours(entry: &Value) -> bool {
    let Value::Map(fields) = entry else { return false };
    matches!(get(fields, "AppName"), Some(Value::Text(name)) if name == NAME.as_bytes())
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

/// The folder of the account to add to: the last one signed in, or the only one.
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
        .map(|e| e.path());
    let first = users.next()?;
    users.next().is_none().then_some(first)
}

/// What the file says about an account's entry, for `program`.
fn state_of(user: &Path, program: &Path) -> State {
    let Ok(bytes) = std::fs::read(user.join("config/shortcuts.vdf")) else { return State::NotAdded };
    let Ok(file) = parse(&bytes) else { return State::NotAdded };
    let Some(Value::Map(entries)) = get(&file, "shortcuts") else { return State::NotAdded };
    let Some((_, Value::Map(fields))) = entries.iter().find(|(_, entry)| is_ours(entry)) else {
        return State::NotAdded;
    };
    match get(fields, "Exe") {
        Some(Value::Text(exe)) if exe == quoted(program).as_bytes() => State::Added,
        _ => State::Outdated,
    }
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
    let Some(root) = found(roots) else {
        return State::NoSteam;
    };
    if is_flatpak(root) {
        return State::FlatpakSteam;
    }
    match user_dir(root) {
        Some(user) => state_of(&user, program),
        None => State::NoUser,
    }
}

/// `shortcuts.vdf` with the entry for `program` added, or updated where it is. And the entry's id.
/// `icon` is the path to the icon, given the id.
fn with_entry(bytes: Option<&[u8]>, program: &Path, icon: impl Fn(u32) -> PathBuf) -> Result<(Vec<u8>, u32), String> {
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

    if !entries.iter().any(|(_, entry)| is_ours(entry)) {
        let next = entries
            .iter()
            .filter_map(|(key, _)| std::str::from_utf8(key).ok()?.parse::<u64>().ok())
            .max()
            .map_or(0, |n| n + 1);
        // What Steam writes for a new entry: it adds any that are missing, and drops none.
        let text = |text: &str| Value::Text(text.as_bytes().to_vec());
        let fields = vec![
            (b"AppName".to_vec(), text(NAME)),
            (b"ShortcutPath".to_vec(), text("")),
            (b"LaunchOptions".to_vec(), text("")),
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
    let Some((_, Value::Map(fields))) = entries.iter_mut().find(|(_, entry)| is_ours(entry)) else {
        return Err("Steam's list of shortcuts can't be read: the new entry is not in it.".into());
    };

    let exe = quoted(program);
    // An entry keeps its id across a moved launcher: the player's collections hang on it.
    let id = match get(fields, "appid") {
        Some(Value::Int(id)) => *id,
        _ => app_id(&exe, NAME),
    };
    let text = |path: String| Value::Text(path.into_bytes());
    set(fields, "appid", Value::Int(id));
    set(fields, "Exe", text(exe));
    set(fields, "StartDir", text(quoted(&start_dir(program))));
    set(fields, "icon", text(icon(id).display().to_string()));

    let mut out = Vec::new();
    write_map(&file, &mut out);
    Ok((out, id))
}

/// Adds the launcher at `program` to the account folder `user`, or updates its entry.
fn add_to(user: &Path, program: &Path, art: &Art) -> Result<(), String> {
    let failed = |e: std::io::Error| format!("Steam's folder can't be written: {e}.");
    let config = user.join("config");
    let grid = config.join("grid");
    let list = config.join("shortcuts.vdf");
    let old = match std::fs::read(&list) {
        Ok(bytes) => Some(bytes),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => None,
        Err(e) => return Err(failed(e)),
    };
    let (new, id) = with_entry(old.as_deref(), program, |id| grid.join(format!("{id}_icon.png")))?;

    std::fs::create_dir_all(&grid).map_err(failed)?;
    for (name, bytes) in [
        (format!("{id}p.jpg"), art.cover),
        (format!("{id}.jpg"), art.wide),
        (format!("{id}_hero.jpg"), art.hero),
        (format!("{id}_logo.png"), art.logo),
        (format!("{id}_icon.png"), art.icon),
    ] {
        std::fs::write(grid.join(name), bytes).map_err(failed)?;
    }

    let backup = config.join("shortcuts.vdf.openrac-bak");
    if let (Some(old), false) = (&old, backup.exists()) {
        std::fs::write(&backup, old).map_err(failed)?;
    }
    let partial = config.join("shortcuts.vdf.openrac-new");
    std::fs::write(&partial, new).map_err(failed)?;
    std::fs::rename(&partial, &list).map_err(failed)
}

/// Adds the launcher to the first Steam of `roots`. `running` says Steam is open: nothing is
/// written then. What it stands at now.
pub fn add(roots: &[PathBuf], program: &Path, art: &Art, running: bool) -> Result<State, String> {
    let Some(root) = found(roots) else {
        return Err("Steam isn't installed here.".into());
    };
    if is_flatpak(root) {
        return Err(FLATPAK_STEAM.into());
    }
    if running {
        return Err(STEAM_OPEN.into());
    }
    let user = user_dir(root).ok_or("Sign in to Steam once, then try again.")?;
    add_to(&user, program, art)?;
    Ok(state_of(&user, program))
}

/// Whether Steam runs on this computer.
pub fn running() -> bool {
    if cfg!(windows) {
        let found = detect::quiet(Path::new("tasklist"))
            .args(["/FI", "IMAGENAME eq steam.exe", "/NH"])
            .output()
            .map(|out| String::from_utf8_lossy(&out.stdout).to_ascii_lowercase());
        found.is_ok_and(|text| text.contains("steam.exe"))
    } else if cfg!(target_os = "macos") {
        detect::quiet(Path::new("pgrep")).args(["-x", "steam_osx"]).output().is_ok_and(|out| out.status.success())
    } else {
        let Ok(processes) = std::fs::read_dir("/proc") else { return false };
        processes.flatten().any(|process| {
            std::fs::read_to_string(process.path().join("comm")).is_ok_and(|name| name.trim_end() == "steam")
        })
    }
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
        let err = add(&[dir.path().to_path_buf()], Path::new("/opt/openrac"), &ART, false).unwrap_err();
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

        assert_eq!(add(&roots, program, &ART, false), Ok(State::Added));

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
        assert_eq!(std::fs::read(user.join("config/shortcuts.vdf.openrac-bak")).unwrap(), steams_list());
    }

    #[test]
    fn adding_again_updates_the_entry_and_keeps_its_id_and_the_first_copy() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        let list = user.join("config/shortcuts.vdf");
        std::fs::write(&list, steams_list()).unwrap();
        add(&roots, Path::new("/old/launcher"), &ART, false).unwrap();
        assert_eq!(state(&roots, Path::new("/new/launcher")), State::Outdated);

        assert_eq!(add(&roots, Path::new("/new/launcher"), &ART, false), Ok(State::Added));

        let file = parse(&std::fs::read(&list).unwrap()).unwrap();
        let Some(Value::Map(entries)) = get(&file, "shortcuts") else { panic!("no list") };
        assert_eq!(entries.len(), 2);
        let Value::Map(fields) = &entries[1].1 else { panic!("not a map") };
        assert_eq!(get(fields, "appid"), Some(&Value::Int(app_id("\"/old/launcher\"", NAME))));
        assert_eq!(get(fields, "Exe"), Some(&Value::Text(b"\"/new/launcher\"".to_vec())));
        assert_eq!(std::fs::read(user.join("config/shortcuts.vdf.openrac-bak")).unwrap(), steams_list());
    }

    #[test]
    fn a_first_shortcut_makes_the_list() {
        let dir = tempfile::tempdir().unwrap();
        let roots = [dir.path().to_path_buf()];
        let user = steam(dir.path());
        assert_eq!(add(&roots, Path::new("/opt/openrac"), &ART, false), Ok(State::Added));
        assert!(!user.join("config/shortcuts.vdf.openrac-bak").exists());
        assert!(!user.join("config/shortcuts.vdf.openrac-new").exists());
    }

    #[test]
    fn nothing_is_written_while_steam_runs() {
        let dir = tempfile::tempdir().unwrap();
        let user = steam(dir.path());
        let result = add(&[dir.path().to_path_buf()], Path::new("/opt/openrac"), &ART, true);
        assert_eq!(result, Err(STEAM_OPEN.to_string()));
        assert_eq!(std::fs::read_dir(user.join("config")).unwrap().count(), 0);
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
        assert_eq!(add(&only, program, &ART, false), Err(FLATPAK_STEAM.to_string()));
        assert_eq!(std::fs::read_dir(user.join("config")).unwrap().count(), 0);

        steam(&native);
        assert_eq!(state(&only, program), State::NotAdded, "the normal Steam comes first");
        assert_eq!(add(&only, program, &ART, false), Ok(State::Added));
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
        assert!(add(&roots, Path::new("/a"), &ART, false).is_err());
    }
}
