// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Keyboard and mouse: the keyboard and mouse controls of Ratchet & Clank and Going Commando,
//! built into the port (port/game/rac1/keyboard_mouse, port/game/rac2/keyboard_mouse,
//! docs/port/KEYBOARD_MOUSE.md), as a built-in entry of the Mods tab. It was the keyboard-mouse mod of openrac-mods. It is always on: the game starts on the
//! gamepad and switches to the keyboard and mouse when the player uses them. The entry shows the
//! settings its manifest declares (the same file the port embeds), and keeps their values in the
//! version's settings file as `kbm_<key> = <value>` lines, as Enhanced graphics keeps its own
//! (enhanced.rs). A copy of the old mod in the list is not loaded by the port any more.

use std::path::Path;

use serde_json::Value;

use crate::differences::{read_switch, read_value, settings_file, with_value, write_settings};
use crate::plugins::{check_value, mark_conflicts, Manifest, PluginView, SettingSpec, SettingView};

/// The entry's id on the Mods tab (`?mod=keyboard-mouse`), the old mod's id.
pub const ID: &str = "keyboard-mouse";
/// The settings file's prefix of its keys (port/game/common/settings.h, kKeyboardMousePrefix).
pub const PREFIX: &str = "kbm_";
/// The manifest the port embeds: its settings, presets and texts.
const MANIFEST: &str = include_str!("../../../port/game/common/keyboard_mouse/openrac-mod.json");

/// The manifest, as the port reads it.
fn manifest() -> Manifest {
    serde_json::from_str(MANIFEST).expect("the built-in keyboard and mouse's manifest is valid")
}

/// Whether a version has it: the discs its manifest names.
pub fn supported(game: &str, version: &str) -> bool {
    manifest().games.get(game).is_some_and(|versions| versions.iter().any(|v| v == version))
}

/// A setting's value as the settings file holds it, or its default.
fn read_setting(text: &str, spec: &SettingSpec) -> Value {
    let key = format!("{PREFIX}{}", spec.key);
    let raw = read_value(text, &key);
    let value = match (spec.kind.as_str(), raw) {
        ("bool", _) => read_switch(text, &key).map(Value::Bool),
        ("int", Some(v)) => v.parse::<i64>().ok().map(Value::from),
        ("float", Some(v)) => v.parse::<f64>().ok().map(Value::from),
        (_, Some(v)) => Some(Value::String(v)),
        _ => None,
    };
    value.and_then(|v| check_value(spec, &v)).unwrap_or_else(|| spec.default.clone())
}

/// A value as the port reads it: 1|0, a plain number, the text of a choice or a keybind.
fn written(value: &Value) -> String {
    match value {
        Value::Bool(on) => (if *on { "1" } else { "0" }).to_string(),
        Value::Number(n) => {
            if let Some(i) = n.as_i64() {
                return i.to_string();
            }
            let text = format!("{:.3}", n.as_f64().unwrap_or(0.0));
            text.trim_end_matches('0').trim_end_matches('.').to_string()
        }
        Value::String(s) => s.clone(),
        other => other.to_string(),
    }
}

/// The entry for `game`/`version`, its values from the version's settings file in `data_home`.
pub fn view(data_home: &Path, game: &str, version: &str) -> PluginView {
    let m = manifest();
    let text = std::fs::read_to_string(settings_file(data_home, game, version)).unwrap_or_default();
    let mut settings: Vec<SettingView> = m
        .settings
        .iter()
        .filter(|spec| spec.for_game(game))
        .map(|spec| SettingView { spec: spec.clone(), value: read_setting(&text, spec), conflicts: Vec::new() })
        .collect();
    mark_conflicts(&mut settings);
    PluginView {
        id: ID.into(),
        name: m.name.clone(),
        version: "Built in".into(),
        author: "OpenRAC".into(),
        description: "Play with a keyboard and mouse: the mouse turns the camera, the icons show your keys. The \
                      game switches to them when you use them, and back to the gamepad when you press it."
            .into(),
        homepage: String::new(),
        license: m.license.clone(),
        enabled: true,
        state: "ready".into(),
        reason: String::new(),
        blocked: String::new(),
        settings,
        presets: m.presets.clone(),
        hooks: Vec::new(),
        shared_hooks: Vec::new(),
        native: false,
        kind: "always-on".into(),
        path: String::new(),
        missing: Vec::new(),
        tool: None,
    }
}

