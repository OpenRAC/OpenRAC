# Enhanced Novalis water

Novalis NTSC-U optionally adds Vulkan probe lighting, soft reflections and local GPU shallow-water
simulation, disturbed by Ratchet's movement/hands. Painted textures, UV animation, GS alpha and draw
order stay intact. OpenGL, original graphics or invalid/missing resources use original water.

## Bake local water data

The baker reads initialized water tables from a local RAM snapshot, capturing all boundaries
independently of the camera. Start unmodified NTSC-U Novalis with `OPENRAC_DUMP_RAM=600` to write
`ram_600.bin` in the working directory. Use matching exported collision; keep snapshots/assets ignored.
Other levels/versions are unsupported.

```sh
python port/enhanced/tools/water/bake_water.py --ram build/water/ram_600.bin \
  --collision /path/to/levels/level_01/collision.glb \
  --output /path/to/levels/level_01/water.bin
```

`water.bin` and `water.mesh` sit beside the manifest. Baking joins equal-height pools, separates
cave layers, clips banks against solid collision and records rest heights, shoreline distance,
outlets, depth and currents. Bridges/submerged floors are not banks. Coast cells and maximum mesh
edges are 0.5 units; longest-edge subdivision avoids uniform expansion of thin triangles. Dense
templates/barycentric mappings upload once.

Versioned metadata hashes interpreted surfaces, settings and collision with SHA-256. Runtime checks
counts, bounds, finite data and matching mesh/coastline fingerprints, not live geometry. Rebake after
input changes; water and probes bake independently.

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

A 128x128 window follows Ratchet in 0.25-unit cells across 32x32 units. Two RGBA32F histories use
512 KiB; separate buffers cache local shoreline/depth/current metadata and full-level coastline/lighting.
World-cell modulo addressing preserves overlap without moving buffers, including negative coordinates.
New/incompatible cells start undisturbed.

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

Banks reflect normal discharge; open boundaries absorb disturbances; friction restores maintained
current. Movement/palm strokes add momentum without removing mass; standing still adds no impulse.
Teleports, large tick gaps and level changes reset history. Interpolated displays never step a tick twice.

Nearby visible wet cells and hero contact select the active window. Compatible surfaces within
0.5 units of its rest height share history, while banks, dry gaps and other storeys remain isolated.
An absorbing guard band spans radii 12 to 15.5 units. Rendering fades dynamic displacement,
velocity deviations and transported detail between radii 12 and 15, before the torus edge.
After contact ends, the last window stays fixed and fades out over five simulation seconds.
Re-entry restores interaction. With no relevant visible water or contact, solver dispatches and
dynamic-map copies stop; decorative wave time can continue.

The GPU expands dense templates from captured coarse triangles. Compute evaluates UV distortion
once per coarse vertex, retaining two opposed layers, signed distortion, scrolling and game timing;
vertex interpolation applies displacement. Runtime builds no subdivided CPU streams.

With resources ready, Novalis hooks bypass/clear legacy pond displacement and flatten river/cave/pipe
builders to authored midpoint rest heights. UV metadata is captured and game values restored after
packet creation. Disabling enhanced graphics restores original calls/ripples. Collision, swimming
and gameplay stay unchanged; the render worker consumes owned snapshots, not live memory.

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

Ratchet's class-0 mesh tracks GPU per-vertex wetness against baked wet cells and the latest
simulated/decorative surface. Contact sets wetness to one; drying takes 20 game seconds. Diffuse
darkening/specular coating preserve emission, alpha and geometry. Other mobys, attachments and
original graphics are excluded. Level/hero identity changes or disabling enhanced graphics reset it;
missing water lets patches dry. Coarse triangles soften waterlines; early mesh preparation may use
the previous frame's images.

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

Synthetic tests cover boundaries, collision layers, outlets, invalid inputs, CPU mesh lookups,
lifecycle/reset, original ripples and CPU/GPU finite-volume agreement. Shader optics use analytic
references. Window tests cover modulo seams, isolated bodies, compatible heights, filtering and
lighting receivers; wetness tests cover animation, rejected contact, drying and model isolation.
Hidden GPU fixtures use no game assets and cannot establish every camera's quality/performance.

## Public references

These describe the mathematics; no source code was copied:

- [GPU Gems: physical-model water](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models).
- [Directional flow](https://catlikecoding.com/unity/tutorials/flow/directional-flow/).
- [GPU Gems 2: vertex texture displacement](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-18-using-vertex-texture-displacement).
- [Audusse et al.: hydrostatic reconstruction](https://publications.imp.fu-berlin.de/478/).
- [RDycore: MUSCL reconstruction](https://rdycore.github.io/RDycore/theory/muscl.html).
- [Clawpack: shallow-water solvers](https://www.clawpack.org/v5.12.x/riemann/Shallow_water_Riemann_solvers.html).
- [GPU Gems: fluid advection](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu).
