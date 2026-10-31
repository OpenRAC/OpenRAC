// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

//! Enhanced graphics: the experimental probe lighting, modern shadows and ambient occlusion of the
//! native port (port/enhanced), as a built-in entry of the Mods tab. It is not a mod the player
//! installs: the game carries the code, off unless `enhanced_rendering = 1` is in the version's
//! settings file (docs/port/SETTINGS.md), and the entry writes that key and the others it shows
//! there, a line at a time, as the game fixes are (differences.rs).
//!
//! With it on, Play starts the game with the Vulkan renderer, the only one that has it, and with
//! `--probes <data>/probes/<version>`: the probe bakes made on this computer from the player's own
//! extracted levels by the `probe-bake` action (openrac-enhanced-baker --bake-all --update).
//! Each baked level records what it was made from (`openrac-bake.json`); a level whose export or
//! baker changed since counts as not ready and is baked again. The baker is named by what it says
//! it bakes (`--print-profile`: its probe format, bake version and quality), not by its file, which
//! every build of a new commit changes. A level without a bake keeps the
//! original lighting. Intel Macs ship without the baker (the maintainers' decision: Embree's static
//! library cannot be built there, tools/build_port.py), so the entry says that there instead of
//! offering a bake.

use std::path::{Path, PathBuf};

use serde::Serialize;
use serde_json::Value;

use crate::differences::{read_switch, read_value, settings_file, with_value, write_settings};
use crate::plugins::{check_value, Choice, PluginView, SettingSpec, SettingView};

mod bakes;
pub use bakes::{bake_log, bake_status, delete_bakes, version_bake_status, BakeStatus};

/// The entry's id on the Mods tab (`?mod=enhanced-graphics`).
pub const ID: &str = "enhanced-graphics";
/// The settings key that switches enhanced rendering on (port/game/common/settings.h).
pub const SWITCH: &str = "enhanced_rendering";
/// The action that bakes the probes (launcher/actions.json).
pub const BAKE_ACTION: &str = "probe-bake";
/// Why an Intel Mac has no bakes, as tools/build_port.py says it when it leaves the baker out.
pub const NO_BAKER_HERE: &str =
    "Intel Macs ship without the probe baker: their enhanced graphics keep the original lighting.";
/// Whether this computer gets the probe baker: every one but an Intel Mac (NO_BAKER_HERE). It is
/// the launcher's own build that counts, as `platform.machine()` does for build_port.py.
pub const SHIPS_BAKER: bool = !cfg!(all(target_os = "macos", target_arch = "x86_64"));

/// Whether a version has the entry: the bake planner reads RAC1's levels (PAL and NTSC-U).
pub fn supported(game: &str, version: &str) -> bool {
    game == "rac1" && matches!(version, "pal" | "ntsc")
}

/// Where a version's probe bakes are kept: `<data>/probes/<version>`.
pub fn probes_dir(data: &Path, version: &str) -> PathBuf {
    data.join("probes").join(version)
}

fn spec(key: &str, name: &str, kind: &str, default: Value, description: &str) -> SettingSpec {
    SettingSpec {
        key: key.into(),
        name: name.into(),
        kind: kind.into(),
        default,
        description: description.into(),
        min: None,
        max: None,
        step: None,
        choices: Vec::new(),
        max_length: None,
        binds: None,
        group: String::new(),
    }
}

fn range(mut s: SettingSpec, min: f64, max: f64, step: f64) -> SettingSpec {
    (s.min, s.max, s.step) = (Some(min), Some(max), Some(step));
    s
}

fn advanced(mut s: SettingSpec) -> SettingSpec {
    s.group = "advanced".into();
    s
}

