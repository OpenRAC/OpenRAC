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
    /// Up Your Arsenal (rac3) is still in development: its build, set-up and Play stay blocked
    /// until a developer turns this on, after the launcher's three confirmations (Settings,
    /// Developer). Default off.
    pub rac3_dev: bool,
    /// The save editor shows every field of a save, raw bytes included (for debugging). Off, the
    /// player's safe set only.
    pub save_editor_advanced: bool,
    /// For developers: after a change to OpenRAC, build the game again incrementally (`port-build`)
    /// instead of from scratch (`port-rebuild`). A build folder that is unusable (no stamp from
    /// OpenRAC's build, another OpenRAC version) is still built from scratch. Default off.
    pub incremental_builds: bool,
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
    /// Vulkan renderer, shown as Metal on macOS because the game runs it through MoltenVK,
    /// or "opengl" (OpenGL 4.1). Android keeps its own renderer setting. iOS uses Metal
    /// through MoltenVK.
    #[serde(deserialize_with = "deserialize_renderer")]
    #[ts(type = "\"opengl\" | \"vulkan\"")]
    pub renderer: String,
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
    /// The multiplayer hub every game's online play starts at (multiplayer.rs): a host name or an
    /// IP address, and a port. The hub logs players in and lists the game servers to pick from.
    /// The official hub (`hub.openrac.dev:7177`) unless changed in Settings; anyone can host one
    /// (the OpenRAC/Server repository). Given to the game as `OPENRAC_SERVER=host:port`.
    pub multiplayer_server_host: String,
    /// The server's port, 1 to 65535; a saved value out of range reads as the official port.
    #[serde(deserialize_with = "deserialize_port")]
    pub multiplayer_server_port: u16,
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
            rac3_dev: false,
            save_editor_advanced: false,
            incremental_builds: false,
            setup_complete: false,
            last_version: None,
            discord_rpc: true,
            check_updates: true,
            check_mod_updates: true,
            update_channel: None,
            update_branch: None,
            discord_client_id: None,
            theme: "openrac".into(),
            game_colors: true,
            detailed_game_log: false,
            renderer: "vulkan".into(),
            safe_rendering: false,
            play_with_mods: false,
            crashes_seen: BTreeMap::new(),
            disc_choice: BTreeMap::new(),
            disc_images: BTreeMap::new(),
            multiplayer_name: String::new(),
            multiplayer_key: String::new(),
            multiplayer_server_host: crate::multiplayer::OFFICIAL_HOST.into(),
            multiplayer_server_port: crate::multiplayer::OFFICIAL_PORT,
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
/// broken settings file never stops the launcher from starting. A multiplayer server saved as the
/// official game server from before the hub (every launcher saved its default) reads as the
/// official hub.
pub fn load(path: &Path) -> Config {
    let mut config: Config =
        std::fs::read_to_string(path).ok().and_then(|text| serde_json::from_str(&text).ok()).unwrap_or_default();
    if config.multiplayer_server_host.trim().eq_ignore_ascii_case(crate::multiplayer::FORMER_OFFICIAL_HOST)
        && config.multiplayer_server_port == crate::multiplayer::FORMER_OFFICIAL_PORT
    {
        config.multiplayer_server_host = crate::multiplayer::OFFICIAL_HOST.into();
        config.multiplayer_server_port = crate::multiplayer::OFFICIAL_PORT;
    }
    config
}

/// Normalizes saved renderer names and names passed to Play.
pub fn renderer_for(renderer: &str) -> &'static str {
    match renderer {
        "vulkan" | "metal" => "vulkan",
        _ => "opengl",
    }
}

fn deserialize_renderer<'de, D: serde::Deserializer<'de>>(deserializer: D) -> Result<String, D::Error> {
    Ok(renderer_for(&String::deserialize(deserializer)?).to_owned())
}

