# Enhanced Novalis water



Novalis NTSC uses the existing painted textures, UV animation, GS alpha and draw order.

Vulkan enhanced mode adds probe-lit blue/turquoise colour, soft Fresnel reflection, broad sunlight

highlights, restrained shoreline crests and a GPU height-field simulation disturbed only by Ratchet.

OpenGL and original graphics retain the original geometry, animated ripples and material. Missing/invalid water bakes also retain it.



## Local water bake



The initial baker reads the complete initialized Novalis water tables from a local game-memory

snapshot. This avoids camera-dependent boundaries and runs the original initialization rather than

reimplementing it in the baker. Start Novalis with `OPENRAC_DUMP_RAM=600`; the game writes

`ram_600.bin` in its working directory. This is a debug capture, not a distributable asset.

Use the snapshot from a normal initial-state level, without geometry-changing mods unless the

collision export includes the same changes. The baker is explicitly NTSC Novalis only and rejects

uninitialized tables. PAL needs its own reviewed data addresses before enabling this feature.



```

python port/enhanced/tools/water/bake_water.py --ram build/water/ram_600.bin \

  --collision /path/to/levels/level_01/collision.glb \

  --output /path/to/levels/level_01/water.bin

```



The bake unions adjoining pool patches at the same elevation and the painted scrolling layers into physical water

fields. Pond coverage uses the static authored sub-block masks, never the camera-dependent live draw masks. It rasterizes rest heights at 0.5 game-unit spacing, intersects the pond with authored water

collision, clips banks against solid collision at the waterline, and calculates shoreline distances.

The exported collision palette identifies water collision so it cannot accidentally become a solid

bank. Overhead bridges and submerged bottoms do not become banks. Flowing strips use downhill or

long-axis flow; unobstructed strip boundaries absorb waves, while solid banks reflect them.



Authored `water-boundaries.json` profiles live under
`port/enhanced/water/profiles/<game>-<version>/level_NN/`, parallel to lighting
profiles. Native level export copies the matching profile beside `manifest.json`,
just like `probe-bake.json`; standalone bundles carry the source profiles too.
The water baker automatically reads the file beside its `--output` path.
An explicit `--boundaries PATH` overrides it. Missing default profiles use built-in
boundary detection; malformed profiles or missing explicit overrides fail the bake.
Novalis currently has an empty extra-outlet list, preserving its existing boundaries.

The file supplies additional open outlet segments:



```json

{"open_edges": [{"a": [10, 20, 5], "b": [12, 20, 5], "width": 2}]}

```



Coordinates are game world units. `width` controls the absorbing strip inward from the outlet.

The bake contains versioned metadata, rest heights, shoreline distance, absorbing weights, hydraulic depth/current and body

indices, plus a SHA-256 fingerprint of interpreted surfaces, settings and collision input. Regenerate

after changing any of these inputs. The runtime validates counts, dimensions and finite data before

allocating GPU buffers, and requires the mesh/coastline fingerprints to match; the fingerprint records provenance but is not currently checked against a

running game's geometry. Generated `water.bin`, `water.mesh`, memory snapshots and diagnostics stay local under

ignored asset/build directories. This pass does not change or rebuild probe lighting.



## Simulation and rendering



One bounded window runs the nonlinear 2D shallow-water solver on the selected nearby Novalis water body.

All painted surfaces belonging to that body sample the same state; other bodies retain procedural waves.

Ping-pong float buffers store surface displacement and x/y discharge (depth times horizontal velocity).

Slope-limited MUSCL face reconstruction uses HLL gravity-wave fluxes and contact-upwind transverse

momentum, with hydrostatic reconstruction for changes in bed depth. The original first-order

Rusanov path remains available only to the GPU comparison fixture.

Four compute substeps per game tick use dt=1/240 s at dx=0.25 game units.

The full-domain verification fixtures also retain the previous dx=0.5, dt=1/120 path. Gravity is 9.81 game units/s².

The depth and velocity bounds (0.02–4 units and ±2.5 units/s per axis) keep the fixed substep inside

the 2D CFL limit. Rendering limits displacement to ±0.25 units to retain the authored water silhouette.



