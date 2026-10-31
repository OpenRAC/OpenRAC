# Unified atmosphere and fog

Enhanced Vulkan rendering uses one camera-aligned transport grid for authored aerial haze,
height fog and up to eight spherical local fog volumes. Original graphics, OpenGL, unbaked
lighting, shader failures and grids exceeding device buffer limits retain the original fog path.
Selecting a smaller supported tier can restore volumetrics without a reload. Enhanced Settings
exposes quality and tuning controls and a reset button; quality is saved as `volumetric_quality`, while other tuning is
session-only. No new level assets or probe bake format are required.

Surfaces, water and particles share medium integration and depth-addressable transport. A
separate fog-only result composes height and local fog over the already shaded sky.

## Rendering

The authored fog's transmission curve is converted into extinction over each segment.
Height and local fog add extinction and colour-weighted scattering before front-to-back
integration. The existing primary sun and its cascaded shadow maps light the medium; volume
samples use no surface normal offset or receiver-plane slope. The existing eight point lights
also illuminate fog, using their authored range falloff. Their visibility is not shadowed.

Quality changes sample density in all three dimensions, preserving medium coefficients.
Off is the default while the feature is experimental. Medium retains the previous
implementation's sample density when explicitly enabled. The launcher does not expose
atmosphere controls; tuning remains available in the in-game enhanced developer panel.

| Quality | Pixels per column | Maximum columns | Depth slices | Maximum grid allocation |
| --- | --- | --- | --- | --- |
| Off | No dispatch | None | None | Dummy descriptor only; original fog |
| Low | 64 | 48 x 27 | 24 | 1.9 MiB |
| Medium | 32 | 96 x 54 | 48 | 15.2 MiB |
| High | 16 | 160 x 90 | 96 | 84.4 MiB |

Depth segments are logarithmically distributed. Each cell stores combined and fog-only
accumulated light/transmission, plus source/extinction, as four RGBA32F records.
Changing quality reallocates the grid and updates every consumer's depth mapping.
Off releases the volume allocation and skips atmosphere shadow preparation/evaluation.
Phase evaluation and bounded sunlight/point-light colour modulation use FP16 arithmetic when
the existing Vulkan shaderFloat16 mode is supported and enabled. Very dim or unusually bright
light colours retain FP32 modulation. Density/scattering coefficients, positions, shadow comparisons,
integration, accumulated radiance/transmittance and final composition remain FP32 to avoid
underflow and broad fog banding. Storage remains RGBA32F; no interfaces or layouts change. Shared
texture-buffer unit 24 is reserved for this transport.

Surfaces sample the preceding integrated endpoint and integrate the remaining partial segment
at their own depth. Four neighbouring columns are interpolated. Water and ordinary world effects
use the same lookup. Source-over blends include in-scattering; additive/subtractive blends only
attenuate their source colour. Menus and HUD drawing retain their original treatment.

After the authored sky shells, a premultiplied fog-only overlay adds height/local fog once to the
sky and clear background. The authored aerial haze is omitted there to avoid applying atmosphere
twice. The current frame's shadow maps are prepared before volume evaluation.

## Scope and verification

This is an authored-colour artistic medium, not an Earth-scale Rayleigh/Mie sky replacement.
Planetary lookup tables, multiple-scattering sky bakes, temporal fog history and particle lighting
caches are outside its scope. The deterministic grid is regenerated each rendered frame;
there is no history to ghost when lights or cameras change.

Local sphere intersections are clipped before quadrature so small volumes do not disappear
between slice centres. Their lighting and sun visibility are evaluated at the cell centre, so very
thin beams and sharp shadow boundaries remain limited by the grid resolution. Fog beyond the
configured range uses the final transport value. Colours stay in the renderer's existing display
colour convention to match authored fog and its fixed-function effect blending.

`atmosphere` checks authored haze, Beer-Lambert attenuation, overlapping media, partial slices,
small spheres, finite outputs and sky separation on the GPU. `atmosphere_off` checks shader
fallback. The existing enhanced-rendering and water tests cover affected shader paths.

`atmosphere_precision` checks emitted FP16 arithmetic and an FP32 shader variant with unchanged
uniform/storage layouts. GPU colour checks cover ordinary, HDR and very dim lights. The same
tests run with `OPENRAC_RENDER_OPT_OFF=shader-float16` to exercise the runtime FP32 fallback.

GPU tests switch High -> Low -> Off -> Medium and verify unchanged homogeneous-medium
attenuation. Settings tests cover every tier, invalid/missing values, save/load and reset.

## Performance

Column-invariant phase and fog-sphere intersections are evaluated once per ray. Active local
volumes are packed in their original order, so disabled entries do not consume slice-loop work.
A conservative per-column mask rejects point lights whose influence cannot intersect the ray.
Exactly empty cells retain the previous transport without lighting or integration; all output
records are still written. There is no density cutoff or early termination of thin media.

The sky fetches four final fog-only endpoints directly, retaining bilinear interpolation but
avoiding partial-slice reconstruction and integration. Its draw is skipped when neither height
fog nor local volumes are enabled. Surface sampling retains the original path: an explicit
rewrite to share its depth calculations did not improve the initial GPU measurements.

On an AMD Radeon 780M, Tracy GPU timestamps at 1920x1080 measured the specialised sky lookup
53-58% faster than the original lookup. Two captures alternated both paths within the same
frame to reduce clock and cache bias, with 600 frames per tier and the first 100 excluded.
For example, Medium in the first capture fell from 0.215 ms to 0.093 ms; the second measured
0.337 ms to 0.144 ms. Absolute timings varied, while the paired improvement remained consistent.
The volume-compute changes were much smaller than the sky improvement and are not assigned
a reliable percentage gain.

These are synthetic production-shader measurements with eight point lights, height fog and one
local volume, without scene shadow maps. They do not establish a whole-game frame-rate gain.
`atmosphere_test --profile` runs the optional Tracy workload, including a fullscreen surface
lookup and both sky paths; its combined timing therefore includes the extra reference draw.
Matching in-game views, scene-shadow costs and other GPUs still need profiling.

Additional GPU regressions check point-light coverage against numerical integration, rejected
lights behind/outside the ray, empty transport and sky draws, sparse local-volume arrays, and
agreement between the original and specialised sky endpoint lookup. Lifecycle checks cover
unused objects before graphics initialisation; a simulated device limit checks original-fog
fallback for an oversized grid and immediate recovery on a smaller tier.
