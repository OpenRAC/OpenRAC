# Probe baking tools

These tools prepare exported levels, plan probe coverage and bake GPU-ready probe assets.
The Python commands coordinate files and region planning. The native baker owns scene conversion,
Embree traversal and probe-map integration.

## Interface

- `bake_level.py`: bake and verify one sparse grid and its planned dense regions.
- `bake_all.py`: coordinate independent level bakes.
- `seed_regions.py`: inspect or regenerate dense coverage for an existing sparse bake.
- `activation.h`: resolve per-level profiles and validate assets against runtime settings.
- `level_scene.h`: convert a loaded level into transport geometry, lights and receiver bounds.
- `scene_geometry.h`: convert static render triangles and their colour, alpha and emission attributes.
- `probe_grid.h`: construct sparse, bounded or exact lattice allocations.
- `probe_fingerprint.h`: calculate source, scene and solver identities shared with activation.
- `batch.h`: bake multiple planned grids against one prepared scene.
- `interior_sources.h`: apply configured interior emission and generated ceiling sources.
- `report.h`: inspect assets and compare sparse and dense coverage.

### Planning order

1. Seed regions from authored transitions and whitelisted initial moby placements.
2. Extend coverage to nearby walls and ceilings within the configured limits.
   Moby-seeded expansion starts only where a source floor has a clear connection
   at a walkable height. Padding cannot seed rooms behind walls or upper storeys.
   Authored lighting-transition bounds remain explicit coverage evidence.
3. Compact the complete coverage, add an outer one-cell apron, then repartition into boxes.
4. Refine shared faces to penalise crowded blend junctions and small boxes.
5. Restore blend borders and emit exact lattice endpoints with source evidence.
6. In the native bake, align each final grid before relocating individual probes.

`region_boxes.py` defines integer cell operations. `region_compaction.py` preserves the native
coverage union, including existing world-bound rounding. `region_repartition.py` owns core
partitioning, final metadata and reporting. `region_refinement.py` owns the incremental contributor
counts and accepts only strict cost improvements in deterministic neighbour order.

Intended coverage remains full strength; fading occurs in the apron outside it. Repartitioning
preserves this core union and fade support. It discourages, but does not prohibit,
more than two overlapping blend bands. The probe budget
can force additional boxes; increasing density can therefore increase the region count.

### Native bake order

The `regions` section in `probe-bake.json` controls local region planning and coarse allocation.
Defaults are `"coarse_bounds": "local_regions"` and `"coarse_padding": 128`: the completed
local-region union plus padding, snapped outward to coarse cells. `"scene"` uses exported
scene bounds. Levels without local regions use scene bounds automatically. Bounds limit probe
allocation, not ray tracing. They save memory without increasing density; surfaces outside
the coarse box have no probe support.

Scene preparation converts the sky to linear radiance, exports authored suns, static geometry,
liquid sources and receiver bounds, then fingerprints the result. Geometry conversion preserves
source order and lazy texture remapping. Mesh normals do not determine transport normals.

The transport implementation remains in `../../lighting/`: `bake.cpp` coordinates preparation and
workers, `bake_alignment.cpp` aligns grids, `bake_scene.cpp` wraps Embree, `bake_transport.cpp`
traces radiance, and `bake_integration.cpp` integrates probe maps. Visibility distance is capped at
1.5 cell diagonals; radiance and occupancy still trace the full scene.
Sparse and dense maps retain filtered first and second distance moments. Sparse sampling halves
visibility variance only beyond the mean blocker distance. This reduces uncertain support without
erasing unobstructed outdoor fill. Very coarse grids and finite angular sampling can still leak.

Repeated texture colour conversions reuse bounded worker-local caches for the current rounding mode.
Each worker retains up to 65,536 exact byte/tint results, using about 1 MiB rather than storing
full linear copies of every texture.
Tinted emission and authored terrain keep their original colour conversion order. Visibility
queries sample alpha without calculating RGB. Batch reports reuse fixed scene and material text;
each grid still receives its own coverage report and runtime validation.

## Depends on

The scene, asset, common and enhanced lighting modules. Embree is the offline traversal dependency.
The Python planner uses the standard library and repository readers.

## State and threads

One `NativeBatch` owns the native process, its pipes and its log for a level. The native batch keeps
one prepared scene for all grids. Bake workers own their trace scratch and write disjoint probe
outputs. The layout refiner owns its cores, stable IDs and contributor counts for one planning call.
Runtime activation validates the same scene and solver identities without performing liquid analysis.

## Errors

Malformed inputs and failed verification stop the bake. Native tool errors reach its command-line
error boundary; Python propagates file, validation and subprocess errors. Registration is replaced
only after every requested asset and sparse/dense pair has been verified. Source snapshots detect
inputs changed during preparation or baking.

## Configuration

Authored JSON lives under `port/enhanced/lighting/profiles/`:

- `<game>/moby-seeds.json`: game-wide moby class whitelist and seed extents.
- `<game>-<version>/emission-materials.json`: shared emission definitions.
- `<game>-<version>/level_NN/`: level bake and emission overrides.

The planner resolves the moby profile from the exported game's identity;
`seed_regions.py --moby-whitelist` can override it. Standalone bundles carry
all authored JSON profiles, including the emission catalog. The example bake
profile stays beside the command-line tools.

`probe-bake.json` configures transport and grids; its `regions` section configures seeding,
coverage, coarse bounds and region budgets. `probe-emission.json` defines shared material emission overrides.
Command-line options can override resolved defaults. See the
[lighting guide](../../lighting/README.md) for commands and profile fields.

Source fingerprints use canonical file bytes for exported levels so runtime floating-point rounding
does not invalidate a bake. In-memory fixtures use structural transport hashes. Solver fingerprints
include sampling, grid, relocation and surface-emission gains. Changing hash field order or interpretation domains
invalidates assets and requires deliberate version handling.

## Automatic interior lighting

Normal level bakes prepare interior lighting before native scene fingerprinting. Exported collision
and retained gameplay feed `openrac-interior-infer`, then unambiguous low-sky samples drive bounded
emitter boosts and enclosed ceiling lights. The inference executable is built with the probe baker.
Levels without suitable authored lighting evidence produce no generated lights.

`regions.interior_lighting` in `probe-bake.json` defaults to `true`; set it to `false` to skip
automatic preparation. It is independent of `regions.enabled`, so sparse-only bakes can use it too.
Explicit `probe-interiors.json` sidecars are preserved. Opting out removes only automatically
generated sidecars. Automatically generated sidecars carry a
generator identity and refresh when collision, gameplay or inference tools change. They stay beside
the level export so runtime validation uses the same source identity; inference reports stay in the
bake output. Changing a generated sidecar invalidates older probe bakes for that export.
The multi-level cache also tracks explicit interior sidecars and records generated
sidecars after preparation, so an unchanged second run reuses the verified bake.

## Tests

Run the planner and orchestration tests with:

```sh
python -m unittest discover -s port/enhanced/tools/probes
```

Native tests include `probe_scene`, `probe_bake`, `probe_alignment`, `probe_bake_parallel`,
`probe_visibility`, `probe_integration`, `interior_lights`, `interior_inference` and `bake_liquids`.
`probe_gpu_vulkan` and `viewer_vulkan` cover uploaded probe data and rendered fixtures.
Region tests compare coverage unions, fade superlevels, deterministic output and allocation budgets.

## Further reading

- [Lighting and bake configuration](../../lighting/README.md)
- [Port coding conventions](../../../docs/CODING_CONVENTIONS.md)
