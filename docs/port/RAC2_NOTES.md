# Going Commando: notes on the game

What the port has learned about Going Commando's code and data: where it differs from Ratchet & Clank,
and how the port handles it. The program folders are [port/game/rac2](../../port/game/rac2/README.md)
and the three `rac2-*` folders beside it.

## Level hooks


`level_hooks`: 31 empty functions of the executable from `0x0026F710` to `0x0026F808`. Each level's
program fills them with hero code of its own, which the engine calls from the code it carries in
every level: state changes, the update in a scene. Rac1 has the same pattern. Assembly calls one
with all its registers (`level_hooks_by_registers`): the stub's C declaration says nothing of what
the level's function takes.

`level_twins` (`tables/level_twins.tsv`, written by `port/tools/hostgen/copycalls.py --twins`): 27 such engine
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

## What differs between the discs


Measured from the programs, not assumed (release_carry.json has each):

- **PAL** runs at 50 Hz and its text is localised: the menus' text ids are others (`0x2DE2` is
  `0xBF3`), the front end's objects are larger (the Options page's list at the front end's block
  + `0x3E810`, not `+ 0x3E1D8`; a list's `+ 0x220` is `+ 0x230`), its pause Options list has seven
  rows, not eight, and the font routine takes a scale (v1.01's `func_0027FC70` is a thunk that passes
  1.0 to the release's own). The executable's data from `0x001A79F0` is 0x88 bytes on.
- **USA v2.00** has v1.01's layout with its fixes: the same menu objects as PAL's (larger), its own
  text ids, 1,117 functions of its own.
- In all three the moby, hero, camera and view records are the same, read by the same code.
- **The level hooks** (the empty functions each level fills, rac2-ntsc's README) moved: v1.01's
  `0x0026F710` to `0x0026F808` are USA v2.00's `0x0026F718` on (one more after `0x0026F7D8`) and
  PAL's `0x0026F580` on (two more). They are alike, and rac2-decomp's catalogues pair them by
  order, which puts v1.01's `func_0026F748` (the hero's state change) on another hook in both.
  release.py pairs them by their callers instead (`pair_stubs`: every v1.01 function that calls a
  hook and has one function of the release of the same shape calls the release's hook at the same
  place); the release's own C keeps its catalogue's spelling (the per-file names).
- **Calls to functions the release does not have.** The release's own C (src/pal, src/us2) still
  calls some v1.01 functions by v1.01's name or a readable one (`module_l00_call_handler_2`,
  `load_scene_chunk`), which the release has under another name or as another function. The
  table marks such a name as not in the release (`func_V1_...`), an empty body in the port. So
  each such word in a release file names the function the release's build linked it to: the
  catalogue's row at the build's value for the name, the same function in every program that
  has one, else the word stays (the per-file names).
