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

Off-screen panels (`func_001FB608` `setup_render_target` and the monitors' background `func_001FB908` on
rac1; `func_00280510`, `func_00285030` and `func_0027B9C0`, a cutscene's object preview, on rac2) are recorded panels
now: the recorder's contract is that every stream between `openrac_frame_target_begin` and `_end`, the
2D-quad and moby-marker streams included, draws into the panel, which the game's own quad then samples
(FRAME_RECORDER.md, section 3.3; `rac1/host/render_panel.c`, `rac2/host/render_panel.c`). The game's
packets for the panel still run (transitional, with the rest of the chain), except its clears: the black
sprite that ends `setup_render_target`'s packet is cut off the packet and recorded as a fill of colour 0
and depth 0 where the panel opens, and each `func_001FB908` sprite (depth 0 written, colour left as it
is: the monitors' background, in the panel and again over the screen after the close) is taken back and
recorded as a depth-only fill at the cursor it began at (`render_panel.c`; `OPENRAC_UI_SKIP=panel_clear`,
`screen_clear` or `func_001FB908` keeps the packets). Checked in hidden runs on box1 (PAL v2.00): the
vendor's monitors and the pause menu look as with the packets (`native2d-progress/rac1/vendor-c1*.png`,
`pause-p1*.png`). Still residual after it: the draw environment and TEST of the panel's setup packet
(the panel's block and offset come from them), everything the monitors' VU1 passes draw, and the console's
frame-buffer plumbing sprites (`append_palette_transfer_packet`'s copy into the small buffer). Checked in hidden runs on
box1: the Gadgetron vendor's six monitors on Novalis look as before (`r2-progress/rt/rac1-vendor-*`).

The small-buffer copy (`append_palette_transfer_packet` `func_001FB8A8`, the DMA call of
`fs_aa_transfer_packet`: 16 textured sprites from the large buffer's block into the small one the display
reads, then two points at block 0) is not sent with a window either (`screen_fade.c`; PAL v2.00 only, the
other releases omit it; `OPENRAC_UI_SKIP=func_001FB8A8` or `OPENRAC_UI_REPLAY` keep it). Its callers are
the boot's loading loop and picture, the space transition's wait and the transition loop; none of the
level's own frame loop (it calls `AA_BlurPass`). In a hidden level-1 run on box1 (`OPENRAC_UI_RESIDUAL`)
these sprites were the residual 16-sprite groups of the first draws only, and 0 in play. Frames with and
without the packet (400 frames, a shot every 4th, `native2d-progress/round2/rac1-palcopy-*`) differ only in
the phase of the "Planet Novalis" caption's fade (a mean of at most 0.3 of 255), which the runs' timing
decides, not the copy.

Going Commando (NTSC-U, hidden runs on box1, gdb call counts and `OPENRAC_UI_LOG`): the rac2 panel
wrappers do not run on the screens one expects. `func_00285030` was called 0 times in the Ship Shack
vendor (level 24: monitor, Triangle; its pages are not the pause panels, `menu_draw_pause_panels`
`func_002CBC58` 0 calls), in the seven pause pages (level 4: Weapons, Gadgets, Quick Select, Items,
Help, Options, Special; `func_002CBC58` ran 448 times and `menu_panel_draw` 315, but the page's panel
has flags 9 and is drawn directly, never through `func_00280510`) and in space flight (level 5:
`gfx_render_object_preview_to_texture` ran 398 times with no object, so `func_0027B9C0` never did).
`func_00284EA8` is a general draw-list header (1,500 or more calls a run) that closes nothing then.
What reaches them is a pause-menu panel whose draw routine is one of four menu routines and whose
flags have bits 1 and 8 clear, or a cutscene that holds an object to preview
(`g_scene_playback_state.capture`); no such screen has been found yet. Residual 2D still drawn from
the game's packets (`OPENRAC_UI_RESIDUAL=1`, kinds seen once per run): untextured sprites (fades,
tints) and a textured sprite at block 0x1E80 from the start of a level, a line, a textured triangle
strip at block 0x3FFB (in play and menus), another at block 0x2F31 (8-bit palette) in the Ship Shack
vendor, and an untextured strip in flight.

## 5.2 Going Commando: the end-of-frame fades and the letterbox bars

`port/game/rac2/host/screen_fade.c` wraps `gfx_set_clear_color_packet` (`func_0027DDC0`, a register write of
RGBAQ and the DMA call of `g_gfx_clear_packet_a`: TEST 0x30000 = depth ALWAYS, no alpha test, PRIM 0x146 = an
alpha-blended sprite, then 16 strips of the 512 x h picture; the same header words on NTSC-U v1.01, v2.00 and
PAL, read from each program) and `DoGifPaging` (`func_0027C660`). RenderFrame's tail writes ALPHA 0x44 (source
over) right after its last DoGifPaging and then the clear colour, the black fade and the white flash. A call
whose list is exactly { that ALPHA write at the end of the last paging, the RGBAQ write, the DMA call of the
packet read above } is taken back and recorded as a whole-picture source-over fill (`OPENRAC_UI_LOG`:
`screen_fade: tint`). Every other caller (pause dimming, wipes, splash, the levels' own) keeps its packets: their
blend is not proved. Checked in hidden runs on box1 (NTSC-U, level 4, 1,500 frames, a shot every 10th): 114 of 114
frames are pixel-identical to `OPENRAC_UI_REPLAY=1`, including the fade-ins (`native2d-progress/round2/rac2-*`).
The sprites' depth write (Z 0) is not kept: nothing in the list reads it after the tail.

`port/game/rac2/host/letterbox.c` wraps `DrawLetterboxBars` (`func_0027D900`). RenderFrame calls it right after
`VU1_addGSregister(0x47, 0x33001)`: alpha test NEVER with AFAIL RGB_ONLY, so each bar is opaque black written to the
colour channels only. When the list ends in that TEST write and the 128-byte packet, with the vertices as read from
the C, the two bars are recorded as `openrac_frame_fill_depth` (depth ALWAYS, RGB written). **Not exercised:** the bars
in cutscenes (levels 6 and 8, game mode 2, hidden runs) come from the other caller, the level's frame loop
(`func_L00_002A3B88` case 2, after `gfx_render_frame_all_layers`), which follows an ALPHA write and no TEST. That caller's
state is not proved and keeps its packets. No run reached the RenderFrame caller.

Left on the game's packets (rac1): the letterbox outside level 6 and the freeze dialog (the callback list of each
level was not read, `func_001F5148`), the debug and draw-overlay rectangles. (rac2): the textured sprite at block
0x1E80, the line, and every other caller of `func_0027DDC0`.