/// A port as saved: 1 to 65535, anything else (a hand edit, a string) the official port, so that
/// a bad value never costs the player the rest of the settings.
fn deserialize_port<'de, D: serde::Deserializer<'de>>(deserializer: D) -> Result<u16, D::Error> {
    let value = serde_json::Value::deserialize(deserializer)?;
    Ok(value
        .as_u64()
        .and_then(|n| u16::try_from(n).ok())
        .filter(|&n| n != 0)
        .unwrap_or(crate::multiplayer::OFFICIAL_PORT))
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

    /// A settings file from before the multiplayer server has the official hub, as has one that
    /// names the official game server from before the hub; another is kept, and a port out of
    /// range reads as the official port without losing the rest.
    #[test]
    fn the_multiplayer_server_defaults_to_the_official_one_and_round_trips() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true, "multiplayerName": "SwiftRobot42"}"#).unwrap();
        let old = load(&path);
        assert_eq!((old.multiplayer_server_host.as_str(), old.multiplayer_server_port), ("hub.openrac.dev", 7177));
        assert_eq!(old.multiplayer_name, "SwiftRobot42");
        std::fs::write(
            &path,
            r#"{"setupComplete": true, "multiplayerServerHost": "game.openrac.dev", "multiplayerServerPort": 7770}"#,
        )
        .unwrap();
        let before_hub = load(&path);
        assert_eq!(
            (before_hub.multiplayer_server_host.as_str(), before_hub.multiplayer_server_port),
            ("hub.openrac.dev", 7177)
        );
        std::fs::write(
            &path,
            r#"{"setupComplete": true, "multiplayerServerHost": "game.openrac.dev", "multiplayerServerPort": 7000}"#,
        )
        .unwrap();
        assert_eq!(load(&path).multiplayer_server_port, 7000);
        let mine = Config {
            multiplayer_server_host: "192.168.1.20".into(),
            multiplayer_server_port: 7000,
            ..Config::default()
        };
        save(&path, &mine).unwrap();
        assert_eq!(load(&path), mine);
        let text = std::fs::read_to_string(&path).unwrap();
        assert!(text.contains("\"multiplayerServerHost\": \"192.168.1.20\""), "{text}");
        assert!(text.contains("\"multiplayerServerPort\": 7000"), "{text}");
        for bad in ["0", "70000", "-1", "\"7177\"", "null"] {
            let json = format!(r#"{{"setupComplete": true, "multiplayerServerPort": {bad}}}"#);
            std::fs::write(&path, json).unwrap();
            let read = load(&path);
            assert!(read.setup_complete, "{bad}");
            assert_eq!(read.multiplayer_server_port, 7177, "{bad}");
        }
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

    /// Files without a renderer use the default. Old Metal selections migrate to Vulkan.
    #[test]
    fn renderer_defaults_to_vulkan() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true}"#).unwrap();
        assert_eq!(load(&path).renderer, "vulkan");
        save(&path, &Config { renderer: "opengl".into(), ..Config::default() }).unwrap();
        assert_eq!(load(&path).renderer, "opengl");
        std::fs::write(&path, r#"{"renderer": "metal"}"#).unwrap();
        let migrated = load(&path);
        assert_eq!(migrated.renderer, "vulkan");
        save(&path, &migrated).unwrap();
        assert!(std::fs::read_to_string(&path).unwrap().contains("\"renderer\": \"vulkan\""));
    }

    #[test]
    fn renderer_names_are_normalized() {
        assert_eq!(renderer_for("metal"), "vulkan");
        assert_eq!(renderer_for("vulkan"), "vulkan");
        assert_eq!(renderer_for("opengl"), "opengl");
        assert_eq!(renderer_for("glide"), "opengl");
    }

    /// An old file with the removed "hdr" key still loads; HDR is the game's own setting now.
    #[test]
    fn old_hdr_key_still_loads() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("launcher.json");
        std::fs::write(&path, r#"{"setupComplete": true, "hdr": "on"}"#).unwrap();
        assert!(load(&path).setup_complete);
    }
}
