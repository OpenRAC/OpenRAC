//! `openrac://` links from the website's Install buttons: which mods to show in the change set.
//!
//! - `openrac://install/<id>` and `openrac://install/<id>@<version>`
//! - `openrac://install?mods=<id>[@<version>],<id>…` (at most 32)
//! - `&game=rac1&disc=pal`: hints for the page to open
//!
//! A link only opens the change set: installing always needs the player's click. It names mods by
//! id, which the launcher looks up in its signed index; it is never fetched as a URL. So anything
//! that is not exactly this shape is refused: another verb or host, `..`, any `%` (no decoding
//! tricks), spaces, a fragment.

use serde::{Deserialize, Serialize};

use crate::mod_error::{ModError, ModErrorKind};
use crate::plugins;

pub const SCHEME: &str = "openrac://";
/// The most mods one link may ask for.
pub const MAX_MODS: usize = 32;
/// Longer than any honest link (32 ids of 64 characters with versions).
const MAX_LEN: usize = 4096;

/// One mod a link asks for, maybe at a version.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct WantedMod {
    pub id: String,
    pub version: Option<String>,
}

/// What an install link asks for.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
#[ts(rename = "DeepLinkInstall")]
pub struct Install {
    pub mods: Vec<WantedMod>,
    pub game: Option<String>,
    pub disc: Option<String>,
}

fn refused(why: &str) -> ModError {
    ModError::new(ModErrorKind::UserFixable, format!("This link can't be opened: {why}."))
}

/// A short lower-case word: a game or disc hint.
fn hint(text: &str) -> bool {
    !text.is_empty() && text.len() <= 16 && text.chars().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit())
}

/// `id` or `id@version`, checked.
fn wanted(spec: &str) -> Result<WantedMod, ModError> {
    let (id, version) = match spec.split_once('@') {
        Some((id, v)) => (id, Some(v)),
        None => (spec, None),
    };
    if !plugins::valid_id(id) {
        return Err(refused(&format!("{spec:?} is not a mod id")));
    }
    if let Some(v) = version {
        if plugins::parse_version(v).is_none() {
            return Err(refused(&format!("{v:?} is not a version")));
        }
    }
    Ok(WantedMod { id: id.to_string(), version: version.map(str::to_string) })
}

/// Reads an install link. Err: why it is refused.
pub fn parse(url: &str) -> Result<Install, ModError> {
    if url.len() > MAX_LEN {
        return Err(refused("it is too long"));
    }
    // The scheme's case does not matter (RFC 3986); nothing else is lowered.
    let rest = match url.get(..SCHEME.len()) {
        Some(head) if head.eq_ignore_ascii_case(SCHEME) => &url[SCHEME.len()..],
        _ => return Err(refused("it is not an openrac:// link")),
    };
    if rest.chars().any(|c| c.is_whitespace() || c.is_control()) || rest.contains(['%', '#', '\\', '"', '\'']) {
        return Err(refused("it holds characters an install link never has"));
    }
    let (path, query) = match rest.split_once('?') {
        Some((p, q)) => (p, Some(q)),
        None => (rest, None),
    };
    // install, install/ or install/<id>[@<version>]: nothing more, no user, port or other verb.
    let one = match path.split_once('/') {
        None if path == "install" => None,
        Some(("install", "")) => None,
        Some(("install", spec)) if !spec.contains('/') => Some(spec),
        _ => return Err(refused("only install links are understood")),
    };
    let mut mods: Option<Vec<WantedMod>> = None;
    let (mut game, mut disc) = (None, None);
    for pair in query.unwrap_or_default().split('&').filter(|p| !p.is_empty()) {
        let (key, value) = pair.split_once('=').unwrap_or((pair, ""));
        let slot_taken = match key {
            "mods" => mods.is_some(),
            "game" => game.is_some(),
            "disc" => disc.is_some(),
            // Another key (a tracker's) is left alone: it asks for nothing.
            _ => continue,
        };
        if slot_taken {
            return Err(refused(&format!("it names {key} twice")));
        }
        match key {
            "mods" => {
                let specs: Vec<&str> = value.split(',').collect();
                if specs.len() > MAX_MODS {
                    return Err(refused(&format!("it asks for more than {MAX_MODS} mods")));
                }
                mods = Some(specs.into_iter().map(wanted).collect::<Result<_, _>>()?);
            }
            "game" if hint(value) => game = Some(value.to_string()),
            "disc" if hint(value) => disc = Some(value.to_string()),
            _ => return Err(refused(&format!("{key}={value:?} is not a game or disc"))),
        }
    }
    let mods = match (one, mods) {
        (Some(spec), None) => vec![wanted(spec)?],
        (None, Some(list)) => list,
        (Some(_), Some(_)) => return Err(refused("it names mods twice")),
        (None, None) => return Err(refused("it names no mod")),
    };
    let mut seen = std::collections::BTreeSet::new();
    if let Some(dup) = mods.iter().find(|m| !seen.insert(m.id.as_str())) {
        return Err(refused(&format!("it names {} twice", dup.id)));
    }
    Ok(Install { mods, game, disc })
}

