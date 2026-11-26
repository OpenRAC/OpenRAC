# Native UI for Up Your Arsenal (and what rac1/rac2 really do) — 2026-11-22

Owner: the rac3 native-UI agent. Work clone `~/RACSwarm/rac3-native-ui` (Mac, box1, box2).
For the PS2-hardware audit agent: section 1 is the inventory of the 2D path as it stands in all
three games; section 2 is the GS-free design the UI moves to.

## 1. How rac1 and rac2 draw their UI today (read from the code, OpenRAC develop fc6d101a8)

**They are not GS-free.** "Native" in rac1/rac2 means: the 2D draw routines are wrapped and their
quads become records, but the records still go through the GS packet interpreter.

- **What is captured.** `libraries.tsv` wraps (port = wrap) the game's 2D routines: rac1
  `host/ui2d.c` (DrawTexturedQuad `func_001F5800`, HUD sprites, glyphs), rac2 `host/ui2d.c`
  (flat box `0x0027DE50`, glyph quad `0x0027E018`, turned sprite, ring, shaded box) and
  `host/font.c` (the prints, passing the whole TEX0). Each routine fills an
  `openrac_game_ui_rect` (`game_host.h`): four corners **in the GS's 12.4 window coordinates with
  the game's XYOFFSET added**, texel coordinates in 12.4, z, RGBAQ, and the texture as a **GS TEX0
  register** (TBP/CBP/PSM). Flags: glyph, sprite, per-corner colours, flat, CLAMP, Gouraud.
- **How it is ordered.** `openrac_ui_send` (`common/lib/ui_quad.h`) writes a **GIF packet** into the
  game's display list in the routine's place: a DMA cnt + VIF DIRECT + PACKED GIF tag + an A+D write
  to the NOP register whose value is `0x4E55 << 48 | index`. The frontend copies the 2D list by
  walking the DMA chain (`frame.cpp direct_packets`, VIF codes skipped), and `GifInterpreter`
  (`renderer/direct`) meets the NOP and calls `native_quad`, which **writes TEX0/CLAMP/PRIM into the
  interpreter's GS register model and kicks the corners as GS vertices**.
- **Textures.** The font pages, palettes and HUD images reach the GPU as **GIF IMAGE transfers in
  the same packets** (`TexturePool::upload`, keyed by the GS block address TBP; CLUTs through CLD
  loads). A draw resolves TEX0 against that pool (`TexturePool::resolve`); glyphs then sample a
  distance field made from the resolved page (`TexturePool::distance_field`, `kUiGlyph`), which is
  what keeps text sharp at high resolution. Skipped world frames keep one-shot uploads alive with
  `retain_uploads`.
- **Layout and wide screen.** Game-side: `common/lib/hud_edges.h` moves the 13 HUD slots anchored
  left/right out by `openrac_game_hud_side()` around the game's own draw (rac1, rac2, rac3 levels).
  GS-side: rac2's widget HUD is bracketed by `kHudSideMarker` NOP markers that the interpreter uses to
  shift vertices left/right of the middle; `kPictureMarker`, `kWholeMarker` likewise. High
  resolution comes from drawing the interpreted primitives at the target's size.
- **Reaching Vulkan/GL.** `DirectRenderer` issues one GL draw per `DirectDraw` group; Vulkan is
  `renderer/vulkan/vkgl` carrying out those GL calls. Order with the world comes from more NOP
  markers (`kWorldMarker` splits the list into `direct_before_world` and the HUD bucket).
- **Other 2D still from packets in all three games:** clears, fades (`FadeToBlack`), the frame
  snapshot the pause/vendor menus read back (`sceGsExecStoreImage`, `kFrameMarker`), render to
  texture (FRAME narrower than the screen; `kMobyMarker` for the vendor and page panels), movies'
  pictures, the letterbox in rac1/rac2.

So the user's "the way we did for RAC1/RAC2" is what rac3 already has: `rac3-pal/host/ui2d.c`
(1,930 lines) and `hud.c` wrap FontPrint's glyph builders (`func_0038BA50/BC30/BE10`), the glyph
quad, turned sprites, rings, boxes, the darkening, the letterbox, Widget3D sprite quads
(`func_003AA780`) and the HUD sprites, for the boot program, frontbin, every level copy
(`func_L01_*`) and the multiplayer core and level 39 (`mp_func_*`). The user's later rule (no GS,
no PS2 hardware model at all, not even as an intermediate format) rules out that design for all
three games.

