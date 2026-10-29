# FSR temporal upscaling

Enhanced Vulkan rendering uses AMD FSR 3.1.4 to reconstruct the world before drawing the HUD at
display resolution. It includes temporal upscaling and sharpening, with no frame generation.

## Interface

- `fsr3.h`: frontend-owned upscaler, displayed poses and synchronous frame hooks.
- `sdk_context.h`: SDK context and imported colour, float depth, motion and reactive images.
- `temporal_math.h`: render extents and camera inversion.

The frontend requires enhanced graphics and compatible probe data. OpenGL, unsupported devices
and builds without the upscaler retain native rendering.

## Depends on

The renderer, scene, Vulkan backend and MIT-licensed AMD FidelityFX SDK 1.1.4. CMake pins commit
`c6efa6bf7f2027b3ec94f28578bb5965eabb9e55` and verifies the download hash. Existing glslang compiles
the SDK's GLSL; no extra shader compiler is required. SDK files stay in the build tree and are
not needed at runtime.

`prepare_sdk.py` embeds the shader dependency closure and corrects Vulkan integration details:
the core memory-requirements entry point and Float16 feature query, unified-memory allocation requiring all requested flags,
the FP16 luma-history image declaration, and the unused frame-generation swapchain callback.
AMD licence notices remain in generated sources.
The generated upscaler header also sizes context storage for the platform's wide characters;
the SDK and its callers use this same header. `sdk_compat.h` supplies missing standard includes
and bounded diagnostic formatting on non-Windows platforms.

## Mixed precision

Supported Vulkan devices use the SDK's optional FP16 history-colour sampler in the accumulation
passes (`FFX_HALF=1` and `FFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1`). Both
`shaderFloat16` and `shaderInt16` must be enabled; input extents must fit the sampler's signed
16-bit indexing. Unsupported devices use FP32. `prepare_sdk.py` queries the promoted Vulkan 1.2
Float16 feature even when the old extension name is absent.

The pinned half pyramid callbacks are incomplete, so luma/shading-change pyramids remain FP32.
RCAS, reprojection coordinates and temporal accumulation also remain FP32. Resource formats and
storage layouts are unchanged. The runtime log reports actual SDK pipeline selection.

`OPENRAC_RENDER_OPT_OFF=fsr-float16` forces only FSR history filtering back to FP32 for comparison;
`shader-float16` disables the renderer's Float16 feature globally. Changing precision recreates
SDK histories. FP16 filtering introduces small numerical differences, particularly on thin detail.

## State and threads

One `Fsr3` belongs to the frontend's graphics owner: the window thread in synchronous mode,
or the worker during world recording. It owns world colour and depth, velocity and reactivity
targets, SDK histories and previous displayed camera/object poses.
The renderer lends its existing command buffer and images. There are no per-frame CPU readbacks,
extra queue submissions or GPU waits. Reconfiguration and destruction wait before freeing SDK data.

Static geometry and sky use depth-derived camera motion. Opaque live mobys overwrite it using
current and previous rigid transforms and skinned joints. Material alpha determines coverage.
A preparticle colour snapshot supplies reactivity for blended and additive effects.

History resets after resolution or level changes, skipped world frames, large camera changes and
explicit invalidation. New or teleported objects reject old history. The HUD and menu snapshot
follow reconstruction. The saved MSAA choice is restored when FSR is inactive.

## Errors

Setup returns `false` with a diagnostic. A dispatch failure presents a spatially scaled world and
native HUD, logs the error, and stops further temporal dispatches until the upscaler is recreated.
Unsupported builds expose no active FSR mode and allocate no SDK resources.

## Configuration

`OPENRAC_FSR3` builds the SDK only with `OPENRAC_ENHANCED_RENDERING` and `OPENRAC_VULKAN`. It defaults
to the enhanced build option. Set it to `OFF` to omit the dependency. For offline builds,
`FETCHCONTENT_SOURCE_DIR_FSR3_SDK` names an extracted copy of the pinned archive.

