//! The folders OpenRAC keeps the player's files in, as one list: Settings > Files shows them on
//! the desktop and on Android, and no other page opens a folder. The game's menu lists the same
//! places in its Files tab (port/game/common/program/file_places.cpp).
//!
//! Each place has an id; the page asks to open a place by its id, never by a path, so it cannot
//! open anything else.

use serde::Serialize;
use std::path::{Path, PathBuf};

/// Which group of the page a place is shown in.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "lowercase")]
#[ts(rename = "FileGroup")]
pub enum Group {
    /// The player's own files, the same for every game.
    Yours,
    /// The picked game version's.
    Game,
    /// Developer tools' output, shown with Developer tools on.
    Developer,
}

/// One folder, as the page shows it.
#[derive(Clone, Debug, Serialize, ts_rs::TS)]
#[ts(rename = "FilePlace")]
pub struct Place {
    pub id: &'static str,
    pub title: &'static str,
    pub help: &'static str,
    pub group: Group,
    /// Where it is, as the player reads it.
    pub path: String,
    /// How many files and folders it holds (at most 1000); 0 when it is empty or not there yet.
    pub count: u32,
    #[serde(skip)]
    pub folder: PathBuf,
}

/// What the places are found from.
pub struct Inputs<'a> {
    /// OpenRAC's data folder (saves::data_home).
    pub data_home: &'a Path,
    /// The folder the player's own files go next to the data folder on Android (the app's files,
    /// HOME): the Files app shows its screenshots/ and perf/ by name. None on a desktop.
    pub android_home: Option<&'a Path>,
    /// The picked version: its game, its version and its set-up folder (install::data_dir).
    pub version: Option<(&'a str, &'a str, &'a Path)>,
    /// Developer tools are on: their group is listed.
    pub developer: bool,
    /// The OpenRAC checkout the launcher runs from, when it is not an installed one: the level
    /// editor's folders are its assets/.
    pub checkout: Option<&'a Path>,
}

/// `name` where the player's own files go: the data folder, or the app's files on Android.
fn own(inputs: &Inputs, name: &str) -> PathBuf {
    inputs.android_home.unwrap_or(inputs.data_home).join(name)
}

fn count(folder: &Path) -> u32 {
    std::fs::read_dir(folder).map(|entries| entries.take(1000).count() as u32).unwrap_or(0)
}

fn place(id: &'static str, title: &'static str, help: &'static str, group: Group, folder: PathBuf) -> Place {
    Place { id, title, help, group, path: folder.display().to_string(), count: count(&folder), folder }
}

/// Every place for `inputs`, in the order the page shows them.
pub fn places(inputs: &Inputs) -> Vec<Place> {
    let home = inputs.data_home;
    let mut out = vec![
        place("screenshots", "Screenshots", "Pictures you took in the game.", Group::Yours, own(inputs, "screenshots")),
        place("saves", "Saves", "Each game's memory card.", Group::Yours, crate::saves::card_root_in(home)),
        place("settings", "Settings", "Each game's settings file.", Group::Yours, home.join("settings")),
        place("controls", "Controls", "Your controls and shortcuts.", Group::Yours, home.join("controls")),
        place("mod-files", "Mod files", "Each mod's own files.", Group::Yours, home.join("mod-data")),
        place(
            "recordings",
            "Diagnostics recordings",
            "Gameplay recordings and their reports.",
            Group::Yours,
            own(inputs, "perf"),
        ),
    ];
    // A game and version are plain names ("rac1", "pal"): anything else lists no game folder, so
    // no place can lead outside them ("../..").
    let version = inputs.version.and_then(|(game, version, data)| {
        crate::texture_packs::id(game, version).ok().map(|id| (game, version, data, id))
    });
    if let Some((game, version, data, id)) = version {
        out.push(place(
            "texture-packs",
            "Texture packs",
            "This game's texture packs.",
            Group::Game,
            crate::texture_packs::pack_dir(data, &id),
        ));
        out.push(place(
            "saved-textures",
            "Saved textures",
            "Textures saved to make a pack.",
            Group::Game,
            crate::texture_packs::dump_dir(home, &id),
        ));
        out.push(place("mods", "Mods", "This game's mods, a folder each.", Group::Game, data.join("plugins")));
        out.push(place(
            "exports",
            "Exports",
            "Models, textures and sounds you exported.",
            Group::Game,
            data.join("exports"),
        ));
        out.push(place(
            "overrides",
            "Overrides",
            "Replaced files the game reads over the disc's.",
            Group::Game,
            data.join("overrides"),
        ));
        out.push(place(
            "logs",
            "Logs and crash reports",
            "A log each time you play, crash reports in crashes.",
            Group::Game,
            data.to_path_buf(),
        ));
        if inputs.developer {
            let (levels, packed) = match inputs.checkout {
                Some(root) => (root.join("assets/levels").join(game), root.join("assets/packed").join(game)),
                None => (data.join("levels"), data.join("packed")),
            };
            out.push(place(
                "editor-projects",
                "Editor projects",
                "The level editor's projects.",
                Group::Developer,
                levels.join(version),
            ));
            out.push(place(
                "packed-levels",
                "Packed levels",
                "Levels packed from the editor.",
                Group::Developer,
                packed.join(version),
            ));
        }
    }
    out
}

