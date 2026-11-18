//! The shared launcher's filesystem API in the native iOS app. UIKit owns import and play;
//! catalogue, settings, controls, saves and reports keep using the desktop core.
mod save_transfer;

use openrac_launcher_core::{
    catalog, config, controls, differences, discs, install, library, overrides, plugins, reports, saves,
};
use serde_json::{json, Value};
use std::ffi::{c_char, CStr, CString};
use std::path::{Path, PathBuf};
use std::sync::{Mutex, OnceLock};

#[derive(Clone, serde::Deserialize)]
#[serde(rename_all = "camelCase")]
struct Engine {
    key: String,
    ready_file: String,
}

#[derive(serde::Deserialize)]
#[serde(rename_all = "camelCase")]
struct Bundle {
    version: String,
    #[serde(default)]
    store_url: String,
    engines: Vec<Engine>,
}

struct Runtime {
    config: config::Config,
    config_file: PathBuf,
    home: PathBuf,
    bundle: Bundle,
}
static RUNTIME: OnceLock<Mutex<Runtime>> = OnceLock::new();

fn value<T: serde::Serialize>(data: T) -> Result<Value, String> {
    serde_json::to_value(data).map_err(|e| e.to_string())
}
fn text<'a>(args: &'a Value, name: &str) -> Result<&'a str, String> {
    args[name].as_str().ok_or_else(|| format!("Missing {name}"))
}
fn data(runtime: &Runtime, game: &str) -> Result<PathBuf, String> {
    if !game.chars().all(|c| c.is_ascii_alphanumeric() || c == '-') || game.is_empty() {
        return Err("Invalid game identifier".into());
    }
    Ok(install::data_dir(runtime.config.install_dir.as_deref().ok_or("No install folder")?, game))
}
fn read_library(runtime: &Runtime) -> Result<Value, String> {
    let mut library = library::library(&runtime.config)?;
    library.platform = openrac_launcher_core::actions::Platform::Ios;
    library.toolchains = false;
    for action in &mut library.repository {
        action.this_platform = false;
        action.runnable = false;
    }
    for game in &mut library.games {
        for version in &mut game.versions {
            let engine = runtime.bundle.engines.iter().find(|e| e.key == version.version.key);
            let supported = engine.is_some();
            let folder = data(runtime, &game.id)?;
            let ready = engine.is_some_and(|e| folder.join(&e.ready_file).is_file());
            for action in &mut version.actions {
                action.this_platform =
                    supported && matches!(action.id.as_str(), "play" | "port-levels" | "port-levels-check");
                if !action.this_platform {
                    action.runnable = false;
                    continue;
                }
                if action.id == "play" {
                    action.done = true; // The signed engine exists in this app, not in a downloaded build folder.
                    action.missing_artifact = false;
                    action.runnable = ready;
                    action.blockers = if ready { vec![] } else { vec!["Import your own disc first".into()] };
                } else if matches!(action.id.as_str(), "port-levels" | "port-levels-check") {
                    action.done = ready;
                    action.runnable = false;
                }
            }
            version.port_outdated = false;
        }
    }
    value(library)
}

