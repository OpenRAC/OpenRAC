# Connecting the games to the launcher

The launcher is scaffolding: the app, its look, the game catalogue, the job
runner and the first actions work, and every game has its actions written
down. This document is for whoever wires the rest up: what "connected" means,
the format of `launcher/actions.json`, and the work left, in packages that can
be taken one at a time.

Read [ARCHITECTURE.md](ARCHITECTURE.md) first for how the parts fit.

## What works today

| Part                                                                                          | State                                                                                                                                                                              |
| --------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| The app: window, header, Games, game pages, Tasks, Settings, first run                        | done; `npm run tauri dev`                                                                                                                                                          |
| Browser preview with mock data                                                                | done; `npm run dev`                                                                                                                                                                |
| Catalogue from `games/*/game.json` and `progress/summary.json`                                | done, tested against this checkout                                                                                                                                                 |
| Disc and input status (names, sizes, links)                                                   | done; checksums are `openrac.py discs`'s, see [Disc verification](#disc-verification)                                                                                              |
| Detecting the checkout, Python, Godot, Docker                                                 | done                                                                                                                                                                               |
| Jobs: queue, live output, cancel (whole process tree)                                         | done, tested on Linux                                                                                                                                                              |
| Repository actions: Identify discs, Verify every checksum, Place inputs, Test OpenRAC's tools | **connected**: Identify discs and the tool tests run from the desktop app on Linux; the other two run with the same command lines from a shell. None yet with a disc in `baserom/` |
| `report-check` for rac1/pal and rac4                                                          | **connected**: the same command lines run from a shell on Linux (they need no disc)                                                                                                |
| Every other per-game action                                                                   | **unverified** or **planned**: the work below                                                                                                                                      |

## Action states

Every action in `actions.json` has a `state`. Change it only when its
definition is met:

| State        | Meaning                                                                            | The launcher                                                  | Done when                                                                                                               |
| ------------ | ---------------------------------------------------------------------------------- | ------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `planned`    | Wanted; no command yet                                                             | shows it greyed out, with its `todo` under "For contributors" | it has a `program` and `args`                                                                                           |
| `unverified` | The command is written from the game's docs but has not been run from the launcher | runs it, marked "Unverified"                                  | someone ran it from `npm run tauri dev` on each listed platform, with real inputs, and it did what its description says |
| `connected`  | Run from the launcher and known to work on its `platforms`                         | runs it, marked "Ready"                                       | (keep it working)                                                                                                       |

When you connect an action: set `state`, narrow `platforms` to what you ran it
on, delete its `todo` (or leave one for the platforms still unchecked), and
say in the commit body what you ran, where, and what it printed at the end.

## actions.json

```jsonc
{
  "format": 1, // this launcher reads 1
  "repository": [/* actions for the checkout as a whole */],
  "versions": {
    "rac1/pal": [/* actions for one version; the key is games/<game>/<version> */],
  },
}
```

One action:

| Field         |                                                                       | Meaning                                                                                                                                                |
| ------------- | --------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `id`          | required                                                              | unique within its list; what the UI sends back                                                                                                         |
| `label`       | required                                                              | the button's title, short ("Build")                                                                                                                    |
| `description` |                                                                       | one or two sentences for players: what it does, how long, what it needs                                                                                |
| `kind`        | required                                                              | `setup`, `build`, `check`, `play` or `edit`: the game page's section                                                                                   |
| `state`       | default `planned`                                                     | see above                                                                                                                                              |
| `program`     | for runnable states                                                   | `{python}`, `{godot}` or `{docker}` (the paths in Settings), a command on PATH (`bash`, `make`), or a path with placeholders (`{dir}/venv/bin/python`) |
| `args`        |                                                                       | the arguments, one per string; placeholders allowed; never a shell line                                                                                |
| `cwd`         | default: the version's folder, or the checkout for repository actions | relative to the checkout (`"."` for the checkout itself)                                                                                               |
| `platforms`   | default: all                                                          | `linux`, `macos`, `windows`; the others' actions are tucked away                                                                                       |
| `requires`    |                                                                       | what must be in place, each shown as a reason when missing: `disc`, `inputs`, `toolchains`, `python`, `godot`, `docker`, `artifact`                    |
| `artifact`    |                                                                       | a file the action makes (a build) or needs (play the build), relative to the checkout; `{artifact}` in `args`                                          |
| `detached`    | default false                                                         | start it and let it run (an emulator, an editor) instead of a job in Tasks                                                                             |
| `docs`        |                                                                       | the document for this step, relative to the checkout (`#anchor` allowed); the Docs button opens it                                                     |
| `todo`        | required while `planned`                                              | for contributors: what is left to do or check                                                                                                          |

Placeholders (anything else is rejected by the tests):

