//! OpenRAC Editor, the level editor: a program of its own, from its repository's releases
//! ([`REPOSITORY`]). The launcher carries none of it. Once the player downloads it, the launcher
//! reads its manifest ([`MANIFEST`]) and lists the actions it brings beside the game's own (the
//! level editor's steps on the Mods page); without it the Mods page offers the download.
//!
//! The editor's tools read the disc with OpenRAC's extractor, so an editor fits an OpenRAC whose
//! `extractor/extractor_api.json` has the version its manifest names (`extractorApi`).

use std::collections::BTreeMap;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::time::SystemTime;

use serde::{Deserialize, Serialize};

use crate::actions::{Action, Platform};
use crate::remote::{Fetch, Hosts};

/// The repository whose releases carry the editor.
pub const REPOSITORY: &str = "nickyyOO/OpenRAC-Editor";
/// The editor's manifest, in its folder.
pub const MANIFEST: &str = "openrac/editor.json";
/// The editor's Python tools, in its folder (`{editor_tools}`).
pub const TOOLS: &str = "openrac/tools";
/// A folder holding an editor to use instead of the downloaded one (a contributor's build).
pub const ENV: &str = "OPENRAC_EDITOR";
/// What OpenRAC's extractor offers the editor's tools, in a checkout.
pub const API_FILE: &str = "extractor/extractor_api.json";
/// The format of the manifest this launcher reads.
const FORMAT: u32 = 1;
/// The most an unpacked editor may take.
const MAX_UNPACKED: u64 = 2 * 1024 * 1024 * 1024;

/// `openrac/editor.json`.
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct Manifest {
    pub format: u32,
    /// The editor's version (its release's tag), `dev` for a build of one's own.
    pub version: String,
    /// The version of OpenRAC's extractor_api.json the editor's tools are written against.
    pub extractor_api: u32,
    /// The editor's program in its folder, by system (`linux`, `windows`, `macos`).
    pub program: BTreeMap<String, String>,
    /// The actions it adds, by version (`rac1/pal`), in actions.json's form.
    #[serde(default)]
    pub versions: BTreeMap<String, Vec<Action>>,
}

/// An editor found in a folder.
#[derive(Debug, Clone)]
pub struct Editor {
    pub dir: PathBuf,
    pub manifest: Manifest,
}

/// What the Mods page's Level editor tab shows of the editor.
#[derive(Debug, Clone, Serialize, ts_rs::TS)]
#[ts(rename = "EditorInstall")]
#[serde(rename_all = "camelCase")]
pub struct Status {
    /// Its folder, whether or not it is there.
    pub folder: String,
    /// It is an app the player installed (a Mac's Applications), not the launcher's download:
    /// the launcher does not remove it.
    pub installed_app: bool,
    /// The player picked its folder (Level editor, "Choose its folder"): the launcher neither
    /// downloads into it nor removes it.
    pub chosen: bool,
    /// The version found there.
    pub version: Option<String>,
    /// Why it cannot be used with this OpenRAC, when it cannot.
    pub problem: Option<String>,
    /// Where it comes from.
    pub repository: String,
}

/// The app a Mac's .dmg installs, by its name.
pub const MAC_APP: &str = "OpenRAC Editor.app";
/// Where in that app its manifest and tools are (the .dmg puts the archive's extras there).
const MAC_APP_EXTRAS: &str = "Contents/Resources";

/// The file in the launcher's data folder naming the editor folder the player picked.
const CHOSEN_FILE: &str = "editor-folder.txt";

/// Where the editor is: [`ENV`] when set; else the folder the player picked; else the launcher's
/// download when there is one; else, on a Mac, the app installed in Applications (the system's or
/// the player's own); else where the launcher downloads it.
pub fn folder() -> PathBuf {
    let downloaded = download_folder();
    if std::env::var_os(ENV).is_some_and(|d| !d.is_empty()) && !cfg!(test) {
        return downloaded;
    }
    if let Some(dir) = chosen() {
        return dir;
    }
    if cfg!(test) || crate::under(&downloaded, MANIFEST).is_file() {
        return downloaded;
    }
    if cfg!(target_os = "macos") {
        let home = crate::detect::home_dir().map(|h| h.join("Applications"));
        for apps in [Some(PathBuf::from("/Applications")), home].into_iter().flatten() {
            if let Some(dir) = app_extras(&apps.join(MAC_APP)) {
                return dir;
            }
        }
    }
    downloaded
}

