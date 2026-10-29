//! The player's controls files, as the port reads them (port/game/common/controls_store.h,
//! port/platform/bindings.h; docs/port/CONTROLS.md):
//!
//! ```text
//! <data home>/controls/global.ini    every game reads it
//! <data home>/controls/<game>.ini    the game's own, read over the global one
//! <data home>/controls/devices.ini   the controllers the port has seen, by id and name
//! ```
//!
//! A file is `key = value` a line. `bind_<control> = names` binds one of the pad's 24 controls
//! (the 16 buttons, each stick's 4 directions) to keys and pad inputs; a value replaces only the
//! kinds it names, so what it leaves out comes from the file below it, and the port's defaults
//! under them all. `pad_*` keys tune how a controller is read, and the global file's `player1` and
//! `player2` say which controller each player is.
//!
//! Everything here keeps the lines it does not change, writes through a file beside the old one
//! as the port does, and leaves a file a newer build wrote (a `format` above [`FORMAT`]) alone. The
//! port reads a changed file within a second, so these work while the game runs too.
//!
//! The page has no controls page yet: these and their commands are what one will use.

use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

/// The format this launcher writes: the port's `kControlsFormat`.
pub const FORMAT: i64 = 1;

/// The pad's inputs by name: port/platform/bindings.cpp's `kPadNames`, in its order (a test
/// checks they agree).
pub const PAD_NAMES: &[&str] = &[
    "PadSouth",
    "PadEast",
    "PadWest",
    "PadNorth",
    "PadBack",
    "PadGuide",
    "PadStart",
    "PadLeftStick",
    "PadRightStick",
    "PadLeftShoulder",
    "PadRightShoulder",
    "PadDpadUp",
    "PadDpadDown",
    "PadDpadLeft",
    "PadDpadRight",
    "PadMisc1",
    "PadRightPaddle1",
    "PadLeftPaddle1",
    "PadRightPaddle2",
    "PadLeftPaddle2",
    "PadTouchpad",
    "PadMisc2",
    "PadMisc3",
    "PadMisc4",
    "PadMisc5",
    "PadMisc6",
    "PadLeftTrigger",
    "PadRightTrigger",
    "PadLeftStickUp",
    "PadLeftStickDown",
    "PadLeftStickLeft",
    "PadLeftStickRight",
    "PadRightStickUp",
    "PadRightStickDown",
    "PadRightStickLeft",
    "PadRightStickRight",
];

/// The value that binds nothing of either kind, and the name that binds no pad input while the
/// keys beside it stay.
pub const UNBOUND: &str = "none";
pub const PAD_NONE: &str = "PadNone";

/// The most of a kind (keys, or pad inputs) one control holds: the port's `kMostOfAKind`.
pub const MOST_OF_A_KIND: usize = 4;

/// One of the pad's controls: its key in the files, its name, whether it is a button (keys may
/// press it) and the port's defaults for it.
struct Target {
    key: &'static str,
    label: &'static str,
    button: bool,
    keys: &'static [&'static str],
    pad: &'static [&'static str],
}

/// In the order the port's `binding_keys()` lists them (a test checks it).
const TARGETS: &[Target] = &[
    Target { key: "bind_cross", label: "Cross", button: true, keys: &["Space"], pad: &["PadSouth"] },
    Target { key: "bind_circle", label: "Circle", button: true, keys: &["KeyE"], pad: &["PadEast"] },
    Target { key: "bind_square", label: "Square", button: true, keys: &["KeyF"], pad: &["PadWest"] },
    Target { key: "bind_triangle", label: "Triangle", button: true, keys: &["KeyR"], pad: &["PadNorth"] },
    Target { key: "bind_l1", label: "L1", button: true, keys: &["ShiftLeft"], pad: &["PadLeftShoulder"] },
    Target { key: "bind_r1", label: "R1", button: true, keys: &["KeyQ"], pad: &["PadRightShoulder"] },
    Target { key: "bind_l2", label: "L2", button: true, keys: &["Digit1"], pad: &["PadLeftTrigger"] },
    Target { key: "bind_r2", label: "R2", button: true, keys: &["Digit3"], pad: &["PadRightTrigger"] },
    Target { key: "bind_l3", label: "L3", button: true, keys: &["KeyC"], pad: &["PadLeftStick"] },
    Target { key: "bind_r3", label: "R3", button: true, keys: &["KeyV"], pad: &["PadRightStick"] },
    Target { key: "bind_start", label: "Start", button: true, keys: &["Enter"], pad: &["PadStart"] },
    Target { key: "bind_select", label: "Select", button: true, keys: &["Backspace"], pad: &["PadBack"] },
    Target { key: "bind_up", label: "D-pad up", button: true, keys: &["ArrowUp"], pad: &["PadDpadUp"] },
    Target { key: "bind_down", label: "D-pad down", button: true, keys: &["ArrowDown"], pad: &["PadDpadDown"] },
    Target { key: "bind_left", label: "D-pad left", button: true, keys: &["ArrowLeft"], pad: &["PadDpadLeft"] },
    Target { key: "bind_right", label: "D-pad right", button: true, keys: &["ArrowRight"], pad: &["PadDpadRight"] },
    Target { key: "bind_lstick_up", label: "Left stick up", button: false, keys: &[], pad: &["PadLeftStickUp"] },
    Target { key: "bind_lstick_down", label: "Left stick down", button: false, keys: &[], pad: &["PadLeftStickDown"] },
    Target { key: "bind_lstick_left", label: "Left stick left", button: false, keys: &[], pad: &["PadLeftStickLeft"] },
    Target {
        key: "bind_lstick_right",
        label: "Left stick right",
        button: false,
        keys: &[],
        pad: &["PadLeftStickRight"],
    },
    Target { key: "bind_rstick_up", label: "Right stick up", button: false, keys: &[], pad: &["PadRightStickUp"] },
    Target {
        key: "bind_rstick_down",
        label: "Right stick down",
        button: false,
        keys: &[],
        pad: &["PadRightStickDown"],
    },
    Target {
        key: "bind_rstick_left",
        label: "Right stick left",
        button: false,
        keys: &[],
        pad: &["PadRightStickLeft"],
    },
    Target {
        key: "bind_rstick_right",
        label: "Right stick right",
        button: false,
        keys: &[],
        pad: &["PadRightStickRight"],
    },
];

