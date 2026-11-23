# The frame recorder and the shared 2D layer

How a game's host C hands the window a frame without the console's display list, and how the
three games' interfaces (HUD, text, menus, fades) share one native 2D layer. Part of the
no-emulation plan: nothing here models the GS, GIF, VIF or DMA, and no record carries a GS
register word.

## 1. Why

Until now the port placed its native drawing inside the game's own VIF1 DMA chain: host C wrote
A+D writes to the GS NOP register into the game's list (`0x574C` world, `0x4D57` world mobys,
`0x4D42` moby lists, `0x4E55` UI quads, `0x4551` effect quads), and at `sceDmaSend` the window walked
the chain tag by tag (`frontend/frame.cpp` `direct_packets`) and ran a GS register interpreter over
it (`renderer/direct` `GifInterpreter`). The markers were found by walking the chain; the native
UI quads took the GS context the game's packets left before them.

The recorder replaces that hand-off with an **ordered list of native commands** that host C appends
to directly. The order of the calls is the draw order.

## 2. Shape

```
game C (matched or translated)          host C (per game)                 window (shared)
──────────────────────────────          ─────────────────                 ───────────────
DrawFrame passes ─────────────────────► openrac_frame_world(layers)  ───► Recording: [World,
MobyProc ─────────────────────────────► openrac_frame_world_mobys()        WorldMobys, Mobys,
wrapped 2D routines ──► ui core (§6) ─► openrac_frame_ui_item(item)        UiItem, UiQuad,
residual 2D (not wrapped yet) ───────────────────────────────────────────► Fill, Clear, ...]
                                         (still GS packets in the game's        │
                                          own list, until P3/P6/R2)             ▼
                                                                         splice by anchor (§4)
                                                                                │
                                                                                ▼
                                                              DirectRenderer back end (§5):
                                                              native DrawState → GL / Vulkan
```

- **Command list** (`renderer/frame/frame_record.h`, CPU only, unit tested): `FrameCommand`s in
  call order, the records they carry (`UiItem`, `UiQuad`, images), and the anchor of each (§4).
- **C API** (`game/common/include/openrac/frame.h`, included by `game_host.h`): `openrac_frame_*`.
- **Back end** (`renderer/direct`): the DirectRenderer's quad batching and GPU state code, driven by
  a native `DrawState` (`renderer/direct/draw_state.h`). The GIF interpreter translates the GS
  registers into that state once, at the edge (and goes with P8); recorder commands make it
  directly.

## 3. The C API

`openrac/frame.h`. Every call is cheap, returns at once, and is a no-op without a window (the
headless program keeps no recording). Coordinates of 2D commands are the game's 2D screen pixels
(`screen_width` x `screen_height` as the game draws its 2D: 512 x 448 PAL), top left origin, floats;
the window scales them to the target's pixels.

