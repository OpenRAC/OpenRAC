//! A mod's own page in the launcher (docs/port/MODS.md, section 9, "A mod's page"): a mod whose
//! manifest names `"tool": {"page": "ui/index.html"}` gets an Open button on its page, which shows
//! that HTML file in a frame of its own (src/components/ModTool.svelte). The page is the mod's
//! code, so it is kept apart from the launcher: it is served from the mod's folder under its own
//! address (the `openrac-mod` scheme, src-tauri), with a content policy that lets it load only its
//! own files and reach no network, and it cannot call the launcher's commands. It asks the launcher
//! for a few things through messages (src/lib/modTool.ts), each one answered here:
//!
//! - the game's files it may read: the levels the launcher read from the disc
//!   (`assets/port/<game>/<version>`, or the install folder's copy) and the textures the game saved
//!   (`texture_dumps/<id>`, texture_packs.rs), listed and read, never written;
//! - a small store of its own (`<data home>/mod-tools/<id>/<key>.json`), for its settings and
//!   presets;
//! - files the player saves or opens through the system's dialogs (src-tauri).
//!
//! Every path the page sends is relative, checked to stay inside its folder once links are resolved.

use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};
use serde_json::Value;

/// The most bytes a page may read in one file (a level's largest texture or model is far smaller).
pub const READ_MAX: u64 = 64 << 20;
/// The most files a listing holds.
pub const LIST_MAX: usize = 200_000;
/// The most bytes one stored value takes, as JSON.
pub const STORE_MAX: usize = 1 << 20;

/// What a page may load from its own folder: its scripts, styles, pictures and WebAssembly, and
/// workers made from them. No network, no plugins, no frames of its own, no forms.
pub const PAGE_CSP: &str = "default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; worker-src 'self' blob:; \
     img-src 'self' blob: data:; style-src 'self' 'unsafe-inline'; font-src 'self' data:; connect-src 'self' blob: data:; \
     object-src 'none'; frame-src 'none'; form-action 'none'; base-uri 'self'";

/// The part of a mod's manifest that names its page.
#[derive(Debug, Clone, Deserialize, PartialEq, Default)]
pub struct Tool {
    /// An HTML file inside the mod.
    pub page: String,
}

/// A folder of the game's files a page may read.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub enum ToolRoot {
    /// The levels the launcher read from the disc.
    Levels,
    /// The textures the game saved while it was played (Texture packs, step 1).
    Dump,
}

/// A file in one of the game's folders, by its path inside it (`/`-separated).
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ToolFile {
    pub path: String,
    #[ts(type = "number")]
    pub size: u64,
}

/// The kinds of file a page may list and read: pictures, models and what describes them.
const READABLE: &[&str] = &["png", "gif", "glb", "gltf", "bin", "json", "txt"];

fn readable(path: &Path) -> bool {
    path.extension().and_then(|e| e.to_str()).is_some_and(|e| READABLE.contains(&e.to_ascii_lowercase().as_str()))
}

/// `rel` inside `root`: relative, `/`-separated, no `.` or `..`, no drive, and still inside `root`
/// once links are resolved. The file must exist.
pub fn inside(root: &Path, rel: &str) -> Result<PathBuf, String> {
    let plain = !rel.is_empty()
        && !rel.contains('\\')
        && !rel.contains('\0')
        && Path::new(rel).components().all(|c| matches!(c, Component::Normal(_)));
    if !plain {
        return Err(format!("{rel:?} is not a path inside the folder"));
    }
    let base = root.canonicalize().map_err(|e| format!("{}: {e}", root.display()))?;
    let path = root.join(rel).canonicalize().map_err(|_| format!("{rel} is not there"))?;
    if !path.starts_with(&base) {
        return Err(format!("{rel} leads out of the folder"));
    }
    Ok(path)
}

/// The file of a mod's page that the `openrac-mod` scheme serves: `rel` inside the mod's folder,
/// for a mod whose manifest names a page.
pub fn page_file(plugins_dir: &Path, id: &str, rel: &str) -> Result<PathBuf, String> {
    if !crate::plugins::valid_id(id) {
        return Err(format!("{id:?} is not a mod's id"));
    }
    let folder = plugins_dir.join(id);
    let manifest = crate::plugins::read_manifest(&folder)?;
    if manifest.tool.is_none() {
        return Err(format!("{id} has no page"));
    }
    let path = inside(&folder, rel)?;
    if !path.is_file() {
        return Err(format!("{rel} is not a file"));
    }
    Ok(path)
}