/// A `pad_*` setting: its key, its range (or a list of words), and the port's default.
struct Tuning {
    key: &'static str,
    label: &'static str,
    low: i64,
    high: i64,
    words: &'static [&'static str],
    default: &'static str,
}

/// port/platform/bindings.cpp's `read_tuning`, key by key.
const TUNING: &[Tuning] = &[
    Tuning { key: "pad_left_deadzone", label: "Left stick dead zone", low: 0, high: 50, words: &[], default: "0" },
    Tuning { key: "pad_left_outer", label: "Left stick outer edge", low: 50, high: 100, words: &[], default: "100" },
    Tuning {
        key: "pad_left_sensitivity",
        label: "Left stick sensitivity",
        low: 25,
        high: 300,
        words: &[],
        default: "100",
    },
    Tuning {
        key: "pad_left_invert_x",
        label: "Left stick: invert horizontal",
        low: 0,
        high: 1,
        words: &[],
        default: "0",
    },
    Tuning {
        key: "pad_left_invert_y",
        label: "Left stick: invert vertical",
        low: 0,
        high: 1,
        words: &[],
        default: "0",
    },
    Tuning { key: "pad_right_deadzone", label: "Right stick dead zone", low: 0, high: 50, words: &[], default: "0" },
    Tuning { key: "pad_right_outer", label: "Right stick outer edge", low: 50, high: 100, words: &[], default: "100" },
    Tuning {
        key: "pad_right_sensitivity",
        label: "Right stick sensitivity",
        low: 25,
        high: 300,
        words: &[],
        default: "100",
    },
    Tuning {
        key: "pad_right_invert_x",
        label: "Right stick: invert horizontal",
        low: 0,
        high: 1,
        words: &[],
        default: "0",
    },
    Tuning {
        key: "pad_right_invert_y",
        label: "Right stick: invert vertical",
        low: 0,
        high: 1,
        words: &[],
        default: "0",
    },
    Tuning { key: "pad_trigger_threshold", label: "Trigger press point", low: 1, high: 100, words: &[], default: "13" },
    Tuning {
        key: "pad_face_buttons",
        label: "Face buttons",
        low: 0,
        high: 0,
        words: &["position", "labels"],
        default: "position",
    },
    Tuning {
        key: "pad_background",
        label: "Controllers work behind other windows",
        low: 0,
        high: 1,
        words: &[],
        default: "1",
    },
];

/// The players' keys, in the global file only, and their defaults (port/platform/players.h).
const PLAYERS: &[(&str, &str)] = &[("player1", "keyboard, auto"), ("player2", "auto")];

/// What each control does in a game, where it is known from the game's own documentation. Shown
/// beside the control; the pad's controls themselves are the same in every game.
const HINTS: &[(&str, &str, &str)] = &[
    // docs/port/RAC1_PAL_TESTING.md, docs/port/WEAPONS.md, launcher/release-notes/0.8.0.md.
    ("rac1", "bind_cross", "Jump"),
    ("rac1", "bind_square", "Wrench (with R1 held: throw it)"),
    ("rac1", "bind_circle", "Use the equipped weapon or gadget"),
    ("rac1", "bind_triangle", "Talk; held: Quick Select"),
    ("rac1", "bind_start", "Pause"),
    ("rac1", "bind_select", "Planet map"),
    ("rac1", "bind_l1", "Look around"),
    ("rac1", "bind_r1", "Crouch"),
    ("rac1", "bind_rstick_up", "Camera"),
    ("rac1", "bind_rstick_down", "Camera"),
    ("rac1", "bind_rstick_left", "Camera"),
    ("rac1", "bind_rstick_right", "Camera"),
    ("rac1", "bind_lstick_up", "Move"),
    ("rac1", "bind_lstick_down", "Move"),
    ("rac1", "bind_lstick_left", "Move"),
    ("rac1", "bind_lstick_right", "Move"),
];

/// Where a value comes from.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[ts(rename = "ControlsLayer")]
#[serde(rename_all = "camelCase")]
pub enum Layer {
    /// The port's own.
    Default,
    /// The global file: every game, every player.
    Global,
    /// The global file's keys for one player (`p2.bind_cross`).
    GlobalPlayer,
    /// The game's own file, every player.
    Game,
    /// The game's own file's keys for one player.
    GamePlayer,
}

/// The players a file can hold keys of their own for (the port's `kPlayers`).
pub const PLAYER_COUNT: u8 = 2;

/// The start of player `player`'s own keys (0 is `p1.`): port/platform/bindings.h's player_prefix.
pub fn player_prefix(player: u8) -> String {
    format!("p{}.", player + 1)
}

/// A key's player, if it is one player's own (`p2.bind_cross`), and the key without it.
pub fn split_player(key: &str) -> (Option<u8>, &str) {
    for player in 0..PLAYER_COUNT {
        if let Some(rest) = key.strip_prefix(&player_prefix(player)) {
            return (Some(player), rest);
        }
    }
    (None, key)
}

/// A binding's or a controller setting's key, for every player or one player's own.
fn is_controls_key(key: &str) -> bool {
    let (_, key) = split_player(key);
    key.starts_with("bind_") || key.starts_with("pad_")
}

