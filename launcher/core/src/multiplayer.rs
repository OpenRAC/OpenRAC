//! The player's multiplayer name: shown to the other players by online mods (openrac-mods'
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

/// The environment variable the game reads the multiplayer hub from, as `host:port`
/// (`[v6]:port` for an IPv6 address): every game's online play starts there (Settings,
/// "Multiplayer server"). The hub logs players in and lists the game servers they pick from.
pub const SERVER_ENV: &str = "OPENRAC_SERVER";

/// The official OpenRAC hub (the OpenRAC/Server repository, README "The hub"), the default.
pub const OFFICIAL_HOST: &str = "hub.openrac.dev";
pub const OFFICIAL_PORT: u16 = 7177;

/// The official game server launchers named before the hub: a setting that still names it is the
/// old default, not the player's choice, so it reads as the official hub ([`crate::config::load`]).
pub const FORMER_OFFICIAL_HOST: &str = "game.openrac.dev";
pub const FORMER_OFFICIAL_PORT: u16 = 7770;

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

/// Whether `host` names a server: an IPv4 or IPv6 address (without brackets), or a host name of
/// dot-separated labels of letters, digits and hyphens (1 to 63 each, no hyphen at either end,
/// 253 characters in all).
pub fn is_valid_host(host: &str) -> bool {
    if host.parse::<std::net::IpAddr>().is_ok() {
        return true;
    }
    let name = host.strip_suffix('.').unwrap_or(host);
    !name.is_empty()
        && name.len() <= 253
        && name.split('.').all(|label| {
            (1..=63).contains(&label.len())
                && label.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'-')
                && !label.starts_with('-')
                && !label.ends_with('-')
        })
}

/// Whether `port` is one a server listens on: 1 to 65535.
pub fn is_valid_port(port: u32) -> bool {
    (1..=65535).contains(&port)
}

/// `host:port`, with an IPv6 address in brackets (`[::1]:7770`).
pub fn address(host: &str, port: u16) -> String {
    if host.contains(':') {
        format!("[{host}]:{port}")
    } else {
        format!("{host}:{port}")
    }
}

/// The server the settings name, as the game gets it ([`SERVER_ENV`]): the official one when the
/// saved host or port is not valid (a settings file edited by hand).
pub fn server(config: &crate::config::Config) -> String {
    let host = config.multiplayer_server_host.trim();
    let port = u32::from(config.multiplayer_server_port);
    if is_valid_host(host) && is_valid_port(port) {
        address(host, config.multiplayer_server_port)
    } else {
        address(OFFICIAL_HOST, OFFICIAL_PORT)
    }
}

/// Whether a server answers at `host:port`: a TCP connection made within `timeout` (for each
/// address the name resolves to), and closed at once. Says why not otherwise.
pub fn reachable(host: &str, port: u32, timeout: std::time::Duration) -> Result<(), String> {
    use std::net::{TcpStream, ToSocketAddrs};
    let host = host.trim();
    if !is_valid_host(host) {
        return Err(format!("\"{host}\" is not a host name or an IP address"));
    }
    if !is_valid_port(port) {
        return Err(format!("{port} is not a port (1 to 65535)"));
    }
    let addrs: Vec<_> =
        (host, port as u16).to_socket_addrs().map_err(|e| format!("cannot find {host}: {e}"))?.collect();
    if addrs.is_empty() {
        return Err(format!("cannot find {host}"));
    }
    let mut last = String::new();
    for addr in addrs {
        match TcpStream::connect_timeout(&addr, timeout) {
            Ok(_) => return Ok(()),
            Err(e) => last = e.to_string(),
        }
    }
    Err(format!("no answer from {}: {last}", address(host, port as u16)))
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
    fn hosts_are_names_or_addresses() {
        for good in ["game.openrac.dev", "localhost", "my-box", "192.168.1.20", "::1", "fe80::1", "a.b.c.", "x1"] {
            assert!(is_valid_host(good), "{good}");
        }
        let long = format!("{}.dev", "a".repeat(64));
        for bad in ["", " ", "-box", "box-", "a..b", "host:7770", "[::1]", "two words", "émile.dev", "a_b.dev", &long]
        {
            assert!(!is_valid_host(bad), "{bad}");
        }
        assert!(is_valid_port(1) && is_valid_port(65535) && is_valid_port(7770));
        assert!(!is_valid_port(0) && !is_valid_port(65536));
    }

    #[test]
    fn the_server_is_host_colon_port_or_the_official_one() {
        let mut config = crate::config::Config::default();
        assert_eq!(server(&config), "hub.openrac.dev:7177");
        config.multiplayer_server_host = "192.168.1.20".into();
        config.multiplayer_server_port = 7000;
        assert_eq!(server(&config), "192.168.1.20:7000");
        config.multiplayer_server_host = "::1".into();
        assert_eq!(server(&config), "[::1]:7000");
        config.multiplayer_server_host = "not a host".into();
        assert_eq!(server(&config), "hub.openrac.dev:7177");
        config.multiplayer_server_host = "localhost".into();
        config.multiplayer_server_port = 0;
        assert_eq!(server(&config), "hub.openrac.dev:7177");
    }

    #[test]
    fn a_listening_server_is_reachable_and_a_closed_port_is_not() {
        let timeout = std::time::Duration::from_secs(2);
        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let port = u32::from(listener.local_addr().unwrap().port());
        assert_eq!(reachable("127.0.0.1", port, timeout), Ok(()));
        drop(listener);
        assert!(reachable("127.0.0.1", port, timeout).is_err());
        assert!(reachable("not a host", 7770, timeout).unwrap_err().contains("not a host name"));
        assert!(reachable("localhost", 0, timeout).unwrap_err().contains("not a port"));
    }

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
