# Offline interior lighting

Normal probe bakes run interior inference and prepare baked interior sources by default.
The inference tool writes a separate JSON report; unambiguous indoor samples drive bounded
emitter boosts and ceiling lights. `regions.interior_lighting: false` skips automatic
preparation. Explicit source sidecars are preserved. Novalis NTSC-U (level 1) has the verified profile.

The bake pipeline reads exported collision and retained gameplay without disc access.
Generated `probe-interiors.json` stays beside the export for runtime fingerprint validation;
reports stay in the bake output. Collision water colours retain the exporter's non-walkable
surface classification. Sources refresh when their inputs or inference tools change.

`port/enhanced/tools/probes/prepare_interiors.py` reads the owned disc with the editor's
existing read-only readers. It exports deduplicated static collision and only
the environment transitions with the hero-light flag. It records the source
game/level and a SHA-256 over gameplay and the collision-containing core tail.
Outputs contain derived game data and must remain local in ignored storage.

The optional Embree tool `openrac-interior-infer`:

1. Samples downward through every collision layer on an XY lattice, retaining
   slopes with absolute geometric normal Z at least 0.65. Authored mesh normals
   and winding are not used. Water cannot supply walkable support.
2. Checks 2.5 units of vertical clearance and a sampled 0.45-unit body radius.
   Links cardinal neighbours with at most a 2-unit height difference, using
   clearance rays above the higher support. These are tunable proxies, not the
   game's movement capsule, jump solver or a proven navigation mesh.
3. Temporarily cuts graph edges crossing the centre plane of each authored
   transition within its oriented Y/Z extent. Real collision remains unchanged;
   no such boundary is introduced into baking rays.
4. Finds supported floor samples beyond each transition endpoint, below its
   centre and visible from the target point. Every qualifying component receives
   the endpoint's light-set/ambient label, so floor gaps do not prevent a room
   from inheriting its mood. Evidence cannot cross collision walls or another
   authored transition boundary. The best supported sample remains the diagnostic
   seed; tiny ledges receive a bounded penalty. Identical labels agree across
   entrances; conflicting labels and components without evidence remain unresolved.
5. Measures unoccluded directions over 16 upper-hemisphere rays, capped at
   128 units. This is finite-distance sky visibility, not irradiance or proof
   of an interior. The report does not equate light set 0 with outdoors.

The JSON preserves per-sample height, component and sky visibility, both side
seeds, oriented transition corners, labels and component sizes. Region boxes
are deliberately not substituted for the irregular sampled regions.

## Reproduce locally

Build with `OPENRAC_PROBE_BAKER=ON` and the existing Embree dependency:

```sh
cmake --build port/build/release --target openrac-interior-infer interior_inference_test
ctest --test-dir port/build/release -R '^interior_inference$' --output-on-failure
python port/enhanced/tools/probes/prepare_interiors.py --disc /path/to/owned-disc.iso --level 1 --output build/probe-gi/novalis-interior-input.json
port/build/release/openrac-interior-infer build/probe-gi/novalis-interior-input.json build/probe-gi/novalis-interior-regions.json 1
```

The official Embree package's `bin` directory must be on PATH on Windows.
Both tools refuse to overwrite reports. No game window or scripted game test
is involved. The synthetic tests cover a roofed room and doorway, rotated
transforms, finite portal extents, conflicting assignments through an unmarked
entrance, stacked surfaces, water support rejection and reversed winding.
They also cover disconnected floor patches, air routes around obstructions, shortest-route
selection, wall occlusion and label separation across another authored transition.

## Lighting-set propagation

`interiors.cpp` samples collision floors and connects support patches. `interior_labels.cpp`
propagates authored moods through enclosed air. Their collision queries and math stay private
in `interior_inference_internal.h`.

Transition endpoints first label visible floor components. Matching authored light-set and ambient
pairs share one label. Direct assignments retain priority, including conflicting authored evidence.

Unlabelled components use a separate air grid at the floor sampler's spacing. Voxels require a
ceiling within 32 units and surrounding collision in at least five of eight horizontal directions.
Air links require an unobstructed collision ray; they do not require walkable ground underneath.
This allows labels to pass around corners, over obstacles and across missing support patches.
Mesh winding does not affect these tests.

The pass uses the shortest six-neighbour air route from authored evidence. Equal-distance moods
remain ambiguous. Each floor component receives its closest supported evidence, while directly
labelled components retain their authored assignments. Lighting boundaries retain their finite
width and extend vertically for air routing, so labels cannot bypass them above the player volume.
The original gameplay transition tests are unchanged.