/// The folder the player picked, while it still holds an editor.
fn chosen() -> Option<PathBuf> {
    let text = std::fs::read_to_string(crate::saves::data_home().join(CHOSEN_FILE)).ok()?;
    let dir = PathBuf::from(text.trim());
    (!text.trim().is_empty() && crate::under(&dir, MANIFEST).is_file()).then_some(dir)
}

/// Where in `picked` an editor is: the folder itself, the app it is (a Mac's .app), or the folder
/// holding the editor's program. None when it holds no editor.
fn editor_in(picked: &Path) -> Option<PathBuf> {
    let mut places = vec![picked.to_path_buf(), picked.join(MAC_APP_EXTRAS), picked.join(MAC_APP).join(MAC_APP_EXTRAS)];
    if picked.is_file() {
        places.extend(picked.parent().map(Path::to_path_buf));
    }
    places.into_iter().find(|dir| read(dir).is_ok())
}

/// Uses the editor in `picked` (a folder the player chose, anywhere) from now on. What is there then.
pub fn choose(picked: &Path, root: Option<&Path>) -> Result<Status, String> {
    let dir = editor_in(picked)
        .ok_or_else(|| format!("{} holds no OpenRAC Editor (its {MANIFEST} is missing)", picked.display()))?;
    let file = crate::saves::data_home().join(CHOSEN_FILE);
    if let Some(parent) = file.parent() {
        std::fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
    }
    std::fs::write(&file, dir.to_string_lossy().as_bytes()).map_err(|e| format!("{}: {e}", file.display()))?;
    Ok(status(root))
}

/// Stops using the folder the player picked (it stays as it is). What is used then.
pub fn forget_chosen(root: Option<&Path>) -> Result<Status, String> {
    let file = crate::saves::data_home().join(CHOSEN_FILE);
    match std::fs::remove_file(&file) {
        Ok(()) => Ok(status(root)),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(status(root)),
        Err(e) => Err(format!("{}: {e}", file.display())),
    }
}

/// The folder of an installed Mac app's manifest and tools, when it has them.
fn app_extras(app: &Path) -> Option<PathBuf> {
    let dir = app.join(MAC_APP_EXTRAS);
    crate::under(&dir, MANIFEST).is_file().then_some(dir)
}

/// Whether `dir` is inside an installed Mac app (its [`MAC_APP_EXTRAS`]).
fn in_app(dir: &Path) -> bool {
    dir.ends_with(MAC_APP_EXTRAS)
        && dir.parent().and_then(Path::parent).is_some_and(|app| app.extension().is_some_and(|e| e == "app"))
}

/// Where the launcher downloads the editor: [`ENV`] when set, else `editor/` in its data folder.
pub fn download_folder() -> PathBuf {
    // A test never reads the editor of whoever runs it: only its own data home's.
    let env = if cfg!(test) { None } else { std::env::var_os(ENV) };
    match env {
        Some(dir) if !dir.is_empty() => PathBuf::from(dir),
        _ => crate::saves::data_home().join("editor"),
    }
}

/// The editor in `dir`, or why there is none.
pub fn read(dir: &Path) -> Result<Editor, String> {
    let path = crate::under(dir, MANIFEST);
    let text = std::fs::read_to_string(&path).map_err(|e| format!("{}: {e}", path.display()))?;
    // A manifest saved by a Windows editor may start with a byte order mark.
    let text = text.strip_prefix('\u{feff}').unwrap_or(&text);
    let manifest: Manifest = serde_json::from_str(text).map_err(|e| format!("{}: {e}", path.display()))?;
    if manifest.format != FORMAT {
        return Err(format!(
            "OpenRAC Editor {} is newer than this launcher reads: update the launcher",
            manifest.version
        ));
    }
    Ok(Editor { dir: dir.to_path_buf(), manifest })
}

/// The editor in [`folder`], read once for each change of its manifest.
pub fn find() -> Option<Arc<Editor>> {
    static CACHE: Mutex<Option<(PathBuf, SystemTime, Arc<Editor>)>> = Mutex::new(None);
    let dir = folder();
    let stamp = std::fs::metadata(crate::under(&dir, MANIFEST)).and_then(|m| m.modified()).ok()?;
    let mut cache = CACHE.lock().unwrap_or_else(std::sync::PoisonError::into_inner);
    if let Some((at, when, editor)) = cache.as_ref() {
        if *at == dir && *when == stamp {
            return Some(editor.clone());
        }
    }
    let editor = Arc::new(read(&dir).ok()?);
    *cache = Some((dir, stamp, editor.clone()));
    Some(editor)
}

