# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

**An experiment, not playable.** It boots to the main menu, which runs and takes the pad, but
draws only its boxes over its background picture: no text, no 3D (below). Levels are not loaded.

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

## How it starts

The disc boots `SCES_524.56`, a loader at `0x00800000` whose data holds the game WAD-compressed
(file offset `0x7080`) as a list of records (the level program format). The port loads that file
as the console's loader would, and its entry (`host/entry.c`, `func_0013ADA8`) unpacks the
records, then does crt0's user-mode steps and calls `main` (`0x0013B330`). `main` runs the loaded
program's entry, `0x00381250`: boot_elf's own front end (frontbin's twin), which is the boot
screens and the main menu. When it returns, `0x0013AFF8` gives the next program's entry (frontbin
or a level, not handled yet).

## Ticks, pacing and input

The game's tick is the port's, as Going Commando's and Ratchet & Clank's are: the shared tick
handler, frame pacing and input of `game/common/program/vsync.cpp` and `game/common/frontend/`.
The front end's loop (`func_00381250`) does, each pass: send the list the last pass built to VIF1
(`func_003A9588`, then `func_003A9508` swaps the two list buffers), read the pads
(`func_003A3F70`), run the mode's update and build the next list, wait for the list's stages
(`func_003A96A8`, already retired: the DMA hand-off is synchronous, `host/dma.c`) and call
`sceGsSyncV`. `sceGsSyncV` is the shared replacement (`libraries.tsv`, the instructions of
rac2-ntsc's): the end of the tick, where the window takes the frame, reads the controllers,
waits for the next tick's time, draws frames between ticks when it can, and runs the game's
vertical blank callback (`func_0013D208`).

- **Rate**: 50 ticks a second (`hostgen.json` `tick_rate`, as Going Commando's PAL). The PAL game
  sets its settings word `0x00143950` to 1 at boot and never clears it: it always asks for PAL's
  50 Hz mode (`func_00391708`) and times its passes in fiftieths (`D_001D9D0C`). The 60 Hz branch
  is the US code's, which the PAL game never takes. Before, the port ticked it at 60 and its menus
  ran 1.2 times too fast.
- **Input**: read at the end of each tick (or just before the next one, with the player's
  Responsive controls, which reads the pads as the tick starts: `frontend::refresh_pads`). The
  game reads them first thing in its pass. Measured with `OPENRAC_INPUT_LATENCY` (a virtual pad
  pressing at random moments; `docs/port/DEBUGGING.md`) in the main menu, hidden window, OpenGL:

  | from a press to | 60 Hz ticks (before) | 50 Hz ticks | 50 Hz, Responsive controls |
  |---|---|---|---|
  | the game's pad read | 27.4 ms | 32.1 ms | 10.2 ms |
  | its tick's frame shown | 45.0 ms | 53.0 ms | 31.1 ms |
  | the frame with the response | 62.5 ms | 73.9 ms | 52.0 ms |

  Averages of 72 to 88 presses each. A hidden window is never focused, so the window's background
  throttle (a sleep of a tick after each swap, on a clock of its own) held these runs to 56 and 46
  ticks a second, and its late ticks lengthen every row; a focused window keeps the full rate.
  The menu's response is in the frame after its tick's: the window draws the list the game sends
  at the top of a pass, which the last pass built, as the console shows it. Native 2D (the game's
  2D routines wrapped, as `rac2/host/ui2d.c`) is drawn on the tick that made it and takes that
  frame (about 20 ms) off.
- **Frames between ticks**: drawn at the display's rate, with the camera and objects between two
  ticks, as soon as the profile reads a world: `profile.cpp`'s `read_state` with a camera
  (`has_camera`) and `read_moby` (`frontend/interpolation.cpp`'s `wants_between`). Until then
  each tick is one frame.

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
