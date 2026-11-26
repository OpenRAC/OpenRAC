# Screen effects and frame copies

How a game's full-screen post effects and the copies it makes of its own frame reach the window
without the console's graphics chip. Part of the no-emulation plan and of the frame recorder
([FRAME_RECORDER.md](FRAME_RECORDER.md)): no GS packet is replayed for them, no frame-buffer block
is read or written, and the values are the game's own.

## 1. Screen effects: `openrac_frame_screen_effect`

```c
openrac_screen_effect e = {OPENRAC_SCREEN_GLOW, tint, threshold, strength, blur};
openrac_frame_anchor(list_cursor);  /* while the game still sends residual packets */
openrac_frame_screen_effect(&e);
```

The window runs the effect where the call puts it in the frame, over the target as drawn so far
(the world, and the 2D before it), with its own passes (`renderer/effects/screen_effect.{h,cpp}`,
`renderer/shaders/screen_effect.{vert,frag}`), drawn by the 2D renderer from the recorded command
(`FrameOp::ScreenEffect`, a draw of no vertices). On a panel target (`openrac_frame_target_begin`)
it works on the panel.

Resource creation and the input copy select texture unit zero explicitly. Processing owns units
zero and one, not whichever unit the preceding world renderer left active.

Inside a split-screen view, a terminal native world effect instead runs on that view's own picture
before composition, with its local dimensions and copied payload. It is not replayed on the combined
frame. The supported ordering and explicit residual-2D/composite-colour blockers are in
[FRAME_RECORDER.md section 3.2](FRAME_RECORDER.md#32-submitted-views-and-isolated-post-effects).

| Kind | Passes |
|---|---|
| `OPENRAC_SCREEN_GLOW` | the frame copied at half size: each channel times `tint`, and a pixel kept only when the frame's alpha times the tint's alpha is above `threshold` (the game's alpha test, 0x80 = 1.0; the world writes the game's alpha into the frame, clamped at 1); blurred `blur` more times; shrunk to a quarter (blurred once more, twice with `blur`) and an eighth (blurred five times); made large again, blurred once, and **added** over the frame times `strength` / 0x80 |
| `OPENRAC_SCREEN_SOFTEN` | the frame at half size, blurred twice (more with `blur`), and **mixed** over the frame by `strength` / 0x80 |

Each blur is a 3 x 3 binomial (four bilinear reads half a texel off each corner), the same filter
the console's four offset sprites give. Sizes are fractions of the window's target, so the effect
looks the same at any resolution and on a wide target covers it all.

### 1.1 Up Your Arsenal

`port/game/rac3-pal/host/screen.c` (the levels' copies, the boot program's 0x003D8978 / 0x003D89F8 /
0x003D9618 and frontbin's rows func_003D85C0 / func_003D8640 / func_003D9260), `host/mp.c` (level
39), `host/mp_maps.c` (the multiplayer maps), all through `rac3_screen_glow` / `rac3_screen_soften`:

- **The screen glow.** DrawFrame (Veldin's 0x00403440) calls 0x004DE670 and 0x004DE6F0 after its
  passes while the bits at 0x001DBCA0 are set; it clears them as each frame starts, and the sky's
  untextured shells (0x004A81B8, bit 4) and some mobys' draws (bits 1 and 2) set them. On the
  console they append two packets the level built at its start (0x004DDE98, 0x004DE5B0; the
  catalogue's rows func_003D7DE8 and func_003D8500). Their meaning, read once: the frame copied into
  256 x 256 through TEST GREATER 0x40 on alpha with RGBAQ = the four words after the glow word
  (0x001DBCA8 to 0x001DBCB4), shrunk to 128 and 64 with blurs (the 256 one twice more when the byte
  0x001DBCC8 is 1), made large again, and added back with ALPHA Cs * FIX + Cd, FIX the word
  0x001DBCB8. The host C reads those words each frame and records the glow; the glow word
  (0x001DBCA4, set to 0x6000 for the frame: the chip memory the glow's buffers take, which
  GetEffectTex's allocator reads) is still set and cleared as the game does.
- **The second effect** (0x004DF310, while 0x001DBCBC is set; 0x004D94C0 sets it, DrawFrame clears
  it after): the frame copied at 256 x 256, blurred twice and mixed back over the frame by an alpha
  the game makes from the depth buffer (the depth turned over, read as a texture in 32 strips, the
  alpha cleared in front of depth 0xFF82FF; 0x004DF1B0). The window mixes the blurred copy over the
  whole frame (`strength` 0x80): the depth at that point of the frame is the native renderer's,
  not the chip's 24-bit depth, so the per-pixel weight is not reproduced.

Level 39 and the maps have the same globals at their own addresses (glow word 0x00245BB4, the blur
byte 0x24 after it); the boot program and frontbin share theirs (glow word 0x001D934C, enable
0x001D9348, the blur byte 0x20 after the word). NTSC-U's are carried (`rac3-ntsc/release/release.json`,
`config/us/names.tsv`).

### 1.2 Beside the enhanced renderer's bloom

The game's glow and the enhanced renderer's bloom ([bloom.md](../../port/enhanced/docs/bloom.md))
are independent and both kept. The glow is the game's own effect: it is drawn whenever the game
asks for it, in both renderers, at its place in the frame (after the world, before the HUD the game
draws after it), with the game's tint and strength, and nothing draws it when the game does not.
The bloom is an optional look of the enhanced renderer over the world's colour, off by default, and
runs before the 2D pass, so with both on the glow's copy includes the bloom's halo. Turning the
bloom on does not turn the glow off: the glow marks what the game's artists chose to glow, and the
two together are what a player who enables bloom asks for.

## 2. Frame copies

| Copy | How the window does it now |
|---|---|
| The pause menu's frame (rac3 0x004C5F70 reads the frame back as 64 x 64 PSMCT24 tiles when a level's pause menu opens, 0x004C6030 writes them back each frame) | `openrac_frame_snapshot_world(id)`: the window keeps the last frame it drew with its world in (the world alone, before its 2D) as port picture `id`, and follows the world while the game draws one under the menu; the write is `openrac_frame_picture(id, NULL, 0x80808080, OPAQUE \| WHOLE)` at the list's place. No sceGsExecStoreImage answer, no block hashing (`host/picture.c`) |
| The main menu's background, the levels' full-screen pictures (Holostar's text screens, 0x004C61A0) | `openrac_frame_picture_image` from the game's own pixels (native-UI agent, #761) |
| GetEffectTex's handles -1 and -2 (0x00404040, boot 0x00389978, frontbin 0x00389598): the frame drawn into a 256 x 256 (-1) or 64 x 64 (-2) PSMCT16 buffer, which the refraction rings and the heat haze sample | still the game's packet, drawn by the 2D path into an off-screen target found by block (`g_offscreen_textures`); see section 3 |

`openrac_frame_snapshot(id)` keeps the target as drawn so far at that point of the frame; a
picture or an effect texture that samples it reads it by its port id (`>= 0x10000`). Snapshots are
whole targets with their first row at the bottom; on a target wider than the game's picture, the
game's 4:3 picture is the middle `picture_scale_x` part.

## 3. What is left

- **The frame copy of GetEffectTex -1 / -2** goes with the effect quads' and strips' native
  materials (P4): once a quad or strip can sample a recorder picture by id
  (`openrac_game_quad.picture`), the handles become `openrac_frame_snapshot(id)` at the list's
  place, with the game's allocator (0x001D4BB0) and its per-frame cache (0x001D6180 / 0x001D6188)
  kept as the game keeps them, and the packet is not sent. The host half is written
  (`rac3_frame_copy` in `host/screen.c`: the same TEX0 from the same allocator, the frame kept as
  picture `RAC3_FRAME_COPY_LARGE` 0x52330010 / `RAC3_FRAME_COPY_SMALL` 0x52330011); it is not bound
  to GetEffectTex yet, because until the effect quads sample a picture by id the rings would lose
  the off-screen target they read today.

## 4. Validation limits

`frame_record` tests effect payload retention and command-limit rollover. `screen_effect` exercises
the post-pass scheduler with fake GL resources: consecutive glow/soften effects never sample their
output attachment, resizing keeps positive buffer sizes, and shader failure stays latched until
release. It does not compile GLSL or check pixels. `renderer_gif` checks placement between draws.

The integration tests also cover submitted-view filtering, owner retention at rollover, independent
post-pass source/output targets and local viewports, and no second execution on the composite.
They do not prove residual-packet ordering, composite bloom/grading order or multiplayer pixels.

Runtime and visual behavior remain unverified. In particular, the soften pass is the whole-frame
approximation described above, not the game's depth-weighted effect. These passes do not finish
native effect textures/materials or liquid strips, and do not permit retiring the residual chain.

## 5. Ratchet & Clank (PAL v2.00): the tint and the AA pass

`port/game/rac1/host/screen_fade.c` wraps `emit_rgba_draw_packet` (`func_001F55C0`), the routine behind
every screen-wide tint (the death and white fades, the underwater tint, FadeToBlack's black, the
dimmed pause and freeze dialogs). Its own C runs and appends its 64-byte packet; with a window the
same tint is then recorded as a whole-target source-over fill at the cursor the packet began at
(`rac1_cover_fill`, picture-whole so a wide window is covered) and the packet is taken back. The
console's depth write of the sprite (Z 0, test always) is not kept. `AA_BlurPass` (`func_001FB848`)
is not sent with a window: the window draws at its own resolution and keeps no small buffer.
`OPENRAC_UI_SKIP=func_001F55C0,func_001FB848` and `OPENRAC_UI_REPLAY` keep the packets;
`OPENRAC_UI_LOG` logs each distinct tint. Other releases omit both (`RAC1_OMIT_*`).

Checked on PAL v2.00, level 1, hidden: the level-start fade-in frames have the same mean brightness
with the packet and with the fill (0, 48, 97, 141), and the pause dimming brightens and darkens the
same (101 to 57). Runs of the whole game are not frame-deterministic, so the rest of each run is
compared by looking at it, not by pixel difference.

### 5.1 FadeToBlack, the screen stripes and the frame copy

`screen_fade.c` also wraps three more routines, and `screen_copy.c` the frame copy (PAL v2.00; the
other releases omit them, `RAC1_OMIT_*`; `OPENRAC_UI_SKIP` naming a function, `OPENRAC_UI_REPLAY` and
no window keep the game's own):

- **FadeToBlack** (`func_001F4E08`). Each of its `frames` steps draws the large buffer black
  (`emit_rgba_draw_packet(0, 0, 0, 0x80)`, never displayed) and a black sprite of alpha
  `0x80 - (n << 7) / (n + 1)` onto the small buffer the display reads, `n` counting down from
  `frames - 1`. The steps multiply: after the step with `n` left the picture is `n / frames` of the one
  that was showing, black at `n = 0`. With a window the same loop of syncs, clock steps and sent lists
  runs (`func_00234AC8`, `func_00122598`, `D_0015F538`, `func_002349B8`, `func_00234948`) with one
  recorded black fill of the whole target in each list, alpha `(frames - n) / frames`, over the picture
  the window shows, and none of the buffer packets. Before this the black of the first packet was
  recorded as an opaque fill (section 5), so a fade jumped to black at once: the arrival cutscene's
  skip on Novalis went from the picture to black in one frame; now it darkens over the eight frames
  (`r2-progress/rac1-fadetoblack-*`).
- **draw_screen_effect** (`func_001F5368`): the stripe table behind the pointer at `0x0015F450`. Its own
  C runs; the rectangles are recorded as depth fills at `0xFFFFF0` of the 24-bit buffer and the packets
  taken back. The depth test is the frame's own, GEQUAL (the level's TEST packet), unless a tint ran
  right before it (the tint wrapper remembers its list position and the frame), whose resident packet
  `0x0013CD90` leaves it at ALWAYS; the letterbox's bars (`0xFFFFF3`, written) reject a GEQUAL stripe on
  their rows, as the console does. Source over (`0x44`) and the colour as it is (`0x64`) are native;
  a table with another ALPHA word keeps its packets. **Not exercised:** no C writes the table, and a
  table forced through `OPENRAC_WRITE` did not reach the routine in a hidden run, so this path has no
  runtime check.
- **The frame copy** (`transfer_draw_buffer_chunks` `func_L00_002A21A8`, `append_draw_buffer_upload_packets`
  `func_L00_002A2258`; `capture_draw_buffer` `func_L00_001F9D40` calls the first). The pause menu and the
  vendor read the frame back when they open and write it back under the menu every frame, and read none
  of it. With a window the read is `openrac_frame_snapshot_world` and the write a whole-target opaque
  `openrac_frame_picture` at the cursor the packets would have used (`D_L00_00161280`, which equals the
  list cursor in the runs); no image store or load is sent. Checked in hidden runs on Novalis: the pause
  menu (Start) and the Gadgetron vendor (Triangle) look as before (the vendor frames differ by a mean of
  0.0 to 0.6 of 255, `r2-progress/rac1-pause-*`, `rac1-vendor-*`).

Still residual on rac1: the render-to-texture routine `func_001FB608` (`setup_render_target`) and the
centred background `func_001FB908` that the vendor's monitors use. Their panels are drawn into an
off-screen block by game packets and shown by `DrawTexturedQuad` of the capture texture. A native version
is a recorded panel (`openrac_frame_target_begin` where `configure_capture_target` runs, `..._end` where
`begin_draw_frame` puts the draw environment back, the quad a picture of the panel at the same
rectangle), but the panel's text and mobys are recorded through the 2D-quad and moby-marker streams, not
as frame commands, so whether they land inside the panel is not settled by the recorder's contract: that
needs window-side work first. Going Commando's Ship Shack and pause panels (`menu_draw_pause_panels`,
`gfx_setup_screen_copy_draw_state` `func_00280510` ... `gfx_run_screen_effect_setup`, then a
`DrawTexturedQuad` of `g_quad_texture`) and the cockpit screen's 256 x 256 target
(`gfx_setup_offscreen_target` `func_0027B9C0`) are the same shape on `func_00285030`.