/// The version of the extractor's API in OpenRAC's folder `root`.
pub fn extractor_api(root: &Path) -> Option<u32> {
    let text = std::fs::read_to_string(crate::under(root, API_FILE)).ok()?;
    let value: serde_json::Value = serde_json::from_str(&text).ok()?;
    value.get("version")?.as_u64().map(|v| v as u32)
}

impl Editor {
    /// The editor's program on this system.
    pub fn program(&self) -> Option<PathBuf> {
        let key = match Platform::current() {
            Platform::Windows => "windows",
            Platform::Macos => "macos",
            Platform::Linux => "linux",
            Platform::Android | Platform::Ios => return None,
        };
        let program = self.manifest.program.get(key)?;
        let path = crate::under(&self.dir, program);
        if path.is_file() {
            return Some(path);
        }
        // An installed Mac app: the manifest names the program from the folder holding the app.
        let beside = self.dir.ancestors().nth(3).filter(|_| in_app(&self.dir))?;
        let path = crate::under(beside, program);
        path.is_file().then_some(path)
    }

    /// Its Python tools (`{editor_tools}`).
    pub fn tools(&self) -> PathBuf {
        crate::under(&self.dir, TOOLS)
    }

    /// Why it cannot be used with OpenRAC's folder `root`, if it cannot.
    pub fn problem(&self, root: &Path) -> Option<String> {
        let version = &self.manifest.version;
        if self.program().is_none() {
            return Some(format!("OpenRAC Editor {version} has no program for this system: download it again"));
        }
        match extractor_api(root) {
            None => Some("this OpenRAC has no extractor_api.json: update OpenRAC".into()),
            Some(api) if api < self.manifest.extractor_api => {
                Some(format!("OpenRAC Editor {version} needs a newer OpenRAC: update OpenRAC"))
            }
            Some(api) if api > self.manifest.extractor_api => {
                Some(format!("OpenRAC Editor {version} is older than this OpenRAC: update the editor"))
            }
            Some(_) => None,
        }
    }

    /// The editor's actions for the version `key`, as the launcher lists them: its documents are
    /// the repository's pages.
    pub fn actions(&self, key: &str) -> Vec<Action> {
        let mut out = self.manifest.versions.get(key).cloned().unwrap_or_default();
        for action in &mut out {
            if let Some(doc) = action.docs.as_mut() {
                if !doc.starts_with("https://") {
                    *doc = format!("https://github.com/{REPOSITORY}/blob/main/{doc}");
                }
            }
        }
        out
    }
}

/// What the page shows of the editor, for OpenRAC's folder `root`.
pub fn status(root: Option<&Path>) -> Status {
    let dir = folder();
    let found = read(&dir);
    Status {
        chosen: chosen().is_some_and(|c| c == dir),
        installed_app: in_app(&dir),
        folder: dir.display().to_string(),
        version: found.as_ref().ok().map(|e| e.manifest.version.clone()),
        problem: match (&found, root) {
            (Ok(editor), Some(root)) => editor.problem(root),
            _ => None,
        },
        repository: format!("https://github.com/{REPOSITORY}"),
    }
}

/// The release's archive for this system.
pub fn asset_name(platform: Platform) -> Option<&'static str> {
    match platform {
        Platform::Linux => Some("openrac-editor-linux-x86_64.zip"),
        Platform::Windows => Some("openrac-editor-windows-x86_64.zip"),
        Platform::Macos => Some("openrac-editor-macos-arm64.zip"),
        Platform::Android | Platform::Ios => None,
    }
}

#[derive(Debug, Deserialize)]
struct Release {
    tag_name: String,
    #[serde(default)]
    assets: Vec<Asset>,
}

#[derive(Debug, Deserialize)]
struct Asset {
    name: String,
    browser_download_url: String,
    #[serde(default)]
    digest: Option<String>,
}

fn latest(fetch: &dyn Fetch, hosts: &Hosts) -> Result<Release, String> {
    let url = format!("{}/repos/{REPOSITORY}/releases/latest", hosts.api);
    let bytes = fetch.get(&url, true).map_err(|e| format!("OpenRAC Editor's releases cannot be read: {e}"))?;
    serde_json::from_slice(&bytes).map_err(|e| format!("OpenRAC Editor's latest release: {e}"))
}