/// Sets one of its settings, checked as the port would read it.
pub fn set_setting(data_home: &Path, game: &str, version: &str, key: &str, value: &Value) -> Result<(), String> {
    if !supported(game, version) {
        return Err(format!("{game} {version} has no keyboard and mouse settings"));
    }
    let m = manifest();
    let spec = m
        .settings
        .iter()
        .find(|s| s.key == key && s.for_game(game))
        .ok_or_else(|| format!("{key} is not a keyboard and mouse setting of {game}"))?;
    let value = check_value(spec, value).ok_or_else(|| format!("{} cannot be {value}", spec.name))?;
    let file = settings_file(data_home, game, version);
    let text = std::fs::read_to_string(&file).unwrap_or_default();
    write_settings(&file, &with_value(&text, &format!("{PREFIX}{key}"), &written(&value)))
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;
    use std::path::PathBuf;

    fn temp(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("openrac-kbm-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    fn the_discs_it_knows() {
        assert!(supported("rac1", "pal"));
        assert!(supported("rac1", "ntsc"));
        assert!(!supported("rac1", "jp"));
        for v in ["ntsc", "pal", "ntsc2"] {
            assert!(supported("rac2", v));
        }
        assert!(!supported("rac3", "ntsc"));
    }

    #[test]
    fn going_commando_has_no_ratchet_only_settings() {
        let home = temp("rac2");
        let v = view(&home, "rac2", "ntsc");
        let has = |key: &str| v.settings.iter().any(|s| s.spec.key == key);
        assert!(has("jump") && has("mouse_sensitivity") && has("key_prompts"));
        // Clicks in menus and the weapon keys work in Going Commando too.
        assert!(has("menu_mouse") && has("weapon_1") && has("weapon_last") && !has("camera_reset"));
        assert!(set_setting(&home, "rac2", "ntsc", "weapon_1", &json!("Digit1")).is_ok());
        assert!(set_setting(&home, "rac2", "ntsc", "jump", &json!("KeyJ")).is_ok());
        let _ = std::fs::remove_dir_all(&home);
    }

    #[test]
    fn the_settings_are_the_manifests_as_written() {
        // The page shows the manifest's lines as they are: nothing is added to them, and the
        // settings the mouse itself owns (its DPI, the system's acceleration) are not here.
        let v = view(&temp("plain"), "rac1", "ntsc");
        let sens = v.settings.iter().find(|s| s.spec.key == "mouse_sensitivity").unwrap();
        let spec = manifest().settings.iter().find(|s| s.key == "mouse_sensitivity").unwrap().clone();
        assert_eq!(sens.spec.description, spec.description);
        assert!(!v.settings.iter().any(|s| s.spec.key == "mouse_dpi" || s.spec.key == "mouse_acceleration"));
    }

    #[test]
    fn defaults_then_written_values() {
        let home = temp("values");
        let v = view(&home, "rac1", "ntsc");
        assert!(v.enabled && v.kind == "always-on");
        let get = |v: &PluginView, key: &str| v.settings.iter().find(|s| s.spec.key == key).unwrap().value.clone();
        assert_eq!(get(&v, "mouse_sensitivity"), json!(1.0));
        assert_eq!(get(&v, "jump"), json!("Space"));
        assert!(!v.presets.is_empty());

        set_setting(&home, "rac1", "ntsc", "mouse_sensitivity", &json!(2.5)).unwrap();
        set_setting(&home, "rac1", "ntsc", "invert_y", &json!(true)).unwrap();
        set_setting(&home, "rac1", "ntsc", "jump", &json!("KeyJ,MouseRight")).unwrap();
        set_setting(&home, "rac1", "ntsc", "stick_ramp_ms", &json!(80)).unwrap();
        let text = std::fs::read_to_string(settings_file(&home, "rac1", "ntsc")).unwrap();
        assert!(text.contains("kbm_mouse_sensitivity = 2.5\n"), "{text}");
        assert!(text.contains("kbm_invert_y = 1\n"), "{text}");
        assert!(text.contains("kbm_jump = KeyJ,MouseRight\n"), "{text}");
        assert!(text.contains("kbm_stick_ramp_ms = 80\n"), "{text}");
        let v = view(&home, "rac1", "ntsc");
        assert_eq!(get(&v, "mouse_sensitivity"), json!(2.5));
        assert_eq!(get(&v, "invert_y"), json!(true));
        assert_eq!(get(&v, "jump"), json!("KeyJ,MouseRight"));
        // Right click is the Quick Select's too: both are marked.
        let jump = v.settings.iter().find(|s| s.spec.key == "jump").unwrap();
        assert!(!jump.conflicts.is_empty());

        // Out of range or not a setting: refused, the file unchanged.
        assert!(set_setting(&home, "rac1", "ntsc", "mouse_sensitivity", &json!(50.0)).is_err());
        assert!(set_setting(&home, "rac1", "ntsc", "nothing", &json!(1)).is_err());
        // A version it does not know writes no file, so no path out of the settings folder.
        assert!(set_setting(&home, "rac1", "../../x", "jump", &json!("KeyJ")).is_err());
        assert!(set_setting(&home, "rac1", "jp", "jump", &json!("KeyJ")).is_err());
        assert!(!settings_file(&home, "rac1", "jp").exists());
        let _ = std::fs::remove_dir_all(&home);
    }

    #[test]
    fn a_broken_value_reads_as_its_default() {
        let home = temp("broken");
        let file = settings_file(&home, "rac1", "pal");
        std::fs::create_dir_all(file.parent().unwrap()).unwrap();
        std::fs::write(&file, "kbm_mouse_sensitivity = fast\nkbm_jump = NotAKey\n").unwrap();
        let v = view(&home, "rac1", "pal");
        let get = |key: &str| v.settings.iter().find(|s| s.spec.key == key).unwrap().value.clone();
        assert_eq!(get("mouse_sensitivity"), json!(1.0));
        assert_eq!(get("jump"), json!("Space"));
        let _ = std::fs::remove_dir_all(&home);
    }
}
