//! A mod's own page (core/src/mod_tools.rs): the `openrac-mod` scheme that serves its files, and
//! the commands behind its messages (src/lib/modTool.ts). Only the launcher's window calls these,
//! after it has checked the page's message.

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::{LazyLock, Mutex};

use openrac_launcher_core::mod_tools::{self, ToolFile, ToolRoot};
use openrac_launcher_core::{launcher_log, plugins, saves, texture_packs};
use tauri::http::{header, Request, Response, StatusCode};
use tauri::{AppHandle, Manager, Runtime, State};

use crate::AppState;

/// The scheme a mod's page is served from: `openrac-mod://localhost/<game>/<mod id>/<path>`
/// (`http://openrac-mod.localhost/...` on Windows and Android).
pub const SCHEME: &str = "openrac-mod";

/// `%XX` escapes of a path segment decoded; None when one is broken or the result is not UTF-8.
fn decode(segment: &str) -> Option<String> {
    let bytes = segment.as_bytes();
    let mut out = Vec::with_capacity(bytes.len());
    let mut i = 0;
    while i < bytes.len() {
        if bytes[i] == b'%' {
            let hex = std::str::from_utf8(bytes.get(i + 1..i + 3)?).ok()?;
            out.push(u8::from_str_radix(hex, 16).ok()?);
            i += 3;
        } else {
            out.push(bytes[i]);
            i += 1;
        }
    }
    String::from_utf8(out).ok()
}

/// The game, the mod's id and the file's path a request of the scheme names
/// (`/<game>/<mod id>/<path>`, each part escaped); None when it names no file.
fn parts(path: &str) -> Option<(String, String, String)> {
    let parts: Vec<String> = path.trim_start_matches('/').split('/').map(decode).collect::<Option<_>>()?;
    let [game, id, rest @ ..] = parts.as_slice() else { return None };
    let plain = !game.is_empty() && game.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    let file = rest.join("/");
    (plain && !file.is_empty()).then(|| (game.clone(), id.clone(), file))
}

/// The file a request of the scheme names, from the install folder's mods.
fn requested<R: Runtime>(app: &AppHandle<R>, path: &str) -> Result<PathBuf, String> {
    let (game, id, rest) = parts(path).ok_or("no mod's file in the address")?;
    let install = app.state::<AppState>().config.lock().unwrap().install_dir.clone().ok_or("no install folder")?;
    mod_tools::page_file(&plugins::dir(&install, &game), &id, &rest)
}

/// The answer to a request of the scheme: the file, with the content policy of a mod's page.
pub(crate) fn serve<R: Runtime>(app: &AppHandle<R>, request: &Request<Vec<u8>>) -> Response<Vec<u8>> {
    let refuse = |status: StatusCode, why: String| {
        Response::builder()
            .status(status)
            .header(header::CONTENT_TYPE, "text/plain; charset=utf-8")
            .body(why.into_bytes())
            .expect("a plain answer")
    };
    if request.method() != tauri::http::Method::GET {
        return refuse(StatusCode::METHOD_NOT_ALLOWED, "a mod's page only reads its files".into());
    }
    let file = match requested(app, request.uri().path()) {
        Ok(file) => file,
        Err(why) => return refuse(StatusCode::NOT_FOUND, why),
    };
    match std::fs::read(&file) {
        Ok(bytes) => Response::builder()
            .header(header::CONTENT_TYPE, mod_tools::content_type(&file))
            .header("Content-Security-Policy", mod_tools::PAGE_CSP)
            .header("X-Content-Type-Options", "nosniff")
            .header(header::CACHE_CONTROL, "no-store")
            .body(bytes)
            .expect("a file's answer"),
        Err(e) => refuse(StatusCode::NOT_FOUND, e.to_string()),
    }
}

/// The folder of the game's files `root` names, for disc `version` (`pal`) of `game`.
fn root_dir(state: &State<'_, AppState>, game: &str, version: &str, root: ToolRoot) -> Result<PathBuf, String> {
    match root {
        ToolRoot::Levels => Ok(crate::reset_layout(state, &format!("{game}/{version}"))?.game_levels()),
        ToolRoot::Dump => Ok(texture_packs::dump_dir(&saves::data_home(), &texture_packs::id(game, version)?)),
    }
}

