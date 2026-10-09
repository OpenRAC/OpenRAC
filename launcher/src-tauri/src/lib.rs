// OpenRAC Launcher: the desktop app. Every file and process operation lives
// in openrac-launcher-core (../core); this file only exposes it to the UI
// (../src) as Tauri commands and forwards job output as events. The list of
// commands and events is docs/ARCHITECTURE.md's; keep them in step with
// src/lib/api.ts and src/lib/mock.ts.
use std::path::{Path, PathBuf};
use std::sync::Mutex;

use openrac_launcher_core::actions::{Platform, Scope};
use openrac_launcher_core::config::{self, Config};
use openrac_launcher_core::detect::{self, Check, Detected, Tool};
use openrac_launcher_core::jobs::{Event, Jobs, Started};
use openrac_launcher_core::library::{self, Library};
use serde::Serialize;
use tauri::{AppHandle, Emitter, Manager, State};
use tauri_plugin_opener::OpenerExt;

struct AppState {
    config: Mutex<Config>,
    config_file: Option<PathBuf>,
    jobs: Jobs,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct AppInfo {
    version: String,
    platform: Platform,
    /// Where launcher.json is.
    config_file: Option<PathBuf>,
}

#[tauri::command]
fn app_info(app: AppHandle, state: State<'_, AppState>) -> AppInfo {
    AppInfo {
        version: app.package_info().version.to_string(),
        platform: Platform::current(),
        config_file: state.config_file.clone(),
    }
}

#[tauri::command]
fn get_config(state: State<'_, AppState>) -> Config {
    state.config.lock().unwrap().clone()
}

#[tauri::command]
fn save_config(state: State<'_, AppState>, config: Config) -> Result<Config, String> {
    let file = state.config_file.as_deref().ok_or("no per-user config folder")?;
    config::save(file, &config)?;
    *state.config.lock().unwrap() = config.clone();
    Ok(config)
}

/// Where to look for a checkout: next to the launcher and the folder it was started from.
fn start_folders() -> Vec<PathBuf> {
    let exe = std::env::current_exe().ok().and_then(|p| p.parent().map(Path::to_path_buf));
    exe.into_iter().chain(std::env::current_dir().ok()).collect()
}

#[tauri::command]
async fn detect() -> Detected {
    tauri::async_runtime::spawn_blocking(|| detect::detect_all(&start_folders())).await.unwrap_or_default()
}

#[tauri::command]
async fn check_root(path: PathBuf) -> Check {
    detect::check_root(&path)
}

#[tauri::command]
async fn check_tool(tool: Tool, path: PathBuf) -> Result<Check, String> {
    tauri::async_runtime::spawn_blocking(move || detect::check_tool(tool, &path)).await.map_err(|e| e.to_string())
}

#[tauri::command]
async fn library(state: State<'_, AppState>) -> Result<Library, String> {
    let config = state.config.lock().unwrap().clone();
    tauri::async_runtime::spawn_blocking(move || library::library(&config)).await.map_err(|e| e.to_string())?
}

/// Runs action `id` of `scope`: a job whose output streams as `job-output`
/// and `job-exit` events, or a detached launch (id 0).
#[tauri::command]
fn run_action(app: AppHandle, state: State<'_, AppState>, scope: Scope, id: String) -> Result<Started, String> {
    let config = state.config.lock().unwrap().clone();
    let plan = library::plan(&config, &scope, &id)?;
    state.jobs.start(&plan, config.root.as_deref(), move |event| {
        let _ = match &event {
            Event::Output { .. } => app.emit("job-output", &event),
            Event::Exit { .. } => app.emit("job-exit", &event),
        };
    })
}

#[tauri::command]
fn cancel_job(state: State<'_, AppState>, id: u32) -> Result<(), String> {
    state.jobs.cancel(id)
}

/// Opens a file or folder of the checkout (a README, baserom/) with the
/// system's default app. Paths outside the checkout are refused.
#[tauri::command]
fn open_path(app: AppHandle, state: State<'_, AppState>, path: String) -> Result<(), String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;
    let root = std::fs::canonicalize(&root).map_err(|e| e.to_string())?;
    let rel = path.split('#').next().unwrap_or_default();
    let target = std::fs::canonicalize(openrac_launcher_core::under(&root, rel)).map_err(|e| format!("{path}: {e}"))?;
    if !target.starts_with(&root) {
        return Err(format!("{path} is outside the OpenRAC folder"));
    }
    app.opener().open_path(target.to_string_lossy(), None::<&str>).map_err(|e| e.to_string())
}

/// Opens a web page in the browser: https only.
#[tauri::command]
fn open_url(app: AppHandle, url: String) -> Result<(), String> {
    if !url.starts_with("https://") {
        return Err(format!("not an https address: {url}"));
    }
    app.opener().open_url(url, None::<&str>).map_err(|e| e.to_string())
}

#[tauri::command]
fn inspect_iso(
    state: State<'_, AppState>,
    target_key: String,
    iso_path: PathBuf,
) -> Result<openrac_launcher_core::iso::IsoInspection, String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;
    let catalog = openrac_launcher_core::catalog::load(&root)?;
    Ok(openrac_launcher_core::iso::inspect_iso(&catalog, &target_key, &iso_path))
}

