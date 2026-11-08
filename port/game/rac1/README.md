# Ratchet & Clank: the port's host C, one copy for every disc

The port's own host C for Ratchet & Clank, compiled into each version's
program (`openrac-rac1-pal`, `openrac-rac1-ntsc`, `openrac-rac1-pal1`, `openrac-rac1-jp`): the overlay hook and the
direct start, the native 2D and world hand-off, the collision kernels, the
hardware routines, the variants of folded functions. A fix written here
holds for every disc, so the player's disc decides nothing but its own
addresses.

```
port/game/rac1/host/*.c          the shared C, with games/rac1's names
port/game/rac1-pal/host/         rac1_version.h: PAL's names and addresses (its own: nothing renamed)
port/game/rac1-ntsc/host/        rac1_version.h: NTSC-U's names and addresses; C only NTSC-U needs
port/game/rac1-pal1/host/        PAL v1.00's: rac1_names.tsv (names) and rac1_version.h (addresses)
port/game/rac1-jp/host/          NTSC-J's, the same way
```

PAL v1.00 and NTSC-J are built from games/rac1's own C (rac1-decomp's other releases), so their
names are games/rac1's at other addresses: a macro per name would chain, and their tables rename
the shared C's function names in one pass (`host/rac1_names.tsv`, release_host.py, cmake/Games.cmake)
and keep only the addresses in `rac1_version.h`. Both are written by
[release_rac1.py](../../tools/hostgen/release_rac1.py) ([rac1-pal1/README.md](../rac1-pal1/README.md)).

A version takes the shared C with `"host_shared": ["../rac1/host"]` in its
`hostgen.json` ([cmake/Games.cmake](../../cmake/Games.cmake)). Each file
includes [rac1.h](host/rac1.h), which reads the version's translation
(`game_protos.h`) and then its table.

## Writing shared C

- **Functions by games/rac1's names**: `func_001F8B6C`, `func_L00_002A8A20`,
  and `func_0022C9A8__game` for the decompilation's C under a `wrap`. A
  version whose decompilation names them otherwise renames them in its
  table (`#define func_001F8B6C game_FUN_001f89a4`).
- **Addresses in `RAC1_ADDR()`**, written `0x%08Xu`, one literal each:
  `OPENRAC_DATA(RAC1_ADDR(0x0018A3D0u) + i * 4)`. A level program's address
  is `RAC1_LADDR(NN, ...)`: `OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))`.
  A version without an entry for an address does not compile.
- **What a version leaves out**: `#if !RAC1_OMIT(func_...)` around a
  function, and `RAC1_OMIT_func_... 1` in that version's table with the
  reason (its decompilation has C of its own for it, its catalogue places
  something else there, its game never calls it).
- **Where the decompilations declare a function otherwise**: the type is
  `RAC1_TYPE_<name>_<what>` in each table.
- **Names in strings**: `RAC1_NAME(func_...)` is the version's name.

[port/tools/versionmap/hostnames.py](../../tools/versionmap/README.md)
checks a table against the shared C and, given the map, against the two
programs' code: every name and address the shared C uses has an entry,
every entry is used, no address is left outside `RAC1_ADDR`, and each
entry agrees with the map; `write` adds what a new piece of shared C uses
from the map. ctest runs the check without the map (`rac1-*_host_table`).

## The game's profile

What the host program's C++ reads of the running game (the camera, the fog, the live mobys and the
display list shown) is rac1's, and the host asks it through rac1's profile
([host::GameProfile](../common/profile.h)), never by a game id of its own:

| File | |
|---|---|
| `game_state.h`, `game_state.cpp` | the address tables, one per disc for the boot program and one per level program, and the reads (`read_game_state`, `read_live_moby`, `moby_beyond_draw_distance`, `shown_chain`), in `openrac::game::rac1` |
| `profile.cpp` | the discs' profiles over those tables, with each executable's addresses of the hero's shadow directions and record, and `host::game_profile()`: `rac1-ntsc` is NTSC-U, `rac1-pal1` PAL v1.00, `rac1-jp` NTSC-J, any other id PAL |
| `profile_rac1-pal1.h`, `profile_rac1-jp.h` | PAL v1.00's and NTSC-J's tables, carried from PAL's by release_rac1.py |

Both versions' programs link them as `openrac_game_rac1_profile`
([cmake/Window.cmake](../../cmake/Window.cmake)), as do the viewer and its tests, which call the
reads directly on a saved memory picture. A table of a new version goes beside the others in
`game_state.cpp`, with its profile in `profile.cpp`; nothing outside this folder changes.

The shared front-end callback supplies the active hero body before its equipment. Original
shadow volumes and enhanced shadow maps follow this body, including playable Clank, on both discs.

## What stays per version

A version's own folder keeps what truly differs: its `hostgen.json`,
`libraries.tsv`, `split_places.tsv`, `hand/` (C for functions its
decompilation has only as assembly), `drafts.tsv`, its table, and host C
for a routine only it has. That C is written in the version's own names and
includes `game_protos.h`, not `rac1.h`.