The bake records effective hydraulic depth from the highest solid floor below the waterline,

clamped to 0.25–1.5 units. Missing floors use a controlled fallback (0.9 for the pond, 0.45 for flowing

water). This is an artistic hydraulic depth rather than an unrestricted deep-water model. The pond's

maintained current is zero; flowing water has a baked 0.8-unit/s current along its authored direction.



The equations evolve disturbances around that maintained reference. Subtracting the reference flux

provides fixed forcing that preserves authored surface levels and currents, including the sloping

cave/pipe art; this does not predict their initial flow from terrain gravity alone. Internal face

fluxes conserve disturbance mass. Solid banks mirror normal discharge, reflecting waves; open

reservoir boundaries and absorbing strips remove disturbances. Bed friction damps horizontal motion

back toward the reference current. Ratchet supplies a local momentum kick along his motion segment,

without removing water mass. Currents carry his disturbances downstream. A stationary hero adds no

continuing impulse. Teleports, large frame gaps and level changes reset history; repeated render

frames do not advance it twice. Original mode bypasses the entire solver.



The bake also writes `water.mesh`, containing every authored rest triangle and painted layer,

plus precomputed dense triangles and barycentric mappings for the original animated texture coordinates.

The baker uses longest-edge bisection to produce at most 0.5 game-unit edges without wasting vertices

on long, thin strips. Enhanced mode loads this geometry, replaces the game's animated water positions

with its fixed rest surface, and samples the compute height image in the vertex shader. Runtime does

not subdivide or decide mesh topology. It preserves scrolling UVs and colour on the baked triangles;

static geometry and texture translations are cached.

The simulation therefore displaces real geometry rather than layering new normals over retail ripples.

Interpolated UVs, colour, alpha and draw order retain the original art. Surface normals follow the

displaced geometry; the simulation slope is not applied a second time. Collision and gameplay physics

are unchanged. Disabling enhanced mode bypasses rest reconstruction, dense mesh substitution, simulation and

displacement, retaining the original triangles and game ripple animation. Re-enabling resets history.



Shore-distance bands add gentle movement at rest, broken up

and faded when subpixel to keep the painted style. They are decorative; reflected interaction waves

come from the solver. Linear probe irradiance and directional shadowing modulate the original blue

colour; Schlick Fresnel and broad specular highlights add surface response. Alpha is unchanged.



Probe irradiance and soft sun visibility are cached on a coarse grid (two game-unit spacing),

then interpolated over the water. The wave and shoreline maps use hardware bilinear filtering.

The filtered simulation image contains displacement and horizontal velocity; mesh derivatives

provide the displaced normal without a per-pixel finite-difference gather. A GPU buffer-to-RGBA32F-image copy makes the compute output available

for filtering, without CPU readback. The original material remains continuous across physical

bank masks; only the simulation is stopped at those boundaries.



`OPENRAC_WATER_OFF=1` is the original-water comparison. The profiler reports `water simulation`

and its cell count. GPU work and resource ownership stay on the graphics thread; hero positions are

read from the immutable scene snapshot. No game memory is read by the render worker.



## Exclusive displacement ownership



When the validated enhanced water renderer is ready and enhanced graphics is enabled, game-side

Novalis hooks bypass the original pond height-field step and reject original pool-height impulses.

They clear all three old displacement buffers, including waves left over when toggling enhanced on.

The original draw/material/UV callbacks still run. For river/cave/pipe packet generation, the two

legacy height endpoints are temporarily replaced by their midpoint, then restored. The baker uses

the same midpoint as its rest surface, so a captured retail ripple is not baked into the new mesh.

Only shallow-water compute output changes the enhanced mesh height. Game collision and swimming

remain separate. Disabling enhanced, using OpenGL, or missing/invalid replacement resources restores

the original calls and height interpolation. The game thread reads a renderer-readiness atomic;

the render worker never reads or writes live game RAM.



## Debug controls



Open **Enhanced > Water simulation** in the debug menu (also available through the F3 overlay's

enhanced controls). These settings last for the session and do not alter saved player settings or

