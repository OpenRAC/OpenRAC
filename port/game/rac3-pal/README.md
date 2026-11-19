# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

**An experiment, not playable.** It boots to the main menu, which runs, takes the pad and draws
its frames, labels and icons (the 2D layer's routines, wrapped: `host/ui2d.c`) over its
background picture, but no 3D (below). Levels are not loaded.

## Running it

The functions with no C are translated from `boot_elf.elf`, the unpacked game the
decompilation's setup puts in `games/rac3/decomp/baserom/` (not from the disc image yet: the
disc's executable is the loader, whose payload retail.py does not unpack).

```sh
cmake --preset release && cmake --build --preset release --target openrac-rac3-pal
# a data folder with iso_data/rac3/SCES_524.56 (the disc's boot executable) and iso_data/rac3/disc.iso
SDL_AUDIODRIVER=dummy "port/build/release/apps/openrac-rac3-pal/Ratchet & Clank 3 (PAL).app/Contents/MacOS/Ratchet & Clank 3 (PAL)" \
    --data <data> --window --skip-movies
```

`OPENRAC_RAC3_CHAINS=N` prints the VIF codes of the first N display lists and the front end's
state words (mode, boot state, boot screen, the pad's held and pressed buttons) each frame.

## Testing the menus

`menus.py` walks every page the front end reaches, each route from the boot screens: the card
notices (an empty card and no card), the main menu, Load Game, New Game and its save questions,
Multiplayer, the multiplayer tutorial, Options with each row, Adjust Screen and every language,
the Sly 2 demo's button combination and the attract movie after 6000 idle frames. It runs the port
hidden and muted, presses the pad by script (`OPENRAC_PRESS`) and reads `OPENRAC_RAC3_MENU_LOG`
(each frame's state words and every string the font printed). After each step the page must have
printed its strings (none empty or "(null)"), changed after the press and still be running; a
picture of each step is kept. Re-run it after every change to the front end:

```sh
python3 port/game/rac3-pal/menus.py --data <data> --out build/rac3-menus \
    --exe "port/build/release/apps/openrac-rac3-pal/Ratchet & Clank 3 (PAL).app/Contents/MacOS/Ratchet & Clank 3 (PAL)"
```

`build/rac3-menus/report.md` has a row for every step; each route's folder has its pictures,
`menu.log`, `pages.txt` (the frames where the page changed) and `run.log`. A failure another branch
owns (the level load, the multiplayer executable, saving) is marked known and does not fail the
run. `--list` prints the routes, `--only` picks some, `--jobs` runs several at once.

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
| `host/ui2d.c` | the 2D layer's routines, wrapped as Going Commando's (rac2/host/ui2d.c): boxes, glyph quads, turned sprites, ring segments, the font's glyphs (drawn from its distance field) and the menus' Widget3D sprites become window records |
| `host/chain_census.c` | `OPENRAC_RAC3_CHAINS` (a rough census: it reads image data inside DIRECT as VIF codes) |
| `host/menu_log.c` | `OPENRAC_RAC3_MENU_LOG`: the front end's state and printed strings a frame, for `menus.py` |
| `profile.cpp` | draws the display list the game sends (as rac4-ntsc), the records at their markers in it |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record |
| `IDENTIFIED.tsv` | every function and global worked out on the way, for the backfill |

`hostgen.json` also lists `from_assembly` (C that forwards registers or has VU0 inline assembly,
translated from its instructions), `executable_extents` (names the C only declares inside another
function start nothing), `unpassed_registers` (a K&R call's missing arguments are the
caller's registers), `candidate_scopes` (frontbin's drafts are for frontbin only: its functions
share the boot program's addresses but not their code) and `asm.signatures` (a translated
function's arguments and result the decompilation's declarations do not give: the text widget's
draw hands its second argument on, and the font's width wrappers return the width they forward).

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

- **The rest of the 2D layer**: the menu's two corner ornaments (Widget3D quads whose PRIM takes
  UV, which the routine does not send) and the dimming fill behind the menu are still the game's
  packets; the shaded print (func_0038F458, a colour a corner) is not wrapped (its prototype is not
  known).
- **Sound**: the 989snd commands are answered with empty replies; the shared sound replacements
  are not bound.
- **The next program**: frontbin and the levels (`0x0013AFF8`, overlays) are not loaded; hostgen's
  `frontbin` group and `relocate` are not set up for them.
- The memory card works through the shared `sceMc*`; saving is untested.
