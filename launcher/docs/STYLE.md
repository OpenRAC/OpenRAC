# The launcher's look

The launcher looks like [openrac.dev](https://openrac.dev): the same dark ink
background, orange header bar, wrench, type and per-game cards. Everything
here comes from [OpenRAC/openrac-site](https://github.com/OpenRAC/openrac-site)
at commit `2964d72`; when the site changes, change the files in the last
section to match.

## Colours

`src/app.css`, on `:root`, with the site's names (its `@theme` in
`src/app/globals.css`):

| Token                         | Value                             | Use                                                |
| ----------------------------- | --------------------------------- | -------------------------------------------------- |
| `--ink`                       | `#181820`                         | the page                                           |
| `--ink-deep`                  | `#12121a`                         | inputs, job output                                 |
| `--panel`                     | `#1d1d28`                         | panels                                             |
| `--panel-hi`                  | `#22222f`                         | cards inside panels, buttons                       |
| `--brand`                     | `#d58a00`                         | section numbers, the selected job                  |
| `--amber`                     | `#ffb443`                         | links, focus rings, blockers ("needs …"), warnings |
| `--gold`                      | `#ffd08a`                         | headings                                           |
| `--lav`, `--soft`, `--dim`    | `#c8d2ff`, `#aab6dc`, `#8f9ac4`   | secondary text, from most to least prominent       |
| `--line`, `--line-strong`     | `rgb(156 161 212 / .22)`, `/ .35` | borders                                            |
| `--text`, `--body`            | `#e8f0ff`, `#c3cdea`              | text (the site's body and prose colours)           |
| `--bar-top`, `--bar-bottom`   | `#e5a013`, `#cd8300`              | the header bar's gradient, primary buttons         |
| `--bar-ink`, `--bar-ink-deep` | `#2a1800`, `#1b1206`              | text on the bar; the selected tab                  |

The site has no status colours; the launcher adds `--ok` (`#8fd694`), `--warn`
(amber) and `--err` (`#ff8a7a`), used only in pills and messages. The theme is
dark only, as the site is.

## Type

| Token          | Font                   | Use                                                |
| -------------- | ---------------------- | -------------------------------------------------- |
| `--font-brand` | Audiowide              | the "OpenRAC" wordmark; RAC1's titles              |
| `--font-head`  | Orbitron               | headings, section numbers; the other games' titles |
| `--font-sans`  | Inter                  | everything else                                    |
| `--font-mono`  | the system's monospace | paths, commands, job output                        |

The fonts are bundled from npm (`@fontsource/audiowide`,
`@fontsource-variable/orbitron`, `@fontsource-variable/inter`), imported in
`src/main.ts`: the app never loads anything from the network.

## Pieces taken from the site

| Site                                                                                                                                                                                                                                         | Launcher                                                                                                                                       |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| `Header.tsx`: the sticky orange bar, the wrench that tilts on hover, "Open**RAC**" in Audiowide, round tabs (selected: dark brown with amber text), the white Discord button                                                                 | `src/components/Header.svelte`                                                                                                                 |
| `Progress.tsx`: a game's card, a gradient from its `from` to its `to` colour, boxes with a thick border in its `border` colour, the "Progress / 59,32 %" pair, the bar with a light sweeping across it, the "… of … code bytes matched" line | `src/components/GameCard.svelte` (smaller), the banner in `src/pages/Game.svelte`, `src/components/ProgressBar.svelte` and `.bar` in `app.css` |
| `globals.css`: section numbers that count up (`01`, `02`, …), the `rise` and `sweep` animations, reduced motion                                                                                                                              | `.sec-num`, `.sections`, `.rise`, `.bar` in `app.css`                                                                                          |
| Numbers: two decimals with a decimal comma (`59,32 %`), bytes grouped with narrow spaces                                                                                                                                                     | `src/lib/format.ts`                                                                                                                            |

The launcher's own additions keep to the same palette: pills (a dot and a
word, coloured by state), rounded buttons (primary ones in the bar's orange),
panels with a thin `--line` border and 16 px corners, dashed borders for what
is planned but not there yet.

## A game's colours

`src/lib/themes.ts` holds each game's `theme` from the site's
`src/lib/projects.ts`, under OpenRAC's ids:

| OpenRAC | Site         | Title font   |
| ------- | ------------ | ------------ |
| `rac1`  | `rac1`       | Audiowide    |
| `rac2`  | `gc`         | Orbitron 600 |
| `rac3`  | `uya`        | Orbitron 600 |
| `rac4`  | `deadlocked` | Orbitron 700 |

`themeStyle(id)` turns a theme into the custom properties the cards use
(`--box`, `--bd`, `--tx`, `--tx2`, `--from`, `--to`, `--title-font`,
`--title-weight`): set it on an element and everything inside it takes that
game's colours. A game without a theme gets `FALLBACK` (the launcher's own
orange); the tests fail until every game in `games/` has one.

The site's cards can show a backdrop image (`/img/<game>-bg.webp`), which the
site does not ship in its repository; the launcher leaves them out and shows
the gradient, as the site does when the image is missing.

## Rules

- Use the tokens, not new colours. A new colour goes on `:root` with a name
  and a line in this file.
- Keep text readable on its background: the pills' and blockers' colours are
  only used on `--panel`, `--panel-hi` or a game's dark boxes.
- Respect `prefers-reduced-motion` (already global in `app.css`).
- Window sizes: the window opens at 1200 × 780 and cannot be made smaller
  than 980 × 640 (`tauri.conf.json`); lay pages out for that range.

## Keeping in step with the site

| When the site changes                                      | Change                                                                                                                            |
| ---------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| `src/app/globals.css` `@theme` colours or fonts            | `src/app.css` `:root`, the tables above                                                                                           |
| `src/lib/projects.ts` themes                               | `src/lib/themes.ts`                                                                                                               |
| `src/components/Header.tsx`, `Progress.tsx`                | `Header.svelte`, `GameCard.svelte`, `Game.svelte`                                                                                 |
| `public/wrench.webp`, `src/app/icon.png`, `apple-icon.png` | `public/`, and `npx tauri icon <1024 px png> -o src-tauri/icons` (then delete the `android/`, `ios/` and `Square*` files it adds) |
| `DISCORD_URL`                                              | `src/lib/links.ts`                                                                                                                |