baked data. Controls include pause, current-speed multiplier, friction, outlet damping, Ratchet's

interaction strength/radius and maximum rendered displacement. Changing current speed resets the

simulation to its new maintained current. The pond's baked current is zero, so its speed multiplier

has no effect. Pause freezes water physics without re-enabling original displacement; reset works

while paused and restores the maintained level/current. Restore defaults resets both tuning and

simulation history. Fixed timestep/gravity and internal solver bounds remain unchanged.



Lit, height, velocity and bank views show the computed surface and shoreline mask. Height uses

red/blue for raised/lowered water; velocity encodes signed X/Y flow in red/green. Controls are disabled

outside an active compatible enhanced level. The UI and render worker exchange locked copies of

the options, and reset requests use an atomic sequence counter.



## Verification



```

python -m unittest discover -s port/enhanced/tools/water -v

ctest --test-dir build/port -R "water_host|water_gpu|water_bake" --output-on-failure

```



The CPU bake fixtures cover patch seams, sloped heights, bridges, solid walls, explicit outlets,

invalid input and atlas offsets. The Vulkan fixture compares 240 shallow-water substeps (height and both velocity components)

against an independent CPU finite-volume reference. It checks closed-basin mass conservation,

positive depth, variable bed depth, maintained current, a still pond, reset, bank reflection, an

interior opening, outlet absorption and the actual enabled/disabled vertex displacement shader. Gameplay checks should include

the pond, river, cave and pipe, entry/movement/stillness, bank reflection, pause, teleport and original

rendering. Use identical warmed views for timing; never compare a remote-desktop run to a local run.



## Technique references



- [GPU Gems, Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models): separate broad movement from lighting detail.