fn command(runtime: &mut Runtime, name: &str, args: &Value) -> Result<Value, String> {
    match name {
        "app_info" => {
            let update = json!({"repository":"OpenRAC/OpenRAC","fork":false,"channel":null,"manifest":"","page":runtime.bundle.store_url});
            Ok(json!({"version":runtime.bundle.version,"platform":"ios","configFile":runtime.config_file,
                "standalone":true,"updates":update,"updateChannels":{"stable":update,"dev":update},
                "noCompositing":false,"safeRenderingAfterCrash":false,"capabilities":{"mods":false,"saveEditing":false,"saveTransfer":true,"openFolders":false,"folderScan":false,"updates":false}}))
        }
        "get_config" | "adopt_godot" => value(&runtime.config),
        "save_config" => {
            let mut changed: config::Config =
                serde_json::from_value(args["config"].clone()).map_err(|e| e.to_string())?;
            changed.root = runtime.config.root.clone();
            changed.install_dir = runtime.config.install_dir.clone();
            changed.developer = false;
            changed.discord_rpc = false;
            changed.check_updates = false;
            changed.check_mod_updates = false;
            changed.auto_update_mods = false;
            config::save(&runtime.config_file, &changed)?;
            runtime.config = changed;
            value(&runtime.config)
        }
        "library" | "sync_progress_from_web" => read_library(runtime),
        "detect" => Ok(json!({"roots":[],"pythons":[],"godots":[],"dockers":[]})),
        "scan_discs" => {
            let found = if let Some(folder) = &runtime.config.discs_dir {
                discs::scan(folder, &catalog::load(runtime.config.root()?)?)
            } else {
                vec![]
            };
            Ok(json!({"found":found,"choices":[]}))
        }
        "startup_game" | "launcher_update_check" | "modset_take_link" => Ok(Value::Null),
        "tools_ready" | "background_status" | "discord_status" | "webview_clicked" | "webview_drew" => Ok(Value::Null),
        "reports_list" => value(reports::list(&data(runtime, text(args, "game")?)?)),
        "latest_crash" => {
            value(reports::latest_crash(&data(runtime, text(args, "game")?)?, args["since"].as_u64().unwrap_or(0)))
        }
        "report_text" => value(reports::copy_text(
            &data(runtime, text(args, "game")?)?,
            text(args, "file")?,
            &runtime.bundle.version,
        )?),
        "export_saves" | "preview_save_import" | "import_saves" | "restore_backup" => {
            save_transfer::command(runtime, name, args)
        }
        "get_saves" | "inspect_saves" | "saves_status" => value(saves::inspect_saves(text(args, "serial")?)),
        "controls_view" => {
            value(controls::view(&runtime.home, args["game"].as_str(), args["player"].as_u64().map(|n| n as u8))?)
        }
        "controls_set" => {
            let scope = serde_json::from_value(args["scope"].clone()).map_err(|e| e.to_string())?;
            value(controls::set(&runtime.home, &scope, text(args, "key")?, args["value"].as_str())?)
        }
        "controls_reset" => {
            let scope = serde_json::from_value(args["scope"].clone()).map_err(|e| e.to_string())?;
            value(controls::reset(&runtime.home, &scope)?)
        }
        "controls_reset_players" => value(controls::reset_players(&runtime.home)?),
        "controls_reset_player" => {
            let scope = serde_json::from_value(args["scope"].clone()).map_err(|e| e.to_string())?;
            let player = args["player"].as_u64().filter(|n| *n <= 1).ok_or("Invalid player")? as u8;
            value(controls::reset_player(&runtime.home, &scope, player)?)
        }
        "controls_copy" => value(controls::copy_game(&runtime.home, text(args, "from")?, text(args, "to")?)?),
        "backup_saves" => value(saves::backup_saves(text(args, "serial")?, args["note"].as_str())?),
        "set_game_fix" => value(differences::set_fix(
            runtime.config.root.as_deref(),
            &runtime.home,
            text(args, "game")?,
            text(args, "version")?,
            text(args, "id")?,
            args["on"].as_bool().ok_or("Missing on")?,
        )?),
        "differences" => value(differences::view(
            runtime.config.root.as_deref(),
            &runtime.home,
            text(args, "game")?,
            text(args, "version")?,
        )?),
        "plugins" | "plugins_list" => value(plugins::list(
            &plugins::dir(runtime.config.install_dir.as_deref().unwrap(), text(args, "game")?),
            text(args, "game")?,
            text(args, "version")?,
        )?),
        "packs" | "packs_list" => value(overrides::list(&overrides::dir(
            runtime.config.install_dir.as_deref().unwrap(),
            text(args, "game")?,
        ))?),
        "enhanced_view" => Ok(Value::Null),
        _ => Err(format!("{name} is not available in this iOS build")),
    }
}

/// Called once with paths supplied by the native application, never from page content.
///
/// # Safety
/// All arguments must point to valid NUL-terminated strings for this call.
#[no_mangle]
pub unsafe extern "C" fn openrac_ios_launcher_init(
    root: *const c_char,
    documents: *const c_char,
    engines: *const c_char,
) {
    let root = PathBuf::from(CStr::from_ptr(root).to_string_lossy().as_ref());
    let documents = PathBuf::from(CStr::from_ptr(documents).to_string_lossy().as_ref());
    let home = documents.parent().unwrap_or(Path::new(".")).join("Library/Application Support/OpenRAC");
    let config_file = home.join("launcher.json");
    let mut config = config::load(&config_file);
    config.root = Some(root);
    config.install_dir = Some(documents.join("OpenRAC"));
    config.setup_complete = true;
    config.developer = false;
    config.discord_rpc = false;
    config.check_updates = false;
    config.check_mod_updates = false;
    config.auto_update_mods = false;
    let bundle: Bundle = serde_json::from_str(&CStr::from_ptr(engines).to_string_lossy()).unwrap_or_else(|_| Bundle {
        version: env!("CARGO_PKG_VERSION").into(),
        store_url: String::new(),
        engines: vec![],
    });
    if config.last_version.is_none() {
        config.last_version = bundle.engines.first().map(|e| e.key.clone());
    }
    let _ = RUNTIME.set(Mutex::new(Runtime { config, config_file, home, bundle }));
}

