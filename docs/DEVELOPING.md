# Developing OpenRAC

How to work on OpenRAC from a checkout. To play, use the launcher from the
[releases page](https://github.com/OpenRAC/OpenRAC/releases/latest) instead
([README.md](../README.md#play)).

## Your checkout and your discs

```sh
git clone https://github.com/OpenRAC/OpenRAC.git && cd OpenRAC
```

Put images of your own discs in `baserom/` ([baserom/README.md](../baserom/README.md)
lists the supported ones), then:

```sh
python3 tools/openrac.py discs     # identify and check your disc images
python3 tools/openrac.py setup     # put each game's files where its build expects them
```

## The launcher

```sh
cd launcher && corepack enable && yarn install && yarn tauri dev
```

The launcher finds the checkout it runs from. With **developer tools** on in
its Settings, it builds the PC port from your tree. More in
[launcher/README.md](../launcher/README.md).

## The PC port

Ratchet & Clank (PAL) is the first game on PC. The port builds the game's
decompiled code into one program that runs the game's own `main`, drawn by
OpenRAC's renderer, with the game's sound, music and speech.

```sh
cd port && cmake --preset release && cmake --build --preset release && ctest --preset release
```

- [port/README.md](../port/README.md): the code and how it fits together.
- [docs/port](port/README.md): the design and the order of work.
- [docs/port/WORKFLOW.md](port/WORKFLOW.md): from a player's report to a fix.
- [port/docs/CODING_CONVENTIONS.md](../port/docs/CODING_CONVENTIONS.md): how
  the port's code is written.

## CI build caches

The checks run on pushes to `develop` and `master` as well as pull requests, so successful
default-branch builds populate caches that later pull requests can restore. Superseded PR and
`develop` runs are cancelled; release checks on `master` finish.

Each native checks configuration has separate compiler, CMake preparation and hostgen caches.
CMake preparation keeps dependency sources and configure/compiler metadata, keyed by tools,
installed packages, runner image, workspace, CPU features and build configuration. A missing or
incompatible identity falls back to normal preparation. Hostgen's cache keeps generated text
under a separate Clang/Python, platform, workspace and compiler-package identity, without native
CPU probes, CMake or Ninja. It can therefore survive a runner CPU change that invalidates CMake.
Objects remain in ccache; Ninja files and test results are regenerated. Cache hits never skip
build or test steps. Pull requests restore native compiler/preparation and Rust caches but do
not save private copies: those copies cannot warm other branches and can evict shared entries
from the repository's cache allowance. Successful branch builds refresh the shared caches.
Compiler identity uses executable contents rather than installation timestamps. The hostgen
cache key covers every game's sources and its shared Python helpers. Prepared source copies and
Ninja's generation stamp are excluded: hostgen always validates its own input fingerprint and
required files before reusing output, including when an older source snapshot is restored.
Launcher checks and builds also cache Yarn archives while retaining
`yarn install --immutable` and its install/link steps.

## The level editor

The launcher's **Mods** tab does all of this for you. By hand, for
Ratchet & Clank (PAL):

```sh
# Turn the 19 levels on your disc into Godot projects (about 20 seconds).
python3 editor/extract.py godot baserom/SCES_509.16.iso assets/levels/rac1/pal --split

# Open one in the Godot 4 editor (level_00 is Veldin).
godot --path assets/levels/rac1/pal/level_00 -e

# Pack your edits into game data. Your disc is not changed.
python3 editor/extract.py pack baserom/SCES_509.16.iso assets/levels/rac1/pal assets/packed/rac1/pal
```

The port plays the packed levels with `--mods assets/packed/rac1/pal`. More in
[editor/README.md](../editor/README.md); to change how things behave, use the
mod SDK ([docs/port/MODS.md](port/MODS.md)).

## The decompilations

Each game version in `games/` is its own project, built from its own
directory with a compiler you supply ([toolchains/README.md](../toolchains/README.md)).
Start with [games/README.md](../games/README.md), the game's own README and
[decomp/](decomp/README.md), which compares the projects and holds the rules
for matching work.
A few tools work across them:

```sh
python3 tools/sources.py status|sync GAME/VER   # follow the original repositories
python3 tools/xmap.py scan|report|ports FROM TO # the function map across the games
python3 tools/port.py FROM TO [--check]         # functions FROM matched that TO has unchanged
python3 tools/permuter run|base|queue PROFILE FUNC   # decomp-permuter for any game
python3 tools/openrac.py progress [--fetch]     # consolidate progress, refresh the README tables
```

## Before you send a change

Read [CONTRIBUTING.md](../CONTRIBUTING.md). OpenRAC's own code is being
restructured package by package: [restructure/PROGRESS.md](restructure/PROGRESS.md)
says what is free to pick up.
