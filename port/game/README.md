# The games

Each folder here is one program the port builds (`openrac-<id>`), or what one game's programs
share. The build makes a program for every folder whose decompilation is present.

| Folder | Disc | Decompilation | Entry (`main`) |
|---|---|---|---|
| [rac1-pal](rac1-pal/README.md) | Ratchet & Clank, PAL v2.00 (SCES-50916) | `games/rac1/decomp` | `func_0012DB18` |
| [rac1-pal1](rac1-pal1/README.md) | Ratchet & Clank, PAL v1.00 (SCES-50916) | `games/rac1/decomp`, release view | as rac1-pal |
| [rac1-jp](rac1-jp/README.md) | Ratchet & Clank, NTSC-J (SCPS-15037) | `games/rac1/decomp`, release view | as rac1-pal |
| [rac1-ntsc](rac1-ntsc/README.md) | Ratchet & Clank, NTSC-U (SCUS-97199) | `games/rac1/ntsc-decomp` | `FUN_0012d9d8` |
| [rac2-ntsc](rac2-ntsc/README.md) | Going Commando, NTSC-U v1.01 (SCUS-97268) | `games/rac2/decomp` | `func_00131AE8` |
| [rac2-ntsc2](rac2-ntsc2/README.md) | Going Commando, USA v2.00 (SCUS-97268) | `games/rac2/decomp`, release view | `func_00131AE8` |
| [rac2-pal](rac2-pal/README.md) | Going Commando, PAL (SCES-51607) | `games/rac2/decomp`, release view | `func_00131B48` |
| [rac3-ntsc](rac3-ntsc/README.md) | Up Your Arsenal, NTSC-U (SCUS-97353) | `games/rac3/decomp`, release view of rac3-pal | as rac3-pal |
| [rac3-pal](rac3-pal/README.md) | Up Your Arsenal, PAL (SCES-52456); its host C and profile are both discs' | `games/rac3/decomp` | `func_0013ADA8` (the port's loader and crt0) |
| [rac4-ntsc](rac4-ntsc/README.md) | Deadlocked, NTSC-U | `games/rac4/decomp` | not known yet |

A *release view* builds a disc from another disc's C: the same functions at the release's
addresses, plus the release's own C where its code differs. Its tables are generated, never
edited by hand. [rac1](rac1/README.md) and [rac2](rac2/README.md) hold what a game's discs share:
host C, the profile, game fixes (`differences.json`) and parity reasons (`parity.tsv`).

Everything else is shared by all games: game memory ([runtime](../runtime)), the translator
([hostgen](../tools/hostgen/README.md)), library replacements and the program itself
([common](common)).

## The common folder

The build takes every `.cpp` file in these folders ([Games.cmake](../cmake/Games.cmake)), so a new
file needs no CMake line.

| Folder | |
|---|---|
| `common/program/` | the program, with or without a window: `main`, options, vsync, the host side of `game_host.h` |
| `common/frontend/` | window builds only: frame pacing, interpolation, world submission, input |
| `common/debug/` | window builds only: the debug overlay and menu (`overlay.h` holds their shared state) |
| `common/lib/` | the library replacements (`cdvd.c`, `mc.c`, `graph.c`, `kernel.c`, `pad.c`, `snd.c`, `libc.c`, `vu0.c`) and routines several games share |

## What a game is

| File | |
|---|---|
| `hostgen.json` | what the game is and how to read its decompilation (keys below) |
| `libraries.tsv` | every library entry point the game calls that the port answers: symbol, library, name, kind (`host`, `game`, `wrap`, `todo`), note, optional C type. A `host` row binds the address to the shared replacement ([common/libraries.tsv](common/libraries.tsv)) |
| `host/` | the program's own C, if any: `wrap` functions, the overlay hook. Discs of one game share theirs (`"host_shared"`), each with a table of its names and addresses |
| `hand/` | C for functions the decompilation has only as assembly |
| `no_c.tsv` | the functions hostgen has no C for, and whether the port needs C for them (below) |
| `icon.png`, `icon-macos.png` | the window's icon; macOS draws the Dock icon as given, so it has its own |

