# No-emulation audit: what in the port still pretends to be a PS2

Audit of OpenRAC `develop` at `fc6d101` (2026-11-22), Up Your Arsenal PAL (`port/game/rac3-pal`,
with `mp/`) first, then Ratchet & Clank (`rac1`, `rac1-pal`, `rac1-ntsc`) and Going Commando
(`rac2`, `rac2-ntsc`, `rac2-pal`) for comparison. Research only: no code was changed.

The rule (the user's, absolute): *no graphics synthesizer, nothing from the PS2, a port and not an
emulation.* The decompiled game C stays. Everything that pretends to be PS2 hardware goes: GS/GIF
packet translation, VIF/VU program interpretation, DMA chains, scratchpad emulation, IOP/SPU2,
CDVD, emulated memory maps. Drawing, audio, input, files and memory become native PC code fed by
game data.

Paths are relative to the OpenRAC root. "Host C" means the port's own C in `port/game/<game>/host`.

---

## 0. The short answer

The world is already native in all three games: terrain, ties, shrubs, mobys, sky, particles,
shadows and lighting are drawn by OpenRAC's GL/Vulkan renderers from the extracted levels. The 2D
layer (HUD, text, menus) is mostly native records too.

**The frame itself still goes through PS2 hardware in all three games.** Every tick the port walks
the game's VIF1 DMA chain in game memory (`direct_packets`, `port/game/common/frontend/frame.cpp`),
pulls the GIF data out of it and runs a GS register interpreter over it (`GifInterpreter`,
`port/renderer/direct/`). The native world and the native 2D records are placed by **markers
smuggled into that chain as A+D writes to the GS NOP register** (`kWorldMarker`, `kMobyMarker`,
`0x4E55` UI quads, effect-quad markers). Whatever the port has not wrapped yet (fades, letterbox,
clears, screen effects, image uploads) is drawn straight from the game's GS packets, and the
textures are identified by GS memory addresses (TBP/CBP). This display-list layer is shared, so
rac1 and rac2 are **not** a fully native model to copy here. They have the same layer, with less
of it left over.

rac3 has three extra problems on top of that:

1. **About 90 % of rac3's code is retail EE machine code, statically recompiled** (rac3-decomp
   PAL 10.4 % matched by size on 2026-11-07, against rac2 95 % and rac1 100 %). That code drags in
   the EE/VU0/MMI instruction models, a global VU0 register file, VU0 data memory, COP0 DMA waits
   and scratchpad DMA channels, which C would not need.
2. **The scratchpad DMA controller is modelled** (`host/spr_dma.c`: toSPR/fromSPR, normal and
   chain mode, register end states), because about 20 hand-written kernels poll CHCR.
3. **VU0 microprograms are answered by microcode entry address** (`hostgen.json`
   `asm.vu0_programs`: 0xC80, 0xDA0, 0xDB0, 0xE98, routed to `host/trig.c`, which works on the
   emulated `vf` registers).

---

## 1. Every PS2-hardware layer still in use, per game

Legend: **●** in use and load-bearing, **◐** partly in use or a fallback, **○** present but not
reached, **—** absent. "Native in rac1/rac2" says whether a sister game already does the system
without the layer.

### 1.1 Summary table