- **Executable functions the release has only in its levels.** USA v2.00 changed some of v1.01's
  executable functions (its `func_US2_002E8010`, `func_US2_002F6220`) and kept v1.01's code in the
  levels (`func_L00_00362138`, `func_L00_00361D08`: the level start's cutscene). In a level only
  the level's copy exists: an assembly translation of the executable's caller reaches it through
  call_copies.tsv's row by the executable's own target, a hook call in its C through a row
  copycalls.py writes under the hook's name, and its globals are where each level's build put them
  (`level_data`), not the executable's addresses its C says.

## What differs from Ratchet & Clank's layouts


- Ratchet's class has no packets in a level: the game loads his mesh and textures from a pack of
  RC2.HDR (`packs[]` at +0x4F08, by armour) over the class at run time. The export reads pack 0
  (`extractor/wads.py`, `hero_pack`) as `mobys/moby_0.glb`, textures `moby_1000.png` on. Packs 1 to 8, his
  other looks, are classes 6001 to 6008 of the export, textures 16 places on for each; `port/game/rac2/profile.cpp`
  has the window draw the one the game has loaded (the pack word `0x001A7290`, PAL `0x001A7310`, which
  `tex_load_level_pack` sets; the armour itself is `0x001A7A18`; or the four skin flags before it), through
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
  6032, and the profile draws the one loaded (`0x00152C18`); format 13. Levels of this game are format 20 (`extractor/port.py`, `GAME_FORMATS`; 17 adds the shrub classes' normals, 18 the gameplay copy the probe baker reads, 19 the emission catalog and the levels' sky and sun profiles, 20 the liquid surfaces).
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
  class's remap table (PatchTieVertexLighting, 0x2F3E78). `extractor/gc_tie_colours.py` does the same,
  byte for byte against retail's vertex colour cache in level 3 (point lights left out), and each
  placement's `colours` is one per vertex, a vertex's light slot its place in that list
  (`tie_vertex_colours` in the manifest); format 10.
- Bangles: parts of a moby class's mesh past its LOD lists (table at header byte 0x2C, in qwords:
  a word, then per bangle the high LOD's first packet and count and the low LOD's). A moby draws them
  by its bangle word (+0x7C, s16; MobyProc 0x2A4E68 at 0x2A5E78): bit 15 clear, the class's mesh and
  the bangles of the bits set; set, those bangles alone (bangle 0 for none). The pieces of a broken
  crate or a destroyed enemy are mobys of the class with one bit each (func_00273978 writes
  `0x8000 | 1 << n`), which the port drew as the whole mesh. The export writes each bangle as class
  `100000 + class * 16 + bangle` (`Format.moby_bangles`, `extractor/moby_class.py`), and the profile adds a
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
- `tables/split_places.tsv`: the catalogue names the levels' copies of the draw callback lists apart from
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

## The end credits


The credits are game state 9 in a level's program, the same code on the three discs: entered by
`func_00278EB0`, updated by `func_00278FC0` (it lists the names, text ids `0x33C0` on in v1.01, and
reads one of 20 pictures a page, by the table at `0x0014B540` + `0x21A0`, into two buffers in turn),
drawn by `func_L00_002E2958`'s copies (`levels/shared/L00_002D7940-wipe.c`), left by
`func_00278F80`. They are asked for with `func_002B49E0(9, 1, ...)`, the credits word
(`0x001B1580` in the executable) saying which part plays: `func_002D01D0`'s menu entry puts 2.

- **The picture.** Each frame the draw writes the page's 512 x 416 picture straight to the frame
  buffer (`func_002FCB00`), then draws the names over it. These frames have no frame set-up and
  nothing else clears the frame buffer: on the console the picture is what wipes the last frame's
  names. The port drew the picture only after a frame set-up (`func_0027B858`, the splash's and the
  title's), so in the credits it never showed, and the names piled up over the last frame
  (host/picture.c). In a level's program the picture is now drawn where the transfer is, after the
  world's marker (a level's tick without a world goes over the picture shown last, and only what
  follows the marker is drawn over it), with the depth test set to pass (the last tick leaves it at
  GREATER). A wider picture gets the blurred sides the title has. The names' short streaks while
  they slide in are the game's own (`func_L00_002E2C30` draws each moving name several times).
