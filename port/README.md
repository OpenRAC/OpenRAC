# The native port

OpenRAC's native PC port: the decompiled games built for a PC and drawn by
a renderer of OpenRAC's own, the way [OpenGOAL](https://github.com/open-goal/jak-project)
ported Jak and Daxter. **No emulation**: nothing here runs the retail
program or models the console's processors, GS, vector units or DMA
controller. The design, the studies behind it and the order of work are in
[docs/port](../docs/port/README.md).

```sh
cd port
cmake --preset release            # fetches SDL3 when the system has none
cmake --build --preset release
ctest --preset release
python3 -m unittest discover -s tools/hostgen    # the translator's tests
```

You need CMake 3.24+, Ninja, Clang (it builds the port, and hostgen uses it
to read the decompilation) and Python 3. `--preset headless` builds without
a window (no SDL3): the runtime, the games and their tests.

## What is here

| Directory | |
|---|---|
| [runtime/](runtime) | game memory, laid out as the console's (a 4 GB reservation whose offsets are its addresses), the contract translated code is written against ([guest.h](runtime/include/openrac/guest.h)), the executable's data loader, the crash handler |
| [tools/hostgen/](tools/hostgen/README.md) | writes a decompilation's C again for game memory: pointers as 32-bit game addresses, calls matched to their definitions as the EE passed arguments, calls through code addresses. It reports the frontier: what the program reaches that has no C |
| [game/rac1/](game/rac1/README.md) | Ratchet & Clank (PAL): hostgen's configuration, the table of the libraries it calls, their replacements (disc, memory card, graphics hand-off, kernel, pads, sound) and `openrac-rac1` |
| [platform/](platform) | the window, input as the console's pad, audio output, timing (SDL3) |
| [renderer/](renderer) | the renderer: per-subsystem renderers in OpenGOAL's manner, the direct renderer for the 2D path, texture conversion (OpenGL 4.1) |
| [viewer/](viewer) | `openrac-viewer`: a level, extracted from the player's disc by the editor, drawn natively with a free camera |
| [common/](common) | the log |
| [tests/](tests) | the runtime's tests |

## How a game is built

```
games/rac1/pal (the decompilation, read only)
   │  hostgen: Clang reads each file as the console's C,
   │  hostgen writes it again for game memory
   ▼
build/.../games/rac1/gen/*.c  +  game/rac1/host/*.c (library replacements)
   │  compiled as C, linked with runtime/
   ▼
openrac-rac1 --data <install>/active/rac1/data
   loads the player's executable's data to its addresses, registers every
   function by code address, runs the game's main()
```

The program stops at the first function that has no C yet and names it.
For OpenRAC's decompilation that is the boot stage, still assembly; a copy
of the decompilation that is further along builds the same way
(`-DOPENRAC_RAC1_SOURCE=<dir>`), and hostgen's report says what it reaches
next.

## Rules

OpenRAC's rules hold here ([AGENTS.md](../AGENTS.md),
[SOURCING.md](../docs/policy/SOURCING.md)): nothing from a disc is ever
committed (no data, code bytes, microcode or media), and the library
replacements are written from what the game's own code shows and what is
publicly known of each API, never from Sony's code or documentation.
Generated C and build output stay in build trees (`port/build/` is ignored).
The C and C++ layout is [.clang-format](.clang-format); comments say why.

GPL-3.0-or-later, as the rest of OpenRAC's top level.