| Placeholder           | Value                                                |
| --------------------- | ---------------------------------------------------- |
| `{root}`              | the checkout, absolute                               |
| `{dir}`               | the version's folder, absolute (`…/games/rac1/pal`)  |
| `{python}`, `{godot}` | the paths in Settings (refused while unset)          |
| `{docker}`            | the path in Settings, or `docker` from PATH          |
| `{disc}`              | the disc image found for the version (absolute)      |
| `{boot}`              | the boot executable `openrac.py setup` placed for it |
| `{artifact}`          | the action's `artifact`, absolute                    |
| `{serial}`, `{key}`   | `SCES_509.16`, `rac1/pal`                            |

`cargo test -p openrac-launcher-core` checks the file against the checkout:
known versions, unique ids, known placeholders (and no version placeholders in
repository actions), a program for every runnable action, a `todo` for every
planned one, and that every `docs` path exists. Run it after every change.

To list what is left:

```sh
python3 -c "import json; f = json.load(open('launcher/actions.json'))
for k, l in [('repository', f['repository'])] + list(f['versions'].items()):
    for a in l:
        if a.get('state', 'planned') != 'connected': print(f\"{k:10} {a['id']:18} {a.get('state', 'planned'):10} {a.get('todo', '')}\")"
```

## Work packages

Each package can be done on its own. Take one, say so where the group
coordinates, and keep each commit to one action or one package
(`feat(launcher): connect rac1/ntsc's build`).

### 1. Disc verification

`core/src/status.rs` finds a disc by name or size and says "found"; it never
hashes a 4 GB image. The checksums are `tools/openrac.py discs`'s job, which the
launcher runs as a job and whose output the user reads.

To show "verified" on the cards: give `openrac.py discs` a machine-readable
mode (for example `--json`, printing `{serial: {"path", "ok", "checked":
"sha1"|"full", "differ": [...]}}`), run it from `status.rs` or as a job whose
result the launcher stores, and cache each result by path, size and
modification time so it is not recomputed. `openrac.py` is a top-level tool
with tests in `tools/test_openrac.py`: the flag and its test go there, in a
`feat(tools)` commit of its own.

### 2. Playing

The games are played in OpenRAC's own runtime
([runtime/README.md](../../runtime/README.md)): the launcher starts no
emulator and needs no BIOS. Every version has a `play-runtime` action,
`runtime/tools/run.py play {serial} {disc}`, which builds the runtime (CMake,
a C++ compiler, SDL3) and runs the disc in it, in a window. It is not
detached, so what the build and the runtime print shows in Tasks, and closing
the game's window ends the job.

| Version   | State      | How far it gets                                                    |
| --------- | ---------- | ------------------------------------------------------------------ |
| rac1/pal  | unverified | menus, memory card, sound, the first level                         |
| rac1/ntsc | unverified | the main menu and the first level                                  |
| rac2/ntsc | unverified | the main menu and the start of the first level; movies are skipped |
| rac3/ntsc | planned    | the runtime has no table for its program yet                       |
| rac4/ntsc | planned    | the runtime has no table for its program yet                       |

"Unverified" here means run from a shell on macOS, not yet from the desktop
app. The repository action `runtime-tests` builds the runtime and runs its
tests.

The port itself has started: `play-port` (rac1/pal so far, unverified in the
same sense) is `runtime/tools/run.py play-port {key} {serial} {disc}`. It
builds the version's decompiled C for the host
([runtime/port/README.md](../../runtime/port/README.md); it needs LLVM with
the wasm32 target and WABT) and plays the disc with those functions running
as native code in place of the game's own, in every level, the interpreter
keeping the rest. What comes next is a renderer of OpenRAC's own on the GPU
([runtime/docs/DESIGN.md](../../runtime/docs/DESIGN.md)).

### 3. Each game's set-up, build and checks

The per-version actions carry their own `todo`s; in short:

- **rac1/pal**: `setup-asm` and `build` run in the Docker image on Linux and
  macOS, natively under Git Bash on Windows. Confirm the artifact and how a
  mismatch shows (exit code). Mind [BUILD_FIDELITY.md](../../games/rac1/pal/docs/BUILD_FIDELITY.md):
  the launcher runs the build; it never changes how it builds.
- **rac1/ntsc**: `setup.sh` downloads and builds its toolchains (long; Linux,
  WSL, or Docker on macOS); `make elf`, `verify-baseline.sh`, `make iso`.
  Find `make elf`'s output for an artifact.
- **rac2**: needs settings the launcher lacks (package 4); the run id that
  `setup.py` creates feeds `build.py`.
- **rac3**: needs Wrench, wibo and a venv (package 4); then
  `tools/setup_asm.py`, `tools/build.py`, `tools/pr_check.py`.