## 2. The GS-free UI path (what this work builds)

1. **Capture data, not packets.** A wrapped 2D routine fills `openrac_game_ui_item`
   (`game_host.h`): four corners in the game's **screen pixels** (512 x 448 PAL, floats, the game's
   2D offset taken out), texel coordinates **in the image's pixels**, a colour per corner (0x80 =
   full, the game's scale), flags (glyph, flat, shaded), an optional clip rectangle, and the image as
   **game data**: `openrac_game_ui_image` = guest addresses of the pixels and the palette, width,
   height, 4/8/32 bits, whether the palette is in the game's 256-entry swizzled order. The routine
   then writes nothing into the display list.
2. **Order without markers.** The item also keeps the display list's cursor (a guest address)
   where the routine would have written. The frontend's chain walk already knows which guest
   address each byte came from; it turns each cursor into an offset in the copied 2D data
   (`Kept::ui_batches`). Rendering splits the residual 2D data at those offsets and draws each batch
   of native items in place. No byte is written for them.
3. **Images as real GPU textures.** The frontend decodes an image from game memory once per content
   (hash of pixels + palette) into RGBA8 on the game thread (`Kept::ui_images`, shared, immutable);
   the render side adds it to the `TexturePool` once (`add`, by hash) and glyphs use its distance
   field (`TexturePool::distance_field`), so text stays sharp at any resolution. Font pages come
   from the font headers the game keeps (`D_001D55D0[font]`: page/palette offsets, log2 sizes,
   PSMT4/8), not from any upload.
4. **Draws.** Each item becomes two triangles in the 2D pass's own vertex format (NDC from screen
   pixels), with a draw state built directly (normal alpha blend, no depth, texture = the pool
   handle), so the GL/Vulkan backends draw it like any other 2D draw. Wide screen: HUD slots keep
   `hud_edges.h` (game-side positions); the batch is inside the same pass, so the existing side
   shift applies until it too becomes item data.
5. **Fallback.** `openrac_game_ui_draw` returns 0 without a window or under `OPENRAC_UI_REPLAY`;
   the wrapper then runs the game's own routine (comparison only).

What still uses the GS model after phase N is listed in each phase's report. The residual 2D data
and `GifInterpreter` go away only when every 2D producer (clears, fades, snapshot, render to texture,
movies) is an item kind too: that is the end state, and the same contract serves rac1/rac2 later.

## 3. Up Your Arsenal's routines (PAL names; frontbin twins in `IDENTIFIED.tsv`)

