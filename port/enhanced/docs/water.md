# Enhanced Novalis water

Novalis NTSC-U has optional Vulkan water rendering. It preserves painted textures, authored UV
animation, GS alpha and draw order, while adding probe lighting, soft reflection and a local GPU
shallow-water simulation. Ratchet's movement and hands disturb the surface. OpenGL, original
graphics and missing or invalid water resources retain the original water path.

## Bake local water data

The baker reads the initialized Novalis water tables from a local memory snapshot. This captures
all authored boundaries without depending on the current camera. Start a normal, unmodified
Novalis level with `OPENRAC_DUMP_RAM=600` to write `ram_600.bin` in the game's working directory.
Use collision exported from the same level. Snapshots and generated assets remain local in
ignored directories. This tool supports NTSC-U Novalis only.

```sh
python port/enhanced/tools/water/bake_water.py --ram build/water/ram_600.bin \
  --collision /path/to/levels/level_01/collision.glb \
  --output /path/to/levels/level_01/water.bin
```

The bake writes `water.bin` and `water.mesh` beside the exported level's manifest. It joins pool
patches at matching heights, retains overlapping cave layers separately, clips banks against
solid collision and records rest heights, shoreline distance, outlets, depth and maintained
currents. Bridges and submerged floors do not become banks. The coastline uses 0.5-unit cells;
longest-edge subdivision limits mesh edges to 0.5 units without expanding long thin triangles
uniformly. Dense mesh templates and barycentric mappings are uploaded once.

Versioned metadata includes a SHA-256 fingerprint of interpreted surfaces, settings and collision.
Runtime checks counts, bounds and finite data, and requires mesh/coastline fingerprints to match.
It does not compare that fingerprint with live game geometry. Rebake after changing the inputs.
The water bake is independent of probe baking.

Export copies the matching tracked profile from
`port/enhanced/water/profiles/<game>-<version>/level_NN/water-boundaries.json`.
The baker reads it beside `--output`; `--boundaries PATH` overrides that selection. Missing default
profiles use detected boundaries. A malformed profile or missing explicit override fails the bake.
Profiles add open outlet segments in game world units:

```json
{"open_edges": [{"a": [10, 20, 5], "b": [12, 20, 5], "width": 2}]}
```

`width` is the absorbing strip inward from the outlet. Novalis's tracked extra-outlet list is empty.

## Simulation and geometry

One 128x128 window follows Ratchet in whole 0.25-unit cells, spanning 32x32 world units. Two
RGBA32F history buffers use 512 KiB together. Additional local buffers cache static shoreline,
depth and current metadata; they are not simulation histories. Full-level coastline and lighting
remain separate. World-cell modulo addressing preserves overlapping history without shifting
buffers, including at negative coordinates. Newly exposed or incompatible cells start undisturbed.

The solver evolves height displacement and horizontal discharge around the authored rest surface
and maintained current. It uses slope-limited MUSCL reconstruction, HLL gravity-wave fluxes,
contact-upwind transverse momentum and hydrostatic bed-depth reconstruction. Reference-flux
forcing preserves the authored slopes and currents rather than deriving their initial flow from
gravity alone. The first-order Rusanov path is retained for comparison tests.

| Quantity | Value |
|---|---|
| Runtime substeps | Four per game tick, dt = 1/240 s |
| Gravity | 9.81 game units/s squared |
| Solver depth bounds | 0.02 to 4 units |
| Velocity bounds | +/-2.5 units/s per axis |
| Rendered displacement limit | +/-0.25 units by default |
| Baked hydraulic depth | 0.25 to 1.5 units; missing floors use 0.9 for ponds, 0.45 for flowing water |
| Maintained current | Zero in the pond; 0.8 units/s along baked flowing-water directions |

Solid banks reflect normal discharge. Open boundaries absorb disturbances; friction damps motion
toward the maintained current. Movement and palm strokes inject local momentum without removing
water mass. Standing still supplies no continuing impulse. Teleports, large tick gaps and level
changes reset history; repeated interpolated displays do not step the same tick twice.

Nearby visible wet cells and hero contact select the active window. Compatible surfaces within
0.5 units of its rest height share history, while banks, dry gaps and other storeys remain isolated.
An absorbing guard band spans radii 12 to 15.5 units. Rendering fades dynamic displacement,
velocity deviations and transported detail between radii 12 and 15, before the torus edge.
After contact ends, the last window stays fixed and fades out over five simulation seconds.
Re-entry restores interaction. With no relevant visible water or contact, solver dispatches and
dynamic-map copies stop; decorative wave time can continue.

The GPU expands dense mesh templates from the captured coarse triangles. A compute pass evaluates
UV distortion once per coarse vertex, preserving the two opposed texture layers, signed distortion,
scroll offsets and game-update timing. Vertex interpolation then applies surface displacement.
Runtime does not subdivide geometry or build expanded CPU vertex streams.