/// The type a served file is sent with.
pub fn content_type(path: &Path) -> &'static str {
    match path.extension().and_then(|e| e.to_str()).map(str::to_ascii_lowercase).as_deref() {
        Some("html" | "htm") => "text/html; charset=utf-8",
        Some("js" | "mjs") => "text/javascript; charset=utf-8",
        Some("css") => "text/css; charset=utf-8",
        Some("json") => "application/json",
        Some("wasm") => "application/wasm",
        Some("png") => "image/png",
        Some("jpg" | "jpeg") => "image/jpeg",
        Some("gif") => "image/gif",
        Some("webp") => "image/webp",
        Some("svg") => "image/svg+xml",
        Some("woff2") => "font/woff2",
        Some("glb") => "model/gltf-binary",
        Some("txt" | "md") => "text/plain; charset=utf-8",
        _ => "application/octet-stream",
    }
}

/// Every readable file under `root`, by its path inside it, sorted. Links are not followed. An
/// absent folder lists nothing.
pub fn list(root: &Path) -> Result<Vec<ToolFile>, String> {
    let mut out = Vec::new();
    if !root.is_dir() {
        return Ok(out);
    }
    let mut stack = vec![PathBuf::new()];
    while let Some(rel) = stack.pop() {
        let dir = root.join(&rel);
        let entries = std::fs::read_dir(&dir).map_err(|e| format!("{}: {e}", dir.display()))?;
        for entry in entries {
            let entry = entry.map_err(|e| e.to_string())?;
            let kind = entry.file_type().map_err(|e| e.to_string())?;
            let name = entry.file_name();
            let Some(name) = name.to_str() else { continue };
            if name.starts_with('.') {
                continue;
            }
            let child = rel.join(name);
            if kind.is_dir() {
                stack.push(child);
            } else if kind.is_file() && readable(&child) {
                if out.len() >= LIST_MAX {
                    return Err(format!("{} holds more than {LIST_MAX} files", root.display()));
                }
                let size = entry.metadata().map_err(|e| e.to_string())?.len();
                let path = child.components().map(|c| c.as_os_str().to_string_lossy()).collect::<Vec<_>>().join("/");
                out.push(ToolFile { path, size });
            }
        }
    }
    out.sort_by(|a, b| a.path.cmp(&b.path));
    Ok(out)
}

/// The bytes of `rel` under `root`: a readable file inside it, at most READ_MAX bytes.
pub fn read(root: &Path, rel: &str) -> Result<Vec<u8>, String> {
    let path = inside(root, rel)?;
    if !path.is_file() || !readable(&path) {
        return Err(format!("{rel} is not a file a mod's page may read"));
    }
    let size = path.metadata().map_err(|e| e.to_string())?.len();
    if size > READ_MAX {
        return Err(format!("{rel} is larger than {} MB", READ_MAX >> 20));
    }
    std::fs::read(&path).map_err(|e| format!("{rel}: {e}"))
}

fn valid_key(key: &str) -> bool {
    !key.is_empty()
        && key.len() <= 64
        && key.chars().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || c == '_' || c == '-')
}

/// The folder of a mod's store.
pub fn store_dir(data_home: &Path, id: &str) -> Result<PathBuf, String> {
    if !crate::plugins::valid_id(id) {
        return Err(format!("{id:?} is not a mod's id"));
    }
    Ok(data_home.join("mod-tools").join(id))
}

fn store_file(data_home: &Path, id: &str, key: &str) -> Result<PathBuf, String> {
    if !valid_key(key) {
        return Err(format!("{key:?} is not a key ([a-z0-9_-], at most 64)"));
    }
    Ok(store_dir(data_home, id)?.join(format!("{key}.json")))
}

/// The value a mod stored under `key`, or None.
pub fn store_get(data_home: &Path, id: &str, key: &str) -> Result<Option<Value>, String> {
    let file = store_file(data_home, id, key)?;
    match std::fs::read_to_string(&file) {
        Ok(text) => serde_json::from_str(&text).map(Some).map_err(|e| format!("{key}: {e}")),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(e) => Err(format!("{key}: {e}")),
    }
}

/// Stores `value` under `key` (null removes it), written whole or not at all.
pub fn store_set(data_home: &Path, id: &str, key: &str, value: &Value) -> Result<(), String> {
    let file = store_file(data_home, id, key)?;
    if value.is_null() {
        return match std::fs::remove_file(&file) {
            Err(e) if e.kind() != std::io::ErrorKind::NotFound => Err(format!("{key}: {e}")),
            _ => Ok(()),
        };
    }
    let text = serde_json::to_string(value).map_err(|e| e.to_string())?;
    if text.len() > STORE_MAX {
        return Err(format!("{key} is larger than {} KB", STORE_MAX >> 10));
    }
    let dir = file.parent().expect("a store file has a folder");
    std::fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    let part = file.with_extension("json.part");
    std::fs::write(&part, text).map_err(|e| format!("{key}: {e}"))?;
    std::fs::rename(&part, &file).map_err(|e| format!("{key}: {e}"))
}