/// # Safety
/// Both arguments must point to valid NUL-terminated strings. Free the reply with the paired function.
#[no_mangle]
pub unsafe extern "C" fn openrac_ios_launcher_command(name: *const c_char, args: *const c_char) -> *mut c_char {
    let result = std::panic::catch_unwind(|| {
        let name = CStr::from_ptr(name).to_string_lossy();
        let args: Value = serde_json::from_str(&CStr::from_ptr(args).to_string_lossy()).map_err(|e| e.to_string())?;
        let mut runtime =
            RUNTIME.get().ok_or("Launcher not initialized")?.lock().map_err(|_| "Launcher state unavailable")?;
        command(&mut runtime, &name, &args)
    })
    .unwrap_or_else(|_| Err("Launcher operation failed".into()));
    let response = match result {
        Ok(value) => json!({"ok":true,"value":value}),
        Err(error) => json!({"ok":false,"error":error}),
    };
    CString::new(response.to_string()).unwrap().into_raw()
}

/// # Safety
/// The pointer must be null or an unreleased reply from `openrac_ios_launcher_command`.
#[no_mangle]
pub unsafe extern "C" fn openrac_ios_launcher_free(value: *mut c_char) {
    if !value.is_null() {
        drop(CString::from_raw(value));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn runtime() -> Runtime {
        // Keep the isolated test output recoverable, like on-device imports.
        let home = tempfile::tempdir().unwrap().keep();
        let config = config::Config {
            root: Some(PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../..")),
            install_dir: Some(home.join("games")),
            ..config::Config::default()
        };
        Runtime {
            config,
            config_file: home.join("launcher.json"),
            home,
            bundle: Bundle {
                version: env!("CARGO_PKG_VERSION").into(),
                store_url: String::new(),
                engines: vec![Engine {
                    key: "rac1/ntsc".into(),
                    ready_file: "port/ntsc/level_00/manifest.json".into(),
                }],
            },
        }
    }

    #[test]
    fn catalogue_keeps_future_games_but_only_runs_an_included_prepared_engine() {
        let runtime = runtime();
        let library = read_library(&runtime).unwrap();
        assert_eq!(library["games"].as_array().unwrap().len(), 6);
        assert_eq!(library["platform"], "ios");
        for game in library["games"].as_array().unwrap() {
            for version in game["versions"].as_array().unwrap() {
                assert!(!version["actions"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .any(|a| a["id"] == "play" && a["runnable"] == true));
                if version["key"] != "rac1/ntsc" {
                    assert!(version["actions"]
                        .as_array()
                        .unwrap()
                        .iter()
                        .all(|a| a["thisPlatform"] == false && a["runnable"] == false));
                }
            }
        }
        let levels = data(&runtime, "rac1").unwrap().join("port/ntsc/level_00");
        std::fs::create_dir_all(&levels).unwrap();
        std::fs::write(levels.join("manifest.json"), "{}").unwrap();
        let library = read_library(&runtime).unwrap();
        let game = library["games"].as_array().unwrap().iter().find(|g| g["id"] == "rac1").unwrap();
        let version = game["versions"].as_array().unwrap().iter().find(|v| v["key"] == "rac1/ntsc").unwrap();
        assert!(version["actions"]
            .as_array()
            .unwrap()
            .iter()
            .any(|a| a["id"] == "play" && a["thisPlatform"] == true && a["runnable"] == true && a["done"] == true));
    }

    #[test]
    fn config_cannot_redirect_the_signed_tools_or_install_folder() {
        let mut runtime = runtime();
        let root = runtime.config.root.clone();
        let install = runtime.config.install_dir.clone();
        let mut changed = serde_json::to_value(&runtime.config).unwrap();
        changed["root"] = json!("/untrusted");
        changed["installDir"] = json!("/outside");
        command(&mut runtime, "save_config", &json!({"config": changed})).unwrap();
        assert_eq!(runtime.config.root, root);
        assert_eq!(runtime.config.install_dir, install);
        assert!(data(&runtime, "../outside").is_err());
        assert!(command(&mut runtime, "unknown_command", &json!({})).is_err());
    }

    #[test]
    fn saves_round_trip_between_ios_and_desktop_with_preview_and_backup() {
        use openrac_launcher_core::save_transfer as transfer;
        let mut ios = runtime();
        let serial = "SCES_509.16";
        let folder = "BESCES-50916RATCHET";
        let card = ios.home.join("memcard").join(serial);
        let save_dir = card.join(folder);
        std::fs::create_dir_all(&save_dir).unwrap();
        let mut bytes = vec![0u8; 16];
        for (record, number) in [(0i32, 3u32), (1, 1234)] {
            bytes.extend_from_slice(&record.to_le_bytes());
            bytes.extend_from_slice(&4i32.to_le_bytes());
            bytes.extend_from_slice(&number.to_le_bytes());
        }
        bytes.extend_from_slice(&4i32.to_le_bytes());
        bytes.extend_from_slice(&8i32.to_le_bytes());
        bytes.extend_from_slice(&[0x00, 0x14, 0x14, 0x23, 0x00, 0x10, 0x10, 0x26]);
        bytes.extend_from_slice(&(-1i32).to_le_bytes());
        std::fs::write(save_dir.join("save0.bin"), &bytes).unwrap();
        let exports = ios.home.join("save-transfers/exports/test");
        let imports = ios.home.join("save-transfers/imports/test");
        std::fs::create_dir_all(&exports).unwrap();
        std::fs::create_dir_all(&imports).unwrap();
        let exported = command(
            &mut ios,
            "export_saves",
            &json!({
                "serial":serial,"slot":null,"directory":exports
            }),
        )
        .unwrap();
        let archive = PathBuf::from(exported["saved"].as_str().unwrap());
        assert_eq!(exported["slots"].as_array().unwrap().len(), 1);
        // The iOS file is read and imported by the desktop's unmodified core.
        let pc = ios.home.join("pc-card");
        let preview = transfer::preview_into(&pc, serial, &archive).unwrap();
        assert_eq!(preview.slots[0].slot.bolts, Some(1234));
        let choice = transfer::SaveImportChoice { replace_all: true, slots: vec![] };
        transfer::import_into(&pc, serial, &archive, &choice).unwrap();
        assert_eq!(std::fs::read(pc.join(folder).join("save0.bin")).unwrap(), bytes);
        // Export on PC, pick a private snapshot on iOS, preview without changing the card.
        let incoming = imports.join("saves.zip");
        transfer::export_card(&pc, serial, Some(0), "desktop-test", &incoming).unwrap();
        let mut old = bytes.clone();
        old[36..40].copy_from_slice(&999u32.to_le_bytes());
        std::fs::write(save_dir.join("save0.bin"), &old).unwrap();
        let preview = command(&mut ios, "preview_save_import", &json!({"serial":serial,"source":incoming})).unwrap();
        assert_eq!(preview["launcherVersion"], "desktop-test");
        assert_eq!(preview["cardHasSaves"], true);
        assert_eq!(preview["slots"][0]["replaces"]["bolts"], 999);
        assert_eq!(std::fs::read(save_dir.join("save0.bin")).unwrap(), old);
        let imported = command(
            &mut ios,
            "import_saves",
            &json!({"serial":serial,"source":incoming,
            "choice":{"replaceAll":false,"slots":[{"folder":folder,"slotIndex":0}]}}),
        )
        .unwrap();
        assert_eq!(std::fs::read(save_dir.join("save0.bin")).unwrap(), bytes);
        let backup = PathBuf::from(imported["backup"]["path"].as_str().unwrap());
        assert_eq!(std::fs::read(backup.join(folder).join("save0.bin")).unwrap(), old);
        assert!(command(&mut ios, "preview_save_import", &json!({"serial":"SCUS_971.99","source":incoming})).is_err());
        assert!(command(&mut ios, "preview_save_import", &json!({"serial":serial,"source":archive})).is_err());
        assert!(command(&mut ios, "export_saves", &json!({"serial":"../outside","directory":exports})).is_err());
        let outside = ios.home.clone();
        assert!(command(&mut ios, "export_saves", &json!({"serial":serial,"directory":outside})).is_err());
    }
}