/// The files of one of the game's folders a mod's page may read.
#[tauri::command]
pub(crate) async fn mod_tool_list(
    state: State<'_, AppState>,
    game: String,
    version: String,
    root: ToolRoot,
) -> Result<Vec<ToolFile>, String> {
    let dir = root_dir(&state, &game, &version, root)?;
    tauri::async_runtime::spawn_blocking(move || mod_tools::list(&dir)).await.map_err(|e| e.to_string())?
}

/// One of those files, as bytes: all of it, or `length` bytes from `start`.
#[tauri::command]
pub(crate) async fn mod_tool_read(
    state: State<'_, AppState>,
    game: String,
    version: String,
    root: ToolRoot,
    path: String,
    start: Option<u64>,
    length: Option<u64>,
) -> Result<tauri::ipc::Response, String> {
    let dir = root_dir(&state, &game, &version, root)?;
    let bytes =
        tauri::async_runtime::spawn_blocking(move || mod_tools::read_range(&dir, &path, start.unwrap_or(0), length))
            .await
            .map_err(|e| e.to_string())??;
    Ok(tauri::ipc::Response::new(bytes))
}

/// What a mod stored under `key`, or null.
#[tauri::command]
pub(crate) fn mod_tool_store_get(id: String, key: String) -> Result<Option<serde_json::Value>, String> {
    mod_tools::store_get(&saves::data_home(), &id, &key)
}

/// Stores `value` under `key` for a mod (null removes it).
#[tauri::command]
pub(crate) fn mod_tool_store_set(id: String, key: String, value: serde_json::Value) -> Result<(), String> {
    mod_tools::store_set(&saves::data_home(), &id, &key, &value)
}

/// A header of a request with a raw body, as text.
fn header_text(request: &tauri::ipc::Request<'_>, name: &str) -> Result<String, String> {
    let value = request.headers().get(name).ok_or_else(|| format!("no {name}"))?;
    let text = value.to_str().map_err(|e| e.to_string())?;
    decode(text).ok_or_else(|| format!("a broken {name}"))
}

/// Saves the bytes a mod's page made (its texture pack's .zip, its preset's .json) where the player
/// chooses: the request's body, its `x-mod`, `x-name` and `x-extension` headers. The name it was
/// saved under, or empty when the player backed out.
#[tauri::command]
pub(crate) async fn mod_tool_save(app: AppHandle, request: tauri::ipc::Request<'_>) -> Result<String, String> {
    let tauri::ipc::InvokeBody::Raw(bytes) = request.body() else { return Err("no file to save".into()) };
    let id = header_text(&request, "x-mod")?;
    let extension = header_text(&request, "x-extension")?;
    let name = mod_tools::save_name(&header_text(&request, "x-name")?, &extension, mod_tools::SAVE_KINDS)?;
    let bytes = bytes.clone();
    #[cfg(target_os = "android")]
    {
        let cache = app.path().app_cache_dir().map_err(|e| e.to_string())?.join("mod-tool-saves");
        std::fs::create_dir_all(&cache).map_err(|e| e.to_string())?;
        let dest = cache.join(&name);
        std::fs::write(&dest, &bytes).map_err(|e| e.to_string())?;
        let mime = if extension == "zip" { "application/zip" } else { "application/json" };
        let saved = crate::android::save_as_type(&app, &dest, &name, mime).await;
        let _ = std::fs::remove_file(&dest);
        let saved = saved?.unwrap_or_default();
        if !saved.is_empty() {
            launcher_log::line(&format!("{id}: saved {saved}"));
        }
        Ok(saved)
    }
    #[cfg(not(target_os = "android"))]
    {
        use tauri_plugin_dialog::DialogExt;
        let mut dialog = app.dialog().file().set_file_name(&name).add_filter(&name, &[extension.as_str()]);
        if let Some(dir) = openrac_launcher_core::reports::save_folder() {
            dialog = dialog.set_directory(dir);
        }
        let picked = tauri::async_runtime::spawn_blocking(move || dialog.blocking_save_file())
            .await
            .map_err(|e| e.to_string())?;
        let Some(picked) = picked else { return Ok(String::new()) };
        let dest = picked.into_path().map_err(|e| e.to_string())?;
        let out = dest.clone();
        tauri::async_runtime::spawn_blocking(move || std::fs::write(&out, &bytes))
            .await
            .map_err(|e| e.to_string())?
            .map_err(|e| format!("{}: {e}", dest.display()))?;
        launcher_log::line(&format!("{id}: saved {}", dest.display()));
        Ok(openrac_launcher_core::reports::tidy(
            &dest.to_string_lossy(),
            openrac_launcher_core::reports::home().as_deref(),
        ))
    }
}

