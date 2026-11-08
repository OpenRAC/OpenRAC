//! For developers only: when the launcher knows more than one disc of a game (PAL and NTSC-U),
//! which of them the game's page plays. A player never sees this: without developer tools on, or
//! with one known disc of a game, nothing here applies and the launcher picks the disc as it
//! always has (the one set up, else the first found; `playerVersion` in `src/lib/player.svelte.ts`).
//!
//! The discs a developer's launcher knows ([`known`]) are those in the games folder, the images
//! each version was set up from (`disc_images` in the settings, [`remember`]) while they are still
//! there and still that disc, and every version with a set-up in the install folder, the one in
//! place or one kept aside. So the switch works without a games folder, for discs picked one at
//! a time.
//!
//! Each version keeps what is its own. The levels, the build, the mods, the memory card and the
//! logs already have a folder or a name per version; only what the set-up copies from the disc
//! (`iso_data/<game>`, and `decompiler_out/<game>` and `out/<game>` once there are any) is one
//! folder per game, which the port reads. Switching moves that folder aside, as
//! `<game>@<version>` beside it (`iso_data/rac1@pal`), and moves the chosen version's back, so
//! nothing is read from the disc again and nothing of the other version is deleted:
//!
//! ```text
//! <install dir>/active/rac1/data/
//!     iso_data/rac1/          the disc being played (what the port reads), NTSC-U here
//!     iso_data/rac1@pal/      PAL's set-up, kept for when PAL is chosen again
//! ```

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};

use serde::Serialize;

use crate::catalog::Catalog;
use crate::config::Config;
use crate::discs::{self, FoundDisc};
use crate::install;

/// The folders of a game's set-up that hold one version at a time (OpenGOAL's layout).
const SET_UP: [&str; 3] = ["iso_data", "decompiler_out", "out"];

/// A game the launcher knows more than one disc of, for a developer to pick from.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct DiscChoice {
    pub game: String,
    /// The versions known (`rac1/pal`, `rac1/ntsc`), in the catalogue's order.
    pub versions: Vec<String>,
    /// The one the game's page plays.
    pub chosen: String,
    /// The image of each version whose image is known (the games folder's, else the one it was
    /// set up from), to set it up from when it has no set-up; a version known only by its set-up
    /// has none.
    pub images: BTreeMap<String, PathBuf>,
}

/// The version of `game` whose disc is set up now (its `buildinfo.json`).
fn set_up_version(config: &Config, game: &str) -> Option<String> {
    let dir = config.install_dir.as_deref()?;
    install::state(dir, game).extracted.map(|b| b.version)
}

/// The version of `game` the launcher plays (a start by the game's name, `--rac1`): the
/// developer's choice of disc, else the one set up, else the game's first. None: no such game.
pub fn played_version(config: &Config, catalog: &Catalog, game: &str) -> Option<String> {
    let versions = &catalog.games.iter().find(|g| g.id == game)?.versions;
    let known = |key: Option<String>| key.filter(|k| versions.iter().any(|v| &v.key == k));
    let chosen = config.developer.then(|| config.disc_choice.get(game).cloned()).flatten();
    known(chosen).or_else(|| known(set_up_version(config, game))).or_else(|| versions.first().map(|v| v.key.clone()))
}

/// `version` has a set-up in `install`: the one in place, or one kept aside ([`kept`]).
fn has_set_up(install: &Path, game: &str, version: &str) -> bool {
    let is = |dir: &Path| install::build_info(dir).is_some_and(|b| b.version == version);
    is(&install::data_dir(install, game).join("iso_data").join(game)) || is(&kept(install, "iso_data", game, version))
}

/// The version `image` is, as the catalogue knows it (`None`: not there, or not a disc it knows).
fn version_of_image(catalog: &Catalog, game: &str, image: &Path) -> Option<String> {
    let id = discs::identity_of(image)?;
    catalog.version_of_disc(Some(game), &id.serial, id.version.as_deref()).map(|v| v.key.clone())
}

