//! The player's multiplayer name: shown to the other players by online mods (rac1-mods'
//! `multiplayer`). The launcher keeps it in its settings (`multiplayerName`), makes a random one at
//! the first start, and gives it to the game when it plays, as the environment variable
//! [`ENV`], which a native mod reads (docs/port/MODS.md, section 9).

use std::collections::hash_map::RandomState;
use std::hash::{BuildHasher, Hasher};

/// The environment variable the game is started with: the player's name, valid
/// ([`is_valid`]).
pub const ENV: &str = "OPENRAC_PLAYER_NAME";

/// The environment variable the game reads this installation's key from: what a host bans.
pub const KEY_ENV: &str = "OPENRAC_IDENTITY_KEY";

/// The shortest and the longest name, in characters.
pub const MIN_LEN: usize = 3;
pub const MAX_LEN: usize = 16;

/// Whether `name` is a multiplayer name: 3 to 16 letters (A to Z, either case), digits and
/// underscores.
pub fn is_valid(name: &str) -> bool {
    (MIN_LEN..=MAX_LEN).contains(&name.len()) && name.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_')
}

/// The first half of a made-up name: at most 6 letters, so that a noun and 3 digits fit.
const ADJECTIVES: [&str; 24] = [
    "Brave", "Swift", "Lucky", "Rusty", "Sneaky", "Bold", "Clever", "Mighty", "Shiny", "Quick", "Wild", "Cosmic",
    "Plasma", "Turbo", "Hyper", "Silent", "Fuzzy", "Atomic", "Jolly", "Grumpy", "Daring", "Nimble", "Stormy", "Zesty",
];

/// The second half: at most 7 letters.
const NOUNS: [&str; 24] = [
    "Lombax", "Robot", "Wrench", "Bolt", "Blarg", "Comet", "Nebula", "Rocket", "Gadget", "Hover", "Raritan", "Taunter",
    "Glove", "Visor", "Drone", "Pilot", "Ranger", "Plumber", "Blaster", "Mutant", "Sprout", "Quasar", "Galaxy",
    "Crate",
];

/// A random name such as `SwiftLombax427`: an adjective, a noun and 2 or 3 digits, always valid.
pub fn random_name() -> String {
    random_name_from(seed())
}

/// The name the number `seed` picks: the same seed, the same name.
pub fn random_name_from(seed: u64) -> String {
    let adjective = ADJECTIVES[(seed % ADJECTIVES.len() as u64) as usize];
    let rest = seed / ADJECTIVES.len() as u64;
    let noun = NOUNS[(rest % NOUNS.len() as u64) as usize];
    let rest = rest / NOUNS.len() as u64;
    // Two digits or three, whichever the seed says, as long as the name stays short enough.
    let three = rest % 2 == 1 && adjective.len() + noun.len() + 3 <= MAX_LEN;
    let digits = if three { format!("{:03}", (rest / 2) % 1000) } else { format!("{:02}", (rest / 2) % 100) };
    let mut name = format!("{adjective}{noun}{digits}");
    name.truncate(MAX_LEN);
    name
}

/// A number no two starts share: the standard library's per-process random hasher keys, mixed with
/// the time. Not for secrets; enough for a name the player can change.
fn seed() -> u64 {
    let mut hasher = RandomState::new().build_hasher();
    let now = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_nanos()).unwrap_or(0);
    hasher.write_u128(now);
    hasher.finish()
}

/// Whether a key could be one of [`new_key`]'s: what the game sends and a save's note keeps.
pub fn is_valid_key(key: &str) -> bool {
    key.len() == 32 && key.bytes().all(|b| b.is_ascii_hexdigit())
}

/// A new key: 128 random bits as hex, with no name or address in it.
pub fn new_key() -> String {
    format!("{:016x}{:016x}", seed(), seed().wrapping_mul(0x9E37_79B9_7F4A_7C15))
}

/// Gives `config` a random name when it has no valid one (the first start, or a settings file from
/// before the name, or a name edited by hand into something invalid), and a key likewise, which
/// then never changes. True when it changed.
pub fn settle(config: &mut crate::config::Config) -> bool {
    let mut changed = false;
    if !is_valid(&config.multiplayer_name) {
        config.multiplayer_name = random_name();
        changed = true;
    }
    if !is_valid_key(&config.multiplayer_key) {
        config.multiplayer_key = new_key();
        changed = true;
    }
    changed
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn names_are_letters_digits_and_underscores_of_3_to_16() {
        for good in ["abc", "Brave_Lombax_42", "A1_", "x234567890123456"] {
            assert!(is_valid(good), "{good}");
        }
        for bad in ["", "ab", "x2345678901234567", "two words", "dash-name", "émile", "semi;colon"] {
            assert!(!is_valid(bad), "{bad}");
        }
    }

    #[test]
    fn every_made_up_name_is_valid() {
        for seed in 0..20_000u64 {
            let name = random_name_from(seed.wrapping_mul(0x9E37_79B9_7F4A_7C15));
            assert!(is_valid(&name), "{name}");
            assert!(name.chars().rev().take(2).all(|c| c.is_ascii_digit()), "{name}");
        }
        assert_eq!(random_name_from(7), random_name_from(7));
        assert!(is_valid(&random_name()));
    }

    #[test]
    fn settles_only_a_missing_or_invalid_name() {
        let mut config = crate::config::Config::default();
        assert!(config.multiplayer_name.is_empty());
        assert!(settle(&mut config));
        assert!(is_valid(&config.multiplayer_name));
        let kept = config.multiplayer_name.clone();
        assert!(!settle(&mut config));
        assert_eq!(config.multiplayer_name, kept);
        config.multiplayer_name = "no spaces please".into();
        assert!(settle(&mut config));
        assert!(is_valid(&config.multiplayer_name));
    }

    #[test]
    fn a_key_is_made_once_and_kept() {
        let mut config = crate::config::Config::default();
        assert!(config.multiplayer_key.is_empty());
        assert!(settle(&mut config));
        assert!(is_valid_key(&config.multiplayer_key));
        let kept = config.multiplayer_key.clone();
        assert!(!settle(&mut config));
        assert_eq!(config.multiplayer_key, kept);
        // A new name leaves the key: a ban does not go with a name.
        config.multiplayer_name = "no spaces please".into();
        assert!(settle(&mut config));
        assert_eq!(config.multiplayer_key, kept);
        assert_ne!(new_key(), new_key());
    }
}
