# openrac-rac3-pal: Up Your Arsenal, PAL

Up Your Arsenal's PAL disc (`SCES_524.56`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac3](../../../games/rac3/decomp), whose C is PAL's own (`src/boot_elf`, `src/frontbin`,
`src/levels`). The NTSC-U drafts are read only where PAL's code is the same; NTSC-U's own program is
this one carried to that disc's addresses ([rac3-ntsc](../rac3-ntsc/README.md)). OpenRAC carries no
patches to rac3-decomp: fixes hostgen needs in its C (such as `func_L07_003C8BA0`'s assembly lines,
written with `\n` escapes so Clang reads them) go into rac3-decomp's master, and the pin follows it.

**An early port, playable in parts.** It boots through the boot screens and the intro movie to the
main menu, whose pages run and take the pad; a new game or a save loads its level, which is drawn
natively from the disc's level data (the world, the mobys, the sky, particles), with the hero, the
HUD and its text, the pause menu and its pages, the game's own saving and loading, sound and the
level's movies. The multiplayer executable runs its menus; its maps do not play yet. What is still
missing is listed [at the end](#what-is-missing). The launcher builds and starts it from a player's
PAL disc (below).

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
# a data folder set up by python3 tools/extractor.py <image> --game rac3 -e --proj-path <data>:
# iso_data/rac3/ with the disc's files, sectors.tsv and programs/ (the two programs, unpacked)
SDL_AUDIODRIVER=dummy "port/build/release/apps/openrac-rac3-pal/Ratchet & Clank 3 (PAL).app/Contents/MacOS/Ratchet & Clank 3 (PAL)" \
    --data <data> --window --skip-movies
```

From the launcher (launcher/actions.json, `rac3/pal`), as for Going Commando: *Set up from your
disc* runs `tools/extractor.py`, which checks the disc against games/rac3/game.json and writes
`iso_data/rac3/` into the game's data folder: the disc's files, every sector outside its file system
as lumps (`lumps/`), `sectors.tsv` (the map the game reads its sectors by), the boot program's and
I5BOOTN.ELF's records unpacked (`programs/*.records`, which LoadExecPS2 copies in: `host/exec.c`),
`buildinfo.json`, and the image, which only the level tools read;
*Build the game* runs `tools/build_port.py rac3-pal --disc {disc}`; *Get the levels ready* reads
the levels into `assets/port/rac3/pal` (`levels-17.json`), and *Play* runs
`openrac-rac3-pal --data {data} --window --levels {root}/assets/port/rac3/pal` after a warning that
the port is early. The game draws a level's world from them ([Drawing the levels](#drawing-the-levels)).

`OPENRAC_RAC3_CHAINS=N` prints the VIF codes of the first N display lists and the front end's
state words (mode, boot state, boot screen, the pad's held and pressed buttons) each frame.

## Testing the menus

`menus.py` walks every page the front end reaches, each route from the boot screens: the card
notices (an empty card and no card), the main menu, Load Game, New Game and its save questions,
Multiplayer (the other executable, there and back), the multiplayer tutorial, Options with each
row, Adjust Screen and every language,
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
owns (the level load, saving) is marked known and does not fail the
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
- **A level** loads and runs: on the way to one (New Game, Load Game, `--level N` with
  `OPENRAC_DIRECT`, or `OPENRAC_RAC3_NEXT`) the boot program's loading screen unpacks its pack (the
  WAD decompressor on the shared routine, `host/wad.c`), loads its sound bank (`0x0013BC30`, the
  shared `snd_BankLoadFromEE_CB`) and draws natively ([The loading screen](#the-loading-screen));
  the level's code is translated from its own C (`src/levels`) and, where it has none, from the
  disc's level programs, by rac3-decomp's catalogue (`places`, `config/levels/functions.tsv`).

`OPENRAC_RAC3_NEXT=FRAME:LEVEL`, a development probe from before the menus could be read, writes what a menu's choice writes, at
the boot program's FRAME-th display list (`-1`: frontbin).

## The multiplayer executable

Multiplayer is another executable. The main menu's MULTIPLAYER row ends the boot program with
`LoadExecPS2("cdrom0:\I5BOOTN.ELF;1", 2, {..., "BOPT=..."})` (`0x00398128`, `0x003980A0`,
`0x00124ED8`), and leaving multiplayer starts `SCES_524.56` again the same way. `I5BOOTN.ELF` is
the boot executable's loader again (rac3-decomp's `src/i5bootn`, matched C) carrying another game
WAD-compressed at file offset `0x7080`: an engine core at `0x00100000` to `0x00241A00` whose
libraries are the boot program's SDK build at other addresses, its own program at `0x0027B100`
(the boot program's front end and 2D layer, `0x0038xxxx` there) and the multiplayer levels'
programs over `0x00241A00`: level 39 is the multiplayer menus, 40 to 55 the maps.

The port builds both into one program. `mp/hostgen.json` (beside this file's, listed in its
`executables`) translates the multiplayer game from the disc's own instructions, every C name with
`mp_` before it (`"prefix"`): the executable read from the disc image (`asm` `retail` `executable`),
its entries found by its calls and by the code addresses its data holds (`discover_entries`,
`discover_pointers`), and level 39's program with it (`levels`, `discover_levels`), its functions
named `func_L39_...` as the catalogue would. cmake/Games.cmake writes it into `gen/mp` and compiles
it into the game's library with the boot program.

`mp/libraries.tsv` is the boot program's rows at the multiplayer addresses, each found by the
instructions (the boot program's function against this one's, immediates and call targets masked;
the system-call wrappers by their call number). The rows with a signature bind to `host/mp.c`,
which hands them to the boot program's host C (the pads, DMA, 989snd's ring with this program's
reply word); its 2D layer is wrapped in `host/ui2d.c` beside the boot program's, through the same
helpers with this program's addresses (its 2D offset `0x00240310`, list cursor `0x002446A8`).

LoadExecPS2 (`host/exec.c`) does what the console does: the program that called it does not come
back (a longjmp to `func_0013ADA8`, the port's entry), the other executable is read from the disc
image, its game unpacked over memory, its functions registered in place of the first's
(`openrac_guest_unregister_all`), and its crt0's steps run with the arguments the kernel hands it
(`argv[0]` the path, then the caller's). The multiplayer program's next-program routine
(`0x00192F68`, the boot program's `0x0013AFF8`) says which of its programs is loaded, for the
overlay hook: its own front end, or level 39.

The menus' level is the multiplayer program's own copy of the front end's routines: its 2D layer
(the catalogue's frontbin rows at level 39's places, frontbin's font print `func_0038E7E8` at
`0x00592238`) is wrapped in `host/ui2d.c` (list cursor `0x0024CAD0`), its KickVif0Chain and WAD decompressor are the
boot program's host C (`host/mp.c`), and the window draws its picture as the 2D path alone
(`profile.cpp` `level_is_menus`). The menus run and take the pad: the network notice, LOCAL PLAY's
CREATE GAME page and its options, and EXIT MULTIPLAYER, which starts `SCES_524.56` again through
its boot screens to the main menu (`menus.py`'s `multiplayer` route).

The two routines that build a font page's TEX0 (the core's `0x0027EAC8`, the boot program's
`0x0038A0E0`; level 39's `0x00589FF8`, the catalogue's `func_L01_004047A8`) return all 64 bits
(`mp/hostgen.json` `asm` `signatures`): cut to 32, the CLUT's address and the page's height were
lost and every glyph read an empty palette. Level 39's screen glow is left out as the boot
program's levels leave it (`host/mp.c`).

The menus' rows (ONLINE PLAY, LOCAL PLAY ...) are GUI text widgets: their draw
(`func_L39_00619B48`) prints at the centre of the rect their getter (`func_L39_00619D20`) works out
from the text's width times the scale that fits it in its box, `0x005946B0` (the boot program's
`func_00390FD8`). That routine returns a float in `$f0` and uses `$v0` only for a test; hostgen
typed it `int` (a function declared nowhere that writes `$v0` returned it), so the scale was
whatever `$f0` held and the rows were printed at x 14267, y -713, off the screen. The draw also
hands its own `$a1`, the parent's rect, on to the getter through the widget's vtable, which hostgen
did not see as a parameter (the boot program's frame widget draw, `func_003EEBF8`, does the same):
the rect was placed in an empty one at the top left. `asm` `live_arguments` gives a translated function
the argument registers it reads on any path before writing them (the signature check's flow
measure, which counts what a pointer call hands on), not only those read before its first call.
The LOADING screen's update (`0x006788F8`) needs it too: it tests `$a0`, the loader's end
(`0x005A4058`'s state at `0x00240288` past 1), only after its first call, so LOADING never ended;
with it the menus hand over to the chosen map's program (map 40's entry `0x004425C8`), which the
port does not translate yet. `mp/hostgen.json`'s
`asm` `result_flow` types such a routine `float` when no value it writes to `$v0` reaches its
`jr $ra` unread (316 of the executable's routines; 27 have callers that read `$f0`, among them the
vector library's). Level 39's one-colour and shadowed glyphs (`0x0058BBB0`, `0x0058BF70`) and the
turned HUD sprite with a colour a corner of both programs (the core's `0x00284F80`, level 39's
`0x0059CFD0`, the boot program's `func_00397680` instruction for instruction, their HUD texture
tables aside) are wrapped as the boot program's.

The menus' music is the boot program's stereo stream (`openrac_stream_play_stereo`, 989snd
command `0x2C`): it plays from the multiplayer menus' first frames. Command `0x30` (a stream query
with a callback, `0x001948B8`, which the music player keeps at `+0x14`) answers 0.

The maps (levels 40 to 53 on PAL: 40 to 49 online and LAN, 50 to 53 their split-screen builds) are
translated as level 39 is, each from its own program at its own addresses (`func_L40_...`):
`mp/maps.py` writes them into `mp/hostgen.json` (`retail` `levels`, `discover_levels`, each map's
entry as a root, `level_copies`) and the rows at the end of
`mp/libraries.tsv`, from rac3-decomp's catalogue. Every map carries a copy of the engine level 39
carries too, so the port stands level 39's host C in for each map's copy (`host/mp_maps.c`, the 2D
layer in `ui2d.c`, the HUD sprites in `hud.c`, one `MP_MAP*` line a map in `host/mp_maps.h`), its
globals level 39's relocated to the map's: the next-program hook (`mp.c`, `0x00192F68`) keeps level
39's and the map's programs as read and gives the map's number (the level word `0x002412DC`) as the
overlay, and `level_copies` tells the runtime which of the map's functions are copies of which of
level 39's (`openrac_guest_add_level_copies`), so `OPENRAC_LDATA(39, ...)` pairs them. The
single-player levels' and level 39's `asm` `signatures` reach the maps' copies as
`copy_signatures`. NTSC-U's are carried by `release_carry_rac3.py` (its maps 54 and 55, builds of
44 and 45, are not yet).

The maps' stock bugs that online play meets are fixed in `host/mp_fixes.c`, around each map's copy
of the game's functions (`mp/fixes.py` writes their rows and `host/mp_fixes.h` from the
catalogue, as `mp/maps.py` does): a gadget moby whose class the map never loaded (a crash), the
credit for hitting a player already dead, shooting and moving while dead, the flux rifle's muzzle
light left lit, a remote sniper shot stopped by a thin wall the shooter's client shot through,
Metropolis's base flag, and the net clock's stalls (turrets waiting on a remote state, a late
player dropped, the local player stalled on a silent host, a remote shot run at the sender's
time). The Horizon Private Server project fixed the same bugs for the console; we read its work
for reference only (CREDITS.md). `OPENRAC_RAC3_MP_STOCK=1` runs the stock code,
`OPENRAC_RAC3_MP_FIXES_LOG=1` logs each fix as it acts.

### Online play

ONLINE PLAY goes to the OpenRAC server (the OpenRAC/Server repository) instead of the retail
Medius, DME and DNAS servers: `host/mp_net.c` and the rows of `mp/libraries.tsv` that name it.
The retail stack (SCE-RT's rt_comm, rt_msg_client and rt_crypt, the Medius client 2.7.1 and DME
2.7.10 in the executable's core, DNAS in level 39) talks to the IOP's network modules, which the
port does not have, so it is cut at four places:

- the network adaptor and its configuration (the front end's module load `0x00198248`, MSIF RPC,
  ezNetCnf and ezNetCtl): an adaptor, one combination, the interface up;
- DNAS (level 39's `0x004B3910`..`0x004B3F88`, the signature `0x0067BBE8`): passed at once; the
  resolver thread (`0x0067A7E0`) writes the server's address;
- Medius at the message level: every request the library sends (`MediusSendLobbyMessage`
  `0x0017DFA8`: class, type and the struct) is answered by a small lobby in `mp_net.c`, whose
  replies go through the library's own dispatcher (`0x00152FB8`) in MediusUpdate
  (`0x0017E1A0`/`0x0017E208`/`0x0017E2D0`), so the library and the menus run as retail. Logging in
  says hello to the server under the account name; the game list is the server's party list of
  this disc's family (`rac3-pal-1`); creating a game hosts a party and lists its settings as the
  party's advert (updated from each WorldReport0); joining one joins that party;
- DME at the session level: connecting to a game's world (`0x0014ADD0`) gives a connection whose
  client indices are the party's order (the host 0, then the members as they joined), joining its
  session (`0x0014B448`) calls the game's three session callbacks, and every message the game or
  DME sends (`0x0014AD08`, `0x0014CAB8`, `0x0014C950`) is relayed unread to the other members
  (the server's `RELAY`); what comes back goes to DME's receive path (`0x0014CE48`) in its update
  (`0x0014B118`), which runs the game's handlers.

`OPENRAC_SERVER=host[:port]` (`[v6]:port` for IPv6; the launcher sets it) names the hub (`hub.openrac.dev:7177` when unset): the game's login is the hub's account login (a new name registers), its city list the hub's game servers, and the city picked the game server the lobby and the rooms run on (a second connection, proving the login with the hub's session token); a plain game server named there is played on directly, without accounts;
`OPENRAC_RAC3_NET_LOG=1` logs every lobby message and room packet. The protocol analysis these
cuts come from (every layer, message and struct, with addresses) is kept with the port's planning
notes (`rac3-mp/protocol.md`). NTSC-U carries the rows rac3_mp_pairs.py pairs; its resolver thread
is not paired yet.

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
- **Frames between ticks in a level**: the camera, the mobys and their poses move from one tick to
  the next as in the menus' description above. The effect quads and particles that carry an id
  (PartProc's pool slot, `host/render.c`) move with them (`interpolates_effect_quads`, as Going
  Commando's). In Veldin at 120 frames a second (box1, GTX 1070, `OPENRAC_SHOT_BETWEEN`), a gun
  bolt steps across a tick's four between-tick frames.
- **The late camera latch** (Responsive controls): `host/camera_latch.c` wraps the follow camera's
  stick read (Veldin's `func_L01_003E92F8`, Going Commando's `func_L00_00424F98`, every
  single-player level's copy). Between ticks the frontend asks it what the next tick will turn with
  the stick as it is now: the camera's spring (`0x00382BC8`, stiffness 2 and damping 4, constants
  here) stepped from the camera's state at +0x1E0 of its data, times its turn speed. The
  multiplayer maps' cameras are not latched. Right stick held in Veldin
  (`OPENRAC_CAMERA_LATCH_LOG`): each tick turned 0.0502 radians and the latch guessed 0.0502, and
  0.0345 against 0.0346 while the turn eases in. The camera only turns since the levels' vector
  library returns its floats (`hostgen.json` `asm` `signatures`: `func_L01_0040F3F8` and five more
  were taken for int functions, and 212 C callers read 0 from them). Before that, the follow
  camera's pitch and the stick turn came out as 0, and the camera sat level with Ratchet's feet,
  cutting him off at the bottom of the picture.
- **Draw distance and active distance** (Video > Draw distance: Original, 150% and up,
  Unlimited; Active distance): the mobys go through MobyProc's own test (the profile's view block,
  near plane and cap). The ties, shrubs and terrain are culled by the game's records, as Going
  Commando's are: `scenery_matrices` names the words in Veldin's program, relocated to the loaded
  level's (`scenery_matrices_level`). TieProc's cap sits 0x40 past the ties' records' word, not
  0x48 (`tie_cap_after_records`). Veldin reads 1,960 ties, 1,894 shrubs, 802 terrain fragments,
  and caps of 960 (ties) and 500 (shrubs). At Original 1,729 shrubs are left out where Ratchet
  stands, at 400% 1,044 to 1,483, at Unlimited none. Active distance is the shared one
  (`frontend/sleeping_mobys.h`). No class needs the game's own distance yet (Going Commando's
  `draw_distance.tsv`): its three update functions (the Electrolyzer controller, the Arctic Leviathan and
  the Y.E.T.I.) have no counterpart here (best opcode similarity 0.32 across all rac3 levels,
  plans/rac2-to-rac3), and the many zone-gated enemies need a look in the running game first.
- **The occlusion mask**: the 0x80 bytes the terrain's, the ties' and MobyProc's culls read (Veldin's
  0x001EA840, Going Commando's D_001BADC0), at each level's own copy: `openrac_front_end.occlusion`
  is named in Veldin's program and relocated through the level (`scenery_matrices_level`). The
  boot program's D_00227480 is no level's: read there, it was zeros and hid everything. Veldin's
  start: 223 of 1,024 bits, 117 of 802 terrain fragments drawn.
- **Wider pictures** (as Going Commando's, rac2/host/wide_view.c): the game's vertical view kept,
  more of the world to the sides. `host/wide_view.c` runs the five draw callback lists of DrawFrame
  with the view tests opened as far as the picture is wider (the cull matrix at the camera block's
  +0x100, the clip constants at the view context's +0x2C0; game fix `fix_wide_liquids`); each list's
  count and tables are read from the loaded level's copy of its runner's instructions. MobyProc's
  drawn test is opened the same way (`fix_wide_moby_effects`). Each callback's argument is the owner
  of the world quads it draws, and each quad has an id from its callback, owner and order, so a frame
  between ticks moves it. `port/game/rac3/differences.json` lists the fixes.

## What the port answers

| | |
|---|---|
| `libraries.tsv` | 204 library entry points, found by their instructions against rac2-ntsc's and rac1-pal's tables, by shape where the SDK version differs, by RPC function number (libmc), and by the game's own calls (libpad) |
| `host/rac3_version.h` | PAL's switch for the host C, which is both discs': NTSC-U ([rac3-ntsc](../rac3-ntsc/README.md)) compiles the same files with its own names and addresses. An address of the game's memory is written `RAC3_ADDR(0x...u)` (PAL's value); after adding one, or a library row, run `release_carry_rac3.py` (rac3-ntsc's README) |
| `host/entry.c` | the loader's unpacking, crt0, InitSystemCallTableAddress (no kernel to scan) |
| `host/dma.c` | sceDmaSend leaving CHCR as the DMAC does (the VIF1 handler retires the frame's wait mask from it), KickVif0Chain |
| `host/sound.c`, `common/lib/snd.c` | 989snd's client API: native sound and stream requests, with answers sent to callbacks at the next snd_Pump. No shared command ring; unexpected raw sends warn once and return 0 |
| `host/pad.c` | the pad start-up and update of the boot program, frontbin and the levels, the port's: each record's report from the port's controllers, read by the game's own reader, no libpad; the multiplayer executable's libpad 4.0 answers (host/mp.c) |
| `host/movie.c` | the movie player entry, the boot program's, frontbin's and every level's copy (`func_L01_004D28B0`, which the levels' story movies start): the shared native player |
| `host/program.c` | which program is loaded (the overlay hook), `OPENRAC_RAC3_NEXT` |
| `headers/vu0.h` | the vector-library helpers of the decompilation's `include/vu0.h` (asm Clang refuses) in C, read in its place (hostgen.json `headers`) |
| `host/trig.c`, `host/vu0.c` | VU0 cos/sin (the host's for now) and sceVu0UnitMatrix |
| `host/ui2d.c` | the 2D layer's routines, wrapped as Going Commando's (rac2/host/ui2d.c): boxes, glyph quads, turned sprites, ring segments, the font's glyphs (drawn from its distance field) and the menus' Widget3D sprites become window records; the levels' copies of them too (below) |
| `host/mp_ui.c` | the native multiplayer screens over level 39's lobby ([MULTIPLAYER_UI.md](../../../docs/port/MULTIPLAYER_UI.md)): the game lists, the staging room's players and the scoreboard (the port's room's), and the player's own dialog, drawn with the game's own print, colours, highlight bar and sounds; a game list's choice made through the game's own cursor and Cross |
| `host/mp_room.c` | the online room, the port's own (up to 24 players): members from the server's party, each player's name, clan, team, skin and READY, the host's START, the countdown and the results; the host's room writes the settings object the map reads, and answers level 39's slot update and slot searches ([rac3-mp-wide-rooms.md](../../../docs/handoff/notes/rac3-mp-wide-rooms.md) section 9) |
| `host/hud.c` | the level HUD's sprites as records, and its slots out at the picture's sides with "HUD: Edges" (Going Commando's rac2/host/hud.c) |
| `host/chain_census.c` | `OPENRAC_RAC3_CHAINS` (a rough census: it reads image data inside DIRECT as VIF codes) |
| `host/menu_log.c` | `OPENRAC_RAC3_MENU_LOG`: the front end's state and printed strings a frame, for `menus.py` |
| `profile.cpp` | draws the display list the game sends (as rac4-ntsc), the records at their markers in it; the camera, view, fog and live mobys of each program; the levels extracted (`has_levels`), the title drawn without a world (`has_title_world`) |
| `host/render.c` | the game's hand-written renderers the port does not run: each level's DrawFrame passes (terrain, ties, shrubs, sky shells, MobyProc) tell the window what to draw from the extracted level, as Going Commando's (below) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record |
| `host/options.c` | the title's Options: the port's Video page in the 16:9 row's place, the game's 16:9 kept off, the subtitles, the sound mode and the language remembered between runs (below) |
| `host/picture.c` | the main menu's background: the picture to the window as a texture and a full-screen 2D record; in a picture wider than 4:3, its blurred edges out to the window's sides, as Going Commando's title |
| `host/view.c` | the game's own 16:9 option kept off, so that the window's Picture setting alone shapes the view (as Going Commando's) |
| `IDENTIFIED.tsv` | every function and global worked out on the way, for the backfill |

The `rac3_pad_state` ctest compiles the real pad wrapper and shared report code with synthetic RAM
and input. It covers one sample/mod report per ready update, quiet setup and reconnect frames,
disconnects, unopened slots, game callback/reader ordering, and motor forwarding. It does not run
the game or decode input through its retail reader. `rac3_pad_ntsc` carries that real host source
and fixture to NTSC-U's names and checks its boot-only initialization and retained overlay state
([native pad lifecycle](../../../docs/port/NATIVE_PADS.md)). The MP library path remains in use;
full game generation and runtime behavior are unverified.

`rac3_program` and `rac3_program_ntsc` test the real loader dispatch wrapper with synthetic
records. Frontbin's program entry is release-specific metadata, not the boot front-end function's
address ([program lifecycle](../../../docs/port/PROGRAM_LIFECYCLE.md)); the tests do not run
LoadExec or the real function registry.

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

## Signatures of the code from assembly

The functions with no C are translated from their instructions, with the signature hostgen
infers from their registers or their callers' declarations. Where that signature drops a value,
the translation runs on whatever the register held: Up Your Arsenal's vector library uses $v0 as a
temporary, so it was inferred `int` while it returns its result in $f0, and the follow camera's
pitch read 0 from it. `hostgen.json` `asm` `signature_check` (asmc.py) checks every translated
function against its code and fails the build on:

- an argument register its code reads on some path from its entry that its signature does not
  take (liveness over the flow, so a parameter read only past an early return counts);
- a call after which the translated code reads $v0 or $f0 that the callee's signature does not
  return;
- an `asm` `signatures` entry claiming a result its code never sets;
- an argument register handed on unread to a call through a pointer (`jalr`, a widget's vtable):
  its callee is not known, but C fills the argument registers in order, so a call that sets $a2
  takes $a0 and $a1 too, set there or handed on from the entry (`$t0-$t3` say nothing, the code
  keeps temporaries in them; nor does the register holding the call's address). The widgets'
  draws hand their parent rectangle (a1) to the rectangle callback at vtable +0x20 this way: the
  frame widget's draw (`func_003EEBF8`, the slot pages' outlines round each slot's picture and
  info) and the levels' copies of it and of the sprite widget's draw (`func_L01_0051E148`,
  `func_L01_005216A8`) lost it as `int f(a0)`. The rule found two more (`func_003EE840__frontbin`,
  `func_L01_004940C0`) and none that are noise; counting every `jalr` as reading $a0-$a3 instead
  gave 260 findings;
- an argument saved to the frame whose slot's address the code then takes (`addiu rX, $sp, k`,
  the slot in the 16 bytes from k): it is read through that address. libgcc's soft-float double
  routines (`dp-bit.c`, which hostgen skips as the host compiler's) save their doubles so and hand
  them to unpack_d; translated as `int f(void)`, `__muldf3` (func_00128F90) multiplied whatever was
  left in the frame, and the vendor's item strip, which fades each item's "Ammo" label and icon
  with its distance from the cursor through it, printed the label at alpha 0. The nine routines
  (`__adddf3`, `__subdf3`, `__muldf3`, `__divdf3`, `__cmpdf2`, `__floatsidf`, `__fixdfsi`,
  `__fixunsdfsi`, `__truncdfsf2`) are the host's now (libraries.tsv, common/lib/libc.c), as
  `__extendsfdf2` was; the rule then found one more (`func_L01_004D59F8`, a third argument handed
  on to the printf at func_0011AF48).

The same check reads every function with C against its own retail code (`asmc.c_argument_drops`,
the report's `c_argument_drops`): matched C may leave out an argument it only hands on to a call,
whose register keeps the caller's value on the EE, so the bytes match while the translated call
passes nothing. random_vector (`func_L01_00443B88`) names only `out` and calls rand_range with no
arguments, which reads its callers' range in $f12/$f13: crate pieces spun by a quaternion of any
length (#717). `asm` `params_from_code` gives each such definition the argument registers its code
reads (as `params_from_declarations` would, typed by the longest agreeing declaration, else int and
float), so its callers' values reach its short calls; again until none is left, since a definition
that takes more makes its callers that hand those registers on read them too. On the PAL disc the
first scan found 97 definitions (29 of them reading float registers); the rule gives 106 their
parameters with the ones it finds through them, none is left, and the zero-filled call arguments
went from 1,041 to 853. Many reads come from a callee whose prototype the declarations widened
(the GUI table builders, `func_L01_004903F0` and its twins, call `func_L01_00518D40`, which takes
two integers and two floats, with one): the registers then hand on whatever the caller held, as
on the EE, where before they were 0. `tools/hostgen/dropped_args.py <gen>` ranks
what is left: a call whose callee's code reads the register the C leaves out is a value the caller
held for something else (a leftover); the rest are prototypes wider than their code.
`asm` `stack_arguments` counts the integer arguments past the eighth too, which the caller passes
in its frame and the callee loads above its own (the levels' copy of frontbin's func_003910C0,
`func_L01_00412C08`, reads a ninth: #719 gave it its signature by hand), in the signatures of the
translated functions, the check and `params_from_code`.

The game's sprintf (newlib's, 0x0011B2E8) is the port's (`host/print.c`), a C variadic function.
Code from assembly and C that calls it through its address (the vendor's cost and bolt total,
`func_L01_004BEC88`) goes by `openrac_guest_call_regs`, whose cast hands the EE's registers on as
fixed arguments: on Apple's arm64 a variadic function reads its unnamed arguments from the stack,
so the vendor printed "0,000" for 15,000 and 1970004 for 0 bolts on the Mac (the Linux boxes, which
pass them in registers, were right). `hostgen.json` `variadic_thunks` gives the variadic functions
their thunks on every platform (`functions.c`), as Windows has for all of them.

The levels' copy of the picture TEX0 getter (`func_L01_0042D898`, boot func_0039B088's `unsigned
long long`) lost its upper word as an `int`, so the in-level SAVE page drew its slot pictures and
empty-slot emblems from no texture; it is the only level copy whose result is narrower than its
boot twin's.

Each finding gets the function's real signature in `asm` `signatures`, checked against its
instructions. A function whose C says `void` but hands its callee's result back is translated from
its instructions instead (`from_assembly`). An exception goes to `asm` `signature_allow` with
its reason. The first run found 1,003 findings in 678 functions, across boot, frontbin and the
49 level programs, and 39 C callers reading zeroed results; both are now 0. The C callers' side is
hostgen's `result_allow` guard (`checks/result_allow.tsv`, empty).

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

The game asks for 512 x 416 (the NTSC-U frame's height) in PAL's 448-line frame, which the console
stretches to its 512-line display: the frame's last 32 lines stay its clear, black, a band along
the bottom of the main menu on the console too (rows 416 to 447 of the buffer are zeros). The port
draws it so, in every Picture shape.

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
The port's texts there (the Video row's name, its page's rows) are in the game's language, as
Going Commando's: the front end's `language` (game_host.h, `options_menu.cpp`'s translations) is a
word the port keeps with the language byte's value each frame (`host/options.c`), since the game
keeps it as a byte between others. Their accented letters are written in this game's font's code
(0x80 and the letter's number, not Going Commando's single bytes: `capitals`).

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
corner; the enhanced renderer's bloom (`port/enhanced/docs/bloom.md`) is the port's glow. A second
effect of the same family (`0x004DF310`, while `0x001DBCBC` is set: the frame blurred twice, tinted
and added back) is left out the same way.

Every level's program carries its own copy of the engine, at its own addresses: the hooks are
written with Veldin's (`func_L01_...`, the names hostgen gives the catalogue's places) and its
globals go through `OPENRAC_LDATA(1, ...)` to the loaded level's. The multiplayer maps (39 to 53)
share most of them, but have their own DrawTfrag, tie and shrub passes and MobyProc, written with
level 39's (`func_L39_...`, `OPENRAC_LDATA(39, ...)`). `IDENTIFIED.tsv` has each function and its
Going Commando pair.

The extracted levels are the shared export (`extractor/extract.py port`), format 14 for this game
(`extractor/port.py` `GAME_FORMATS`): terrain, ties, shrubs, mobys with skeletons and animations, the
sky's shells and its panorama, collision, (5) Ratchet's mesh and textures from his first pack
(`extractor/wads.py` `RAC3_PAL`, the table LoadPlayerDisplayTextures reads), which the level's class 0
draws him with, and (6) the terrain and shrubs lit as the game lights them at load and the ties'
colours per vertex, as Going Commando's (`games.py` `world_lighting`, `UYA_TIE`): the lighting
routines are Going Commando's instructions, and the (cos, sin) table they light by is in each
level's own program, not the loader the disc boots (`lighting.program_normal_table`). Against Going
Commando's format 20 it has every input of the enhanced renderer's bake.
(7) adds his wrench (pack 31, class 71), the banks of what he holds and his
ship's three hulls and 32 paint jobs (class 3412), where Veldin's copies of Going Commando's
loaders read them (`wads.RAC3_PAL`). (8) adds the chrome and glass maps the metal packets are drawn
with, from the GS RAM lump's stash as Going Commando's (`games.py` `maps_in_textures`: the core
index's offsets are its, 0 and 0x4000, CLUTs 0 and 0x400). (9) adds the mobys' glow vertices, their
light sets and their bangles as classes of their own (`moby_glow_tail`, `moby_lights`,
`moby_bangles`): the profile draws a moby's bangles by its bangle word (+0x7C), as Going
Commando's. (10) adds his other 30 looks, packs 1 to 30, as classes 6001 to 6030: the profile
draws class 0 as the look of the armour at `0x001D4B60`, or of the skin byte at `0x001427AB` when it
is set, as LoadPlayerDisplayTextures picks the pack. In Inferno Mode the game draws him as the level's class 0x1F3A
(Infernox), whose class data hero_mesh_swap copies over his slot; the profile draws that class while
his slot holds its packet table. (11) adds the gameplay copy the probe baker
plans enhanced lighting from (`bake-inputs/gameplay.bin`), as Going Commando's 18. (12) adds the
reviewed glowing textures and the levels' sun sets, as Going Commando's 19. (13) adds level 97, the loading
screen's ship scene (below). (14) adds the liquid surfaces the baker lights from
(`bake-inputs/liquids.json`), as Going Commando's 20. (17) adds level 97's sky, the loading screen's
stars and galaxy (below). (18) marks the tie materials the game blends opaque whatever their texels'
alpha (ALPHA_1 0x20, Cs; glTF extras `"opaque"`, `extractor/ties.py`), which the window draws solid
where a picture below full alpha would make a material part-transparent: the Galactic Rangers'
dropship on Veldin is three ties (6759, 6760 and 6762, moved with it by the game and drawn with the
mobys) whose hull pictures are at alpha 0x0B to 0x1E, and it was see-through.

## The loading screen

Between levels the boot program runs its loading screen (mode 6, the pass `0x003C3BB8`): its draw,
`0x003BCEB0`, draws the scene's sky (the stars and the galaxy), then the scene's mobys through MobyProc's draw
side, `0x003CB6B8`, which walks the jobs the boot program's MobyProc queued. Those mobys are the
boot program's table's: Ratchet's ship (class 3412) and its parts (4308 to 4325) and a planet
(7683), whose classes are the scene's pack's: one of the loading packs the table of contents lists
from `+0x1288` (after their base sector), pack 11 the ship's scene, which the loading screen unpacks
at `0x006F3000` (picked at random among the scenes by `0x003BE0F0`). The export reads it as a small
core (`extractor/level.py` `load_wad_loading`: GS RAM at word 1, data base at word 2, the GS upload at
4 and 5, the classes at 0xa and 0xc, the moby textures at 0xe and 0xf, the texture data at 0x17)
and writes it as level 97, with Ratchet's pack. The profile names that level the loading screen's
world (`loading_world`), and `0x003CB6B8` puts the world's and the mobys' markers in the list, so the
window draws those mobys natively with the boot program's camera.

The stars and the galaxy are a sky of the scene's own: the pack's word 0 holds the parts it has
(`0x003BBDC0` keeps it at `0x001D8CD4`), and with bit 0 its word 9 is a sky, at the data base itself
on both discs, which LoadSky's copy (`0x00398A48`) makes the boot program's (`0x001DA5F0`): three
shells, the star field (untextured), a 256 x 256 and the 512 x 512 galaxy picture. SkyDraw's copy
(`0x003CE568`, under `0x003BD108`) turns each shell into `0x001DA610` (`0x003CE450`) and draws it with
SkyDrawShell's copy (`0x003CE798`). Its triangles went out as the game's GS packets, which the window
drew in the 4:3 middle while the ship and its trail filled a wide window. The export writes that sky
as level 97's (`Level.sky_at`, the index's `+0x10` cannot name offset 0) and `host/render.c` draws
`0x003CE798` natively, as the levels' `0x004A8158`: around the camera with the window's view, so a
wide window shows more of the sky. The levels' flights (below) run the levels' SkyDrawShell copy,
already native, with the same sky, which their LoadSky copy took. While the game reads the disc it resends a list that
ends before that pass's markers: such a list is drawn whole under the world (`frame.cpp`), else
its clear covered the ship.

Leaving a planet, the flight is the level's own: each story level's program carries a copy of the
loading screen (the catalogue's frontbin rows `func_003BD4B8`, the loop, and `func_003BD030`,
level_load; Veldin's `0x004D7AE0` and `0x004D7658`), which runs before the next program is copied
in. level_load sets level_current (`0x001D545C`) to the level asked for as the loop starts, so
`host/tools.c` (`openrac_game_flight`) names the loading world while it differs from the level
loaded: the window draws level 97 then too, not the planet being left (whose sky, and terrain, the
flight's DrawTfrag copy marks, were drawn round the ship). The boot program's wrap of the hold is
at its own addresses; the levels' copy of the ship's glass is wrapped too (`func_L01_004B04D8`, the
catalogue's frontbin row `func_003A7AF8`), and their planet quads go through the levels'
DrawWorldQuad, `func_L01_00407E28`.

The loading screen is the game's table row for the trip (level_load, `func_003BD030`'s rules: the
Phoenix to a Vid-comic level and back is row 7, `value` 6 at `0x0016C580 + 0x2E`): a full-screen
picture and two turning sprites, with no moby pass for the world to split at. `host/tools.c`
(`openrac_game_loading_draws_world`) says so, and the window draws that 2D over the loading world's
clear instead of under it (it was black for the whole load). A trip no rule names (a level that is
not the Phoenix asking for a Vid-comic one, or one asking for itself) takes the default row 8, which
the game draws black.

Between ticks the ship and the flight's camera move fast: in the approach (its ticks stretched
2.5 times, `host/loading.c`) more than the 4 units a moby (8 for the camera) may move in a tick and
still be drawn part way (`frontend/geometry.cpp`), so they were drawn where each tick left them,
about 20 times a second. A moby or a camera that keeps its pace (this tick's move within that step
of the last tick's) is now drawn part way at any speed (`Kept::prev_paces`, `camera_pace`).

Measured in a run (`OPENRAC_MOTION_LOG`, docs/port/DEBUGGING.md), the ship, its glass and the eye
were drawn part way through the approach, but the exhaust was not: the engine flames (`0x003A6B90`,
two rows of world quads laid along the engines from their joints each tick) and the trail (`0x003A8850`,
a pool of 512 puffs at `*0x001D6EEC` that drift, grow and fade) went out through DrawWorldQuad with
no owner and no id, up to about 135 quads a tick, so between ticks they stood where the tick left
them while the hull moved on: at the approach's 20 ticks a second the exhaust stepped behind the
smoothly moving ship. The flames are now owned by the ship (as the glass, levels' copy
`func_L01_004AD768` too) and each puff carries its record's place as its id (`0x003D98C0`'s wrap), so
frames between ticks move both. The sky's shells, which turn about 0.04 degrees a tick there, are
turned part way too (`Kept::prev_sky_shells`).

Not drawn yet: the planet (class 7683, a picture on a quad the ship flies towards, past its draw
distance until the scene's last part, which the port's quicker loading never reaches) and the
level's name ("Veldin" at `0x002D2018`, which `0x003C1890` prints through FontPrint fading in with
the timer at `0x00225734` once `0x002D2008 + 0x51` is set; no string is printed in our runs).

To look at a level without the game (no game code runs; the viewer's own camera):

```sh
python3 extractor/extract.py port <disc> build/rac3pal --level 1 --anywhere
openrac-viewer build/rac3pal/level_01 --camera 365,103,82,90,-5 --screenshot veldin.png --frames 5
```

What a level's picture still lacks (Veldin, 2026-11-20, hidden runs on box2 with
`OPENRAC_RAC3_NEXT=60:1`): the sky, terrain, ties, shrubs, fog and the live mobys (Ratchet posed
and animated) draw natively with the game's camera, but

- Obani's lava (level 11): its picture is black at the moment (the profile's camera reads a zero
  forward axis there), so it is not seen yet. Its lava is the level's own terrain and ties
  (`terrain_0020`, `terrain_0023`, `tie_0109`), which the native world draws, and the enhanced
  catalog lights it.

Water and other liquids reach the window as Going Commando's do. The game's own draw callbacks
build them as effect strips, and the strips sent before the world's mobys are drawn with the liquids,
after the ties (`renderer/effects`). Up Your Arsenal's surfaces are mobys of class 6576, whose draw
(level 14's `0x003B24C8`, in the draw callback list run after the ties) strips a path of the
gameplay file (`IDENTIFIED.tsv`). Level 14's square pool draws so: the 23 points of path 121 make
its strips (115 vertices in the frame's five liquid strips: ALPHA 0x44 TEST 0x5310B, as Dobbo's
water, and layers blended over it, ALPHA 0x58 and 0x6A), hidden runs on box1 at 84b906a67 with
`--level 14`.

The open water of Florana, Marcadia, Daxx and nine more levels (rivers and pools at one height, the
plane class 2871 on Florana, `func_L02_0030FA80`) is not a strip of the game's: its draw callback
(`func_L02_00310838`, DrawFrame's third list) sends a VU1 program of its own (microprogram 10, the
boot program's `0x00116070`) a grid's parameters, and the program draws it. `host/water_plane.c` is
that callback, the grid and the program's per-vertex arithmetic in C (read from the program, checked
vertex for vertex against it with synthetic parameters, offline), drawn through the liquids' strip
routine (Veldin's `0x00408AAC` after its setup `0x00407588`):

- the grid follows the view, what is on it does not: its first row is the middle of the line where
  the view's bottom edge meets the plane (a bottom corner by the camera's roll, run along the view's
  horizontal right axis to the other side plane), then 50 rows a unit (`0x001DC568`) apart along the
  view's horizontal forward axis, 50 two apart and 5 two hundred apart; 33 points either side of a
  row's centre, the step the bottom line's length over 64, widened row by row as the view widens;
- the height is the plane (pvar +0x4C) plus the eight waves (`0x0026EB80`, `func_L02_0030FC68`):
  waves 4 to 7 fade over the first 50 rows, 0 to 3 over the next 50, the last 5 rows are flat;
- the surface (effect texture +0x00, none on Florana) is mapped in the world, one repeat every
  +0x24 units, scrolled +0x2C units a second towards +0x28 degrees, coloured by +0x30..+0x32 shaded
  by the waves' slope along the view (`0x001E00D4`), blended by FIX +0x33, writing depth;
- the reflection (effect texture +0x04, Florana's 100) is the view's ray reflected by the waves'
  normal, read from a paraboloid map with the zenith at ST (0, 0) (S, T, Q = R.x, R.y,
  2(|R| + R.z)): flat water reads the zenith everywhere and only the waves move it, so nothing swims
  with the camera; coloured by +0x34..+0x36 and blended by FIX +0x37 (opaque on Florana);
- both are fogged by depth towards FOGCOL +0x3A..+0x3C, from +0x3D percent at +0x40 units to +0x3E
  percent at +0x44 (`func_L02_00310368`), not the level's fog: the strip program's fog line (VU
  `0x3AC`..`0x3AF`) is uploaded with the water's for its strips and put back after them.

The program's mipmapped surface texture (TEX1 MMIN 5, MXL 6) gets a mip chain the window makes from the
picture (`TexturePool::ensure_mipmaps`, asked for by the effect strip's TEX1: `Tex1::mipmapped`), so
distant water does not shimmer; the reflection and the UI's and quads' effect textures keep one level.
The levels with such water (class
2871, one plane each but Tyhrranosis's two): 2, 4, 5, 9, 12, 14, 17, 18, 19, 20, 22 and 29.

### Shadows, enhanced graphics and texture packs

The mobys' shadows are Going Commando's shadow volumes (`profile.cpp`): MobyProc (Veldin's
`0x00441B78`) copies the four directions the window casts them along from the loaded level's copy of
Going Commando's `0x1B1B90` (Veldin's `0x001E0070`, the multiplayer maps' `0x002935A0`), and every
moby with mode `0x400` and a slab across its up axis (`+0x70`, `+0x74`) casts one, Ratchet with
them.

Enhanced graphics (probe GI, sun cascades, his shadow map, GTAO, AgX, volumetrics) draw this game as
Going Commando's (`docs/port/RAC2_NOTES.md`, Enhanced graphics): format 11 keeps the gameplay copy
the baker plans from (`bake-inputs/gameplay.bin`, tag `rac3-v1`; the gameplay header is Going
Commando's, `assets/world/gameplay.cpp`), and local grids grow from Ratchet, the vendor and the
crates (`port/enhanced/lighting/profiles/rac3`, the same class numbers as Going Commando's).

```sh
port/build/enhanced/openrac-enhanced-baker --bake-all --levels-dir <levels> --output-dir <data>/probes/pal --jobs 0
OPENRAC_GL_API=vulkan OPENRAC_ENHANCED=1 port/build/enhanced/openrac-rac3-pal --data <data> \
  --levels <levels> --probes <data>/probes/pal --window
```

From format 12 the export resolves this game's catalog of glowing textures
(`profiles/rac3-pal/material-overrides.json`, 36 textures: lamps and light strips, screens and panels,
a lit window, and Obani's lava) into each level's `probe-emission.json`, picked from the 192
brightest saturated world textures of the 34 single-player levels on contact sheets. Four levels bake
their sun from the directional set most of their ties and shrubs take (`light_set` in
`profiles/rac3-pal/level_NN/probe-bake.json`: 6, 7, 8 and 10; set 15, which levels 3 and 30 use most,
is empty). The launcher's Enhanced graphics entry and its Prepare the enhanced lighting action
(`probe-bake`) take this disc, and its Build the game makes the baker. From format 14 the export
keeps the liquid surfaces (`bake-inputs/liquids.json`, `rac3-strips-v1`): each moby of class 6576
draws one triangle strip through a path of the gameplay file (the paths are its header word 0x78), its
first layer a white effect texture under the pvar's colour, so the surfaces the review
(`profiles/rac3-pal/liquids.json`) lists by that colour word are lit as emitters, as Going Commando's
lava. None is listed yet. Florana's two orange surfaces (`280c0080`) are seen in game (hidden runs
on the Mac, hero teleported beside them): a dark red mud that kills, which gives off no light.
Marcadia's blue pool (`0f141660`) draws as water; its orange ones (`10080280`) and those of levels 7
and 34 are not seen yet, and colour alone is not taken as emission. Texture
packs (`docs/port/TEXTURE_PACKS.md`) work as in the other games: the
pictures in `<data>/texture_packs/rac3-pal/` replace the world's, the mobys', the sky's and the
interface's textures.

## The hero in a level

`OPENRAC_RAC3_HERO=PATH` (`host/hero_log.c`) writes a line for each frame of a level in which the
hero's state words changed: the level's mode, his state and its type (rac3-decomp
`enums/hero_state.h`, Going Commando's numbering), health, the combo's swing, the item in his hand,
the gun Circle brings out and their rounds, the items owned, bolts, area, deaths, the last
checkpoint and the level's progress flags, then his position. `OPENRAC_RAC3_MOBYS=FRAME,...` adds
the live mobys within 60 units and the level's spawn points. `host/tools.c` gives the hero to
`OPENRAC_WALK` and `OPENRAC_RECORD`, and `OPENRAC_TELEPORT` and `OPENRAC_INVINCIBLE` work.
The words are in IDENTIFIED.tsv.

Seen on Veldin (New Game, or `OPENRAC_RAC3_NEXT=60:1`), scripted with `OPENRAC_PRESS`:
the arrival (state 116), the opening walk (101, 102, 104), control; run (2); jump (7) and Cross
again in the air, the double jump (14), the heli-pack's glide (8); the wrench combo (19, swings 0,
1, 2); a gun drawn with Circle (held item 47, the N60) firing a round every 12 frames while Circle
is held, its rounds going down one a shot; a Tyhrranoid's blows (class 5821) taking one health each
(state 22) down to 0, then death (57) and the respawn at full health; a fall from the cliffs
(state 121), the death counted and the respawn at the level's start. A new game owns no gun on
Veldin; the bolts of the crates are picked up.

## The level's 2D: the HUD, its text and the pause menu

Every level's program carries its own copy of the 2D layer (rac3-decomp's catalogue folds them with
frontbin's; hostgen names them after Veldin's places): the boxes, glyph quads, turned sprites, ring
segments, the font's glyph builders and prints, the darkening and the Widget3D sprites, the same
instructions as the boot program's, writing to the level's display list (Veldin's cursor
`0x001E1068`). They are wrapped as the boot program's are (`host/ui2d.c`, through the same code), so
the HUD, the help and tutorial messages, the subtitles, the names of what is picked up and every
page of the pause menu are window records, their text drawn from the font's distance field. Their
prototypes are the assembly's (hostgen.json `asm` `signatures`): the catalogue's frontbin rows have
only `INCLUDE_ASM`, and a level place hostgen renames from them has no C of its own: translated as
an `int` function, a 64-bit or float result is cut, and an argument the code hands on in a register
it does not name is lost (as the boot program's Widget3D pose did, 409d2795). Two did that to every
text of a level: the TEX0 the prints draw their glyphs with (`func_L01_004047A8`, boot
func_0038A0E0's `unsigned long`) lost its upper word, so every glyph drew blank (the help message's
orange box over Ratchet, empty), and the text widget's draw (`func_L01_0051F940`, boot
func_003F03F0's `int (int, int)`) lost the second argument its rectangle callback reads, so the pause
menu's rows were placed at x = -132509. The signatures of the frontbin rows whose C (or the boot
program's twin's, `config/frontend_pairs.tsv`) returns 64 bits or a float are given the same way.

The HUD is Going Commando's (`host/hud.c`): its 13 slots (0x90 bytes, Veldin's at `0x002E60D0`, each
drawn by its callback from the update `func_L01_0041FE80`, Going Commando's func_0028E458 to the
instruction), whose places are anchored to the screen's sides by the same flags, and its sprites
(func_0028E868, func_0028ECD8, func_0028F078 and the gradient func_0028EA58 there). With the
player's **HUD: Edges** the slots anchored left or right are drawn out at the sides of a picture
wider than 4:3 (`game/common/lib/hud_edges.h`, as Ratchet & Clank's and Going Commando's); centred
ones stay. The weapon, ammunition and bolts panels are a tree of widgets as Going Commando's
(`func_L01_004D24C0`, its func_0034E4F8), but drawn, with every other screen of the GUI (the help
messages, the pause menu), by `func_L01_00478B10` once a frame: Going Commando's two side markers go
around that draw, so the bolts' panel goes out to the right while the weapon's panel in the top
middle, the messages and the pause menu stay.

The pause menu draws over the world it paused on, as Going Commando's (rac2/host/picture.c): when
it opens the game reads the frame buffer back to main memory in 64 x 64 tiles
(`func_L01_004C5F70`, func_002FC8D0's instructions) and each frame writes them back as image
transfers (`func_L01_004C6030`, func_002FC990's) before darkening them and drawing the menu. The
window keeps the frame it drew instead (whole, at its own resolution and shape) and a marker draws
it where the transfers were (`host/picture.c`): nothing of the game's transfers is sent.

## What is missing

- **The level's 2D still sent as the game's packets**: the screen fade DrawFrame draws while its
  counters run (`func_L01_004054F0`, boot func_0038AD28), `func_L01_00407588` (boot func_0038CB30)
  and `func_L01_00415788` (boot func_00392B28), the screen glow's copies and the second screen
  effect (above); `OPENRAC_UI_RESIDUAL=1` lists their kinds. The HUD textures' uploads
  (`func_L01_004045B8`, `func_L01_004046F0`) are uploads, not draws.
- **Holostar's blue box** (levels 13 and 14, while Ratchet is down): a solid dark blue rectangle of
  about 256 x 224 in the middle of the picture. Not the level's picture routine
  (`func_L01_004C61A0`, wrapped as the boot program's, never called there) nor any wrapped 2D
  routine; the residual kinds there include a sprite textured from the frame buffer (block 0x1E00).
- **The bolts count** is printed (its text widget, "0" at the start) but does not show in its panel,
  with the game's own packets too (`OPENRAC_UI_REPLAY=1`): not yet found.
- **Other levels' HUD**: the wrappers are written with Veldin's places, and every level's copy that
  the catalogue folds with them runs through them; levels whose HUD has routines of its own (the
  multiplayer maps' and the arena's) are not checked.
- **The pause menu's Options** have no Video row (Going Commando's copy of its Camera page,
  rac2/host/options.c): the port's settings window (the menu key) has the same rows in a level.
- **The mods**: no hook points, script binding or mods' pages in rac3's menus yet (Going
  Commando's rac2/host/mod_pages.c, sdk/bindings); so no save states either, which the SDK gives
  only to Ratchet & Clank's mods.
- **Music in the boot front end**: the menu's sound effects play (the front end's bank, an SBlk
  version 3 block, which the shared bank reader reads), but the boot front end asks for no music:
  its stream table (the level table LoadLevelToc fills, `0x00160C40 + 0x7A04`) is never loaded and
  the music channel stays at track -1. A level's music plays (two-part VAG streams, command 0x2C).
  Command 0x30 (a stream query whose answer the level's stream code stores at its record's +0x14,
  `0x00460170`, which nothing in Veldin's stream code reads) is not known to the ring and answers 0.
  `OPENRAC_AUDIO_TICKED=1 OPENRAC_AUDIO_DUMP=out.wav OPENRAC_SOUND_LOG=1` captures and logs it.
- **OPENRAC_GIVE** gives the twenty guns (`all`, `weapons`), an item by its number and `bolts:N`;
  the gadgets' item numbers are not known yet, so `gadgets` means nothing here.
- **Touch hints**: the on-screen pad follows the level's mode and the hero's state
  (`host/touch.c`), but the game's "press triangle" prompts are not read.
- **The multiplayer maps** (levels 40 to 53) run their menus but not their maps.