/// Which file a change goes to.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[ts(rename = "ControlsScope")]
#[serde(rename_all = "lowercase", tag = "kind", content = "game")]
pub enum Scope {
    Global,
    Game(String),
}

/// One of the pad's controls: what presses it, and where each kind came from.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct BindingRow {
    /// Its key in the files, `bind_cross`.
    pub key: String,
    pub label: String,
    /// Whether keys may press it (the sticks' directions take pad inputs only).
    pub button: bool,
    /// What it does in this game, where known.
    pub hint: Option<String>,
    /// The keys that press it now, by name.
    pub keys: Vec<String>,
    /// The pad inputs that press it now: names, or two joined by `+` for a chord.
    pub pad: Vec<String>,
    pub keys_from: Layer,
    pub pad_from: Layer,
    /// The values the global and the game's file hold for it, as written.
    pub global: Option<String>,
    pub game: Option<String>,
    /// Pad inputs that press another control too, alone: both would press together.
    pub shared: Vec<String>,
}

/// One `pad_*` setting as it is now.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct TuningRow {
    pub key: String,
    pub label: String,
    pub value: String,
    pub from: Layer,
    /// A number's range; words, for a setting that takes them.
    pub low: i64,
    pub high: i64,
    pub words: Vec<String>,
}

/// A player's controller: the global file's value, or the default.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct PlayerRow {
    pub key: String,
    /// The names: "keyboard", "auto", controller ids.
    pub wants: Vec<String>,
    /// The global file sets it (else it is the default).
    pub set: bool,
}

/// A controller the port has seen.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
pub struct DeviceSeen {
    pub id: String,
    pub name: String,
}

/// Something in a file the port leaves out, and why.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ControlsIssue {
    pub layer: Layer,
    pub key: String,
    pub message: String,
}

/// The controls as a game reads them (or the global ones alone, without a game), for a page.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ControlsView {
    pub game: Option<String>,
    /// The player the view is of (0 or 1), or none: every player's.
    pub player: Option<u8>,
    pub global_file: String,
    pub game_file: Option<String>,
    /// A newer build wrote it: it is shown, never changed.
    pub global_read_only: bool,
    pub game_read_only: bool,
    pub bindings: Vec<BindingRow>,
    pub tuning: Vec<TuningRow>,
    pub players: Vec<PlayerRow>,
    pub devices: Vec<DeviceSeen>,
    /// Every pad input's name, for a list to choose from.
    pub pad_names: Vec<String>,
    pub issues: Vec<ControlsIssue>,
}

/// The folder of the controls files.
pub fn controls_dir(data_home: &Path) -> PathBuf {
    data_home.join("controls")
}

fn plain_name(name: &str) -> bool {
    !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-')
}

/// A scope's file.
pub fn file_of(data_home: &Path, scope: &Scope) -> Result<PathBuf, String> {
    match scope {
        Scope::Global => Ok(controls_dir(data_home).join("global.ini")),
        Scope::Game(game) if plain_name(game) => Ok(controls_dir(data_home).join(format!("{game}.ini"))),
        Scope::Game(game) => Err(format!("{game} is not a game")),
    }
}

// ---- The file, line by line (port/platform/controls_file.h) ----

/// A line's key and value, trimmed; None for a comment, a blank line, or one without `=`.
fn line_pair(line: &str) -> Option<(&str, &str)> {
    let line = line.trim();
    if line.is_empty() || line.starts_with('#') {
        return None;
    }
    let (k, v) = line.split_once('=')?;
    let k = k.trim();
    (!k.is_empty()).then(|| (k, v.trim()))
}

/// Every key's value, its last line's.
pub fn values(text: &str) -> Vec<(String, String)> {
    let mut out: Vec<(String, String)> = Vec::new();
    for (k, v) in text.lines().filter_map(line_pair) {
        match out.iter_mut().find(|(key, _)| key == k) {
            Some(entry) => entry.1 = v.to_string(),
            None => out.push((k.to_string(), v.to_string())),
        }
    }
    out
}

fn value_of(text: &str, key: &str) -> Option<String> {
    text.lines().rev().filter_map(line_pair).find(|(k, _)| *k == key).map(|(_, v)| v.to_string())
}

/// The file's format, 0 for one without.
pub fn format_of(text: &str) -> i64 {
    value_of(text, "format").and_then(|v| v.parse().ok()).unwrap_or(0)
}

/// `text` with `key = value`: its last line rewritten where it is and any earlier one removed, or
/// a line added at the end (after a format line, for a file without one). Every other line stays.
pub fn with_key(text: &str, key: &str, value: &str) -> String {
    let mut lines: Vec<String> = text.lines().map(str::to_string).collect();
    if key != "format" && value_of(text, "format").is_none() {
        lines.insert(0, format!("format = {FORMAT}"));
        lines.insert(0, "# OpenRAC controls (docs/port/CONTROLS.md)".to_string());
    }
    let last = lines.iter().rposition(|l| line_pair(l).is_some_and(|(k, _)| k == key));
    let line = format!("{key} = {value}");
    match last {
        None => lines.push(line),
        Some(at) => {
            lines[at] = line;
            let mut i = 0;
            lines.retain(|l| {
                let keep = i >= at || !line_pair(l).is_some_and(|(k, _)| k == key);
                i += 1;
                keep
            });
        }
    }
    lines.into_iter().map(|l| l + "\n").collect()
}

/// `text` without any line of the keys `gone` says.
pub fn without_keys(text: &str, gone: impl Fn(&str) -> bool) -> String {
    text.lines().filter(|l| !line_pair(l).is_some_and(|(k, _)| gone(k))).map(|l| format!("{l}\n")).collect()
}

fn read(path: &Path) -> String {
    std::fs::read_to_string(path).unwrap_or_default()
}

