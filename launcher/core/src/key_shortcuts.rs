//! The game's keyboard shortcuts (F12 for a screenshot, F11 for full screen...), as Settings >
//! Controls shows and changes them. The game keeps each in the global controls file as
//! `shortcut_<id>` (port/game/common/frontend/shortcuts.cpp, whose list this one follows); one the
//! file does not name has its default. A value is keys by comma, each with its Ctrl+, Shift+ and
//! Alt+ first: "F11,Alt+Enter".

use serde::Serialize;
use std::path::Path;

use crate::controls::{self, Scope};

/// One shortcut: (id, name, section, default keys), in the game's order.
const ALL: &[(&str, &str, &str, &str)] = &[
    ("screenshot", "Screenshot", "Game", "F12"),
    ("full_screen", "Full screen", "Game", "F11,Alt+Enter"),
    ("volume_down", "Volume down", "Game", "F9"),
    ("volume_up", "Volume up", "Game", "F10"),
    ("frame_counter", "Frame counter", "Game", "F2"),
    ("enhanced_graphics", "Enhanced graphics", "Game", "F4"),
    ("texture_pack", "Show or hide the texture pack", "Game", "Ctrl+F2"),
    ("stats_panel", "Stats panel", "Developer", "F3"),
    ("record_diagnostics", "Record diagnostics", "Developer", "Ctrl+F3"),
    ("pause", "Pause and resume", "Developer", "F5"),
    ("step_tick", "Step one tick", "Developer", "F6"),
    ("slower", "Slower", "Developer", "F7"),
    ("faster", "Faster", "Developer", "F8"),
    ("level_select", "Level select", "Developer", "F1"),
];

const MODIFIERS: [&str; 3] = ["Ctrl+", "Shift+", "Alt+"];

/// One shortcut as the page shows it.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "ShortcutRow")]
pub struct Row {
    pub id: String,
    pub name: String,
    /// "Game" or "Developer".
    pub section: String,
    /// The keys bound now, as the file writes them; empty: none.
    pub keys: String,
    pub defaults: String,
}

/// `value` checked, as the file writes it: keys the game knows, each modifier once.
fn checked(value: &str) -> Result<String, String> {
    let mut out = Vec::new();
    for one in value.split(',').map(str::trim).filter(|s| !s.is_empty()) {
        let mut rest = one;
        let mut had = [false; 3];
        while let Some(i) = MODIFIERS.iter().position(|m| rest.starts_with(m)) {
            if had[i] {
                return Err(format!("{one}: {} twice", MODIFIERS[i].trim_end_matches('+')));
            }
            had[i] = true;
            rest = &rest[MODIFIERS[i].len()..];
        }
        if !controls::is_key_name(rest) {
            return Err(format!("{one} is not a key"));
        }
        let mods: String = MODIFIERS.iter().zip(had).filter(|(_, on)| *on).map(|(m, _)| *m).collect();
        out.push(format!("{mods}{rest}"));
    }
    Ok(out.join(","))
}

/// A value as the game reads it: the keys it knows, the others left out ("F12,NotAKey" is F12).
fn readable(value: &str) -> String {
    value.split(',').filter_map(|one| checked(one).ok()).filter(|k| !k.is_empty()).collect::<Vec<_>>().join(",")
}

fn file_key(id: &str) -> String {
    format!("shortcut_{id}")
}

/// Every shortcut with its keys now.
pub fn view(data_home: &Path) -> Result<Vec<Row>, String> {
    let path = controls::file_of(data_home, &Scope::Global)?;
    let values = controls::values(&std::fs::read_to_string(&path).unwrap_or_default());
    Ok(ALL
        .iter()
        .map(|(id, name, section, defaults)| {
            let stored = values.iter().find(|(k, _)| *k == file_key(id)).map(|(_, v)| v.clone());
            Row {
                id: id.to_string(),
                name: name.to_string(),
                section: section.to_string(),
                keys: stored.map(|v| readable(&v)).unwrap_or_else(|| defaults.to_string()),
                defaults: defaults.to_string(),
            }
        })
        .collect())
}

/// Binds `keys` to shortcut `id` ("" binds none); None puts its default back.
pub fn set(data_home: &Path, id: &str, keys: Option<&str>) -> Result<Vec<Row>, String> {
    if !ALL.iter().any(|(known, ..)| *known == id) {
        return Err(format!("{id} is not a shortcut"));
    }
    let path = controls::file_of(data_home, &Scope::Global)?;
    let text = controls::writable(&path)?;
    let key = file_key(id);
    let changed = match keys {
        Some(keys) => controls::with_key(&text, &key, &checked(keys)?),
        None => controls::without_keys(&text, |k| k == key),
    };
    if changed != text {
        controls::write(&path, &changed)?;
    }
    view(data_home)
}

/// Every shortcut back to its default.
pub fn reset(data_home: &Path) -> Result<Vec<Row>, String> {
    let path = controls::file_of(data_home, &Scope::Global)?;
    let text = controls::writable(&path)?;
    let changed = controls::without_keys(&text, |k| k.starts_with("shortcut_"));
    if changed != text {
        controls::write(&path, &changed)?;
    }
    view(data_home)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_then_set_and_back() {
        let home = tempfile::tempdir().unwrap();
        let rows = view(home.path()).unwrap();
        assert_eq!(rows.len(), ALL.len());
        assert_eq!(rows[0].keys, "F12");
        let rows = set(home.path(), "screenshot", Some("Shift+Ctrl+KeyP, F12")).unwrap();
        assert_eq!(rows[0].keys, "Ctrl+Shift+KeyP,F12");
        let text = std::fs::read_to_string(home.path().join("controls/global.ini")).unwrap();
        assert!(text.contains("shortcut_screenshot = Ctrl+Shift+KeyP,F12"), "{text}");
        assert_eq!(set(home.path(), "pause", Some("")).unwrap()[9].keys, "");
        assert_eq!(set(home.path(), "screenshot", None).unwrap()[0].keys, "F12");
        assert_eq!(reset(home.path()).unwrap()[9].keys, "F5");
    }

    #[test]
    fn shows_what_the_game_reads_of_a_hand_written_file() {
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("controls");
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(
            dir.join("global.ini"),
            "format = 1\nshortcut_screenshot = F12 , NotAKey\nshortcut_pause =\nshortcut_faster = Ctrl+\n",
        )
        .unwrap();
        let rows = view(home.path()).unwrap();
        assert_eq!(rows[0].keys, "F12");
        assert_eq!(rows.iter().find(|r| r.id == "pause").unwrap().keys, "");
        assert_eq!(rows.iter().find(|r| r.id == "faster").unwrap().keys, "");
    }

    #[test]
    fn leaves_a_newer_file_alone() {
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("controls");
        std::fs::create_dir_all(&dir).unwrap();
        let newer = "format = 999\nshortcut_screenshot = F1\n";
        std::fs::write(dir.join("global.ini"), newer).unwrap();
        assert!(set(home.path(), "screenshot", Some("F2")).is_err());
        assert!(reset(home.path()).is_err());
        assert_eq!(std::fs::read_to_string(dir.join("global.ini")).unwrap(), newer);
    }

    #[test]
    fn refuses_what_the_game_cannot_read() {
        let home = tempfile::tempdir().unwrap();
        assert!(set(home.path(), "nothing", Some("F1")).is_err());
        assert!(set(home.path(), "pause", Some("NotAKey")).is_err());
        assert!(set(home.path(), "pause", Some("Ctrl+Ctrl+F1")).is_err());
        assert!(set(home.path(), "pause", Some("Mouse1")).is_err());
    }
}
