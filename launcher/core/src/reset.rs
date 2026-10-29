//! Starting the level editor over: the resets under the Mods page's "Your levels" (src/components/EditorReset.svelte).
//!
//! Two resets, each for one version of a game:
//!
//! - [`Reset::Levels`], "Read the game's levels again": the game's copy of the levels (what Play and Models
//!   and textures read, with its readiness stamps `levels-<format>.json` and `verified-<format>.json`
//!   and the visibility added to it) and the enhanced lighting baked from them. The page reads them
//!   from the disc again straight after. Nothing the player made is touched.
//! - [`Reset::Editor`], "Delete the level project": the Godot projects the levels were read into and
//!   the models and textures exported from them. With `with_edits`, also what the player made: the
//!   packed levels and the mods put together from them, the imported edits and added packs, and the
//!   copies of older projects the extractor kept aside.
//!
//! Where these are (a checkout keeps them under `assets/`, an installed launcher in the game's data
//! folder, `install::player_folder`), and every folder there:
//!
//! ```text
//! assets/port/<game>/<version>/       <data>/port/<version>/       the game's levels      Levels
//! <data>/probes/<version>/                                         enhanced lighting      Levels
//! assets/levels/<game>/<version>/     <data>/levels/<version>/     Godot projects         Editor
//! <data>/exports/                                                  models and textures    Editor
//! assets/packed/<game>/<version>/     <data>/packed/<version>/     packed levels          Editor + edits
//! <data>/mods/<version>/                                           the mods put together  Editor + edits
//! <data>/overrides/                                                imported edits, packs  Editor + edits
//! assets/levels/<game>/<version>-backups/                          older projects kept    Editor + edits
//! ```
//!
//! plus the hidden `.<version>-*` folders the tools leave beside their output when a run is stopped
//! midway (editor/extract.py `publish`). Nothing else is ever removed: not the disc's files
//! (`iso_data`), the saves, the settings, the native mods (`plugins`) or the built game.
//!
//! The folders are made here from the game and version ids, never from a path the page sends. Each
//! one must be strictly inside one of the two roots (`<checkout>/assets`, `<data>`) once every link
//! on the way is resolved; one that is itself a link, leaves its root through a linked folder, or is
//! not a folder refuses the whole reset before anything is removed. Removing never follows a link
//! inside a folder either (`std::fs::remove_dir_all` takes the link away, not what it points at).

use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

/// Which reset.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "ResetKind")]
pub enum Reset {
    /// The game's copy of the levels, read from the disc again afterwards.
    Levels,
    /// The level editor's projects and exports; with edits, the player's own work too.
    Editor,
}

/// One group of folders a reset removes, as the confirmation lists it.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "ResetPart")]
pub enum Part {
    /// The game's levels (with their stamps and visibility) and the lighting baked from them.
    GameLevels,
    /// The Godot projects the levels were read into, with any change not packed.
    Projects,
    /// The models and textures exported for editing.
    Exports,
    /// The packed levels, and the mods put together from them and the packs.
    Packed,
    /// The imported edits and the packs added (the game's, for all its discs).
    Overrides,
    /// The copies of projects an older editor made that the extractor kept aside.
    Backups,
}

impl Part {
    /// The player's own work: removed only when they ask for it.
    pub fn theirs(self) -> bool {
        matches!(self, Part::Packed | Part::Overrides | Part::Backups)
    }
}

/// A group of folders, its size and where it is.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "ResetPartView")]
pub struct PartView {
    pub part: Part,
    /// Removed only when the player ticks "also my edits and packs".
    pub theirs: bool,
    pub bytes: u64,
    pub files: u64,
    /// The folders that are there (the developer view names them).
    pub folders: Vec<PathBuf>,
}

/// What a reset would remove: every group it can, the player's own included for the editor's.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ResetPreview {
    pub reset: Reset,
    pub parts: Vec<PartView>,
}

/// What a reset removed.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ResetDone {
    pub bytes: u64,
    pub files: u64,
    pub folders: u64,
}

/// Where one version's editor data is.
#[derive(Debug, Clone)]
pub struct Layout {
    /// `<checkout>/assets`: a checkout's levels, projects and packed levels.
    assets: PathBuf,
    /// `<install>/active/<game>/data`: everything else, and an installed launcher's all.
    data: PathBuf,
    game: String,
    version: String,
    /// An installed launcher's copy: its `assets/` folders are in `data` (install::player_folder).
    installed: bool,
}

