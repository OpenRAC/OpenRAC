# openrac-rac2-ntsc: Going Commando, USA v1.01

Going Commando's USA disc v1.01 (`SCUS_972.68`), built by [hostgen](../../tools/hostgen/README.md)
from [games/rac2](../../../games/rac2/decomp). The other two discs are carried from it. Shared parts:
[rac2](../rac2/README.md); game facts: [RAC2_NOTES.md](../../../docs/port/RAC2_NOTES.md).

```sh
python3 tools/build_port.py rac2-ntsc --disc <your disc image>
python3 tools/extractor.py <your disc image> --game rac2 --proj-path <data> -e
port/build/release/openrac-rac2-ntsc --data <data> --levels <its levels> --window
```

Functions still in assembly are translated from the disc's executable and level programs
(`--disc`, or `OPENRAC_RAC2_NTSC_RETAIL_IMAGE`; without it, `games/rac2/decomp/baserom/SCUS_972.68`).
With neither they are stubs and the game stops at once. `OPENRAC_RAC2_NTSC_SOURCE` builds another
checkout of the decompilation. The launcher builds and plays it from the player's disc.

A scripted run (Cross 0x4000, Start 0x8, Down 0x40, Up 0x10, Triangle 0x1000):

```sh
OPENRAC_HIDDEN=1 OPENRAC_PRESS=300:4000:10,500:8:10,700:4000:10,900:8:10 \
  port/build/release/openrac-rac2-ntsc --data <data> --window --frames 1500 --no-card --skip-movies --stop-on-missing
```

## Files

| File | |
|---|---|
| `hostgen.json` | the game and how hostgen reads it: `asm` (C from the disc's instructions), `skip` (the other releases' C, libgcc), `from_assembly` (wrappers whose C drops registers), `level_hooks`, `unit_aliases` |
| `libraries.tsv` | the library calls the port answers: symbol-table names, system calls by number, SDK functions matched to rac1's and rac4's by their instructions |
| `host/rac2_version.h`, `host/rac2_addresses.h` | this disc's table for the shared host C (v1.01's: nothing renamed) |
| `hand/` | the port's C in place of matched C where the port must differ (`overrides.tsv`) |
| `tables/derived_names.json` | names rac2-decomp's own build derives, kept here so hostgen runs without that build (below) |
| `tables/split_places.tsv` | variants of functions the catalogue folds into one name |
| `tables/call_copies.tsv`, `data_copies.tsv`, `level_twins.tsv` | calls and data that go through a level's copy of a function (copycalls.py) |
| `tables/jump_tables.json` | switch tables hostgen reads from the code (jumptables.py) |
| `checks/fcheck*.tsv` | fcheck's verdicts on rac2-decomp's functional C ([BACKFILL.md](../../../docs/port/BACKFILL.md)) |

After syncing `games/rac2/decomp`, build its boot program and write the derived names again
(hostgen warns, and `test_derived` fails, when the C uses a name the copy lacks):

```sh
python3 port/tools/hostgen/derived.py --game port/game/rac2-ntsc/hostgen.json --source <built rac2-decomp>
```
