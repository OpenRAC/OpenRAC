//! The webview's own environment, set before the window opens.
//!
//! The launcher's window is a WebKitGTK webview, and two of WebKitGTK's
//! settings draw nothing on some systems:
//!
//! - The DMA-BUF renderer draws a blank or flickering window under Wayland
//!   with NVIDIA's own driver. There its buffers go through shared memory
//!   instead, and only there. Turning the renderer off
//!   (`WEBKIT_DISABLE_DMABUF_RENDERER=1`) leaves WebKitGTK 2.50 without a
//!   backing store, and it crashes on the first accelerated draw (a click).
//!   Shared memory alone still crashes, on a desktop and on a laptop with AMD
//!   and NVIDIA, so there the accelerated compositing mode is turned off too.
//! - The accelerated compositing mode draws nothing at all on a Steam Deck:
//!   the window opens empty, and the launcher has to be started with
//!   `WEBKIT_DISABLE_COMPOSITING_MODE=1` for anything to show. A Deck is
//!   told apart by its own file (`/etc/os-release` says `steamos`) and by
//!   gamescope, which says so in the environment of the programs it runs.
//!
//! On a system none of these name, the player's Safe rendering setting
//! (`Config::safe_rendering`) turns the accelerated compositing mode off. The
//! launcher turns it on by itself after a start whose window crashed before
//! it drew a click: a [`DrawCheck`] file stays behind.

use std::path::{Path, PathBuf};

/// The variables the webview gets, when the player has not set them. `safe`:
/// the player's Safe rendering setting.
pub fn webkit_env(safe: bool) -> Vec<(&'static str, &'static str)> {
    env_for(wayland(), nvidia(), steamos(), under_gamescope(), safe)
}

/// The variables the webview gets, given what the system is.
fn env_for(wayland: bool, nvidia: bool, steamos: bool, gamescope: bool, safe: bool) -> Vec<(&'static str, &'static str)> {
    let mut vars = Vec::new();
    if wayland && nvidia {
        vars.push(("WEBKIT_DMABUF_RENDERER_FORCE_SHM", "1"));
    }
    if (wayland && nvidia) || steamos || gamescope || safe {
        vars.push(("WEBKIT_DISABLE_COMPOSITING_MODE", "1"));
    }
    vars
}

/// Whether the system is SteamOS: `/etc/os-release` says so.
fn steamos() -> bool {
    std::fs::read_to_string("/etc/os-release").map(|text| is_steamos(&text)).unwrap_or(false)
}

fn is_steamos(release: &str) -> bool {
    release.lines().any(|line| {
        let mut parts = line.splitn(2, '=');
        parts.next() == Some("ID") && parts.next().map(|value| value.trim().trim_matches('"')) == Some("steamos")
    })
}

/// Whether the programs run under gamescope, which says so in the environment.
fn under_gamescope() -> bool {
    std::env::var_os("GAMESCOPE_WAYLAND_DISPLAY").is_some()
}

fn wayland() -> bool {
    std::env::var_os("WAYLAND_DISPLAY").is_some()
}

fn nvidia() -> bool {
    Path::new("/proc/driver/nvidia/version").exists()
}

/// The per-user config folder the app keeps launcher.json in, found before the app exists: what
/// Tauri's `app_config_dir` gives on Linux (`$XDG_CONFIG_HOME`, else `~/.config`, then the app's
/// identifier).
pub fn config_dir(identifier: &str) -> Option<PathBuf> {
    let base = std::env::var_os("XDG_CONFIG_HOME")
        .map(PathBuf::from)
        .filter(|dir| dir.is_absolute())
        .or_else(|| std::env::var_os("HOME").map(|home| PathBuf::from(home).join(".config")))?;
    Some(base.join(identifier))
}

/// A start of the launcher that has not yet shown its window can draw: a file holding the
/// launcher's process id, written before the window opens and removed once a click was drawn,
/// or at a quit that came before any click. Found at the next start, its launcher crashed.
pub struct DrawCheck {
    file: PathBuf,
}

/// What the file of a [`DrawCheck`] said at a start.
#[derive(Debug, PartialEq, Eq)]
pub enum LastStart {
    /// No file: the last start drew, or never was.
    Drew,
    /// The file of a launcher that is not running: it crashed before it drew a click.
    Crashed,
    /// The file of a launcher still running: this start is a second one, which shows the first.
    Running,
}

