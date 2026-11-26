# RAC1 native screen covers

## Current boundary

`port/game/rac1/host/screen_cover.{h,c}` implements cover commands for a caller
that already owns a native 2D target, clip and blend/test policy. It packs game
colours, clips rectangles, draws letterbox geometry (plain, or at the bars' own
depth through `openrac_frame_fill_depth`) and issues a depth-only clear through
the Fill/Clear C API. It carries no GS words.

**One retail cover entry point is bound: PAL v2.00's `draw_letterbox_bars`
(`func_001F5148`), on level 6 only: while the bars are enabled and while they shrink in
play** (`port/game/rac1/host/letterbox.c`,
"The level-6 binding" below). Every other site, level and release keeps its
packets: source inspection found state and target dependencies that the
Fill/Clear calls do not express, and keeping those sites residual avoids silently
losing their rendering or their state changes. The helpers are tested production
adapter code, not evidence that another retail call satisfies their contract.

## Native helper contract

- The view uses positive game-pixel dimensions and an optional half-open clip.
  Requested rectangles are intersected with both. Empty intersections are handled
  without drawing. Invalid dimensions and unsupported blends return 0 for the
  caller's original path, not an unconditional successful skip.
- Colours are `0xAABBGGRR`, with 128 full alpha. All byte values through 255 are
  preserved. Inputs outside a byte return 0 without a new clamping policy.
  Alpha-zero commands are not discarded: their alpha writes can still matter.
- Only opaque and source-over covers are supported. Arbitrary screen-effect
  blends are outside this helper, not silently changed to source over.
- A whole-target cover may use a NULL Fill rectangle. A picture-only cover must
  retain explicit coordinates; NULL would widen it. Letterbox bars use opaque
  full-alpha black and an already-updated height in 0..24 game pixels.
- Height animation, enable flags and frame timing remain the game's responsibility.
  The helper does not run the letterbox update a second time.
- `rac1_cover_fill_depth` and `rac1_cover_letterbox_depth` take a depth, its bit
  count and a test (`openrac_frame_fill_depth`); the bars write every colour
  channel and the depth. They return 0 when the window did not draw (no window,
  `OPENRAC_UI_REPLAY`, a value out of range): the caller keeps its packet.
- Fill coordinates are normalized to the existing recorder default of 512x448.
  A 416-line game view keeps its bar height through that ratio. This boundary must
  be reconciled with P2's explicit screen/view records before those contracts are
  combined; do not apply both normalizations.
- The depth helper writes only `OPENRAC_CLEAR_DEPTH`. It rejects partial clips
  and picture-only placement because the existing Clear is whole-target only.
  It does not pretend that any given retail clear is depth-only. The recorder's
  Clear executor writes the far value: clip -1 in the 2D pass (GL's -1..1 range;
  the world passes switch to 0..1 and back), stored 0. It used to draw clip-space
  0, which that range stores as 0.5; `renderer_gif_test` checks the vertex depth.
- Each emitted command takes the supplied reached anchor. No helper begins or
  ends a recording, reorders a world boundary, sends a list or changes a clock.

## Retail findings and activation blockers

The evidence is the pinned RAC1 PAL C under `src/game/render/draw.c` and
`framebuf.c`, its function catalogue, and the existing per-version port tables.

