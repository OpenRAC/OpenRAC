# Going Commando armour models: structure, swapping, port hooks (research, 2026-11-22)

Addresses are v1.01 NTSC-U unless noted. PAL / v2.00 (US2) in brackets. Sources: rac2 decomp
(games/rac2/decomp), OpenRAC develop checkout (RACSWARM/ngpp-armour/OpenRAC), exported
assets/port/rac2/ntsc/level_00 (mobys/moby_0.glb, moby_6001..6008.glb, textures/).

## 1. The nine packs (RC2.HDR +0x4F08)
Table in the loaded RC2.HDR image (TOC at 0x0014B540, `g_copy_setup_data`): base sector word at +0x4F04,
then 16-byte rows at +0x4F08 + pack*16 = (model sector, model sectors, texture sector, texture sectors),
sectors relative to the base (editor/wads.py `hero_pack` l.265; Version.hero_packs=0x4F04,
hero_armours=9). Wrench packs 9-11 (12-14 duplicates). Pack = armour level (the shop's armour rows hold
the level as `value`), for 0-4:

| pack | look | verts / textures (export class, glb) |
|---|---|---|
| 0 | Ratchet, no armour (green/black suit) | 4273+803+401+160, tex moby_1000..1003 (class 0, moby_0.glb) |
| 1 | Tetrafiber (blue plates) | 6001, tex 1016..1019 |
| 2 | Duraplate (orange/steel) | 6002, tex 1032..1035 (+8x8 untextured prim) |
| 3 | Electrosteel (red/black) | 6003, tex 1048..1051 |
| 4 | Carbonox (grey robotic, red eyes) | 6004, ONE 256x256 tex 1064 (different UV layout, 5769 verts) |
| 5 | grey look, helmet-style head (fur shows) | 6005, 1080..1082 |
| 6 | purple/red clown-style costume | 6006, 1096..1099 |
| 7 | base suit with different head/ears | 6007, 1112..1115 |
| 8 | reduced mesh (1468 verts, 1 tex, same pixels as 1000) | 6008, tex 1128 |

Tier names confirmed by the NG++ mod (perks_test.cpp: armour levels 1-4 = Tetrafiber, Duraplate,
Electrosteel, Carbonox). Texture numbers are 1000 + 16*pack + slot.
Packs 5-8 are NOT armours: `func_00292078` replaces the requested pack with 6/8/7/5 when one of four
bytes in `g_unlocked_item_flags` (0x1A7A40 base) is set: 0x1A7A49 (item 9) -> 6, 0x1A7A42 (item 2) -> 8,
0x1A7A4B (item 11) -> 7, 0x1A7A4A (item 10) -> 5. Items 2/9/10/11 are exactly the ones the unlockable-options
menu (`func_002CF5B0` menu_handle_unlockable_options_input) re-loads the pack for. So 5-8 = the optional skins
(unlockables), they override whatever armour is worn. (Names of the skins not determined.)

## 2. Structure of a pack
- Model file (pack's first part): a normal moby class blob for class 0 (skeleton in the level; packets
  only here). Export (port glb): one moby, 113 nodes, 1 skin, 4-5 primitives = one per texture, plus a
  `<name>_metal` mesh (1 primitive, chrome map `metal_chrome`, 110-762 verts) for packets using the
  level's chrome. Primitive vertex counts above. glTF materials are named after textures
  (`moby_1019` ...), one image each, doubleSided; the 4th-slot material of packs 0-3 is alphaMode MASK.
- Texture file (second part, loaded first): u32 count at +0, offsets at +4.., each record: width @+8, height
  @+0xC, 16x16 CLUT (256 x RGBA32) at +0x20, 8-bit indexed pixels at +0x420. Sizes per pack: 256x256
  (main body atlas), 128x128 x2, 32x32 (or 64x64) small. Uploaded to VRAM by `tex_load_pack_to_vram`
  with TEX0 words stored in `g_tex_fill_table` (0x1A72A0). Packs 4 and 8 have one texture only.
- Textures are NOT shared with pack 0: each pack has its own PNG set (1016.. etc.). But in packs
  0-3, 5, 7 the main 256x256 atlas shares one UV layout and the fur/face/hands/boots texels are
  pixel-identical to pack 0 (measured: pack 1 differs in 33 % of texels, 2 in 60 %, 3 in 59 %, 5 in 30 %,
  7 in 15 %). So armour plates and fur live in the SAME primitive/texture: a per-primitive or per-material
  tint cannot separate them; separation needs a texel mask (texels that differ from pack 0/7) or a
  joint mask. Pack 4 (Carbonox) is a different atlas, whole body is armour (only the eye/fur look in the
  head region), so a whole-instance tint is nearly exactly "armour only" there.
- Small textures: 1003-style 32x32 = eye/face detail, 128x128 ones = limbs/boots/belts, 256x256 =
  torso/arms/head atlas.

