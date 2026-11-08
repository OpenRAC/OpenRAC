//! Starting one game straight from a shortcut: the launcher's `--play <id>` argument, and the
//! `.desktop` file on the Linux desktop that passes it. Steam's per-game entries (steam.rs) hand
//! the launcher the same argument.
//!
//! `<id>` is the game's folder name under `port/game/` (`rac1-pal`), a version key with its slash
//! made a dash, so it is safe in a file name and in Steam's launch options. The key itself
//! (`rac1/pal`) is read too.

use std::path::{Path, PathBuf};

use crate::catalog::Catalog;
use crate::desktop::Places;
use crate::detect;

const FLAG: &str = "--play";

/// The launch options that start game `id`.
pub fn play_options(id: &str) -> String {
    format!("{FLAG} {id}")
}

/// A version key as the id shortcuts carry: `rac1/pal` is `rac1-pal`.
pub fn game_id(key: &str) -> String {
    key.replace('/', "-")
}

/// The game a start asks for: the value after `--play`, if it is one, or a game by its name alone
/// (`--rac1`, `--rac2`: the launch options a player types in Steam or a shortcut themselves).
pub fn play_arg<S: AsRef<str>>(args: &[S]) -> Option<String> {
    if let Some(at) = args.iter().skip(1).position(|a| a.as_ref() == FLAG) {
        let value = args.get(at + 2)?.as_ref();
        return (!value.is_empty() && !value.starts_with('-')).then(|| value.to_string());
    }
    args.iter().skip(1).find_map(|a| game_flag(a.as_ref()))
}

/// `--rac<n>`: the game `rac<n>`.
fn game_flag(arg: &str) -> Option<String> {
    let game = arg.strip_prefix("--")?;
    let number = game.strip_prefix("rac")?;
    (!number.is_empty() && number.bytes().all(|b| b.is_ascii_digit())).then(|| game.to_string())
}

/// The version key `arg` names (an id or a key), if the catalogue has it.
pub fn version_for_arg(catalog: &Catalog, arg: &str) -> Option<String> {
    catalog.version_keys().into_iter().find(|key| key == arg || game_id(key) == arg)
}

/// The version a start's `arg` plays: the version it names, or for a game's name (`rac1`) the one
/// the launcher plays of it (disc_choice.rs `played_version`).
pub fn version_to_play(config: &crate::config::Config, catalog: &Catalog, arg: &str) -> Option<String> {
    version_for_arg(catalog, arg).or_else(|| crate::disc_choice::played_version(config, catalog, arg))
}

/// A shortcut's `.desktop` file: `title`, the icon `icon`, starting `program --play <id>`.
pub fn shortcut_entry(title: &str, icon: &str, program: &Path, id: &str) -> String {
    format!(
        "[Desktop Entry]\nType=Application\nName={title}\nComment=Play {title} with OpenRAC\nExec={} {}\nIcon={icon}\nTerminal=false\nCategories=Game;\n",
        detect::desktop_exec(program),
        play_options(id)
    )
}

/// The user's Desktop folder: what `xdg-user-dir` says, else `~/Desktop`.
pub fn desktop_dir(home: &Path) -> PathBuf {
    let said = detect::quiet(Path::new("xdg-user-dir")).arg("DESKTOP").output().ok();
    said.filter(|o| o.status.success())
        .map(|o| PathBuf::from(String::from_utf8_lossy(&o.stdout).trim()))
        .filter(|p| p.is_absolute() && p.is_dir())
        .unwrap_or_else(|| home.join("Desktop"))
}

/// A file name for `title`: no path characters.
fn file_name(title: &str) -> String {
    let clean: String = title.chars().map(|c| if matches!(c, '/' | '\\' | ':' | '\0') { '-' } else { c }).collect();
    format!("{clean}.desktop")
}