- [Directional Flow](https://catlikecoding.com/unity/tutorials/flow/directional-flow/): orient patterns along the current. The original Novalis UV flow is retained here.

- [GPU Gems 2, Using Vertex Texture Displacement](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-18-using-vertex-texture-displacement): local dynamic height fields. This implementation uses shallow-water finite volumes and vertex texture displacement.



- [Audusse et al., A fast and stable well-balanced scheme with hydrostatic reconstruction for shallow water flows](https://publications.imp.fu-berlin.de/478/): hydrostatic reconstruction for finite-volume shallow-water fluxes. Our reference-current forcing is specific to preserving the authored water surfaces.



## Physical surface lighting



Enhanced water derives its geometric normal from the displaced rest mesh, including authored

slopes and the shallow-water height field. It uses air/water Schlick Fresnel (IOR 1.333,

normal-incidence reflectance 0.02037), GGX with height-correlated Smith visibility and fixed

roughness 0.22. Directional sunlight, directional fill and the existing eight local lights

supply untinted specular radiance. The sun samples the existing shadow atlas at the displaced

receiver; its opaque-material 10% shadow floor is removed for water. Local lights retain the

engine's authored radius falloff and do not acquire new shadow maps.



Beer–Lambert absorption uses RGB coefficients (0.85, 0.24, 0.12) per world unit and the

refracted viewing angle from Snell's law. This preserves clear shallows and a blue/cyan body.

The fourth hydraulic bake component stores unclamped floor depth independently of the solver's

effective depth; old v3 bakes with zero in that slot fall back to effective depth until rebaked.

No runtime bake or additional texture allocation is required. The static coast atlas stores

optical depth in Z, replacing the old artificial shore-normal gradient.



The original texture supplies submerged colour, with blue body colour for absorbed transmission.

Probe irradiance lights this colour and supplies an approximate low-frequency reflection;

this is not scene refraction or a resolved environment reflection. Reflection/transmission use

complementary Fresnel weights, without the previous minimum-light clamp or tinted Blinn highlight.

The existing decorative crest bands receive the same illumination. Original mode and alpha are

unchanged. GPU fixtures check the production Fresnel, GGX and absorption helpers against analytic

values; bake fixtures check that optical depth is independent of solver depth.



### Smooth wave normals



Lighting uses central differences of the bilinearly filtered simulation height at neighbouring

cells, combined with the undeformed authored surface slope. The rest position is interpolated

separately, so displacement is not counted twice and small tessellation triangles do not produce

faceted highlights. Samples clamp within each body's atlas rectangle; differences become one-sided

at its edge. The actual displaced geometric normal remains the shadow receiver-plane normal.

This changes shading only, preserving the displacement mesh, simulation, alpha and original mode.



### Animated hand interaction



Enhanced Novalis captures Ratchet's two palm roots (class-0 joints 56 and 76) from the same

animation evaluator used to draw him. Joint-local packed translations are transformed by his

captured model matrix on the game thread. Two optional world points travel with the immutable

scene snapshot; the render worker never follows game pointers. Missing poses skip hand interaction.



Each moving palm sweeps a local kernel along its actual 3D path. Near-surface strokes drag water

along the hand's horizontal motion; vertical entries produce a smaller radial push. Hands use

60% of the body's interaction radius and half its strength multiplier. They can disturb water

while Ratchet's body is stationary. Hands above the water, stationary hands, first appearances,

teleports, resets and discontinuous poses do not add impulses. The same pause/reset/strength

controls apply, and original mode retains its original behaviour. GPU tests exercise both hands,

airborne and stationary rejection, vertical entry, dry-cell exclusion and zero mass injection.



## Wakes and transported surface detail



MUSCL minmod slopes reconstruct perturbation height and velocity around the maintained authored

reference field. Slopes fall back to zero at walls, outlets and dry cells. Face depths and velocities

retain the existing positivity/speed bounds and fixed CFL-safe timestep. HLL carries gravity waves;

transverse momentum follows the upwind mass flux, avoiding gravity-speed diffusion of sideways shear.

The reference-flux subtraction still preserves authored still water and currents.



Ratchet's body and horizontal hand strokes seed localized counter-rotating vortex pairs behind

their direction of movement. These are momentum impulses with no direct mass source; subsequent

transport and bank interactions evolve the wake. The body spends less of its impulse on the broad

radial mound. Vertical hand entries retain their small radial disturbance. Strength 3 remains the

default; no new gravity, mesh density, bake or buffer allocation is required.



The formerly unused fourth simulation component carries a scalar surface marker. Semi-Lagrangian

backtracing uses simulated velocity, with bilinear sampling inside the same body's wet cells;

dry neighbours use the receiver's value. A slow world-space reseeding prevents indefinite pattern

loss. Wake impulses locally mark the trail. Four existing normal taps also recover marker gradients

for subtle moving optical ripples; the marker modulates surface colour and crest breakup. Original

painted textures keep their existing UVs: this is additional advected surface detail, not a claim

that all original texture coordinates are fluid particles. The Flow detail debug view displays the

actual advected marker. Pause and reset freeze/reseed it with the physical state.



Tests compare both transports against a double-precision CPU reference for 240 substeps, measure

energy/vorticity decay of a Gaussian pulse and vortex, verify signed marker advection, and check

that the production wake kernel has opposite circulation on its two sides. Existing checks retain

mass/positivity, walls/outlets, hand rejection, reset and original-material coverage.



Technique references (mathematical descriptions; no source code copied):

- [RDycore MUSCL reconstruction](https://rdycore.github.io/RDycore/theory/muscl.html).

- [Clawpack shallow-water solvers](https://www.clawpack.org/v5.12.x/riemann/Shallow_water_Riemann_solvers.html).

- [GPU Gems fluid advection](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu).



The default linear friction is 0.12/s (previously 0.35/s) so wakes can turn and linger rather

than stopping quickly. Body/hand vortex radii have minima of 1.0/0.75 world units to keep them

resolved on the existing 0.5-unit grid. Surface ribbons require both rotational flow and departure

from the maintained current; ordinary gravity waves do not turn the whole pond into contour bands.

Their width is normalized by the marker gradient and antialiased in screen space. Numerical decay

comparisons deliberately hold friction at 0.35/s in both paths to isolate transport improvements.



## Fine surface normals



Enhanced water combines the simulated height slope with broad optical wavelets at wavelengths

1.1 and 0.67 world units, with height amplitudes 0.014 and 0.006 units. Slow phase bending curves

the crests and a spatial envelope varies their strength, breaking up repeated straight lines.

Analytic derivatives include both bending and envelope gradients;

only the shading normal changes. The existing source UV direction orients the elongated crossing

wavelets, with a consistent sign for mirrored UVs. Original painted brightness gently weights the

strength, without interpreting compression-block edges as physical height.



Local simulated velocity drives two overlapping 1.5-second flow phases; each phase resets while

its blend weight is zero, limiting stretching and preventing a visible reset. Slow wave motion

remains in still water. The simulation clock freezes these ripples while paused. Analytic pixel

footprints fade under-resolved waves before the Nyquist limit to suppress distant sparkling.

The existing simulation slope and original rest slope are retained. Mesh displacement, shadows'

receiver plane, alpha, texture colour and original rendering remain unchanged. This adds arithmetic

but no texture samples, images, bake data or simulation passes. **Fine ripples** in the session-only

water debug controls scales the effect from 0 (off) to 2; default 1.



## Gerstner swell and artwork tint



A gentle 2.75-unit Gerstner wave adds 0.045-unit vertical amplitude and 0.02925-unit

horizontal amplitude over the shallow-water displacement, around 2.5–4 times the

fine ripple wavelengths. Its phase advances on the simulation clock at 1.4 radians/s.

The existing baked dense mesh is reused. A coast/depth taper pins the swell to banks

and dry cells; this costs one additional coast-map fetch per vertex, with no new pass.

Shared analytic derivatives, including the interpolated taper derivative, transform

the smooth surface tangents in the fragment shader. Simulation lookup uses rest

coordinates so lateral displacement cannot move a pixel into another field.

The analytic swell is visual only; it does not change collision or the interactive solver.



The original painted texture's linear chroma gently modulates the blue body colour

at 32% strength. Chroma is bounded and dark pixels use a luminance floor, preserving

the blue tint without amplifying compression or turning dark artwork into bright colour.

Alpha and the original non-enhanced material are unchanged.



Reference for the Gerstner equations (implementation written here):

[GPU Gems, Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models).



### Inward boundary waves



A second low-steepness Gerstner layer has wavelength 2.2 units (twice the main optical

ripple scale), vertical amplitude 0.042 units, horizontal amplitude 0.0091 units and phase speed 2 radians/s. Its phase is

based on the baked distance to the actual shoreline, so crests travel inward

rather than away from an arbitrary rectangular atlas edge. Four clamped vertex

coast reads recover the inward distance gradient. Interpolated distance/direction

are shared with shading, where continuous derivatives include the curved direction

and bank taper. Waves attenuate over distance and fade where opposing banks meet

and the inward direction becomes ambiguous. No rebake, new buffer or simulation

pass is needed. The existing 2.75-unit swell remains underneath this layer.



The separate three-layer cave water callback is included as semantic surfaces 5–7.

Each receives its own still-water field because the pools overlap in XY at different

heights. The callback tags each layer independently, then clears the tag after drawing;

its original UV/glow animation remains while enhanced mode flattens legacy displacement.



## Shared local simulation window



Dynamic history consists of exactly two fixed 128x128 RGBA32F state buffers (512 KiB

combined), shared by the rendered water surfaces. The 0.25-unit cells give twice

the linear simulation resolution of the former full-level 0.5-unit fields. The

window spans 32x32 world units and follows Ratchet in whole-cell increments.

The baked displacement mesh retains its existing 0.5-unit maximum edge; the finer

solver also supplies the smooth shading normals. Static shore/depth data remains

at its baked 0.5-unit resolution.



World-cell coordinates address the state toroidally. Power-of-two modulo handles

negative coordinates as well as positive ones. Existing cells retain their physical

slots as the window moves; no history buffer is shifted or copied. Reads of newly

exposed rows/columns return the undisturbed reference until the next step writes

them. Switching to a body more than 0.5 units away in height, teleports, and large tick gaps reset

history. Compatible body switches retain the existing world-addressed state.



Selection uses nearby baked wet cells, vertical separation, conservative camera

side-plane visibility and hero contact. The side-plane test includes a displacement

margin and is independent of the reversed-depth convention. With neither relevant

visible water nor contact, solver dispatches and dynamic-map copies stop. Procedural

wave time continues independently. Interaction can still prepare the material when

the effects pass has no visible draws. Repeated interpolated frames do not advance

or prepare the same tick twice.



The solver has an absorbing guard band from radius 12 to 15.5 units. Rendering fades

interactive displacement, velocity deviations and advected detail to their reference

between radii 12 and 15, before the torus edge. Gerstner swells, boundary waves,

fine optical normals and the enhanced lighting remain on distant surfaces. Surfaces within 0.5 units of the selected water height share dynamic samples and

wet-domain metadata, including separate draw tags. The metadata pass prefers the

selected field at overlaps and fills uncovered cells from compatible wet fields.

Baked banks and dry gaps remain barriers; pools farther apart in height stay isolated.



Two additional local buffers cache static coast/depth/current metadata whenever

the window origin or selected body changes. They are not extra simulation states.

This preparation pass avoids repeated full-bake lookups in the hot solver loop.

The existing render-output staging buffer/image are also fixed at 128x128; their

GPU copy supports the existing filtered texture path without a CPU readback.

Static full-level coastline and lighting data are retained separately.



GPU checks exercise negative modulo addresses, preservation of overlapping slots,

new-column initialization, exact fine-to-bake metadata sampling, a uniform-water

invariant through 240 fine substeps, filtered sampling at a wrap seam, body isolation,

and radial fading. Existing transport, mass, wake, hands, original-mode and material

compilation checks remain in place. Camera-side culling has conservative edge cases.



Compatible-height rendering compares the original rest-surface height, so shoreline

filtering cannot pull the height toward a dry-cell sentinel. Metadata uses baked wet-cell heights. Vertex displacement and smooth normals

use the same compatibility test. The two state buffers, window size and simulation

resolution are unchanged. GPU regressions cover adjacent fields 0.3 units apart,

an excluded pool 3 units above, and both sides of the 0.5-unit sampling threshold.



## Maintenance and verification

The CPU-only WaterMesh module owns baked mesh validation, rest-surface lookup and
vertex caching. The controller owns GPU resources, activation and simulation timing.
Reinitialization resets resource sizes and timing; paused frames do not advance
the solver. Original ripple routing follows the same enhanced-water readiness state.

Coast metadata shares physical torus slots with the histories. Preparation seeds both
histories for newly wet cells, exposed cells and incompatible height changes, while
preserving compatible neighbours. Compute buffers are allocated as storage resources
even when they also have texture views.

Lighting-cache receivers exclude dry-bank sentinels. World-space probe lookups bypass
screen-tile masks. Direct shadows use the displaced surface. Fine-wave phases use
world coordinates so compatible surfaces agree.

Checks cover baking, original ripple routing, CPU mesh/cache behavior, controller
lifecycle, shader compilation and CPU/GPU solver comparisons. Runtime fixtures use
synthetic data and a hidden graphics context; they do not launch the game. These
checks do not replace visual inspection or gameplay profiling.


Lighting receiver lookup first tests the centre cell. Wet centres require one
coast fetch instead of the full 25-cell search; dry centres retain the original
scan/tie order and skip candidates that cannot improve the nearest receiver.
GPU regression fixtures compare receiver positions and validity against the
original full scan across eight wet/dry patterns. CPU field selection clips its
search bounds once, avoiding duplicate edge-cell visits without changing tie order.
No simulation resolution, substep count, shading or wave parameters are reduced.


## Leaving the water

After Ratchet loses water contact, the active local simulation continues for five
seconds with its last window fixed in world space. A smoothstep weight blends
displacement, velocity deviations, marker detail and derived normals back to their
undisturbed values. No new body/hand impulses are injected during this tail.
The solver stops once the weight reaches zero; re-entry restores full interaction.
The fade uses simulation time and pauses with the water debug pause control.
Decorative swells and the enhanced water material remain active. Explicitly
disabling enhanced water still returns immediately to the original water behavior.