// ---- Values (port/platform/bindings.cpp's read_binding) ----

fn is_key_name(name: &str) -> bool {
    crate::plugins::BIND_NAMES.contains(&name) && !name.starts_with("Mouse") && !name.starts_with("Wheel")
}

fn is_pad_name(name: &str) -> bool {
    PAD_NAMES.contains(&name)
}

/// What one value names: each kind it names (None: left to the layer below), and what it leaves
/// out, with why.
#[derive(Debug, Default, PartialEq, Eq)]
struct Read {
    keys: Option<Vec<String>>,
    pad: Option<Vec<String>>,
    dropped: Vec<String>,
}

fn read_value(target: &Target, value: &str) -> Read {
    if value.trim() == UNBOUND {
        return Read { keys: Some(Vec::new()), pad: Some(Vec::new()), dropped: Vec::new() };
    }
    let mut keys: Vec<String> = Vec::new();
    let mut pad: Vec<String> = Vec::new();
    let mut pad_named = false;
    let mut dropped = Vec::new();
    for name in value.split(',').map(str::trim).filter(|n| !n.is_empty()) {
        if name == PAD_NONE {
            pad_named = true;
        } else if let Some((with, input)) = name.split_once('+') {
            let (with, input) = (with.trim(), input.trim());
            if is_pad_name(with) && is_pad_name(input) && with != input {
                let chord = format!("{with}+{input}");
                if !pad.contains(&chord) && pad.len() < MOST_OF_A_KIND {
                    pad.push(chord);
                }
            } else {
                dropped.push(format!("{name} is not a chord of two pad inputs"));
            }
        } else if is_pad_name(name) {
            if !pad.iter().any(|p| p == name) && pad.len() < MOST_OF_A_KIND {
                pad.push(name.to_string());
            }
        } else if is_key_name(name) {
            if !target.button {
                dropped.push(format!("{name}: a stick's direction takes pad inputs only"));
            } else if !keys.iter().any(|k| k == name) && keys.len() < MOST_OF_A_KIND {
                keys.push(name.to_string());
            }
        } else {
            dropped.push(format!("{name} is not a key or pad input the port knows"));
        }
    }
    Read { keys: (!keys.is_empty()).then_some(keys), pad: (!pad.is_empty() || pad_named).then_some(pad), dropped }
}

/// A target's value as the port writes it: keys, then pad inputs; `PadNone` for keys without a
/// pad input, `none` for nothing.
pub fn binding_value(keys: &[String], pad: &[String]) -> String {
    if keys.is_empty() && pad.is_empty() {
        return UNBOUND.to_string();
    }
    let mut names: Vec<&str> = keys.iter().chain(pad.iter()).map(String::as_str).collect();
    if pad.is_empty() {
        names.push(PAD_NONE);
    }
    names.join(",")
}

fn check_tuning(key: &str, value: &str) -> Result<String, String> {
    let t = TUNING.iter().find(|t| t.key == key).ok_or_else(|| format!("{key} is not a controller setting"))?;
    let value = value.trim();
    if !t.words.is_empty() {
        return if t.words.contains(&value) {
            Ok(value.to_string())
        } else {
            Err(format!("{key} takes {}", t.words.join(" or ")))
        };
    }
    let number = match value {
        "on" | "true" | "yes" if t.high == 1 => 1,
        "off" | "false" | "no" if t.high == 1 => 0,
        v => v.parse::<i64>().map_err(|_| format!("{key} takes a whole number"))?,
    };
    if number < t.low || number > t.high {
        return Err(format!("{key} takes {} to {}", t.low, t.high));
    }
    Ok(number.to_string())
}

fn check_player(value: &str) -> String {
    let mut names: Vec<&str> = Vec::new();
    for name in value.split(',').map(str::trim).filter(|n| !n.is_empty() && *n != UNBOUND) {
        if !names.contains(&name) {
            names.push(name);
        }
    }
    if names.is_empty() {
        UNBOUND.to_string()
    } else {
        names.join(", ")
    }
}

/// A key and its value checked as the port reads them, and written the way it writes them: the
/// value to put in the file, or why it cannot go there.
pub fn checked(scope: &Scope, key: &str, value: &str) -> Result<String, String> {
    // One player's own key is checked as the key it is for.
    if let (Some(_), rest) = split_player(key) {
        if rest.starts_with("bind_") || rest.starts_with("pad_") {
            return checked(scope, rest, value);
        }
        return Err(format!("{key} is not one of the controls' keys"));
    }
    if let Some(target) = TARGETS.iter().find(|t| t.key == key) {
        let read = read_value(target, value);
        if let Some(why) = read.dropped.first() {
            return Err(why.clone());
        }
        return Ok(match (read.keys, read.pad) {
            (Some(k), Some(p)) => binding_value(&k, &p),
            (Some(k), None) => k.join(","),
            (None, Some(p)) if p.is_empty() => PAD_NONE.to_string(),
            (None, Some(p)) => p.join(","),
            (None, None) => return Err(format!("{key} needs a key or a pad input")),
        });
    }
    if key.starts_with("pad_") {
        return check_tuning(key, value);
    }
    if PLAYERS.iter().any(|(k, _)| *k == key) {
        return match scope {
            Scope::Global => Ok(check_player(value)),
            Scope::Game(_) => Err("which controller each player is, is set for every game".to_string()),
        };
    }
    Err(format!("{key} is not one of the controls' keys"))
}

// ---- The view ----

/// One player's own keys in a file, without their prefix.
fn own_values(text: &str, player: u8) -> Vec<(String, String)> {
    let prefix = player_prefix(player);
    values(text).into_iter().filter_map(|(k, v)| k.strip_prefix(&prefix).map(|k| (k.to_string(), v))).collect()
}

