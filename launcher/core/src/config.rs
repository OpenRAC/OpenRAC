//! The launcher's own settings: where the OpenRAC checkout and the programs
//! it runs are. Stored as `launcher.json` in the per-user config folder (the
//! app decides where; see `src-tauri/src/lib.rs`), never in the checkout.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase", default)]
pub struct Config {
    /// The OpenRAC checkout (holds games/, tools/ and baserom/).
    pub root: Option<PathBuf>,
    /// Python 3.10+, for tools/openrac.py, the editor and the games' Python tools.
    pub python: Option<PathBuf>,
    /// Godot 4 (4.7+), for the level editor.
    pub godot: Option<PathBuf>,
    /// Docker, which rac1/pal's and rac4's builds run in.
    pub docker: Option<PathBuf>,
    /// Show what contributors use: builds, checks, toolchains, progress.
    /// Off, the launcher shows a player's three steps: set up from your disc, play, edit levels.
    pub developer: bool,
    /// The save editor shows every field of a save, raw bytes included (for debugging). Off, the
    /// player's safe set only.
    pub save_editor_advanced: bool,
    /// Where games are set up from the player's discs (OpenGOAL's install
    /// directory); unset: the launcher's per-user data folder.
    pub install_dir: Option<PathBuf>,
    /// The player's folder of PlayStation 2 disc images, looked through for the games OpenRAC
    /// knows (discs.rs): with it nobody picks an image by hand.
    #[serde(default)]
    pub discs_dir: Option<PathBuf>,
    /// The player's own folder for the games' memory cards (their saves): a folder per disc
    /// serial in it, as in the default `<data home>/memcard`, so one folder can be synced
    /// (Syncthing, a cloud drive). Unset: the default. Given to each game's start as `--cards`
    /// (saves.rs `card_dir_in`); not on Android, which keeps its files in its own place.
    pub save_dir: Option<PathBuf>,
    /// The first-run setup was finished (or skipped).
    pub setup_complete: bool,
    /// The version the launcher showed last (`rac1/pal`).
    pub last_version: Option<String>,
    /// Discord Rich Presence enabled (default true).
    pub discord_rpc: bool,
    /// Look for a new launcher when it starts (default true). Off, it looks only when asked
    /// (Settings, "Check now"); an update is never installed without the player's click.
    pub check_updates: bool,
    /// Look for newer versions of the installed mods when the launcher starts, once a day, and
    /// offer them (default true). Off, mods are checked only on the Mods tab's "Check now"; a mod
    /// is never updated without the player's click.
    pub check_mod_updates: bool,
    /// Which builds the launcher updates to (update::Channel): the releases, or the newest
    /// development build. Unset: the one it was built from (a release's build, the releases).
    pub update_channel: Option<crate::update::Channel>,
    /// A fork's launcher only: the branch whose development builds the development channel follows
    /// (update::UpdateSource::chosen). Unset: the one it was built from, or the default branch.
    pub update_branch: Option<String>,
    /// Custom Discord application ID (optional).
    pub discord_client_id: Option<String>,
    /// The page layout: "compact" (default) or "classic", the original design (src/layouts/).
    #[ts(type = "\"compact\" | \"classic\"")]
    pub layout: String,
    /// The launcher's theme (src/lib/palettes.json): "openrac" (default), "oled", "nord"
    /// or "mocha". The accent is not a theme's: it is the picked game's.
    pub theme: String,
    /// The picked game's own bar, gold and coal (palettes.json game_colors) instead of the
    /// launcher's, in the launcher and the game's menu. On by default; off, every game wears the gold.
    pub game_colors: bool,
    /// The game writes a detailed log (OPENRAC_LOG=verbose): level loads, controller changes,
    /// what the port is missing. Off by default, so a log stays small.
    pub detailed_game_log: bool,
    /// What the game draws with on Windows, Linux and macOS: "vulkan" (the default), the port's
    /// Vulkan renderer (OPENRAC_GL_API=vulkan; on macOS the MoltenVK its bundle carries),
    /// "opengl" (OpenGL 4.1), or, on macOS only, "metal", the port's Metal renderer
    /// (OPENRAC_GL_API=metal, port/renderer/metal). Where Vulkan or Metal cannot start, the game
    /// draws with OpenGL (port/platform/window.cpp); "metal" on any other system plays as
    /// "vulkan" (renderer_for). Not used on Android, which keeps its own choice
    /// (src-tauri/src/android.rs).
    #[ts(type = "\"opengl\" | \"vulkan\" | \"metal\"")]
    pub renderer: String,
    /// HDR output (port docs/port/HDR.md): "off" (default) or "on", given to each game's start
    /// (OPENRAC_HDR) over the game's own setting. On: HDR when the system has the display in HDR
    /// mode. On every system; it needs the Vulkan renderer. Anything else reads as "off" (load).
    #[ts(type = "\"off\" | \"on\"")]
    pub hdr: String,
    /// Linux: the launcher's own window draws without WebKitGTK's accelerated compositing mode
    /// (webkit.rs), for a graphics driver that crashes it. Turned on by the launcher itself after
    /// a start whose window crashed before it drew a click; the player may turn it off again.
    pub safe_rendering: bool,
    /// The Mods switch under Play on the game's page: the game loads the mods the Mods tab switched on (the
    /// gameplay mods, the packs and the packed levels). Off by default: Play starts the game with no
    /// mods at all (`--no-mods`, docs/port/MODS.md). It took the place of Settings' Purist mode.
    pub play_with_mods: bool,
    /// For each game, the time (seconds since 1970) of the newest crash report the player has
    /// dismissed: crashes up to then are seen, and no notice, button or mark is shown for them.
    pub crashes_seen: BTreeMap<String, u64>,
    /// For developers whose launcher knows more than one disc of a game: the version each
    /// game's page plays (`rac1` -> `rac1/ntsc`). Used only with developer tools on
    /// (disc_choice.rs); without one, the disc set up, as for a player.
    pub disc_choice: BTreeMap<String, String>,
    /// For developers: the image each version was last set up from (`rac1/ntsc` -> the .iso), so
    /// the disc switch offers that version without a games folder (disc_choice.rs). An image
    /// moved, deleted or now another disc is not offered.
    pub disc_images: BTreeMap<String, PathBuf>,
    /// The name online mods show the other players (multiplayer.rs): 3 to 16 letters, digits and
    /// underscores, made up at the first start and changed in Settings. Given to the game as
    /// `OPENRAC_PLAYER_NAME`.
    pub multiplayer_name: String,
    /// This installation's key (multiplayer.rs): random, made at the first start and never shown.
    /// What a host's ban holds on to, so a new name gets round none. Given to the game as
    /// `OPENRAC_IDENTITY_KEY`.
    #[serde(default)]
    pub multiplayer_key: String,
    /// Where the launcher keeps its own copy of the figures it fetched from bytematch.dev: in
    /// the per-user folder beside its settings, which the launcher can always write (a
    /// checkout it does not own, as an installed launcher carries one, is read-only).
    /// Not saved with the settings: the app sets it at every start.
    #[serde(skip)]
    pub progress_cache: Option<PathBuf>,
    /// The mod indexes to read (index.rs), in order; empty: the built-in list (`index::INDEXES`).
    pub mod_indexes: Vec<String>,
    /// Offer pre-release versions of mods. Unset: on for the dev channel, off for releases.
    pub prerelease_mods: Option<bool>,
    /// An install link kept across "Switch to the dev build": its change set opens after the
    /// restart (src/lib/deepLink.svelte.ts), then it is cleared.
    pub pending_deep_link: Option<String>,
    /// Update mods from the index by themselves (mod_updates.rs): patch and minor versions at
    /// start, every 6 hours and before each game start; majors wait for a click (default true).
    pub auto_update_mods: bool,
    /// Removing a mod removes the dependencies the launcher installed for it that nothing else
    /// needs (default true); the player's own are asked about.
    pub cleanup_unused_deps: bool,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            root: None,
            python: None,
            godot: None,
            docker: None,
            install_dir: None,
            discs_dir: None,
            save_dir: None,
            developer: false,
            save_editor_advanced: false,
            setup_complete: false,
            last_version: None,
            discord_rpc: true,
            check_updates: true,
            check_mod_updates: true,
            update_channel: None,
            update_branch: None,
            discord_client_id: None,
            layout: "compact".into(),
            theme: "openrac".into(),
            game_colors: true,
            detailed_game_log: false,
            renderer: "vulkan".into(),
            hdr: "off".into(),
            safe_rendering: false,
            play_with_mods: false,
            crashes_seen: BTreeMap::new(),
            disc_choice: BTreeMap::new(),
            disc_images: BTreeMap::new(),
            multiplayer_name: String::new(),
            multiplayer_key: String::new(),
            progress_cache: None,
            mod_indexes: Vec::new(),
            prerelease_mods: None,
            pending_deep_link: None,
            auto_update_mods: true,
            cleanup_unused_deps: true,
        }
    }
}

