# RAC1 native screen covers

## Current boundary

`port/game/rac1/host/screen_cover.{h,c}` implements cover commands for a caller
that already owns a native 2D target, clip and blend/test policy. It packs game
colours, clips rectangles, draws letterbox geometry and issues a depth-only
clear through the existing Fill/Clear C API. It carries no GS words.

**No retail cover entry point is replaced yet.** There are no new `libraries.tsv`
or hostgen bindings. Source inspection found state and target dependencies that
the current Fill/Clear calls do not express. Keeping those sites residual avoids
silently losing their rendering or their state changes. The helpers are tested
production adapter code, not evidence that a retail call satisfies their contract.

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
- Fill coordinates are normalized to the existing recorder default of 512x448.
  A 416-line game view keeps its bar height through that ratio. This boundary must
  be reconciled with P2's explicit screen/view records before those contracts are
  combined; do not apply both normalizations.
- The depth helper writes only `OPENRAC_CLEAR_DEPTH`. It rejects partial clips
  and picture-only placement because the existing Clear is whole-target only.
  It does not pretend that any given retail clear is depth-only. The recorder's
  Clear executor draws its depth at clip-space 0, which the 2D pass (GL's -1..1
  range; the world passes switch to 0..1 and back) stores as 0.5, not the far 0.
  Fix that, with a test, before any depth clear is bound.
- Each emitted command takes the supplied reached anchor. No helper begins or
  ends a recording, reorders a world boundary, sends a list or changes a clock.

## Retail findings and activation blockers

The evidence is the pinned RAC1 PAL C under `src/game/render/draw.c` and
`framebuf.c`, its function catalogue, and the existing per-version port tables.

| Site | Proved behavior | Why it remains residual |
|---|---|---|
| `FadeToBlack`, `func_001F4E08` | The C takes one integer count, not an extra colour argument suggested by an old symbol spelling. It waits, bumps the game clock, initializes chains, draws through both large/small buffers and sends/swaps once per fade step, then initializes another chain. | Replacing the loop with a Fill changes timing and buffer/copy semantics. Its lower `emit_rgba_draw_packet` call is not independent of residual blend, target, mask and depth state. AA/copy targets are outside this milestone. |
| `emit_rgba_draw_packet`, `func_001F55C0` | Four integer colour channels; writes RGBAQ and references the full-screen strip packet. The owned PAL disc's packet metadata sets TEST to always pass and draws a blended sprite. | Fill does not inherit ALPHA, FRAME masks/target, ZBUF write policy or scissor. Keeping only the colour write does not preserve these effects. Byte-range colour packing alone is not an activation proof. |
| `draw_letterbox_bars`, `func_001F5148` | Updates the height toward 24 while enabled, toward 0 otherwise; zero emits nothing. The draw sets PRIM and RGBAQ, then top/bottom geometry at the viewport edges. | The caller proof below establishes a depth-tested, depth-writing bar at Z `0xFFFFF3`, not a depth-independent Fill. A binding still needs a native rectangle with depth, the other releases' enable/height addresses and the open prefix closed (see "What a binding needs"); the helper is not a retail binding. |
| Audit "Z clear", `func_001FB908` | The current function is `draw_centered_capture_background(int w, int h)`, a vendor/capture-background draw. It sets destination-preserving RGB blending and TEST, uses alpha 128, and emits `w / 32` centred sprite strips. | It is not a generic depth-only clear: alpha writes, inherited write masks and ZBUF policy matter, and the centred region need not cover the current target. Whole-target Clear would widen partial coverage and can lose alpha effects. Framebuffer target conversion is a separate unit. |

Do not use the audit label as a signature or replace `func_001FB908` with a
zero-argument depth clear. No other retail depth-only site has been proved by
this milestone.

PAL v2.00 has the directly inspected bodies and level catalogue placements.
PAL v1.00 and NTSC-J release tables carry these function identities, but their
generated host-name tables do not yet contain bindings for this work. The NTSC-U
host table likewise lacks these wrapper mappings. The helper itself uses no game
addresses and needs no per-version table edits. Activation requires checked
signatures/state evidence and tool-generated carry, not hand-edited tables.

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
| TEST_1 | alpha GEQUAL 128, failing pixels write RGB only, DATE off, depth GEQUAL | Level packet `0x001C2FF0` (split place of `func_00234F40`), sent at `0x00210998`. Only on the **enabled edge**: `update_hud` returns at its first test while enable is set, and `draw_help`'s C, read three calls deep, writes no TEST: it draws through `DrawRectOverlay`, `DrawTexturedQuad` (REGLIST TEX0, PRIM, RGBAQ, UV, XYZ2) and `FontPrintWindow`. On the shrinking edge (enable 0, height 23 to 1) the 13 HUD bank callbacks run first, through pointers: not proved. |
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

### What a binding needs

1. **A native rectangle with depth.** With the Z24 target the bar compares and writes
   window depth `(2^24 - 13) / (2^24 - 1)`, about `0.9999992847442201`. Plain Fill
   turns depth test and write off. Example: over an old depth of `0xFFFFEF` the retail
   bar passes and stores `0xFFFFF3`, so a later rectangle at `0xFFFFF0` fails there; a
   Fill leaves `0xFFFFEF` and the rectangle passes. An old depth of `0xFFFFF4` rejects
   the retail bar but not a Fill. Needed: a flat rectangle with an explicit GEQUAL
   test, that depth (mapped as the interpreter maps a vertex Z: `z / max_z`, then
   `2d - 1`, so equal depths compare equal) and depth writes, on the screen target with
   the whole scissor, telling the caller whether the window drew it (as
   `openrac_frame_ui_item` does). No register-shaped record is needed.
2. **The game's update, once.** A wrapper runs `draw_letterbox_bars`' own C, which
   steps the height and appends the packet, records the native bar at the cursor the
   packet began at, and moves the cursor back. That address then holds `DoGifPaging`'s
   tail tag, walked after the HUD/help range, so the bar keeps its place. Until the HUD
   callbacks are proved, only on the enabled edge; the shrinking frames keep the packet.
3. **Per-version addresses.** The wrapper reads the enable and height words
   (`RAC1_ADDR(0x0015F544u)`, `RAC1_ADDR(0x0015F548u)`). No other release's
   `rac1_version.h` has them, and those tables are generated: PAL v1.00 and NTSC-J by
   `release_rac1.py` with versionmap's `--map`, NTSC-U by `versionmap/hostnames.py
   write --map --gen`. Without those inputs `hostnames.py check` fails for the other
   releases, and the tables are never edited by hand.
4. **The prefix, at runtime.** For the port's own path, a bounded `OPENRAC_DUMP_DRAWS`
   run over the tube's shot logs each bar vertex with its target block, write mask and
   TEST, and the FRAME, offset and scissor writes before it. The depth write mask is not
   in that log.

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
height-seeding actor's NTSC-U row is an unchecked aligned match. No table was edited
and no retail binding is enabled on any release.

## Tests

`rac1_screen_cover_test` compiles the production C implementation as C and
captures its Fill/Clear values with the real CPU FrameRecorder. Synthetic cases
check colour/alpha limits, whole-target versus picture placement, clipping and
empty intersections, bar geometry in 448/416-line views, depth-only masks,
unsupported clear scope and before/after-world anchor order. They do not exercise
the GPU or claim the blocked retail bindings are active.

Future scissor work should keep half-open native clips explicit. Movie overlays
need P2's screen/view scope and target lifetime. Strip materials must preserve
their own tests, blend and delayed-parent state rather than borrow this cover
contract. None of those families is converted here.