/// A game or version id: letters, digits, `_` and `-` only, so that it is one folder name.
fn plain_id(id: &str) -> Result<&str, String> {
    let ok = !id.is_empty() && id.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-');
    if ok {
        Ok(id)
    } else {
        Err(format!("{id:?} is not a game or version id"))
    }
}

impl Layout {
    /// `root` is the checkout (or an installed launcher's copy, `installed`), `install` the install
    /// folder from Settings, `game`/`version` the ids (`rac1`, `pal`).
    pub fn new(root: &Path, install: &Path, game: &str, version: &str, installed: bool) -> Result<Self, String> {
        let (game, version) = (plain_id(game)?, plain_id(version)?);
        Ok(Layout {
            assets: root.join("assets"),
            data: crate::install::data_dir(install, game),
            game: game.to_string(),
            version: version.to_string(),
            installed,
        })
    }

    /// `assets/<kind>/<game>/<version>` of a checkout, or where an installed launcher keeps it.
    fn assets_of(&self, kind: &str) -> PathBuf {
        if self.installed {
            self.data.join(kind).join(&self.version)
        } else {
            self.assets.join(kind).join(&self.game).join(&self.version)
        }
    }

    /// The level editor's Godot projects, one folder per level (`level_NN`).
    pub fn projects(&self) -> PathBuf {
        self.assets_of("levels")
    }

    /// Where the extractor keeps the projects it replaced (`<time>/level_NN`).
    pub fn backups(&self) -> PathBuf {
        self.projects().with_file_name(format!("{}-backups", self.version))
    }

    /// The packed levels (`changes.txt`, `packed.json`, `level_NN/`).
    pub fn packed(&self) -> PathBuf {
        self.assets_of("packed")
    }

    /// The game's copy of the levels (`levels-<format>.json`).
    pub fn port(&self) -> PathBuf {
        self.assets_of("port")
    }

    /// `rac1/pal`.
    pub fn key(&self) -> String {
        format!("{}/{}", self.game, self.version)
    }

    /// The version id, `pal`.
    pub fn version(&self) -> &str {
        &self.version
    }

    /// The folders a reset may remove (they need not be there): the roots, with every folder of
    /// each part under one of them.
    fn roots(&self) -> [&Path; 2] {
        [&self.assets, &self.data]
    }

    /// Every folder of `part`, whether or not it is there.
    pub fn folders(&self, part: Part) -> Vec<PathBuf> {
        let v = &self.version;
        match part {
            Part::GameLevels => {
                let port = self.assets_of("port");
                let mut out = vec![self.data.join("probes").join(v)];
                out.extend(leftovers(&port));
                out.insert(0, port);
                out
            }
            Part::Projects => {
                let projects = self.assets_of("levels");
                let mut out = leftovers(&projects);
                out.insert(0, projects);
                out
            }
            Part::Exports => vec![self.data.join("exports")],
            Part::Packed => {
                let packed = self.assets_of("packed");
                let mut out = vec![self.data.join("mods").join(v)];
                out.extend(leftovers(&packed));
                out.insert(0, packed);
                out
            }
            Part::Overrides => vec![self.data.join("overrides")],
            Part::Backups => vec![self.backups()],
        }
    }
}

/// The parts of `reset`, the player's own last.
pub fn parts(reset: Reset) -> &'static [Part] {
    match reset {
        Reset::Levels => &[Part::GameLevels],
        Reset::Editor => &[Part::Projects, Part::Exports, Part::Packed, Part::Overrides, Part::Backups],
    }
}

/// The hidden folders a stopped run of the tools left beside `out`: `.<name>-<random>` (and
/// `.<name>-old-<random>`), from Python's `tempfile.mkdtemp(prefix=f".{out.name}-")`.
fn leftovers(out: &Path) -> Vec<PathBuf> {
    let (Some(parent), Some(name)) = (out.parent(), out.file_name()) else { return Vec::new() };
    let prefix = format!(".{}-", name.to_string_lossy());
    let Ok(entries) = std::fs::read_dir(parent) else { return Vec::new() };
    let mut found: Vec<PathBuf> =
        entries.flatten().filter(|e| e.file_name().to_string_lossy().starts_with(&prefix)).map(|e| e.path()).collect();
    found.sort();
    found
}