- **rac4**: `setup_asm.sh`, the Docker build, `audit_matches.py` with the
  project's venv.

Each game's own docs are the authority on its commands; the action's `docs`
field points at them. If a game's tool needs a change to be driven (a flag, a
machine-readable result), make it in that game's directory under its rules,
in a commit of its own, and verify the game as its docs say.

### 4. More settings

Some games need tools and folders the launcher does not know yet: rac2's
runtime folder (outside the tree; `build/rac2-runtime` works) and Wrench's
`wrenchbuild`; rac3's wibo and Wrench; per-game virtual environments. A design
that keeps `actions.json` the single place: a `tools` map in `Config`
(`{"wrench": "/path/to/wrenchbuild", "wibo": ...}`) with placeholders
`{tool:wrench}`, declared in a top-level `"tools"` list in `actions.json` (name,
label, hint, how to detect), so Settings lists them without UI changes. Add
the placeholder form to `PLACEHOLDERS` handling and its tests.

A "Download helpers" action (Wrench, wibo) must pin versions and checksums,
like rac1/ntsc's `setup.sh` does.

### 5. Windows

rac1/pal and rac4's scripts are bash; rac1/ntsc and rac2 use WSL for their
compilers. Decide per game between Git Bash (`bash` from Git for Windows) and
WSL (`wsl.exe --cd <path> -- <command>`, which needs paths converted to
`/mnt/c/...`: a `{wsl:dir}`-style placeholder). Test the job cancel on
Windows (`taskkill /T`) against a WSL process.

### 6. Updates

- **The launcher**: Tauri's updater plugin, as the T3SDK launcher does
  (signed `latest.json` on GitHub releases), plus a release workflow that
  builds the installers. Needs a signing key held by the maintainers.
- **The checkout**: the planned `update` repository action: `git pull` when the
  tree is clean, refusing otherwise. Players who do not use git need a
  different answer (open decision below).

### 7. The level editor

rac1/pal has the whole path written down, in the order a user takes it:

| Action           | State      | What it does                                                                      |
| ---------------- | ---------- | --------------------------------------------------------------------------------- |
| `editor-extract` | unverified | the disc's 19 levels into a Godot project in `assets/godot`                       |
| `editor-import`  | unverified | Godot reads the meshes and textures once, headless, so the first open is quick    |
| `editor-open`    | unverified | the Godot editor on that project                                                  |
| `editor-pack`    | planned    | edited scenes back into level data, under `build/`; the packer does not exist yet |
| `editor-play`    | planned    | the runtime with a packed level in place of the disc's; needs the packer          |
| `editor-tests`   | unverified | the unit tests of `editor/`                                                       |

The unverified ones ran from a shell on macOS with a real disc and Godot
4.7.2 (extract into a new folder, the refusal of an existing one, the
headless import, the tests); none has been run from the desktop app yet. The
extractor needs Python 3.10 or newer, which is why Settings refuses an older
one. It never overwrites `assets/godot`, so once that exists the page should
offer Open, not Extract (an `artifact` on extract and a "skip when present"
rule, or a status field). The other versions carry a planned `editor-extract`
that says what the editor lacks for them
([editor/README.md](../../editor/README.md)).

### 8. Release and CI

CI runs the launcher's checks (the `launcher` job in
`.github/workflows/checks.yml`). Building installers in CI, and where they are
published, waits on the license (below) and package 6.

## Open decisions

These are not the launcher's to settle alone ([AGENTS.md](../../AGENTS.md), rule 11):

- **License.** The launcher's code is under "everything else" in
  [LICENSE.md](../../LICENSE.md): not chosen yet
  ([open question 1](../../docs/policy/OPEN_QUESTIONS.md#1-licensing)).
  Releases wait on it.
- **How players get OpenRAC.** Today the launcher needs a git checkout. A
  player-facing release could bundle `tools/` and the game folders instead,
  which changes what "update" means and where builds write.
- **Network.** The launcher fetches nothing today. Live progress (the site's
  `/progress.json`) and update checks would be its first network calls.
- **Which games and versions to put first.** The launcher shows every version
  in `games/`; the four games are rac1 (PAL and NTSC-U), rac2, rac3 and rac4.

## Keeping things in step

| When this changes                       | Change this too                                                                                                      |
| --------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| A game.json gains a version             | an entry in `actions.json` (the tests insist) and, for a new game, a theme in `src/lib/themes.ts` (the tests insist) |
| A game's build commands or output paths | its actions' `args`, `artifact` and `docs`                                                                           |
| A Rust command or type                  | `src/lib/api.ts` and `src/lib/mock.ts`                                                                               |
| The website's colours or cards          | `src/app.css`, `src/lib/themes.ts` ([STYLE.md](STYLE.md))                                                            |