/// The newest release's version.
pub fn newest(fetch: &dyn Fetch, hosts: &Hosts) -> Result<String, String> {
    latest(fetch, hosts).map(|r| r.tag_name)
}

/// Downloads the newest release for this system into [`folder`], checked against the release's
/// digest or its SHA256SUMS, in place of the one there. What is there then.
pub fn install(fetch: &dyn Fetch, hosts: &Hosts, root: Option<&Path>) -> Result<Status, String> {
    let name = asset_name(Platform::current()).ok_or("OpenRAC Editor runs on a computer only")?;
    let release = latest(fetch, hosts)?;
    let asset = release
        .assets
        .iter()
        .find(|a| a.name == name)
        .ok_or_else(|| format!("OpenRAC Editor {} has no {name}", release.tag_name))?;
    let sums = match release.assets.iter().find(|a| a.name == crate::remote::SUMS) {
        Some(sums) => Some(String::from_utf8_lossy(&fetch.get(&sums.browser_download_url, false)?).into_owned()),
        None => None,
    };
    let bytes = fetch.get(&asset.browser_download_url, false)?;
    let digest = asset.digest.as_deref().and_then(|d| d.strip_prefix("sha256:")).map(str::to_ascii_lowercase);
    crate::remote::verify(&bytes, name, digest.as_deref(), sums.as_deref())?;
    put_in_place(&bytes, &download_folder())?;
    Ok(status(root))
}

/// Unpacks the archive `bytes` next to `dir` and swaps it in; the old editor goes only once the
/// new one reads.
fn put_in_place(bytes: &[u8], dir: &Path) -> Result<(), String> {
    put_in_place_with(bytes, dir, &|from, to| std::fs::rename(from, to))
}

/// [`put_in_place`] with its folder moves done by `rename` (a test's to fail them, as Windows does
/// while the editor runs from the folder). A move that fails leaves the old editor as it was.
fn put_in_place_with(
    bytes: &[u8],
    dir: &Path,
    rename: &dyn Fn(&Path, &Path) -> std::io::Result<()>,
) -> Result<(), String> {
    let parent = dir.parent().ok_or("the editor's folder has no parent")?;
    std::fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
    let fresh = dir.with_extension("new");
    let old = dir.with_extension("old");
    for leftover in [&fresh, &old] {
        if leftover.exists() {
            std::fs::remove_dir_all(leftover).map_err(|e| format!("{}: {e}", leftover.display()))?;
        }
    }
    let unpacked = unpack(bytes, &fresh)
        .and_then(|()| read(&fresh).map_err(|e| format!("the download is not OpenRAC Editor ({e})")));
    let editor = match unpacked {
        Ok(editor) => editor,
        Err(e) => {
            let _ = std::fs::remove_dir_all(&fresh);
            return Err(e);
        }
    };
    make_runnable(&editor);
    let had_one = dir.exists();
    if had_one {
        if let Err(e) = rename(dir, &old) {
            let _ = std::fs::remove_dir_all(&fresh);
            return Err(format!("{}: {e} (close OpenRAC Editor first)", dir.display()));
        }
    }
    if let Err(e) = rename(&fresh, dir) {
        // The old editor goes back, so the player still has one.
        if had_one {
            let _ = rename(&old, dir);
        }
        let _ = std::fs::remove_dir_all(&fresh);
        return Err(format!("{}: {e} (close OpenRAC Editor first)", dir.display()));
    }
    if old.exists() {
        let _ = std::fs::remove_dir_all(&old);
    }
    if cfg!(target_os = "macos") {
        let _ = std::process::Command::new("/usr/bin/xattr").arg("-dr").arg("com.apple.quarantine").arg(dir).status();
    }
    Ok(())
}

/// The editor's programs can run, though the .zip kept no Unix modes (one packed on Windows).
fn make_runnable(editor: &Editor) {
    #[cfg(unix)]
    for program in editor.manifest.program.values() {
        use std::os::unix::fs::PermissionsExt;
        let path = crate::under(&editor.dir, program);
        if let Ok(meta) = std::fs::metadata(&path) {
            if meta.is_file() {
                let _ =
                    std::fs::set_permissions(&path, std::fs::Permissions::from_mode(meta.permissions().mode() | 0o755));
            }
        }
    }
    #[cfg(not(unix))]
    let _ = editor;
}