| Site | Proved behavior | Why it remains residual |
|---|---|---|
| `FadeToBlack`, `func_001F4E08` | The C takes one integer count, not an extra colour argument suggested by an old symbol spelling. It waits, bumps the game clock, initializes chains, draws through both large/small buffers and sends/swaps once per fade step, then initializes another chain. | **Now native on PAL v2.00** (`screen_fade.c`, docs/port/SCREEN_EFFECTS.md 5.1): the loop runs as the game's, each sent list holding one recorded black fill at the console's total alpha, and no buffer packet. (Earlier: replacing the loop with a Fill changes timing and buffer/copy semantics. Its lower `emit_rgba_draw_packet` call is not independent of residual blend, target, mask and depth state. AA/copy targets are outside this milestone.) |
| `emit_rgba_draw_packet`, `func_001F55C0` | Four integer colour channels; writes RGBAQ and references the full-screen strip packet. The owned PAL disc's packet metadata sets TEST to always pass and draws a blended sprite. | Fill does not inherit ALPHA, FRAME masks/target, ZBUF write policy or scissor. Keeping only the colour write does not preserve these effects. Byte-range colour packing alone is not an activation proof. |
| `draw_letterbox_bars`, `func_001F5148` | Updates the height toward 24 while enabled, toward 0 otherwise; zero emits nothing. The draw sets PRIM and RGBAQ, then top/bottom geometry at the viewport edges. | The caller proof below establishes a depth-tested, depth-writing bar at Z `0xFFFFF3`, not a depth-independent Fill. Bound on PAL v2.00 level 6 (the enabled edge, and the shrinking frames in play), as two depth fills ("The level-6 binding"); the freeze dialog's shrinking frames, other levels and the other releases keep the packet. |
| Audit "Z clear", `func_001FB908` | The current function is `draw_centered_capture_background(int w, int h)`, a vendor/capture-background draw. It sets destination-preserving RGB blending and TEST, uses alpha 128, and emits `w / 32` centred sprite strips. | It is not a generic depth-only clear: alpha writes, inherited write masks and ZBUF policy matter, and the centred region need not cover the current target. Whole-target Clear would widen partial coverage and can lose alpha effects. Framebuffer target conversion is a separate unit. |

Do not use the audit label as a signature or replace `func_001FB908` with a
zero-argument depth clear. No other retail depth-only site has been proved by
this milestone.

PAL v2.00 has the directly inspected bodies and level catalogue placements.
PAL v1.00 and NTSC-J release tables carry these function identities; their
generated name tables now list `draw_letterbox_bars`' names, but both releases
leave its wrapper out (`release.json` `omit`, `RAC1_OMIT_func_001F5148`), as
NTSC-U's header does. The helper itself uses no game addresses. Activation
elsewhere requires checked signatures/state evidence and tool-generated carry,
not hand-edited tables.

## Letterbox caller proof: PAL level 6

This proof uses the pinned C, the official function and split-place tables and
read-only structural facts from the owned PAL v2.00 disc (no bytes saved). No game
was run. It is a static proof with the gaps named below, not a runtime snapshot.

### Reached calls and counter ownership