/// `path`, checked: None when it is not there; else the folder itself, resolved, when it is a real
/// folder strictly inside one of `roots` (resolved too). A link, a file, or a folder that leaves
/// its root through a linked folder on the way is an error.
pub fn checked(path: &Path, roots: &[&Path]) -> Result<Option<PathBuf>, String> {
    let meta = match std::fs::symlink_metadata(path) {
        Ok(meta) => meta,
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(None),
        Err(e) => return Err(format!("{}: {e}", path.display())),
    };
    if meta.file_type().is_symlink() {
        return Err(format!("{} is a link, not a folder of OpenRAC's: left alone", path.display()));
    }
    if !meta.is_dir() {
        return Err(format!("{} is not a folder: left alone", path.display()));
    }
    // Written under a root (Layout makes every folder so) and still inside it once resolved, and
    // never the root itself.
    let under = roots.iter().filter(|root| path.starts_with(root)).find_map(|root| {
        let real_root = std::fs::canonicalize(root).ok()?;
        let real = std::fs::canonicalize(path).ok()?;
        (real != real_root && real.starts_with(&real_root)).then_some(real)
    });
    under.map(Some).ok_or_else(|| format!("{} is outside OpenRAC's folders: left alone", path.display()))
}

/// The size of the folder `dir` and how many files it holds, without following a link.
pub fn size(dir: &Path) -> (u64, u64) {
    let mut bytes = 0;
    let mut files = 0;
    let mut todo = vec![dir.to_path_buf()];
    while let Some(next) = todo.pop() {
        let Ok(entries) = std::fs::read_dir(&next) else { continue };
        for entry in entries.flatten() {
            let Ok(kind) = entry.file_type() else { continue };
            if kind.is_dir() {
                todo.push(entry.path());
            } else {
                files += 1;
                if kind.is_file() {
                    bytes += entry.metadata().map(|m| m.len()).unwrap_or(0);
                }
            }
        }
    }
    (bytes, files)
}

/// The folders of `parts` that are there, checked (`checked`), by part.
fn resolve(layout: &Layout, parts: &[Part]) -> Result<Vec<(Part, Vec<PathBuf>)>, String> {
    let roots = layout.roots();
    parts
        .iter()
        .map(|&part| {
            let mut found = Vec::new();
            for folder in layout.folders(part) {
                if let Some(real) = checked(&folder, &roots)? {
                    found.push(real);
                }
            }
            Ok((part, found))
        })
        .collect()
}

/// What `reset` would remove, every part with its size; the editor's lists the player's own work
/// too, marked `theirs`, for the page to show beside its tick box.
pub fn preview(layout: &Layout, reset: Reset) -> Result<ResetPreview, String> {
    let parts = resolve(layout, parts(reset))?
        .into_iter()
        .map(|(part, folders)| {
            let (bytes, files) = folders.iter().map(|f| size(f)).fold((0, 0), |(b, n), (b2, n2)| (b + b2, n + n2));
            PartView { part, theirs: part.theirs(), bytes, files, folders }
        })
        .collect();
    Ok(ResetPreview { reset, parts })
}