| What | Boot program | Levels (Veldin's copy, 34-49 places) | Multiplayer |
|---|---|---|---|
| Font print / shaded print | `func_0038EBC8` FontPrint, `func_0038F458` | `func_L01_00410208`, `func_L01_00410A98` | `mp_func_00281240`, `mp_func_L39_00592238` |
| Glyph builders | `func_0038BA50` / `BC30` / `BE10` | `func_L01_004064A8` / `6688` / `6868` | `mp_func_0027F980`, `mp_func_L39_0058BD90` |
| Font data | FontUpload `func_0038E2B0`, FontSelect `func_0038E530`, headers `D_001D55D0[2]`, page TEX0 `D_0031CF50[2][16]`, palettes `D_0031CFD0[2][16]` | level copies | core copies |
| Glyph / turned quad | `func_0038B8B0`, `func_0038B500` | `func_L01_00406078`, `func_L01_00405CC8` | |
| Flat, shaded box, ring, darken | `func_0038B310`, `func_0038C628`, `func_0038C518`, `func_0038B200` | `L01_00405AD8`, `L01_00407080`, `L01_00406F70`, `L01_004059C8` | `mp_func_0027EEB8`, `0027FE08`, `0027EDA8` |
| HUD sprites | `func_00397680` (shaded), `func_00397A18` | `L01_004202A8`, `L01_00420720`, `L01_004208E0`, `L01_00420498`, `L01_00420AE8`; tables `*0x001DFD20/24` | `mp_func_00284F80`, `mp_func_00285318` |
| Widget3D (frames, art) | `func_003AA780` | `func_L01_004CBC60` | |
| GUI element render | | | L39 `0x005D69C0` (mp_ui.c wraps it) |
| Help boxes, dialogs | MessageBoxDraw `func_003AF7C0` / `func_003AC590` (FontPrintWrapped `func_0038FDF8`) | level copies | |
| Loading screen | `host/loading.c` (pack 97 scene) + prints | | |
| Letterbox | `func_0038AD28` | `func_L01_004054F0` | |
| Frame snapshot (pause, vendor) | | `func_L01_004C5F70` SaveFrame | |

## 4. Revised plan (2026-11-22, after the no-emulation audit)

The audit (`no-emulation-audit.md`) settles the transport: the UI goes through **P1's native frame
recorder** (`openrac_frame_*`, an ordered command list host C appends to), not through anchors in
the DMA chain, markers or the GIF interpreter. Section 2's point 2 ("order without markers" by
chain offsets) is dropped. This work is the audit's **P3 (rac3 residual 2D)** plus the rac3 UI and
the UI half of **P4 (images by the game's tables)**; claimed in `NO-EMULATION-STATUS.md`.

**Landed as groundwork (no behaviour change, independent of P1):**
- `port/game/common/include/openrac/ui_item.h`: the record. `openrac_game_ui_item` (corners in the
  game's screen pixels, texels in image pixels, colour per corner, clip, flags) and
  `openrac_game_ui_image` (game addresses of pixels/palette, size, 4/8/32 bits, swizzled palette).
  No GS word anywhere. Proposed to P1 as the recorder's UI record.
- `port/game/common/frontend/ui_images.{h,cpp}` + `tests/ui_images_test.cpp`: decode a game image
  to RGBA8 (PSMT4 low nibble first, PSMT8 with the stored-order 256 palette, 32-bit), content hash,
  a per-tick cache by description and a cross-tick cache by content.
- `TexturePool::ui_image(hash, image)`: one pool texture per content hash (bilinear, game alpha);
  glyphs take `distance_field()` of it.
- `native-ui-backend-proposal.patch`: a back end that turns the record into a DirectState/draw with
  no GS register write (`GifInterpreter::native_items`), offered to P1 for the recorder back end.

**Image identity for fonts and the HUD (P4, UI part).** Every program copy has the same two
uploaders: page `func_00389EF0(src, log2w, log2h, psm)` and palette `func_0038A028(src, psm)`,
each returning the handle the game stores in its tables (font pages `D_0031CF50[font][page]`,
palettes `D_0031CFD0[font][n]`, HUD entries). Wrapping them records handle -> {source address,
size, 4/8 bits}; a glyph's or HUD sprite's texture then becomes an `openrac_game_ui_image` in the
wrapper (translated once at the record's source, never sent). When every consumer is native the
wrappers also stop writing the IMAGE packet. Copies (catalogue rows func_00389B10 / func_00389C48
frontbin):

| Program | Page uploader | Palette uploader | TEX0 maker |
|---|---|---|---|
| boot | 0x00389EF0 | 0x0038A028 | 0x0038A0E0 |
| frontbin | 0x00389B10 | 0x00389C48 | 0x00389D00 |
| levels 01..36 | Veldin 0x004045B8 (each level's in `config/levels/functions.tsv` row func_00389B10) | 0x004046F0 | 0x004047A8 |
| level 39 / maps 40..53 | 39: 0x00589E08, 40: 0x00456D20 ... (same row) | 39: 0x00589F40 ... | 39: 0x00589FF8 ... |
| multiplayer core | to verify (FontPageTex0 is 0x0027EAC8; same layout gives 0x0027E8D8 / 0x0027EA10) | | |

**Phases once P1 has the UI call (each a PR into develop):**
1. Text: the glyph builders (boot, frontbin, level, L39, maps, mp core copies) build
   `openrac_game_ui_item` with the page image and send it through the recorder; the prints write no
   header. Done when `OPENRAC_UI_RESIDUAL` shows no glyph and no font upload is read for drawing.
2. HUD sprites/quads, help boxes, loading and save screens (P3's fade, letterbox, `0x0038CB30`,
   `0x00392B28`, the clear, the loading stars and galaxy), Holostar's box.
3. Front end, pause, vendors (Widget3D quads, turned sprites, rings, boxes, darkening).
4. mpui rac3 skin (`mp_ui.c` draw functions) through the same record.
Wide screen: HUD slots keep `hud_edges.h` (positions moved game-side); rac2-style side shifting,
if rac3 needs it, becomes a recorder scope (P1), not a marker.

## 5. What the rac3 adapter reads (data map, 2026-11-22)

Scope change (coordinator): P1 designs one shared core (recorder + 2D/UI layer: text and fonts,
sprites, rects, highlight, anchoring, scaling, fades) and a per-game adapter contract. This agent
writes the rac3 adapter and the shared text/HUD/menu code in that core, after P1's design note.
Nothing rac3-specific goes in shared code. The groundwork of section 4 (`ui_item.h`, `ui_images`,
`TexturePool::ui_image`, the back-end patch) is **not landed**: it is a proposal for P1's core, on
branch `feat/rac3-native-ui` in `~/RACSwarm/rac3-native-ui/OpenRAC` (commit "wip"), and moves into
P1's layout once the note is out.

Addresses are the boot program's (PAL). frontbin, every level, level 39 and the maps have the same
code at other addresses (catalogue rows in `config/levels/functions.tsv`, `config/frontend_pairs.tsv`;
hostgen names level copies after Veldin's, `func_L01_*`). NTSC-U through `release_carry_rac3.py`.

### 5.1 Fonts (all plain game data in EE memory; no GS needed)

- Two fonts. `D_001D55D0[2]`: each a font header (`T_3896E8`, decomp `38E2B0.c`): `+0` u8 palette
  count, `+1` u8 page count, `+0x10 + 2p` (u8 palette index, u8 page index) for glyph page p,
  `+0x30[16]` palette formats (0x14 = 16 colours, else 256 in stored order), `+0x40[16]` page formats
  (0x14 = 4-bit, 0x13 = 8-bit), `+0x50[16]` {s16 log2 width, s16 log2 height}, `+0x90[16]` palette
  offsets from the header, `+0xD0[16]` page pixel offsets from the header.
- `D_001D55D8[2]` the glyph tables, `D_001D55E0[2]` the kerning tables; FontSelect `func_0038E530`
  copies the selected font's into `D_001D55C0` (header), `D_001D55C4` (glyphs), `D_001D55C8`
  (kerning) and the current font index into `D_001D55E8`.
- Glyph record (8 bytes, `D_001D55C4[0][index]`): `w0` bits 0-3 the glyph's page, bits 2-15 (>>2 &
  0x3FFC) its kerning list's offset in `D_001D55C8` (entries {s16 next char, s8 adjust, u8 end}),
  bits 25-31 the left bearing; `w4` bits 9-16 height, 17-22 (cell), 23-28 width, 29-31 the advance
  term; mask 0x1F800000 nonzero = drawable. Char -> glyph index: `func_0038E488` (control codes:
  colours, button icons). Width of a string: `func_0038E928` (C, matched).
- FontPrint `func_0038EBC8` (asm; FontPrintShaded `func_0038F458`, FontPrintWrapped `func_0038FDF8`
  for boxed text): per glyph, the page's (palette, image) indices -> TEX0 via MakeTex0 `func_0038A0E0`;
  the glyph quad through DrawGlyph `func_0038BA50` / DrawGlyphShaded `BC30` / DrawGlyphShadowed `BE10`
  (u, v, w, h in texels of the page; colour; snap; x, y, w, h on screen). The adapter can either take
  the glyph builders' arguments (as `ui2d.c` does now) with the page known from the header, or lay out
  text itself from the tables above (shared text code).
- The page textures are the header's own bytes: page j = header + `oD0[j]`, `1 << pt[j].x` by
  `1 << pt[j].y`, 4 or 8 bits; palette k = header + `o90[k]`. FontUpload `func_0038E2B0` only sends
  them to the GS (page `func_00389EF0`, palette `func_0038A028`, handles into `D_0031CF50[font][16]`,
  `D_0031CFD0[font][16]`) and is not needed natively.

### 5.2 HUD textures (plain game data)

- HudTextureTex0 `func_00396E18(n)` (C, matched; levels `func_L01_00420080`): entry
  `*D_001D9EA8 + 4n` = {u16 palette index, u16 image index}; images `*D_001D9EAC` and palettes
  `*D_001D9EB0`, 8 bytes each {void* data, u16 GS address (0 until uploaded), u8 log2 width, u8 log2
  height}. Images are 8-bit indices, palettes 256 colours in stored order (0x400 bytes). Levels:
  `*0x001DFD20`, `*0x001DFD24`, palettes the word after (Veldin). The GS side (lazy upload queue
  `D_00225C00[64]`, count `D_001D9BFC`, allocator `D_001D4BB0`) is not needed natively.
- Sprites by texture number: DrawHudSprite / Bar / SpriteColour / Gradient (`func_L01_004202A8`,
  `00420720`, `004208E0`, `00420498`), DrawHudSpriteShaded `func_00397680` / `func_L01_00420AE8`
  (texture, four colours, centre, half sizes, angle), `func_00397A18`; the HUD depth `0x001DFD0C`.
- HUD layout: 13 slots of 0x90 at Veldin's `0x002E60D0` (+0x18 draw callback, +0x50/+0x54 place,
  +0x60 anchor flags: 1 top, 2 bottom, 4 left, 8 right), updated by `func_L01_0041FE80`. Anchoring
  for wide screen = those flags (`hud_edges.h` today; the shared core's anchoring tomorrow).

### 5.3 Menus

- Widget3D tree (front end and in-level pages): built by `func_003AF470` from the widget table
  ("PARSING WIDGET3D'S", 146 widgets); draw through each widget's vtable (`func_003EE038`, slot +0x1C).
  Classes: container `func_003EE7E8` (children +0x2C, count +0x48), box `func_003EFBB0` (vtable
  0x001D9768; separators and highlight via `func_0038C6F0`), text `func_003F03F0` (vtable 0x001D9798,
  string at +0x3C; bounds `func_003F05C8`), sprite `func_003F2158` (vtable 0x001D97F8 -> Widget3D mesh
  `func_003AA780`, posed by `func_003AA168`). Text set by id `func_003E8048`.
- Main menu items `D_00337640` {widget id, text id, enabled, action}; Options `func_003B55D8`; slot
  pages `func_003B6B90` / `func_003B7748` (four slots: planet, date, play time, bolts; textures
  0xED26, 0xEAA6); list widgets `D_00280CF0[4]` (0xB4 each).
- Boxes and highlight primitives: flat `func_0038B310`, shaded `func_0038C628`, ring `func_0038C518`,
  darkening `func_0038B200`, turned sprite `func_0038C0B8`, glyph quad `func_0038B8B0`.
- Message/help boxes: MessageBoxDraw `func_003AF7C0` -> `func_003AC590` (buttons line "\x11 Yes ...
  \x12 No", message via FontPrintWrapped). Card dialog number `D_001D4CE8` (handlers 0x003322F8).
- Pause menu (level mode 4, `D_L01_001D6694`), vendor (mode 5): the same widget classes in each
  level's copy; the frame they open over is SaveFrame `func_L01_004C5F70` (P6).
- Multiplayer (level 39): GUI element render `0x005D69C0`, screen manager at $gp-0x6AC, update
  `0x006862D0`, event read `0x00686138` (mp_ui.c's header has the rest).

### 5.4 Leftover 2D (P3)

Fade / letterbox `func_0038AD28` (levels `func_L01_004054F0`, frontbin `func_0038A948`),
`func_0038CB30` (levels `func_L01_00407588`), `func_00392B28` (levels `func_L01_00415788`), the frame
clear and hand-over (`0x00142120`), the loading screen's stars and galaxy (texture 0x2DAF,
`host/loading.c`), Holostar's sprite from block 0x1E00 (levels 13, 14), the missing bolts count.

## 6. Status (2026-11-22 night)

Branch `feat/rac3-native-text` work is on top of P1's local `feat/frame-recorder` (wip5), in
`~/RACSwarm/rac3-native-ui/OpenRAC` (Mac, latest) and pushed to box1's clone as
`feat/rac3-native-text-6`. Not on GitHub: P1's branch is not there yet, and this cannot land before it.

Done on the branch (built on box1; front-end runs compared before the GPU hang):
- `port/ui` (`openrac/ui.h`, `ui/ui.cpp`, module `ui` on layer 6 beside game/common): adapter,
  screen, glyph, quad, rect, scissor (converts to the recorder's 512 x 448 rect space), image.
- `frontend/ui_images` + test, installed as the recorder's image resolver in `frontend.cpp`.
- rac3: `host/ui_adapter.c` (screen from the 2D offset, font pages from the headers, HUD images from
  the HUD tables), `ui2d.c` (prints' glyphs and in-text glyph quads through the core for the boot
  program, frontbin and the levels; no glyph markers; the print's GIF header dropped; every other
  record through the recorder, no `openrac_ui_send` markers left in ui2d.c; SetScissor wrapped for
  boot/frontbin/levels), `hud.c` (the levels' HUD sprites and gradients through the core).
- NTSC-U: release.json evidence for 0x001D55D0 and L01:001D64F0/F4; carry rerun.
- menus.py, 13 routes: 135 ok both before and after (the one known multiplayer failure in both);
  every picture identical except the Video page, where a fifth row showed: the game cuts its 2D to
  its 448 lines with SetScissor, which native items did not follow. Fixed with the scissor wrap
  (build 6); not yet re-run because of the GPU hang.

Still through the older TEX0 records (no markers, but GS words): the multiplayer prints and HUD
(their font/scissor tables are not wrapped yet), glyph quads with other textures, turned sprites,
rings, boxes, darkening, letterbox, Widget3D quads. Still markers: hud.c's HUD-side pair,
picture.c's frame marker.

Next (phase 3): texture identity for the other getters (EffectTex0 0x00389978, PictureTex0
0x0039B088, the Widget3D textures) by wrapping the getters per copy and mapping the value they hand
out back to (table, index); then the boxes/rings/darkening as FLAT/SHADED items; the letterbox and
fade as fills (P3); the HUD-side markers as core placement.

## 7. Status (2026-11-23, after box1's reboot)

- Phase 1 is one commit, `c99e8557e feat(rac3): text, flat 2D and the scissor through a shared native 2D
  layer`, on branch `feat/rac3-native-text` in `~/RACSwarm/rac3-native-ui/OpenRAC` (Mac), on top of P1's
  local `feat/frame-recorder` wip5; bundle `native-ui-phase1.bundle` next to this file. Not on GitHub:
  it lands as a PR into develop right after P1's recorder does.
- Proof on box1: menus.py 13 routes, 135 steps ok, the same known multiplayer failure as before, and all
  136 pictures byte-identical to develop's; Veldin and its pause menu look the same;
  OPENRAC_UI_LOG: 86,001 glyphs drawn from their font page, 0 as TEX0 records. ui_images test passes.
- HUD tried and reverted: the HUD tables' image pointers (0x005F1D00 for the menus' corner ornaments)
  hold no image once it has been uploaded (every texel the same index), so those textures must be
  captured at the game's upload queue (D_00225C00) or extracted (P4). Drawn from memory they were
  solid squares.
- Found: the NTSC-U build of `host/hud.c` fails on develop (`MP_HUD(mp_func_L39_, ...)`: token-pasted
  names are not renamed by release_host.py).

## 8. Landed (2026-11-23)

- #760 (develop 1a18038fe): phase 1, text, flat 2D, the scissor, `port/ui`, `frontend/ui_images`.
- #761 (develop 3b420f736): pictures (`func_003A9BE8` and copies) through `picture_image`, the mpui skin's panels.
- Help boxes: MessageBoxDraw's box is DrawBevelBox (seven flat boxes, native since #760) and its text is native;
  only its Widget3D frame (func_003AC9F0) is left, with the Widget3D work.
- Waiting on P4's PR: HUD sprites and the corner ornaments (patches in `p4-for-native-ui/`).
- Still to do: the loading screen's stars and galaxy, the fade (func_0038AD28's fade counters), the Widget3D
  quads from the posed mesh instead of reading back the game's packets, the multiplayer executable's prints,
  boxes and scissor, the textured menu routines once P4 names their textures, the HUD-side markers as core
  placement, movies (P2), the pause tiles (P6).