/// The .zip `bytes` into `into`: files and folders only, inside it, with their Unix modes (the
/// program's execute bit).
fn unpack(bytes: &[u8], into: &Path) -> Result<(), String> {
    let mut archive =
        zip::ZipArchive::new(std::io::Cursor::new(bytes)).map_err(|e| format!("the download is not a .zip ({e})"))?;
    let mut written: u64 = 0;
    for i in 0..archive.len() {
        let mut entry = archive.by_index(i).map_err(|e| e.to_string())?;
        let name = entry.name().map_err(|e| e.to_string())?.to_string();
        // A name with \\ is one folder on Windows and a single odd file elsewhere: refused everywhere.
        if name.contains('\\') {
            return Err(format!("the .zip names a file with \\ in place of / ({name})"));
        }
        let Some(relative) = entry.enclosed_name() else {
            return Err(format!("the .zip names a file outside its folder ({name})"));
        };
        if entry.is_symlink() {
            return Err(format!("the .zip holds a link ({name})"));
        }
        let target = into.join(relative);
        if entry.is_dir() {
            std::fs::create_dir_all(&target).map_err(|e| format!("{}: {e}", target.display()))?;
            continue;
        }
        if let Some(parent) = target.parent() {
            std::fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
        }
        let mut data = Vec::new();
        (&mut entry).take(MAX_UNPACKED - written + 1).read_to_end(&mut data).map_err(|e| format!("{name}: {e}"))?;
        written += data.len() as u64;
        if written > MAX_UNPACKED {
            return Err("the .zip unpacks to more than 2 GB".into());
        }
        std::fs::write(&target, data).map_err(|e| format!("{}: {e}", target.display()))?;
        #[cfg(unix)]
        if let Some(mode) = entry.unix_mode() {
            use std::os::unix::fs::PermissionsExt;
            let _ = std::fs::set_permissions(&target, std::fs::Permissions::from_mode(mode & 0o755));
        }
    }
    Ok(())
}