The host's C++ reads the running game (camera, live mobys, display list, hero) only through the
game's profile, `host::GameProfile` ([common/profile.h](common/profile.h)): a game's `profile.cpp`,
in its shared folder or its own. A game with none links [profile_none.cpp](common/profile_none.cpp).
Shared code never compares a game id.

`hostgen.json` keys:

| Key | |
|---|---|
| `id`, `title`, `game`, `serial` | `rac1-pal`; `Ratchet & Clank (PAL)`; the extractor's game; the executable on the disc |
| `source` | the decompilation, from the top of OpenRAC; `-DOPENRAC_<ID>_SOURCE=<dir>` builds another copy |
| `frame_rate` | 50 or 60 |
| `entry` | the game's `main`, or null while unknown (the program then says so and stops) |
| `sources`, `skip`, `includes`, `defines` | the C to translate and how to read it |
| `names` | how the decompilation names code and data by address (regular expressions with `addr` and `overlay` groups) |
| `symbols` | splat symbol files with named globals and functions |
| `derived` | names the decompilation's own build derives, read from that build or the port's copy ([derived.py](../tools/hostgen/derived.py)) |
| `places` | where each level function sits in each level |
| `groups` | parts of the tree that are separate programs over the same addresses (rac3's menu) |
| `overlay_hook` | true if `host/` provides `openrac_game_loaded_overlay` |
| `hand`, `drafts`, `overrides` | the port's C for assembly-only functions, the decompilation's own drafts, and the port's C in place of matched C ([hostgen](../tools/hostgen/README.md#a-games-configuration)) |
| `asm` | C written from the assembly where there is no C, read from the disc where the files are not there ([hostgen](../tools/hostgen/README.md#functions-with-no-c-c-from-their-assembly)) |
| `roots` | where hostgen's frontier report starts |
| `data_allow`, `data_refs` | the data guard's allow table and the disc's data places ([below](#the-functions-with-no-c)) |

## The functions with no C

[no_c.py](../tools/hostgen/no_c.py) writes each rac1 program's `no_c.tsv`. It reads every call,
jump, branch, address and data word that refers to a function in the executable and all level
programs, and sorts it: class `a` needs no C (linker remnants, padding, pieces of other functions,
unused), `b` is replaced by the host on purpose, `c` needs C. Every class `c` function has C now,
translated from the disc. Rows whose evidence starts with "by hand:" are kept when it runs again.

Calls are not the only way in. The game also reaches functions through addresses in its data (a
moby's update, a menu page's callbacks) and the port through `wrap` rows. hostgen's data guard
fails the build when any of these has no C, with no disc needed:

- `checks/data_refs.tsv`: each place the disc's data holds a function's address, written from the
  disc by [data_refs.py](../tools/hostgen/data_refs.py) (addresses only). Write it again when a
  version is added or its catalogue moves.
- functions a global's initializer in the decomp C names, and the `__game` side of `wrap` rows.
- `checks/data_allow.tsv`: the ones allowed to stay without C, each with its reason (a name, or a
  place where the port has no function).

## Moving a game on

1. Put its `main` in `entry`, and its title and level loops in `roots`.
2. Build it and read `build/<preset>/games/<id>/gen/report.md`: what is translated, what became a
   stub, and what the program reaches that has no C.
3. Bind the library functions it calls in `libraries.tsv`. A library the shared code lacks gets a
   replacement in `common/lib/` and a row in `common/libraries.tsv`.
4. If its levels load their own programs, give it an overlay hook ([rac1/host/boot.c](rac1/host/boot.c)).

A new disc of a game with a decompilation of another disc is a release view: see
[rac1](rac1/README.md#carried-releases) and [rac2](rac2/README.md).
