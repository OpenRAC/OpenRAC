# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

**Runs as far as its menus.** It boots to the main menu, which runs and takes the pad, but
draws only its boxes over its background picture: no text, no 3D (below). Levels are not loaded,
so a new game or a save does not start yet. The launcher builds and starts it from a player's PAL
disc (below), with a warning that says so.

## Running it

The functions with no C are translated from the game's own instructions: from the player's disc
image when the build is given one (`tools/build_port.py rac3-pal --disc IMAGE`, or
`-DOPENRAC_RAC3_PAL_RETAIL_IMAGE=IMAGE`), else from `boot_elf.elf`, the unpacked game the
decompilation's setup puts in `games/rac3/decomp/baserom/`. The disc's executable is the loader,
which carries the game WAD-compressed at file offset `0x7080`; hostgen.json's `asm` `loader` says
where, and which of its records are code (`core.text`, `.text`) and which to leave out (the VU
microcode), and `retail.py` (`loader_parts`) unpacks it as `host/entry.c` does at run time. Both
give the same C (compared 2026-11-19: every generated file equal but for the source's name in its
comments).

```sh
cmake --preset release && cmake --build --preset release --target openrac-rac3-pal
# a data folder with iso_data/rac3/SCES_524.56 (the disc's boot executable) and iso_data/rac3/disc.iso
SDL_AUDIODRIVER=dummy "port/build/release/apps/openrac-rac3-pal/Ratchet & Clank 3 (PAL).app/Contents/MacOS/Ratchet & Clank 3 (PAL)" \
    --data <data> --window --skip-movies
```

From the launcher (launcher/actions.json, `rac3/pal`), as for Going Commando: *Set up from your
disc* runs `tools/extractor.py`, which checks the disc against games/rac3/game.json and writes
`iso_data/rac3/` (`SCES_524.56`, `disc.iso`, `buildinfo.json`) into the game's data folder;
*Build the game* runs `tools/build_port.py rac3-pal --disc {disc}`; *Get the levels ready* reads
the levels into `assets/port/rac3/pal` (`levels-4.json`), and *Play* runs
`openrac-rac3-pal --data {data} --window --levels {root}/assets/port/rac3/pal` after a warning that
no level loads yet. Nothing reads the levels until the first level loads.

`OPENRAC_RAC3_CHAINS=N` prints the VIF codes of the first N display lists and the front end's
state words (mode, boot state, boot screen, the pad's held and pressed buttons) each frame.

## How it starts

The disc boots `SCES_524.56`, a loader at `0x00800000` whose data holds the game WAD-compressed
(file offset `0x7080`) as a list of records (the level program format). The port loads that file
as the console's loader would, and its entry (`host/entry.c`, `func_0013ADA8`) unpacks the
records, then does crt0's user-mode steps and calls `main` (`0x0013B330`). `main` runs the loaded
program's entry, `0x00381250`: boot_elf's own front end (frontbin's twin), which is the boot
screens and the main menu. When it returns, `0x0013AFF8` gives the next program's entry (frontbin
or a level, not handled yet).

## What the port answers

| | |
|---|---|
| `libraries.tsv` | 204 library entry points, found by their instructions against rac2-ntsc's and rac1-pal's tables, by shape where the SDK version differs, by RPC function number (libmc), and by the game's own calls (libpad) |
| `host/entry.c` | the loader's unpacking, crt0, InitSystemCallTableAddress (no kernel to scan) |
| `host/dma.c` | sceDmaSend leaving CHCR as the DMAC does (the VIF1 handler retires the frame's wait mask from it), KickVif0Chain |
| `host/sound.c` | the 989snd reply wait and flush: answered at once (no IOP, no sound yet) |
| `host/pad.c` | libpad 4.0, Deadlocked's host pad at this disc's addresses |
| `host/movie.c` | the movie player entry: the shared native player |
| `host/trig.c`, `host/vu0.c` | VU0 cos/sin (the host's for now) and sceVu0UnitMatrix |
| `host/spr_dma.c` | the scratchpad DMA channels the kernels start |
| `host/chain_census.c` | `OPENRAC_RAC3_CHAINS` |
| `profile.cpp` | draws the display list the game sends (as rac4-ntsc) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record |
| `IDENTIFIED.tsv` | every function and global worked out on the way, for the backfill |

`hostgen.json` also lists `from_assembly` (C that forwards registers or has VU0 inline assembly,
translated from its instructions), `executable_extents` (names the C only declares inside another
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
- **The next program**: frontbin and the levels (`0x0013AFF8`, overlays) are not loaded; hostgen's
  `frontbin` group and `relocate` are not set up for them.
- The memory card works through the shared `sceMc*`; saving is untested.
