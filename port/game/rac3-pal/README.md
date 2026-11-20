# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`).
The NTSC-U drafts are read only where PAL's code is the same.

**An experiment, not playable.** It boots to the main menu, which runs, takes the pad and draws
its frames, labels and icons (the 2D layer's routines, wrapped: `host/ui2d.c`) over its
background picture, but no 3D (below). Levels are not loaded.

## Running it

The functions with no C are translated from the disc image: the player's (`--disc`, or
`OPENRAC_RAC3_PAL_RETAIL_IMAGE`), else `games/rac3/decomp/baserom/SCES_524.56.iso`. The disc's
executable is the loader; retail.py reads it as the game it carries packed (the same sections as
the decompilation's `boot_elf.elf`, which the build does not need).
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
the levels into `assets/port/rac3/pal` (`levels-6.json`), and *Play* runs
`openrac-rac3-pal --data {data} --window --levels {root}/assets/port/rac3/pal` after a warning that
no level plays yet. The game draws a level's world from them ([Drawing the levels](#drawing-the-levels)).

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
  game reads them early in its pass. Measured with `OPENRAC_INPUT_LATENCY` (a virtual pad
  pressing at random moments; `docs/port/DEBUGGING.md`) in the main menu, hidden window:

  | from a press to | 60 Hz ticks (before) | 50 Hz | 50 Hz, Responsive controls | Vulkan, 50 Hz | Vulkan, Responsive |
  |---|---|---|---|---|---|
  | the game's pad read | 27.4 ms | 30.4 ms | 9.8 ms | 16.9 ms | 10.3 ms |
  | its tick's frame shown | 45.0 ms | 50.2 ms | 29.3 ms | 53.1 ms | 46.1 ms |
  | the frame with the response | 62.5 ms | 71.1 ms | 50.2 ms | 73.8 ms | 66.6 ms |

  OpenGL unless marked, averages of 72 to 88 presses each (the "before" column is the tree
  before the 50 Hz change, the others the main menu with native 2D). A hidden window is never
  focused, so the window's background throttle (a sleep of a tick after each swap, on a clock of
  its own) held these runs to 56 (60 Hz) and 48 (50 Hz) ticks a second, and its late ticks
  lengthen every row; a focused window keeps the full rate. Vulkan hands each tick's frame to the
  render worker, which presents it at the next handoff (frontend/README.md: bounded latency for
  overlapping work), hence its later frames. The menu's response is in the frame after its tick's,
  as on the console and in Going Commando: the list drawn is the one the last pass built (sent at
  the top of the pass), and the 2D records go with the list whose markers name them
  (`frontend/frame.cpp`, `ui_kept`).
- **Frames between ticks**: the frame rate is the player's (Video > Frame rate: Original, the
  display's, 30 to 360 or unlimited, as Going Commando's), the game's time its own 50 ticks a
  second. With a world camera the frames between ticks move the camera and the mobys from one
  tick to the next (`profile.cpp`'s `read_state` gives the view's rows and `read_moby` the mobys,
  and `frontend/interpolation.cpp`'s `wants_between` takes them), as Going Commando's. The front
  end's menus have no world camera (their background is a 2D record, `host/picture.c`), where
  Going Commando's title has one: the profile's `frames_between_without_camera` gives them the
  frames between ticks all the same, the 2D the tick recorded (`ui_kept`) drawn again unchanged
  until the next tick. Nothing the game counts moves with them: its passes are the ticks, the
  menus' animations and idle count (`D_001D5688`) step once a pass, and the T0 time
  (`D_001D4AC0`) and the frame time (`D_001D9D0C`) stay 0 at any frame rate, so
  func_003E3E68 never takes a double step. Movies play on the player's own clock, a picture a tick.

  Measured in the main menu (Apple M5 Max, 120 Hz display, hidden window with
  `OPENRAC_FOCUSED_PACING`, the last 10 s of a 25 s run, `OPENRAC_PACING` and
  `OPENRAC_WATCH=1D5688:1:i`; OpenGL unless marked):

  | `frame_rate`, `vsync` | frames a second | ticks a second | the menu's idle count a second | before (a frame a tick) |
  |---|---|---|---|---|
  | `original` | 50.1 | 50.00 | 50.00 | 50 |
  | `display`, on | 120.1 | 49.94 | 49.94 | 50 |
  | `display`, off | 120.1 | 50.04 | 50.04 | 50 |
  | `60`, off | 60.1 | 50.00 | 50.00 | 50 |
  | `144`, off | 144.1 | 49.93 | 49.93 | 50 |
  | `240`, off | 239.9 | 50.00 | 50.00 | 50 |
  | `unlimited`, off | 1806 | 50.00 | 50.00 | 50 |
  | Vulkan, `144`, off | 144.2 | 50.07 | 50.07 | 50 |
  | Vulkan, `unlimited`, off | 4147 | 50.00 | 50.00 | 50 |

  A frame between ticks at 144 is the tick's own picture, pixel for pixel (`OPENRAC_SHOT_BETWEEN`
  at tick 800: the tick's frame and its four between-tick frames the same). A cap under the
  tick rate (`30`) still draws every tick's frame in the menus (50 a second): their lists send
  images, and a tick's frame with images is never left out (`frame.cpp`, `retain_tick_uploads`;
  in a level the HUD keeps them). Without `OPENRAC_FOCUSED_PACING` a hidden window is never
  focused, and the window's background throttle holds it to a frame a tick (48 a second at
  `unlimited`).

