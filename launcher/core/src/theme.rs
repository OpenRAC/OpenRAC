//! The launcher's colours for the game: the theme picked in Settings and the picked game's accent,
//! as the launcher wears them, handed to the game's options drawer, so that it looks
//! like the launcher it was started from (port/game/common/ui_colors.h). The colours
//! are src/lib/palettes.json's, the file the launcher's pages read.

const PALETTES: &str = include_str!("../../src/lib/palettes.json");

/// The colours the game's menu (its drawer, its bar of tabs) draws with, by their app.css names.
const DRAWER: &[&str] = &[
    "--ink-deep",
    "--panel",
    "--panel-hi",
    "--brand",
    "--bar-top",
    "--bar-bottom",
    "--bar-stripe",
    "--bar-stripe-alt",
    "--bar-ink",
    "--bar-ink-deep",
    "--amber",
    "--gold",
    "--text",
    "--soft",
    "--dim",
    "--line",
    "--line-strong",
    "--err",
    "--control",
    "--control-hi",
    "--control-card",
    "--control-card-hi",
    "--menu-top",
    "--menu-bottom",
    "--menu-glow",
];

/// The variable the game reads them from: `name=#rrggbb[aa]` pairs, comma-separated, each name
/// app.css's without its leading dashes ("panel-hi=#22222f,line=#9ca1d438,...").
pub const ENV: &str = "OPENRAC_UI_COLORS";

/// The drawer's colours for theme `theme` (an unknown one is the launcher's own) and the accent of
/// `game`.
pub fn ui_colors(theme: &str, game: Option<&str>) -> String {
    let data: serde_json::Value = serde_json::from_str(PALETTES).unwrap_or_default();
    let palette =
        data["palettes"].as_array().and_then(|all| all.iter().find(|p| p["id"] == theme)).map(|p| &p["colors"]);
    let accent = game.map(|game| &data["accents"][game]);
    // The OpenRAC theme (or an unknown one) sets no surfaces of its own.
    let own_theme = palette.is_none_or(|colors| colors.as_object().is_none_or(|c| c.is_empty()));
    DRAWER
        .iter()
        .filter_map(|token| {
            // The OpenRAC theme wears the game's own coal (its accent's --tint-*, as app.css does);
            // another theme keeps its own surfaces.
            let tint = format!("--tint-{}", token.trim_start_matches('-'));
            let tinted = own_theme.then(|| accent.and_then(|a| a[tint.as_str()].as_str())).flatten();
            let value = tinted.or_else(|| {
                [accent, palette, Some(&data["base"])].into_iter().flatten().find_map(|colors| colors[*token].as_str())
            })?;
            Some(format!("{}={}", token.trim_start_matches('-'), hex(value)?))
        })
        .collect::<Vec<_>>()
        .join(",")
}

/// A CSS colour as palettes.json writes them, `#rrggbb` or `rgb(r g b / a)`, as `#rrggbb[aa]`.
fn hex(css: &str) -> Option<String> {
    if css.len() == 7 && css.starts_with('#') && css[1..].chars().all(|c| c.is_ascii_hexdigit()) {
        return Some(css.to_ascii_lowercase());
    }
    let inner = css.strip_prefix("rgb(")?.strip_suffix(')')?;
    let (rgb, alpha) = inner.split_once('/').map_or((inner, None), |(c, a)| (c, Some(a)));
    let channels = rgb.split_whitespace().map(|c| c.parse::<u8>().ok()).collect::<Option<Vec<_>>>()?;
    let [r, g, b] = channels[..] else { return None };
    let mut out = format!("#{r:02x}{g:02x}{b:02x}");
    if let Some(alpha) = alpha {
        let a = alpha.trim().parse::<f32>().ok()?.clamp(0.0, 1.0);
        out += &format!("{:02x}", (a * 255.0).round() as u8);
    }
    Some(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn color<'a>(colors: &'a str, name: &str) -> Option<&'a str> {
        colors.split(',').find_map(|pair| pair.strip_prefix(name)?.strip_prefix('='))
    }

    #[test]
    fn hands_the_game_the_theme_and_the_accent() {
        let own = ui_colors("openrac", None);
        // All but the menu's own ground, which only a game's accent sets.
        assert_eq!(own.split(',').count(), DRAWER.len() - 3, "{own}");
        assert_eq!(color(&own, "menu-top"), None);
        assert_eq!(color(&own, "panel"), Some("#121212"));
        assert_eq!(color(&own, "line"), Some("#ffffff17"));
        assert_eq!(color(&own, "amber"), Some("#f5c040"));
        // A theme sets its own; what it leaves is the launcher's.
        let nord = ui_colors("nord", None);
        assert_eq!(color(&nord, "panel"), Some("#3b4252"));
        assert_eq!(color(&nord, "amber"), Some("#f5c040"));
        assert_eq!(color(&own, "control"), Some("#121212"));
        assert_eq!(color(&own, "control-card"), Some("#1e1e1e"));
        assert_eq!(color(&nord, "control"), Some("#3b4252"));
        assert_eq!(ui_colors("no-such", None), own);
        // The game's accent; Ratchet & Clank's is the launcher's own.
        let rac3 = ui_colors("nord", Some("rac3"));
        assert_eq!(color(&rac3, "amber"), Some("#ffd23f"));
        let rac2 = ui_colors("openrac", Some("rac2"));
        assert_eq!(color(&rac2, "menu-top"), Some("#161f22"));
        // Going Commando's coal, on the OpenRAC theme only.
        assert_eq!(color(&rac2, "panel-hi"), Some("#192125"));
        assert_eq!(color(&ui_colors("nord", Some("rac2")), "panel-hi"), Some("#434c5e"));
        // The bar's stripes and ink too, for the game's menu bar (debug/menu_window.cpp).
        assert_eq!(color(&own, "bar-stripe"), Some("#c99222"));
        assert_eq!(color(&rac3, "bar-stripe-alt"), Some("#d7ac27"));
        assert_eq!(color(&rac3, "bar-ink"), Some("#2a2000"));
        assert_eq!(color(&rac3, "panel"), Some("#3b4252"));
        assert_eq!(ui_colors("openrac", Some("rac1")), own);
    }

    #[test]
    fn reads_the_colours_palettes_json_writes() {
        assert_eq!(hex("#E8F0FF").as_deref(), Some("#e8f0ff"));
        assert_eq!(hex("rgb(168 153 132 / 0.36)").as_deref(), Some("#a899845c"));
        assert_eq!(hex("rgb(1 2 3)").as_deref(), Some("#010203"));
        assert_eq!(hex("var(--amber)"), None);
        assert_eq!(hex("rgb(1 2)"), None);
    }
}
