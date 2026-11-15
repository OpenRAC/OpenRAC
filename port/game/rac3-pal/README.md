# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

Not playable yet: its `main` is not known (`entry` is null) and `libraries.tsv` is empty. The
menu (`frontbin`) is a group of its own; the online program (`i5bootn`) is left out.

| File | |
|---|---|
| `hostgen.json` | the game and how hostgen reads it ([port/game](../README.md#what-a-game-is)) |
| `libraries.tsv` | the library calls the port answers (none yet) |