/// Removes what `reset` covers (the player's own work only `with_edits`). Every folder is checked
/// before the first is removed: one that fails the check removes nothing.
pub fn apply(layout: &Layout, reset: Reset, with_edits: bool) -> Result<ResetDone, String> {
    let wanted: Vec<Part> = parts(reset).iter().copied().filter(|p| with_edits || !p.theirs()).collect();
    let mut done = ResetDone::default();
    for (_, folders) in resolve(layout, &wanted)? {
        for folder in folders {
            let (bytes, files) = size(&folder);
            std::fs::remove_dir_all(&folder).map_err(|e| format!("cannot remove {}: {e}", folder.display()))?;
            done.bytes += bytes;
            done.files += files;
            done.folders += 1;
        }
    }
    Ok(done)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn write(path: &Path, bytes: usize) {
        std::fs::create_dir_all(path.parent().unwrap()).unwrap();
        std::fs::write(path, vec![b'x'; bytes]).unwrap();
    }

    /// A checkout and an install folder with every folder a reset knows filled, and what must stay.
    struct World {
        _tmp: tempfile::TempDir,
        root: PathBuf,
        install: PathBuf,
    }

    impl World {
        fn new() -> World {
            let tmp = tempfile::tempdir().unwrap();
            let root = tmp.path().join("OpenRAC");
            let install = tmp.path().join("install");
            let data = crate::install::data_dir(&install, "rac1");
            for (rel, n) in [
                ("assets/port/rac1/pal/level_00/manifest.json", 10),
                ("assets/port/rac1/pal/levels-4.json", 2),
                ("assets/port/rac1/pal/verified-4.json", 2),
                ("assets/port/rac1/.pal-abc123/half.bin", 5),
                ("assets/port/rac1/ntsc/levels-4.json", 2),
                ("assets/levels/rac1/pal/level_00/project.godot", 100),
                ("assets/levels/rac1/pal/level_00/.godot/imported/a.ctex", 50),
                ("assets/levels/rac1/pal-backups/20261001-000000/level_03/project.godot", 7),
                ("assets/levels/rac1/ntsc/level_00/project.godot", 1),
                ("assets/packed/rac1/pal/level_00/gameplay_pal.wad", 30),
                ("assets/godot/keep.txt", 1),
                ("tools/openrac.py", 1),
            ] {
                write(&root.join(rel), n);
            }
            for (rel, n) in [
                ("probes/pal/level_00.bin", 40),
                ("probes/ntsc/level_00.bin", 1),
                ("exports/level_00-textures/a.png", 20),
                ("overrides/packs/mine/openrac-mod.json", 3),
                ("overrides/overrides.json", 3),
                ("mods/pal/level_00/x.bin", 4),
                ("mods/ntsc/level_00/x.bin", 1),
                ("iso_data/rac1/disc.iso", 9),
                ("saves/card.ps2", 9),
                ("plugins/plugins.json", 9),
            ] {
                write(&data.join(rel), n);
            }
            World { _tmp: tmp, root, install }
        }

        fn layout(&self) -> Layout {
            Layout::new(&self.root, &self.install, "rac1", "pal", false).unwrap()
        }

        fn data(&self) -> PathBuf {
            crate::install::data_dir(&self.install, "rac1")
        }

        /// Every file left, relative to the folder above the checkout.
        fn left(&self) -> Vec<String> {
            let top = self.root.parent().unwrap().to_path_buf();
            let mut out = Vec::new();
            let mut todo = vec![top.clone()];
            while let Some(dir) = todo.pop() {
                for e in std::fs::read_dir(&dir).unwrap().flatten() {
                    let kind = std::fs::symlink_metadata(e.path()).unwrap().file_type();
                    if kind.is_dir() {
                        todo.push(e.path());
                    } else {
                        out.push(e.path().strip_prefix(&top).unwrap().to_string_lossy().replace('\\', "/"));
                    }
                }
            }
            out.sort();
            out
        }
    }

    #[test]
    fn ids_must_be_one_folder_name() {
        let w = World::new();
        for bad in ["", "..", "rac1/pal", "../x", "a b", "pal\\x"] {
            assert!(Layout::new(&w.root, &w.install, bad, "pal", false).is_err(), "{bad}");
            assert!(Layout::new(&w.root, &w.install, "rac1", bad, false).is_err(), "{bad}");
        }
    }

    #[test]
    fn the_folders_of_a_checkout_and_of_an_installed_launcher() {
        let w = World::new();
        let data = w.data();
        let checkout = w.layout();
        assert_eq!(
            checkout.folders(Part::GameLevels),
            vec![
                w.root.join("assets/port/rac1/pal"),
                data.join("probes").join("pal"),
                w.root.join("assets/port/rac1").join(".pal-abc123"),
            ]
        );
        assert_eq!(checkout.folders(Part::Projects), vec![w.root.join("assets/levels/rac1/pal")]);
        assert_eq!(checkout.folders(Part::Backups), vec![w.root.join("assets/levels/rac1/pal-backups")]);
        assert_eq!(checkout.folders(Part::Packed), vec![w.root.join("assets/packed/rac1/pal"), data.join("mods/pal")]);
        assert_eq!(checkout.folders(Part::Exports), vec![data.join("exports")]);
        assert_eq!(checkout.folders(Part::Overrides), vec![data.join("overrides")]);

        let installed = Layout::new(&w.root, &w.install, "rac1", "pal", true).unwrap();
        assert_eq!(installed.folders(Part::Projects), vec![data.join("levels").join("pal")]);
        assert_eq!(installed.folders(Part::Backups), vec![data.join("levels").join("pal-backups")]);
        assert_eq!(installed.folders(Part::GameLevels)[0], data.join("port").join("pal"));
        assert_eq!(installed.folders(Part::Packed)[0], data.join("packed").join("pal"));
        // The same as the folders the launcher opens for an installed launcher.
        assert_eq!(
            crate::install::player_folder(&w.install, "assets/levels/rac1/pal"),
            Some(installed.folders(Part::Projects)[0].clone())
        );
    }

    #[test]
    fn previews_sizes_without_removing() {
        let w = World::new();
        let before = w.left();
        let levels = preview(&w.layout(), Reset::Levels).unwrap();
        assert_eq!(levels.parts.len(), 1);
        assert_eq!((levels.parts[0].bytes, levels.parts[0].files), (10 + 2 + 2 + 40 + 5, 5));
        let editor = preview(&w.layout(), Reset::Editor).unwrap();
        let of = |p: Part| editor.parts.iter().find(|v| v.part == p).unwrap().clone();
        assert_eq!((of(Part::Projects).bytes, of(Part::Projects).theirs), (150, false));
        assert_eq!((of(Part::Exports).bytes, of(Part::Exports).theirs), (20, false));
        assert_eq!((of(Part::Packed).bytes, of(Part::Packed).theirs), (34, true));
        assert_eq!((of(Part::Overrides).bytes, of(Part::Overrides).theirs), (6, true));
        assert_eq!((of(Part::Backups).bytes, of(Part::Backups).theirs), (7, true));
        assert_eq!(w.left(), before);
    }

    #[test]
    fn reading_the_levels_again_removes_only_the_games_levels() {
        let w = World::new();
        let done = apply(&w.layout(), Reset::Levels, false).unwrap();
        assert_eq!((done.bytes, done.files, done.folders), (59, 5, 3));
        let left = w.left();
        assert!(!left.iter().any(|f| f.starts_with("OpenRAC/assets/port/rac1/pal/")), "{left:?}");
        assert!(!left.iter().any(|f| f.contains("/.pal-")), "{left:?}");
        assert!(!left.iter().any(|f| f.contains("probes/pal/")), "{left:?}");
        // The other disc's, the projects, the packs, the exports and everything of set-up stay.
        for kept in [
            "OpenRAC/assets/port/rac1/ntsc/levels-4.json",
            "OpenRAC/assets/levels/rac1/pal/level_00/project.godot",
            "OpenRAC/assets/packed/rac1/pal/level_00/gameplay_pal.wad",
            "install/active/rac1/data/probes/ntsc/level_00.bin",
            "install/active/rac1/data/exports/level_00-textures/a.png",
            "install/active/rac1/data/overrides/overrides.json",
            "install/active/rac1/data/iso_data/rac1/disc.iso",
        ] {
            assert!(left.contains(&kept.to_string()), "{kept} went: {left:?}");
        }
    }

    #[test]
    fn resetting_the_editor_keeps_the_players_work_unless_asked() {
        let w = World::new();
        apply(&w.layout(), Reset::Editor, false).unwrap();
        let left = w.left();
        assert!(!left.iter().any(|f| f.starts_with("OpenRAC/assets/levels/rac1/pal/")), "{left:?}");
        assert!(!left.iter().any(|f| f.contains("/exports/")), "{left:?}");
        for kept in [
            "OpenRAC/assets/levels/rac1/pal-backups/20261001-000000/level_03/project.godot",
            "OpenRAC/assets/levels/rac1/ntsc/level_00/project.godot",
            "OpenRAC/assets/packed/rac1/pal/level_00/gameplay_pal.wad",
            "OpenRAC/assets/port/rac1/pal/levels-4.json",
            "install/active/rac1/data/overrides/packs/mine/openrac-mod.json",
            "install/active/rac1/data/mods/pal/level_00/x.bin",
        ] {
            assert!(left.contains(&kept.to_string()), "{kept} went: {left:?}");
        }
    }

    #[test]
    fn resetting_the_editor_with_edits_removes_them_and_nothing_else() {
        let w = World::new();
        let done = apply(&w.layout(), Reset::Editor, true).unwrap();
        assert_eq!(done.bytes, 150 + 20 + 34 + 6 + 7);
        assert_eq!(
            w.left(),
            vec![
                "OpenRAC/assets/godot/keep.txt",
                "OpenRAC/assets/levels/rac1/ntsc/level_00/project.godot",
                "OpenRAC/assets/port/rac1/.pal-abc123/half.bin",
                "OpenRAC/assets/port/rac1/ntsc/levels-4.json",
                "OpenRAC/assets/port/rac1/pal/level_00/manifest.json",
                "OpenRAC/assets/port/rac1/pal/levels-4.json",
                "OpenRAC/assets/port/rac1/pal/verified-4.json",
                "OpenRAC/tools/openrac.py",
                "install/active/rac1/data/iso_data/rac1/disc.iso",
                "install/active/rac1/data/mods/ntsc/level_00/x.bin",
                "install/active/rac1/data/plugins/plugins.json",
                "install/active/rac1/data/probes/ntsc/level_00.bin",
                "install/active/rac1/data/probes/pal/level_00.bin",
                "install/active/rac1/data/saves/card.ps2",
            ]
        );
        // Again: nothing left to remove is not a failure.
        assert_eq!(apply(&w.layout(), Reset::Editor, true).unwrap(), ResetDone::default());
    }

    #[test]
    fn checks_paths_against_the_roots() {
        let w = World::new();
        let assets = w.root.join("assets");
        let roots = [assets.as_path()];
        assert!(checked(&assets.join("levels/rac1/missing"), &roots).unwrap().is_none());
        assert!(checked(&assets.join("levels/rac1/pal"), &roots).unwrap().is_some());
        // The root itself, a folder written outside it, one that climbs out, and a file.
        assert!(checked(&assets, &roots).is_err());
        assert!(checked(&w.root.join("tools"), &roots).is_err());
        assert!(checked(&assets.join("levels/../../tools"), &roots).is_err());
        assert!(checked(&assets.join("levels/rac1/pal/level_00/project.godot"), &roots).is_err());
    }

    #[cfg(unix)]
    #[test]
    fn refuses_links_out_and_removes_nothing() {
        use std::os::unix::fs::symlink;
        let w = World::new();
        let outside = w.root.parent().unwrap().join("precious");
        write(&outside.join("photo.jpg"), 8);

        // The projects' folder is a link elsewhere: the whole reset is refused, the exports stay too.
        let projects = w.root.join("assets/levels/rac1/pal");
        std::fs::remove_dir_all(&projects).unwrap();
        symlink(&outside, &projects).unwrap();
        assert!(preview(&w.layout(), Reset::Editor).is_err());
        assert!(apply(&w.layout(), Reset::Editor, false).is_err());
        assert!(outside.join("photo.jpg").is_file());
        assert!(w.data().join("exports/level_00-textures/a.png").is_file());
        std::fs::remove_file(&projects).unwrap();

        // A linked folder on the way out of the root (assets/levels/rac1 -> elsewhere).
        let rac1 = w.root.join("assets/levels/rac1");
        std::fs::remove_dir_all(&rac1).unwrap();
        std::fs::create_dir_all(outside.join("pal")).unwrap();
        symlink(&outside, &rac1).unwrap();
        assert!(apply(&w.layout(), Reset::Editor, false).is_err());
        assert!(outside.join("photo.jpg").is_file());
        std::fs::remove_file(&rac1).unwrap();

        // A link inside a folder that goes: the link goes, not what it points at.
        write(&w.root.join("assets/levels/rac1/pal/level_00/project.godot"), 1);
        symlink(&outside, w.root.join("assets/levels/rac1/pal/level_00/linked")).unwrap();
        let shown = preview(&w.layout(), Reset::Editor).unwrap();
        assert_eq!(shown.parts.iter().find(|p| p.part == Part::Projects).unwrap().bytes, 1);
        apply(&w.layout(), Reset::Editor, false).unwrap();
        assert!(!w.root.join("assets/levels/rac1/pal").exists());
        assert!(outside.join("photo.jpg").is_file());
    }
}