| Call | Records |
|---|---|
| `openrac_frame_begin()` | starts a new recording (optional: the window also closes one at each tick it captures) |
| `openrac_frame_anchor(gaddr list_cursor)` | transitional (§4): the game's display-list write pointer the next commands belong at; 0 for none |
| `openrac_frame_world(layers)` | the world (sky, terrain, ties, shrubs: `OPENRAC_DRAW_*`) is drawn here; the 2D before it goes under the world |
| `openrac_frame_world_mobys()` | the world's moby pass starts here; effect strips sent before it are drawn before the mobys |
| `openrac_frame_mobys(list, count)` | a moby list drawn with a camera of its own, into the current target (a panel, or the screen over a menu's world); nonzero when the window draws it |
| `openrac_frame_ui_item(const openrac_game_ui_item*)` | a quad of the 2D layer from the game's data alone (`openrac/ui_item.h`): corners, texels of an image named by game addresses, a colour per corner, flags, clip. No GS state |
| `openrac_frame_ui_quad(const openrac_game_ui_rect*)` | transitional: the older UiQuad record (TEX0, CLAMP), drawn in the 2D state in effect where it lands, as before. Retired by P4 |
| `openrac_frame_effect_quad(index)` | transitional: the world effect quad `index` (from `openrac_game_effect_quad_marker`) takes the alpha test and depth mode in effect here. Retired by P4 (materials on the record) |
| `openrac_frame_fill(rect, rgba, blend)` | a flat rectangle (fades, letterbox bars, darkening); `rect` NULL for the whole target, which on a wide picture is the whole window |
| `openrac_frame_clear(rgba, what)` | clears the target's colour and/or depth (`OPENRAC_CLEAR_COLOUR`, `_DEPTH`) |
| `openrac_frame_picture(id, rect, rgba, blend)` | draws a picture the port holds (a snapshot, or one set with `_picture_set`) over `rect`; `rect` NULL: the whole picture area, and with `OPENRAC_PICTURE_WHOLE` in `blend`'s high bits the whole target (a backdrop's sides on a wide window) |
| `openrac_frame_picture_image(const openrac_game_ui_image*, rect, rgba, blend)` | a full-screen or large picture straight from the game's own data (menu backgrounds, loading pictures, the multiplayer menus' city pictures): decoded once per content into a normal texture, no GS image transfer |
| `openrac_frame_picture_set(id, const uint8_t* rgba, w, h)` | gives the port picture `id` RGBA pixels the host decoded itself (a movie frame, a picture host C assembled from tiles); the window keeps it until set again |
| `openrac_frame_snapshot(id)` | keeps the frame drawn so far as picture `id` (pause tiles, Holostar's frame-textured sprite) |
| `openrac_frame_target_begin(id, w, h)` / `_end()` | draws into the off-screen panel `id` (a port id, never a GS block) until `_end` |
| `openrac_frame_scissor(rect)` | cuts what follows to `rect` (NULL: the whole target) |
| `openrac_frame_end()` | closes the recording (optional) |

Blend modes are a fixed native set: `OPENRAC_BLEND_OPAQUE`, `_MIX` (source over, by source alpha),
`_ADD` (source times its alpha added), `_SUBTRACT` (taken away), `_MULTIPLY` (the destination
darkened by the source's alpha). Colours are 0xAABBGGRR with 0x80 full (the games' units).

`openrac_frame_strip` (vertices, ST, RGBA, material, fog line) is reserved for P5 with P4's
material; the effect strips still come from the chain until then.

## 4. The transition: anchors instead of markers

While a game still sends residual 2D as GS packets (fades, clears, the loading screen's stars until
P3, P6 and R2 move them), the recorded commands must keep their place among those packets. No
marker is written into the game's memory for that. Instead each command carries an **anchor**: the
value of the game's own list write pointer when the command was recorded (`openrac_frame_anchor`).
Host C already knows it: it is the cursor the old markers were written at.

When the window walks the chain the game sent (still `direct_packets`, until P7), it notes every
DMA tag's address and where its data lands in the 2D stream. A recording **matches** the chain if
any of its anchors is the address of a tag of that chain; the most recent non-empty recording that
matches is used (a list drawn a tick late, or sent again by a fade, finds the recording that was
made while it was built, which replaces the `ui_kept` generations for games on the recorder). Each
command is then **spliced** before the first tag (searching forward from the previous command's)
whose address equals its anchor. A command whose anchor is found nowhere is dropped: the list
never reached that place (a list sent again and cut short, as the loading screen's while it reads the
disc), just as a marker written there would not have been in it. A command with no anchor goes where
the one before it went.
The splice is an offset into the window's own 2D stream, kept beside it (`FrameSplice`), never bytes
in it.

The walk applies the transitional semantics at the splice points exactly where the markers applied
them: `World` gives the split before the world, `WorldMobys` closes the early strips, `Mobys`
notes a texture's mobys with the frame width in effect, `EffectQuad` takes the TEST/ZBUF in effect.
The interpreter runs the 2D commands at their offsets (§5).

A recording with no anchors, or a frame with no chain at all, is drawn in call order alone. That
is the end state: once a game sends no residual packets (P7, R3), it stops calling
`openrac_frame_anchor`, `direct_packets` is not called for it, and §4 goes away with P8.

Each step is shippable: a game moves one call site at a time from markers to the recorder, and both
paths coexist in one frame (rac1 and rac2 keep their markers until R1).

## 5. The back end

`DrawState` (`renderer/direct/draw_state.h`) is the GPU state of one batched draw, in native values:

- `Blend`: off, or colour = (ks·w + ms)·Cs + (kd·w + md)·Cd with k in {-1, 0, 1}, m in {0, 1}, the
  weight w the source alpha, the destination alpha or a constant (1.0 = full, up to about 2). The
  presets above are named constructors. `translate_blend` turns it into GL factors (the existing
  table, now on the coefficients).
- `AlphaTest`: compare (never, always, less, less-equal, equal, greater-equal, greater, not-equal),
  reference (0x80 = 1.0), what a failing pixel still writes (nothing, colour, colour without alpha,
  depth): the double draw stays as it was.
- `DepthTest`: off, or greater-equal / greater / never against the reversed-depth buffer; and depth
  write.
- `TextureCombine` (modulate, decal, highlight, highlight2) with whether the texture's alpha is used;
  filter (linear or nearest, per magnification and minification); wrap (clamp or repeat per axis).
- Channel write mask, scissor in the target's 2D pixels, fog colour, and the target and source as
  **port target ids** (§3). The interpreter's legacy GS-block targets are given ids below 0x4000;
  recorder panels get ids from 0x10000 up. Effect quads that sample a panel find it by id
  (`g_offscreen_textures`).

`DirectState` (the interpreter's) is that state plus what only the packets need (the frame-buffer
source rules); the GS-to-native translation is one function, `GifInterpreter::ensure_draw`. The
recorder's executor (`GifInterpreter::set_commands` / `run_command` / `finish_commands`; it lives
in the interpreter only because it shares its draw list) builds draws from commands directly, so
recorded and residual draws merge in one ordered list and one flush. `UiItem`s are drawn by
`native_item` from their own state (the usual 2D blend, no depth, the image's texture or its distance
field for glyphs); fills, clears and pictures by `native_rect`; a snapshot is a draw of no vertices
that copies the target as drawn so far into the picture's texture.

Images: `openrac_frame_ui_item` and `openrac_frame_picture_image` resolve the game image on the game
thread through `frontend::recorder::set_image_resolver` (the 2D layer's content-hash cache,
`frontend/ui_images`), so the recording carries decoded images (`UiImage`: hash and RGBA pixels)
and the render side only makes the pool's texture (`TexturePool::ui_image`).

## 6. One 2D layer for three games

The interfaces of rac1, rac2 and rac3 do the same things with different data: fonts, HUD slots,
menus, highlight bars, fades, letterbox, pictures, wide-screen placement. One shared core does the
processing (capture → records → native drawing); each game gives an **adapter** that says where its
data is. No game logic moves: the decompiled C stays per game, and only what turns its 2D calls
into drawing is shared.

### 6.1 The core (`port/ui`, library `openrac_ui_core`, C++ with a C contract)

- **Images**: a game image (pixels, palette, size, depth, palette order: `openrac_game_ui_image`)
  decoded to RGBA once per content (`frontend/ui_images` → `TexturePool::ui_image`), shared by
  every game. Fonts get their distance field from the same decoded page.
- **Text**: a font is a page image plus a glyph table (cell, advance, kerning, baseline) the
  adapter reads from the game; the core lays out a string (wrapping, alignment, scale, shadow,
  colour codes the adapter maps) and emits one `UiItem` per glyph with `OPENRAC_UI_GLYPH`.
- **Sprites, rects, boxes, gradients, ring segments, highlight bars**: helpers that emit `UiItem`s
  (FLAT/SHADED for colour-only shapes).
- **Placement**: screen anchoring (left, centre, right, top, bottom) and "HUD: Edges" move out
  past a 4:3 picture's sides (today's `kHudSideMarker` logic as data on the item: an anchor side and
  the HUD side width); the native-resolution scale is the back end's (items are in game pixels,
  drawn at the target's resolution, glyphs from the distance field).
