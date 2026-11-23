# openrac-rac1-pal: Ratchet & Clank, PAL v2.00

The port's main program: Ratchet & Clank's PAL disc v2.00 (`SCES_509.16`), built by
[hostgen](../../tools/hostgen/README.md) from [games/rac1](../../../games/rac1). The other rac1 discs
are carried from it. Shared parts: [rac1](../rac1/README.md) and [common](../common).

```sh
python3 tools/build_port.py rac1-pal
port/build/release/openrac-rac1-pal --data <data> --window
```

`--data` is the folder the extractor or the launcher's "Set up from your disc" wrote (it holds
`iso_data/rac1/SCES_509.16`, the disc's files and `sectors.tsv`, the map the game reads its sectors
by; the image itself is never opened). Other options: `--cards DIR` (memory cards, default
the launcher's save folder), `--levels DIR`, `--frames N`, `--keep-going` (log a function without C
and carry on), `--mods DIR` (levels edited in Godot,
[editor](../../../editor/README.md#pack-your-edits)). `-DOPENRAC_RAC1_PAL_SOURCE=<dir>` builds
another checkout of the decompilation.

## How it runs

1. Game memory is laid out as the console's ([runtime](../../runtime)).
2. The executable's segments are copied from the player's `SCES_509.16`: that is the game's data
   (its code bytes are never run).
3. Every function is registered by address; a level's functions are looked up in the level
   program the game loaded last (`ParseBin`, [boot.c](../rac1/host/boot.c)).
4. The game's own `main` (`func_0012DB18`) runs.

The whole game plays. Functions still in assembly run as the port's C (hand C, host C, or C
written from the assembly); hostgen's report lists what is left.

## Files

| File | |
|---|---|
| `hostgen.json` | the game and how hostgen reads it ([port/game](../README.md#what-a-game-is)) |
| `libraries.tsv` | every library call and what the port does with it; `todo` rows are not replaced yet |
| `split_places.tsv` | variants of functions the catalogue folds into one name, each with its own places |
| `host/rac1_version.h` | this disc's table for the shared host C (PAL's names are games/rac1's own) |
| `host/*.c` | C only the PAL discs need (crt0, debug print, TLB handler, ship glass, water) |
| `hand/` | C for functions the decompilation has only as assembly |
| `no_c.tsv` | the functions with no C ([port/game](../README.md#the-functions-with-no-c)) |
| `fcheck.tsv`, `fcheck_artefacts.tsv` | fcheck's verdicts on the hand C ([BACKFILL.md](../../../docs/port/BACKFILL.md)) |

Known bugs: [KNOWN_ISSUES.md](../../../docs/port/KNOWN_ISSUES.md).
