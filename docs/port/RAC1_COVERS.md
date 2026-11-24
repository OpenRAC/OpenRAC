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
  It does not pretend that any given retail clear is depth-only.
- Each emitted command takes the supplied reached anchor. No helper begins or
  ends a recording, reorders a world boundary, sends a list or changes a clock.

## Retail findings and activation blockers

The evidence is the pinned RAC1 PAL C under `src/game/render/draw.c` and
`framebuf.c`, its function catalogue, and the existing per-version port tables.

| Site | Proved behavior | Why it remains residual |
|---|---|---|
| `FadeToBlack`, `func_001F4E08` | The C takes one integer count, not an extra colour argument suggested by an old symbol spelling. It waits, bumps the game clock, initializes chains, draws through both large/small buffers and sends/swaps once per fade step, then initializes another chain. | Replacing the loop with a Fill changes timing and buffer/copy semantics. Its lower `emit_rgba_draw_packet` call is not independent of residual blend, target, mask and depth state. AA/copy targets are outside this milestone. |
| `emit_rgba_draw_packet`, `func_001F55C0` | Four integer colour channels; writes RGBAQ and references the full-screen strip packet. The owned PAL disc's packet metadata sets TEST to always pass and draws a blended sprite. | Fill does not inherit ALPHA, FRAME masks/target, ZBUF write policy or scissor. Keeping only the colour write does not preserve these effects. Byte-range colour packing alone is not an activation proof. |
| `draw_letterbox_bars`, `func_001F5148` | Updates the height toward 24 while enabled, toward 0 otherwise; zero emits nothing. The draw sets PRIM and RGBAQ, then top/bottom geometry at the viewport edges. | It inherits FRAME/target, TEST, ZBUF and scissor. Plain native Fill is depth-independent and has its own clip. The residual PRIM/RGBAQ state must also remain available to following consumers. The geometry helper does not solve this state handoff. |
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
