# The launcher's look

The launcher looks like [openrac.dev](https://openrac.dev): the same black
page, gold header bar, type and per-game cards. Everything here comes from
[OpenRAC/openrac-site](https://github.com/OpenRAC/openrac-site) and its
`STYLE.md`; when the site changes, change the files in the last section to
match. The game's own menu (the desktop's bar and window, Android's drawer)
follows the same rules.

## In short

- One look for every game: the gold (`--amber`) and warm coals below (user
  decision). A game keeps only its picture, its card gradient, its title's
  typeface and the light under the bar, in its own colour (`--glow`, the only
  key of its `palettes.json` `accents`). No second accent colour.
- No grey, no brown and no white mix as a fill (`rgb(255 255 255 / …)`). The
  only neutrals are the blacks and coals below; a thin `--line` only splits
  things (rows of a list, a bar from the page).
- The page is a warm coal (`--ink` `#1c1a16`, `--ink-deep` `#14120e`), never
  navy and never pure black: text on a black page glares and blurs for many
  eyes (halation; user decision), so the launcher sits at a dimmed dark
  theme's lightness, a little warmer than grey so it goes with the gold
  (OKLCH chroma 0.008, the gold's hue). The game list beside it (and Android's
  bar at the foot of a narrow window) is darker than the page: `--rail` `#161410`.
  Android's rail on the left has no ground of its own: the page's runs under it. Pure black is the OLED
  theme's alone.
- The light under the bar: a soft glow of `--glow` (the picked game's colour,
  gold for Ratchet & Clank) laid over the window from the bar down
  (`body::after` in app.css: two radial gradients, 13 % and 7 %, screen
  blend, so it only lightens), fading out by about two thirds down. It eases
  from game to game, on every theme (OLED too, user decision). The game's menu window has the
  same light along its top (8 %, see below).
- Text is an off white (`--text` `#e6e6eb`, about 15:1 on the page), not a
  near white: more contrast than that only tires the eyes.
- Cards and panels have no border and no stripes: a flat `#24221e`
  (`--panel`, `--card`), `#2c2a26` when hovered or open (`--panel-hi`,
  `--card-hi`). A hint inside a page (Mods off, say) is warm, `--panel` mixed
  9 % with `--amber`, as the Get mods card. Unlike the site, stripes on a big surface tire the eyes (user
  decision). The one place left with stripes is the game menu's right side:
  faint, only in its top left corner (user decision).
- Controls (buttons, fields, chips, segmented choices, the search box) are
  filled with `--control` and have no edge or border (user decision: gold
  edges looked bad). The fill alone sets a control apart.
- Contrast rule: a control is one step lighter than what is behind it, in the
  blacks and coals only, never navy: `#24221e` on the page (`#2e2c28` under
  the pointer), `#2e2c28` on a `#24221e` card (`#363430`). See "Control
  fills" below.
- Switches: the track is a track (`--ink-deep`, a dim knob); on, it takes the gold tint of a control that is on (`--on-fill`) and a
  gold knob. Never a solid gold track: a list of switches that are on would be
  a wall of gold.
- Controls stand on their own, with a 6 px gap between them. Never join a row
  of buttons or tabs into one capsule with dividers. A segmented choice is the
  one exception: its values sit on one strip, the picked one in the gold tint.
- On or picked: a gold tint on the page's black (`--on-fill`, the accent at
  22 % over `--ink`) with gold text (`aria-pressed`, `data-on`,
  `aria-checked` on `.segmented`). A picked row in a list (the
  game list) is the accent at 12 %. A row under the pointer is `--control`.
- Badges, tags, count chips and progress tracks sit on `--ink-deep`, with no
  border.
- Corners: 6 px (`--radius-sm`) for buttons, fields, chips, tabs and small
  boxes; 4 px (`--radius-xs`) for badges; 12 px (`--radius`) for cards, panels,
  pictures and dialogs. Round only for what is really round (dots, switches,
  avatars, count bubbles, icon-only circles).
- Focus on a field: a 2 px gold ring inside it. Other controls keep the
  focus outline.
- The main action: a flat fill in the accent with dark text and no edge
  (`button.primary`), lifting 2 px under the pointer.
- Anything you can tap is at least 40 px tall (44 px on Android); fields on
  Android use 16 px text.
- The header on a desktop has no bar of its own (user decision): it lies over
  the page, which starts at the window's top (the light under it too), so the
  ground is one from top to bottom. The logo's name is in `--text`, its first
  letter (the mark) in `--amber`, as on Android's rail. Its buttons (the tabs
  Games, Mods, Develop, Settings; the site and Discord icons at the right; a
  job; an update; the version badge) are the theme's buttons (`--control`,
  `--control-hi` under the pointer), 32 px tall, gold text in Inter Medium
  (500, as thin as the logo beside them), no edge. The open tab is filled with
  `--amber`, its words in `--ink-deep`, and its chip slides from tab to tab.
  Once the page scrolls under the header, a haze fades in behind it: blurred,
  darkened at the top and fading out below, with no edge (never shown at a
  page's top). The game's menu bar on the desktop looks the same (see below),
  without icons.
- A main button (Play, the drawer's Resume over the game) is a flat
  `--amber` fill with dark text, no stripes, no edge.
- A game's title box on its page is the launcher's own: `--panel` at 92 % over
  the picture, the title in `--gold`, the year and the notes in `--amber`, and
  the shade over the picture is `--ink` (`themeStyle` in `src/lib/themes.ts`).
  The site's own card colours (navy boxes, lavender text) are not used.

## Colours

`src/app.css`, on `:root`, with the site's names (its `@theme` in
`src/app/globals.css`):

| Token                                    | Value                                | Use                                                   |
| ---------------------------------------- | ------------------------------------ | ----------------------------------------------------- |
| `--ink`                                  | `#1c1a16`                            | the page                                              |
| `--ink-deep`                             | `#14120e`                            | badges, tracks, job output, the darkest parts         |
| `--rail`                                 | `#161410`                            | the game list beside the page, Android's rail         |
| `--panel`                                | `#24221e`                            | cards and panels (`--card`)                           |
| `--panel-hi`                             | `#2c2a26`                            | a card under the pointer or open (`--card-hi`)        |
| `--glow`                                 | `#f5c040` (a game's own: its accent) | the light under the bar and along the game menu's top |
| `--brand`                                | `#d6a020`                            | section numbers, small labels                         |
| `--amber`                                | `#f5c040`                            | the main button, links, focus rings, what is on       |
| `--gold`                                 | `#ffe08f`                            | headings, numbers                                     |
| `--lav`, `--soft`, `--dim`               | `#d4d4de`, `#b4b4bf`, `#8b8b98`      | secondary text, from most to least prominent          |
| `--line`, `--line-strong`                | `rgb(255 255 255 / .09)`, `/.16`     | lines between rows; never round a control             |
| `--text`, `--body`                       | `#e6e6eb`, `#cacad3`                 | text, neutral so the gold is the only accent          |
| `--control`, `--control-hi`              | `#24221e`, `#2e2c28`                 | plain controls on the page                            |
| `--control-card`, `--control-card-hi`    | `#2e2c28`, `#363430`                 | plain controls on a card                              |
| `--on-fill`, `--on-fill-hi`              | the accent at 22 % / 28 % on ink     | a control that is on or picked                        |
| `--radius`, `--radius-sm`, `--radius-xs` | 12, 6, 4 px                          | cards; controls; badges                               |
| `--bar-stripe`, `--bar-stripe-alt`       | `#c99222`, `#bd8619`                 | the header bar's stripes                              |
| `--bar-ink`, `--bar-ink-deep`            | `#2a1800`, `#1b1206`                 | text on gold (desktop: `--ink-deep`)                  |
| `--bar-name`                             | `#000`                               | the logo on the bar (desktop: `--ink-deep`)           |
| `--on-game`                              | `#fff`                               | text on a game's colours or picture                   |

The site has no status colours; the launcher adds `--ok` (`#8fd694`), `--warn`
(amber) and `--err` (`#ff8a7a`), used only in pills and messages. The theme is
dark only, as the site is.

These are the "OpenRAC" theme. The other themes (Settings → General: OLED
black, Nord, Catppuccin Mocha) live in `src/lib/palettes.json`, read by
`src/lib/palettes.ts` and the core (`core/src/theme.rs`). A theme sets the
page, panel, control, text, line and status tokens, never the
accent (`--amber`, `--gold`, `--brand`, the bar's): that stays gold. So a colour that should follow the theme uses a
token, such as `color-mix(in srgb, var(--ink-deep) 75%, transparent)` for a
shade over a picture, not a fixed `rgb()`.

The game's options drawer wears the same colours: Play passes them in
`OPENRAC_UI_COLORS` (`port/game/common/ui_colors.h`). For that,
`palettes.json` repeats app.css's own colours under `base`, and a test keeps
the two equal.

### Control fills

Buttons, fields, chips and tabs have a solid fill and no edge. The fill is one
step lighter than what is behind it (each surface above the one under it is
lighter, as dark themes show depth), and it stays in the blacks and coals: no
navy (user decision), no brown, no mid grey.

| Surface                         | Fill      | Control on it        |
| ------------------------------- | --------- | -------------------- |
| page (`--ink`)                  | `#1c1a16` | `#24221e`, `#2e2c28` |
| card, panel, drawer (`--panel`) | `#24221e` | `#2e2c28`, `#363430` |
| card in the game (`--panel-hi`) | `#2c2a26` | `#2e2c28`, `#363430` |

In code, `--control` / `--control-hi` is the fill on the page, and
`--control-card` / `--control-card-hi` the fill on a card: any surface filled
with `--card` or `--card-hi` sets `--control: var(--control-card)` (and
`-hi`) for what is inside it. Each theme in `palettes.json` sets both pairs
from its own colours (one step lighter than the page and than its cards).

The site's lighter home bands (`#111111`, with darker card stripes on them)
have no place in the launcher, which has no such bands; its page is already
lighter than the site's.

The game's menu gets the same colours (`OPENRAC_UI_COLORS`). Its drawer and
window are coal sheets (`--panel`), its cards a flat `--panel-hi`, and the
controls on them are `--control-card`, with no edge. A row under the
pointer or the focus lifts to `--control-card-hi` with a short gold bar at its
left (no gold tint, which reads brown on dark). Key hints and the "Paused while
open" badge sit on `--control-card-hi` and `--control-card`, never black. Under
the menu, on the desktop and in Android's drawer alike, the whole game is
blurred and dimmed (`--ink-deep`, 45 % at the top to 82 % at the foot, darker
at the sides); the window has no shadow over it (user decision). The desktop's
menu bar is the launcher's desktop header over it: no bar of its own and no
extra dim. The logo's name in
`--text` and its mark in `--amber`, the tabs and the Quit and Resume buttons
filled with `--panel` (`--control-card` under the pointer), `--amber` text, no
edge. The open tab's pill is `--amber`, its words in `--bar-ink-deep`, and it
slides from tab to tab.

The game's window ground is the theme's coal with a glow of the game's colour
along its top: `--panel-hi` mixed 8 % with `--glow`, more of which looks odd
on the coal. Android's drawer and its controller settings panel have the same
light along their top, under the header too. The window's right side is a
darker plain ground, with faint stripes in its top left corner.

## Logo

`src/assets/openrac-logo.svg` is the logo. `src/components/Logo.svelte` draws
its paths as they are, in the current colour, either whole or as the mark
alone (the "O" with the lombax's head) where the name does not fit. Its box is
centred on the capitals of "RAC", so it lines up with text beside it; the
ears stick out above.

- Header: the logo's name in `--text`, its mark in `--amber`, the version beside it.
- Header bar under 560 px: the mark.
- Android's rail and the first run's welcome: the mark in `--amber`.

## Type

| Token          | Font                   | Use                                                |
| -------------- | ---------------------- | -------------------------------------------------- |
| `--font-brand` | Audiowide              | the "OpenRAC" wordmark; RAC1's titles              |
| `--font-head`  | Orbitron               | headings, section numbers; the other games' titles |
| `--font-sans`  | Inter                  | everything else                                    |
| `--font-mono`  | the system's monospace | paths, commands, job output                        |

The fonts are bundled from npm (`@fontsource/audiowide`,
`@fontsource-variable/orbitron`, `@fontsource-variable/inter`), imported in
`src/main.ts`: no font or style loads from the network.

## Pieces taken from the site

| Site                                                                                                                                                                                                                                         | Launcher                                                                                               |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| `Header.tsx`: the sticky gold bar, the logo, the tabs in one darker capsule (selected: a dark chip with gold text), the site and Discord links                                                                                               | `src/components/Header.svelte`                                                                         |
| `Progress.tsx`: a game's card, a gradient from its `from` to its `to` colour, boxes with a thick border in its `border` colour, the "Progress / 59,32 %" pair, the bar with a light sweeping across it, the "… of … code bytes matched" line | the band in `src/pages/GamePlayer.svelte`, `src/components/ProgressBar.svelte` and `.bar` in `app.css` |
| `globals.css`: section numbers that count up (`01`, `02`, …), the `rise` and `sweep` animations, reduced motion                                                                                                                              | `.sec-num`, `.sections`, `.rise`, `.bar` in `app.css`                                                  |
| Numbers: two decimals with a decimal comma (`59,32 %`), bytes grouped with narrow spaces                                                                                                                                                     | `src/lib/format.ts`                                                                                    |

The launcher's own additions keep to the same palette: pills (a dot and a
word on a tint of their state's colour, 4 px corners), buttons with 6 px
corners (the primary one a flat fill in the accent), panels as fills with no
border and 12 px corners.

## A game's colours

`src/lib/themes.ts` holds each game's `theme` from the site's
`src/lib/projects.ts`, under OpenRAC's ids:

| OpenRAC | Site         | Title font   |
| ------- | ------------ | ------------ |
| `rac1`  | `rac1`       | Audiowide    |
| `rac2`  | `gc`         | Orbitron 600 |
| `rac3`  | `uya`        | Orbitron 600 |
| `rac4`  | `deadlocked` | Orbitron 700 |
| `rac5`  | `sm`         | Orbitron 600 |
| `rac6`  | `sac`        | Orbitron 600 |

`themeStyle(id)` turns a theme into the custom properties the cards use
(`--box`, `--bd`, `--tx`, `--tx2`, `--from`, `--to`, `--title-font`,
`--title-weight`): set it on an element and everything inside it takes that
game's colours. A game without a theme gets `FALLBACK` (the launcher's own
gold); the tests fail until every game in `games/` has one.

**Game colours** (Settings, General, a switch under the themes, on by
default) puts each game's own colours on: its bar, buttons and gold
(`--bar-*`, `--amber`, `--gold`, `--brand`) and, on the OpenRAC theme only,
its coal (`--tint-*`, the launcher's ladder at the same OKLCH lightness in the
game's hue). They live in palettes.json `game_colors`; `applyAccent` sets them
with `data-game-colors` on the page, and the core hands them to the game's menu
(`ui_colors(theme, game, game_colors)`). The light under the bar stays as
without it. Off, every game wears the gold.

## The desktop's game list

Each game in the list is its disc (`SideList.svelte`, `Disc.svelte`, user
decision): its print, or the game's picture as a label. The list has no panel
of its own, it sits on the page (user decision). The open game's disc is a bit
larger and turned; the disc under the mouse or the pad's focus turns a little.
A crash is a red dot at the disc's foot. A long name breaks before the game's
own name ("Going Commando"). A narrow window shows the discs only.

## Android's Games tab

A shelf of discs (`DiscShelf.svelte`, user decision, after Hakimba's
cartouche), landscape only. The rail stays on the left, over the wave, which
shows through it soft, as through frosted glass.

- Left: the wheel. Each game is a disc with its real print, shipped in
  `public/img/discs/` (`scripts/disc_prints.py`, user decision), or the game's
  picture as a label if it fails to load. Up, down, a drag or a tap on a neighbour turns it on a spring. The
  disc's shadow follows the turn (`--shade`), so it never pops in. Behind it a
  soft wave of light in the game's colour, no specks (user decision)
  (`WaveBackdrop.svelte`, after the PSP and PS3 menus; still with reduced
  motion). The GPU draws it (WebGL) at the screen's own pixels, each sheet one smooth band with soft edges (no threads), only in the band it can reach, at most 60 frames a
  second. It follows the wheel, not its bounce. A finger held on the disc
  turns it. The discs below fade out down to the screen's edge, under the foot.
  Opening Games from another tab, the wave only fades in (it never slides with
  the page under the rail) and gives a short push, as after one turn of the
  wheel down.
- Right: year and region, the name in the game's title font, its newest save
  or progress, notes (crash, mods), then Play (A), Game page (Y) and "..."
  (`src/lib/gameMenu.svelte.ts`), and the Mods switch under them, the same pill as on the desktop's game page (X turns it).
  The buttons and the pad's glyphs are filled (`--control-card`), never see-through.
  With game colours on, the disc picked brings its game's colours. Sizes scale with the window height (`clamp()`).
- Top right: What's new chips, hidden when empty. Foot: the running job and
  the pad hints.

Play puts the disc into a console (`DiscLaunch.svelte`, our own drawing, no
maker's marks; timings in `src/lib/insertion.ts`, after cartouche). B before
the click puts it back. With reduced motion the game starts at once.

On Android a game's page has no Play, Mods switch or menu (they are beside the
disc); its band has a "‹ Games" button.

## A game's picture

Each game has three pictures in `public/img/`, under the site's names (`rac1`,
`gc`, `uya`, `deadlocked`, `size-matters`, `sac`), listed in `src/lib/media.ts`:

- `<name>-bg.webp`, 1920 × 620, no logo: under the game's band once its page is
  open, and faintly behind the page;
- `<name>-tile.webp`, 1280 × 569: a disc's label until its print is
  downloaded, with the game's name at its foot in its title font, the game's
  own name on the line under the series (user decision: no logos);
- `<name>-thumb.webp`, 192 × 132: lists without discs, shown 64 × 44.

`python3 scripts/game_art.py` makes all three from the game's SteamGridDB hero
(links in the script), with one look for every game: shadows pulled toward the
page's coal, a light from the top right in the game's accent, the foot sinking
into the bar's ink (`--bar-ink`), a soft bloom on bright parts (user decision).
The characters are not retouched. Each frame keeps the faces whole and clear of
the name.

The band shows the picture at 60 %, from its upper part
(`object-position: 50% 25%`). Behind the page it is a screen blend at 20 % with
faded edges, so it only adds light.

`python3 scripts/media.py thumbs` makes the thumbnails again from the
pictures; `python3 scripts/media.py still RECORDING GAME --at S` writes a
picture, its tile (without a logo) and its thumbnail from a frame of a
recording of your own play. `node scripts/steam_art.mjs` makes Steam's pictures
of the launcher (`src-tauri/steam/`) again after `rac1-bg.webp` or the logo
changes; `bash scripts/steam_game_art.sh` (ffmpeg) makes each game's own, for
its entry from the game's page (`src-tauri/steam/games/<game>/`), from its
picture and its name in Orbitron, after a game's picture changes.

The band shows no moving clip: one was planned (an animated WebP under the
picture), and taken out on 2026-10-20 with the old `*-gameplay.gif` files,
which nothing showed. A still picture does the band's work without the
download, and a page with a Play button in it needs nothing moving behind it.

Where each picture comes from is in
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md); a new file adds its line
there when it is added.

## Structure

- A page has one main action, in the accent's gold, in a fixed place. Stops,
  dead ends and secondary actions are never gold.
- Secondary actions go in a menu beside the main one (`Menu.svelte`), with
  anything that removes set apart at the end in `--err`.
- A panel holds rows, not more boxes: lists are rows with a `--line` rule
  between them (`SaveManagerBlock.svelte`).
- A state is a dot and a few words, or a tick; a pill only where a list of
  states has to be scanned (the developer pages, Tasks).
- What only a contributor reads (serials, percentages, file names, paths) is
  shown with developer tools on.
- A task done in a few steps in order is a numbered list (`ol.steps` in
  `app.css`: the Mods tab's level editor, models and textures): each step a
  row, its name and one line on the left, its controls on the right.
- A page with several jobs gives each a tab of its own (the Mods tab: My
  mods, Level editor, Models and textures, Audio), so every one can be seen from the
  top; the main action stays in the page's head, above the tabs.
- A setting is a row: its name and at most one short line on the left, its
  control on the right (`.setting` in `app.css`). On/off is a switch
  (`input.switch`), two or three choices a segmented control (`.segmented`),
  more a list of radios. Settings are grouped in panels by what they are
  about; what most players never change goes under Advanced, folded
  (`details.more`), which opens by itself when something in it is on. A panel
  with nothing to choose on this system is not shown, and a choice that only
  matters with another one (Android's GPU driver, with Vulkan) shows with it.
- A list of things that each have settings of their own (the gameplay mods)
  is an overview of rows (a switch, a name and a line, one badge, "Settings ›"),
  and each opens a page of its own with a back button and where you are
  (`ModPage.svelte`), not settings unfolded in the list.
- A setting applies as it changes, without Enter or a Save button, and says
  that it saved. A number with both ends is a slider (the accent up to
  the value, a mark at the default) with its value in a field beside it; a
  setting off its default has a Reset beside its control.
- No path in plain view on a page players share screenshots of (Mods): a
  folder is opened with a button, and its path waits in a folded About or
  Details (`lib/paths.ts`, `Reason.svelte`).

## Rules

- Use the tokens, not new colours. A new colour goes on `:root` with a name
  and a line in this file.
- Keep text readable on its background: the pills' and blockers' colours are
  only used on `--panel`, `--panel-hi` or a game's dark boxes.
- Respect `prefers-reduced-motion` (already global in `app.css`).
- Motion and spacing have tokens too: `--ease` and `--fast`, `--medium`,
  `--slow` for transitions; `--page-x`, `--page-top`, `--page-bottom` and
  `--page-gap` for every page's margins. A page's sections rise in turn as
  it opens (`.page > section` in `app.css`).
- What a slide keeps apart from the page (`view-transition-name`) is not
  sticky inside a scroller whose overflow can change during the slide:
  WebKitGTK stops drawing it until the slide ends, where Chromium does not.
  So a tab with a list scrolls in columns (`SideFrame.svelte`), not in `<main>`.
- Hover styles go inside `@media (hover: hover)`: on a touch screen a tapped
  control would otherwise keep them until the next tap elsewhere. Each hover
  selector also starts with `html:where(:not([data-input="pad"]))`
  (`:global(...)` in a component), so a mouse resting over one control does
  not light it beside a controller's focus on another. A press shows with
  `:active` (and `.pad-press` for a controller's A), a keyboard's or
  controller's focus with `:focus-visible` or `html[data-input="pad"]`.
- With a controller (`src/lib/pad.ts`) the focus always stays on something
  A presses: when the control with it goes away (a page changed, a dialog
  opened or closed, a row removed) it moves to the one in its place. A
  control the pad must reach that is out of Tab's order (a roving tab, radio
  or menu item) uses its role inside `tablist`, `radiogroup` or `menu`. What
  floats over the page and should be reached only past its end (the toasts)
  carries `data-pad-aside`. A container can steer the pad out of it with
  `data-pad-up`/`-down`/`-left`/`-right`: a selector to land on, or `none` to
  stay (`steer` in `pad.ts`). Going up into the bar lands on the open tab.
- `src/lib/stylesheets.test.ts` holds `app.css` and every component's
  `<style>` to three of these rules: no hex colour outside `:root`, no hover
  style outside `@media (hover: hover)` or without the controller's guard. It
  names the file and line of what breaks one.
- Android follows Android's own patterns, not the desktop's: a navigation rail
  on the left (a bar at the bottom below 600 px wide, no line beside it; the
  controller never focuses it, LB and RB change tabs) in place of the gold
  bar, Material's 24/16 px margins, a ripple under the finger
  (`src/lib/ripple.ts`), snackbar-style toasts and no text selection on
  controls. The `--nav-*` tokens say what the navigation takes of the window.
- Window sizes: the window opens at 1200 × 780, can be resized, and cannot be
  made smaller than 960 × 560 (`tauri.conf.json`). 560 is as tall as a
  1366 × 768 laptop at 125 % scaling leaves room for above the taskbar, so the
  floor does not go higher. On a screen with less room than that (a 1080p
  screen at 150 % or 200 %) the window opens at what the screen has room for,
  and that is its floor (`core/src/window.rs`). A page scrolls down, never
  sideways. A tiling
  window manager may ignore the floor, so lay a page out to still fit from
  760 px wide.

## Keeping in step with the site

| When the site changes                           | Change                                                                  |
| ----------------------------------------------- | ----------------------------------------------------------------------- |
| `src/app/globals.css` `@theme` colours or fonts | `src/app.css` `:root`, the tables above                                 |
| `src/lib/projects.ts` themes                    | `src/lib/themes.ts`                                                     |
| `src/components/Header.tsx`, `Progress.tsx`     | `Header.svelte`, `SideList.svelte`, `GamePlayer.svelte`                 |
| `src/app/icon.png`, `apple-icon.png`            | nothing: the app icons are the logo's mark (see THIRD_PARTY_NOTICES.md) |
| `DISCORD_URL`                                   | `src/lib/links.ts`                                                      |
