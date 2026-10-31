# Offline interior lighting

Probe bakes infer interiors by default, using unambiguous indoor samples to guide bounded
emitter boosts and ceiling lights. Set `regions.interior_lighting: false` to skip preparation;
explicit source sidecars remain authoritative. Novalis NTSC-U (level 1) has the verified profile.

The native batch worker reads exported collision and retained gameplay without disc access, then
infers rooms and builds interior sources before GI in the same process. Generated
`probe-interiors.json` stays beside the export for runtime fingerprint validation; reports stay
with the bake. Sources refresh when inputs/tools change. Exported water remains non-walkable.

For manual input preparation, `port/enhanced/tools/baker/prepare_interiors.py` reads the owned
disc through existing read-only editor readers. It exports deduplicated static collision,
hero-light environment transitions, game/level identity and a SHA-256 over gameplay and the
collision-containing core tail. Derived game data stays local in ignored storage.

The unified `openrac-enhanced-baker --infer-interiors` stage:

1. Samples every collision layer on an XY lattice. Walkable slopes need absolute geometric
   normal Z at least 0.65; authored normals, winding and water cannot supply floor support.
2. Checks 2.5-unit vertical clearance and a sampled 0.45-unit body radius. Cardinal floor
   neighbours link across height differences up to 2 units, using clearance rays above the
   higher support. These are tunable proxies, not a movement capsule, jump solver or navigation mesh.
3. Cuts graph edges at each transition's centre plane within its oriented Y/Z extent.
   Collision and bake rays remain unchanged.
4. Labels every qualifying floor component below the transition centre and visible from its endpoint target. Gaps therefore
   need not split a room's mood. Walls and other transition boundaries block evidence. The best
   support remains the diagnostic seed; tiny ledges get a bounded penalty. Identical labels agree;
   conflicting or missing evidence remains unresolved.
5. Tests 16 upper-hemisphere rays up to 128 units. This measures finite-distance sky visibility,
   not irradiance or proof of enclosure; light set 0 does not mean outdoors.

JSON retains sample heights, components, sky visibility, both endpoint seeds, oriented transition
corners, labels and component sizes. Boxes do not replace the irregular sampled regions.

## Reproduce locally

Build with `OPENRAC_PROBE_BAKER=ON` and Embree:

```sh
cmake --build port/build/release --target openrac-enhanced-baker interior_inference_test
ctest --test-dir port/build/release -R '^interior_inference$' --output-on-failure
python port/enhanced/tools/baker/prepare_interiors.py --disc /path/to/owned-disc.iso --level 1 --output build/probe-gi/novalis-interior-input.json
port/build/release/openrac-enhanced-baker --infer-interiors build/probe-gi/novalis-interior-input.json build/probe-gi/novalis-interior-regions.json 1
```

On Windows, put the official Embree package's `bin` directory on PATH. Both tools refuse report
overwrites; neither opens the game. Synthetic tests cover roofed rooms, doorways, rotated/finite
portals, conflicting entrances, stacked surfaces, water rejection, reversed winding, disconnected
floors, routes around obstacles, shortest routes, walls and intervening transitions.

## Lighting-set propagation

`interiors.cpp` samples and connects floors; `interior_labels.cpp` propagates moods through
enclosed air. Shared queries/math remain private in `interior_inference_internal.h`.

Visible components receive endpoint labels first. Matching light-set/ambient pairs share a label;
direct evidence, including conflicts, keeps priority.

Unlabelled components use an air grid at floor-sampling spacing. Voxels need a ceiling within
32 units and walls in five of eight horizontal directions. Unobstructed six-neighbour links can
cross floor gaps and obstacles, regardless of winding. The shortest route supplies each
component's closest evidence; equal-distance moods stay ambiguous. Finite-width transition
boundaries extend vertically to prevent routes above them. Gameplay transition tests are unchanged.

The air cache has its own `Settings::max_cells` budget; overflow fails inference. Resolution and
conservative enclosure checks can leave narrow/exposed routes unresolved. A label indicates mood,
not exact runtime state, source coverage or sufficient illumination. Reports and overlays show
actual placed rectangles.

## Interior sources

`prepare_interior_lights.py --inference REPORT --level-dir EXPORTED_LEVEL` writes a fresh
`probe-interiors.json` beside the export. It rejects mismatched game/version/level and keeps only
unambiguous samples with sky visibility at most 0.2. Keep both files ignored and compare settings
in fresh output directories.

Before transport, the baker clamps and linearises each sample's original ambient and both
directional colours into an approximate RGB mood. It does not reconstruct original material
response. Ceiling rectangles face into the room along geometric normals averaged over a 3 by 3
sample footprint. Steep faces and unrelated storeys are excluded; winding and sunlight do not
orient the sources.

Placement requires:

- A ceiling 3 to 32 units above a receiver one unit above the floor, with absolute normal Z at least 0.65.
- Walls in five of eight directions within 24 units at receiver and half-ceiling height.
- No more than two escaping rays out of 32 upper-hemisphere rays within 128 units.
- All four rectangle corners visible from the receiver.

These winding-independent checks reject open shelters/overhangs. Defaults are a 3 by 3 rectangle,
0.75 units below the ceiling along its normal, six-unit spacing, 128 sources per component and
1024 per level.

Visible emitters within eight units receive at most a 2x bake-only boost. Surface radiance is
capped at 4 and point intensity at 64; already brighter sources retain their originals. One RGB
multiplier preserves hue, without changing visible runtime emission.

Ceiling strength defaults to 1.0 of the reference. A deterministic 128-sample estimate of existing
direct floor irradiance reduces the added energy. This is a scalar remainder, not an RGB fit.
Generated radiance is capped at 12 with RGB ratios preserved; no ambient term enters every bounce.

`bake_level.py` bakes finite, one-sided rectangles into sparse and dense probes using geometry
visibility and diffuse bounces. Sources add no collision or visible fixtures. Runtime samples
existing atlases; changing/removing the sidecar invalidates the normal source fingerprint.

`interior-lights-report.json` records rectangles, half-edges, radiance, reference receivers,
target/existing irradiance, boost counts, rejected placements and limits. Optional controls live
in the sidecar's `settings`; see `InteriorLightSettings` in
`port/enhanced/lighting/interior_lights.h`.

Only labelled patches receive generated lighting; unresolved/conflicting components remain
unchanged. The reference omits material-dependent colour conversion, and overlapping lights are
not globally fitted. The goal is plausible mood, not an exact match. Synthetic `interior_lights`
tests cover placement, enclosure, budgets, hue caps, one-sided emission and occlusion.

## Showing generated sources

Enhanced, **Ceiling area lights** reads rectangles from `interior-lights-report.json`. Frames use
normalised emission colour; cyan three-unit arrows mark the emitting hemisphere, not range.
Hidden portions stay faint. The overlay changes neither depth nor lighting and is independent of
**Probe spheres**.

`ceiling_area_lights` is saved, defaults off and requires enhanced Vulkan with a verified active
bake. Missing reports show nothing; malformed reports disable only this diagnostic. Level changes
clear markers. `area_light_overlay` checks extents, direction and malformed inputs;
`area_light_overlay_vulkan` checks visible/occluded lines in a hidden synthetic scene without game data.