The air cache is bounded by `Settings::max_cells`, independently of the floor-sampling budget.
Exceeding it stops inference with an error. Grid resolution and conservative enclosure tests can
leave narrow or exposed connections unresolved; this is mood inference rather than an exact
reconstruction of runtime lighting state.

The source report and runtime overlay show actual placed rectangles. A lighting label is mood
evidence, not proof of adequate illumination or complete source coverage.

## Interior sources

`prepare_interior_lights.py --inference REPORT --level-dir EXPORTED_LEVEL`
writes a new `probe-interiors.json` beside the exported level. It rejects a
different game/version/level and retains only unambiguous samples with sky
visibility at most 0.2. Keep the report and sidecar in ignored storage.
Use a fresh output directory when comparing source settings.

The native baker reads this sidecar before preparing transport. It uses
each sample's original ambient colour and both directional colours as a loose
RGB mood reference. The encoded reference is clamped and decoded to linear
colour; it is not an exact reconstruction of the original material response.
Ceiling sources follow the average geometric normal of a 3 by 3 ceiling sample
footprint, facing into the room regardless of triangle winding. Steep faces and
hits on unrelated storeys are excluded. Directional light settings do not rotate
these sources.

A source needs a ceiling between 3 and 32 units above a receiver one unit above
the floor, a ceiling geometric normal with absolute Z at least 0.65, and walls
in five of eight directions within 24 units at both receiver and half-ceiling height.
At most two of 32 upper-hemisphere rays may escape within 128 units, rejecting
open-sided shelters and outdoor overhangs. These tests ignore mesh winding.
All four corners must be visible from the receiver. The default rectangle is
3 by 3 units, offset 0.75 units into the room along the ceiling normal, with six units
between sources. Defaults permit 128 sources per component and 1024 per level.

Existing emitters visible within eight units of accepted source samples receive
at most a 2x bake-time boost. Surface radiance is capped at 4 and point intensity
at 64; originals already above these limits are preserved. A single RGB
multiplier preserves the original emission hue.
Runtime emission appearance is unchanged by this bake-only boost.

Ceiling strength defaults to 1.0 of the mood reference. A deterministic
128-sample estimate of existing direct floor irradiance reduces the ceiling
contribution before adding it. This estimates a scalar energy remainder, not
an RGB match. Each generated source is capped at linear radiance 12 with its
RGB ratios preserved. No ambient term is injected into every bounce.

`bake_level.py` bakes these sources into sparse and dense probes together.
Rectangles emit on one side, have finite extent, and use ordinary geometry
visibility and diffuse bounce transport. They add no collision geometry and
cannot block or appear in the game. Runtime reads the resulting probe atlases
through the existing enhanced lighting path. The sidecar is included in the
normal source fingerprint, so removing or changing it invalidates the bake.

`interior-lights-report.json` in the batch output records accepted source
positions, half-edges, radiance, reference receivers, target irradiance,
estimated existing irradiance, boost counts, rejected placements and limits.
All controls are optional fields of the sidecar's `settings` object; see
`InteriorLightSettings` in `port/enhanced/lighting/interior_lights.h`.

Generated lighting covers labelled patches, not every interior. Unseeded and
conflicting components remain unchanged. Generated sources have no visible
fixtures, the reference omits material-dependent original colour conversion,
and overlapping source contributions are not globally fitted. The goal is a
plausible colour and brightness mood, not an exact match to original rendering.

Synthetic `interior_lights` fixtures cover placement, enclosure rejection,
source budgets, hue-preserving caps, one-sided emission and occlusion.

## Showing generated sources

Enhanced, **Ceiling area lights** shows the exact source rectangles from the
current bake's `interior-lights-report.json`. Frames use the emitted colour,
normalised for visibility. Cyan arrows point toward the emitting hemisphere;
their three-unit length is a marker, not a light range. Hidden portions remain
faintly visible through geometry. The overlay never changes scene depth or
lighting, and works independently of **Probe spheres**.

The `ceiling_area_lights` setting persists the toggle and defaults to off.
It requires enhanced Vulkan rendering and an active, verified probe bake.
Missing reports produce no markers; malformed reports disable the diagnostic
without disabling the bake. Markers are cleared on level changes.

The `area_light_overlay` fixture checks source extents, arrow orientation and
malformed reports. `area_light_overlay_vulkan` checks visible and occluded
line rendering in a hidden synthetic scene, without loading game assets.