/// Everything a page shows of the controls: `game`'s (over the global ones) or the global ones
/// alone, for every player or, with `player`, as that player reads them (their own keys over every
/// player's in each file). The rows' `global` and `game` values are those of the keys this view
/// edits: the player's own with `player`.
pub fn view(data_home: &Path, game: Option<&str>, player: Option<u8>) -> Result<ControlsView, String> {
    if player.is_some_and(|p| p >= PLAYER_COUNT) {
        return Err("there are two players".to_string());
    }
    let global_path = file_of(data_home, &Scope::Global)?;
    let game_path = game.map(|g| file_of(data_home, &Scope::Game(g.to_string()))).transpose()?;
    let global = read(&global_path);
    let game_text = game_path.as_deref().map(read).unwrap_or_default();
    let mut layers = vec![(Layer::Global, values(&global))];
    if let Some(p) = player {
        layers.push((Layer::GlobalPlayer, own_values(&global, p)));
    }
    layers.push((Layer::Game, values(&game_text)));
    if let Some(p) = player {
        layers.push((Layer::GamePlayer, own_values(&game_text, p)));
    }
    let edited = |key: &str| match player {
        Some(p) => format!("{}{key}", player_prefix(p)),
        None => key.to_string(),
    };
    let mut issues = Vec::new();

    let mut bindings = Vec::new();
    for target in TARGETS {
        let mut keys: Vec<String> = target.keys.iter().map(|k| k.to_string()).collect();
        let mut pad: Vec<String> = target.pad.iter().map(|p| p.to_string()).collect();
        let (mut keys_from, mut pad_from) = (Layer::Default, Layer::Default);
        for (layer, values) in &layers {
            let Some((_, value)) = values.iter().find(|(k, _)| k == target.key) else { continue };
            let read = read_value(target, value);
            for why in read.dropped {
                issues.push(ControlsIssue { layer: *layer, key: target.key.to_string(), message: why });
            }
            if let Some(k) = read.keys {
                keys = k;
                keys_from = *layer;
            }
            if let Some(p) = read.pad {
                pad = p;
                pad_from = *layer;
            }
        }
        bindings.push(BindingRow {
            key: target.key.to_string(),
            label: target.label.to_string(),
            button: target.button,
            hint: game
                .and_then(|g| HINTS.iter().find(|(hg, k, _)| *hg == g && *k == target.key))
                .map(|h| h.2.to_string()),
            keys,
            pad,
            keys_from,
            pad_from,
            global: value_of(&global, &edited(target.key)),
            game: game.and_then(|_| value_of(&game_text, &edited(target.key))),
            shared: Vec::new(),
        });
    }
    // A lone pad input on two controls: both press together.
    let lone: Vec<(usize, String)> = bindings
        .iter()
        .enumerate()
        .flat_map(|(i, b)| b.pad.iter().filter(|p| !p.contains('+')).map(move |p| (i, p.clone())))
        .collect();
    for (i, row) in bindings.iter_mut().enumerate() {
        row.shared = row.pad.iter().filter(|p| lone.iter().any(|(j, q)| *j != i && q == *p)).cloned().collect();
    }

    let mut tuning = Vec::new();
    for t in TUNING {
        let mut value = t.default.to_string();
        let mut from = Layer::Default;
        for (layer, values) in &layers {
            if let Some((_, v)) = values.iter().find(|(k, _)| k == t.key) {
                match check_tuning(t.key, v) {
                    Ok(v) => {
                        value = v;
                        from = *layer;
                    }
                    Err(why) => issues.push(ControlsIssue { layer: *layer, key: t.key.to_string(), message: why }),
                }
            }
        }
        tuning.push(TuningRow {
            key: t.key.to_string(),
            label: t.label.to_string(),
            value,
            from,
            low: t.low,
            high: t.high,
            words: t.words.iter().map(|w| w.to_string()).collect(),
        });
    }

    let players = PLAYERS
        .iter()
        .map(|(key, default)| {
            let set = value_of(&global, key);
            let text = set.clone().unwrap_or_else(|| default.to_string());
            PlayerRow {
                key: key.to_string(),
                wants: text
                    .split(',')
                    .map(str::trim)
                    .filter(|n| !n.is_empty() && *n != UNBOUND)
                    .map(str::to_string)
                    .collect(),
                set: set.is_some(),
            }
        })
        .collect();

    let devices = values(&read(&controls_dir(data_home).join("devices.ini")))
        .into_iter()
        .filter(|(k, _)| k != "format")
        .map(|(id, name)| DeviceSeen { id, name })
        .collect();

    Ok(ControlsView {
        game: game.map(str::to_string),
        player,
        global_file: global_path.to_string_lossy().into_owned(),
        game_file: game_path.as_ref().map(|p| p.to_string_lossy().into_owned()),
        global_read_only: format_of(&global) > FORMAT,
        game_read_only: format_of(&game_text) > FORMAT,
        bindings,
        tuning,
        players,
        devices,
        pad_names: PAD_NAMES.iter().map(|n| n.to_string()).collect(),
        issues,
    })
}

// ---- Changes ----

fn writable(path: &Path) -> Result<String, String> {
    let text = read(path);
    if format_of(&text) > FORMAT {
        return Err(format!("{} was written by a newer OpenRAC; it is left as it is", path.display()));
    }
    Ok(text)
}

fn write(path: &Path, text: &str) -> Result<(), String> {
    crate::differences::write_settings(path, text)
}

/// The game a scope's view is of: its own, or none for the global file.
fn game_of(scope: &Scope) -> Option<&str> {
    match scope {
        Scope::Global => None,
        Scope::Game(g) => Some(g),
    }
}

