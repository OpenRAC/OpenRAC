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

## What the port answers

| | |
|---|---|
| `libraries.tsv` | 204 library entry points, found by their instructions against rac2-ntsc's and rac1-pal's tables, by shape where the SDK version differs, by RPC function number (libmc), and by the game's own calls (libpad) |
| `host/entry.c` | the loader's unpacking, crt0, InitSystemCallTableAddress (no kernel to scan) |
| `host/dma.c` | sceDmaSend leaving CHCR as the DMAC does (the VIF1 handler retires the frame's wait mask from it), KickVif0Chain |
| `host/sound.c` | 989snd's command boundary (snd_Pump, snd_SendCommandSync, snd_QueueCommandToRing, the bank loads) on the ring rac2 shares (`common/lib/snd_ring.c`): the native sound engine plays the banks' sounds and the VAG streams |
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

## Checking the C against PAL

Many candidates (nonmatching/) are NTSC-U's C with its names moved to PAL addresses; a literal
address, an offset into a table or a callee the move did not know stays NTSC-U's, and runs
wrong on PAL (the font select's palette tables 0x80 off, the text lookup's $gp fallback, the save
allocator's buffer top at 0x0022760C, each found only by running the game).
`check_c_constants.py` finds them all at once: for every function the build takes from C, it
compares the C's literals, data symbols and callees with PAL's own instructions of that function
(the setup's `asm/`), and also flags a $gp register variable, frame-rate values and the 50 Hz
(x*5+2)/6 timer scaling where C and assembly disagree, a draft nonmatching/README.md marks
"differs", and a draft of one program read for the other's function at the same address. Run it
after every decomp pin bump, from a decomp checkout whose setup wrote `asm/` and `baserom/*.elf`:

```sh
python3 port/game/rac3-pal/check_c_constants.py --asm-source <decomp with asm/>   # ~1 min
```

It exits 1 when a candidate has a strong hit; leave each one out in `hostgen.json`
`candidates_left_out` (its assembly is translated instead) with the reason it prints, or correct
the draft in rac3-decomp. Weak hits (small literals, inline assembly) are hints for review. The
first run (rac3-decomp `c73ab8f`) checked 285 candidates and 2,434 matched functions and found 40
candidates with strong hits, the three above among them; the 2 strong hits in matched C are
matched $gp register variables (`func_0012AA78`, `func_0012FD50`).

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
- **Music**: the menu's sound effects play (the front end's bank, an SBlk version 3 block, which
  the shared bank reader now reads), but the boot front end asks for no music: its stream table
  (the level table LoadLevelToc fills, `0x00160C40 + 0x7A04`) is never loaded and the music channel
  stays at track -1. The streams' command (0x2C, two mono files as rac2's) is bound and untested.
  `OPENRAC_AUDIO_TICKED=1 OPENRAC_AUDIO_DUMP=out.wav OPENRAC_SOUND_LOG=1` captures and logs it.
- **The next program**: frontbin and the levels (`0x0013AFF8`, overlays) are not loaded; hostgen's
  `frontbin` group and `relocate` are not set up for them.
- Saving from a level: the memory card works through the shared `sceMc*` (below), but no level
  loads yet, so the game's own saves in a level are untested.

## The memory card

The game's own card code runs as it is on the shared library (`port/game/common/lib/mc.c`), the
card being a folder (`--cards`, by default `<data home>/memcard/SCES_524.56`). It checks the card
when it starts and reads Ratchet & Clank's and Going Commando's saves from the folders beside it.
New Game on a card without its save asks to create one (Yes is **Circle**, No Triangle), which
writes `BESCES-52456RATCHET/` (`icon.sys`, `static.ico`, `net.bin`, `save0.bin` to `save3.bin`
and a file named as the folder); the Load page lists the four slots and reads the chosen save
whole. `host/save_format.h` is the save's layout (launcher/docs/RAC3_SAVE_FORMAT.md) and
`host/play.c` tells the shared code which files are saves. `OPENRAC_TRACE_MC=1` prints every card
command; `OPENRAC_RAC3_CHAINS` also prints the card machine's state and its dialog.

Two translations were wrong on the card's path and are left to their assembly: the scratch buffer
allocator (`func_003932E8`, whose NTSC-U draft reads NTSC-U's address of the buffer's top) and the
menu pages' callback callers (`func_003E73D0` and four more, which pass the page on in a0).