impl Config {
    pub fn root(&self) -> Result<&Path, String> {
        self.root.as_deref().ok_or_else(|| "the OpenRAC folder is not set (Settings)".to_string())
    }
}

/// Reads `path`; a missing or unreadable file gives the defaults, so a
/// broken settings file never stops the launcher from starting.
pub fn load(path: &Path) -> Config {
    let mut config: Config =
        std::fs::read_to_string(path).ok().and_then(|text| serde_json::from_str(&text).ok()).unwrap_or_default();
    config.hdr = hdr_mode(&config.hdr).into();
    config
}

/// The HDR setting as "off" or "on": anything but "on" is "off", the game's default.
pub fn hdr_mode(value: &str) -> &'static str {
    match value {
        "on" => "on",
        _ => "off",
    }
}

/// The renderer Play starts the game with on `platform`, from Settings' `renderer`: "vulkan",
/// "opengl", or "metal" on macOS only. Metal anywhere else (a settings file carried over from a
/// Mac) is Vulkan, the default; anything unknown is OpenGL, as it always was.
pub fn renderer_for(renderer: &str, platform: crate::actions::Platform) -> &'static str {
    match renderer {
        "vulkan" => "vulkan",
        "metal" if platform == crate::actions::Platform::Macos => "metal",
        "metal" => "vulkan",
        _ => "opengl",
    }
}

