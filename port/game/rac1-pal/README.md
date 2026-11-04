# openrac-rac1-pal: Ratchet & Clank (PAL), native

The native program of Ratchet & Clank (PAL), built from the decompilation
in [games/rac1](../../../games/rac1) by
[hostgen](../../tools/hostgen/README.md). It is one of the port's games
([port/game](../README.md)): what is particular to it is here, the rest
(the library replacements, the program) is shared ([common](../common)).

```sh
cd port
cmake --preset release && cmake --build --preset release
build/release/openrac-rac1-pal --data <install>/active/rac1/data
```

`--data` is the folder the extractor wrote from the player's disc
([tools/extractor.py](../../../tools/extractor.py), or the launcher's "Set
up from your disc"): it holds `iso_data/rac1/SCES_509.16` and `disc.iso`.
The first memory card is the folder the launcher's save manager shows
(`~/.local/share/openrac/memcard/SCES_509.16`, or under `$XDG_DATA_HOME`;
`~/Library/Application Support/OpenRAC/memcard/SCES_509.16` on macOS) unless
`--cards` says otherwise; the game's save folder and files are ordinary files
in it. `--frames N` stops after N frames; `--keep-going` logs a
function without C and carries on instead of stopping at it. `--mods DIR`
plays levels edited in Godot: DIR is what `editor/extract.py pack` wrote, a
list of disc sectors (`patches.tsv`) and the files read in their place
([editor/README.md](../../../editor/README.md#pack-your-edits)).

To build another copy of the decompilation (your own checkout, further
along than OpenRAC's), configure with `-DOPENRAC_RAC1_PAL_SOURCE=<its directory>`.

## How it runs

1. Game memory is laid out as the console's ([port/runtime](../../runtime)).
2. The executable's loadable segments are copied to their addresses from
   the player's `SCES_509.16`: that is the game's data (its code bytes come
   along as data and are never run).
3. Every function is registered by code address; a level's functions are
   looked up in the level program that the game loaded last (`ParseBin`,
   [boot.c](../rac1/host/boot.c)).
4. The game's own `main` (`func_0012DB18`, matched C) runs: the boot stage,
   then each level's loop.

**Where it is today.** The whole game plays. Functions that are still
assembly in the decompilation run as the port's own C (hand or host C, or C
written from their assembly); hostgen's report lists what is left.

## Files

| File | |
|---|---|
| [hostgen.json](hostgen.json) | the game and how hostgen reads it ([port/game](../README.md#what-a-game-is)) |
| [libraries.tsv](libraries.tsv) | every library entry point the game calls, and what the port does with it: most are bound to the shared replacements ([common/libraries.tsv](../common/libraries.tsv)) |
| [host/rac1_version.h](host/rac1_version.h) | this version's names and addresses for the host C both discs share ([port/game/rac1](../rac1/README.md)): PAL's are games/rac1's own, so nothing is renamed |
| [../rac1/host](../rac1/host) | the game's host C, one copy for both discs: ParseBin wrapped, so a level code address means the loaded level's function (its overlay hook), the native 2D and world hand-off, collision, the hardware routines, the HUD at a wide picture's edges, the port's rows on the Video options page and what the window's menus need of the game (front_end.c) |

The replacements themselves are shared by every game, by library, in
[common/lib](../common/lib): `cdvd.c` (the disc image), `mc.c` (memory
cards as folders), `graph.c` (frames, display, texture uploads, the display
list hand-off), `kernel.c` (interrupt handlers, no IOP), `pad.c`, `snd.c`,
`libc.c`, `vu0.c`. The program is [common/main.cpp](../common/main.cpp) and
[common/program](../common/program).

## What is not done

Known bugs are in [docs/port/KNOWN_ISSUES.md](../../../docs/port/KNOWN_ISSUES.md);
library calls the port does not replace yet are the `todo` rows of
[libraries.tsv](libraries.tsv).