/// Writes the shortcut for game `id` into `dir`, with its icon (when there is one) in `places`'
/// icon theme. The file is made executable, which some desktops need before they run it. Returns
/// its path.
pub fn write_shortcut(
    dir: &Path,
    places: &Places,
    title: &str,
    id: &str,
    program: &Path,
    icon: Option<&[u8]>,
) -> Result<PathBuf, String> {
    let failed = |e: std::io::Error| format!("The shortcut can't be written: {e}.");
    let icon_name = match icon {
        Some(bytes) => {
            let icons = places.icons.join("512x512/apps");
            std::fs::create_dir_all(&icons).map_err(failed)?;
            std::fs::write(icons.join(format!("dev.openrac.{id}.png")), bytes).map_err(failed)?;
            format!("dev.openrac.{id}")
        }
        None => "dev.openrac.launcher".to_string(),
    };
    std::fs::create_dir_all(dir).map_err(failed)?;
    let path = dir.join(file_name(title));
    std::fs::write(&path, shortcut_entry(title, &icon_name, program, id)).map_err(failed)?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        std::fs::set_permissions(&path, std::fs::Permissions::from_mode(0o755)).map_err(failed)?;
    }
    Ok(path)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn args(list: &[&str]) -> Vec<String> {
        list.iter().map(|s| s.to_string()).collect()
    }

    #[test]
    fn the_game_is_read_after_play() {
        assert_eq!(play_arg(&args(&["x", "--play", "rac1-pal"])), Some("rac1-pal".into()));
        assert_eq!(play_arg(&args(&["x", "--play"])), None);
        assert_eq!(play_arg(&args(&["x", "--play", "--other"])), None);
        assert_eq!(play_arg(&args(&["--play", "rac1-pal"])), None, "the program's own name is not an option");
        assert_eq!(play_arg(&args(&["x", "--", "openrac://install/a"])), None);
    }

    #[test]
    fn a_game_is_named_alone_by_its_flag() {
        assert_eq!(play_arg(&args(&["x", "--rac1"])), Some("rac1".into()));
        assert_eq!(play_arg(&args(&["x", "--window", "--rac12"])), Some("rac12".into()));
        assert_eq!(play_arg(&args(&["x", "--rac2", "--play", "rac1-pal"])), Some("rac1-pal".into()), "--play first");
        for not in ["--rac", "--racx", "--rac1-pal", "-rac1", "rac1", "--rac1 "] {
            assert_eq!(play_arg(&args(&["x", not])), None, "{not}");
        }
        assert_eq!(play_arg(&args(&["--rac1"])), None, "the program's own name is not an option");
    }

    #[test]
    fn a_games_name_plays_the_version_set_up_or_its_first() {
        let catalog = crate::catalog::load(&crate::testing::checkout()).unwrap();
        let game = &catalog.games.iter().find(|g| g.versions.len() > 1).expect("a game with two versions");
        let (first, second) = (game.versions[0].key.clone(), game.versions[1].key.clone());
        let config = crate::config::Config::default();
        assert_eq!(version_to_play(&config, &catalog, &game.id), Some(first.clone()));
        assert_eq!(version_to_play(&config, &catalog, &game_id(&second)), Some(second.clone()), "a version, as named");
        assert_eq!(version_to_play(&config, &catalog, "rac99"), None);
        // The disc set up from the player's own.
        let install = tempfile::tempdir().unwrap();
        let info = crate::install::data_dir(install.path(), &game.id).join("iso_data").join(&game.id);
        std::fs::create_dir_all(&info).unwrap();
        let text = format!(r#"[{{"serial": "S", "version": "{second}", "elf_sha1": "0", "files": 1, "image": "i"}}]"#);
        std::fs::write(info.join("buildinfo.json"), text).unwrap();
        let set_up = crate::config::Config { install_dir: Some(install.path().into()), ..config.clone() };
        assert_eq!(version_to_play(&set_up, &catalog, &game.id), Some(second.clone()));
        // A developer's choice of disc is the one played.
        let mut chose = config.clone();
        chose.disc_choice.insert(game.id.clone(), second.clone());
        assert_eq!(version_to_play(&chose, &catalog, &game.id), Some(first), "only with developer tools on");
        chose.developer = true;
        assert_eq!(version_to_play(&chose, &catalog, &game.id), Some(second));
    }

    #[test]
    fn an_id_or_a_key_names_a_version() {
        let catalog = crate::catalog::load(&crate::testing::checkout()).unwrap();
        let key = catalog.version_keys().remove(0);
        assert_eq!(version_for_arg(&catalog, &game_id(&key)), Some(key.clone()));
        assert_eq!(version_for_arg(&catalog, &key), Some(key));
        assert_eq!(version_for_arg(&catalog, "nope-pal"), None);
        assert_eq!(version_for_arg(&catalog, "../x"), None);
    }

    #[test]
    fn the_entry_starts_the_launcher_with_the_game() {
        let text = shortcut_entry("Game (PAL)", "dev.openrac.g-pal", Path::new("/a b/OpenRAC.AppImage"), "g-pal");
        assert!(text.contains("Exec=\"/a b/OpenRAC.AppImage\" --play g-pal\n"), "{text}");
        assert!(text.contains("Name=Game (PAL)\n") && text.contains("Icon=dev.openrac.g-pal\n"));
    }

    #[test]
    fn a_shortcut_is_written_executable_with_its_icon() {
        let dir = tempfile::tempdir().unwrap();
        let places = Places::in_home(dir.path());
        let desk = dir.path().join("Desktop");
        let path = write_shortcut(&desk, &places, "A/B", "g-pal", Path::new("/opt/openrac"), Some(b"png")).unwrap();
        assert_eq!(path, desk.join("A-B.desktop"));
        assert!(std::fs::read_to_string(&path).unwrap().contains("--play g-pal"));
        assert_eq!(std::fs::read(places.icons.join("512x512/apps/dev.openrac.g-pal.png")).unwrap(), b"png");
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            assert_eq!(std::fs::metadata(&path).unwrap().permissions().mode() & 0o111, 0o111);
        }
        let plain = write_shortcut(&desk, &places, "C", "c", Path::new("/o"), None).unwrap();
        assert!(std::fs::read_to_string(plain).unwrap().contains("Icon=dev.openrac.launcher"));
    }
}