- **Skipping, a PC addition.** On the console the update ends the credits on R2 pressed (the pad
  block's +0x1A4 & 2): the credits word goes to 1 if it was 0, as the ending's last page leaves it,
  then `PopGameState` (`func_002B4B70`) goes back to the state they were asked from. With Skip
  cutscenes on (the default), Start presses R2 there (host/tools.c, `skip_buttons`), so the game
  takes them down itself, the same way it does when they finish.

## The armour vendor

The armour vendor is the weapon vendor's menu in mode 3: a Megacorp Vendor (class 11) whose pvars
+0x94 is 2 (`OpenVendorMenu`, func_002F8178: 0 weapons, 1 the channel list, 4 the equipment, 2
armour). Its list (`shop_rebuild_unlocked_slot_list`, func_002F71B8, hook point
`rac2.armor_vendor.build_list`) is 12-byte rows {icon, price, armour} at `g_vendorUi` +0x140 (the
count at +0x740): each armour above the one worn that its planet unlocked (bit armour of
`g_game_flag_table[0x8D]`, or the extras of a finished game), under the icon 0xEA92 + armour, priced 25,000,
100,000, 250,000 and 1,000,000 bolts (`0x001AD268`). The worn armour is the save block's s16 +0x20
(`0x001A7A18`; PAL `0x001A7AA0`, USA v2.00 `0x001A7A20`), 4 Carbonox the best: with it worn the
list is empty. The armour vendors are on Dobbo, Boldan, Snivelak and Grelbin; the name under the
preview is the armour's item record's (Carbonox's text 0x0AB0, USA v2.00 0x331B), its description
`g_shop_armor_text_ids[armour]` (`0x001AD250`).

Each level's copy of the mode-3 purchase dialog (func_L00_00373D40, hook point
`rac2.armor_vendor.buy`; PAL func_L00_00376810, USA v2.00 func_L00_00375068) asks, then pays the row's
price when the bolts cover it and the worn armour is not 4, and closes the menu; the close
(`shop_close_sub_menu`, func_002F9198) then wears the row's armour and loads its pack. Its preview
(`PrepareVendorSelectionPreview`, func_002FB1A0, hook point `rac2.vendor.preview`) loads the row's
armour pack over class 0 and makes the preview objects at `g_vendorUi` +0x2C: class 0, Ratchet in
that armour, then classes 1290 (Carbonox only) and 2675 (armours 3 and 4) for extra pieces, drawn
with the vendor's camera, outside the level's list, so a mod reaches them by class in the menu scope
(`moby_material_class`). Ratchet's fur and the armours 1 to 3 share their textures (one UV layout;
only the plates' texels differ); Carbonox is the whole-body suit with a texture of its own.

## Ratchet & Clank's saves: the Barlow vendor's free weapons


On a console, Going Commando looks for Ratchet & Clank saves on the memory card and gives the
first game's weapons away at the Gadgetron vendor on Barlow. The port keeps that.

**What the game does** (the same code on the three discs):

- The vendor's card check (`func_002FAE68`) asks the card state machine (`func_00298F00`) for
  its state 24 on the first card, then on the second: a card the game knows (type 2, formatted).
- States 25 to 27 open `BASCUS-97199RATCHET/save0.bin` to `save4.bin`, then
  `BESCES-50916RATCHET/…` and `BISCPS-15037RATCHET/…` (USA, Europe, Japan; every disc looks
  for all three), read the first `0x2800` bytes of each and check the save (`func_0029AF58`). A
  region's first missing file ends that region.
- In a save's item block (block 10, a byte per rac1 item id) the bytes `0x0A` (Bomb Glove),
  `0x0D` (Visibomb Gun), `0x12` (Walloper), `0x13` (Tesla Claw) and `0x19` (Decoy Glove) set the
  bits `0x01`, `0x08`, `0x02`, `0x10` and `0x04` of the word at `0x001A7A20` (v1.01;
  `D_001A7A00[8]`), and any good save sets `0x80`. The card's `x1C` counts 1 plus each weapon
  found.
- The vendor then picks its message from `0x80` and that count, marks its items 12, 53, 17, 14
  and 18 (the same five weapons in Going Commando's ids) free for the bits set (`func_002FADB0`
  → `func_002FAD68`) and clears `0x80`.

**What the port does**: the first card shows the other games' save folders as well, from the
serial folders beside it (`memcard/SCUS_971.99/BASCUS-97199RATCHET`,
`memcard/SCES_509.16/BESCES-50916RATCHET`, `memcard/SCPS_150.37/…`), read only
(port/game/common/lib/mc.c, "The other games' saves"). Going Commando reads Ratchet & Clank's
saves byte for byte as it would from a console's card, and every region the player has a save
of counts, as the game itself checks all three. Nothing is ever written, created or deleted in
another game's folder. The second card shows only its own folders.

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
`profiles/rac2-ntsc/liquids.json` reviews emit (`extractor/enhanced/liquids.py`, the baker's
`rac2_liquid_triangles`). Not carried yet: the enhanced water (Novalis's tables and hooks; this
game's water is a camera-following sheet at one height, `func_L06_00402570` in levels 6, 8, 11, 13,
16 and 18, and other routines in 1, 9 and 20) and the hands' water ripples.
