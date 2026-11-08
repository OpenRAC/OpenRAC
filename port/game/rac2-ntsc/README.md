# Going Commando NTSC-U native start-up

`openrac-rac2-ntsc` boots Ratchet & Clank: Going Commando (SCUS-97268) from
its decompilation to the front end's main menu and works it: the splash and
the memory card check, the intro movies (skipped with `--skip-movies`), the
menu's three buttons with their labels (New Game, Load, Options), Up and Down
moving the selection, Cross opening an entry and Triangle backing out. Each
entry's page draws: Options (16:9, Progressive Scan, Vibration, Subtitles,
navigable), Load (with no card: "Please insert a Memory Card ... Go back?")
and New Game (with no card: "No Memory Card (PS2) inserted ... Continue
anyway? Yes / No"). Yes there starts the game: the first level's load is
where it stops (below).

A run that does all of it, hidden, with the frames on which the pad is
pressed (Cross 0x4000, Start 0x8, Down 0x40, Up 0x10, Triangle 0x1000):

```sh
OPENRAC_HIDDEN=1 OPENRAC_PRESS=300:4000:10,500:8:10,700:4000:10,900:8:10,1300:40:8,1340:40:8,1400:4000:8,1500:40:8,1600:1000:8,1700:10:8,1760:4000:8,2000:1000:8 \
  port/build/release/openrac-rac2-ntsc --data build/rac2-data --window --frames 2200 \
  --no-card --skip-movies --stop-on-missing
```

## Building and running

The default build reads OpenRAC's own `games/rac2/decomp` (synced at
2e09b830, 2026-11-01). It needs the boot executable, which
`python3 tools/build_port.py rac2-ntsc --disc <your disc image>` has hostgen read
straight from the image (`OPENRAC_RAC2_NTSC_RETAIL_IMAGE`; no copy is made).
Without `--disc` the build falls back to `games/rac2/decomp/baserom/SCUS_972.68`
(for developers: `python3 tools/levels.py dump` writes it from
`baserom/SCUS_972.68.iso`, or copy it from your rac2-decomp's `baserom/`); with
neither, the functions with no C (the boot code, `main` included) are stubs and the
game exits at once. `OPENRAC_RAC2_NTSC_SOURCE`
points it at another checkout instead (your rac2-decomp, further along):

```sh
cd port
cmake --preset release
cmake --build --preset release -j 8 --target openrac-rac2-ntsc
python3 tools/extractor.py <SCUS_972.68 image> --game rac2 --proj-path build/rac2-data -e --boot-only   # from the top
OPENRAC_HIDDEN=1 OPENRAC_PRESS=300:4000:10,500:8:10,700:4000:10,900:8:10 \
  port/build/release/openrac-rac2-ntsc --data build/rac2-data --window --frames 1500 \
  --no-card --skip-movies --stop-on-missing
```

From a checkout's launcher (no Developer tools needed): set the game up from
your disc, then Play (`launcher/actions.json`, rac2/ntsc's `play`:
`--data {data} --window --levels {root}/assets/port/rac2/ntsc`, after a warning
to accept). Play first reads the levels (Get the levels ready, `editor/extract.py
port`) when their stamp, `levels-<format>.json` with the format of
`editor/port.py` `GAME_FORMATS`, is not there, so a format bump reads them again;
the game has no `--check-levels` yet, so that stamp is all Play waits for. An
installed launcher reads and passes `<data>/port/ntsc` instead. Its Build the game runs
`tools/build_port.py rac2-ntsc --disc {disc}`. Play is a player's, with a warning first, and so
are Build the game and Rebuild: Play on a fresh checkout is Build and play. An installed launcher on
Linux or macOS carries no `openrac-rac2-ntsc` (its build needs the player's disc, which CI has not):
its Build and play runs `tools/build_game.py`, which fetches the source and builds it with the
player's compiler. A Windows installer carries the game built without the disc, as Android's does
(below): the player needs no compiler.

On Android the APK (`tools/build_android.py`, CI's too) carries the game built
without the disc: the functions still in assembly are called through slots
(`asm_interface.json`, `asmext.py`). Once the game is set up from the disc, Build
and play first runs `tools/asm_part.py build`. The APK's Python translates those
functions from the disc (`asmdev.py`, from the pickled program the build wrote),
and the Clang and lld it carries (Zig's, `libopenrac-zig.so`)
compile them into `<data>/asm-part/<tag>/libopenrac-rac2-ntsc-part.so`. The tag
names the kit and tools it is built from, so another APK builds its own. The game
loads it before it starts (`--asm-part`). It takes a few minutes, once for each
APK whose game changed; nothing of the disc is in the APK.

A Windows installer does the same with `tools/part-zig/zig.exe` (Zig's Windows
release, CI's `launcher.yml`), into `libopenrac-rac2-ntsc-part.dll`. The part is
compiled with the free headers in Zig's release and links no C library: a DLL
imports only from the program, which on Windows exports just what
`asm_part_exports.def` names.

After a sync that changes the asm stage's view of the code, write
`asm_interface.json` again with the disc (`hostgen.py ... --retail-image <iso>
--asm-interface-out port/game/rac2-ntsc/asm_interface.json`); a build without the
disc then gives the same C as one with it. Write `asm_part_exports.def` again too:
`tools/asm_part.py kit rac2-ntsc --build <build> --out <kit> --windows-zig <zig>`,
then `tools/asm_part.py exports rac2-ntsc --disc <iso> --kit <kit> --out <work>`
(names only; a name the program no longer has stops its Windows link, and a name
the part newly needs stops the part's).

For Going Commando `OPENRAC_RAC2_NTSC_RETAIL_IMAGE` gives only the boot
executable: hostgen's `retail.read_image` uses the editor's reader
(`editor/disc.py`, Ratchet & Clank's discs) when the image is one and otherwise
reads SYSTEM.CNF's BOOT2 file through `tools/openrac_common/iso9660.py`, with no
level programs.

What the translation reads (`hostgen.json`):

- `symbol_addrs/usa/symbol_addrs.txt`, and the values the decompilation's
  own build derived (`derived`, as aliases and as symbols) for the names it
  carries from rac1-decomp (`r1_func_...`, `r1_D_...`) and for its
  family-qualified ones (`F<hash>_FUN_<addr>`): its `build/boot/placed.json`
  when the checkout has been built, else `derived_names.json` here, the same
  table kept with the port (names and addresses only). Without either,
  hostgen stops: the functions that use those names would be stubs and the
  menu's set-up crashes. After syncing `games/rac2/decomp`, build its boot
  program and write the copy again (hostgen warns, and `test_derived` fails,
  when the C uses a name the copy does not know):

  ```sh
  python3 port/tools/hostgen/derived.py --game port/game/rac2-ntsc/hostgen.json --source <built rac2-decomp at the synced commit>
  ```
- `baserom/SCUS_972.68` and `baserom/levels` (`asm.retail`): every function
  still in assembly is translated from the retail executable's and the level
  programs' instructions, entries found by their calls (`discover_entries`). Its `include_asm.h` defines the markers
  to nothing, so `defines` sets `INCLUDE_ASM_H` and gives `ASM_FUNC` the
  marker hostgen reads.
- `skip`: the other releases' C (`src/pal`, `src/us2`); libgcc's `__udivdi3`,
  `__umoddi3` (host rows), fp-bit's `__pack_f` file, whose `mode` typedefs Clang
  reads but hostgen cannot prototype, and libgcc's other units, whose macros and
  `__builtin_setjmp` it cannot read. All translated from assembly instead.
- `from_assembly`: 15 wrappers whose C omits registers they forward
  (`arguments_filled_with_zero` in the report), translated from assembly.

## The port's own code (`port/game/rac2/host/`)

Shared by the three discs' programs ([rac2](../rac2/README.md)): this folder's `host/rac2_version.h`
renames nothing, PAL's and v2.00's give their names and addresses.

| File | Replaces |
|---|---|
| `font.c` | the font prints (`0x0027F538` and nine siblings) and the GUI text element's draw (`0x00336A78`), passing the font's TEX0 from `0x0027C878` whole, as rac1's port does for FontPrintWindow (`rac1/host/hand.c`) |
| `spr_dma.c` | the scratchpad's DMA channels (toSPR, fromSPR) the hand-written kernels start and poll (`CollLine`, `0x002761F8`, and about twenty more): `asm.dma_stores` sends their word stores here, and the transfer is done at once |
| `level.c` | wraps ParseBin (`0x00131CB8`): sets the per-level relocation once and says which program is loaded (`overlay_hook`), as `rac1/host/boot.c` does |
| `entry.c` | crt0 (`0x00131AE8`): `_InitSys`, `FlushCache(0)`, `main(argc, argv)` with the block at `0x00146B40`, without the thread and heap system calls; `_InitSys`'s scan of kernel memory (`0x0011F718`) and its Deci2 console thread (`0x0011B800`) |
| `dma.c` | `sceDmaSend` (`0x00127220`), leaving CHCR as after an end tag so the game's VIF1 handler (`0x002FCE18`) retires the whole frame wait mask (`0x001AD520`); `KickVif0Chain` (`0x002FC070`) |
| `io.c` | `sceOpen` (`0x0011E0D8`), declared variadic |
| `movie.c` | `PlayFmvMovie` (`0x0034EF58`: first sector, size, ...): the shared native player |
| `print.c` | `printf`, `sprintf`, `kprintf`, `scePrintf`: formatted by the port |
| `sound.c` | `snd_SendCommandSync` (`0x00132C48`) and `snd_QueueCommandToRing` (`0x00132DF0`): every command, as all wrappers end there; a set-parameters reply of 0 is what frees a finished voice's slot, and music is two mono VAG files, right and left |
| `trig.c` | the VU0 sine and cosine (`0x00283198`, `0x00283180`), from the microprogram the game loads at `0xC80`: where its constants are, for the shared body ([common/lib/vu0_sine.h](../common/lib/vu0_sine.h)) |
| `ui2d.c` | the 2D layer's draw routines (font prints, glyph quads, HUD sprites), wrapped as rac1's `ui2d.c`: their quads go to the window as records, glyphs drawn from the font's distance field at the frame's resolution ([common/lib/ui_quad.h](../common/lib/ui_quad.h)) |
| `hud.c` | the HUD in a wider picture with "HUD: Edges": the 13 slots ([common/lib/hud_edges.h](../common/lib/hud_edges.h)) and the widget HUD (`0x0034E4F8`) moved out by `kHudSideMarker` |
| `wide_view.c` | the five draw callback lists and the sphere test (`0x00283DB8`) against the wider picture ([common/lib/wide_view_lists.h](../common/lib/wide_view_lists.h)), game fix `fix_wide_liquids` |
| `options.c` | the game's own 16:9 (`0x001A7BB1`) kept off: the window's Picture setting gives the shape |
| `wad.c` | the WAD decompressor (`0x0029D4D0`), through the shared body ([common/lib/wad_routine.h](../common/lib/wad_routine.h)) |

`libraries.tsv` binds 164 entry points to the shared replacements: the
names in the symbol table, the kernel's system-call wrappers by the number
each loads into `$v1`, and the SDK functions whose instructions (relocations
masked) are those of rac1-pal's or rac4's bound ones. Several of the latter
start four bytes after their catalogue place, at the linker's fill word.

## Level hooks

`level_hooks`: 31 empty functions of the executable from `0x0026F710` to `0x0026F808`. Each level's
program fills them with hero code of its own, which the engine calls from the code it carries in
every level: state changes, the update in a scene. Rac1 has the same pattern. Assembly calls one
with all its registers (`level_hooks_by_registers`): the stub's C declaration says nothing of what
the level's function takes.

`level_twins` (`level_twins.tsv`, written by `port/tools/hostgen/copycalls.py --twins`): 27 such engine
functions are entered away from the address their name says (the ammo pickup's update `func_0031FC28`
at `0x0031FC60`, after leftovers of the function before), so the level catalogue lists their copies
under level names. Their entry is their address in the port, and their hook calls go through those
copies: before, the pickup's give (hook `0x0026F740`) ran the empty stub, so ammo from crates was never
added and no message showed.

## Names the C has from somewhere else

`unit_aliases`: a file's name for another function. Twins matched beside another level's copy keep
that level's names (`func_L20_002C7588` in level 13's hero update is `func_L00_002B4458` there),
and some calls are spelled with another level's addresses or a kseg0 one (`func_80338398`). Each row
comes from comparing the function's calls with retail's jals.

`small_copies_own_data`: the catalogue's fingerprint leaves operands out, so a leaf of up to this
many bytes can be another field in another level: it reads what the running copy forms.

## What differs from Ratchet & Clank's layouts

- Ratchet's class has no packets in a level: the game loads his mesh and textures from a pack of
  RC2.HDR (`packs[]` at +0x4F08, by armour) over the class at run time. The export reads pack 0
  (`editor/wads.py`, `hero_pack`) as `mobys/moby_0.glb`, textures `moby_1000.png` on. Packs 1 to 8, his
  other looks, are classes 6001 to 6008 of the export, textures 16 places on for each; `port/game/rac2/profile.cpp`
  has the window draw the one the game has (armour `0x001A7A18`, or the four flags before it), through
  `LiveMoby::model_class`, so class 0 stays the hero everywhere else. His sequences are compressed on
  the disc, so the glTF has none: the game draws him from its memory.
- What he holds is not in a level at all, unlike Ratchet & Clank's gadget table: the 42 items are banks
  on the disc (RC2.HDR +0x4B3C, class and sectors, `wads.item_banks`), loaded by `func_00294988` when he
  takes one out, each a class and one texture. Every level exports them all, textures `moby_1203.png`
  on. The wrench (class 71, level has its skeleton) is packs 9 to 11, `moby_1200.png` to `moby_1202.png`,
  its forms 1 and 2 classes 6021 and 6022. His ship (class 3412) has its picture from RC2.HDR +0x48F0:
  18 paint jobs, the Ship Shack's, of which `func_002926B8` loads the one the low five bits of
  `0x001A7AF2` name (the Ship Shack's preview loads others) and keeps its number at `0x00152C30`. The
  faces are exported with paint 0, `moby_1300.png`, the others as `ship_paint_01.png` to `_17`, listed
  in the manifest's `ship_paints`; the window draws those faces with the paint loaded (format 15). Its hull is the disc's too (RC2.HDR +0x48D8, three of them): every
  level's load and every flight's loads it over the class (`func_002925F0`, by the low bits of
  `0x001A7AF0`), and most levels' tables have no class 3412 of their own, so parked ships drew their
  wings and canopy only. The export has hull 0 as class 3412 in every level, hulls 1 and 2 as 6031 and
  6032, and the profile draws the one loaded (`0x00152C18`); format 13. Levels of this game are format 20 (`editor/port.py`, `GAME_FORMATS`; 17 adds the shrub classes' normals, 18 the gameplay copy the probe baker reads, 19 the emission catalog and the levels' sky and sun profiles, 20 the liquid surfaces).
- Chrome and glass: the core index's +0x90 to +0x9C hold 0, 0, 0x4000, 0x400 in every level. The
  pixels are offsets into the stash: the GS RAM table's entries past its count, whose first is the
  128 x 128 chrome at stash offset 0 and the next the 64 x 64 glass at 0x4000 (the stashed moby
  textures follow from 0x5000); the CLUTs are the lump's first two, at 0 and 0x400. The game sends
  them to GS blocks 0xB00 and 0xC00 as PSMT8H, CLUTs 0x2C18 and 0x2C1C. Checked byte for byte
  against the retail GS memory in levels 0, 1, 3 and 4. Offset 0 of the texture data, which format
  8 to 11 read, is the first terrain texture (some levels' alpha 0xFF stripes: a hard white line on
  the visor and the vendor). `Format.maps_in_textures`; format 14.
- A moby's animation fields start at +0x40 (`port/game/rac2/profile.cpp`, `moby_anim_fields`).
- Glow: a packet's compact vertex table ends with a count of its last vertices whose lit colours
  func_00274D90 overwrites with the moby's glow word, +0x78 (`moby_glow_word`), when its mode has
  0x10 (the lift button's grey pulse and green, lamps, eyes). The export marks them light slot -2
  (`Format.moby_glow_tail`) and writes the gameplay file's 16 light sets the mobys are lit with
  (`Format.moby_lights`); format 9.
- Tie colours: no 64 slot colours in the instance record. The gameplay file's header word 0x94
  holds one entry per instance (a base colour and shift, then 5:5:5:1 slot colours); the game lights
  the slots with the instance's directional set and spreads them over the vertices through the
  class's remap table (PatchTieVertexLighting, 0x2F3E78). `editor/gc_tie_colours.py` does the same,
  byte for byte against retail's vertex colour cache in level 3 (point lights left out), and each
  placement's `colours` is one per vertex, a vertex's light slot its place in that list
  (`tie_vertex_colours` in the manifest); format 10.
- Bangles: parts of a moby class's mesh past its LOD lists (table at header byte 0x2C, in qwords:
  a word, then per bangle the high LOD's first packet and count and the low LOD's). A moby draws them
  by its bangle word (+0x7C, s16; MobyProc 0x2A4E68 at 0x2A5E78): bit 15 clear, the class's mesh and
  the bangles of the bits set; set, those bangles alone (bangle 0 for none). The pieces of a broken
  crate or a destroyed enemy are mobys of the class with one bit each (func_00273978 writes
  `0x8000 | 1 << n`), which the port drew as the whole mesh. The export writes each bangle as class
  `100000 + class * 16 + bangle` (`Format.moby_bangles`, `editor/moby_class.py`), and the profile adds a
  `LiveMoby` per part drawn (`model_only`: that class or nothing); format 11.
- Terrain and shrub colours: the game lights them at level load as Ratchet & Clank does.
  PatchTfragVertexLighting (0x2F0468, called for every terrain piece by the level's loader with the
  point lights cleared) is LightTfrags: the same 8-byte light records, the 5:5:5:1 base colour plus
  the piece's or vertex's directional set, from the (cos, sin) table at 0x238F00 that it copies to
  the scratchpad. The shrubs' routine (0x2E2C38) is LightShrubs. Format 12 exports both
  (`Format.world_lighting`, `wads.Version.normal_table`); before it the terrain and shrubs went out
  unlit, drawn at 1.0, so dark walls were white and gold rails tan. Checked against the console's
  memory: terrain 40,600 of 40,600 colours in level 0, 60,422 in level 3, 45,330 in level 4, 26,470 in
  level 1, 10,070 in level 5, 11,766 of 11,781 in level 22; shrubs 12,552, 47,952, 30,336 and
  6,960 of as many in levels 0, 3, 4 and 22. The ties keep their own reader (`gc_tie_colours.py`).
- The frame is drawn at FBP 0xF4, past both display buffers (`draw_buffer_block`), not right after
  the display as in Ratchet & Clank.
- `split_places.tsv`: the catalogue names the levels' copies of the draw callback lists apart from
  the executable's routines, which then would not find their tables in a level.

## The flights

The flight to a planet is not the level's: `func_002EAB18` loads one of four lumps (RC2.HDR +0x3E58, by
`g_loadingSceneVariant`) with a sky, mobys and textures of its own, and `func_002E8BA8` draws it while
`g_exitSceneMode` is 2 and the game state 6. They export as levels 98 down to 95, read as rac1's lump, and
`openrac_game_flight` (host/tools.c) makes the window draw that world. A level's exit can play a scene in
that world before the next program loads: level 0's ends so (game state 2, then 1, then 6), and the
scene's draw draws the world bound, the lump's. `g_exitSceneMode` stays 2 into the next level, whose
opening scene is its own world, so the port takes the lump's world while the sky bound (`g_pSkyData`) is
the lump's (`g_memoryArenaTable` +0x18 the lump, sky at its data base plus its +0x48 word). A flight
draws no terrain, so the sky's draw names the world's camera (`openrac_game_world_view`): at the frame's
end the game holds another view's, the cockpit screen's moby (class 2714, its own list and camera).

## Enhanced graphics

Ratchet & Clank's enhanced rendering (`port/enhanced`: probe GI and reflections, sun cascades and
his shadow map, GTAO, AgX colour, texture upscaling, FSR/ASR, motion blur, volumetric fog, HDR
output) draws this game too. It is opt-in as there: `enhanced_rendering = 1` (or `OPENRAC_ENHANCED=1`)
with the Vulkan renderer and the bakes made from the player's own levels; off, the game draws as before.

```sh
port/build/release/openrac-enhanced-baker --bake-all --levels-dir <levels> --output-dir <data>/probes/ntsc --jobs 0
OPENRAC_GL_API=vulkan OPENRAC_ENHANCED=1 port/build/release/openrac-rac2-ntsc --data <data> \
  --levels <levels> --probes <data>/probes/ntsc --window
```

Play from the same `<levels>` the baker read: interior inference writes `probe-interiors.json` into
each level, and the bake is checked against those files. From format 18 the export keeps the
gameplay copy the planner reads (`bake-inputs/gameplay.bin`): the environment transitions (header
word 0x84) and the light bank (0x04), `port/assets/world/gameplay.cpp`. Local grids grow from the
transitions and from Ratchet, the vendor and the crates (`port/enhanced/lighting/profiles/rac2`);
the ship levels and the arenas (5, 10, 15, 22, 23, 25) bake one scene grid
(`profiles/rac2-ntsc/level_NN`). From format 19 the export also resolves this game's catalog of
glowing textures (`profiles/rac2-ntsc/emission-materials.json`: lamps, light panels, screens, a lava
floor) into `probe-emission.json`, and nine levels bake their sun from the directional set most of
their scenery is lit by (`light_set`). Ratchet's fur detail takes `profiles/rac2/fur.json`, the fur
islands of his body atlas (the same mesh in every level). From format 20 the lava of levels 14 and
19 is a bake input as Gaspar's is: its surfaces (class 4959, drawn by `func_00303070` from the third
draw callback list over the corners `func_00303210` takes from four class 0x1130 markers) go to
`bake-inputs/liquids.json` with their effect texture, and the ones
`profiles/rac2-ntsc/liquids.json` reviews emit (`editor/enhanced/liquids.py`, the baker's
`rac2_liquid_triangles`). Not carried yet: the enhanced water (Novalis's tables and hooks; this
game's water is a camera-following sheet at one height, `func_L06_00402570` in levels 6, 8, 11, 13,
16 and 18, and other routines in 1, 9 and 20) and the hands' water ripples.

## The texture word (TEX0)

`0x0027C878` (rac1's GetEffectTex) returns a texture's whole TEX0 from its
cache at `0x001B9740`: TBP0, width and format in the low word, CBP and the
palette's load control in the high one. It is in `returns_64`, the glyph
and quad builders that take it (`0x0027E1A0` DrawTexturedQuad2d, `0x0027F2A8`,
`0x0027FC70`, `0x0027F640`) have 64-bit `asm.signatures` (DrawTexturedQuad2d
keeps it with an `sd` in a call's delay slot, which the register inference
does not see as an input), and the C that held it in an `int` is replaced by
`host/font.c`. This is rac1's pattern (`returns_64` on func_001F4868 and its
FontPrintWindow wrappers). `0x0028E640` (GetHudIconTex0, the Options page's
icons and other HUD sprites) returns a whole TEX0 too and is in `returns_64`;
its two C callers in `0x002D5E98.c` still pass it on as an `int`.

## Not there yet

- The first level's load (New Game, then Yes): after the intro cinematic
  (skipped) the loader (0x002EC408 -> 0x002EAB18 -> 0x00294070 -> 0x00293D30)
  reads 635 sectors at 1277354 to 0x00A54000 and hands them to the WAD
  decompressor, which finds no WAD header there. Levels also need their
  programs and the overlay hook, the level-copy functions (`func_..._N`)
  that register at one address in overlay 30 (logged at start), and the
  world renderer.
- `rom0:ROMVER` cannot be opened (the game logs it and goes on).
