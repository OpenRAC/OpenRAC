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
send routine (rac3: SendImage, `func_00397D60`, `func_00397980__frontbin`, `func_L01_004215C0`),
with the game's source address and the image's size. From then on an image named by that address
reads the kept bytes, not what memory holds now (`GameImageSources`, `frontend/ui_images.h`): it is
what the game sent, as the console shows it. Bytes kept over (part of) earlier ones replace them.
This is a copy of the game's data, keyed by the game's address; nothing records where the chip put
it.

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

## 4. Materials

A world effect quad can now carry a native material instead of the chip's words:
`openrac_game_material_effect_quad(const openrac_game_material_quad*)` (`openrac/game_host.h`), with
`openrac_material` (`openrac/material.h`) and the image as section 1 names it.

- **The material:** a blend in the one linear form the 2D path already uses (`renderer::Blend`:
  coefficients of Cs and Cd weighed by the source alpha, the target's alpha or a constant), so the
  fixed set (opaque, mix, add, subtract, multiply) and the layers weighed by the target's alpha
  (a mask that keeps the colour and writes alpha, Cs Ad + Cd, (Cs - Cd) Ad + Cd) are all one form;
  the alpha test (comparison, reference, what a failing pixel still writes); the depth test against
  the world and the depth write; the channels written; the texture function and whether the
  texel's alpha counts; the filters and the wrap. No field is a GS word.
- **Ownership:** host C fills a material only from state its own record's code sets. State a draw
  inherits from the display list around it (TEST and ZBUF for rac3's DrawWorldQuad callers) is not
  guessed: such a quad stays on `openrac_game_effect_quad` and its marker.
- **Lifetime:** the game thread checks the material (a field out of range: the call returns 0 and
  the caller draws its older way) and decodes the image at the call, through the 2D layer's cache
  (`frontend/effect_material`). The tick's capture owns the quad's native state and the decoded
  pixels (`Kept::effect_images`); the render thread draws that capture alone, through
  `TexturePool::ui_image`, and never reads game memory or a handle map.
- **Drawing:** `EffectRenderer` draws a native quad in its place among the frame's quads, from
  `EffectState` (`renderer/effects/effect_state.h`): `effect_gl_state` gives the batch's GL values,
  `translate_blend` its blend, `effect_passes` the alpha test's draws (also those of the quads that
  still carry words). A native quad is never moved to the opaque pass and never samples an
  off-screen target.
- **Not yet:** no game calls it (rac3's quads inherit TEST and ZBUF); particles, sky sprites and
  strips (P5) keep their words; frame copies (GetEffectTex -1, -2) keep their block path.

## 5. Where each game stands

| Game | By the game's data | Still by the chip's memory |
|---|---|---|
| rac3 | font pages; HUD images (`rac3_hud_image`, with group 0 kept at SendImage); the native effect quad path is ready, unused (section 4) | the HUD's other routines until they draw `ui_item`s; effect quads and particles (TEX0, ALPHA, upload records matched by block; TEST and ZBUF from the list); the multiplayer executable's HUD |
| rac1, rac2 | nothing yet | all (R1 to R3) |