/// The link a start was handed, from its arguments (the program's name first). The launcher's own
/// entries pass it after `--` (`Exec=… -- %u`, Windows `"…" -- "%1"`), so it can never be read as
/// an option; Tauri's own registration passes it alone. Either way it must be the one argument
/// and start with `openrac://`: a link that breaks out of its quotes brings more arguments, and
/// then nothing is taken.
pub fn link_from_args<S: AsRef<str>>(args: &[S]) -> Option<String> {
    let link = match args.get(1..)? {
        [dash, link] if dash.as_ref() == "--" => link.as_ref(),
        [link] => link.as_ref(),
        _ => return None,
    };
    let head = link.get(..SCHEME.len())?;
    head.eq_ignore_ascii_case(SCHEME).then(|| link.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ok(url: &str) -> Install {
        parse(url).unwrap_or_else(|e| panic!("{url}: {e}"))
    }

    fn want(id: &str, version: Option<&str>) -> WantedMod {
        WantedMod { id: id.into(), version: version.map(str::to_string) }
    }

    #[test]
    fn the_three_forms_are_read() {
        assert_eq!(ok("openrac://install/lock-strafe").mods, vec![want("lock-strafe", None)]);
        assert_eq!(ok("openrac://install/lock-strafe@1.2.0").mods, vec![want("lock-strafe", Some("1.2.0"))]);
        let many = ok("openrac://install?mods=lock-strafe@1.2.0,camera-tweaks&game=rac1&disc=pal");
        assert_eq!(many.mods, vec![want("lock-strafe", Some("1.2.0")), want("camera-tweaks", None)]);
        assert_eq!((many.game.as_deref(), many.disc.as_deref()), (Some("rac1"), Some("pal")));
        // The forms a URL parser may hand on: a slash before the query, the scheme in capitals.
        assert_eq!(ok("openrac://install/?mods=a,b").mods.len(), 2);
        assert_eq!(ok("OpenRAC://install/a").mods, vec![want("a", None)]);
        assert_eq!(
            ok("openrac://install/item-randomizer@2.0.0-beta.1").mods[0].version.as_deref(),
            Some("2.0.0-beta.1")
        );
        // A tracker's key asks for nothing and is left alone.
        assert_eq!(ok("openrac://install/a?utm_source=site").mods.len(), 1);
    }

    #[test]
    fn anything_else_is_refused() {
        let thirty_three = (0..33).map(|i| format!("m{i}")).collect::<Vec<_>>().join(",");
        let refused = [
            "",
            "javascript:alert(1)",
            "https://openrac.dev/mods/a",
            "openrac:install/a",
            "openrac://remove/a",
            "openrac://evil.example/install/a",
            "openrac://user@install/a",
            "openrac://install:80/a",
            "openrac://install/../x",
            "openrac://install/a/../b",
            "openrac://install/a/b",
            "openrac://install/%2e%2e",
            "openrac://install/lock%2Dstrafe",
            "openrac://install/a#b",
            "openrac://install/a b",
            "openrac://install/a\n",
            "openrac://install/A",
            "openrac://install/a@1.2",
            "openrac://install/a@latest",
            "openrac://install/a@",
            "openrac://install/-a",
            "openrac://install",
            "openrac://install?mods=",
            "openrac://install?mods=a,,b",
            "openrac://install?mods=a,a",
            "openrac://install?mods=a&mods=b",
            "openrac://install/a?mods=b",
            "openrac://install/a?game=../x",
            "openrac://install/a?disc=PAL",
            "openrac://x\" --inspect \"",
            "openrac://install/a\" --inspect=9229 \"",
        ];
        for url in refused {
            assert!(parse(url).is_err(), "{url:?} was taken");
        }
        assert!(parse(&format!("openrac://install?mods={thirty_three}")).is_err());
        let thirty_two = (0..32).map(|i| format!("m{i}")).collect::<Vec<_>>().join(",");
        assert_eq!(ok(&format!("openrac://install?mods={thirty_two}")).mods.len(), 32);
        assert!(parse(&format!("openrac://install/{}", "a".repeat(5000))).is_err());
    }

    #[test]
    fn a_start_takes_only_the_one_argument_after_the_dash() {
        let args = |a: &[&str]| a.iter().map(|s| s.to_string()).collect::<Vec<_>>();
        let link = "openrac://install/a";
        assert_eq!(link_from_args(&args(&["launcher", "--", link])).as_deref(), Some(link));
        assert_eq!(link_from_args(&args(&["launcher", link])).as_deref(), Some(link));
        // A link that broke out of its quotes brings more arguments: none is taken.
        assert_eq!(link_from_args(&args(&["launcher", "--", "openrac://x", "--inspect", ""])), None);
        assert_eq!(link_from_args(&args(&["launcher", "openrac://x", "--inspect"])), None);
        assert_eq!(link_from_args(&args(&["launcher", "--inspect", "--", link])), None);
        assert_eq!(link_from_args(&args(&["launcher", "--", "https://example.com"])), None);
        assert_eq!(link_from_args(&args(&["launcher", "--", "--inspect"])), None);
        assert_eq!(link_from_args(&args(&["launcher", "--"])), None);
        assert_eq!(link_from_args(&args(&["launcher"])), None);
        assert_eq!(link_from_args::<String>(&[]), None);
    }
}
