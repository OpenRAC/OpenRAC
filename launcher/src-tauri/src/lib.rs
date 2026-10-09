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
use openrac_launcher_core::{catalog, disc};
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

/// Adds the user's disc image for version `key` (`rac1/pal`): checks that
/// the image is that game, then links it into baserom/. Returns where.
#[tauri::command]
async fn add_disc(state: State<'_, AppState>, key: String, path: PathBuf) -> Result<String, String> {
    let root = state.config.lock().unwrap().root.clone().ok_or("the OpenRAC folder is not set")?;
    tauri::async_runtime::spawn_blocking(move || {
        let catalog = catalog::load(&root)?;
        let version = catalog.version(&key).ok_or_else(|| format!("no version {key}"))?;
        disc::add(&root, version, &path).map(|place| place.display().to_string())
    })
    .await
    .map_err(|e| e.to_string())?
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

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
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
            add_disc,
            run_action,
            cancel_job,
            open_path,
            open_url,
        ])
        .run(tauri::generate_context!())
        .expect("the launcher failed to start");
}