The only direct C caller of `draw_letterbox_bars` is `DrawDebugProfiler`
(`func_001F3D78`, the frame renderer despite its name). Level 6 reaches it on two
routes, each storing draw mask `0x100FF` first (checked in the level's code):

| Edge | Level-6 address evidence |
|---|---|
| Play (game mode 0): `draw_debug_profiler_if_enabled` -> frame renderer | `0x00210D98`, call at `0x00210DE4` to `0x00210520` |
| Freeze dialog (mode 4, debug overlay 0): `draw_dialog_background_and_text` -> frame renderer | `0x00210E30`, call at `0x00210E64` to `0x00210520` |
| Frame renderer -> final alternate 2D setup | call at `0x00210998` to `0x002AF158` |
| Optional debug hook | call at `0x002109B8` to `0x002A6178`; its C body is empty |
| HUD, help, letterbox | calls at `0x002109E0`, `0x002109E8`, `0x002109F0` to `0x0024B160`, `0x0021CE68`, `0x00211B40` |

Letterbox runs only with mask bit `0x80` and `draw_config[16]`. Cutscene playback
(mode 2) goes through `prepare_debug_profiler_render`, which forces `0x7F`: no HUD,
help, bar or counter step. A switch to mode 3 during the play tick stores `0x7F` too.

The level-6 body reads enable `0x0015F504`, height `0x0015F508` and cursor
`0x00161240` (the executable's are `0x0015F544`, `0x0015F548`, `0x00161000`), checked
in the level's own code, not by offset. `breakable_window_update`
(`func_L06_003010E0`, a glass containment tube) sets enable to 1 and height to **18**
(store at `0x0030121C`) 30 frames after its sphere is hit with the hero within 32
units, for its camera shot. The next reached bar call raises 18 to 19; the bar holds
at 24 and shrinks to 0 once the shot clears enable. `ResetDrawGlobals` clears both.
Keep these writers and the one step per reached frame; never animate it from the
port's frame tick.

### State at the bar

The bar is one 128-byte packet at the cursor (DMA CNT, 7 quadwords): a REGLIST tag
(level `0x00160960`) setting PRIM `0x104` (flat, untextured triangle strip, no
blending, context 1) and RGBAQ `0x80000000` (black, alpha 128, Q 0), then a REGLIST
tag (`0x00160970`) with eight XYZF2 at Z `0xFFFFF3`, fog 0: the top and bottom bars
across the viewport from `g_screen_extent` (resident `0x0013E610`..`0x0013E61C`),
joined by two zero-area triangles. It writes no TEST, FRAME, ZBUF, scissor or offset.

| Inherited | Value at the bar | Evidence, and what is not proved |
|---|---|---|
| TEST_1 | alpha GEQUAL 128, failing pixels write RGB only, DATE off, depth GEQUAL | Level packet `0x001C2FF0` (split place of `func_00234F40`), sent at `0x00210998`. Only on the **enabled edge**: `update_hud` returns at its first test while enable is set, and `draw_help`'s C, read three calls deep, writes no TEST: it draws through `DrawRectOverlay`, `DrawTexturedQuad` (REGLIST TEX0, PRIM, RGBAQ, UV, XYZ2) and `FontPrintWindow`. On the shrinking edge (enable 0, height 23 to 1) the 13 HUD bank callbacks run first, through pointers: not proved from the C; measured equal in play in game ("The level-6 binding"). |
| FRAME_1, ZBUF_1, XYOFFSET_1, SCISSOR_1 | `SetPalMode`'s draw environment (below), sent by `ResetGsRegisters` each frame | Every C writer before the bar puts the same value back: the sky's and `draw_fogged_fullscreen_sprite`'s unmasked ZBUF; the moby shadow pass (`DrawMobysCleanUp` -> `configure_graphics_setup_dma_chain` -> `submit_graphics_setup_command_stream`: level packet `0x001BCAD0` masks FRAME to alpha, the function restores `0x80080`, or `0x80070` at 416 lines); `vu1_gs_regs_font` re-sending `draw_environment`; `FontPrintWindow` restoring the full scissor at its single exit. Not covered: functions that are still assembly (`PartProc` among 552 in the inspected trees), three registered draw callbacks with no C (`draw_level16_reflection_mesh_batch`, `draw_mine_predicted_impact_marker`, `draw_moby_radial_ripple_mesh_buffers`), and the VU1 programs' own GIF output (retail only; the port does not run it). |
| PRMODECONT | 1 | Reset packet; no other writer in C. |
| FBA_1, DTHE, COLCLAMP, PABE | 0 (its writers restore 0), 0, on, 0 | None changes an unblended black alpha-128 bar in 32-bit colour. |

The callback list comes from every `AddDrawCallback` and `enqueue_callback_list_4`
registration in the resident, shared and level-6 C, read four calls deep.

`SetPalMode` patches the draw environment (level 6's `ResetGsRegisters` sends the
resident `0x0013D0C0`/`0x0013D010`):

| Normal screen field | 448-line mode | 416-line mode |
|---|---|---|
| Colour target base / width / format / write mask | 128 / 512 / 32-bit / 0 | 112 / 512 / 32-bit / 0 |
| Depth target base / format / write mask | 240 / 24-bit / 0 | 216 / 24-bit / 0 |
| Scissor (inclusive) | x 0..511, y 0..447 | x 0..511, y 0..415 |
| Viewport offset in 12.4 | x `0x7000`, y `0x7200` | x `0x7000`, y `0x7300` |

The wide renderer already classifies these untextured, full-width triangles as
whole-target draws (`direct.cpp`'s triangle runs). A native binding must keep that.

### Following consumers

- `draw_subtitles` never runs in a frame with the bar: the play hook (mode 3 test)
  and the frame renderer (mode 2 test) read the same level word `0x0015F6A8`, and the
  bar's routes run in modes 0 and 4.
- Play route, after the bar: `DoGifPaging` (tags and texture uploads), then the
  overlay block. `emit_rgba_draw_packet` (underwater tint, fade, white fade) writes
  RGBAQ, then the resident packet `0x0013CD90`: depth test ALWAYS, PRIM blended sprite,
  strips at depth 0 over the screen. `draw_screen_effect` draws `DrawRectOverlay`
  rectangles (own PRIM and RGBAQ) at depth `0xFFFFF0`. In a frame with no fade or tint
  the depth test is still GEQUAL, so the retail screen effect is **rejected on the bar
  rows**. The main loop then runs `Menu_RunStateHandler` (memory-card handlers from a
  table: not inspected), the save icon (depth test ALWAYS) and the debug text (own
  TEST and PRIM at depth `0xFFFFF3`, which passes GEQUAL).
- Freeze route: `draw_freeze_message_box` starts with `emit_rgba_draw_packet(0, 0, 0,
  0x30)`, which resets depth, PRIM and RGBAQ before anything reads them.

Every inspected following producer sets its own PRIM and RGBAQ: the bar's PRIM/RGBAQ
have no reader there (open: the memory-card handlers). Its depth has one,
`draw_screen_effect`.

### The level-6 binding

`letterbox.c` wraps `draw_letterbox_bars` on PAL v2.00 (its `libraries.tsv` `wrap`
row). PAL v1.00 and NTSC-J (`release.json` `omit`, `rac1_version.h`) and NTSC-U
(`rac1_version.h`) define `RAC1_OMIT_func_001F5148` and keep their own path: the
packet and state facts above are PAL v2.00's only, and those releases have no
checked mapping of the enable and height words (`hostnames.py check` reports them
missing without the omission).

1. **The game's update, once.** The wrapper reads the list cursor and the enable
   word, runs `draw_letterbox_bars`' own C (the height step and the packet; the
   counter is never animated from the port's frame tick), and then, only when the
   enable word was set before the step (growing or holding) or it was clear with the
   height above 0 and the game mode (`D_L00_0015F6A8`) is play, 0 (shrinking), the
   current level (`0x0015EE84`) is 6, the
   game appended exactly the 128-byte packet at a reachable RAM position and
   `OPENRAC_UI_SKIP` does not name `func_001F5148`, records the two bars at the
   cursor the packet began at and moves the cursor back. That address then holds
   `DoGifPaging`'s tail tag, walked after the HUD/help range, so the bars keep their
   place. When the window does not draw them (no window, `OPENRAC_UI_REPLAY`) the
   packet stays.
2. **The bar's own depth.** Each bar is an `openrac_frame_fill_depth`: opaque black
   with alpha 128, every colour channel and the depth written (FRAME's and ZBUF's
   write masks are 0), tested GEQUAL at `0xFFFFF3` of the 24-bit buffer. The depth
   maps as a residual vertex's (`depth_clip`: `z / (2^24 - 1)`, then `2d - 1`),
   window depth `(2^24 - 13) / (2^24 - 1)`, about `0.9999992847442201`, so equal
   depths compare equal. A plain Fill would not do: over an old depth of `0xFFFFEF`
   the retail bar passes and stores `0xFFFFF3`, so `draw_screen_effect`'s rectangle
   at `0xFFFFF0` fails there, while a Fill leaves `0xFFFFEF` and the rectangle
   passes; an old depth of `0xFFFFF4` rejects the bar but not a Fill. The bars span
   the target's whole width (`OPENRAC_PICTURE_WHOLE`, as the wide renderer treated
   the full-width triangles) from the screen extent (416 lines scale into the
   recorder's 448), on the screen with the whole scissor: the bar's own target and
   scissor (rac1's host C records no panel or scissor).
3. **Kept and dropped.** Kept: colour, coverage, the depth test and the depth write
   `draw_screen_effect` reads, the place in the walk. The alpha test (GEQUAL 128)
   passes an alpha-128 bar, as on the console. Dropped: the PRIM and RGBAQ the
   packet leaves, which no inspected following producer reads (each sets its own).

Measured, not proved from the C: the shrinking frames in play (enable 0, height 23
to 1, game mode 0). There the 13 HUD bank callbacks run before the bars through
pointers. The in-game check of 2026-11-24 (R2-COVERS-REPORT) logged the same
`fbp 0x70 fbmsk 0x0 test a15/80/f3 d00 z12` at the bars' vertices in shrinking
frames as on the enabled edge, and the packet and the binding gave identical bar
rows over 71 compared frames. That is evidence for the measured scene, not a proof
for every callback the HUD can run.

Limits, not proved here (the first two keep the packet):

- The freeze dialog's shrinking frames (game mode 4): not measured.
- Other levels: only level 6's caller state was read.
- The memory-card handlers `Menu_RunStateHandler` runs after the bars, not
  inspected for PRIM or RGBAQ reads.
- What the static proof does not cover (functions still in assembly, the three
  callbacks without C, the VU1 programs' own output) and the depth write mask,
  which `OPENRAC_DUMP_DRAWS` does not log: the in-game comparison of the two paths
  (`OPENRAC_UI_SKIP=func_001F5148`) matched in the measured scene, which checks the
  port's own path there only.
- The other releases: names and places match (below), packet data and state are
  not checked; their enable and height mappings must come from versionmap.

### Chain placement and release coverage

`SetupGifPaging(0)` reserves a head tag before HUD/help/bars. A nonempty bar appends
one 128-byte packet at the current cursor; an empty one does not move it.
`DoGifPaging` later splices texture uploads in before that draw range and jumps back
to its start, and its tail jumps past the uploads. An anchor names the reached bar
position in that walk order, not a sorted address. Loading and overflow can abandon a
built list: a counter step is no permission to draw an unreached native command. The
recorder's reachability rules stay as they are.

Official `config/versions/*/functions.tsv` mappings for the void letterbox and its
frame caller:

| Release | Letterbox boot / level 6 | Frame renderer boot / level 6 |
|---|---|---|
| PAL v2.00 | `001F5148` / `00211B40` | `001F3D78` / `00210520` |
| PAL v1.00 | `001F4C98` / `002116C0` | `001F38D0` / `002100A0` |
| NTSC-U | `001F4D98` / `002117C0` | `001F39D0` / `002101A0` |
| NTSC-J | `001F56A8` / `00211E48` | `001F42D8` / `00210828` |

Both functions have checked fingerprint mappings and executable plus 19 level
placements; the PAL v1.00 and NTSC-J release tables carry both with the same code, and
NTSC-U's `us_map.tsv` gives the same identity. These establish names and places, not
equal packet data: the packet and state facts above are PAL v2.00 only. The
height-seeding actor's NTSC-U row is an unchecked aligned match. The binding is
enabled on PAL v2.00 only; no generated table was edited by hand.

## Tests

`rac1_screen_cover_test` compiles the production C implementation as C and
captures its Fill/Clear values with the real CPU FrameRecorder. Synthetic cases
check colour/alpha limits, whole-target versus picture placement, clipping and
empty intersections, bar geometry in 448/416-line views, depth-only masks,
unsupported clear scope and before/after-world anchor order. They do not exercise
the GPU.

`rac1_letterbox_test` compiles the production wrapper `letterbox.c` as C over a
synthetic original operation (it steps the height and appends a 128-byte packet,
nothing else) and the real CPU FrameRecorder. On level 6 with the bars enabled: one
game step (18 to 19), the cursor put back and two depth fills at `0xFFFFF3`,
GEQUAL, colour and depth written, at the packet's anchor; at full height the step
holds 24. Shrinking in play: one step (24 to 23), the packet taken back and the bars
recorded at 23; the last step (1 to 0) appends and records nothing. The freeze
dialog's shrinking frames keep the packet (its enabled frames are recorded); another
level and `OPENRAC_UI_SKIP` (both also while shrinking), no window and a packet of
another size keep the packet; an uncached-alias cursor is put back as the game wrote
it; 416 lines scale into the 448-line space. `rac1_screen_cover_test` checks
`rac1_cover_letterbox_depth`'s commands and its no-window answer, and
`renderer_gif_test` that a depth fill at a residual vertex's Z gets that vertex's
clip depth.

Future scissor work should keep half-open native clips explicit. Movie overlays
need P2's screen/view scope and target lifetime. Strip materials must preserve
their own tests, blend and delayed-parent state rather than borrow this cover
contract. None of those families is converted here.