/// The settings the entry's page shows, with the port's defaults and ranges (settings.cpp). The
/// diagnostic ones (probe spheres, receiver biases, GTAO directions and steps) stay in the game's
/// own Enhanced menu.
pub fn settings() -> Vec<SettingSpec> {
    let mut quality = spec("gtao_quality", "Occlusion quality", "choice", "performance".into(), "");
    quality.choices = vec![
        Choice { value: "performance".into(), name: "Performance".into() },
        Choice { value: "quality".into(), name: "Quality".into() },
    ];
    let mut shadows = spec(
        "shadow_quality",
        "Shadow quality",
        "choice",
        "high".into(),
        "Low uses the smallest shadow textures. Medium balances detail and cost. High keeps the sharpest shadows.",
    );
    shadows.choices = vec![
        Choice { value: "off".into(), name: "Off".into() },
        Choice { value: "low".into(), name: "Low".into() },
        Choice { value: "medium".into(), name: "Medium".into() },
        Choice { value: "high".into(), name: "High".into() },
    ];
    let mut texture_filter = spec(
        "texture_filter",
        "Texture upscaling",
        "choice",
        "edge-aware".into(),
        "Enlarges world textures while keeping edges sharp.",
    );
    texture_filter.choices = vec![
        Choice { value: "normal".into(), name: "Normal".into() },
        Choice { value: "edge-aware".into(), name: "Edge-aware (2x)".into() },
    ];
    let mut temporal = spec(
        "temporal_quality",
        "FSR temporal upscaling",
        "choice",
        "quality".into(),
        "Reconstructs a lower-resolution world image. The HUD stays at full resolution.",
    );
    temporal.choices = vec![
        Choice { value: "off".into(), name: "Off".into() },
        Choice { value: "native-aa".into(), name: "Native AA".into() },
        Choice { value: "quality".into(), name: "Quality".into() },
        Choice { value: "balanced".into(), name: "Balanced".into() },
        Choice { value: "performance".into(), name: "Performance".into() },
    ];
    vec![
        shadows,
        spec("gtao", "Ambient occlusion", "bool", true.into(), "Soft shade in corners and under objects."),
        quality,
        texture_filter,
        temporal,
        spec(
            "agx_preview",
            "Color Correction",
            "bool",
            true.into(),
            "Balances colour and contrast for a softer, more natural image, and lets you adjust brightness.",
        ),
        range(
            spec("agx_brightness", "Brightness", "float", 0.5.into(), "Makes the image darker or brighter."),
            0.0,
            1.0,
            0.01,
        ),
        spec("motion_blur", "Motion blur", "bool", true.into(), "Subtle motion blur to enhance animations."),
        advanced(range(spec("temporal_sharpness", "FSR sharpening", "float", 0.2.into(), ""), 0.0, 1.0, 0.05)),
    ]
}

/// A setting's value as the settings file holds it, or its default.
fn read_setting(text: &str, spec: &SettingSpec) -> Value {
    if spec.key == "shadow_quality" && read_switch(text, "enhanced_shadows") == Some(false) {
        return "off".into();
    }
    let raw = read_value(text, &spec.key);
    let value = match (spec.kind.as_str(), raw) {
        ("bool", _) => read_switch(text, &spec.key).map(Value::Bool),
        ("int", Some(v)) => v.parse::<i64>().ok().map(Value::from),
        ("float", Some(v)) => v.parse::<f64>().ok().map(Value::from),
        ("choice", Some(v)) => Some(Value::String(v)),
        _ => None,
    };
    value.and_then(|v| check_value(spec, &v)).unwrap_or_else(|| spec.default.clone())
}

/// A value as the port reads it: 1|0, a plain number, a choice's name.
fn written(value: &Value) -> String {
    match value {
        Value::Bool(on) => (if *on { "1" } else { "0" }).to_string(),
        Value::Number(n) => {
            let x = n.as_f64().unwrap_or(0.0);
            let text = format!("{x:.3}");
            text.trim_end_matches('0').trim_end_matches('.').to_string()
        }
        Value::String(s) => s.clone(),
        other => other.to_string(),
    }
}

/// The entry as the Mods tab shows it: a mod's row and page, with the bake's status beside it.
#[derive(Debug, Clone, Serialize, PartialEq, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct EnhancedView {
    #[serde(rename = "mod")]
    pub entry: PluginView,
    pub bake: BakeStatus,
}

/// The entry for `game`/`version`, its values from the version's settings file in `data_home`.
pub fn view(data_home: &Path, game: &str, version: &str, bake: BakeStatus) -> EnhancedView {
    let text = std::fs::read_to_string(settings_file(data_home, game, version)).unwrap_or_default();
    let enabled = read_switch(&text, SWITCH).unwrap_or(false);
    let settings = settings()
        .into_iter()
        .map(|spec| {
            let value = read_setting(&text, &spec);
            SettingView { spec, value, conflicts: Vec::new() }
        })
        .collect();
    EnhancedView {
        entry: PluginView {
            id: ID.into(),
            name: "Enhanced graphics".into(),
            version: "Experimental".into(),
            author: "OpenRAC".into(),
            description:
                "Improved lighting and effects whilst preserving the charm of the original visuals we all love.".into(),
            homepage: String::new(),
            license: "GPL-3.0-or-later".into(),
            enabled,
            state: if enabled { "ready" } else { "off" }.into(),
            reason: String::new(),
            blocked: String::new(),
            settings,
            presets: Vec::new(),
            hooks: Vec::new(),
            shared_hooks: Vec::new(),
            native: false,
            kind: "built-in".into(),
            path: String::new(),
            missing: Vec::new(),
        },
        bake,
    }
}