/// A file a mod's page is saving in parts (mod_tool_save_start, _write, _end): where it goes.
struct Saving {
    file: mod_tools::PartSave,
    id: String,
    /// Android: the name the player is offered in its "save as" screen at the end, and its type.
    #[cfg_attr(not(target_os = "android"), allow(dead_code))]
    name: String,
    #[cfg_attr(not(target_os = "android"), allow(dead_code))]
    mime: &'static str,
}

static SAVING: LazyLock<Mutex<(u64, HashMap<u64, Saving>)>> = LazyLock::new(|| Mutex::new((0, HashMap::new())));

/// Starts saving a large file a mod's page makes in parts (its texture pack): asks the player where
/// on a desktop (on Android at the end), and answers the number its parts are sent with; 0 when
/// they backed out.
#[tauri::command]
pub(crate) async fn mod_tool_save_start(
    app: AppHandle,
    id: String,
    name: String,
    extension: String,
) -> Result<u64, String> {
    let name = mod_tools::save_name(&name, &extension, mod_tools::SAVE_KINDS)?;
    let mime = if extension == "zip" { "application/zip" } else { "application/json" };
    #[cfg(target_os = "android")]
    let dest: PathBuf = {
        let cache = app.path().app_cache_dir().map_err(|e| e.to_string())?.join("mod-tool-saves");
        std::fs::create_dir_all(&cache).map_err(|e| e.to_string())?;
        cache.join(&name)
    };
    #[cfg(not(target_os = "android"))]
    let dest: PathBuf = {
        use tauri_plugin_dialog::DialogExt;
        let mut dialog = app.dialog().file().set_file_name(&name).add_filter(&name, &[extension.as_str()]);
        if let Some(dir) = openrac_launcher_core::reports::save_folder() {
            dialog = dialog.set_directory(dir);
        }
        let picked = tauri::async_runtime::spawn_blocking(move || dialog.blocking_save_file())
            .await
            .map_err(|e| e.to_string())?;
        let Some(picked) = picked else { return Ok(0) };
        picked.into_path().map_err(|e| e.to_string())?
    };
    let file = mod_tools::PartSave::create(&dest)?;
    let mut saving = SAVING.lock().unwrap();
    saving.0 += 1;
    let token = saving.0;
    saving.1.insert(token, Saving { file, id, name, mime });
    Ok(token)
}

/// The next part of a file being saved: the request's body, its `x-token` header.
#[tauri::command]
pub(crate) fn mod_tool_save_write(request: tauri::ipc::Request<'_>) -> Result<(), String> {
    let tauri::ipc::InvokeBody::Raw(bytes) = request.body() else { return Err("no bytes to save".into()) };
    let token: u64 = header_text(&request, "x-token")?.parse().map_err(|_| "a broken x-token")?;
    let mut saving = SAVING.lock().unwrap();
    let save = saving.1.get_mut(&token).ok_or("no such file being saved")?;
    let written = save.file.write(bytes);
    if written.is_err() {
        if let Some(save) = saving.1.remove(&token) {
            save.file.abort();
        }
    }
    written
}