/// A file name the player is offered when a page saves one: the page's name, made plain (letters,
/// digits, spaces, `.`, `-`, `_`), with `extension` (one of `allowed`) at its end.
pub fn save_name(name: &str, extension: &str, allowed: &[&str]) -> Result<String, String> {
    if !allowed.contains(&extension) {
        return Err(format!("a mod's page cannot save .{extension} files"));
    }
    let stem: String = name
        .trim()
        .trim_end_matches(&format!(".{extension}"))
        .chars()
        .map(|c| if c.is_alphanumeric() || matches!(c, ' ' | '.' | '-' | '_') { c } else { '_' })
        .take(80)
        .collect();
    let stem = stem.trim_matches(|c: char| c == '.' || c == ' ');
    Ok(format!("{}.{extension}", if stem.is_empty() { "file" } else { stem }))
}

/// The kinds of file a page may save through the dialog: a texture pack and its own settings.
pub const SAVE_KINDS: &[&str] = &["zip", "json"];

#[cfg(test)]
mod tests {
    use super::*;

    fn temp(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("openrac-mod-tools-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    fn paths_stay_inside() {
        let root = temp("inside");
        std::fs::create_dir_all(root.join("a")).unwrap();
        std::fs::write(root.join("a/b.png"), b"x").unwrap();
        assert!(inside(&root, "a/b.png").is_ok());
        for bad in ["", "/etc/passwd", "../x", "a/../a/b.png", "a\\b.png", "./a/b.png", "missing.png"] {
            assert!(inside(&root, bad).is_err(), "{bad:?} was let through");
        }
        #[cfg(unix)]
        {
            let outside = temp("inside-outside");
            std::fs::write(outside.join("secret.png"), b"s").unwrap();
            std::os::unix::fs::symlink(&outside, root.join("link")).unwrap();
            assert!(inside(&root, "link/secret.png").is_err());
            assert!(read(&root, "link/secret.png").is_err());
        }
    }

    #[test]
    fn lists_and_reads_only_game_files() {
        let root = temp("list");
        std::fs::create_dir_all(root.join("level_00/textures")).unwrap();
        std::fs::write(root.join("level_00/textures/moby_0001.png"), b"png").unwrap();
        std::fs::write(root.join("level_00/manifest.json"), b"{}").unwrap();
        std::fs::write(root.join("level_00/notes.exe"), b"no").unwrap();
        std::fs::write(root.join(".hidden.png"), b"no").unwrap();
        let files = list(&root).unwrap();
        let paths: Vec<&str> = files.iter().map(|f| f.path.as_str()).collect();
        assert_eq!(paths, ["level_00/manifest.json", "level_00/textures/moby_0001.png"]);
        assert_eq!(files[1].size, 3);
        assert_eq!(read(&root, "level_00/textures/moby_0001.png").unwrap(), b"png");
        assert!(read(&root, "level_00/notes.exe").is_err());
        assert!(list(&root.join("absent")).unwrap().is_empty());
    }

    #[test]
    fn store_round_trips() {
        let home = temp("store");
        assert_eq!(store_get(&home, "nanotech-upscaler", "presets").unwrap(), None);
        let value = serde_json::json!({"presets": [{"name": "Sharp", "scale": 4}]});
        store_set(&home, "nanotech-upscaler", "presets", &value).unwrap();
        assert_eq!(store_get(&home, "nanotech-upscaler", "presets").unwrap(), Some(value));
        store_set(&home, "nanotech-upscaler", "presets", &Value::Null).unwrap();
        assert_eq!(store_get(&home, "nanotech-upscaler", "presets").unwrap(), None);
        assert!(store_set(&home, "nanotech-upscaler", "../x", &Value::Bool(true)).is_err());
        assert!(store_set(&home, "../x", "a", &Value::Bool(true)).is_err());
        let big = Value::String("x".repeat(STORE_MAX + 1));
        assert!(store_set(&home, "nanotech-upscaler", "big", &big).is_err());
    }

    #[test]
    fn save_names_are_plain() {
        assert_eq!(save_name("Nanotech 4x.zip", "zip", SAVE_KINDS).unwrap(), "Nanotech 4x.zip");
        assert_eq!(save_name("../../a/b", "json", SAVE_KINDS).unwrap(), "_.._a_b.json");
        assert_eq!(save_name("", "zip", SAVE_KINDS).unwrap(), "file.zip");
        assert!(save_name("x", "exe", SAVE_KINDS).is_err());
    }

    #[test]
    fn served_types() {
        assert_eq!(content_type(Path::new("a/engine.wasm")), "application/wasm");
        assert_eq!(content_type(Path::new("index.HTML")), "text/html; charset=utf-8");
        assert_eq!(content_type(Path::new("x")), "application/octet-stream");
    }
}
