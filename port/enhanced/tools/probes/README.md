# Probe baking tools

Python prepares exports and coverage; the native baker converts scenes, traces with Embree and
integrates GPU-ready probe maps.

## Interface

| File | Responsibility |
|---|---|
| `bake_level.py` | Bake and verify sparse/local grids |
| `bake_all.py` | Coordinate level bakes |
| `seed_regions.py` | Inspect/regenerate local coverage for a sparse bake |
| `activation.h` | Resolve profiles and validate runtime assets |
| `level_scene.h` | Export transport geometry, lights and receiver bounds |
| `scene_geometry.h` | Convert static triangles with colour, alpha and emission |
| `probe_grid.h` | Allocate sparse, bounded or exact lattice grids |
| `probe_fingerprint.h` | Share source, scene and solver identities with activation |
| `batch.h` | Bake all planned grids from one prepared scene |
| `interior_sources.h` | Apply interior emission and ceiling sources |
| `report.h` | Inspect assets and compare sparse/local coverage |

### Planning order

1. Seed authored transitions and whitelisted initial moby placements.
2. Extend to nearby walls/ceilings within limits. Moby expansion requires a clear walkable-height
   floor connection; padding cannot seed through walls or into upper storeys. Authored transition
   bounds remain explicit evidence.
3. Compact coverage, add a one-cell apron and repartition into boxes.
4. Refine faces to penalise crowded blend junctions and small boxes.
5. Restore blend borders and emit exact lattice endpoints/source evidence.
6. Align each final grid, then relocate individual probes in the native bake.

`region_boxes.py` owns integer cell operations; `region_compaction.py` preserves native coverage,
including rounded world bounds. `region_repartition.py` partitions cores and reports metadata.
`region_refinement.py` maintains contributor counts and accepts only strict cost reductions in
deterministic neighbour order.

Intended coverage stays full strength; fading occurs outside it in the apron. Partitioning
preserves core coverage and fade support but may leave more than two overlapping bands. Probe
budgets can split boxes, so greater density can increase region count.

### Native bake order

The `regions` section in `probe-bake.json` controls planning and coarse allocation. Defaults
`"coarse_bounds": "local_regions"` and `"coarse_padding": 128` bound the local-region union plus
padding, snapped outward to coarse cells. `"scene"`, or no local regions, uses scene bounds.
This limits allocation, not tracing; receivers outside the coarse box have no support.

Preparation converts sky to linear radiance and exports authored suns, static geometry, liquid
sources and receiver bounds, preserving source order and lazy texture remapping. It fingerprints
the scene; transport normals ignore authored mesh normals.

Transport lives in `../../lighting/`: `bake.cpp` coordinates workers, `bake_alignment.cpp` aligns
grids, `bake_scene.cpp` owns Embree, `bake_transport.cpp` traces radiance, and
`bake_integration.cpp` integrates maps. Visibility distance is capped at 1.5 cell diagonals;
radiance/occupancy trace the full scene. Both grids store filtered first/second distance moments.
Sparse sampling halves variance only beyond the mean blocker, preserving unobstructed fill.
Coarse grids and finite angular sampling can still leak.

Each worker caches up to 65,536 exact byte/tint conversions per rounding mode, about 1 MiB,
rather than full linear textures. Tint/emission/terrain retain their colour-conversion order;
visibility reads alpha only. Reports reuse scene/material text but retain per-grid coverage
and runtime validation.

## Depends on

Scene, asset, common and enhanced lighting modules, plus offline Embree. Python uses the standard
library and repository readers.

## State and threads

One `NativeBatch` owns a level's process, pipes and log, sharing one prepared scene across grids.
Workers own trace scratch and disjoint outputs. Each planning call owns refinement cores, stable
IDs and contributor counts. Runtime checks identities without repeating liquid analysis.

## Errors

Invalid inputs or verification failures stop baking. Native errors reach the command-line
boundary; Python propagates file/validation/subprocess errors. Registration changes only after
all assets and sparse/local pairs verify. Source snapshots detect changes during preparation/baking.

## Configuration

Authored JSON lives under `port/enhanced/lighting/profiles/`:

- `<game>/moby-seeds.json`: class whitelist and seed extents.
- `<game>-<version>/emission-materials.json`: shared emission definitions.
- `<game>-<version>/level_NN/`: per-level bake/emission overrides.

Game identity selects the moby profile; `seed_regions.py --moby-whitelist` overrides it.
Standalone bundles include all profiles/catalogs. The example profile stays beside the tools.
`probe-bake.json` configures transport/grids; `regions` controls seeds, coverage, bounds and budgets.
`probe-emission.json` supplies material overrides; CLI values override defaults. See the
[lighting guide](../../lighting/README.md).

Export fingerprints hash canonical file bytes, independent of runtime rounding. In-memory fixtures
use structural hashes. Solver hashes include sampling, grids, relocation and emission gains;
changing field order or interpretation domains invalidates data and needs version handling.

## Automatic interior lighting

Before fingerprinting, exported collision and retained gameplay feed `openrac-interior-infer`
(built with the baker). Unambiguous low-sky samples guide bounded boosts and enclosed ceiling
lights; missing authored evidence creates no sources.

`regions.interior_lighting` in `probe-bake.json` defaults to `true`, independently of
`regions.enabled`, including sparse-only bakes. Set it to `false` to remove generated sidecars
and skip preparation. Explicit `probe-interiors.json` files remain authoritative.

Generated sidecars record their generator identity and refresh when collision, gameplay or tools
change. They stay beside exports for runtime identity checks; reports stay with bakes. Changes
invalidate old bakes. The multi-level cache tracks explicit sidecars and records generated ones
after preparation, allowing unchanged runs to reuse verified data.

## Tests

```sh
python -m unittest discover -s port/enhanced/tools/probes
```

Native tests: `probe_scene`, `probe_bake`, `probe_alignment`, `probe_bake_parallel`,
`probe_visibility`, `probe_integration`, `interior_lights`, `interior_inference`, `bake_liquids`.
`probe_gpu_vulkan` and `viewer_vulkan` cover uploads/rendering. Planner tests check coverage unions,
fade superlevels, determinism and budgets.

## Further reading

- [Lighting and bake configuration](../../lighting/README.md)
- [Port coding conventions](../../../docs/CODING_CONVENTIONS.md)