/// Sets `key` in `scope`'s file to `value` (checked and written as the port writes it), or with
/// None removes it, so the layer below shows through. Returns the controls as they are then.
pub fn set(data_home: &Path, scope: &Scope, key: &str, value: Option<&str>) -> Result<ControlsView, String> {
    let path = file_of(data_home, scope)?;
    let text = writable(&path)?;
    let changed = match value {
        Some(v) => with_key(&text, key, &checked(scope, key, v)?),
        None => without_keys(&text, |k| k == key),
    };
    if changed != text {
        write(&path, &changed)?;
    }
    view(data_home, game_of(scope), split_player(key).0)
}

/// Puts `scope` back to what it inherits: the game's file goes; the global file loses its
/// bindings and controller settings and keeps the rest (the players).
pub fn reset(data_home: &Path, scope: &Scope) -> Result<ControlsView, String> {
    let path = file_of(data_home, scope)?;
    let text = writable(&path)?;
    match scope {
        Scope::Game(_) => {
            if path.exists() {
                std::fs::remove_file(&path).map_err(|e| format!("{}: {e}", path.display()))?;
            }
        }
        Scope::Global => {
            let changed = without_keys(&text, is_controls_key);
            if changed != text {
                write(&path, &changed)?;
            }
        }
    }
    view(data_home, game_of(scope), None)
}

/// The players back to the defaults, in the global file.
pub fn reset_players(data_home: &Path) -> Result<ControlsView, String> {
    let path = file_of(data_home, &Scope::Global)?;
    let text = writable(&path)?;
    let changed = without_keys(&text, |k| PLAYERS.iter().any(|(p, _)| *p == k));
    if changed != text {
        write(&path, &changed)?;
    }
    view(data_home, None, None)
}

/// Takes one player's own bindings and settings out of `scope`'s file, so the player reads every
/// player's again; the view returned is that player's.
pub fn reset_player(data_home: &Path, scope: &Scope, player: u8) -> Result<ControlsView, String> {
    if player >= PLAYER_COUNT {
        return Err("there are two players".to_string());
    }
    let path = file_of(data_home, scope)?;
    let text = writable(&path)?;
    let prefix = player_prefix(player);
    let changed = without_keys(&text, |k| k.starts_with(&prefix));
    if changed != text {
        write(&path, &changed)?;
    }
    view(data_home, game_of(scope), Some(player))
}

/// The bindings and controller settings of `scope`, as a file of their own (a profile to share).
pub fn export(data_home: &Path, scope: &Scope, to: &Path) -> Result<(), String> {
    let text = read(&file_of(data_home, scope)?);
    let mut out = format!("# OpenRAC controls (docs/port/CONTROLS.md)\nformat = {FORMAT}\n");
    for (k, v) in values(&text) {
        if is_controls_key(&k) {
            out.push_str(&format!("{k} = {v}\n"));
        }
    }
    write(to, &out)
}

/// Replaces `scope`'s bindings and controller settings with those of a file (exported, or another
/// game's), each checked; the rest of `scope`'s file stays. What cannot be read is left out and
/// reported in the view's issues as its message.
pub fn import(data_home: &Path, scope: &Scope, from: &Path) -> Result<ControlsView, String> {
    let incoming = std::fs::read_to_string(from).map_err(|e| format!("{}: {e}", from.display()))?;
    if format_of(&incoming) > FORMAT {
        return Err(format!("{} was written by a newer OpenRAC", from.display()));
    }
    let path = file_of(data_home, scope)?;
    let text = writable(&path)?;
    let mut changed = without_keys(&text, is_controls_key);
    let mut skipped = Vec::new();
    for (k, v) in values(&incoming) {
        if !is_controls_key(&k) {
            continue;
        }
        match checked(scope, &k, &v) {
            Ok(v) => changed = with_key(&changed, &k, &v),
            Err(why) => skipped.push(ControlsIssue { layer: Layer::Default, key: k, message: why }),
        }
    }
    write(&path, &changed)?;
    let mut view = view(data_home, game_of(scope), None)?;
    for mut issue in skipped {
        issue.message = format!("not imported: {}", issue.message);
        view.issues.push(issue);
    }
    Ok(view)
}