## What the port answers

| | |
|---|---|
| `libraries.tsv` | 204 library entry points, found by their instructions against rac2-ntsc's and rac1-pal's tables, by shape where the SDK version differs, by RPC function number (libmc), and by the game's own calls (libpad) |
| `host/entry.c` | the loader's unpacking, crt0, InitSystemCallTableAddress (no kernel to scan) |
| `host/dma.c` | sceDmaSend leaving CHCR as the DMAC does (the VIF1 handler retires the frame's wait mask from it), KickVif0Chain |
| `host/sound.c` | 989snd's command boundary (snd_Pump, snd_SendCommandSync, snd_QueueCommandToRing, the bank loads) on the ring rac2 shares (`common/lib/snd_ring.c`): the native sound engine plays the banks' sounds and the VAG streams |
| `host/pad.c` | libpad 4.0, Deadlocked's host pad at this disc's addresses |
| `host/movie.c` | the movie player entry, the boot program's and frontbin's: the shared native player |
| `host/program.c` | which program is loaded (the overlay hook), `OPENRAC_RAC3_NEXT` |
| `headers/vu0.h` | the vector-library helpers of the decompilation's `include/vu0.h` (asm Clang refuses) in C, read in its place (hostgen.json `headers`) |
| `host/trig.c`, `host/vu0.c` | VU0 cos/sin (the host's for now) and sceVu0UnitMatrix |
| `host/spr_dma.c` | the scratchpad DMA channels the kernels start |
| `host/ui2d.c` | the 2D layer's routines, wrapped as Going Commando's (rac2/host/ui2d.c): boxes, glyph quads, turned sprites, ring segments, the font's glyphs (drawn from its distance field) and the menus' Widget3D sprites become window records |
| `host/chain_census.c` | `OPENRAC_RAC3_CHAINS` (a rough census: it reads image data inside DIRECT as VIF codes) |
| `host/menu_log.c` | `OPENRAC_RAC3_MENU_LOG`: the front end's state and printed strings a frame, for `menus.py` |
| `profile.cpp` | draws the display list the game sends (as rac4-ntsc), the records at their markers in it; the camera, view, fog and live mobys of each program; the levels extracted (`has_levels`), the title drawn without a world (`has_title_world`) |
| `host/render.c` | the game's hand-written renderers the port does not run: each level's DrawFrame passes (terrain, ties, shrubs, sky shells, MobyProc) tell the window what to draw from the extracted level, as Going Commando's (below) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record |
| `host/options.c` | the title's Options: the port's Video page in the 16:9 row's place, the game's 16:9 kept off, the subtitles, the sound mode and the language remembered between runs (below) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record; in a picture wider than 4:3, its blurred edges out to the window's sides, as Going Commando's title |
| `host/view.c` | the game's own 16:9 option kept off, so that the window's Picture setting alone shapes the view (as Going Commando's) |
| `IDENTIFIED.tsv` | every function and global worked out on the way, for the backfill |

`hostgen.json` also lists `from_assembly` (C that forwards registers or has VU0 inline assembly,
translated from its instructions), `candidate_folders_left_out` (the NTSC-U drafts of boot_elf and
frontbin give way to the programs' own instructions, which the build reads from the disc: some
drafts were far from PAL's code and crashed frontbin's start-up and the level loading screen), `executable_extents` (names the C only declares inside another
function start nothing) and `unpassed_registers` (a K&R call's missing arguments are the
caller's registers).
translated from its instructions), `executable_extents` (names the C only declares inside another
function start nothing), `unpassed_registers` (a K&R call's missing arguments are the
caller's registers), `candidate_scopes` (frontbin's drafts are for frontbin only: its functions
share the boot program's addresses but not their code) and `asm.signatures` (a translated
function's arguments and result the decompilation's declarations do not give: the text widget's
draw hands its second argument on, and the font's width wrappers return the width they forward).

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

The darkening over it (`func_0038B200`: a colour, 0x60 of black, over the picture as sixteen
32-pixel sprites of the fixed packet at `0x00141850`) is one flat 2D record, and the four corner
ornaments (`func_00397680`, a HUD texture turned about its centre with a colour a corner) are
records too: the main menu, its pages and Options draw nothing from the game's packets but the
frame's own clear and hand-over (`OPENRAC_UI_RESIDUAL=1` lists what is left).

## The options

The title's Options page (the boot program's screen at `D_001DA168`) is a list widget of two
columns, the rows' names and values, over the options at `D_00143950`: 16:9 (+0xAD), Subtitles
(+6), Sound, stereo or mono (+8) and Language (+0xB7, the game's 1 English, 2 French, 3 German,
4 Spanish, 5 Italian). As Going Commando's port does with its 16:9 page (rac2/host/options.c), the
16:9 row is the port's **Video** page: the window's picture has the player's shape, so the game's
16:9 (which narrows the view for a television that stretches it back) is kept off. Cross on it
shows the port's settings (`openrac_game_option_*`, the in-game menu's entries marked for the
game's pages) in the same list, four rows at a time scrolling with the cursor as the game's own
lists scroll; Cross or Right steps the row under the cursor, Left steps it back, Triangle goes
back to the game's rows. A step is saved to the settings file and applied at once.

Each row has its picture beside the list, the preview widget's animation for the row (the items'
fourth word at `D_0037F830`: 16:9's 5, Subtitles' 2, Sound's 4, Language's 9), a Widget3D sprite
drawn as 2D records. The Video row shows the 16:9 row's picture, the one the game gives its
screen setting, and the Video page keeps it while it is open.

A console forgets the options at power off; the port keeps them in the settings file
(`game_subtitles`, `game_playback`, and the language as the console's `game_language`, which
the front end's start-up reads back through `sceScfGetLanguage`). The in-window menu (Esc) and the
F-key overlay (F3) are the shared ones, and the pad is the shared keyboard and controller path
(Return is Start).

## Drawing the levels

As Going Commando's port (`rac2/host/render.c`): the game's renderers, hand-written VU1 and GS
packet builders, do not run. Each level's frame pass, DrawFrame (Veldin's `0x00403440`, Going
Commando's `func_0027BD70`, whose calls line up with it one for one), runs as translated, and its
passes end in `host/render.c`, which tells the window what the game drew this frame and where in
the display list (`openrac_game_draw`, the world's and the mobys' markers): DrawTfrag (the
terrain, and the world's marker), the tie and shrub passes, SkyDrawShell (each shell with the
matrix the sky code built for it, `0x001E0E10` in Veldin) and MobyProc (the mobys' drawn flags as
the console sets them, `+0x31`; the window draws the live mobys the profile reads), PartProc
(each live particle of the pool, as Going Commando's records and texture table) and DrawWorldQuad
(the draw callbacks' quads, their textures from GetEffectTex's upload records). The per-frame
lighting and texture-packet passes do nothing: the extracted level carries what they computed. The
window draws the world natively from the extracted level (`--levels`, the scene module's level
renderer: `port/scene`), with the camera, view, fog and mobys the profile reads.

The game's own screen glow (Going Commando has none) is left out: DrawFrame references two
prebuilt GS packets while `0x001DBCA0` is set, the frame copied into a 256 x 256 buffer (FBP 0x198)
and shrunk (`0x004DE670`), then blurred and added back over the frame (`0x004DE6F0`, ALPHA 0x221).
Replayed over the native world they drew a ghost of the picture and a copy of it in its top left
corner; the enhanced renderer's bloom (`port/enhanced/docs/bloom.md`) is the port's glow.

Every level's program carries its own copy of the engine, at its own addresses: the hooks are
written with Veldin's (`func_L01_...`, the names hostgen gives the catalogue's places) and its
globals go through `OPENRAC_LDATA(1, ...)` to the loaded level's. The multiplayer maps (39 to 53)
share most of them, but have their own DrawTfrag, tie and shrub passes and MobyProc, written with
level 39's (`func_L39_...`, `OPENRAC_LDATA(39, ...)`). `IDENTIFIED.tsv` has each function and its
Going Commando pair.

The extracted levels are the shared export (`editor/extract.py port`), format 6 for this game
(`editor/port.py` `GAME_FORMATS`): terrain, ties, shrubs, mobys with skeletons and animations, the
sky's shells and its panorama, collision, (5) Ratchet's mesh and textures from his first pack
(`editor/wads.py` `RAC3_PAL`, the table LoadPlayerDisplayTextures reads), which the level's class 0
draws him with, and (6) the terrain and shrubs lit as the game lights them at load and the ties'
colours per vertex, as Going Commando's (`games.py` `world_lighting`, `UYA_TIE`): the lighting
routines are Going Commando's instructions, and the (cos, sin) table they light by is in each
level's own program, not the loader the disc boots (`lighting.program_normal_table`). Against Going
Commando's format 20 it lacks: the chrome and glass maps (grey), the mobys' glow vertices, light
sets and bangles, his other skins, his wrench and what he holds, his ship, and the enhanced
renderer's bake inputs (gameplay copy, emissions, liquids).

To look at a level without the game (no game code runs; the viewer's own camera):

```sh
python3 editor/extract.py port <disc> build/rac3pal --level 1 --anywhere
openrac-viewer build/rac3pal/level_01 --camera 365,103,82,90,-5 --screenshot veldin.png --frames 5
```

What a level's picture still lacks (Veldin, 2026-11-20, hidden runs on box2 with
`OPENRAC_RAC3_NEXT=60:1`): the sky, terrain, ties, shrubs, fog and the live mobys (Ratchet posed
and animated) draw natively with the game's camera, but

- the second screen effect DrawFrame references (`0x004DF310`, two packets at `0x001D3650` and
  `0x001D3EF0` while `0x001DBCBC` is set) is not identified and still replayed;
- the export's gaps above (chrome and glass, glow, bangles, Ratchet's other skins, what he holds,
  his ship), water, shadows and the enhanced renderer's inputs.

## What is missing

- **The Options rows' text is cut at both sides**: each row's text is laid out about three glyphs
  left of its column on the left and one right on the right, and clipped by its panel (the
  translated layout, func_003AC590 → func_0038FDF8; the game's own packets show the same). The
  Video page's rows inherit it.
- **The mods' pages** (`openrac_api.ui_main_item`, Going Commando's rac2/host/mod_pages.c) are not
  in rac3's menus yet.

- **Music**: the menu's sound effects play (the front end's bank, an SBlk version 3 block, which
  the shared bank reader now reads), but the boot front end asks for no music: its stream table
  (the level table LoadLevelToc fills, `0x00160C40 + 0x7A04`) is never loaded and the music channel
  stays at track -1. The streams' command (0x2C, two mono files as rac2's) is bound and untested.
  `OPENRAC_AUDIO_TICKED=1 OPENRAC_AUDIO_DUMP=out.wav OPENRAC_SOUND_LOG=1` captures and logs it.
- **A level**: see [The next program](#the-next-program).
- The memory card works through the shared `sceMc*`; saving is untested.
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