When replacement resources are ready, Novalis hooks bypass legacy pond displacement, clear its
old buffers and flatten the river/cave/pipe builders to their authored midpoint rest heights.
They capture UV animation metadata and immediately restore the game's values after packet creation.
Disabling enhanced graphics restores original calls and ripple routing. Collision, swimming and
other gameplay remain unchanged. The render worker reads owned snapshots, not live game memory.

## Surface lighting and wetness

Probe irradiance and directional visibility are cached at two-unit spacing and interpolated across
the water. A filtered GPU image exposes displacement and velocity without CPU readback. Dry banks
are rejected as lighting receivers; world-space GI lookups bypass screen-tile masks.

The material combines blue/turquoise body colour, Schlick Fresnel at IOR 1.333, GGX/Smith sunlight
highlights and RGB Beer-Lambert absorption coefficients (0.85, 0.24, 0.12). Bounded painted chroma
contributes 32 percent of the tint. Alpha is unchanged. Smooth normals follow displaced tangents;
simulation slope is not applied twice.

A 2.75-unit Gerstner swell has 0.045-unit vertical amplitude and 1.4 rad/s phase speed. A second
2.2-unit wave travels inward from actual shoreline distance, with 0.042-unit vertical amplitude and
2 rad/s phase speed. Both taper at banks and remain visual only. Optical wavelets at 1.1 and
0.67 units use amplitudes 0.014 and 0.006 units. Analytic derivatives include phase bending and
spatial variation. Two overlapping 1.5-second flow phases limit stretching; pixel footprints fade
under-resolved detail. These fine waves change shading normals, not geometry or collision.

Ratchet's class-0 mesh has GPU per-vertex wetness. Animated samples test baked wet cells and the
latest available simulated/decorative surface. Contact sets wetness to one; drying takes
20 game seconds. Diffuse darkening and a specular coat leave emission, alpha and geometry intact.
Other mobys, attached equipment and original graphics do not receive this coating. Level or hero
identity changes, or disabling enhanced graphics, reset it. Missing water can leave existing
patches drying. Coarse triangles soften the waterline, and contact may use the previous frame's
water images when mesh preparation runs first.

## Controls

**Graphics > Enhanced Settings** contains session-only water controls: pause, current scale,
friction, outlet damping, interaction strength/radius, displacement limit and fine ripples.
Current changes reset the simulation; the still pond has no base current to scale. Pause freezes
physics and decorative water time. Reset works while paused. Restore defaults resets tuning and
history. Lit, height, velocity, banks and flow-detail views expose the GPU state.

Default interaction strength is 1.4, friction is 0.12/s and fine-ripple strength is 1. Ratchet wetness
controls default to diffuse scale 0.32, specular strength 4, dry/wet roughness 0.52/0.30 and
reflectivity 0.02037. Its diagnostic shows wet samples in blue and dry samples in grey. These
controls do not alter contact or drying. `OPENRAC_WATER_OFF=1` selects original water for comparison.

## Modules and verification

- `water_mesh.*` validates baked geometry and caches rest-surface mappings; `water_mesh_gpu.cpp`
  expands and draws the dense templates.
- `water_asset.cpp` loads the bake, `water_resources.cpp` owns GPU allocation/release, and
  `water_simulation.cpp` selects and advances the window. `water.cpp` binds materials and exposes
  the debug controls; `water_internal.h` keeps their shared state private.
- `wetness.*` owns Ratchet's independent GPU contact and drying history.

```sh
python -m unittest discover -s port/enhanced/tools/water -v
ctest --test-dir build/port -R 'water_|wetness_' --output-on-failure
```

Synthetic fixtures cover bake boundaries, collision layers, outlets, invalid inputs, CPU mesh
lookups, lifecycle/reset behavior, original ripple routing and CPU/GPU finite-volume agreement.
Optical checks compare production shader helpers with analytic values. Window checks cover modulo
seams, body isolation, compatible heights, filtered sampling and lighting receivers. Wetness tests
cover animation, contact rejection, drying and model isolation. The GPU tests use hidden synthetic
contexts, not game assets. They do not establish visual quality or performance for every camera.

## Public references

These describe the mathematics; no source code was copied:

- [GPU Gems: physical-model water](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models).
- [Directional flow](https://catlikecoding.com/unity/tutorials/flow/directional-flow/).
- [GPU Gems 2: vertex texture displacement](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-18-using-vertex-texture-displacement).
- [Audusse et al.: hydrostatic reconstruction](https://publications.imp.fu-berlin.de/478/).
- [RDycore: MUSCL reconstruction](https://rdycore.github.io/RDycore/theory/muscl.html).
- [Clawpack: shallow-water solvers](https://www.clawpack.org/v5.12.x/riemann/Shallow_water_Riemann_solvers.html).
- [GPU Gems: fluid advection](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu).