impl DrawCheck {
    pub fn new(dir: &Path) -> Self {
        Self { file: dir.join("webview-check") }
    }

    /// What the last start left behind.
    pub fn last_start(&self) -> LastStart {
        let Ok(text) = std::fs::read_to_string(&self.file) else { return LastStart::Drew };
        match text.trim().parse::<u32>() {
            Ok(pid) if pid != std::process::id() && Path::new(&format!("/proc/{pid}")).exists() => LastStart::Running,
            _ => LastStart::Crashed,
        }
    }

    /// This start has not drawn yet.
    pub fn begin(&self) {
        if let Some(dir) = self.file.parent() {
            let _ = std::fs::create_dir_all(dir);
        }
        let _ = std::fs::write(&self.file, std::process::id().to_string());
    }

    /// No check, whoever's file it was.
    pub fn clear(&self) {
        let _ = std::fs::remove_file(&self.file);
    }

    /// This start drew a click, or quit before any: its file goes, if it is still its own.
    pub fn end(&self) {
        let own = std::fs::read_to_string(&self.file).is_ok_and(|text| text.trim() == std::process::id().to_string());
        if own {
            let _ = std::fs::remove_file(&self.file);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_decks_webview_draws_without_the_accelerated_compositing_mode() {
        let vars = env_for(true, false, true, false, false);
        assert_eq!(vars, vec![("WEBKIT_DISABLE_COMPOSITING_MODE", "1")]);
        let vars = env_for(false, false, false, true, false);
        assert_eq!(vars, vec![("WEBKIT_DISABLE_COMPOSITING_MODE", "1")]);
    }

    #[test]
    fn an_ordinary_desktop_gets_nothing_turned_off() {
        assert!(env_for(false, false, false, false, false).is_empty());
        assert!(env_for(true, false, false, false, false).is_empty());
    }

    #[test]
    fn wayland_with_nvidia_uses_shared_memory_without_the_accelerated_compositing_mode() {
        let vars = env_for(true, true, false, false, false);
        assert_eq!(vars, vec![("WEBKIT_DMABUF_RENDERER_FORCE_SHM", "1"), ("WEBKIT_DISABLE_COMPOSITING_MODE", "1")]);
        assert!(env_for(false, true, false, false, false).is_empty());
    }

    #[test]
    fn safe_rendering_turns_the_accelerated_compositing_mode_off_anywhere() {
        assert_eq!(env_for(false, false, false, false, true), vec![("WEBKIT_DISABLE_COMPOSITING_MODE", "1")]);
        let vars = env_for(true, true, false, false, true);
        assert_eq!(vars, vec![("WEBKIT_DMABUF_RENDERER_FORCE_SHM", "1"), ("WEBKIT_DISABLE_COMPOSITING_MODE", "1")]);
    }

    #[test]
    fn a_start_that_never_drew_is_found_at_the_next() {
        let dir = std::env::temp_dir().join(format!("openrac-draw-check-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        let check = DrawCheck::new(&dir);
        assert_eq!(check.last_start(), LastStart::Drew);
        check.begin();
        check.end();
        assert_eq!(check.last_start(), LastStart::Drew);
        // A launcher that is gone left its file: no process has id 0 in /proc.
        std::fs::write(dir.join("webview-check"), "0").unwrap();
        assert_eq!(check.last_start(), LastStart::Crashed);
        check.end();
        assert_eq!(check.last_start(), LastStart::Crashed, "another start's file stays");
        // A launcher still running (init) is a second start's, not a crash.
        std::fs::write(dir.join("webview-check"), "1").unwrap();
        assert_eq!(check.last_start(), LastStart::Running);
        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn steamos_is_told_apart_by_its_own_release_file() {
        assert!(is_steamos("NAME=\"SteamOS\"\nID=steamos\nID_LIKE=arch\n"));
        assert!(is_steamos("ID=\"steamos\"\n"));
        assert!(!is_steamos("NAME=\"Fedora Linux\"\nID=fedora\nVERSION_ID=42\n"));
        assert!(!is_steamos("ID=steamosx\n"));
    }
}