/// Switches enhanced rendering on or off for the version.
pub fn set_enabled(data_home: &Path, game: &str, version: &str, on: bool) -> Result<(), String> {
    let file = settings_file(data_home, game, version);
    let text = std::fs::read_to_string(&file).unwrap_or_default();
    write_settings(&file, &with_value(&text, SWITCH, if on { "1" } else { "0" }))
}

/// Sets one of the entry's settings, checked as the port would read it.
pub fn set_setting(data_home: &Path, game: &str, version: &str, key: &str, value: &Value) -> Result<(), String> {
    let all = settings();
    let spec = all.iter().find(|s| s.key == key).ok_or_else(|| format!("{key} is not an enhanced graphics setting"))?;
    let value = check_value(spec, value).ok_or_else(|| format!("{} cannot be {value}", spec.name))?;
    let file = settings_file(data_home, game, version);
    let text = std::fs::read_to_string(&file).unwrap_or_default();
    let text = if key == "shadow_quality" {
        // Keep the renderer's quality enum and legacy enable switch compatible.
        let enabled = value != "off";
        let text = with_value(&text, "enhanced_shadows", if enabled { "1" } else { "0" });
        if enabled {
            with_value(&text, key, &written(&value))
        } else {
            text
        }
    } else {
        with_value(&text, key, &written(&value))
    };
    write_settings(&file, &text)
}

/// What Play adds: environment variables, then arguments.
pub type PlayAdditions = (Vec<(String, String)>, Vec<String>);