| # | Layer (what it pretends to be) | rac1 | rac2 | rac3 | Main files |
|---|---|:-:|:-:|:-:|---|
| L1 | **VIF1 DMA chain walk**: the display list read tag by tag (refe/cnt/next/ref/call/ret/end, scratchpad-sourced tags) at `sceDmaSend`/`shown_chain` | ● | ● | ● | `game/common/frontend/frame.cpp` (`direct_packets`, `capture_2d_path`), `game/common/program/game_host.cpp` (`openrac_game_dma_send`), `game/common/lib/graph.c` (`sceDmaSend`), `game/rac2/host/dma.c`, `game/rac3-pal/host/dma.c` |
| L2 | **GIF/GS interpreter**: PACKED/REGLIST/IMAGE tags, A+D writes, two GS contexts, PRIM assembly, the GS blend equation, alpha/depth tests and the double draw, scissor, FRAME/ZBUF targets, fog | ● | ● | ● | `renderer/direct/direct.cpp`, `direct.h`, `gs.h`, `renderer/vulkan/graph/graph_state_translation.cpp`, `renderer/shaders/direct.{vert,frag}` |
| L3 | **GS memory as identity**: textures by TBP/CBP with the CLUT (CSM1 swizzle) converted at draw time; uploads from GIF IMAGE and `sceGsExecLoadImage`; render targets and the frame buffer by GS block (`g_draw_buffer_block`, `g_offscreen_textures`, `display_block`); GS register words (TEX0/TEX1/CLAMP/ALPHA/TEST/ZBUF) inside the "native" UI, effect and particle records | ● | ● | ● | `renderer/textures/texture_pool.{h,cpp}`, `renderer/textures/pcsx2_pack.cpp`, `renderer/renderer.h` (`UiQuad`, `EffectQuad`), `game/common/lib/effect_texture.h`, `renderer/world/gs_pass.cpp`, `game/common/lib/graph.c` |
| L4 | **VIF UNPACK / VU1 input decode**: effect strips read as unpacks to VU1 addresses 0–3 (GIF tag, ST, RGBA, XYZ) plus MSCAL, and the fog line at VU 0x3AC–0x3AF | ● Oltanis ice, Novalis | ● lava, water | ● class-6576 liquids | `frame.cpp` (`direct_packets`, the `command >= 0x60` branch) |
| L5 | **DMA controller model**: channel registers (CHCR/MADR/QWC/TADR/SADR) written and left "as the DMAC leaves them"; scratchpad channels 8/9 in normal and chain mode; DMAC and VIF1 completion handlers run; COP0 `bc0t` DMA waits taken as done; VIF0 chains sent and ignored | ○ (WadDecompress subroutines unreachable) | ● | ● | `game/rac3-pal/host/spr_dma.c`, `host/dma.c`; `game/rac2/host/spr_dma.c`, `dma.c`, `handasm_vu0.c`; `game/rac1-pal/host/spr_dma.c`; `tools/hostgen/asmc.py` / `lower.py` (`dma_stores`, `dma_done`); `game/common/lib/kernel.c` (`openrac_game_run_dmac_handlers`) |
| L6 | **Scratchpad as a console region**: 16 KB at 0x70000000 plus a 16 KB guard modelling the TLB miss; host code writes joint matrices and light banks into it because hand-written kernels read them there | ● (joint matrices) | ● (CollLine, kernels, lights) | ● (joint pose kernel, coll_line, MobyProc lights and shadows, PartProc table, Widget3D) | `runtime/include/openrac/guest.h`, `runtime/memory.cpp` (`take_scratchpad_overrun`), `game/common/program/game_host.cpp` (`openrac_game_moby_chain`, `openrac_game_moby_local_pose`), `game/rac3-pal/profile.cpp` (light bank, shadow directions) |
| L7 | **Emulated memory map of hardware**: EE register pages (0x10000000), VU memory (0x11000000, VU0 data memory at 0x11004000), GS privileged registers (0x12000000), uncached RAM aliases, kernel-segment stores | ● | ● | ● (rac3 `func_00125090` stores T0 registers) | `runtime/include/openrac/guest.h`, `runtime/memory.cpp`, `game/rac3-pal/host/kernel.c`, `game/rac1-pal/host/tlbtrap.c` |
| L8 | **VU0 model**: a global `openrac_vu0` register file (vf, vi, ACC, Q, I, MAC/STATUS/clip flags) for translated code; VU0 microprograms answered by entry address; sine constants read from the game's own microcode | ◐ (5 `vcallms` stubs; `vu0_sine.c` reads microcode constants) | ● (`vu0_programs` 0xC80/0xC90/0xD18, `trig.c`, `handasm.c`) | ● (`vu0_programs` 0xC80/0xDA0/0xDB0/0xE98, `host/trig.c`) | `runtime/include/openrac/ee_ops.h`, `ee.h`, `tools/hostgen/asmc.py`, `game/common/lib/vu0_sine.c`, `game/rac2/host/trig.c`, `game/rac3-pal/host/trig.c` |
| L9 | **EE CPU recompilation**: retail R5900 instructions translated to C (128-bit GPRs, HI/LO, MMI lanes, delay slots, COP1 with EE rounding, saturating conversions) | ◐ (hand-written asm only) | ◐ (~4 % of code) | ● (~90 % of code, plus 767 matched functions translated back from instructions, `from_assembly`) | `tools/hostgen/asmc.py`, `runtime/include/openrac/ee.h`, `ee_ops.h`, `runtime/ee.cpp` |
| L10 | **PS2 float arithmetic in the native renderer and assets**: the VU's truncating multiply/add emulated in GLSL (`vu_mul`, `vu_add` on raw bits), and the EE FPU model in animation, lighting, fog and LOD | ● | ● | ● | `renderer/world/vu_float.{h,cpp}`, `renderer/shaders/world/tie.vert`, `renderer/world/fog.cpp`, `tie_lod.cpp`, `assets/geometry/{moby_animation,moby_lighting,lighting,tfrag_lighting}.h`, `game/common/moby_proc.cpp`, `common/ps2_float.h`, `runtime/ps2float.cpp` (hostgen `ps2_float` lists) |
| L11 | **Kernel and IOP services**: INTC/DMAC handler tables run "where the console would raise the interrupt"; threads and semaphores; SIF RPC with a 2 MB IOP memory (the "stash", server 0x11); IOP heap and module loads; `LoadExecPS2` reading another ELF from the disc image at run time; crt0 and syscall-table scans | ● | ● | ● (plus exec and the network modules) | `game/common/lib/kernel.c`, `game/rac3-pal/host/{exec.c,entry.c,kernel.c}`, `game/rac2/host/entry.c`, `game/rac1-pal/host/crt0.c`, `runtime/elf.cpp` |
| L12 | **989snd/IOP sound boundary and an SPU2 voice model**: a command ring "of the same shape as retail's" with IOP reply words; 48 voices with the chip's Gaussian table, ADSR register words and pitch words | ◐ (rac1 is cut at the 989snd API, `snd.c`; the SPU voice model is still used) | ● ring + SPU | ● ring + SPU | `game/common/lib/snd_ring.c`, `snd.c`, `sound_engine.cpp`, `game/rac2/host/sound.c`, `game/rac3-pal/host/sound.c`, `audio/spu.{h,cpp}`, `audio/envelope.cpp`, `audio/pitch.cpp` |
| L13 | **CDVD by absolute sector from the disc image**: `sceCdRead`, the stream reads, banks, VAG and PSS read from `iso_data/<game>/disc.iso` by LBA | ● | ● | ● | `game/common/lib/cdvd.c`, `game/common/lib/sound_engine.cpp`, `game/common/program/movie.cpp`, `game/rac3-pal/host/exec.c` |
| L14 | **Pad reply bytes**: libpad/libpad2 answered with DualShock 2 report layouts | ● | ● | ● (libpad 4.0) | `game/common/lib/pad.c`, `game/rac3-pal/host/pad.c` |
| L15 | **Memory card command model**: libmc commands answered at once, `sceMcSync` polling, PS2 card folder names (`BESCES-…`) | ● | ● | ● | `game/common/lib/mc.c` |
| L16 | **Frame buffer read-back and write-back**: `sceGsExecStoreImage` answered with the drawn frame, keyed by hashes of the pixel blocks it handed back; pictures written into frame-buffer blocks | ● pause menu, boot pictures | ● title, pause | ● main menu picture, pause tiles, Holostar | `renderer/direct/direct.h` (`FrameSnapshot`), `game/common/lib/graph.c`, `game/rac3-pal/host/picture.c`, `game/rac2/host/picture.c`, `game/rac1/host/boot_picture.c` |
| L17 | **IOP network modules faked**: MSIF RPC, ezNetCnf and ezNetCtl answer "an adaptor, one combination, the interface up"; DNAS passed | — | — | ● (online play) | `game/rac3-pal/host/mp_net.c`, `game/rac3-pal/mp/libraries.tsv` |
| L18 | **Debug and compare tools that need the PS2 path**: `OPENRAC_UI_REPLAY` (replay the game's GS packets), `OPENRAC_RAC3_CHAINS` (VIF census), `OPENRAC_DUMP_VIF`, `OPENRAC_UI_RESIDUAL` | ● | ● | ● | `game/rac3-pal/host/chain_census.c`, `frame.cpp`, `direct.cpp` |

### 1.2 rac3: which game systems go through which layer

| rac3 system | What runs now | Hardware layers it goes through | rac1 / rac2 native handling of the same system |
|---|---|---|---|
| **Terrain (tfrag)** | DrawTfrag `func_L01_004B1728` (and `func_L39_005F4690` for the MP maps) is host C (`host/render.c`) that tells the window to draw the extracted terrain; the occlusion mask and scenery matrices are read from the level | L1 (the world marker is an A+D NOP write into the VIF1 list), L10 (lighting baked with `ps2_float`) | Same in both: `rac1/host/render_native.c`, `rac2/host/render.c`. Native drawing; same marker hand-off |
| **Ties** | Host C pass `func_L01_004B3238`; ties from the extracted level; per-vertex colours from the export (`UYA_TIE`) | L1 marker, L10 (`tie.vert` `vu_mul`/`vu_add` LOD-colour blend) | Same in both |
| **Shrubs** | Host C `func_L01_004A59F8`; extracted shrubs | L1 marker | Same in both |
| **Mobys** | MobyProc `func_L01_00441B78` is host C (drawn flags, the live mobys read by `profile.cpp`), drawn natively with GPU skinning. The joint pose `func_L01_00440090` writes matrices **into the scratchpad** (`openrac_game_moby_chain`) for the game's own readers | L1 moby markers, L6 (pose and light bank to the scratchpad), L10 (`moby_animation.h` on `ps2_float`), L5 (the hand-written joint pose kernel 0x003CA858/0x003CA9C0 streams through the scratchpad by DMA chain) | rac1 and rac2: native drawing, same scratchpad pose hand-off. rac2's MobyAnim joint pose is replaced (`handasm_render_big.c`) |
| **Sky** | SkyDrawShell → `rac3_sky_draw_shell`; native shells with the game's matrix | L1 marker | Same in both |
| **Particles** | PartProc → `rac3_part_proc`: each live pool record goes to the window | L3 (the texture is resolved from a GS TBP/CBP entry of the table PartProc sends to the scratchpad), L6 | Same in both (records); same GS-address texture identity |
| **Effect quads** (glows, beams, draw callbacks) | DrawWorldQuad `func_L01_00407E28` → `rac3_world_quad`: a native record | L3 (the record carries TEX0/TEX1/CLAMP/ALPHA words; its texture comes from GetEffectTex upload records by GS address; TEST/ZBUF are taken from the GS context in effect at its marker in the chain), L1 | Same in both (`rac1/host/effect_quad.c`, `rac2/host/render.c`) |
| **Liquids** (class 6576 water, mud, lava) | The game's draw callbacks build VU1 strip packets; the port **decodes the VIF unpacks** to get the vertices | L4, L1, L2/L3 (blend from ALPHA words) | rac1 (Oltanis ice, Novalis ripples) and rac2 (lava, sea sheets) decode the same way. Not native anywhere |
| **Shadows** | Native shadow volumes from MobyProc's four directions (copied from the scratchpad, +0x3300) | L6 | Same as rac2 |
| **Screen glow and the second screen effect** | Left out (`func_L01_004DE670`, `…6F0`, `…DF310` return); the enhanced bloom stands in | none now, but the effect is missing | rac2 has none; rac1's screen effects `func_001F5368` go through L2 |
| **Frame copies** (pause-menu tiles, main-menu picture, Holostar's frame-textured sprite) | Pause tiles and the main-menu picture are wrapped (`host/picture.c`). Holostar's blue box (levels 13 and 14) is still drawn from GS packets that sample block 0x1E00 | L16, L3 (frame identity by block), L2 for Holostar | rac1/rac2 pause menus: the same FrameSnapshot keyed on block hashes |
| **HUD** | 13 slots and sprites wrapped as records (`host/hud.c`, `ui2d.c`) | L1 (records placed by `0x4E55` markers), L3 (HUD textures uploaded by GIF IMAGE packets, `func_L01_004045B8`/`…46F0`, resolved by TBP) | rac1 `hud.c`/`hud_panels.c`, rac2 `hud.c`: the same records-and-markers pattern |
| **Text** | Font prints wrapped; glyphs drawn from a distance field | L3 (the font page's TEX0 getter, a 64-bit GS word; the SDF built from the GIF-uploaded page), L1 | Same in both (rac2 `font.c`) |
| **Menus** (front end, pause, options, Widget3D art) | Wrapped: boxes, glyph quads, turned sprites, ring segments, Widget3D quads (`ui2d.c`, `options.c`) | L1, L3; Widget3D poses through the scratchpad (L6) | Same pattern in both |
| **Residual 2D** | Still the game's GS packets: the screen fade (`func_L01_004054F0` / boot `func_0038AD28`, the letterbox only partly as records), `func_L01_00407588` (boot `func_0038CB30`), `func_L01_00415788` (boot `func_00392B28`), the frame clear and hand-over, Holostar's box | L1 + L2 + L3 in full | rac1: fades (`FadeToBlack` `func_001F4E08`), letterbox, Z clear, screen effects, render-to-texture still go through L2 (no wrapper found). rac2: similar remainder |
| **Multiplayer UI** (level 39, the maps' HUD) | `host/mp_ui.c` native lobby screens over the game's lobby; level 39's 2D layer wrapped as the boot program's (`ui2d.c`, `mp.c`, `mp_maps.c`) | L1, L3, plus the same residual L2 | No counterpart (rac1/rac2 have no online play) |
| **Loading screen** | The ship scene's mobys native (level 97 export); stars and galaxy are game triangles | L1, L2 (the stars and the galaxy picture are drawn from GS packets) | rac1 boot pictures wrapped (`boot_picture.c`) |
| **Movies** | Native MPEG-2 decoder (`port/media`), PSS from the disc image; subtitles via the wrapped font | L13 (disc sectors), L1 (the movie's 2D list built in the game's list buffer) | Same native player in all three |
| **Audio** | 989snd's EE client runs from instructions; its RPCs are answered by the shared ring (`snd_ring.c`) and the native sound engine; banks and VAG streams from disc sectors | L12 (ring with IOP reply words, SPU voice model), L11 (`sceSifCallRpc`), L13 | rac1: cut at the 989snd API (`common/lib/snd.c`), no ring; still the SPU voice model. rac2: the same ring as rac3 |
| **Streaming** (level loads, music, speech) | `sceCdRead` by LBA from `disc.iso`; the WAD decompressor is native (`wad.c` → `common/lib/wad_routine.c`), its scratchpad DMA skipped | L13; L5 for the kernels that stream into the scratchpad | Same in both |
| **Saves and memory cards** | libmc commands to files (`common/lib/mc.c`); the PS2 card folder layout | L15 | Same in all three |
| **Input** | SDL → libpad 4.0 reply bytes (`host/pad.c`) | L14 | rac1/rac2: `common/lib/pad.c` (libpad2 layout). Same layer |
| **VU0 math macros** | (a) the decompilation's `include/vu0.h` helpers are plain C in `port/game/rac3-pal/headers/vu0.h`; (b) matched C with other VU0 inline asm is **translated back from its retail instructions** (`from_assembly`, 767 entries) on the global `openrac_vu0`; (c) the hand-written vector library and all non-matched code go through L8/L9 | L8, L9, L7 (VU0 data memory 0x11004000) | rac1: matched C, helpers as host C (`rac1/host/vector.c`, `common/lib/vu0.c`; `qcopy`/`qzero`), 5 `vcallms` stubs. rac2: hand-written vectors as C on `openrac_vu0` (`handasm.c`), microprogram HLE (`trig.c`) |
| **Scratchpad** | A mapped 16 KB region with a guard; DMA channels 8/9 modelled (`spr_dma.c`); hostgen sends DMA register stores there (`dma_stores`) and takes COP0 waits as done (`dma_done`) | L5, L6, L7 | rac2: the same model. rac1: region only (its DMA code is unreachable) |
| **Program switches** (boot ↔ frontbin ↔ levels ↔ multiplayer) | Overlays by hook; `LoadExecPS2` (`host/exec.c`) longjmps, **reads `I5BOOTN.ELF` from the disc image**, unpacks it over memory, re-registers functions, resets the "kernel" | L11, L13 | rac1/rac2 have no second executable |
| **Online play** | Medius/DME answered at the message level by `mp_net.c` against the OpenRAC server | L17 (IOP network modules faked below it) | — |

### 1.3 How the hand-off works today (all three games)

```
game C (matched or translated)            port host C                        window (C++)
─────────────────────────────            ───────────                        ────────────
DrawFrame passes ───────────────► host/render.c: openrac_game_draw(layers)
                                   put_marker_at(list, 0x574C…)  ──► into the game's VIF1 list buffer
2D routines (wrapped) ──────────► host/ui2d.c: openrac_game_ui_quad(rect) + 0x4E55 marker ──► same buffer
residual 2D routines ───────────────────────────────────────────────► raw GIF packets in the same buffer
sceDmaSend(VIF1, list) ─────────► host/dma.c: CHCR end state, DMAC handlers ──► game_host: g_chain = tag
                                                                       frame.cpp direct_packets(): walk DMA tags,
                                                                       collect DIRECT/GIF + markers + VIF unpacks
                                                                       GifInterpreter: GS registers → draws;
                                                                       markers → world, mobys, UI quads, effects
```

So the native records already exist. What is left PS2-shaped is the **ordering and state carrier**
(the DMA chain and the GS register context) and **the leftovers** that still have no record.

---

## 2. What is already native (do not redo it)

| System | Where | Games |
|---|---|---|
| World geometry from extracted levels: terrain, ties, shrubs, mobys (GPU skinning, bangles, glow tails, metal and chrome maps), sky shells and panorama, collision | `editor/extract.py port`, `editor/port.py` `GAME_FORMATS` (rac3 format 14), `port/scene`, `renderer/world/*`, `renderer/vulkan/*` | all three |
| Visibility, LOD, PVS from the game's own records (occlusion mask, scenery matrices, draw distances); wider pictures | `game/<g>/host/wide_view.c`, `profile.cpp`, `renderer/world/{tfrag_lod,tie_lod}.cpp` | all three |
| Lighting baked at extraction (LightTfrags, LightShrubs, tie colours), point lights and light sets read as data; the enhanced renderer (probe GI, cascades, GTAO, AgX, volumetrics, bloom) | `editor/lighting.py`, `assets/geometry/*lighting*`, `port/enhanced` | all three |
| Particles and effect quads as records (needing only the material fix of L3) | `host/render.c` (`rac3_part_proc`, `rac3_world_quad`), `renderer/effects` | all three |
| Shadow volumes, hero shadow map | `scene/moby_shadow`, `renderer/effects/shadow_volume.cpp` | all three |
| The 2D layer's routines as records: boxes, glyphs (SDF text), sprites, ring segments, Widget3D quads, HUD slots with "HUD: Edges" | `game/<g>/host/ui2d.c`, `hud.c`, `common/lib/ui_quad.h` | all three |
| Menu background and pause pictures kept as window textures (no GS writes) | `host/picture.c` | rac2, rac3 (rac1 boot pictures) |
| Native multiplayer screens and online play through the OpenRAC server | `host/mp_ui.c`, `host/mp_net.c`, `docs/port/MULTIPLAYER_UI.md` | rac3 |
| Movies: MPEG-2 decoder of OpenRAC's own, PSS demux | `port/media` | all three |
| The 989snd sound player itself (grain scripts, priorities, volume groups) and reverb, written as OpenGOAL's 989snd port is | `port/audio/sound_player.cpp`, `reverb.cpp` | all three |
| The WAD decompressor (in memory, not through the scratchpad) | `common/lib/wad_routine.c`, `game/<g>/host/wad.c` | all three |
| Timing: the tick handler, frame pacing at 50/60 Hz, interpolated frames between ticks, input latency work | `game/common/program/vsync.cpp`, `platform/clock.cpp`, `frontend/interpolation.cpp` | all three |
| Input from SDL (keyboard/mouse, controllers, touch, gyro), saves as files | `port/platform`, `common/lib/mc.c` | all three |
| The VU0 helpers of rac3's `include/vu0.h` as plain C | `game/rac3-pal/headers/vu0.h` | rac3 |
| 31+ fastfunc / libvu0 vector routines as C | `game/rac1/host/vector.c`, `common/lib/vu0.c`, `rac3-pal/host/vu0.c` | rac1, all |
| No VU1 microcode runs anywhere, no GS VRAM image, no IPU, no SPU RAM, no IOP CPU | by design (`docs/port/DESIGN.md`) | all three |

---

## 3. The VU0 inline-asm macros: how they compile now, and how they should

The VU0 macro-mode asm in the matched C (`lqc2`, `sqc2`, `vadd`, `vmul`, `vopmula`, `vsqrt`,
`abs.s` as `fabs`, `sqrt.s`, `qcopy`, `qzero`…) is part of the matched source and stays there. The
port must compile it without a VU0.

**Now:**

- **rac3:** the shared helpers of the decompilation's `include/vu0.h` (`vec_add_ab`, `vec_sub_ab`,
  `vec_neg`, `vec_copy`, dot, cross, scale, `abs`, `sqrt`, `plzcw`, `pextlh`…) are already plain
  scalar C in `port/game/rac3-pal/headers/vu0.h`, read before the decompilation's header
  (hostgen.json `headers`). **Native already.**
- **rac3, everything else:** matched C with VU0 inline asm *outside* that header has its asm
  replaced by hostgen with a marker (`=j` constraints, `hostgen/README.md` "Preparing the
  sources"). The function then becomes a stub, which is **translated from its retail instructions**
  (`from_assembly`: 767 rac3 functions, many for this reason), running on the global
  `openrac_vu0` register file. That is VU0 emulation. All untranslated (asm) code uses the same
  model.
- **rac1:** the helpers are host C (`qcopy`/`qzero` in the runtime, `rac1/host/vector.c`); five
  `vcallms` functions are stubs.
- **rac2:** the hand-written vector library is C on `openrac_vu0` (`rac2/host/handasm.c`, which
  keeps `vf3` alive for the cross product); trig is microprogram HLE (`rac2/host/trig.c`).

**Target:**

1. **One shared native vector header** (`port/game/common/include/openrac/vu0_native.h`): a
   `openrac_v4` type on Clang's `ext_vector_type(4)` float (SSE on x86-64, NEON on arm64, no
   intrinsics needed), with the macro-mode operations as `static inline` functions on values
   (`v4_add`, `v4_madd`, `v4_opmula`/`opmsub` as cross product, `v4_clip`, field masks `.xyz` as
   a select). Each game's `headers/vu0.h` maps its helpers onto it.
2. **hostgen lowers inline VU0 asm blocks to C on locals**, not to a stub: each `asm()` block's
   instructions become statements on `openrac_v4` locals named after the `vf` registers, with
   `lqc2`/`sqc2` turned into a 16-byte load and store through the operands. No global register file:
   a block only sees its own registers, which is how GCC compiled it. This shrinks `from_assembly`
   back to the functions that forward registers.
3. **What can only be written against a global VU0** (hand-written routines that leave results in
   `vf` for the caller, `rac2/host/handasm.c`'s `vf3`, rac3's vector library in levels
   `func_L01_0040F0B0`/`…F0D0`) gets host C with explicit parameters and results, one routine at
   a time, through `libraries.tsv` host rows. Then `openrac_vu0`, VU0 data memory and the flags
   model are only reached by translated asm, and they go when that asm gets C (decompilation work,
   section 5, P-X).

---

## 4. rac3: native replacements, as packages

Each package is sized for one agent (Opus for the design ones, Sonnet for the mechanical ones). It
lists its scope, what it reuses, how done is proven, and what it depends on. Packages marked
**shared** change code all three games use; they are written once and each game is moved onto
them. rac1/rac2 follow-ups are listed in section 6.

Proof for every package: `cmake --build` and ctest on box1; `menus.py` for front-end changes;
hidden muted runs (`SDL_AUDIODRIVER=dummy`) on box1 of Veldin (`OPENRAC_RAC3_NEXT=60:1`), Florana,
Holostar 13/14 and Marcadia, and the multiplayer menus. Screenshot comparison only after asking
the user (WORKFLOW: ask before scripted runs and screenshots).

### P1 — The native frame recorder (shared, design; first)

Replace "markers in the game's VIF1 list" with an ordered native command list the host C appends
to directly.

- New C API in `game/common/include/openrac/game_host.h` (`openrac_frame_*`): `begin_frame`, `world`
  (layers, camera), `mobys` (list, count, own camera, target), `ui_quad` (record), `effect_quad`,
  `strip`, `fill` (rect, colour, blend), `clear` (colour/depth), `picture` (texture, rect),
  `snapshot` (keep / draw the frame drawn so far), `target_begin`/`target_end` (an off-screen panel
  by a port id, not a GS block), `scissor`, `end_frame`. The order of calls is the draw order.
- The window consumes it in place of `capture_2d_path`. The existing `UiQuad`, `EffectQuad`,
  particle and strip records are reused as they are. The DirectRenderer's GPU state code (blend,
  depth, double draw) becomes the recorder's back end for flat and textured quads.
- `GifInterpreter` stays behind as the fallback (P8 removes it) so each game moves over piece by
  piece.
- Reuse: `renderer/renderer.h` `FrameInput`, the `ui_kept` generations, `frontend/interpolation.cpp`
  ids, `renderer/direct` draw batching.
- Done when: rac3 Veldin draws identically with the world, mobys and UI records issued through the
  recorder (P2), and a unit test orders a recorded frame without any DMA data.

### P2 — rac3 hand-off onto the recorder (rac3, mechanical; after P1)

- `host/render.c` (`put_marker_at`, `draw_tfrag`, MobyProc, `func_003CB6B8`), `ui2d.c`
  (`openrac_ui_send` callers, the glyph batch markers), `hud.c`, `picture.c`, `movie.c`,
  `mp_maps.c`, `mp_ui.c`: call the recorder instead of writing DMA/GIF marker quadwords into the
  game's list.
- `profile.cpp`: `loading_world` and `level_is_menus` splits decided from recorder order, not from
  marker offsets.
- Done when: rac3 writes no `0x574C…`/`0x4E55`/effect markers (grep), and the front end, a level,
  the loading screen and the multiplayer menus look as before.

### P3 — rac3 residual 2D as records (rac3, implementation; after P1)

- Wrap the routines still sent as GS packets: the screen fade `func_L01_004054F0` / boot
  `func_0038AD28` / frontbin `func_0038A948` (the letterbox records exist; finish the fade
  counters), `func_L01_00407588` (boot `func_0038CB30`), `func_L01_00415788` (boot `func_00392B28`),
  the frame clear and hand-over packet (`0x00142120`, the draw environment), the loading screen's
  stars and galaxy (texture `0x2DAF`), and level 39's and the maps' copies of the same.
- Find and fix Holostar's blue box (levels 13 and 14: a sprite textured from block 0x1E00) and the
  missing bolts count, both of which are in this residual.
- Reuse: `ui2d.c`'s `Rac3Ui2d` copies, `common/lib/ui_quad.h`, rac2's `ui2d.c` for pairs.
- Done when: `OPENRAC_UI_RESIDUAL=1` reports nothing in the front end, Veldin, Holostar and the MP
  menus.

### P4 — Textures by the game's own tables, materials without GS words (shared, design; after P1)

- **Texture identity:** HUD, font, effect and particle textures are loaded from the game's tables
  (the HUD tables at `*D_001D9EA8`/`*D_001D9EAC` and Veldin's `0x001DFD20`/`24`, the GetEffectTex
  records, PartProc's table, the font page getters `func_0038A0E0` / `func_L01_004047A8`) and
  converted to RGBA once (the PSMT8/CSM1 conversion is a data-format conversion and stays, done at
  load or extraction time). They are keyed by (program, table, index), not TBP/CBP. Extract the HUD
  and font banks in `editor/` where they are static.
- **Materials:** `UiQuad`, `EffectQuad`, particles and strips carry a texture handle and native
  enums (blend mode from a fixed set: opaque, alpha, additive, subtractive, modulate-destination;
  alpha-test threshold; depth test and write; filter; wrap), translated once in host C at the
  record's source. `translate_blend(gs::Alpha)` and `gs_pass.cpp`'s `TEST_1` words become that
  translation table, in one place.
- `TexturePool::upload`/`resolve(TEX0)` and the GIF IMAGE path go unused for rac3.
- Decision for the user: PCSX2-format texture packs (`renderer/textures/pcsx2_pack.cpp`) hash GS
  memory data. Either rehash them over the converted textures or drop that compatibility.
- Done when: no rac3 record carries a TEX0/ALPHA/TEST word, and the HUD, text and effects draw
  with no image upload in the frame.

### P5 — Liquid and effect strips without VIF decoding (rac3, implementation; after P1, P4)

- Wrap the strip builders the draw callbacks call (class 6576's draw, level 14 `0x003B24C8`, and
  rac3's counterpart of the liquid grid's packet builder that `frame.cpp` names,
  `build_interpolated_dma_packet` `func_L02_002A52D0`): hand the
  vertices (position, ST, RGBA), the layer's material and the fog line to `openrac_frame_strip`.
  Do not let the game build the VU1 packet.
- Remove the UNPACK/MSCAL branch of `direct_packets` for rac3.
- Reuse: `renderer/effects` liquids pass, `bake-inputs/liquids.json`.
- Done when: level 14's pool, Florana's mud and Marcadia's pool draw from strip records with
  `OPENRAC_DUMP_VIF` unset and no unpacks read.
- **Status (2026-11-25):** the shared record landed: `openrac_frame_strip` / `FrameOp::Strip`, with
  a native material, the image, the fog line and `OPENRAC_STRIP_LIQUID` (FRAME_RECORDER.md 3.2,
  TEXTURES.md 4); `EffectRenderer` draws it like a native quad, with a mip chain on request. First
  producer: the open water plane (`water_plane.c`: Florana, Marcadia, Daxx and the other levels with
  class 2871). Still on the strip routine: class 6576's draw, level 14 `0x003B24C8` and the other
  hand-written strip builders; the UNPACK/MSCAL branch of `direct_packets` stays until they move.
- **Status (2026-11-26, rac3 hand-built strips):** the levels' strip routine (frontbin `func_003D3A24`,
  Veldin `func_L01_00408AAC`, and its setup `func_L01_00407588`) is wrapped in `host/render.c`: the
  capture (`effect_state.c`) now also keeps CLAMP_1, TEX0_1 and TEX1_1, and when TEST_1, ALPHA_1, CLAMP_1,
  TEX0_1 and TEX1_1 were set by the caller with nothing appended since, the strip is recorded with
  `openrac_frame_strip` from the routine's own arrays (view context fog line and FOGCOL), so every caller
  of the routine (class 6576's draw, level 14's `0x003B24C8` = `func_L02_003A2EC8`, the glints and
  ribbons) goes native without a caller of its own changing; the game's routine runs when the state is
  not proven, the texture is not an effect texture or the window refuses. Depth follows the chain's rule
  through `OPENRAC_STRIP_CHAIN_DEPTH` (written only where the window places the strip with the
  liquids). `OPENRAC_RAC3_STRIP_TRACE` counts. The UNPACK/MSCAL branch stays for rac1, rac2 and the
  strips the proof refuses.

### P6 — Screen effects and frame copies natively (rac3, implementation; after P1)

- The screen glow (`0x001DBCA0`, `func_L01_004DE670`/`…6F0`) and the second effect (`0x001DBCBC`,
  `func_L01_004DF310`): native post passes (downsample, blur, tint, add) with the game's enable
  words and parameters read from the prebuilt packets' constants. They are no longer left out.
- Frame copies: the pause tiles (`func_L01_004C5F70`/`…6030`), the main menu picture
  (`func_003A9BE8`) and any frame-textured sprite (Holostar) use `openrac_frame_snapshot`/`picture`.
  `sceGsExecStoreImage`/`LoadImage` into frame-buffer blocks are answered without `FrameSnapshot`'s
  block hashing.
- Done when: the glow is visible in a level that sets it, and no rac3 path reads or writes a
  frame-buffer block address.

### P7 — rac3 VIF1/DMAC hand-off removed (rac3, mechanical; after P2, P3, P5, P6)

- `sceDmaSend(VIF1)` (`host/dma.c` `func_0012D938`) becomes only the tick boundary: no chain kept,
  no CHCR end state. The frame's wait mask is retired by wrapping the VIF1 handler (`0x003A9EF8`)
  and the wait (`func_003A96A8`) natively. KickVif0Chain (`func_003A9348` and copies) does nothing.
- `profile.cpp` `sends_display_list` off for rac3; `GifInterpreter` is not constructed for it;
  `chain_census.c`, `OPENRAC_RAC3_CHAINS` and `OPENRAC_UI_REPLAY` for rac3 removed.
- `sceGsPutDispEnv`/`PutDrawEnv` (`common/lib/graph.c`) only report the display size and the
  frame's height to the window; no GS register decoding beyond that.
- Done when: rac3 runs the front end, a level, the loading screen and MP with `direct_packets`
  never called (assert), and `GUEST_GS_REGS` unmapped for rac3.

### P8 — The GIF interpreter retired (shared, mechanical; after P7 and the rac1/rac2 follow-ups R1–R3)

- Delete `renderer/direct/direct.cpp`'s GIF parsing (`GifInterpreter::gif`, `vif`, register
  writes, IMAGE), `gs.h`, `direct_packets`, the TBP-resolve and upload paths of `TexturePool`, and
  the GS-state translation in `renderer/vulkan/graph/graph_state_translation.cpp`. Keep the quad
  batching and GPU state code under the recorder.
- Done when: no file includes `renderer/direct/gs.h`, and the three games' ctest and menu walks
  pass.

### P9 — Scratchpad DMA and the DMAC model removed (rac3, implementation; independent of P1–P8)

- List every hostgen `dma_stores` site and every `bc0t` wait in rac3 (the boot program, frontbin,
  levels, MP): the joint pose kernel `0x003CA858`/`0x003CA9C0`, `coll_line`, MobyProc's light-bank
  toSPR (0x50 qwords), PartProc's texture table, Widget3D's vertices `func_003A08E8`/`…0A28`, the
  ~20 others.
- Replace each with host C that does its job on memory (`libraries.tsv` host rows), as
  `wad_routine.c` did for the WAD decompressor. Where a kernel is too big for that now, hostgen
  rewrites the idiom *write MADR/SADR/QWC; write CHCR.STR; poll* into one `memcpy` at translation
  time, with no register-page state (a narrow pattern in `asmc.py`, tested).
- `coll_line`: bound the cell gather natively and drop `GUEST_SCRATCHPAD_GUARD` and
  `take_scratchpad_overrun`.
- The scratchpad stays as 16 KB of plain game working memory at its address, because the game C
  names 0x70000000. That is data memory, not hardware; nothing behind it is modelled.
- Delete `host/spr_dma.c`; set `asm.dma_stores`/`dma_done` false.
- Reuse: rac2's `spr_dma.c` findings and `handasm_render_big.c`; `openrac_game_moby_chain` stays as
  the native pose provider.
- Done when: rac3 runs with `openrac_ee_sw_dma` removed, and a store to `0x1000D000`/`D400` faults.

### P10 — VU0: microprograms by name, inline asm as C (rac3 + shared hostgen; independent)

- **P10a microprograms (rac3):** the callers of `vcallms` (`func_0038D478`, `func_0038D490`,
  `func_0038D6E8`, `func_0038D708`, `func_L01_0040F0B0`, `func_L01_0040F0D0` and the MP copies) get
  host C with explicit arguments and results (cos, sin, Euler rows, quaternion rows), so
  `asm.vu0_programs` and `rac3_vu0_*` on `OPENRAC_VU0->vf` go. Sine and cosine: the series with the
  game's documented order (as `common/lib/vu0_sine.c` does for rac1/rac2) where the last place
  matters, with the constants as numbers rather than read from the game's microcode at run time
  (a decision for the user: see section 7).
- **P10b inline asm lowering (shared, hostgen):** section 3, items 1 and 2: `vu0_native.h` and
  hostgen's lowering of `asm()` blocks to `openrac_v4` locals; rerun rac3's `from_assembly`
  classification and drop every entry that was there only for VU0 asm.
- Done when: rac3's `asm.vu0_programs` is empty, `from_assembly` lists only register-forwarding
  wrappers, and `hostgen`'s report shows no matched C function translated because of VU0 asm.

### P11 — Sound at the 989snd API and a native mixer (rac3 + shared; independent)

- **P11a (rac3):** cut at the 989snd EE API, as rac1 does (`common/lib/snd.c`): bind the client's
  entry points (snd_PlaySound…, the bank loads, the stream calls; rac3-decomp core
  `0x0013B330`–`0x0013D420`) to the engine directly and delete the command ring path for rac3
  (`host/sound.c`'s `snd_SendCommandSync`/`QueueCommandToRing`/`snd_Pump` answers, the reply word,
  `snd_ring.c` use). Commands 0x2C/0x30 become stream API calls. Then do the same for rac2.
- **P11b (shared):** replace the SPU2 voice model (`audio/spu.cpp`: the chip's Gaussian table,
  ADSR register words, pitch words, 48-voice limit) with a native mixer: samples decoded once to
  float, a standard resampler, envelopes converted from the bank's ADSR fields to times and levels
  at bank load, a float mix with one limiter. The voice limit and priorities stay where 989snd's
  logic needs them (`sound_player.cpp`), as policy, not chip channels.
- Done when: no rac3 path calls `sceSifCallRpc` for sound, and the audio tests and a muted capture
  (`OPENRAC_AUDIO_DUMP`) of Veldin's opening match by ear (the user listens).

### P12 — Kernel and IOP shims out (rac3, implementation; independent)

- **Interrupts:** the VBlank callback (`func_0013D208`) and the DMAC handlers are called as named
  tick hooks of the port, not through INTC/DMAC handler tables (`common/lib/kernel.c` keeps the
  table only for games not moved yet).
- **IOP:** find what rac3 parks in the IOP stash (server 0x11, `sceSifSetDma`) and give those call
  sites a host buffer API; remove `iop_memory`, the IOP heap, module loads and `sceSifCallRpc` for
  rac3 (after P11a, sound no longer needs it).
- **LoadExecPS2:** switch between the boot and multiplayer programs natively: both payloads are
  unpacked at build or extract time (`retail.py` `loader_parts` already does it for translation),
  stored as the port's own program images, and started without reading `I5BOOTN.ELF` from
  `disc.iso` at run time. The argument block (`BOPT=…`) is passed as data. `host/exec.c`'s
  longjmp stays as the program switch.
- **Register pages:** after P7 and P9, unmap `GUEST_EE_REGS`, `GUEST_VU_MEM` and `GUEST_GS_REGS`
  for rac3; fix what faults (T0 writes through `func_00125090`, timer reads). The uncached RAM
  aliases stay as pointer aliases (they are the game's address arithmetic, not hardware).
- **Network:** cut below the game's Medius layer only; remove the ezNetCnf/ezNetCtl/MSIF answers
  where the game's own code no longer reaches them.
- Done when: rac3 boots, goes to multiplayer and back, and saves, with the register pages
  unmapped, and `kernel.c` reports no handler-table dispatch for rac3.

### P13 — Files instead of disc sectors (shared; independent)

- `common/lib/cdvd.c`: build a sector→file map from the ISO's directory at extraction time and read
  each requested LBA range from the extracted files under `iso_data/<game>/` (the WADs, the level
  files, the stream files). `disc.iso` is no longer opened at run time. Movies (`program/movie.cpp`)
  and VAG streams (`sound_engine.cpp`) read through the same map. Edited-level patches and virtual
  streams stay as overlays on the map.
- Done when: a run with `disc.iso` removed (only the extracted files present) plays the three
  games' intros and loads a level.

### P14 — Pads and cards above the libraries (shared; low priority, independent)

- Pads: write the game's own pad state block from `platform/input` (rac3's reader `func_003A31C8`
  and state machine `func_003A3B40` wrapped) instead of building libpad's DualShock 2 report bytes.
- Cards: keep the game's save bytes and folder names (players' saves stay compatible), but answer
  the game's save and load routines directly instead of modelling libmc's command and `sceMcSync`
  polling.
