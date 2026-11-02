//! The decompilations' latest progress from bytematch.dev, fetched at the launcher's start.
//!
//! The page asks for it (`sync_progress_from_web` in `src-tauri/src/lib.rs`) when its own fetch of
//! the site is refused by the content security policy (docs/ARCHITECTURE.md, Security). Two kinds
//! of request: the site's list of every project ([`catalog::BYTEMATCH_PROJECTS`]), then the own
//! JSON of each project a version of the checkout takes ([`catalog::bytematch_projects_for`]),
//! which carries its functional C summary. The figures are kept in the launcher's progress copy
//! ([`catalog::update_progress_from_bytematch`]); offline, that copy is read as it was.
//!
//! The requests go through [`Fetch`]: [`QuickCurl`] in the launcher, an answer from memory in the
//! tests, so no test needs the network.
//!
//! [`QuickCurl`]: crate::remote::QuickCurl

use std::collections::BTreeMap;
use std::path::Path;

use crate::catalog;
use crate::remote::Fetch;

/// `url`'s body as text, or None when it could not be fetched or is not UTF-8.
fn fetch_text(fetch: &dyn Fetch, url: &str) -> Option<String> {
    fetch.get(url, false).ok().and_then(|bytes| String::from_utf8(bytes).ok())
}

/// Fetches bytematch.dev's project list and the own JSON of each project the checkout `root`'s
/// versions take, and keeps the figures in `cache` (the launcher's progress copy). A project whose
/// JSON cannot be fetched keeps its figures without the functional summary. An error when the list
/// cannot be fetched (`cache` is then left as it was) or the figures cannot be kept.
pub fn sync(root: &Path, cache: &Path, fetch: &dyn Fetch) -> Result<(), String> {
    let list = fetch_text(fetch, catalog::BYTEMATCH_PROJECTS)
        .ok_or_else(|| format!("{} could not be fetched", catalog::BYTEMATCH_PROJECTS))?;
    let mut jsons = BTreeMap::new();
    for (key, slug) in catalog::bytematch_projects_for(root, &list).unwrap_or_default() {
        if let Some(text) = fetch_text(fetch, &catalog::bytematch_project_url_for(&key, &slug)) {
            jsons.insert(key, text);
        }
    }
    catalog::update_progress_from_bytematch(root, cache, &list, &jsons).map(|_| ())
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;

    use super::*;

    /// Answers from memory, and remembers what was asked.
    #[derive(Default)]
    struct Site {
        pages: BTreeMap<String, Vec<u8>>,
        asked: RefCell<Vec<String>>,
    }

    impl Fetch for Site {
        fn get(&self, url: &str, api: bool) -> Result<Vec<u8>, String> {
            assert!(!api, "bytematch.dev is not GitHub's API");
            self.asked.borrow_mut().push(url.to_string());
            self.pages.get(url).cloned().ok_or_else(|| format!("{url}: not found"))
        }
    }

    /// A checkout with one game of two versions: PAL found by its serial, NTSC by the project
    /// its game.json names.
    fn checkout() -> tempfile::TempDir {
        let dir = tempfile::tempdir().unwrap();
        let game = dir.path().join("games/rac1");
        std::fs::create_dir_all(&game).unwrap();
        std::fs::write(
            game.join("game.json"),
            r#"{"id": "rac1", "title": "Ratchet & Clank", "versions": {
                "pal": {"region": "PAL", "serial": "SCES_509.16"},
                "ntsc": {"region": "NTSC-U", "serial": "SCUS_971.99", "bytematch": "lombyte-project/Lombyte"}}}"#,
        )
        .unwrap();
        dir
    }

    const PROJECTS: &str = r#"[
        {"owner": "OpenRAC", "repo": "rac1-decomp", "version": "SCES_509.16", "timestamp": "2026-10-16T01:00:00.000Z",
         "measures": {"matched_code": 3199668, "total_code": 3712808, "matched_code_percent": 86.18}},
        {"owner": "lombyte-project", "repo": "Lombyte", "version": "SCUS_971.99",
         "measures": {"matched_code": 3300000, "total_code": 3677000, "matched_code_percent": 89.75}}
    ]"#;

    #[test]
    fn the_list_and_each_projects_json_are_kept() {
        let root = checkout();
        let mut site = Site::default();
        site.pages.insert(catalog::BYTEMATCH_PROJECTS.into(), PROJECTS.into());
        site.pages.insert(
            "https://bytematch.dev/OpenRAC/rac1-decomp.json".into(),
            br#"{"functional": {"code": 370968, "code_percent": 9.99, "functions": 389}}"#.to_vec(),
        );
        // Lombyte's own JSON is not there: its figures are kept without the functional summary.
        let mine = tempfile::tempdir().unwrap();
        let cache = mine.path().join("progress.json");

        sync(root.path(), &cache, &site).unwrap();

        let mut asked = site.asked.borrow().clone();
        assert_eq!(asked.remove(0), catalog::BYTEMATCH_PROJECTS, "the list first");
        asked.sort();
        assert_eq!(
            asked,
            ["https://bytematch.dev/OpenRAC/rac1-decomp.json", "https://bytematch.dev/lombyte-project/Lombyte.json"]
        );
        let catalog = catalog::load_with(root.path(), Some(&cache)).unwrap();
        let pal = catalog.version("rac1/pal").unwrap().progress.as_ref().unwrap();
        assert_eq!(pal.percent, 86.18);
        assert_eq!(pal.functional.as_ref().map(|f| f.functions), Some(389));
        let ntsc = catalog.version("rac1/ntsc").unwrap().progress.as_ref().unwrap();
        assert_eq!(ntsc.percent, 89.75);
        assert!(ntsc.functional.is_none());
    }

    #[test]
    fn rac5_uses_its_us_report_even_when_the_project_defaults_to_pal() {
        let root = tempfile::tempdir().unwrap();
        let game = root.path().join("games/rac5");
        std::fs::create_dir_all(&game).unwrap();
        std::fs::write(
            game.join("game.json"),
            r#"{"id":"rac5","title":"Size Matters","versions":{"ntsc":{
                "region":"NTSC-U","serial":"SCUS_976.15","bytematch":"OpenRAC/rac5-sm-decomp"}}}"#,
        )
        .unwrap();
        let mut site = Site::default();
        site.pages.insert(
            catalog::BYTEMATCH_PROJECTS.into(),
            br#"[{"owner":"OpenRAC","repo":"rac5-sm-decomp","version":"SCES_550.19",
                "timestamp":"2026-10-27T17:10:01Z","measures":{
                "matched_code":87784,"total_code":6418460,"matched_code_percent":1.37}}]"#
                .to_vec(),
        );
        site.pages.insert(
            "https://bytematch.dev/OpenRAC/rac5-sm-decomp/SCUS_976.15.json".into(),
            br#"{"measures":{"matched_code":618320,"total_code":6418204,
                "matched_code_percent":9.63385},"commit":{"timestamp":"2026-11-01T18:00:00Z"}}"#
                .to_vec(),
        );
        let mine = tempfile::tempdir().unwrap();
        let cache = mine.path().join("progress.json");

        sync(root.path(), &cache, &site).unwrap();

        assert!(site.asked.borrow().iter().any(|url| url.ends_with("/SCUS_976.15.json")));
        let catalog = catalog::load_with(root.path(), Some(&cache)).unwrap();
        let progress = catalog.version("rac5/ntsc").unwrap().progress.as_ref().unwrap();
        assert_eq!((progress.matched_code, progress.total_code), (618320, 6418204));
        assert_eq!(progress.date.as_deref(), Some("2026-11-01"));

        site.pages.remove("https://bytematch.dev/OpenRAC/rac5-sm-decomp/SCUS_976.15.json");
        sync(root.path(), &cache, &site).unwrap();
        let offline = catalog::load_with(root.path(), Some(&cache)).unwrap();
        assert_eq!(offline.version("rac5/ntsc").unwrap().progress.as_ref().unwrap().matched_code, 618320);
    }

    #[test]
    fn offline_the_kept_copy_stays() {
        let root = checkout();
        let mine = tempfile::tempdir().unwrap();
        let cache = mine.path().join("progress.json");
        std::fs::write(&cache, "{\"kept\": true}").unwrap();

        let site = Site::default();
        assert!(sync(root.path(), &cache, &site).is_err());
        assert_eq!(*site.asked.borrow(), [catalog::BYTEMATCH_PROJECTS], "nothing more is asked");
        assert_eq!(std::fs::read_to_string(&cache).unwrap(), "{\"kept\": true}");
    }

    #[test]
    fn a_list_that_is_not_text_counts_as_not_fetched() {
        let root = checkout();
        let mine = tempfile::tempdir().unwrap();
        let cache = mine.path().join("progress.json");
        let mut site = Site::default();
        site.pages.insert(catalog::BYTEMATCH_PROJECTS.into(), vec![0xff, 0xfe]);
        assert!(sync(root.path(), &cache, &site).is_err());
        assert!(!cache.exists());
    }
}
