# Enhanced Novalis water

Novalis (NTSC-U and PAL) optionally adds Vulkan probe lighting, soft reflections and local GPU shallow-water
simulation, disturbed by Ratchet's movement/hands. Enhanced surfaces transmit the actual scene with
Beer-Lambert absorption and gentle, depth-faded refraction (at most six render pixels). Original art
supplies the blue/cyan medium tint and stylised ripple detail. One GPU colour/depth snapshot per visible
water pass avoids attachment feedback; foreground/sky rejection checks the complete filter footprint.
Scene lighting and fog are retained once, with Fresnel, probe reflections, foam and direct GGX added
at the surface. Depth copies also resolve multisampling. No snapshot is taken without a replacement
water mesh. OpenGL, original graphics or invalid/missing bake resources use original water.

`water_gpu` compares scene depth, refraction rejection and composition with CPU values; `effect_material`
checks real water transmission, deeper absorption, foreground occlusion, resizing and 1x/4x MSAA.

## Projectile and explosion interaction

Blaster shots, thrown bombs and missiles disturb water along their swept path. Explosions send a
single radial pressure impulse from the original game's explosion routine, scaled by the blast size.
The Bomb Glove's separate detonation path is observed across its native update, including immediate
underwater deletion; its expanding damage ring emits only one impulse.
Impulses change water momentum without creating water mass; gameplay damage and collision stay intact.
The game thread captures owned positions and events, so the renderer never reads live game memory.

Impacts can wake the local simulation while Ratchet is standing on shore. The existing 32-metre
simulation window and five-second settling period still apply; swimming takes priority over remote
impacts outside that window. Up to 16 impulses are processed per tick, with explosions taking priority.
Surface contact uses FP32 swept tests to catch fast crossings and reject shots above the water.
Colour/depth/shadow preparation and interpolated display frames never apply an event twice.

Live tuning is under Enhanced graphics > Materials & water > Water simulation. Projectile and
explosion strength/radius are separate multipliers (default 1x). Strength ranges from 0 to 4x;
zero disables that source. Radius ranges from 0.1 to 4x, preserving the relative sizes of weapons
and blasts, with a final radius cap of 4 world units. Changes affect new impulses immediately;
existing waves keep settling. These controls are session-only and Restore water defaults resets them.

`water_interactions` checks regional class selection, swept capture, slot reuse and coarse contact.
`water_runtime` checks idle-pond activation, impacts, duplicate prepares, pause, finite bounded waves
and water mass. `water_host` verifies explosion capture and exact forwarding of all original arguments.

## Floating lilies

With enhanced water active, RAC1 water lilies (class 695, selected by the shared PAL/NTSC game
profile) bob using the same GPU displacement evaluator as the liquid surface. The whole lily
bobs at its anchor and tilts with the nearby wave slope, preserving its authored height offset
and mesh shape. Height and tilt exponentially lerp toward the current wave pose (6/s and 4/s by default),
so lilies lag behind the waves with a sense of momentum. Enhanced graphics > Misc > Lily momentum
sets the tilt response time from 0 to 1 second (default 0.25); height responds 1.5 times faster.
Zero disables smoothing. The value is saved as `lily_momentum` and restored by Reset enhanced.
The filter follows the water game clock, seeds new attachments immediately and resets after large
gaps. A rigid rotation also updates lighting normals; no vertex bending is applied. Gameplay
positions, collision and interaction stay unchanged. This follows the water feature's existing
bake availability; unsupported levels/versions and disabled or missing water retain original poses.

Colour, depth, shadows and temporal coverage share the displaced position. Dynamic water offsets
reject old FSR object history while active, avoiding reuse of an undeformed pose; inactive water
keeps normal temporal accumulation. No simulation readback or CPU per-vertex update is needed.
The atlas's existing alpha component carries rest height so attachments can distinguish nearby
water from another storey. Shared GPU texture bindings use reserved units 18-20; attachment poses use unit 25.
One compute invocation updates each unique anchor before any drawing; all passes read that pose.
History follows live object identities, retaining momentum during native lily drift and height
adjustments. New objects and teleports seed a fresh pose; static attachments use their fixed anchors.