- Done when: `pad.c`/`mc.c` are reached by no rac3 call (`libraries.tsv` rows replaced by wraps).

### P15 — Native float in the renderer and assets (shared, decision first)

- Replace `renderer/world/vu_float` (the GLSL `vu_mul`/`vu_add`), `fog.cpp`, `tie_lod.cpp`, and the
  `ps2_float` arithmetic in `assets/geometry/*` and `game/common/moby_proc.cpp` with IEEE float.
  Expect last-place differences against retail captures; the tests that pin byte parity change to
  tolerances.
- Not the game code: hostgen's `ps2_float` lists (functions whose float results the game cuts to
  integers each tick, rac2's vector leaves) and the EE's round-toward-zero and flush-to-zero mode
  are CPU arithmetic, not hardware. They keep gameplay on retail's path. See section 7.

### Ordered list (with what can run at once)

| Order | Package | Kind | Depends on | Machine for proof |
|---|---|---|---|---|
| 1 | **P1** native frame recorder | shared, design (Opus) | — | box1 |
| 1 | **P9** scratchpad DMA / DMAC model out | rac3 (Opus) | — | box1, box2 |
| 1 | **P10a** VU0 microprograms by name | rac3 (Sonnet) | — | box1 |
| 1 | **P13** files instead of disc sectors | shared (Sonnet) | — | box1 |
| 2 | **P2** rac3 hand-off onto the recorder | rac3 (Sonnet) | P1 | box1 |
| 2 | **P4** textures by table, native materials | shared, design (Opus) | P1 | box1 |
| 2 | **P10b** inline VU0 asm lowered to C | shared hostgen (Opus) | — (after P10a for rac3's list) | box2 (hostgen runs) |
| 2 | **P11a** rac3 sound at the 989snd API | rac3 (Opus) | — | box1 |
| 3 | **P3** rac3 residual 2D as records | rac3 (Opus) | P1, P2 | box1 |
| 3 | **P5** liquid strips without VIF | rac3 (Sonnet) | P1, P4 | box1 |
| 3 | **P6** screen effects and frame copies | rac3 (Opus) | P1, P2 | box1 |
| 3 | **P11b** native mixer instead of the SPU model | shared (Opus) | P11a | box1 |
| 4 | **P7** rac3 VIF1/DMAC hand-off removed | rac3 (Sonnet) | P2, P3, P5, P6 | box1 |
| 4 | **P12** kernel and IOP shims out | rac3 (Opus) | P7, P9, P11a | box1 |
| 5 | **P8** GIF interpreter retired | shared (Sonnet) | P7, R1–R3 | box1 |
| any | **P14** pads and cards above the libraries | shared (Sonnet) | — | box1 |
| after decision | **P15** native float in renderer/assets | shared (Sonnet) | user decision | box1 |
| ongoing | **P-X** decompile what drags in L5/L8/L9 | rac3-decomp matching | — | swarms |

At most 3–4 agents at once (WORKFLOW). Wave 1 is P1, P9, P10a and P13, all independent.

---

## 5. What this plan cannot remove by itself: the recompiled retail code (L9)

About 90 % of rac3's code (by bytes) has no C and runs as hostgen's translation of the player's
own retail instructions. That is CPU code, not hardware, and it is the game's own logic. But it is
"from the PS2" in the strictest reading, and it is what keeps L5 (COP0 DMA waits, CHCR polling),
L7 (VU0 data memory) and L8 (the global VU0 register file, MAC/clip flags) alive. The packages
above remove the hardware layers at the boundary. They cannot make every translated function plain
C.

**P-X (rac3-decomp, ongoing):** prioritise decompiling the families that need the hardware idioms,
so P9 and P10 need no hostgen patterns. That is the hand-written scratchpad kernels, the vector
library in every level's copy, and the joint pose kernel. Their lists come out of P9 and P10a. This
is matching work under rac3-decomp's rules (WORKFLOW "Matching"), not port work.

---

## 6. rac1 and rac2 follow-ups (so P8 can delete the shared layers)

| | rac1 | rac2 |
|---|---|---|
| **R1** recorder hand-off (as P2) | `rac1/host/render_native.c`, `ui2d.c`, `hud.c`, `hud_panels.c`, `boot_picture.c`, `water_surfaces.c`, `effect_quad.c`, `rac1-pal/host/water.c` (the GS NOP that "travels with that chain") | `rac2/host/render.c`, `ui2d.c`, `font.c`, `hud.c`, `picture.c`, `movie.c`, `mod_pages.c` |
| **R2** residual 2D (as P3, P6) | FadeToBlack `func_001F4E08`, letterbox `func_001F5148`, screen effects `func_001F5368`, Z clear `func_001FB908`, render to texture `func_001FB608`, `AA_BlurPass` `func_001FB848`, the frame snapshot | the title and credits pictures, the Ship Shack panels (off-screen targets by block), the cockpit screen's 256×256 target (block 0x1DF) |
| **R3** strips (as P5), DMA hand-off out (as P7) — **blocked on P4/P5/P7 (2026-11-25, see below)** | Oltanis ice and Hoven (`draw_textured_effect_list`, shared L12/L14 code), Novalis ripples (`update_and_draw_scrolling_overlay_layers` and the wave builder), all ending in `DrawEnvOverlayMesh` (`func_L00_001FDE48`); `shown_chain` double buffer | lava `func_00303070` is **already native** (it draws quads through `draw_world_quad`, `rac2/host/render.c` `world_quad`); sea and the other water strips still come from the chain; `rac2/host/spr_dma.c` **gone** (P9, #771; CollLine is `rac2/host/coll_line.c`); `handasm_vu0.c` DMA waits are report-only fragments already; `rac2/host/dma.c` stays until P7/P8 |
| VU0 (as P10) | 5 `vcallms` stubs; `vu0_sine.c` microcode constants | `vu0_programs` 0xC80/0xC90/0xD18 (`trig.c`), `handasm.c` vectors on `openrac_vu0` |
| Sound (as P11a) | already at the API | the same ring as rac3: do it right after rac3's |

R3 findings (2026-11-25, read from develop f7e809b0a):

- **No shared strip hand-off exists yet.** `openrac_frame_strip` and the material for strips are
  P4/P5 work (claimed by the p456 lead); rac3's P5 never landed (#790 is a WIP merge: no
  `host/strips.c`, no `FrameOp::Strip`), and P7 is the same (`rac3-pal/host/dma.c` still calls
  `openrac_game_dma_send`). `docs/port/FRAME_RECORDER.md` says it: "the effect strips still come
  from the chain until then". R3 reuses that API, so it waits for it.
- **What a strip hand-off needs for rac1.** The strips end in `DrawEnvOverlayMesh` (`func_L00_001FDE48`
  PAL: copies xyz, RGBA and ST arrays into an UNPACK packet; the VU1 program does the rest). The GS
  state each strip draws with is not in that call: the callers write it into the chain first (Novalis:
  ALPHA and two effect textures in `update_and_draw_scrolling_overlay_layers`; Oltanis and Hoven:
  `vu1_add_g_sregister(6/7)` and the static packet `D_L12_001CB7A0`, plus the template qwords at
  `D_L00_00160F80/A0/C0`: PRIM with fog, context and second-texture bits). The wrap therefore
  has to be at the callers (matched C in the decomp, so copyable) and give `openrac_frame_strip` the
  material, the fog line and the effect texture's pixels (`openrac_effect_texture`, as rac1's quad
  routine does), anchored with `openrac_frame_anchor_at` at the level chain cursor so the strip
  keeps its place before the moby pass. Novalis' enhanced-water tags (`water.c`, surface 1-7, UV
  motion) move onto the same record.
