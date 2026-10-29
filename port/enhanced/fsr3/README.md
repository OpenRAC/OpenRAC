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

Motion blur is on by default. Toggle it in Enhanced Settings or the launcher's enhanced graphics
settings. Gameplay blur is relative to the camera rig's smoothed tracking focus: jumping and
orbiting the camera do not apply Ratchet's full motion to the scenery. The HUD and gameplay sky
stay sharp. Real-time RAC1 cutscenes use camera-relative blur, including sky rotation.
Prerendered movies are unchanged.

The Motion Blur section controls exposure, maximum trail length, samples, minimum motion,
trail spread, depth tolerance and the cutscene multiplier. Strength 1 is a 1/60-second exposure;
the default 0.5 is 1/120 second, adjusted for the displayed-frame interval. Trails default to
128 display pixels. Ranges and defaults are in [settings](../../../docs/port/SETTINGS.md).
With FSR Off, enabling blur selects Native AA; disabling it restores the saved Off choice.
Unsupported builds retain native rendering.

The game thread reads the active follow camera's focus and record/state identity for both PAL
and NTSC, then interpolates it with the displayed camera. Missing or invalid focus, identity
changes and large jumps suppress gameplay blur until matching history returns. Other camera
records are not interpreted as follow cameras, and there is no fallback to Ratchet's root.
Mode transitions skip one blur frame without resetting FSR history; camera cuts reset it normally.

Each depth pixel projects a previous position travelling with the focus through the previous
camera. Subtracting this reference from FSR motion preserves perspective, depth and opaque
object/skinned motion. Transparent effects have no independent motion vectors.

Blur runs after reconstruction, leaving SDK input and history unchanged. Jump flooding carries
source coordinates through eight neighbours at decreasing power-of-two jumps. Two R32UI targets
pack exact 16-bit X/Y coordinates, including above 2048 pixels. The first pass initializes seeds
implicitly; stationary pixels are invalid seeds. Candidate selection uses velocity-space reach,
side allowance and foreground-depth priority. The final gather includes the exact propagated
source so thin features cannot fall between trail samples.

The jump flood is adapted from SphynxMotionBlurToolkit's `jfp_simple.glsl` at commit
`d0c16fc1581eb757e993ff6eaf5877078c8d24cd` (MIT; full notice in
[THIRD_PARTY_NOTICES.md](../../../THIRD_PARTY_NOTICES.md)). Its Godot wrappers, experimental history
filter and demo assets are not imported.

`motion-blur-fast-path` is on by default. It preserves candidate order and depth/fitness ties,
rejects duplicate candidates, prefetches seed coordinates and uses integer gathers for interior
stride-one pixels. On eligible frames, FSR reconstructs directly into the blur input and the
resolve writes unchanged pixels too, avoiding a separate full-size colour copy. Reset,
mode-transition and unknown-target frames reconstruct into the normal display. If blur fails,
its unblurred input is presented before the HUD. The reference path remains available with
`OPENRAC_RENDER_OPT_OFF=motion-blur-fast-path`; there is no CPU readback or extra history.
`motion blur JFA passes` reports the flood pass count in diagnostics.

The full-resolution flood adds bandwidth and render passes. This single-frame filter cannot
reconstruct hidden background; overlapping motion and occlusion can still show artifacts.

`camera_target` checks focus reads and invalid data for both versions and all levels.
`motion_blur` covers focus identity, jumps, eye-motion independence, cutscenes, depth, exposure,
foreground occlusion, thin opposing motion, radius/edge coverage and stale seeds. It compares
36 complete synthetic frames against the reference, including odd 4099x11 dimensions.
`fsr3_frame` compares 13 world/HUD frames across quality changes, Native AA, output replacement
and cinematic transitions. Both GPU suites use Vulkan validation. `settings` checks persistence
and invalid values.