#[tauri::command]
fn import_iso(
    state: State<'_, AppState>,
    target_key: String,
    iso_path: PathBuf,
) -> Result<openrac_launcher_core::iso::ImportResult, String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;
    let catalog = openrac_launcher_core::catalog::load(&root)?;
    openrac_launcher_core::iso::import_iso(&root, &catalog, &target_key, &iso_path)
}

#[tauri::command]
fn sync_progress_from_web(
    state: State<'_, AppState>,
) -> Result<openrac_launcher_core::catalog::Catalog, String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;

    // Attempt to fetch latest numbers from openrac.dev using curl (available across all Linux distros)
    let output = std::process::Command::new("curl")
        .arg("-s")
        .arg("--connect-timeout")
        .arg("4")
        .arg("https://openrac.dev/progress.json")
        .output();

    if let Ok(out) = output {
        if out.status.success() {
            if let Ok(json_str) = String::from_utf8(out.stdout) {
                if let Ok(catalog) = openrac_launcher_core::catalog::update_progress_from_openrac_dev(&root, &json_str) {
                    return Ok(catalog);
                }
            }
        }
    }

    // Offline fallback: load from cached summary.json seamlessly
    openrac_launcher_core::catalog::load(&root)
}

#[tauri::command]
fn apply_progress_json(
    state: State<'_, AppState>,
    json: String,
) -> Result<openrac_launcher_core::catalog::Catalog, String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;
    openrac_launcher_core::catalog::update_progress_from_openrac_dev(&root, &json)
}

#[cfg(target_os = "linux")]
fn ensure_linux_desktop_integration() {
    let icon_bytes = include_bytes!("../icons/icon.png");
    if let Ok(home) = std::env::var("HOME") {
        let home_path = PathBuf::from(home);
        let icon_dir = home_path.join(".local/share/icons/hicolor/512x512/apps");
        let app_dir = home_path.join(".local/share/applications");
        let _ = std::fs::create_dir_all(&icon_dir);
        let _ = std::fs::create_dir_all(&app_dir);

        let icon_path1 = icon_dir.join("openrac-launcher.png");
        let icon_path2 = icon_dir.join("dev.openrac.launcher.png");
        let _ = std::fs::write(&icon_path1, icon_bytes);
        let _ = std::fs::write(&icon_path2, icon_bytes);

        let exe_path = std::env::current_exe().unwrap_or_else(|_| PathBuf::from("openrac-launcher"));
        let exe_str = exe_path.to_string_lossy();

        let desktop_content1 = format!(
            "[Desktop Entry]\nType=Application\nName=OpenRAC Launcher\nComment=OpenRAC Launcher\nExec={}\nIcon=openrac-launcher\nTerminal=false\nCategories=Game;Development;\nStartupWMClass=openrac-launcher\n",
            exe_str
        );
        let desktop_content2 = format!(
            "[Desktop Entry]\nType=Application\nName=OpenRAC Launcher\nComment=OpenRAC Launcher\nExec={}\nIcon=dev.openrac.launcher\nTerminal=false\nCategories=Game;Development;\nStartupWMClass=dev.openrac.launcher\n",
            exe_str
        );

        let desk1 = app_dir.join("openrac-launcher.desktop");
        let desk2 = app_dir.join("dev.openrac.launcher.desktop");
        let _ = std::fs::write(desk1, desktop_content1);
        let _ = std::fs::write(desk2, desktop_content2);
    }
}

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            #[cfg(target_os = "linux")]
            ensure_linux_desktop_integration();

            if let Some(window) = app.get_webview_window("main") {
                if let Some(icon) = app.default_window_icon() {
                    let _ = window.set_icon(icon.clone());
                }
            }

            let config_file = app.path().app_config_dir().ok().map(|dir| dir.join("launcher.json"));
            let config = config_file.as_deref().map(config::load).unwrap_or_default();
            app.manage(AppState { config: Mutex::new(config), config_file, jobs: Jobs::default() });
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            app_info,
            get_config,
            save_config,
            detect,
            check_root,
            check_tool,
            library,
            run_action,
            cancel_job,
            open_path,
            open_url,
            inspect_iso,
            import_iso,
            sync_progress_from_web,
            apply_progress_json,
        ])
        .run(tauri::generate_context!())
        .expect("the launcher failed to start");
}