## 3. How the game swaps armour
- Armour level: s16 at 0x001A7A18 = `g_player_stats` (0x1A7A00) +0x18 (NG++ reads "g_player_stats +0x18";
  same word). The decomp calls it `g_player_bolts.level` (g_player_bolts is at 0x1A79F8, +0x20): a
  misnomer, one word. [PAL: nanotech 0x1A7A94 vs ntsc 0x1A7A0C, so about 0x1A7AA0; US2 about 0x1A7A20; not verified.]
- Loaded pack: int at 0x001A7290 (`g_iop_block_table[0]` in the decomp) [PAL 0x001A7310, US2 0x001A7290]. THIS is
  what the port draws by, not 0x1A7A18 (profile.cpp l.111/156, comment says so; RAC2_NOTES.md l.~73 text is stale).
- SetArmor = `func_002923C0` `tex_load_level_pack(pack)` (72 bytes, src/game/00291758.c:358)
  [PAL func_PAL_00292CD0 = tex_load_level_pack_pal (0x292CD0), US2 0x292C70]:
  1. `func_00292078` `tex_load_pack_to_vram` (684 B) [PAL 0x292988, US2 0x292928]: waits DMA, picks pack
     (arg, or 6/8/7/5 by the flag bytes), reads textures from disc (`load_read_disc_with_pause` 0x2B7C80)
     into load buffer 0x1A7294, uploads each CLUT+image to VRAM (CLUT base 0x1A72E8, image 0x1A72F8),
     fills 0x1A72A0 TEX0 table, then reads the pack's MODEL part over the buffer.
  2. `func_00292328` `tex_build_draw_packets` (152 B) [PAL 0x292C38, US2 0x292BD8]: builds GS tag packets
     at 0x1C4DC0 (0x18 bytes each), calls `func_002933D0` tex_relocate_render_groups (relocates the model's
     pointers, rewrites texture indices through the table, calls tex_draw_batches 0x2932B8).
  3. `func_002934B8` `load_copy_header_relocate_pointers(g_class_cache_objects 0x1CDB80, g_level_pack_data
     0x1BAF38)` (60 B) [PAL 0x293DC8, US2 0x293D68]: copies the class header over class 0's in the level.
  4. stores pack in 0x1A7290.
  `func_00292408` LoadPackSingleTexture is the wrench's analogue.
- Writing 0x1A7A18 alone does nothing (RAC2_IDENTIFIED.tsv row 315): the reload only runs through the callers:
  - shop exit `func_002F9198` shop_close_sub_menu (src/game/002F5220.c:682-683 / 002F9198 file), mode 3
    (armour vendor): restores `g_player_bolts.level` (saved or alt) and calls tex_load_level_pack(level).
    [PAL func_PAL_002FA118 0x2FA118, US2 func_US2_002FA028].
  - purchase/preview path `func_002FB1A0` PrepareVendorSelectionPreview (3604 B; src/game/002F9BC0-preview-g8.c:111):
    on each cursor move loads the pack of the highlighted armour row (if `0x1A7290` differs).
    [PAL src/pal/game/002FC1D0-preview-g8.c; US2 src/us2/game/002FC008-g8.c; exact entry addresses not extracted.]
  - menu toggle `func_002CF5B0` (src/game/002CF5B0-g32.c:101-103), items 2,9,10,11 (skins) via
    tex_load_level_pack(level). [PAL func_PAL_002D0500, US2 func_US2_002D0510.]
  - main loop's level-exit branch (0x1A8B84 set) reloads via func_00292078 (IDENTIFIED row 315).

## 4. Armour vendor preview
Mode 3 of PrepareVendorSelectionPreview: no special preview class. It (a) loads the cursor row's pack with
tex_load_level_pack (so the normal class 0 now has that armour), (b) builds objects from
`g_preview_classes` (0x1AD500): ids {0, 0x50A (1290), 0xA73 (2675), 0xA73, -1}: class 0 = Ratchet himself
(drawn with the loaded pack), 1290 shown only for pack 4 (flags|=1), 2675 for packs >= 3 (extra armour
pieces, attached via AttachManipulator slots 22/23/24), (c) rotates it (`rotZ = facing`), name text id from
`g_shop_armor_text_ids` (0x1AD250: 0x2FD2..0x2FD5 for levels 1-4). On exit it restores the owned pack.
(`func_L00_0037A2E8` BindVendorPreviewModel is the WEAPON vendor's item preview, not armour.)
Port: needs nothing special. The vendor's moby is a live class-0 moby; profile.cpp `hero_class` reads
0x1A7290 which follows the cursor, so the preview shows the highlighted armour through the same path.
(Armour 5+ NG++ tiers have no game pack: the mod keeps level-4 look.)

## 5. Port: where it is chosen, drawn, coloured
- Choice: port/game/rac2/profile.cpp `hero_class` l.221-230: `look = *(int32*)ram[r.armour]` (r.armour =
  0x1A7290 [PAL 0x1A7310]); overridden by the four flag bytes `look_flags` (l.112: {0x1A7A49,6},{0x1A7A42,8},
  {0x1A7A4B,7},{0x1A7A4A,5}); returns 6000+look for look in 1..8, else -1 (class 0 = pack 0). Set at
  profile.cpp l.293-294 (`live.class_id == 0 -> live.model_class = hero_class(...)`).
- Draw: frame.cpp l.1913-1914 `moby_classes.find(replaced ? replacement->oclass : m.model_class)`, falling back to
  class_id; `scene::Instance instance{cls->second.first, m.matrix, {1,1,1,1}}` l.1918.
  There is a mod "replacement" path (`replacements`, v.moby == m.address, v.oclass) at l.1899-1914: it can already
  draw an exported class (e.g. 6004) on the hero moby's address ("class replacement" API, see visuals.cpp
  `g_visuals`, `visuals[i].oclass`) but it is aimed at props/previews (`preview_replaces_class`).