/// Copies one game's own controls over another's (both games' files; the global one is not
/// touched).
pub fn copy_game(data_home: &Path, from: &str, to: &str) -> Result<ControlsView, String> {
    let source = file_of(data_home, &Scope::Game(from.to_string()))?;
    if !source.exists() {
        return reset(data_home, &Scope::Game(to.to_string()));
    }
    import(data_home, &Scope::Game(to.to_string()), &source)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn home() -> tempfile::TempDir {
        tempfile::tempdir().unwrap()
    }

    fn row<'a>(v: &'a ControlsView, key: &str) -> &'a BindingRow {
        v.bindings.iter().find(|b| b.key == key).unwrap()
    }

    #[test]
    fn pad_names_and_targets_are_the_ports() {
        let source = include_str!("../../../port/platform/bindings.cpp");
        let names: Vec<&str> = source
            .lines()
            .filter_map(|l| l.trim().strip_prefix("{\"Pad"))
            .map(|rest| rest.split('"').next().unwrap())
            .collect();
        let ours: Vec<String> = PAD_NAMES.iter().map(|n| n.trim_start_matches("Pad").to_string()).collect();
        // The table of names first; the defaults' table names some of them again after it.
        assert_eq!(&names[..PAD_NAMES.len()], ours.iter().map(String::as_str).collect::<Vec<_>>());
        let keys: Vec<&str> = source
            .lines()
            .filter_map(|l| l.trim().strip_prefix("{\"bind_"))
            .map(|rest| rest.split('"').next().unwrap())
            .collect();
        let targets: Vec<&str> = TARGETS.iter().map(|t| t.key.trim_start_matches("bind_")).collect();
        assert_eq!(keys, targets);
        assert!(include_str!("../../../port/platform/bindings.h")
            .contains(&format!("inline constexpr int kMostOfAKind = {MOST_OF_A_KIND};")));
    }

    #[test]
    fn the_defaults_are_the_ports() {
        let h = home();
        let v = view(h.path(), Some("rac1"), None).unwrap();
        assert_eq!(row(&v, "bind_cross").keys, ["Space"]);
        assert_eq!(row(&v, "bind_cross").pad, ["PadSouth"]);
        assert_eq!(row(&v, "bind_l2").pad, ["PadLeftTrigger"]);
        assert_eq!(row(&v, "bind_lstick_up").pad, ["PadLeftStickUp"]);
        assert!(row(&v, "bind_lstick_up").keys.is_empty());
        assert_eq!(row(&v, "bind_cross").hint.as_deref(), Some("Jump"));
        assert!(v.bindings.iter().all(|b| b.keys_from == Layer::Default && b.pad_from == Layer::Default));
        assert!(v.bindings.iter().all(|b| b.shared.is_empty()));
        assert_eq!(v.players[0].wants, ["keyboard", "auto"]);
        assert_eq!(v.players[1].wants, ["auto"]);
        assert!(v.issues.is_empty());
        assert_eq!(v.tuning.iter().find(|t| t.key == "pad_left_deadzone").unwrap().value, "0");
    }

    #[test]
    fn a_game_reads_over_the_global_file_kind_by_kind() {
        let h = home();
        set(h.path(), &Scope::Global, "bind_cross", Some("KeyJ, PadEast")).unwrap();
        set(h.path(), &Scope::Game("rac1".into()), "bind_cross", Some("PadNorth")).unwrap();
        let v = set(h.path(), &Scope::Game("rac1".into()), "pad_left_deadzone", Some("12")).unwrap();
        let cross = row(&v, "bind_cross");
        assert_eq!(cross.keys, ["KeyJ"]);
        assert_eq!(cross.keys_from, Layer::Global);
        assert_eq!(cross.pad, ["PadNorth"]);
        assert_eq!(cross.pad_from, Layer::Game);
        assert_eq!(cross.global.as_deref(), Some("KeyJ,PadEast"));
        assert_eq!(cross.game.as_deref(), Some("PadNorth"));
        // PadNorth is Triangle's too: shared.
        assert_eq!(cross.shared, ["PadNorth"]);
        let dz = v.tuning.iter().find(|t| t.key == "pad_left_deadzone").unwrap();
        assert_eq!((dz.value.as_str(), dz.from), ("12", Layer::Game));
        // Another game sees the global file only.
        let other = view(h.path(), Some("rac2"), None).unwrap();
        assert_eq!(row(&other, "bind_cross").pad, ["PadEast"]);
    }

    #[test]
    fn values_are_checked_and_written_as_the_port_writes_them() {
        let g = Scope::Global;
        assert_eq!(checked(&g, "bind_cross", " PadEast , Space ").unwrap(), "Space,PadEast");
        assert_eq!(checked(&g, "bind_cross", "Space,PadNone").unwrap(), "Space,PadNone");
        assert_eq!(checked(&g, "bind_cross", "PadNone").unwrap(), "PadNone");
        assert_eq!(checked(&g, "bind_cross", "none").unwrap(), "none");
        assert_eq!(
            checked(&g, "bind_select", "PadLeftShoulder+PadRightShoulder").unwrap(),
            "PadLeftShoulder+PadRightShoulder"
        );
        assert!(checked(&g, "bind_cross", "MouseLeft").is_err());
        assert!(checked(&g, "bind_cross", "PadNowhere").is_err());
        assert!(checked(&g, "bind_cross", "KeyA+PadSouth").is_err());
        assert!(checked(&g, "bind_lstick_up", "KeyW").is_err());
        assert!(checked(&g, "bind_nothing", "PadSouth").is_err());
        assert_eq!(checked(&g, "pad_left_deadzone", "20").unwrap(), "20");
        assert!(checked(&g, "pad_left_deadzone", "90").is_err());
        assert_eq!(checked(&g, "pad_left_invert_x", "on").unwrap(), "1");
        assert_eq!(checked(&g, "pad_face_buttons", "labels").unwrap(), "labels");
        assert!(checked(&g, "pad_face_buttons", "upside").is_err());
        assert_eq!(checked(&g, "player2", "keyboard,auto, auto").unwrap(), "keyboard, auto");
        assert!(checked(&Scope::Game("rac1".into()), "player1", "auto").is_err());
        assert!(file_of(Path::new("/x"), &Scope::Game("../etc".into())).is_err());
    }

    #[test]
    fn lines_are_kept_and_a_newer_file_is_left_alone() {
        let text = "# mine\nformat = 1\nbind_cross = PadEast\nlauncher_note = keep\nbind_cross = PadWest\n";
        assert_eq!(
            with_key(text, "bind_cross", "PadNorth"),
            "# mine\nformat = 1\nlauncher_note = keep\nbind_cross = PadNorth\n"
        );
        assert_eq!(
            with_key("", "bind_l1", "PadMisc1"),
            "# OpenRAC controls (docs/port/CONTROLS.md)\nformat = 1\nbind_l1 = PadMisc1\n"
        );

        let h = home();
        let path = file_of(h.path(), &Scope::Game("rac1".into())).unwrap();
        std::fs::create_dir_all(path.parent().unwrap()).unwrap();
        std::fs::write(&path, "format = 5\nbind_cross = PadEast\n").unwrap();
        assert!(set(h.path(), &Scope::Game("rac1".into()), "bind_cross", Some("PadWest")).is_err());
        assert!(reset(h.path(), &Scope::Game("rac1".into())).is_err());
        assert_eq!(std::fs::read_to_string(&path).unwrap(), "format = 5\nbind_cross = PadEast\n");
        assert!(view(h.path(), Some("rac1"), None).unwrap().game_read_only);
    }

    #[test]
    fn resets_put_back_what_is_inherited_and_nothing_else() {
        let h = home();
        let game = Scope::Game("rac1".into());
        set(h.path(), &Scope::Global, "bind_cross", Some("PadEast")).unwrap();
        set(h.path(), &Scope::Global, "player2", Some("keyboard")).unwrap();
        set(h.path(), &game, "bind_cross", Some("PadNorth")).unwrap();
        // One control back to the global file's.
        let v = set(h.path(), &game, "bind_cross", None).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadEast"]);
        set(h.path(), &game, "bind_r1", Some("PadMisc1")).unwrap();
        let v = reset(h.path(), &game).unwrap();
        assert!(!Path::new(v.game_file.as_deref().unwrap()).exists());
        assert_eq!(row(&v, "bind_r1").pad, ["PadRightShoulder"]);
        let v = reset(h.path(), &Scope::Global).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadSouth"]);
        assert_eq!(v.players[1].wants, ["keyboard"]); // the players stay
        let v = reset_players(h.path()).unwrap();
        assert_eq!(v.players[1].wants, ["auto"]);
        assert!(!v.players[1].set);
    }

    #[test]
    fn export_import_and_copy() {
        let h = home();
        let game = Scope::Game("rac1".into());
        set(h.path(), &game, "bind_cross", Some("PadEast")).unwrap();
        set(h.path(), &game, "pad_right_invert_y", Some("1")).unwrap();
        let out = h.path().join("mine.ini");
        export(h.path(), &game, &out).unwrap();
        let text = std::fs::read_to_string(&out).unwrap();
        assert!(text.contains("bind_cross = PadEast"));
        assert!(text.contains("pad_right_invert_y = 1"));

        // Imported into the global file, with a bad line left out and said.
        std::fs::write(&out, format!("{text}bind_l1 = PadNowhere\nplayer1 = none\n")).unwrap();
        let v = import(h.path(), &Scope::Global, &out).unwrap();
        assert_eq!(row(&v, "bind_cross").global.as_deref(), Some("PadEast"));
        assert!(v.issues.iter().any(|i| i.key == "bind_l1" && i.message.starts_with("not imported")));
        assert!(!v.players[0].set); // players are not part of a profile

        let v = copy_game(h.path(), "rac1", "rac2").unwrap();
        assert_eq!(v.game.as_deref(), Some("rac2"));
        assert_eq!(row(&v, "bind_cross").game.as_deref(), Some("PadEast"));
        assert_eq!(row(&v, "bind_cross").pad_from, Layer::Game);
    }

    #[test]
    fn a_player_reads_their_own_keys_over_every_players() {
        let h = home();
        let game = Scope::Game("rac1".into());
        set(h.path(), &Scope::Global, "bind_cross", Some("Space,PadEast")).unwrap();
        let v = set(h.path(), &Scope::Global, "p2.bind_cross", Some("PadNorth")).unwrap();
        assert_eq!(v.player, Some(1));
        assert_eq!(row(&v, "bind_cross").pad, ["PadNorth"]);
        assert_eq!(row(&v, "bind_cross").pad_from, Layer::GlobalPlayer);
        assert_eq!(row(&v, "bind_cross").keys, ["Space"]); // every player's key stays
        assert_eq!(row(&v, "bind_cross").global.as_deref(), Some("PadNorth"));
        // A game's change for every player wins over the global file's for one player.
        set(h.path(), &game, "bind_cross", Some("PadWest")).unwrap();
        let v = view(h.path(), Some("rac1"), Some(1)).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadWest"]);
        assert_eq!(row(&v, "bind_cross").pad_from, Layer::Game);
        set(h.path(), &game, "p2.pad_left_deadzone", Some("20")).unwrap();
        let v = view(h.path(), Some("rac1"), Some(1)).unwrap();
        let dz = v.tuning.iter().find(|t| t.key == "pad_left_deadzone").unwrap();
        assert_eq!((dz.value.as_str(), dz.from), ("20", Layer::GamePlayer));
        // Player 1 and every player's view know nothing of player 2's.
        let v = view(h.path(), None, Some(0)).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadEast"]);
        let v = view(h.path(), None, None).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadEast"]);
        assert!(checked(&game, "p3.bind_cross", "PadEast").is_err());
        assert!(checked(&game, "p1.player1", "auto").is_err());
        assert!(view(h.path(), None, Some(2)).is_err());

        // Export carries the players' own keys; player 2's reset takes only theirs.
        let out = h.path().join("mine.ini");
        export(h.path(), &Scope::Global, &out).unwrap();
        assert!(std::fs::read_to_string(&out).unwrap().contains("p2.bind_cross = PadNorth"));
        let v = reset_player(h.path(), &Scope::Global, 1).unwrap();
        assert_eq!(row(&v, "bind_cross").pad, ["PadEast"]);
        assert_eq!(row(&v, "bind_cross").global, None);
        set(h.path(), &Scope::Global, "p1.bind_l1", Some("PadMisc1")).unwrap();
        reset(h.path(), &Scope::Global).unwrap();
        let text = std::fs::read_to_string(file_of(h.path(), &Scope::Global).unwrap()).unwrap();
        assert!(!text.contains("p1.") && !text.contains("bind_cross"));
    }

    #[test]
    fn devices_the_port_saw_are_listed() {
        let h = home();
        let dir = controls_dir(h.path());
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(dir.join("devices.ini"), "format = 1\n030000abcd = DualSense Wireless Controller\n").unwrap();
        let v = view(h.path(), None, None).unwrap();
        assert_eq!(v.devices, [DeviceSeen { id: "030000abcd".into(), name: "DualSense Wireless Controller".into() }]);
    }
}
