# OpenRAC Launcher

A desktop app for playing the Ratchet & Clank games through OpenRAC, for all
four games. A player opens it, adds the image of their own disc, presses
Play (the game runs natively in OpenRAC's own runtime, with no emulator), and
can open the levels in Godot to edit them. That is all it shows by default.

With **developer tools** switched on in Settings it is also a contributor's
console: place what each build needs, set up the toolchains, build, check,
and see each decompilation's progress. Either way it runs the same commands a
contributor types, from the user's own OpenRAC checkout, and shows their
output live.

It is a [Tauri 2](https://tauri.app/) app: a Rust side (`core/` and
`src-tauri/`) that does every file and process operation, and a Svelte 5 UI
(`src/`) styled after [openrac.dev](https://openrac.dev).

> **Status: scaffolding.** The app builds and runs, reads every game from
> `games/*/game.json`, and runs the repository-wide actions (identify discs,
> place inputs). Most per-game actions are written from each game's docs but
> not yet run from the launcher, and some are only planned. What is left, and
> how to connect it, is in [docs/INTEGRATION.md](docs/INTEGRATION.md).

| Games                                                                                          | A game                                                                                               |
| ---------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| ![The games, with each version's progress and what is in place](docs/screenshots/library.webp) | ![Ratchet & Clank (PAL): disc, set up, build, check, play, level editor](docs/screenshots/game.webp) |

## Read first

| You want to                                            | Read                                         |
| ------------------------------------------------------ | -------------------------------------------- |
| Connect a game's build, checks or play to the launcher | [docs/INTEGRATION.md](docs/INTEGRATION.md)   |
| Understand how the parts fit, add a command or a page  | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| Change the look, add a game's colours                  | [docs/STYLE.md](docs/STYLE.md)               |

OpenRAC's own rules apply here too ([AGENTS.md](../AGENTS.md),
[CONTRIBUTING.md](../CONTRIBUTING.md), [SOURCING.md](../docs/policy/SOURCING.md)):
the launcher never downloads, uploads or bundles a game, and writes into the
checkout only through the tools it runs.

## Develop

You need Node 20+ and Rust (stable). For the desktop app on Linux, Tauri's
system libraries too (Debian and Ubuntu:
`libwebkit2gtk-4.1-dev libgtk-3-dev librsvg2-dev libsoup-3.0-dev`; see
[Tauri's prerequisites](https://tauri.app/start/prerequisites/) for the others).

```sh
cd launcher
npm ci
npm run dev           # the UI in a browser at http://localhost:1430, with mock data (no Rust needed)
npm run tauri dev     # the desktop app, with the real Rust side
npm run tauri build   # an installer for this platform, in target/release/bundle/
```

The browser preview (`npm run dev`) reads the real `games/*/game.json`,
`progress/summary.json` and `actions.json`, and makes up the rest (which discs
are found, what a job prints): see `src/lib/mock.ts`. Add `?setup` to the
address for the first-run screen, `?version=rac1/pal` for a game page,
`?page=tasks` for Tasks.

## Check

```sh
npm run verify                              # svelte-check, ESLint, Prettier, Vitest
cargo test -p openrac-launcher-core         # the Rust logic; no WebKit needed
cargo clippy --workspace -- -D warnings     # needs Tauri's system libraries
cargo fmt --check
```

`cargo test -p openrac-launcher-core` also checks `actions.json` against the
checkout: every version key exists in a game.json, no duplicate ids, only known
placeholders, every `docs` path exists. CI runs all of these
(`.github/workflows/checks.yml`, the `launcher` job).

## Layout

```
launcher/
├── actions.json        what the launcher can run, per version: the file to edit to connect a game
├── core/               Rust, no window: games, actions, status, settings, detection, jobs (+ tests)
├── src-tauri/          the desktop app: Tauri commands over core/, window and bundle config, icons
├── src/                the UI: Svelte 5 + TypeScript
│   ├── lib/            api.ts (commands and types), mock.ts (browser preview), app state, queue, themes
│   ├── components/     header, game card, action card, progress bar, path field, toasts, icons
│   └── pages/          Library, Game, Tasks, Settings (also the first-run screen)
├── public/             the wrench logo and favicon, from openrac-site
└── docs/               ARCHITECTURE, INTEGRATION, STYLE
```

## Where it keeps things

- Its settings: `launcher.json` in the per-user config folder
  (`~/.config/dev.openrac.launcher/` on Linux,
  `~/Library/Application Support/dev.openrac.launcher/` on macOS,
  `%APPDATA%\dev.openrac.launcher\` on Windows). Settings shows the path.
- Nothing else. Discs stay in `baserom/`, build output where each game's build
  puts it, the level editor's project in `assets/godot`, all ignored by git.

## License and credits

The launcher's own code falls under "everything else" in
[LICENSE.md](../LICENSE.md): its license is not chosen yet
([open question 1](../docs/policy/OPEN_QUESTIONS.md#1-licensing)). It takes its
look, logo and favicon from the OpenRAC website (MIT) and bundles three fonts
(SIL Open Font License); see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
Its design follows the T3SDK launcher for Thief: Deadly Shadows (MIT), and two
small helpers are adapted from it (credited in the same file).
