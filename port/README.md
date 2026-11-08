# The native port

The decompiled games built as native PC programs, drawn by OpenRAC's own
renderer, the way [OpenGOAL](https://github.com/open-goal/jak-project) ported
Jak and Daxter. **No emulation**: nothing here runs the retail program or
models the console's processors, GS, vector units or DMA. Design, studies and
the order of work: [docs/port](../docs/port/README.md).

## Build and test

```sh
cd port
cmake --preset release            # fetches SDL3 when the system has none
cmake --build --preset release
ctest --preset release
python3 -m unittest discover -s tools/hostgen    # the translator's tests
```

You need CMake 3.24+, Ninja, Clang (it builds the port, and hostgen uses it to
read the decompilation) and Python 3.

- `--preset headless` builds without a window (no SDL3): the runtime, the
  games and their tests.
- macOS builds with the Command Line Tools (`xcode-select --install`) alone; the
  Xcode app is not needed. The programs run on macOS 12 and later
  (`CMAKE_OSX_DEPLOYMENT_TARGET`). `--preset release-x86_64` builds for Intel
  Macs and `--preset release-universal` for Apple silicon and Intel in one
  binary; both build SDL3 in, as Homebrew's is for one architecture only.
  With the Xcode app, `cmake --preset xcode` writes
  `build/xcode/openrac_port.xcodeproj`, to build, run and debug in Xcode (its
  GPU frame capture shows the Metal renderer's passes).
- On Linux the build uses mold or lld and ccache when installed
  (`-DOPENRAC_PORT_FAST_BUILD=OFF` turns that off).
- `python3 tools/build_port.py [GAME] [--source DIR]`, run from the top of
  OpenRAC, builds one game without the steps above. The launcher's "Build the
  game" runs it. It configures `build/release` once, then builds only what
  changed. On Windows it sets up Visual Studio's x64 environment and finds
  Clang and Ninja. `--source` builds from your own copy of the decompilation
  and is remembered.

## Enhanced rendering

Probe GI, modern shadows and GTAO are experimental. Every build compiles them
in (`OPENRAC_ENHANCED_RENDERING`, ON by default). They stay off until the
`enhanced_rendering` setting or `OPENRAC_ENHANCED=1` turns them on; off, the
game draws exactly as a build without them.

- Players turn it on in the launcher: Mods, **Enhanced graphics**. That plays
  with Vulkan and bakes the probes on their computer from their own extracted
  levels, into `<data>/probes/<version>` (`--probes`).
- It needs Vulkan and a bake that matches the level. OpenGL, or a level
  without a bake, keeps the original lighting.
- Only the offline baker needs Embree: `python3 tools/build_port.py
  --probe-baker` builds a pinned Embree into `openrac-enhanced-baker` (or
  `-DOPENRAC_PROBE_BAKER=ON` with Embree 4 installed). The game never links it.

Its sources, shaders, tools and tests are in [`enhanced/`](enhanced/README.md).

## What is here

| Directory | |
|---|---|
| [runtime/](runtime/README.md) | Game memory laid out as the console's (a 4 GB reservation whose offsets are its addresses), the contract translated code is written against ([guest.h](runtime/include/openrac/guest.h)), the executable loader, the crash handler |
| [tools/hostgen/](tools/hostgen/README.md) | Rewrites a decompilation's C for game memory, and reports what the program reaches that has no C |
| [game/](game/README.md) | One directory per game version, plus [game/common/](game/common): the library replacements (disc, memory card, graphics, kernel, pads, sound) and the program, `openrac-<id>` |
| [platform/](platform/README.md) | Window, pads, audio output, timing (SDL3) |
| [renderer/](renderer/README.md) | The OpenGL 4.1 renderer, in OpenGOAL's style; [graph/](renderer/graph/README.md) plans passes and barriers, [ui/](renderer/ui/README.md) draws over the picture, [vulkan/](renderer/vulkan/README.md) runs it on Vulkan 1.3 (`OPENRAC_GL_API=vulkan`), [metal/](renderer/metal/README.md) on Metal (`OPENRAC_GL_API=metal`) |
| [scene/](scene/README.md) | A level and its live mobys, ready to draw |
| [materials/](materials/README.md) | CPU-only material catalogs, override resolution and shared validation |
| [viewer/](viewer/README.md) | `openrac-viewer`: one extracted level with a free camera |
| [enhanced/](enhanced/README.md) | Enhanced rendering (above) |
| [assets/](assets) | Readers for the disc and its level, picture and sound data |
| [audio/](audio/README.md), [media/](media/README.md) | The sound engine; the movie decoder |
| [extract/](extract/README.md) | `openrac-extractor`: sets a game up from the player's disc |
| [android/](android/README.md) | The Android build |
| [common/](common/README.md) | What every module may use: log, crash report, JSON reader, the PS2's float arithmetic, small helpers |
| [tests/](tests) | The port's tests |
| [docs/](docs/CODING_CONVENTIONS.md) | The coding conventions |
| [tools/](tools) | [conventions](tools/conventions/README.md) and [layering](tools/layering/README.md) checks (CI), [regress](tools/regress) runs, [versionmap](tools/versionmap/README.md), [perf](tools/perf) |

## How a game is built

```
games/rac1/decomp (the decompilation, read only)
   │  hostgen: Clang reads each file as the console's C,
   │  hostgen writes it again for game memory
   ▼
build/.../games/rac1-pal/gen/*.c  +  game/common/lib/*.c (library replacements, shared)
                                 +  game/rac1-pal/host/*.c (the game's own, if any;
                                    rac1's versions share game/rac1/host)
   │  compiled as C, linked with runtime/
   ▼
openrac-rac1-pal --data <install>/active/rac1/data
   loads the executable's data to its addresses, registers every
   function by code address, runs the game's main()
```

With `--stop-on-missing` the program stops at the first function that has no
C and names it. `-DOPENRAC_RAC1_PAL_SOURCE=<dir>` builds from another copy of
the decompilation. [game/README.md](game/README.md) says how far each version
gets.

On macOS each windowed program is also an app bundle named after the game, so
the Dock and crash reports show "Ratchet & Clank (PAL)":
`build/release/apps/openrac-rac1-pal/Ratchet & Clank (PAL).app`
([cmake/MacApp.cmake](cmake/MacApp.cmake)). `build/release/openrac-rac1-pal`
links to it for a shell. The bundle declares itself a game, so Game Mode turns
on in full screen.

## Settings and controls

- **Settings**: `<data home>/openrac/settings/<id>.ini` (`--settings FILE`
  names another): window mode, monitor, resolution, aspect up to 32:9, HUD
  position, render scale, anti-aliasing, texture filter, vsync, frame rate,
  frame-rate counter, volume. Every key is in
  [docs/port/SETTINGS.md](../docs/port/SETTINGS.md).
- **Controls**: `<data home>/openrac/controls/` (`--controls DIR`). The menu's
  Controls tab changes them; [docs/port/CONTROLS.md](../docs/port/CONTROLS.md)
  says what they hold.
- In Ratchet & Clank the settings are also rows of the title menu's Options,
  Video page ([options_menu.cpp](game/common/options_menu.cpp)). A change is
  applied and saved at once. The game's own 50/60 Hz row is gone: the game
  ticks at its own rate and the window shows it at the display's.

## Mods

Mods are shared libraries the program loads at start through a versioned C
interface ([sdk/include/openrac/openrac_mod.h](../sdk/include/openrac/openrac_mod.h)).
They take the game's hook points (`hook` rows of a version's `libraries.tsv`,
written by hostgen into `hooks.c`) and run each tick and each pad report.
Without a mod, the hook points run the game's own C.

- The launcher installs mods in `<data>/plugins/` and writes the list the
  program reads, `plugins.json` (`--plugins FILE` names another).
- The program writes what it loaded, and why it skipped any, to `status.json`
  beside it, and a `mods` line in the log header.
- `--no-mods` (the launcher's Purist mode) loads none.
- Script mods are `.luau` files run in Luau ([cmake/Luau.cmake](cmake/Luau.cmake)),
  one sandboxed state each, with a time budget per call
  ([mod_scripts.cpp](game/common/mod_scripts.cpp)). They reach the same hooks
  through [game/rac1/script_binding.c](game/rac1/script_binding.c), which
  `python3 ../sdk/tools/gen_binding.py` writes from
  [sdk/bindings/rac1.json](../sdk/bindings/rac1.json). A new `hook` row needs
  its line there; ctest's `mod_binding` fails when they disagree.

The loader is [mods.cpp](game/common/mods.cpp). Design, manifest and SDK:
[docs/port/MODS.md](../docs/port/MODS.md). Mods for Ratchet & Clank:
[OpenRAC/openrac-mods](https://github.com/OpenRAC/openrac-mods).

## Debugging

- **F3** shows the debug overlay: frame times, renderer, memory, mobys with an
  inspector, game state, shaders, with pause, single step and slow motion.
- `--debug-dump FILE` writes each frame's measurements as JSON lines.
- `OPENRAC_TRACE_INPUT=1` logs the pad report sent to the game every 25 frames
  in which a port is read, scripted input included. With `OPENRAC_DEBUG=1`,
  RAC1 PAL also logs the processed pad state and sticks every 100 frames.
- `OPENRAC_DIRECT=1` with `--level N` starts a new game straight in level N,
  without menu input. `--skip-movies` skips every movie (each skip is logged).
  This tests fresh-game level loading only, not saves or menus, so also test
  the normal New Game path. Example (PowerShell):

  ```powershell
  $env:OPENRAC_DIRECT = '1'
  $env:OPENRAC_PRESS = '100:4000:5' # acknowledge the initial no-card warning
  .\build\release\openrac-rac1-pal.exe --data <extracted-data> --window --no-card --level 0 --skip-movies --stop-on-missing
  ```

Every option and `OPENRAC_*` variable: [docs/port/DEBUGGING.md](../docs/port/DEBUGGING.md).

## Rules

OpenRAC's rules hold here ([CONTRIBUTING.md](../CONTRIBUTING.md),
[SOURCING.md](../docs/policy/SOURCING.md)):

- Nothing from a disc is committed: no data, code bytes, microcode or media.
- Library replacements are written from what the game's code shows and what
  is public about each API, never from Sony's code or documentation.
- Generated C and build output stay in build trees (`port/build/` is ignored).
- Code follows [docs/CODING_CONVENTIONS.md](docs/CODING_CONVENTIONS.md):
  layout ([.clang-format](.clang-format)), comments, naming, ownership,
  errors, the C boundary, layering. CI checks it with
  [tools/conventions](tools/conventions/README.md),
  [tools/layering](tools/layering/README.md) and [.clang-tidy](.clang-tidy).
- The port builds on Linux, macOS and Windows (Clang against MSVC's
  libraries). CI builds all of it on Linux and rac1/pal's program on Windows.
- Host code uses only the standard library and the runtime. A POSIX-only
  header or function (`execinfo.h`, `unistd.h`, `fseeko`, ...) goes behind
  `_WIN32` with its Windows form beside it, as in [runtime/](runtime). For a
  stack trace call `openrac_host_caller`
  ([guest.h](runtime/include/openrac/guest.h)), not `backtrace`.

GPL-3.0-or-later, as the rest of OpenRAC's top level.