`water_gpu` checks matched wave displacement, authored offsets, nonzero rest heights, dry/outside
coverage, disabled attachments, missing bodies, filtered height/tilt, pause and new-anchor seeding. `camera_target` also verifies the lily-only
profile selection for both regions.

## Reflections

Water uses the shared [environment reflections](../lighting/README.md#environment-reflections)
and material controls. Missing capture coverage falls back to local GI. Reflection capture and
the water simulation bake are independent. Water selects captures up to 0.5 world units toward
the camera-facing side of the interface, keeping box-projection rays at the true displaced surface.
This keeps shallow cave water out of underwater/floor influence fade bands that otherwise mix dark
ceiling captures with bright diffuse-GI fallback. Opaque receivers keep their original selection;
GPU tile masks conservatively cover the bounded offset. Reflection strength and BRDF are unchanged.

Water debug views separate direct highlights, environment reflection, illumination, surface mask,
capture-only reflection, GI fallback and cached GI. `OPENRAC_WATER_VIEW` selects these as 5-11 for
scripted captures; 0 is normal shading and 12 compares the original reflection selection.

## Bake local water data

The launcher's enhanced-lighting preparation automatically bakes Novalis (NTSC-U and PAL) water before GI
and reflections. Export retains the original overlay in `bake-inputs/overlay.bin`; preparation
reads its authored tables and applies the game's initial grid spacing and strip rest heights.
No game launch or RAM snapshot is required. Older exports recover the input from `--disc`, or
must be exported again. Other levels are unsupported.

`water-bake.json` records input and output hashes. Unchanged water is kept; missing or changed
files, collision, outlet settings or baker code trigger preparation again. Outputs stay beside
the level manifest. The standalone snapshot tool remains available for comparison:

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
input changes; automatic preparation updates water before fingerprinting the probe bake.

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
darkening/specular coating preserve emission, alpha and geometry. Wet patches also reflect the
local, box-projected environment probes, using the wetness roughness, reflectivity and specular
strength controls. Reflections fade with drying; missing captures retain the direct GGX highlight.
Other mobys, attachments and original graphics are excluded. Level/hero identity changes or disabling
enhanced graphics reset it; missing water lets patches dry. Coarse triangles soften waterlines; early mesh preparation may use
the previous frame's images.

## Controls

**Graphics > Enhanced Settings** contains session-only water controls: pause, current scale,
friction, outlet damping, interaction strength/radius, displacement limit and fine ripples.
Current changes reset the simulation; the still pond has no base current to scale. Pause freezes
physics and decorative water time. Reset works while paused. Restore defaults resets tuning and
history. Lit, height, velocity, banks and flow-detail views expose the GPU state.

Default interaction strength is 1.4, friction is 0.12/s and fine-ripple strength is 1. Ratchet wetness
controls default to diffuse scale 0.28, specular strength 1.00, dry/wet roughness 0.50/0.34 and
reflectivity 0.0632. Its diagnostic shows wet samples in blue and dry samples in grey. These
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

Water and world particles also share the optional [atmosphere transport](atmosphere.md) at their
own surface depth.

Foam follows sharp positive crest curvature with steep slopes or converging velocity, rather than
absolute ripple height. Broad swells, raised plateaus and smooth rotating flow retain transmission.
Curvature and compression reuse the four existing wave taps; no extra wave or depth reads are added.
Thin shoreline foam remains. Water views 13/14/15 show foam off, foam coverage and the old height-based
response for native capture comparisons. `water_gpu` checks flat/broad/trough/crest cases, height-offset
invariance, both wave-sampling paths and clamped atlas borders against CPU values.

Water contacts blend the complete shaded surface into the original scene across a 0.12 world-unit
normal-depth band. Coverage uses the unshifted scene depth and baked plane normal, so fine ripples
and refracted floor samples cannot move the shoreline. Refraction fades out with coverage; deeper
water is unchanged. This reuses the existing depth fetch and adds a scene-colour fetch only within
contacts (or the existing rejected-refraction path). Water views 16/17 show contact coverage and the
unblended reference. GPU fixtures check analytic contact depths, projection types, silhouettes,
refraction rejection, MSAA and unchanged deeper water.