The Enhanced menu and launcher expose `temporal_quality`: `off`, `native-aa`, `quality` (default),
`balanced`, or `performance`. Display-to-render ratios are respectively 1x, 1x, 1.5x, 1.7x and 2x,
within the existing render-scale setting. `temporal_sharpness` ranges from 0 to 1, default 0.2.
See [settings](../../../docs/port/SETTINGS.md) for the enhanced master switches.

## Tests

- `fsr3_shaders`: FP32/FP16 variants, typed FP16 SPIR-V arithmetic, unchanged descriptors and pyramid fallback.
- `fsr3_sdk`: uniform RGB, moving-detail FP32/FP16 comparisons, sharpening, history reset and precision changes on the GPU.
- `fsr3_frame`: preset extents, camera inversion and native-pixel HUD coverage.
- `fsr3_motion`: rigid/skinned velocities, history rejection and exact batched/individual coverage,
  including alpha cutouts and glow exclusion.
- `python -m unittest discover -s port/enhanced/fsr3`: source embedding and repeatable preparation.

GPU fixtures use Vulkan validation and no game data.

## Limitations

Input colour is LDR. Translucent objects and particles have reactive coverage rather than individual
motion vectors. Animated sky shells use camera motion. Dynamic velocities upload current and previous
poses together, then instance consecutive objects sharing one geometry/material run. Multi-material
objects retain instance-major ordering to preserve equal-depth alpha coverage. The individual-draw
reference is the registry's `fsr-motion-batch` switched off (`OPENRAC_RENDER_OPT_OFF=fsr-motion-batch`,
[RENDER_PERF.md](../../../docs/port/RENDER_PERF.md#fsr-motion-batch-object-motion-in-batches)).

## Late world effects

After temporal resolve clears HUD depth, `late_world_depth.frag` restores display-resolution
world depth for late effects such as pickup glimmers, compensating raster jitter. HUD packets
retain their GS depth state; `before_hud` runs after restoration. `fsr3_frame` checks behind-wall
occlusion, front visibility and exact HUD coverage in Quality, Performance and Native AA.

## Further reading

[AMD's integration guide](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/super-resolution-upscaler/).

## Camera-target-relative motion blur

Enhanced Settings offers **Motion blur** (off by default) and **Blur strength**
(0–1, default 0.5). Gameplay uses the game camera rig's smoothed tracking focus as
its translation reference, rather than Ratchet's body or the orbiting camera eye.
Ratchet can jump relative to that focus without applying his full vertical velocity
to the scenery. Scenery still responds to actual focus movement. Manual camera
rotation/orbit does not itself move the reference. Gameplay sky and the HUD stay sharp.

RAC1's reader follows the active camera pointer at eye address +0x40, checks the
default follow record's id (+0x86 == 0), then reads its saved state (+0x70) and focus
(+0x40). The layout is shared by PAL/NTSC and their per-level address tables. Other
camera records are left unknown rather than interpreting a different saved-state
layout. Camera record/state identity changes, missing data, nonfinite values and
large target jumps suppress gameplay blur until matching history returns. There is
no fallback to Ratchet's root. The focus is captured on the game thread and interpolated
with the displayed camera, then compared with the previous displayed focus.

Each depth pixel projects a hypothetical previous position travelling with that focus
through the previous camera. Subtracting this reference velocity from FSR motion handles
perspective and depth without subtracting a single screen-space vector everywhere.
Opaque object and skinned motion remain; transparent effects have no independent velocity.

Real-time RAC1 cutscenes (level mode 2) use normal camera-relative blur from the original
FSR vectors, including camera/object movement and sky rotation. No focus or visible hero
is required. Both modes share the controls. Mode transitions skip one blur frame without
resetting FSR history; camera cuts retain their usual reset. Prerendered movies are unchanged.

Blur runs after reconstruction, leaving SDK input/history unchanged. With FSR Off it
selects Native AA; disabling blur restores the saved Off choice. This uses single-sample
rendering while active. Unsupported builds retain native rendering. The dedicated Motion Blur section exposes persistent exposure, radius, sampling,
minimum-motion, JFA spread, depth tolerance and cutscene multiplier controls; see
[settings](../../../docs/port/SETTINGS.md). Strength 1 is a 1/60-second exposure and 0.5 is
1/120 second, normalized by the displayed-frame interval. Trails default to a 128-display-pixel cap, adjustable from 1 to 128 pixels. Jump-flood velocity dilation is adapted from SphynxMotionBlurToolkit's
`jfp_simple.glsl` at commit `d0c16fc1581eb757e993ff6eaf5877078c8d24cd` (MIT; full notice
in [THIRD_PARTY_NOTICES.md](../../../THIRD_PARTY_NOTICES.md)). The Godot wrappers,
experimental history filter and demo assets are not imported.

Every moving render pixel initializes its source coordinate; stationary pixels use an
explicit invalid seed. The first flood generates seeds directly, avoiding a separate initialization pass.
Two R32UI targets pack exact 16-bit X/Y coordinates in four bytes per pixel and
ping-pong through eight-neighbour passes at
power-of-two jumps down to one pixel, covering the configured display-pixel blur radius. Seed
coordinates stay exact even above 2048 pixels. Candidate selection follows the upstream
velocity-space reach/side tests and gives nearer depth priority over background speed.
The final gather samples the propagated source directly as well as its velocity trail,
so a thin source cannot fall between all reconstruction taps. No tile-max reduction
precedes JFA. The flood starts from implicit per-pixel seeds every frame, including after resize;
previous seed contents are never read by the initial pass. Duplicate candidates and
candidates behind an accepted foreground source are rejected early. The final gather
discards stationary fragments, preserving the existing display without fetching or
rewriting their colour. Resolution, sample count and blur selection rules are unchanged.
`motion blur JFA passes` records the pass count in frame diagnostics.

The full-resolution flood costs extra bandwidth and render passes. This remains an
approximate single-frame filter: hidden surfaces and complicated overlapping motion can
still show artifacts; JFA does not reconstruct unseen background.

`camera_target` checks focus reads and invalid data for both versions and all levels.
`motion_blur` covers target identity, jumps with a stationary focus, eye-motion independence,
normal cutscene blur, different receiver depths, exposure, foreground occlusion,
thin opposing motion, multi-jump radius/edge coverage and stale-seed rejection.
`fsr3_frame` checks HUD preservation, mode transitions and the Native AA fallback;
`settings` checks persistence and invalid values.


The `motion-blur-fast-path` optimisation retains the reference path for exact comparisons
(`OPENRAC_RENDER_OPT_OFF=motion-blur-fast-path`). It unrolls the eight neighbours in the
same order, caches a repeated rejected source and omits the redundant minimum-speed
check on seeds that passed it in the initial flood. Candidate fitness, depth priority,
resolution, radius and sample counts stay unchanged.

On eligible frames FSR writes directly to the blur's storage-capable colour input. The
final gather also copies unchanged pixels into the display, eliminating the separate
full-size snapshot blit. Reset, mode-transition and unknown-target frames still dispatch
straight into the normal display. If the blur fails after reconstruction, its unblurred
input is presented before the HUD. There is no CPU readback or additional history.

The `motion_blur` regression compares 36 complete synthetic images byte-for-byte against
the reference, including dense motion/depth discontinuities and odd 4099x11 dimensions.
`fsr3_frame` compares 13 complete reconstructed world/HUD frames across quality changes,
Native AA, output replacement and cinematic transitions. Both paths use Vulkan validation.


Later jump-flood passes prefetch all independent seed coordinates before the dependent
motion/fitness lookups. For interior pixels at stride one, four integer texture gathers
supply the same centre and eight neighbours; quarter-texel coordinates avoid selection
boundaries. Image edges use the original bounds-checked integer fetches. The implicit
initial-seed pass keeps its direct candidate evaluation. All candidates are considered
in the original order, preserving depth/fitness ties exactly. No passes, neighbours,
resolution or precision are removed. The same pixel-exact regressions cover both paths.