/// Ends a file being saved: kept (`keep`), the name it was saved under (Android: "" when the player
/// backed out of its "save as" screen); or dropped, nothing left of it.
#[tauri::command]
pub(crate) async fn mod_tool_save_end(app: AppHandle, token: u64, keep: bool) -> Result<String, String> {
    let save = SAVING.lock().unwrap().1.remove(&token).ok_or("no such file being saved")?;
    if !keep {
        save.file.abort();
        return Ok(String::new());
    }
    let id = save.id;
    let dest = tauri::async_runtime::spawn_blocking(move || save.file.finish()).await.map_err(|e| e.to_string())??;
    #[cfg(target_os = "android")]
    {
        let saved = crate::android::save_as_type(&app, &dest, &save.name, save.mime).await;
        let _ = std::fs::remove_file(&dest);
        let saved = saved?.unwrap_or_default();
        if !saved.is_empty() {
            launcher_log::line(&format!("{id}: saved {saved}"));
        }
        Ok(saved)
    }
    #[cfg(not(target_os = "android"))]
    {
        let _ = &app;
        launcher_log::line(&format!("{id}: saved {}", dest.display()));
        Ok(openrac_launcher_core::reports::tidy(
            &dest.to_string_lossy(),
            openrac_launcher_core::reports::home().as_deref(),
        ))
    }
}

/// A small file the player picks for a mod's page (a preset to import): its bytes, at most the
/// store's limit; none when they backed out (an empty file is refused).
#[tauri::command]
pub(crate) async fn mod_tool_open(app: AppHandle, extension: String) -> Result<tauri::ipc::Response, String> {
    if !mod_tools::SAVE_KINDS.contains(&extension.as_str()) {
        return Err(format!("a mod's page cannot open .{extension} files"));
    }
    #[cfg(target_os = "android")]
    {
        let bytes = crate::android::pick_small(app, mod_tools::STORE_MAX as u64).await?.unwrap_or_default();
        Ok(tauri::ipc::Response::new(bytes))
    }
    #[cfg(not(target_os = "android"))]
    {
        let picked: Option<PathBuf> = {
            use tauri_plugin_dialog::DialogExt;
            let dialog = app.dialog().file().add_filter(&extension, &[extension.as_str()]);
            let picked = tauri::async_runtime::spawn_blocking(move || dialog.blocking_pick_file())
                .await
                .map_err(|e| e.to_string())?;
            match picked {
                Some(p) => Some(p.into_path().map_err(|e| e.to_string())?),
                None => None,
            }
        };
        let Some(path) = picked else { return Ok(tauri::ipc::Response::new(Vec::new())) };
        let bytes = read_small(&path)?;
        if bytes.is_empty() {
            return Err("the file is empty".into());
        }
        Ok(tauri::ipc::Response::new(bytes))
    }
}

#[cfg_attr(target_os = "android", allow(dead_code))]
fn read_small(path: &Path) -> Result<Vec<u8>, String> {
    let size = path.metadata().map_err(|e| e.to_string())?.len();
    if size > mod_tools::STORE_MAX as u64 {
        return Err(format!("the file is larger than {} KB", mod_tools::STORE_MAX >> 10));
    }
    std::fs::read(path).map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests {
    use super::{decode, parts};

    #[test]
    fn addresses_name_a_file() {
        let got = parts("/rac1/nanotech-upscaler/ui/index.html");
        assert_eq!(got, Some(("rac1".into(), "nanotech-upscaler".into(), "ui/index.html".into())));
        assert_eq!(parts("/rac1/upscaler/ui%2Fmain.js").map(|p| p.2), Some("ui/main.js".into()));
        for bad in [
            "/",
            "/rac1",
            "/rac1/upscaler",
            "/rac1/upscaler/",
            "//upscaler/a.html",
            "/ra%20c1/upscaler/a.html",
            "/rac1/up/%zz",
        ] {
            assert_eq!(parts(bad), None, "{bad:?} was let through");
        }
        // `..` passes here as a name; mod_tools::page_file refuses it.
        assert_eq!(parts("/rac1/upscaler/%2E%2E/x").map(|p| p.2), Some("../x".into()));
    }

    #[test]
    fn segments_decode() {
        assert_eq!(decode("ui%2Findex.html").as_deref(), Some("ui/index.html"));
        assert_eq!(decode("Nanotech%20Upscaler").as_deref(), Some("Nanotech Upscaler"));
        assert_eq!(decode("plain").as_deref(), Some("plain"));
        assert_eq!(decode("%zz"), None);
        assert_eq!(decode("%2"), None);
        assert_eq!(decode("%ff"), None);
    }
}
