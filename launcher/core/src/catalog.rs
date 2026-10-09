//! The games and their versions, read from the checkout: every
//! `games/<game>/game.json` (the same manifests `tools/openrac.py` reads) and
//! the consolidated `progress/summary.json` that `openrac.py progress` writes.
//!
//! Only the fields the launcher shows are parsed; everything else in
//! game.json is ignored, so the manifests can grow without launcher changes.

use std::collections::BTreeMap;
use std::path::Path;

use serde::{Deserialize, Serialize};

/// Every game in the checkout, in game order (rac1, rac2, ...).
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Catalog {
    pub games: Vec<Game>,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Game {
    /// `rac1`, `rac2`, ...: the directory under `games/`.
    pub id: String,
    pub title: String,
    pub year: Option<u32>,
    /// Names other regions released the game under (`"pal": "Ratchet: Gladiator"`).
    pub titles: BTreeMap<String, String>,
    pub versions: Vec<Version>,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Version {
    /// `rac1/pal`: the key used everywhere (progress, actions, commit scopes).
    pub key: String,
    pub game: String,
    /// `pal`, `ntsc`: the directory under `games/<game>/`.
    pub name: String,
    /// The game's title in this region.
    pub title: String,
    pub region: String,
    pub serial: String,
    /// The project's directory, relative to the checkout (`games/rac1/pal`).
    pub dir: String,
    pub disc: Option<FileSpec>,
    pub boot: Option<FileSpec>,
    /// What `openrac.py setup` places for this version.
    pub inputs: Vec<Input>,
    /// What the version's build reproduces, in one sentence.
    pub target: Option<String>,
    /// The manifest's own next steps after `openrac.py setup`, as prose.
    pub setup: Option<String>,
    /// Which machines the build runs on, as prose.
    pub build_host: Option<String>,
    /// The version's README, relative to the checkout.
    pub readme: Option<String>,
    pub source: Option<Source>,
    pub progress: Option<Progress>,
}

/// A file game.json describes by name and checksums (the disc image, the boot executable).
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct FileSpec {
    pub file: String,
    #[serde(default)]
    pub size: Option<u64>,
    #[serde(default)]
    pub sha1: Option<String>,
}

/// One input `openrac.py setup` places: `from` is `disc`, `boot`,
/// `toolchains` or `link` (then `target` names what it links to).
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Input {
    pub from: String,
    pub path: String,
    #[serde(default)]
    pub target: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Source {
    pub name: String,
    pub repo: String,
    #[serde(default)]
    pub license: Option<String>,
    #[serde(default)]
    pub commit: Option<String>,
    #[serde(default)]
    pub date: Option<String>,
}

/// One row of progress/summary.json (snake_case on disk, camelCase to the UI).
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all(serialize = "camelCase", deserialize = "snake_case"))]
pub struct Progress {
    pub matched_code: u64,
    pub total_code: u64,
    pub matched_functions: Option<u64>,
    pub total_functions: Option<u64>,
    pub fuzzy_percent: Option<f64>,
    pub percent: f64,
    pub date: Option<String>,
    /// What the project counts, from game.json's progress note.
    #[serde(default)]
    pub note: Option<String>,
}

// --- game.json as it is on disk -------------------------------------------------

#[derive(Deserialize)]
struct RawGame {
    id: String,
    title: String,
    #[serde(default)]
    year: Option<u32>,
    #[serde(default)]
    titles: BTreeMap<String, String>,
    /// In the manifest's order (serde_json's preserve_order), so a game's
    /// first version stays first: rac1's PAL, the project OpenRAC began with.
    #[serde(default)]
    versions: serde_json::Map<String, serde_json::Value>,
}

#[derive(Deserialize)]
struct RawVersion {
    #[serde(default)]
    region: String,
    #[serde(default)]
    serial: String,
    #[serde(default)]
    disc: Option<FileSpec>,
    #[serde(default)]
    boot: Option<FileSpec>,
    #[serde(default)]
    inputs: Vec<Input>,
    #[serde(default)]
    target: Option<String>,
    #[serde(default)]
    setup: Option<String>,
    #[serde(default)]
    build: Option<RawBuild>,
    #[serde(default)]
    source: Option<Source>,
}

#[derive(Deserialize)]
struct RawBuild {
    #[serde(default)]
    host: Option<String>,
    #[serde(default)]
    readme: Option<String>,
}

#[derive(Deserialize)]
struct RawSummaryRow {
    progress: Progress,
    #[serde(default)]
    note: Option<String>,
}

/// Reads the catalogue from the checkout at `root`. A game.json that does
/// not parse is an error (CI keeps them valid); a missing or unreadable
/// progress/summary.json only leaves the progress out.
pub fn load(root: &Path) -> Result<Catalog, String> {
    let games_dir = root.join("games");
    let mut manifests: Vec<_> = std::fs::read_dir(&games_dir)
        .map_err(|e| format!("cannot read {}: {e}", games_dir.display()))?
        .filter_map(|entry| entry.ok())
        .map(|entry| entry.path().join("game.json"))
        .filter(|path| path.is_file())
        .collect();
    manifests.sort();

    let summary = read_summary(root);
    let mut games = Vec::new();
    for path in manifests {
        let text = std::fs::read_to_string(&path).map_err(|e| format!("cannot read {}: {e}", path.display()))?;
        let raw: RawGame = serde_json::from_str(&text).map_err(|e| format!("{}: {e}", path.display()))?;
        games.push(game(raw, &summary).map_err(|e| format!("{}: {e}", path.display()))?);
    }
    Ok(Catalog { games })
}

fn read_summary(root: &Path) -> BTreeMap<String, RawSummaryRow> {
    std::fs::read_to_string(root.join("progress").join("summary.json"))
        .ok()
        .and_then(|text| serde_json::from_str(&text).ok())
        .unwrap_or_default()
}

fn game(raw: RawGame, summary: &BTreeMap<String, RawSummaryRow>) -> Result<Game, String> {
    let mut versions = Vec::new();
    for (name, value) in raw.versions {
        let v: RawVersion = serde_json::from_value(value).map_err(|e| format!("version {name}: {e}"))?;
        versions.push({
            let key = format!("{}/{}", raw.id, name);
            let progress = summary.get(&key).map(|row| Progress { note: row.note.clone(), ..row.progress.clone() });
            Version {
                title: raw.titles.get(&name).cloned().unwrap_or_else(|| raw.title.clone()),
                dir: format!("games/{}/{}", raw.id, name),
                game: raw.id.clone(),
                region: v.region,
                serial: v.serial,
                disc: v.disc,
                boot: v.boot,
                inputs: v.inputs,
                target: v.target,
                setup: v.setup,
                build_host: v.build.as_ref().and_then(|b| b.host.clone()),
                readme: v.build.and_then(|b| b.readme),
                source: v.source,
                progress,
                key,
                name,
            }
        });
    }
    Ok(Game { id: raw.id, title: raw.title, year: raw.year, titles: raw.titles, versions })
}

impl Catalog {
    pub fn version(&self, key: &str) -> Option<&Version> {
        self.games.iter().flat_map(|g| &g.versions).find(|v| v.key == key)
    }

    pub fn version_keys(&self) -> Vec<String> {
        self.games.iter().flat_map(|g| &g.versions).map(|v| v.key.clone()).collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::checkout;

    #[test]
    fn reads_every_game_in_the_checkout() {
        let catalog = load(&checkout()).expect("the checkout's game.json files parse");
        let ids: Vec<_> = catalog.games.iter().map(|g| g.id.as_str()).collect();
        assert_eq!(ids, ["rac1", "rac2", "rac3", "rac4"]);
        // In game.json's order: rac1's PAL first.
        assert_eq!(catalog.version_keys(), ["rac1/pal", "rac1/ntsc", "rac2/ntsc", "rac3/ntsc", "rac4/ntsc"]);
        let pal = catalog.version("rac1/pal").unwrap();
        assert_eq!(pal.serial, "SCES_509.16");
        assert_eq!(pal.dir, "games/rac1/pal");
        assert!(pal.disc.as_ref().is_some_and(|d| d.size.is_some() && d.sha1.is_some()));
    }

    #[test]
    fn regional_titles_and_progress_are_attached() {
        let dir = tempfile::tempdir().unwrap();
        let game = dir.path().join("games").join("rac9");
        std::fs::create_dir_all(&game).unwrap();
        std::fs::write(
            game.join("game.json"),
            r#"{"id": "rac9", "title": "Nine", "year": 2009, "titles": {"pal": "Nine (EU)"},
                "versions": {"pal": {"region": "PAL", "serial": "SCES_000.00", "extra": [1, 2]},
                             "ntsc": {"region": "NTSC-U", "serial": "SCUS_000.00"}}}"#,
        )
        .unwrap();
        std::fs::create_dir_all(dir.path().join("progress")).unwrap();
        std::fs::write(
            dir.path().join("progress").join("summary.json"),
            r#"{"rac9/pal": {"title": "Nine", "progress": {"matched_code": 5, "total_code": 10,
                "matched_functions": null, "total_functions": null, "fuzzy_percent": null,
                "date": "2026-10-08", "percent": 50.0}, "note": "all of it"}}"#,
        )
        .unwrap();
        let catalog = load(dir.path()).unwrap();
        let pal = catalog.version("rac9/pal").unwrap();
        assert_eq!(pal.title, "Nine (EU)");
        assert_eq!(pal.progress.as_ref().unwrap().percent, 50.0);
        assert_eq!(pal.progress.as_ref().unwrap().note.as_deref(), Some("all of it"));
        let ntsc = catalog.version("rac9/ntsc").unwrap();
        assert_eq!(ntsc.title, "Nine");
        assert!(ntsc.progress.is_none());
    }
}
