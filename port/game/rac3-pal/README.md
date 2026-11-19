# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

**An experiment, not playable.** It boots to the main menu, which runs and takes the pad, but
draws only its boxes over its background picture: no text, no 3D (below). Levels are not loaded.

## Running it

The functions with no C are translated from the disc image: the player's (`--disc`, or
`OPENRAC_RAC3_PAL_RETAIL_IMAGE`), else `games/rac3/decomp/baserom/SCES_524.56.iso`. The disc's
executable is the loader; retail.py reads it as the game it carries packed (the same sections as
the decompilation's `boot_elf.elf`, which the build does not need).

```sh
cmake --preset release && cmake --build --preset release --target openrac-rac3-pal
# a data folder with iso_data/rac3/SCES_524.56 (the disc's boot executable) and iso_data/rac3/disc.iso
SDL_AUDIODRIVER=dummy "port/build/release/apps/openrac-rac3-pal/Ratchet & Clank 3 (PAL).app/Contents/MacOS/Ratchet & Clank 3 (PAL)" \
    --data <data> --window --skip-movies
```

`OPENRAC_RAC3_CHAINS=N` prints the VIF codes of the first N display lists and the front end's
state words (mode, boot state, boot screen, the pad's held and pressed buttons) each frame.

## How it starts

The disc boots `SCES_524.56`, a loader at `0x00800000` whose data holds the game WAD-compressed
(file offset `0x7080`) as a list of records (the level program format). The port loads that file
as the console's loader would, and its entry (`host/entry.c`, `func_0013ADA8`) unpacks the
records, then does crt0's user-mode steps and calls `main` (`0x0013B330`). `main` runs the loaded
program's entry, `0x00381250`: boot_elf's own front end (frontbin's twin), which is the boot
screens and the main menu. When it returns, `0x0013AFF8` copies the next program in and gives
its entry (`host/program.c`).

## The next program

The front end ends when a menu's choice sets its exit word (`0x001D5B74`) and the level to go to
(`0x001D9D04`). On its way out (`0x003A7CB8`, `0x003BD870`) it reads frontbin, the front end's
own program in the MISC wad, when that level is below zero, else the level's program, to a buffer
at the top of memory (`*0x001D4B50`) that `0x0013AFF8` copies over the boot program's (from `.lit`,
`0x001D5680`, up: hostgen.json's `relocate`). The port wraps `0x0013AFF8` and says which program
is loaded through the overlay hook: frontbin is hostgen's overlay 100 (`groups`), a level its own
number. retail.py reads both from the disc (`asm` `retail` `programs`), so their functions with no
C are translated from their own instructions.

- **frontbin** loads and runs: its start-up, its widgets, its intro movie (the native player through
  its own copy of the entry, `0x003AF890`) and then its menu loop, sending a display list each frame.
- **A level** does not run yet. On the way to one (New Game, Load Game, or level 1 by
  `OPENRAC_RAC3_NEXT`) the boot program's loading screen now unpacks its pack (the WAD
  decompressor on the shared routine, `host/wad.c`) and loads its sound bank (`0x0013BC30`, the
  shared `snd_BankLoadFromEE_CB`), then waits in its own hand-written draw kernel (`0x003CA858`,
  `0x003CA9C0`: VU0 and a scratchpad DMA chain the port does not run), which a native drawing of
  the loading screen is to replace. The level's code itself is not translated yet:
  hostgen.json's `sources` has no `src/levels` and no `places` catalogue (rac3-decomp's
  `config/levels/functions.tsv`), as Going Commando's has.

Until the menus can be read, `OPENRAC_RAC3_NEXT=FRAME:LEVEL` writes what a menu's choice writes, at
the boot program's FRAME-th display list (`-1`: frontbin).

## What the port answers

| | |
|---|---|
| `libraries.tsv` | 204 library entry points, found by their instructions against rac2-ntsc's and rac1-pal's tables, by shape where the SDK version differs, by RPC function number (libmc), and by the game's own calls (libpad) |
| `host/entry.c` | the loader's unpacking, crt0, InitSystemCallTableAddress (no kernel to scan) |
| `host/dma.c` | sceDmaSend leaving CHCR as the DMAC does (the VIF1 handler retires the frame's wait mask from it), KickVif0Chain |
| `host/sound.c` | the 989snd reply wait and flush: answered at once (no IOP, no sound yet) |
| `host/pad.c` | libpad 4.0, Deadlocked's host pad at this disc's addresses |
| `host/movie.c` | the movie player entry, the boot program's and frontbin's: the shared native player |
| `host/program.c` | which program is loaded (the overlay hook), `OPENRAC_RAC3_NEXT` |
| `headers/vu0.h` | the vector-library helpers of the decompilation's `include/vu0.h` (asm Clang refuses) in C, read in its place (hostgen.json `headers`) |
| `host/trig.c`, `host/vu0.c` | VU0 cos/sin (the host's for now) and sceVu0UnitMatrix |
| `host/spr_dma.c` | the scratchpad DMA channels the kernels start |
| `host/chain_census.c` | `OPENRAC_RAC3_CHAINS` |
| `profile.cpp` | draws the display list the game sends (as rac4-ntsc) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record |
| `IDENTIFIED.tsv` | every function and global worked out on the way, for the backfill |

`hostgen.json` also lists `from_assembly` (C that forwards registers or has VU0 inline assembly,
translated from its instructions), `candidate_folders_left_out` (the NTSC-U drafts of boot_elf and
frontbin give way to the programs' own instructions, which the build reads from the disc: some
drafts were far from PAL's code and crashed frontbin's start-up and the level loading screen), `executable_extents` (names the C only declares inside another
function start nothing) and `unpassed_registers` (a K&R call's missing arguments are the
caller's registers).

## The menu's background

The main menu's background is a still picture, not a video: the front end's data (786 sectors at
sector 673402, read to `0x00C73000` while it boots) is a table of WAD-compressed entries, and the
picture (512x416, alpha 0x7F, in a 512x448 buffer at `0x00EA5010` after a 16-byte header of its
size) is unpacked from it once at boot and never changes afterwards (its hash is the same over a
minute of the menu). The IPU is used only by the movie player (`0x003AFC48`, one caller); libmpeg
has no other caller. Each frame the list uploads the picture straight into the buffer the frame is
drawn in, block `0x1E00` (FBP `0xF0`, set by the packet at `0x00142120`), in fourteen 512x32
transfers (`func_003A9BE8`: pixels, the frame's address, width, height; it returns the picture's
TEX0), then darkens it and draws the menu over it. The port wraps that routine (`host/picture.c`):
the picture goes to the window as a texture of its own and the display list gets a full-screen 2D
record reading it in place of the transfers, as the 2D layer's quads do; nothing is sent to the GS
for it.

## What is missing

- **Menu text**: the menus' text and icons go to VU1 (UNPACK, then MSCNT), not as GIF packets; the
  direct renderer draws only the GIF part (the boxes). A native renderer for that VU1 program, or
  the port's own text drawing hooked at the game's text calls, is next.
- **Sound**: the 989snd commands are answered with empty replies; the shared sound replacements
  are not bound.
- **A level**: see [The next program](#the-next-program).
- The memory card works through the shared `sceMc*`; saving is untested.