/// The folder of place `id`, made if it is not there yet; an error for an id the list has not.
pub fn folder(inputs: &Inputs, id: &str) -> Result<PathBuf, String> {
    let place =
        places(inputs).into_iter().find(|p| p.id == id).ok_or_else(|| format!("{id} is not a place of your files"))?;
    std::fs::create_dir_all(&place.folder).map_err(|e| format!("{}: {e}", place.folder.display()))?;
    Ok(place.folder)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lists_the_places_by_group() {
        let home = tempfile::tempdir().unwrap();
        let data = home.path().join("active/rac1/data");
        let inputs = Inputs {
            data_home: home.path(),
            android_home: None,
            version: Some(("rac1", "ntsc", &data)),
            developer: false,
            checkout: None,
        };
        let list = places(&inputs);
        let ids: Vec<&str> = list.iter().map(|p| p.id).collect();
        assert_eq!(
            ids,
            [
                "screenshots",
                "saves",
                "settings",
                "controls",
                "mod-files",
                "recordings",
                "texture-packs",
                "saved-textures",
                "mods",
                "exports",
                "overrides",
                "logs"
            ]
        );
        assert_eq!(list[0].folder, home.path().join("screenshots"));
        assert_eq!(list[6].folder, data.join("texture_packs").join("rac1-ntsc"));
        assert!(list.iter().all(|p| p.group != Group::Developer));
    }

    #[test]
    fn developer_group_only_with_developer_tools() {
        let home = tempfile::tempdir().unwrap();
        let data = home.path().join("data");
        let inputs = Inputs {
            data_home: home.path(),
            android_home: None,
            version: Some(("rac1", "pal", &data)),
            developer: true,
            checkout: None,
        };
        assert!(places(&inputs).iter().any(|p| p.id == "editor-projects" && p.group == Group::Developer));
        let root = home.path().join("OpenRAC");
        let inputs = Inputs { checkout: Some(&root), ..inputs };
        let projects = places(&inputs).into_iter().find(|p| p.id == "editor-projects").unwrap();
        assert_eq!(projects.folder, root.join("assets/levels/rac1/pal"));
    }

    #[test]
    fn android_keeps_screenshots_in_the_app_files() {
        let home = tempfile::tempdir().unwrap();
        let app = home.path().join("files");
        let share = app.join("share/openrac");
        let inputs =
            Inputs { data_home: &share, android_home: Some(&app), version: None, developer: false, checkout: None };
        let list = places(&inputs);
        assert_eq!(list[0].folder, app.join("screenshots"));
        assert_eq!(list.iter().find(|p| p.id == "recordings").unwrap().folder, app.join("perf"));
        assert_eq!(list.iter().find(|p| p.id == "saves").unwrap().folder, share.join("memcard"));
    }

    #[test]
    fn a_version_that_is_not_a_plain_name_lists_no_game_folder() {
        let home = tempfile::tempdir().unwrap();
        let data = home.path().join("data");
        for (game, version) in [("rac1", "../../escape"), ("../x", "pal"), ("rac1", ""), ("rac1", "pal/..")] {
            let inputs = Inputs {
                data_home: home.path(),
                android_home: None,
                version: Some((game, version, &data)),
                developer: true,
                checkout: None,
            };
            assert!(places(&inputs).iter().all(|p| p.group == Group::Yours), "{game}/{version}");
            assert!(folder(&inputs, "editor-projects").is_err());
        }
        assert!(!home.path().parent().unwrap().join("escape").exists());
    }

    #[test]
    fn opens_only_listed_places_and_makes_them() {
        let home = tempfile::tempdir().unwrap();
        let inputs =
            Inputs { data_home: home.path(), android_home: None, version: None, developer: false, checkout: None };
        let dir = folder(&inputs, "screenshots").unwrap();
        assert!(dir.is_dir());
        assert!(folder(&inputs, "../etc").is_err());
        // A file where the folder goes: said, not opened.
        std::fs::write(home.path().join("settings"), "x").unwrap();
        assert!(folder(&inputs, "settings").is_err());
        assert!(folder(&inputs, "logs").is_err(), "no version picked: no game places");
    }
}
