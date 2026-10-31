//! The one error type of the mod hub (index.rs, resolver.rs, index_apply.rs, deeplink.rs): what
//! went wrong, which mod, and the one action the page can offer for it. The rest of the launcher
//! keeps `Result<_, String>`; `From<String>` lets this meet that code.
//!
//! The page words each kind and action itself (src/lib/modsText.ts): `detail` is for the log and a
//! "Copy details" box, never the headline.

use serde::{Deserialize, Serialize};

/// The error classes of the mod hub (the plan's taxonomy): each is handled its own way.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub enum ModErrorKind {
    /// Offline, a timeout, GitHub down: try again, or use the cached list.
    Transient,
    /// A signature or a hash that does not match: never retried or bypassed.
    UpstreamBad,
    /// A conflict, no fitting version, the game running: the player can fix it.
    UserFixable,
    /// An unknown mod, a release that went away.
    NotFound,
    /// Something the launcher got wrong.
    Bug,
}

/// The one fix a page offers next to an error.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase", tag = "type")]
pub enum ModAction {
    Retry,
    UseCached,
    /// No version fits this launcher's mod interface, but one fits the dev build's.
    SwitchToDev,
    /// The mod needs the dev build: the newest version that fits this one.
    UseStableVersion {
        id: String,
        version: String,
    },
    /// Turn `replace` off and install `with` in its place.
    ReplaceConflict {
        replace: String,
        with: String,
    },
    TurnOff {
        ids: Vec<String>,
    },
    CloseGame,
    /// Open the mod's issue tracker with the details filled in.
    Report {
        url: String,
    },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct ModError {
    pub kind: ModErrorKind,
    pub mod_id: Option<String>,
    /// The technical reason, for the log and "Copy details".
    pub detail: String,
    pub action: Option<ModAction>,
}

impl ModError {
    pub fn new(kind: ModErrorKind, detail: impl Into<String>) -> ModError {
        ModError { kind, mod_id: None, detail: detail.into(), action: None }
    }

    pub fn for_mod(mut self, id: &str) -> ModError {
        self.mod_id = Some(id.to_string());
        self
    }

    pub fn with(mut self, action: ModAction) -> ModError {
        self.action = Some(action);
        self
    }

    /// An error from older code that says only a sentence: sorted by what it says, so a failed
    /// download can be retried and a bad hash is never.
    pub fn from_text(text: &str) -> ModError {
        let lower = text.to_ascii_lowercase();
        if lower.contains("sha-256") || lower.contains("signature") || lower.contains("not the file") {
            ModError::new(ModErrorKind::UpstreamBad, text)
        } else if lower.contains("not found") || lower.contains("has no release") || lower.contains("holds no mod") {
            ModError::new(ModErrorKind::NotFound, text)
        } else if lower.contains("could not be fetched") || lower.contains("curl") || lower.contains("timed out") {
            ModError::new(ModErrorKind::Transient, text).with(ModAction::Retry)
        } else {
            ModError::new(ModErrorKind::Bug, text)
        }
    }
}

impl std::fmt::Display for ModError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match &self.mod_id {
            Some(id) => write!(f, "{id}: {}", self.detail),
            None => f.write_str(&self.detail),
        }
    }
}

impl From<String> for ModError {
    fn from(text: String) -> ModError {
        ModError::from_text(&text)
    }
}

impl From<&str> for ModError {
    fn from(text: &str) -> ModError {
        ModError::from_text(text)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn old_errors_are_sorted_by_what_they_say() {
        let hash = ModError::from("x.zip is not the file the release lists (its SHA-256 differs)".to_string());
        assert_eq!(hash.kind, ModErrorKind::UpstreamBad);
        assert_eq!(hash.action, None, "a bad hash is never retried");
        let net = ModError::from("https://x could not be fetched (timeout)");
        assert_eq!((net.kind, net.action), (ModErrorKind::Transient, Some(ModAction::Retry)));
        assert_eq!(ModError::from("https://x: not found").kind, ModErrorKind::NotFound);
        assert_eq!(ModError::from("something odd").kind, ModErrorKind::Bug);
    }

    #[test]
    fn it_reads_as_json_the_page_can_act_on() {
        let e = ModError::new(ModErrorKind::UserFixable, "conflict")
            .for_mod("lock-strafe")
            .with(ModAction::ReplaceConflict { replace: "classic-controls".into(), with: "lock-strafe".into() });
        let json = serde_json::to_value(&e).unwrap();
        assert_eq!(json["kind"], "userFixable");
        assert_eq!(json["modId"], "lock-strafe");
        assert_eq!(json["action"]["type"], "replaceConflict");
        assert_eq!(e.to_string(), "lock-strafe: conflict");
    }
}