/// What Play adds when the version's settings switch enhanced rendering on: the Vulkan renderer
/// and the probe bakes' folder. `settings` is the version's settings file's text.
pub fn play_additions(settings: &str, data: &Path, version: &str) -> Option<PlayAdditions> {
    if !read_switch(settings, SWITCH).unwrap_or(false) {
        return None;
    }
    let env = vec![("OPENRAC_GL_API".to_string(), "vulkan".to_string())];
    let args = vec!["--probes".to_string(), probes_dir(data, version).display().to_string()];
    Some((env, args))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temp(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("openrac-enhanced-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    fn simplified_settings_preserve_shadow_compatibility() {
        let home = temp("shadow-options");
        let all = settings();
        assert_eq!(
            all.iter().filter(|s| s.group == "advanced").map(|s| s.key.as_str()).collect::<Vec<_>>(),
            vec!["temporal_sharpness"]
        );
        assert!(all
            .iter()
            .all(|s| s.key != "enhanced_shadows" && (s.key == "motion_blur" || !s.key.starts_with("motion_blur_"))));
        for version in ["pal", "ntsc"] {
            let file = settings_file(&home, "rac1", version);
            std::fs::create_dir_all(file.parent().unwrap()).unwrap();
            std::fs::write(&file, "enhanced_shadows = 0\nshadow_quality = low\nmotion_blur = 0\n").unwrap();
            let shadow = all.iter().find(|s| s.key == "shadow_quality").unwrap();
            let text = std::fs::read_to_string(&file).unwrap();
            assert_eq!(read_setting(&text, shadow), Value::from("off"));
            for quality in ["low", "medium", "high", "off"] {
                set_setting(&home, "rac1", version, "shadow_quality", &Value::from(quality)).unwrap();
                let text = std::fs::read_to_string(&file).unwrap();
                assert_eq!(read_setting(&text, shadow), Value::from(quality));
                assert_eq!(read_switch(&text, "enhanced_shadows"), Some(quality != "off"));
                assert_eq!(read_switch(&text, "motion_blur"), Some(false));
                assert_ne!(read_value(&text, "shadow_quality"), Some("off".into()));
            }
        }
        let _ = std::fs::remove_dir_all(home);
    }

    #[test]
    fn off_until_switched_on_and_settings_land_in_the_ports_file() {
        let home = temp("settings");
        let file = settings_file(&home, "rac1", "pal");
        std::fs::create_dir_all(file.parent().unwrap()).unwrap();
        std::fs::write(&file, "msaa = 4\ngtao = 1\nprobe_lighting = 0\nagx_exposure = 1.5\n").unwrap();
        let v = view(&home, "rac1", "pal", BakeStatus::default());
        assert!(!v.entry.enabled);
        let value =
            |v: &EnhancedView, key: &str| v.entry.settings.iter().find(|s| s.spec.key == key).unwrap().value.clone();
        assert_eq!(value(&v, "gtao"), Value::Bool(true));
        assert!(v.entry.settings.iter().all(|s| s.spec.key != "probe_lighting"));
        assert!(set_setting(&home, "rac1", "pal", "probe_lighting", &Value::Bool(false)).is_err());
        assert_eq!(value(&v, "texture_filter"), Value::from("edge-aware"));
        assert_eq!(value(&v, "temporal_quality"), Value::from("quality"));
        assert_eq!(value(&v, "shadow_quality"), Value::from("high"));
        assert_eq!(value(&v, "temporal_sharpness"), Value::from(0.2));
        assert_eq!(value(&v, "agx_preview"), Value::Bool(true));
        assert_eq!(value(&v, "motion_blur"), Value::Bool(true));

        assert_eq!(value(&v, "agx_brightness"), Value::from(0.5));
        for brightness in [0.0, 0.5, 1.0] {
            set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(brightness)).unwrap();
            assert_eq!(
                value(&view(&home, "rac1", "pal", BakeStatus::default()), "agx_brightness"),
                Value::from(brightness)
            );
        }
        assert!(set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(1.01)).is_err());
        assert!(set_setting(&home, "rac1", "pal", "agx_brightness", &Value::from(-0.01)).is_err());
        set_setting(&home, "rac1", "pal", "agx_preview", &Value::Bool(false)).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "agx_preview"), Value::Bool(false));
        set_enabled(&home, "rac1", "pal", true).unwrap();
        set_setting(&home, "rac1", "pal", "motion_blur", &Value::Bool(true)).unwrap();
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("low")).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "shadow_quality"), Value::from("low"));
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("medium")).unwrap();
        set_setting(&home, "rac1", "pal", "gtao_quality", &Value::from("quality")).unwrap();
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("off")).unwrap();
        assert_eq!(value(&view(&home, "rac1", "pal", BakeStatus::default()), "shadow_quality"), Value::from("off"));
        set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("medium")).unwrap();
        set_setting(&home, "rac1", "pal", "texture_filter", &Value::from("normal")).unwrap();
        let text = std::fs::read_to_string(&file).unwrap();
        assert!(text.starts_with("msaa = 4\ngtao = 1\n"), "{text}");
        for line in [
            "enhanced_rendering = 1",
            "gtao_quality = quality",
            "enhanced_shadows = 1",
            "shadow_quality = medium",
            "texture_filter = normal",
            "agx_exposure = 1.5",
            "agx_preview = 0",
            "motion_blur = 1",
        ] {
            assert!(text.lines().any(|l| l == line), "{line} in {text}");
        }
        let v = view(&home, "rac1", "pal", BakeStatus::default());
        assert!(v.entry.enabled);
        assert_eq!(value(&v, "texture_filter"), Value::from("normal"));
        assert_eq!(value(&v, "shadow_quality"), Value::from("medium"));
        assert_eq!(value(&v, "motion_blur"), Value::Bool(true));
        // Out of the port's range, another type, or not one of the entry's: refused, file kept.
        assert!(set_setting(&home, "rac1", "pal", "gtao", &Value::from("yes")).is_err());
        assert!(set_setting(&home, "rac1", "pal", "probe_spheres", &Value::Bool(true)).is_err());
        assert!(set_setting(&home, "rac1", "pal", "texture_filter", &Value::from("unknown")).is_err());
        assert!(set_setting(&home, "rac1", "pal", "shadow_quality", &Value::from("unknown")).is_err());
        assert_eq!(std::fs::read_to_string(&file).unwrap(), text);
        let _ = std::fs::remove_dir_all(&home);
    }

    #[test]
    fn play_takes_vulkan_and_the_bakes_only_when_on() {
        let data = Path::new("/data/rac1");
        assert_eq!(play_additions("", data, "pal"), None);
        assert_eq!(play_additions("enhanced_rendering = 0\n", data, "pal"), None);
        let (env, args) = play_additions("enhanced_rendering = 1\n", data, "pal").unwrap();
        assert_eq!(env, vec![("OPENRAC_GL_API".to_string(), "vulkan".to_string())]);
        // Joined, not spelled out: Windows joins with a backslash.
        let probes = data.join("probes").join("pal");
        assert_eq!(args, vec!["--probes".to_string(), probes.display().to_string()]);
    }

    #[test]
    fn written_values_read_back_as_the_port_parses_them() {
        assert_eq!(written(&Value::from(1.0)), "1");
        assert_eq!(written(&Value::from(1.25)), "1.25");
        assert_eq!(written(&Value::from(0.05)), "0.05");
        assert_eq!(written(&Value::Bool(false)), "0");
    }
}
