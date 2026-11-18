# openrac-rac1-ntsc: Ratchet & Clank, NTSC-U

Ratchet & Clank's NTSC-U disc (`SCUS_971.99`), built by [hostgen](../../tools/hostgen/README.md)
from the NTSC decomp ([games/rac1/ntsc-decomp](../../../games/rac1/ntsc-decomp)). Its tables were
carried from [rac1-pal](../rac1-pal/README.md) by [versionmap](../../tools/versionmap/README.md):
what PAL's port knows by address, moved to this disc's addresses. Shared parts:
[rac1](../rac1/README.md).

```sh
python3 tools/build_port.py rac1-ntsc
port/build/release/openrac-rac1-ntsc --data <data> --cards <a folder> --window --levels <its levels>
```

The options are rac1-pal's. Its levels are its own disc's (the launcher's "Get the levels ready"
writes `assets/port/rac1/ntsc`; without `--levels` the program reads `<data>/port/ntsc`). Without a
card folder the game waits on its no-card warning for a button (`OPENRAC_PRESS` gives one,
[port/README.md](../../README.md)).

## Files

| File | |
|---|---|
| `hostgen.json` | rac1-pal's lists moved to this disc (`relocate` from 0x15EF00, level hooks); `defines` gives C for two inline-assembly macros (`qzero`, `MOBY_CALL_UPDATE`: without it no moby updates); `skip` leaves out libgcc and `malloc_extend_top.c`, which Clang cannot read |
| `libraries.tsv` | rac1-pal's rows at this disc's addresses; each note keeps PAL's symbol |
| `split_places.tsv` | rac1-pal's, only where this catalogue folds the same places into one name |
| `host/rac1_version.h` | this disc's table for the shared host C, checked against the map by [hostnames.py](../../tools/versionmap/README.md#the-shared-host-cs-tables) |
| `host/catalogue.c` | the levels' copy of WadDecompress, which this catalogue lists apart |
| `host/ship_glass.c`, `host/water.c` | C only the NTSC disc needs |
| `hand/` | C for functions the NTSC decomp has only as assembly, from rac1-pal's `hand/` |
| `hand/functional/` | PAL's matched or functional C for functions the NTSC decomp has only as assembly (each file's header says where it came from) |
| `hand/overrides.tsv` | the port's C in place of matched C where the port must differ |
| `hand/fcheck.tsv` | fcheck's verdict per `hand.tsv` row (`unchecked` until fcheck runs the NTSC-U disc) |
| `hand/allow.tsv` | what the hand C does differently from the US code on purpose (below) |
| `drafts.tsv` | the NTSC decomp's own pending C read in place of a stub |
| `no_c.tsv` | the functions with no C ([port/game](../README.md#the-functions-with-no-c)) |

Known trouble spots of the US game's code: [RAC1_NTSC_HINTS.md](../../../docs/port/RAC1_NTSC_HINTS.md).

## Checking the hand C against the US code

Carried C can keep what the carry got wrong: a neighbour's address, PAL's name for a routine the
US program has elsewhere, a level 0 routine called from a level with its own copy.
[xcheck.py](../../tools/versionmap/xcheck.py) compares every placed `hand.tsv` row's C (Clang's IR,
prepared as hostgen does) with the US function's instructions: calls by the function bound at each
place, globals by address with their offsets, each level copy against the home copy. What differs
on purpose is in `hand/allow.tsv` with its reason; a row there that matches nothing fails too.
ctest runs it as `rac1-ntsc_hand_xcheck` (skipped without the retail program):

```sh
python3 port/tools/versionmap/xcheck.py check port/game/rac1-ntsc --gen port/build/release/games/rac1-ntsc/gen
python3 port/tools/versionmap/xcheck.py report port/game/rac1-ntsc --gen ... -v FUN_L07_00314150   # one row
```

In a worktree, `--asm-source` (or `OPENRAC_RAC1_NTSC_ASM_SOURCE`) names the checkout whose
`games/rac1/ntsc-decomp` holds `config/us/SCUS_971.99` and `config/us/overlays`. Where code
indexes an array, an element within 0x1000 bytes passes.