- **rac2.** The surface grid (`func_00303070`) needs nothing: it already reaches the window as
  effect quads. Its strips are the weapon and beam routines (`gfx_draw_twin_beam_strips`,
  `gfx_draw_ribbon_strip`, `gfx_draw_wavy_link_strip`, `blast_draw_ripple_mesh`) and a level's
  water drawn between `func_0027CEE0` and `func_0027CF58`; the audit's "sea sheets" are the latter and
  still need the same API.
- **Order for whoever takes it:** P4/P5's strip record and material (rac3 first, it has the claim),
  then rac1 Oltanis/Hoven (one wrapper per version, `draw_textured_effect_list`), Novalis, then rac2's
  level water; the DMA hand-off (`dma.c`, `shown_chain`) goes with P7/P8, after every chain consumer
  is native. Check each with a hidden before/after run on box1 (Oltanis, Novalis; a lava and a sea
  level in rac2), frames compared.

---

## 7. Decisions for the user

1. **Game-logic float fidelity.** The EE's round-toward-zero and flush-to-zero (a host FPU mode,
   `runtime/ee.cpp`) and hostgen's software `ps2float` for listed functions keep the game C's
   arithmetic on retail's path (rac2's hero drifted off within a few hundred ticks without them).
   They model the EE's FPU, not a device. Proposal: keep the FPU mode, keep `ps2_float` only for
   game functions where a measured divergence shows, and make the renderer and assets IEEE (P15).
2. **Microcode-derived constants.** `common/lib/vu0_sine.c` (rac1, rac2) reads the sine series'
   constants from the game's own VU0 microprogram at run time, so no microcode bytes are
   committed. A native version needs either those four constants written as numbers (they are
   numbers, not code, but they come from the microcode) or a series of the port's own (last-place
   differences against retail).
3. **PCSX2-format texture packs** (`renderer/textures/pcsx2_pack.cpp`) are keyed on GS-memory data.
   Rehash them over the converted textures (P4), or drop the format.
4. **The scratchpad and RAM layout at console addresses.** The pointer model (game memory as one
   reservation whose offsets are the console's addresses, `DESIGN.md` item 7) is how the matched C
   runs unchanged, as OpenGOAL does it. The plan keeps RAM and the 16 KB scratchpad as plain data
   memory at those addresses and removes everything modelled behind them (register pages, DMA,
   guard, VU memory). Moving the scratchpad elsewhere would mean editing matched C.
5. **Debug tools that replay the PS2 path** (`OPENRAC_UI_REPLAY`, `OPENRAC_DUMP_VIF`,
   `OPENRAC_RAC3_CHAINS`) go with P7/P8. They were for comparing native against the game's packets.