/// Every disc of `game` the launcher knows, by version in the catalogue's order, with its image
/// when one is known: one found in the games folder (`found`), else the one the version was set
/// up from (`disc_images`) while it is still that disc; a version with a set-up and neither is
/// known without an image.
pub fn known(config: &Config, catalog: &Catalog, found: &[FoundDisc], game: &str) -> Vec<(String, Option<PathBuf>)> {
    let Some(entry) = catalog.games.iter().find(|g| g.id == game) else { return Vec::new() };
    let mut out = Vec::new();
    for v in &entry.versions {
        let image = found.iter().find(|f| f.version == v.key).map(|f| f.path.clone()).or_else(|| {
            config
                .disc_images
                .get(&v.key)
                .filter(|path| path.is_file() && version_of_image(catalog, game, path).as_deref() == Some(&v.key))
                .cloned()
        });
        let set_up = || config.install_dir.as_deref().is_some_and(|dir| has_set_up(dir, game, &v.key));
        if image.is_some() || set_up() {
            out.push((v.key.clone(), image));
        }
    }
    out
}

/// The games a developer can switch discs for: developer tools on, and at least two versions of
/// the game among the discs known ([`known`]). The choice remembered in the settings when it is
/// still known, else the disc set up, else the first known: what the launcher plays without a
/// choice.
pub fn choices(config: &Config, catalog: &Catalog, found: &[FoundDisc]) -> Vec<DiscChoice> {
    if !config.developer {
        return Vec::new();
    }
    let mut out = Vec::new();
    for game in &catalog.games {
        let discs = known(config, catalog, found, &game.id);
        if discs.len() < 2 {
            continue;
        }
        let versions: Vec<String> = discs.iter().map(|(v, _)| v.clone()).collect();
        let pick = |key: Option<&str>| key.filter(|k| versions.iter().any(|v| v == k)).map(str::to_string);
        let chosen = pick(config.disc_choice.get(&game.id).map(String::as_str))
            .or_else(|| pick(set_up_version(config, &game.id).as_deref()))
            .unwrap_or_else(|| versions[0].clone());
        let images = discs.into_iter().filter_map(|(v, image)| Some((v, image?))).collect();
        out.push(DiscChoice { game: game.id.clone(), versions, chosen, images });
    }
    out
}

/// A developer set `game` up from `image`: remembers it as the image of the version it is, so the
/// disc switch knows that disc without a games folder. Off for players. Returns whether `config`
/// changed (the caller saves it).
pub fn remember(config: &mut Config, catalog: &Catalog, game: &str, image: &Path) -> bool {
    if !config.developer {
        return false;
    }
    let Some(version) = version_of_image(catalog, game, image) else { return false };
    let image = std::fs::canonicalize(image).unwrap_or_else(|_| image.to_path_buf());
    config.disc_images.insert(version, image.clone()) != Some(image)
}

/// Plays `version`'s disc for its game from now on: refused unless that choice is offered
/// ([`choices`]). Puts that version's set-up in place (switch) and remembers the choice in
/// `config`, which the caller saves.
pub fn choose(config: &mut Config, catalog: &Catalog, found: &[FoundDisc], version: &str) -> Result<(), String> {
    let v = catalog.version(version).ok_or_else(|| format!("no version {version}"))?;
    let offered = choices(config, catalog, found);
    if !offered.iter().any(|c| c.game == v.game && c.versions.iter().any(|k| k == version)) {
        return Err(format!("{version} cannot be chosen: developer tools are off or its disc is not known"));
    }
    if let Some(install) = config.install_dir.as_deref() {
        switch(install, &v.game, version)?;
    }
    config.disc_choice.insert(v.game.clone(), version.to_string());
    Ok(())
}

/// Where `version`'s set-up of `game` waits while another is played: `<sub>/<game>@<name>`.
pub fn kept(install: &Path, sub: &str, game: &str, version: &str) -> PathBuf {
    let name = version.rsplit('/').next().unwrap_or(version);
    install::data_dir(install, game).join(sub).join(format!("{game}@{name}"))
}