/// Removes the downloaded editor (not a contributor's, named by [`ENV`]).
pub fn remove() -> Result<(), String> {
    if std::env::var_os(ENV).is_some_and(|d| !d.is_empty()) {
        return Err(format!("the editor in {} is your own ({ENV}): it is not removed", folder().display()));
    }
    // An app the player installed stays: only the launcher's download goes.
    let dir = download_folder();
    if dir.exists() {
        std::fs::remove_dir_all(&dir).map_err(|e| format!("{}: {e} (close OpenRAC Editor first)", dir.display()))?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    const MANIFEST_TEXT: &str = r#"{"format": 1, "version": "v1.0.0", "extractorApi": 1,
        "program": {"linux": "openrac-editor.x86_64", "windows": "openrac-editor.exe", "macos": "Ed.app/Contents/MacOS/Ed"},
        "versions": {"rac1/pal": [{"id": "editor-open", "label": "Open", "kind": "edit", "area": "editor",
            "state": "unverified", "program": "{editor}", "args": ["--path", "{projects}/level_{level}"],
            "requires": ["editor"], "docs": "docs/LEVEL_EDITOR.md"}]}}"#;

    fn zip_of(files: &[(&str, &[u8], u32)]) -> Vec<u8> {
        let mut out = std::io::Cursor::new(Vec::new());
        let mut zip = zip::ZipWriter::new(&mut out);
        for (name, data, mode) in files {
            let options = zip::write::SimpleFileOptions::default()
                .compression_method(zip::CompressionMethod::Stored)
                .unix_permissions(*mode);
            zip.start_file(*name, options).unwrap();
            zip.write_all(data).unwrap();
        }
        zip.finish().unwrap();
        out.into_inner()
    }

    struct Answers(BTreeMap<String, Vec<u8>>);

    impl Fetch for Answers {
        fn get(&self, url: &str, _api: bool) -> Result<Vec<u8>, String> {
            self.0.get(url).cloned().ok_or_else(|| format!("{url}: not found"))
        }
    }

    fn program_key() -> &'static str {
        if cfg!(windows) {
            "openrac-editor.exe"
        } else if cfg!(target_os = "macos") {
            "Ed.app/Contents/MacOS/Ed"
        } else {
            "openrac-editor.x86_64"
        }
    }

    /// A folder the player picks is used until it is forgotten: the editor's own folder, its app
    /// or its program; one holding no editor is refused.
    #[test]
    fn a_chosen_folder_is_used_until_forgotten() {
        let home = tempfile::tempdir().unwrap();
        let mine = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(mine.path().join("openrac")).unwrap();
        std::fs::write(mine.path().join(MANIFEST), MANIFEST_TEXT).unwrap();
        std::fs::write(mine.path().join("openrac-editor.x86_64"), "").unwrap();
        crate::saves::with_data_home(home.path(), || {
            let empty = tempfile::tempdir().unwrap();
            assert!(choose(empty.path(), None).unwrap_err().contains("holds no OpenRAC Editor"));
            assert!(!status(None).chosen);

            let got = choose(&mine.path().join("openrac-editor.x86_64"), None).unwrap();
            assert!(got.chosen);
            assert_eq!(got.version.as_deref(), Some("v1.0.0"));
            assert_eq!(folder(), mine.path());
            assert!(find().is_some());

            let left = forget_chosen(None).unwrap();
            assert!(!left.chosen);
            assert_eq!(folder(), download_folder());
            assert!(mine.path().join(MANIFEST).is_file(), "forgetting leaves the folder as it is");
        });
    }

    /// A Mac's installed app keeps the manifest and tools in Contents/Resources; its program is
    /// found from the folder holding the app, as the manifest names it.
    #[test]
    fn an_installed_app_runs_its_own_program() {
        let apps = tempfile::tempdir().unwrap();
        let app = apps.path().join(MAC_APP);
        let extras = app.join(MAC_APP_EXTRAS);
        std::fs::create_dir_all(extras.join("openrac")).unwrap();
        std::fs::create_dir_all(app.join("Contents/MacOS")).unwrap();
        std::fs::write(app.join("Contents/MacOS/Ed"), "").unwrap();
        let program = "OpenRAC Editor.app/Contents/MacOS/Ed";
        std::fs::write(
            extras.join(MANIFEST),
            format!(
                r#"{{"format": 1, "version": "v1.0.0", "extractorApi": 1,
                "program": {{"linux": "{program}", "windows": "{program}", "macos": "{program}"}}}}"#
            ),
        )
        .unwrap();
        assert_eq!(app_extras(&app).as_deref(), Some(extras.as_path()));
        assert!(in_app(&extras));
        assert!(!in_app(apps.path()));
        let editor = read(&extras).unwrap();
        assert_eq!(editor.program(), Some(app.join("Contents/MacOS/Ed")));
        assert_eq!(editor.tools(), extras.join(TOOLS));
    }

    #[test]
    fn reads_the_manifest_and_checks_the_extractors_api() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("openrac")).unwrap();
        std::fs::write(dir.path().join(MANIFEST), MANIFEST_TEXT).unwrap();
        let editor = read(dir.path()).unwrap();
        assert_eq!(editor.manifest.version, "v1.0.0");
        let actions = editor.actions("rac1/pal");
        assert_eq!(
            actions[0].docs.as_deref(),
            Some("https://github.com/nickyyOO/OpenRAC-Editor/blob/main/docs/LEVEL_EDITOR.md")
        );
        assert!(editor.actions("rac2/pal").is_empty());
        // No program yet: that is the problem.
        let root = tempfile::tempdir().unwrap();
        assert!(editor.problem(root.path()).unwrap().contains("no program"));
        let program = crate::under(dir.path(), program_key());
        std::fs::create_dir_all(program.parent().unwrap()).unwrap();
        std::fs::write(&program, "").unwrap();
        assert!(editor.problem(root.path()).unwrap().contains("extractor_api.json"));
        std::fs::create_dir_all(root.path().join("extractor")).unwrap();
        for (api, want) in [(1, None), (2, Some("update the editor")), (0, Some("update OpenRAC"))] {
            std::fs::write(root.path().join(API_FILE), format!(r#"{{"version": {api}}}"#)).unwrap();
            let problem = editor.problem(root.path());
            assert_eq!(problem.is_some(), want.is_some(), "{api}: {problem:?}");
            if let (Some(problem), Some(want)) = (problem, want) {
                assert!(problem.contains(want), "{problem}");
            }
        }
    }

    #[test]
    fn refuses_a_newer_manifest() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("openrac")).unwrap();
        std::fs::write(dir.path().join(MANIFEST), MANIFEST_TEXT.replace(r#""format": 1"#, r#""format": 2"#)).unwrap();
        assert!(read(dir.path()).unwrap_err().contains("update the launcher"));
    }

    #[test]
    fn installs_a_checked_release_in_place_of_the_old_one() {
        let Some(name) = asset_name(Platform::current()) else { return };
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("editor");
        std::fs::create_dir_all(dir.join("openrac")).unwrap();
        std::fs::write(dir.join("stale.txt"), "old").unwrap();
        let archive = zip_of(&[(MANIFEST, MANIFEST_TEXT.as_bytes(), 0o644), (program_key(), b"#!", 0o755)]);
        let sums = format!("{}  {name}\n", crate::sha256_hex(&archive));
        let hosts = Hosts { api: "https://api.test".into(), ..Hosts::default() };
        let release = format!(
            r#"{{"tag_name": "v1.0.0", "assets": [{{"name": "{name}", "browser_download_url": "https://dl.test/{name}"}},
                {{"name": "SHA256SUMS", "browser_download_url": "https://dl.test/SHA256SUMS"}}]}}"#
        );
        let mut answers = BTreeMap::new();
        answers.insert(format!("https://api.test/repos/{REPOSITORY}/releases/latest"), release.into_bytes());
        answers.insert(format!("https://dl.test/{name}"), archive.clone());
        answers.insert("https://dl.test/SHA256SUMS".into(), sums.into_bytes());
        let fetch = Answers(answers);
        assert_eq!(newest(&fetch, &hosts).unwrap(), "v1.0.0");
        put_in_place(&archive, &dir).unwrap();
        assert!(!dir.join("stale.txt").exists(), "the old editor is gone");
        let editor = read(&dir).unwrap();
        assert!(editor.program().is_some());
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            let mode = std::fs::metadata(editor.program().unwrap()).unwrap().permissions().mode();
            assert_eq!(mode & 0o111, 0o111, "the program can run");
        }
        // A damaged download is refused before anything is changed.
        let mut bad = fetch.0.clone();
        bad.insert(format!("https://dl.test/{name}"), b"not it".to_vec());
        let release: Release =
            serde_json::from_slice(&bad[&format!("https://api.test/repos/{REPOSITORY}/releases/latest")]).unwrap();
        assert_eq!(release.assets.len(), 2);
        let sums = String::from_utf8(bad["https://dl.test/SHA256SUMS"].clone()).unwrap();
        assert!(crate::remote::verify(b"not it", name, None, Some(&sums)).is_err());
    }

    #[test]
    fn refuses_an_archive_that_reaches_outside_its_folder() {
        let home = tempfile::tempdir().unwrap();
        let archive = zip_of(&[("../evil.txt", b"x", 0o644)]);
        assert!(unpack(&archive, &home.path().join("x")).unwrap_err().contains("outside"));
        assert!(!home.path().join("evil.txt").exists());
    }

    #[test]
    fn reads_a_manifest_saved_on_windows() {
        // A byte order mark and \r\n line ends, as Notepad saves it.
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("openrac")).unwrap();
        let text = format!("\u{feff}{}", MANIFEST_TEXT.replace('\n', "\r\n"));
        std::fs::write(dir.path().join(MANIFEST), text).unwrap();
        assert_eq!(read(dir.path()).unwrap().manifest.version, "v1.0.0");
    }

    #[test]
    fn each_computer_has_its_own_archive() {
        let names: Vec<_> = [Platform::Linux, Platform::Windows, Platform::Macos].map(asset_name).into_iter().collect();
        for (name, system) in names.iter().zip(["linux", "windows", "macos"]) {
            let name = name.unwrap();
            assert!(name.ends_with(".zip") && name.contains(system), "{name}");
        }
        assert_eq!(names.iter().collect::<std::collections::BTreeSet<_>>().len(), 3);
        assert_eq!(asset_name(Platform::Android), None);
        assert_eq!(asset_name(Platform::Ios), None);
    }

    #[test]
    fn a_program_path_with_spaces_and_folders_is_found_on_any_system() {
        // The manifest's paths use /; the folders are joined with this system's separator.
        let root = Path::new("base");
        let path = crate::under(root, "OpenRAC Editor.app/Contents/MacOS/OpenRAC Editor");
        let want: PathBuf = ["base", "OpenRAC Editor.app", "Contents", "MacOS", "OpenRAC Editor"].iter().collect();
        assert_eq!(path, want);
        assert_eq!(path.components().count(), 5);
    }

    #[test]
    fn installs_under_a_folder_with_spaces_dots_and_letters_past_ascii() {
        // A Windows user folder such as C:\Users\Mateusz Kłysz, and a folder name with a dot
        // (the swap uses <name>.new and <name>.old beside it).
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("Mateusz Kłysz").join("Open RAC").join("editor.v2");
        let archive = zip_of(&[(MANIFEST, MANIFEST_TEXT.as_bytes(), 0o644), (program_key(), b"#!", 0o755)]);
        put_in_place(&archive, &dir).unwrap();
        put_in_place(&archive, &dir).unwrap();
        assert!(read(&dir).unwrap().program().is_some());
        let left: Vec<_> = std::fs::read_dir(dir.parent().unwrap()).unwrap().map(|e| e.unwrap().file_name()).collect();
        assert_eq!(left, vec![std::ffi::OsString::from("editor.v2")], "no .new or .old left beside it");
    }

    #[cfg(unix)]
    #[test]
    fn a_program_packed_without_unix_modes_still_runs() {
        // An archive made on Windows keeps no execute bit.
        use std::os::unix::fs::PermissionsExt;
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("editor");
        let archive = zip_of(&[(MANIFEST, MANIFEST_TEXT.as_bytes(), 0o644), (program_key(), b"#!", 0o644)]);
        put_in_place(&archive, &dir).unwrap();
        let program = read(&dir).unwrap().program().unwrap();
        assert_eq!(std::fs::metadata(program).unwrap().permissions().mode() & 0o111, 0o111);
    }

    #[test]
    fn refuses_names_with_backslashes() {
        let home = tempfile::tempdir().unwrap();
        let archive = zip_of(&[("openrac\\editor.json", MANIFEST_TEXT.as_bytes(), 0o644)]);
        assert!(unpack(&archive, &home.path().join("x")).unwrap_err().contains("in place of /"));
    }

    #[test]
    fn an_update_that_cannot_move_the_folder_keeps_the_old_editor() {
        // Windows will not move a folder while a program in it runs (the editor left open).
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("editor");
        let old = zip_of(&[(MANIFEST, MANIFEST_TEXT.as_bytes(), 0o644), (program_key(), b"old", 0o755)]);
        put_in_place(&old, &dir).unwrap();
        let new_text = MANIFEST_TEXT.replace("v1.0.0", "v2.0.0");
        let new = zip_of(&[(MANIFEST, new_text.as_bytes(), 0o644), (program_key(), b"new", 0o755)]);
        let in_use = || std::io::Error::new(std::io::ErrorKind::PermissionDenied, "in use");
        // Which move fails: the old folder aside, or the new one into its place.
        for failing in [0, 1] {
            let count = std::cell::Cell::new(0);
            let rename = |from: &Path, to: &Path| {
                let n = count.get();
                count.set(n + 1);
                if n == failing {
                    Err(in_use())
                } else {
                    std::fs::rename(from, to)
                }
            };
            let error = put_in_place_with(&new, &dir, &rename).unwrap_err();
            assert!(error.contains("close OpenRAC Editor first"), "{failing}: {error}");
            assert_eq!(read(&dir).unwrap().manifest.version, "v1.0.0", "{failing}: the old editor is still there");
            assert_eq!(std::fs::read(read(&dir).unwrap().program().unwrap()).unwrap(), b"old");
            let left: Vec<_> = std::fs::read_dir(home.path()).unwrap().map(|e| e.unwrap().file_name()).collect();
            assert_eq!(left, vec![std::ffi::OsString::from("editor")], "{failing}: nothing left beside it");
        }
        // Once the editor is closed the update goes through.
        put_in_place(&new, &dir).unwrap();
        assert_eq!(read(&dir).unwrap().manifest.version, "v2.0.0");
    }

    #[test]
    fn a_broken_download_leaves_nothing_behind() {
        let home = tempfile::tempdir().unwrap();
        let dir = home.path().join("editor");
        assert!(put_in_place(b"not a zip", &dir).is_err());
        let not_editor = zip_of(&[("readme.txt", b"hi", 0o644)]);
        assert!(put_in_place(&not_editor, &dir).unwrap_err().contains("not OpenRAC Editor"));
        assert_eq!(std::fs::read_dir(home.path()).unwrap().count(), 0);
    }
}