/// Writes `config` to `path` through a temporary file, so a crash never
/// leaves half a file behind.
pub fn save(path: &Path, config: &Config) -> Result<(), String> {
    if let Some(dir) = path.parent() {
        std::fs::create_dir_all(dir).map_err(|e| format!("cannot create {}: {e}", dir.display()))?;
    }
    let text = serde_json::to_string_pretty(config).map_err(|e| e.to_string())?;
    let tmp = path.with_extension("json.tmp");
    if let Err(e) = std::fs::write(&tmp, text) {
        let _ = std::fs::remove_file(&tmp);
        return Err(format!("cannot write {}: {e}", tmp.display()));
    }
    if let Err(e) = std::fs::rename(&tmp, path) {
        let _ = std::fs::remove_file(&tmp);
        return Err(format!("cannot write {}: {e}", path.display()));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trips_and_tolerates_a_broken_file() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("sub").join("launcher.json");
        assert_eq!(load(&path), Config::default());
        let config = Config {
            root: Some("/somewhere/OpenRAC".into()),
            setup_complete: true,
            last_version: Some("rac1/pal".into()),
            ..Config::default()
        };
        save(&path, &config).unwrap();
        assert_eq!(load(&path), config);
        let text = std::fs::read_to_string(&path).unwrap();
        assert!(text.contains("\"setupComplete\": true"), "{text}");
        std::fs::write(&path, "{ not json").unwrap();
        assert_eq!(load(&path), Config::default());
    }

    /// A settings file from before the option looks for updates, and turning it off is kept.
    #[test]
    fn checks_for_updates_unless_turned_off() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert!(load(&path).check_updates);
        save(&path, &Config { check_updates: false, ..Config::default() }).unwrap();
        assert!(!load(&path).check_updates);
        assert!(std::fs::read_to_string(&path).unwrap().contains("\"checkUpdates\": false"));
    }

    /// A settings file without the option wears the game's colours; turning them off is kept.
    #[test]
    fn wears_game_colours_unless_turned_off() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert!(load(&path).game_colors);
        save(&path, &Config { game_colors: false, ..Config::default() }).unwrap();
        assert!(!load(&path).game_colors);
    }

    /// A settings file from before the mod check offers mod updates; turning it off is kept.
    #[test]
    fn checks_for_mod_updates_unless_turned_off() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert!(load(&path).check_mod_updates);
        save(&path, &Config { check_mod_updates: false, ..Config::default() }).unwrap();
        assert!(!load(&path).check_mod_updates);
        assert!(std::fs::read_to_string(&path).unwrap().contains("\"checkModUpdates\": false"));
    }

    /// A settings file from before the mod hub updates mods and cleans up after them.
    #[test]
    fn mods_update_and_clean_up_by_default() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        let c = load(&path);
        assert!(c.auto_update_mods && c.cleanup_unused_deps);
    }

    /// A settings file from before the renderer choice draws with OpenGL; Vulkan, chosen, is kept.
    #[test]
    fn renderer_defaults_to_vulkan() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert_eq!(load(&path).renderer, "vulkan");
        save(&path, &Config { renderer: "opengl".into(), ..Config::default() }).unwrap();
        assert_eq!(load(&path).renderer, "opengl");
        save(&path, &Config { renderer: "metal".into(), ..Config::default() }).unwrap();
        assert_eq!(load(&path).renderer, "metal");
    }

    /// Metal only on macOS: a Mac's "metal" plays with Vulkan on any other system.
    #[test]
    fn metal_is_for_macos_only() {
        use crate::actions::Platform;
        assert_eq!(renderer_for("metal", Platform::Macos), "metal");
        for platform in [Platform::Windows, Platform::Linux, Platform::Android, Platform::Ios] {
            assert_eq!(renderer_for("metal", platform), "vulkan", "{platform:?}");
        }
        for platform in [Platform::Macos, Platform::Windows, Platform::Linux] {
            assert_eq!(renderer_for("vulkan", platform), "vulkan");
            assert_eq!(renderer_for("opengl", platform), "opengl");
            assert_eq!(renderer_for("glide", platform), "opengl");
        }
    }

    /// HDR is Off or On: anything else reads as Off.
    #[test]
    fn hdr_is_off_or_on() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert_eq!(load(&path).hdr, "off");
        for (old, now) in [("off", "off"), ("on", "on"), ("bright", "off")] {
            std::fs::write(&path, format!(r#"{{"setupComplete": true, "hdr": "{old}"}}"#)).unwrap();
            assert_eq!(load(&path).hdr, now, "{old}");
        }
    }
}