/// Makes `version` the set-up of `game` that is played: the one in place now moves aside
/// ([`kept`]) and `version`'s, if it was set up before, comes back. Nothing is deleted or read
/// again; a version never set up is left to set up (the folder is then free).
pub fn switch(install: &Path, game: &str, version: &str) -> Result<(), String> {
    let data = install::data_dir(install, game);
    let current = install::state(install, game).extracted.map(|b| b.version);
    if current.as_deref() == Some(version) {
        return Ok(());
    }
    let active = |sub: &str| data.join(sub).join(game);
    if SET_UP.iter().any(|sub| active(sub).exists()) {
        let Some(current) = current else {
            return Err(format!("the set-up in {} is not complete: remove the set-up first", data.display()));
        };
        for sub in SET_UP {
            let to = kept(install, sub, game, &current);
            if active(sub).exists() && to.exists() {
                return Err(format!("{} is already there: move it away first", to.display()));
            }
        }
        for sub in SET_UP {
            if active(sub).exists() {
                let to = kept(install, sub, game, &current);
                std::fs::rename(active(sub), &to).map_err(|e| format!("cannot move {}: {e}", active(sub).display()))?;
            }
        }
    }
    for sub in SET_UP {
        let from = kept(install, sub, game, version);
        if from.exists() {
            std::fs::rename(&from, active(sub)).map_err(|e| format!("cannot move {}: {e}", from.display()))?;
        }
    }
    Ok(())
}

