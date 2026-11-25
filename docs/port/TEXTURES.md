# Textures by the game's own data

How the window knows which picture a draw samples, without the console's graphics chip. Part of
the no-emulation plan (P4): no texture is named by where the chip kept it (TBP, CBP), no record
carries a TEX0, ALPHA or TEST word, and nothing models the chip's memory, its CLUT buffer or its
CSM layouts. The only conversion is a data-format one: 4- or 8-bit indices through the game's
palette into RGBA, once per content.

## 1. Identity

A texture is named by the game's own data: the address of its pixels and of its palette in game
memory, its size and depth, and whether the palette is in the game's stored order
(`openrac_game_ui_image`, `openrac/ui_item.h`). Each game's host C finds those from the game's own
tables, the way the game's code does before it builds an upload:

| Game | What | Table | Host C |
|---|---|---|---|
| rac3 | HUD and menu images (by texture number, HudTextureTex0's argument) | entries `*0x001D9EA8` (boot, frontbin) or L01 `*0x001DFD20` (levels): {u16 palette, u16 image}; images and palettes the next two words: {data, u16, u8 log2 w, u8 log2 h} | `rac3_hud_image` (`rac3-pal/host/textures.c`) |
| rac3 | font pages | the font headers at `0x001D55D0` | `rac3_ui_font_page` (`rac3-pal/host/ui_adapter.c`) |

The window decodes the image once per content (`frontend/ui_images`) and the texture pool keeps
one GPU texture per content hash (`TexturePool::ui_image`). The content hash is the **content id**:
the same picture is the same texture wherever the game keeps it.

## 2. Images the game sends once

Some images are not kept where the game's tables point after the game has handed them over. rac3's
HUD bank sends its group 0 once, when the bank is loaded (`func_00393548`, frontbin's
`func_00393168`, the levels' `func_L01_0041C648`), from a buffer the boot program decompresses into
and then reuses: after that, the table's pointers hold whatever the buffer holds next (the menus'
corner ornaments at 0x005F1D00 read back as one colour).

For those, host C keeps the bytes at the moment the game hands them over:
`openrac_game_image_keep(data, bytes)` (`openrac/game_host.h`). It is called from the game's own
send routine (rac3: SendImage, `func_00397D60`, `func_00397980__frontbin`, `func_L01_004215C0`; the
multiplayer executable's core `0x00285660`, level 39's `0x0059D6B0` and each map's copy, each called
only by its program's HUD bank's group upload), with the game's source address and the image's size.
From then on an image named by that address reads the kept bytes, not what memory holds now
(`GameImageSources`, `frontend/ui_images.h`): it is what the game sent, as the console shows it.
Bytes kept over (part of) earlier ones replace them. When another executable starts over the game's
memory (rac3's LoadExecPS2 between the boot program and the multiplayer one), everything kept is let
go (`openrac_game_images_forget`; rac3's `rac3_textures_forget` also lets go the texture handles'
images). This is a copy of the game's data, keyed by the game's address; nothing records where the
chip put it.

## 3. Texture packs

- **OpenRAC packs** name a texture by the hash of its size and RGBA texels
  (`texture_pack::image_hash`), which is already a content id: a game image decodes to the same
  texels its upload converted to, so a pack made from the old path replaces the native one too.
  `TexturePool::ui_image` asks the pack before it makes the texture, and dumps under the same name.
- **PCSX2-format packs** name a texture by the hash of its 8-bit indices and its palette in index
  order (and the format: 8-bit, or 8-bit in the top byte of 32-bit words). Those are the game's own
  data too: while such a pack is loaded the image cache keeps the indices and the palette in index
  order (`UiImage::indexed`) and the pool looks both names up (`pcsx2_pack::replace_indexed8`).
  This is the re-keying the user approved: the pack's names are matched against the converted
  texture's source data, not against the chip's memory.

## 4. Materials (to do)

Effect quads, particles, sky sprites and strips still carry the chip's words (`openrac_game_quad`'s
CLAMP, TEX0, TEX1, ALPHA; the TEST and ZBUF taken from the list at their marker). They move to a
native material: a blend from the fixed set (opaque, mix, add, subtract, multiply), the alpha test
(compare and reference), depth test and write, filtering and wrap, translated once in host C where
the record is made, and a texture named as in section 1.

## 5. Where each game stands

| Game | By the game's data | Still by the chip's memory |
|---|---|---|
| rac3 | font pages; HUD images (`rac3_hud_image`, with group 0 kept at SendImage), the multiplayer executable's too (its HUD sprites, gradients and turned sprites, in level 39 and the maps) | the HUD's other routines until they draw `ui_item`s; effect quads and particles (TEX0, ALPHA, upload records matched by block); the multiplayer executable's routines given a texture handle (glyph quads, turned sprites, the turned HUD quad), until its texture getters are wrapped |
| rac1, rac2 | nothing yet | all (R1 to R3) |