- Colour: `api_moby_material` (port/game/common/mod_api/visuals.cpp l.113-155; header openrac_mod.h l.1714-1766;
  C++ `moby_material(moby, material, rgb)` mod.hpp) stores {mod,loads,{moby,material,rgb}}; per frame
  `mods::moby_materials` (mod_play.cpp l.627) -> frame.cpp `mod_materials` l.1327 -> `apply_mod_material`
  l.1350 (called l.2321 for every instance of that moby address, whatever its model_class):
  multiplies `instance.tint` (rgb; energy 0.35 share) and sets `instance.mod_material` (kind) which
  `pack_moby_lights` packs into the instance's lights int (level.h l.~203) -> mesh.vert l.133 `v_mod_material`
  -> mesh.frag: platinum(2)/obsidian(3)/energy(4) glow/rim, rgb in `v_mod_rgb`. This is WHOLE-INSTANCE only.
  NG++'s armour glow uses this (armour.cpp: energy on the hero moby).
- Per-submesh/texture control today: `scene::Primitive.material` index into `LevelData` Materials
  (level.h l.73-97: `colour`, `emission`, `diffuse_scale`, image); fragment `material_colour` uniform
  (level_renderer.cpp l.1974/2059) is per material. The material-override catalog
  (port/materials/schema.cpp: `surface.tint`, `emission.runtime_intensity`, `emission.bake_mesh_rgb`,
  `emission.texture`, selectors layer/class_id/slot/texture hash; README: layered game/disc/level/ordered
  MODS) can colour/glow ONE exported class's ONE material slot (e.g. class 6004 slot moby_1064). There is
  no mod runtime API for it, it is enhanced-render-only authoring, and it is per class, not per instance. Texture
  packs can replace moby_10xx.png (static, per level load). Nothing masks fur versus armour inside one texture.

## Recommended hook (smallest)
Add one render-only call, e.g. `hero_look(mod, moby, pack, part_rgb, glow_rgb, glow_level)` (interface 1.3x),
stored like `moby_material` (g_materials entry, cleared by level load / mod unload):
1. pack override: in `frame.cpp` before l.1913, if a mod entry for `m.address` has `pack` in 1..8 use class
   `6000 + pack` as `m.model_class` (independent of 0x1A7290; works in shop preview and cutscenes where class 0
   exists). Skin flags still apply only if pack == 0 (inherit). Only classes exported in the level (6001-6008 exist
   in levels with a class 0; guard like `replacements` does with `moby_classes.contains`).
2. armour-only tint: add to `scene::Instance` an `armour_tint` rgba and a per-class texel mask. Mask built once at
   export/level load: for class 6000+p (p in 1,2,3,5,7) texel mask = texels whose colour differs from pack 0's
   same-UV texel (the shared fur/face/hand/boot texels are identical); for pack 4 mask = all, minus the eye/fur
   rectangle. Ship the mask as a 1-bit/8-bit alpha PNG next to textures (`moby_10xx_armour.png`) via the editor
   export (`editor/port.py` hero look classes, `hero_look_texture`), loaded as an extra image on the Material.
   In mesh.frag, when `v_mod_material == kArmourTint` multiply texture rgb by `mix(vec3(1), tint, mask(uv))` and add
   emission `glow*mask`. Reuse the existing energy glow plumbing for the glow level.
   Cheaper no-shader variant: bake per-tier recoloured copies of moby_10xx.png (hue shift with the mask) in a texture
   pack and let the hook choose among them; zero renderer change but fixed palettes.
3. Keep the existing whole-moby `moby_material` for the glow/rim as now.
Because the pack-4 look is the best vanilla armour and NG++ already uses it for tiers 5-12, `hero_look(pack=4,
tint=tier rgb)` is the primary use. Open points: PAL/US2 addresses of 0x1A7A18 and vendor functions to be
checked with dis.py if the mod reads them directly (the hook above avoids reading game memory).