/// Before a developer sets `game` up from `image`: when another version's disc is set up, it is
/// kept aside ([`switch`]) instead of being replaced by the extractor. Off for players: their
/// set-up replaces the one before, as it always has.
pub fn before_set_up(config: &Config, catalog: &Catalog, game: &str, image: &Path) -> Result<(), String> {
    let Some(install) = config.install_dir.as_deref().filter(|_| config.developer) else { return Ok(()) };
    let Some(id) = discs::identity_of(image) else { return Ok(()) };
    let Some(version) = catalog.version_of_disc(Some(game), &id.serial, id.version.as_deref()) else {
        return Ok(());
    };
    match install::state(install, game).extracted {
        Some(b) if b.version != version.key => switch(install, game, &version.key),
        _ => Ok(()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::checkout;

    fn disc(version: &str, serial: &str) -> FoundDisc {
        FoundDisc {
            path: format!("/games/{serial}.iso").into(),
            serial: serial.into(),
            game: "rac1".into(),
            version: version.into(),
            title: "Ratchet & Clank".into(),
            region: String::new(),
        }
    }

    fn both() -> Vec<FoundDisc> {
        vec![disc("rac1/ntsc", "SCUS_971.99"), disc("rac1/pal", "SCES_509.16")]
    }

    /// A set-up of `version` in `install`, as the extractor leaves it, with a file of its own.
    fn set_up(install: &Path, version: &str, serial: &str) {
        let dir = install::data_dir(install, "rac1").join("iso_data").join("rac1");
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(dir.join("disc.iso"), version).unwrap();
        std::fs::write(
            dir.join("buildinfo.json"),
            format!(
                r#"[{{"serial": "{serial}", "version": "{version}", "elf_sha1": "ab", "files": 3, "image": "x.iso"}}]"#
            ),
        )
        .unwrap();
    }

    fn played(install: &Path) -> String {
        std::fs::read_to_string(install::data_dir(install, "rac1").join("iso_data/rac1/disc.iso")).unwrap()
    }

    #[test]
    fn a_developer_with_both_discs_can_choose_and_the_choice_is_kept() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path();
        set_up(install, "rac1/pal", "SCES_509.16");
        let mut config = Config { developer: true, install_dir: Some(install.into()), ..Config::default() };

        // Offered, in the catalogue's order, with the disc set up as the default.
        let offered = choices(&config, &catalog, &both());
        assert_eq!(offered.len(), 1, "{offered:?}");
        assert_eq!(offered[0].versions, ["rac1/pal", "rac1/ntsc"]);
        assert_eq!(offered[0].chosen, "rac1/pal");

        // NTSC-U, never set up: PAL's set-up moves aside, the folder is free for NTSC-U's.
        choose(&mut config, &catalog, &both(), "rac1/ntsc").unwrap();
        assert_eq!(config.disc_choice.get("rac1").map(String::as_str), Some("rac1/ntsc"));
        assert_eq!(choices(&config, &catalog, &both())[0].chosen, "rac1/ntsc");
        assert!(install::state(install, "rac1").extracted.is_none());
        assert!(kept(install, "iso_data", "rac1", "rac1/pal").join("buildinfo.json").is_file());

        // Set up from the NTSC-U disc, then back to PAL: each set-up is the one it was.
        set_up(install, "rac1/ntsc", "SCUS_971.99");
        choose(&mut config, &catalog, &both(), "rac1/pal").unwrap();
        assert_eq!(played(install), "rac1/pal");
        assert!(kept(install, "iso_data", "rac1", "rac1/ntsc").join("disc.iso").is_file());
        choose(&mut config, &catalog, &both(), "rac1/ntsc").unwrap();
        assert_eq!(played(install), "rac1/ntsc");
        assert_eq!(install::state(install, "rac1").extracted.unwrap().version, "rac1/ntsc");
        assert!(!kept(install, "iso_data", "rac1", "rac1/ntsc").exists());

        // Both are set up now: still offered with one disc, or none, left in the folder.
        let only_pal = [disc("rac1/pal", "SCES_509.16")];
        assert_eq!(choices(&config, &catalog, &only_pal)[0].versions, ["rac1/pal", "rac1/ntsc"]);
        assert_eq!(choices(&config, &catalog, &[])[0].chosen, "rac1/ntsc");

        // A choice whose disc is not known any more is not used.
        let fresh = tempfile::tempdir().unwrap();
        let config = Config { install_dir: Some(fresh.path().into()), ..config };
        assert!(choices(&config, &catalog, &only_pal).is_empty());
        let other = [disc("rac1/pal", "SCES_509.16"), disc("rac1/ntsc", "SCUS_971.99")];
        let config = Config { disc_choice: [("rac1".into(), "rac1/gone".into())].into(), ..config };
        assert_eq!(choices(&config, &catalog, &other)[0].chosen, "rac1/pal");
    }

    #[test]
    fn players_get_no_choice_and_nothing_moves() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path();
        set_up(install, "rac1/pal", "SCES_509.16");
        let mut config = Config { developer: false, install_dir: Some(install.into()), ..Config::default() };
        assert!(choices(&config, &catalog, &both()).is_empty());
        assert!(choose(&mut config, &catalog, &both(), "rac1/ntsc").is_err());
        assert!(config.disc_choice.is_empty());
        assert_eq!(played(install), "rac1/pal");

        // A choice left from developer mode does nothing once it is off.
        config.disc_choice.insert("rac1".into(), "rac1/ntsc".into());
        assert!(choices(&config, &catalog, &both()).is_empty());
    }

    #[test]
    fn one_disc_found_means_no_switch() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let mut config = Config { developer: true, ..Config::default() };
        let one = [disc("rac1/ntsc", "SCUS_971.99")];
        assert!(choices(&config, &catalog, &one).is_empty());
        assert!(choices(&config, &catalog, &[]).is_empty());
        assert!(choose(&mut config, &catalog, &one, "rac1/ntsc").is_err());
        // Two images of the same version are one disc (the scan keeps the first).
        let same = [disc("rac1/pal", "SCES_509.16"), disc("rac1/pal", "SCES_509.16")];
        assert!(choices(&config, &catalog, &same).is_empty());
    }

    /// A developer setting the other disc up by hand keeps the first one aside; a player's set-up
    /// replaces it, as it always has.
    #[test]
    fn a_developers_second_set_up_keeps_the_first() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path().join("install");
        let image = dir.path().join("us.iso");
        crate::discs::tests::image(&image, "BOOT2 = cdrom0:\\SCUS_971.99;1\r\n");
        set_up(&install, "rac1/pal", "SCES_509.16");

        let player = Config { install_dir: Some(install.clone()), ..Config::default() };
        before_set_up(&player, &catalog, "rac1", &image).unwrap();
        assert_eq!(played(&install), "rac1/pal", "a player's set-up is left to the extractor");

        let developer = Config { developer: true, ..player };
        before_set_up(&developer, &catalog, "rac1", &image).unwrap();
        assert!(install::state(&install, "rac1").extracted.is_none());
        assert!(kept(&install, "iso_data", "rac1", "rac1/pal").join("disc.iso").is_file());
    }

    /// No games folder: a version set up before and kept aside, and the one set up now, are two
    /// known discs, and switching between them needs no folder and no image.
    #[test]
    fn set_ups_alone_offer_the_switch() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path();
        set_up(install, "rac1/pal", "SCES_509.16");
        switch(install, "rac1", "rac1/ntsc").unwrap();
        set_up(install, "rac1/ntsc", "SCUS_971.99");
        let mut config = Config { developer: true, install_dir: Some(install.into()), ..Config::default() };
        assert_eq!(config.discs_dir, None);

        let offered = choices(&config, &catalog, &[]);
        assert_eq!(offered.len(), 1, "{offered:?}");
        assert_eq!(offered[0].versions, ["rac1/pal", "rac1/ntsc"]);
        assert_eq!(offered[0].chosen, "rac1/ntsc", "the disc set up is the default");
        assert!(offered[0].images.is_empty(), "known by their set-ups only");

        choose(&mut config, &catalog, &[], "rac1/pal").unwrap();
        assert_eq!(played(install), "rac1/pal");
        assert_eq!(choices(&config, &catalog, &[])[0].chosen, "rac1/pal");
        choose(&mut config, &catalog, &[], "rac1/ntsc").unwrap();
        assert_eq!(played(install), "rac1/ntsc");

        // One set-up is one disc: nothing to switch between.
        let dir = tempfile::tempdir().unwrap();
        set_up(dir.path(), "rac1/pal", "SCES_509.16");
        let one = Config { developer: true, install_dir: Some(dir.path().into()), ..Config::default() };
        assert!(choices(&one, &catalog, &[]).is_empty());
    }

    /// The image a developer picked to set a second version up is known while it is there and is
    /// still that disc; a player's pick is not remembered.
    #[test]
    fn a_picked_image_is_a_known_disc_while_it_is_there() {
        let catalog = crate::catalog::load(&checkout()).unwrap();
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path().join("install");
        let image = dir.path().join("picked.iso");
        crate::discs::tests::image(&image, "BOOT2 = cdrom0:\\SCUS_971.99;1\r\nVER = 1.00\r\n");
        set_up(&install, "rac1/pal", "SCES_509.16");

        let mut player = Config { install_dir: Some(install.clone()), ..Config::default() };
        assert!(!remember(&mut player, &catalog, "rac1", &image));
        assert!(player.disc_images.is_empty());

        let mut config = Config { developer: true, ..player };
        assert!(remember(&mut config, &catalog, "rac1", &image));
        assert!(!remember(&mut config, &catalog, "rac1", &image), "already remembered");
        let offered = choices(&config, &catalog, &[]);
        assert_eq!(offered.len(), 1, "{offered:?}");
        assert_eq!(offered[0].versions, ["rac1/pal", "rac1/ntsc"]);
        assert_eq!(offered[0].chosen, "rac1/pal");
        assert_eq!(offered[0].images.get("rac1/ntsc"), config.disc_images.get("rac1/ntsc"));
        choose(&mut config, &catalog, &[], "rac1/ntsc").unwrap();
        assert!(install::state(&install, "rac1").extracted.is_none(), "left to set up from the image");

        // The games folder's image of a version comes first.
        let folder = [disc("rac1/ntsc", "SCUS_971.99")];
        let offered = choices(&config, &catalog, &folder);
        assert_eq!(offered[0].images.get("rac1/ntsc"), Some(&folder[0].path));

        // Overwritten by another disc, or gone: not known any more.
        crate::discs::tests::image(&image, "BOOT2 = cdrom0:\\SCES_509.16;1\r\nVER = 2.00\r\n");
        assert!(choices(&config, &catalog, &[]).is_empty());
        std::fs::remove_file(&image).unwrap();
        assert!(choices(&config, &catalog, &[]).is_empty());
        assert!(choose(&mut config, &catalog, &[], "rac1/ntsc").is_err());

        // A file that is no disc is not remembered.
        let junk = dir.path().join("junk.iso");
        std::fs::write(&junk, b"no disc").unwrap();
        assert!(!remember(&mut config, &catalog, "rac1", &junk));
    }

    #[test]
    fn switching_never_overwrites_a_kept_set_up() {
        let dir = tempfile::tempdir().unwrap();
        let install = dir.path();
        set_up(install, "rac1/pal", "SCES_509.16");
        std::fs::create_dir_all(kept(install, "iso_data", "rac1", "rac1/pal")).unwrap();
        assert!(switch(install, "rac1", "rac1/ntsc").is_err());
        assert_eq!(played(install), "rac1/pal");

        // A set-up cut short (no buildinfo.json) is not moved under a guessed name.
        let dir = tempfile::tempdir().unwrap();
        let half = install::data_dir(dir.path(), "rac1").join("iso_data").join("rac1");
        std::fs::create_dir_all(&half).unwrap();
        assert!(switch(dir.path(), "rac1", "rac1/ntsc").is_err());
        assert!(half.is_dir());
    }
}