- **Fades and letterbox**: `openrac_frame_fill` over the whole target; the fade's level and the bars'
  height come from the game's own counters, read by the adapter.
- **Pictures and frame copies**: `openrac_frame_snapshot` / `_picture` / `_picture_image` /
  `_picture_set`. **Full-screen pictures are in scope for every game**: the rac2 and rac3 menu
  backgrounds (rac3 `host/picture.c`, rac2 `host/picture.c`), the multiplayer menus' moving city
  backgrounds and their pictures, the loading-screen pictures, and rac1's boot and pause pictures
  (`rac1/host/boot_picture.c`). Today they arrive as GS image uploads and frame-buffer writes
  (audit L2, L16: `sceGsExecLoadImage`, `FrameSnapshot`'s block hashes). They become recorder
  picture commands, the images decoded from the game's own data into normal textures, with no GS
  image transfer and no frame-buffer block address. Owners: rac3 under P3 (backgrounds, loading,
  multiplayer pictures) and P6 (frame copies, snapshots); rac1 and rac2 under R2.

### 6.2 The adapter contract (one per game, C)

```c
typedef struct openrac_ui_adapter {
    float screen_width, screen_height;               /* the game's 2D screen */
    /* Fonts: the page image and glyph table of font `font` (0 the main font). */
    int (*font)(int font, openrac_game_ui_image* page, openrac_ui_glyphs* glyphs);
    /* Images by the game's own tables: table, index -> image (HUD bank, menu art, icons). */
    int (*image)(int table, int index, openrac_game_ui_image* out);
    /* HUD slots: how many, and slot i's state (shown, position, anchor side, image, colour). */
    int (*hud_slots)(void);
    int (*hud_slot)(int i, openrac_ui_hud_slot* out);
    /* Fade and letterbox: the game's counters as a level 0..1 and a bar height in screen pixels. */
    void (*screen_cover)(float* fade, uint32_t* fade_rgba, float* bar);
    /* Text codes: the game's colour/button codes in a string -> colour or icon. */
    int (*text_code)(const char* at, openrac_ui_text_code* out);
} openrac_ui_adapter;
```

The wrapped draw routines of each game (today `rac*/host/ui2d.c`, `hud.c`) call the core instead of
building `openrac_game_ui_rect`s; the core emits recorder commands. rac3 moves first (the native-UI
agent writes its adapter and the text, HUD and menu parts); rac1 and rac2 follow in R1–R3. The exact
fields of the adapter are settled by the first adapter (rac3) and frozen when rac1's is written.

### 6.3 Who else draws through it

- **The multiplayer screens** (`port/mpui`): its `Skin` draws text, panels and highlight bars; the
  rac3 skin calls the core (`openrac_frame_ui_item` via the core's text and box helpers) instead of
  the game's own routines.
- **Mods**: the mod SDK's overlay and menu hooks (`mod_overlay`, interface 1.14 and 1.31) keep their
  triangle lists for now; their HUD text (`openrac_game_hud_texts`) goes through the core's text.
  A later SDK interface can expose the core's helpers directly.

## 7. Order of work

| Step | What | Who |
|---|---|---|
| P1 | this note; `frame.h`; recording, anchors and splices; `DrawState`; rac3's world hand-off (terrain, world mobys, moby lists, effect quads) and its UI records on the recorder; tests | P1 |
| P1 → native UI | `openrac_frame_ui_item` drawn natively (UiItem, UiImage); the rac3 adapter, text, HUD, menus | native-UI agent |
| P2 | the rest of rac3's markers (picture, movie, mp) onto the recorder | P2 |
| P3, P6 | rac3's residual 2D and frame copies as `fill`/`clear`/`picture`/`snapshot`; rac3's full-screen pictures (menu backgrounds, loading, multiplayer city backgrounds) as `picture_image`/`picture_set` (P3), frame copies as `snapshot` (P6) | P3, P6 |
| P4 | materials on effect quads, particles, strips; `ui_quad` and `effect_quad` retired | P4 |
| R1–R3 | rac1 and rac2 onto the recorder and the core with their adapters; their full-screen pictures (rac2 title, pause and menu backgrounds, rac1 boot and pause pictures) under R2 | R1–R3 |
| P7, P8 | no chain walked for a game with no residual; the GIF interpreter deleted; §4 goes | P7, P8 |

## 8. Where each game stands

| Game | On the recorder | Still markers in its list |
|---|---|---|
| rac3 | `host/render.c`: world, world mobys, moby lists, effect quads, loading screen; the 2D core (`openrac/ui.h`, `host/ui_adapter.c`): the single-player prints' text from the font headers, flat boxes, shaded boxes, the darkening and the letterbox bars as colour-only quads, the 2D scissor (SetScissor); `host/ui2d.c` and `host/hud.c`: the other 2D routines as `ui_quad` records (TEX0, until their textures come from the game's tables: the HUD's images are not resident in game memory after their upload, so they need capturing at the upload or extracting, P4) | `hud.c`'s two HUD-side markers (until the core's placement), `picture.c` (P3, P6), `mp_ui.c`, `mp_maps.c`, `movie.c` (P2) |
| rac1, rac2 | nothing yet | all (R1) |

## 9. Tests

- `frame_record_test`: a recorded frame ordered without any DMA data; anchors matched against a
  chain's tags and spliced in order (late list, resent list, missing anchor); recordings kept and
  chosen as the window does.
- `renderer_direct_test` / `renderer_gif_test`: the native `DrawState` (blend presets and the GS
  translation give the same GL factors as before; the double draw), and the executor drawing a fill
  and a UiItem with no packets.
