# Probe baking tools

Python prepares exports; the native baker plans local coverage, converts scenes, traces with
Embree and integrates GPU-ready probe maps.

The launcher invokes `openrac-enhanced-baker --bake-all` directly. One native process owns source
preparation, memory-aware level scheduling, a shared tracing pool, cache checks, verification and
installation. Source decoding, water numerics and lighting all run in-process. Missing retained
source inputs or a required water profile need a fresh level export.

```sh
openrac-enhanced-baker --bake-all --levels-dir /exports/ntsc --output-dir /cache/ntsc --jobs 0 --update
```

`--threads`, `--memory-mib`, `--rays` and `--bounces` override resource or transport controls.
`--dry-run` lists validated destinations without writing. `--profiles-dir` selects reviewed
shared emission and moby definitions. `water_sources` in the per-level `probe-bake.json`
configures native water decoding; no Python executable is needed by the bake flow.

The native cache uses semantic identities rather than executable timestamps, checks every
registered asset and keeps partial work for retry. Installation preserves the previous directory
until its replacement succeeds. Windows renames tolerate brief sharing locks. Cache version 5
replaces the Python coordinator's older stamps on the first native preparation run.

Older cache stamps require a rebake; no result migration is performed. Launcher bundles omit
the Python reference bakers and planners. `bake_all.py` and `bake_level.py` remain checkout-only
reference entry points. The `--coordinate-levels` protocol remains available for their
comparison tests. Standalone native stages include
`--batch-output`, `--infer-interiors`, `--prepare-level-interiors`, `--prepare-level-emission` and
`--bake-water`. See [water tools](../water/README.md).

## Interface

| File | Responsibility |
|---|---|
| `bake_level.py` | Bake and verify sparse/local grids |
| `pipeline.h` | Native launcher flow: scheduling, cache ownership, progress and publication |
| `bake_all.py` | Checkout-only reference coordinator |
| `seed_regions.py` | Inspect/regenerate local coverage for a sparse bake |
| `activation.h` | Resolve profiles and validate runtime assets |
| `level_scene.h` | Export transport geometry, lights and receiver bounds |
| `scene_geometry.h` | Convert static triangles with colour, alpha and emission |
| `probe_grid.h` | Allocate sparse, bounded or exact lattice grids |
| `probe_fingerprint.h` | Share source, scene and solver identities with activation |
| `batch.h` | Bake all planned grids from one prepared scene |
| `region_plan.h` | Seed, expand, merge and partition automatic local regions |
| `final_plan.h` | Assemble final bounds, preserve resume identity and register verified assets |
| `emission_prepare.h` | Refresh explicit material definitions while preserving local overrides |
| `emission_audit.h` | Inventory authored bright static surfaces from the resident decoded scene |
| `interior_sources.h` | Apply interior emission and ceiling sources |
| `report.h` | Inspect assets and compare sparse/local coverage |

### Planning order

1. Seed authored transitions and whitelisted initial moby placements.
2. Extend to nearby walls/ceilings within limits using native two-sided Embree collision queries. Moby expansion requires a clear walkable-height
   floor connection; padding cannot seed through walls or into upper storeys. Authored transition
   bounds remain explicit evidence.
3. Compact coverage, add a one-cell apron and repartition into boxes.
4. Refine faces to penalise crowded blend junctions and small boxes.
5. Restore blend borders and emit exact lattice endpoints/source evidence.
6. Fit upright grids to playable footprints, align them, then relocate individual probes.
7. With adaptive density enabled, verify reduced grids against the fine bake before reflection capture.

`region_seeds.cpp` reads retained gameplay and the matching moby profile. Collision is loaded once
for floor projection and the two-sided enclosure flood. `region_merge.cpp` preserves world and
float-allocated coverage; `region_partition.cpp` partitions cores and `region_refine.cpp` accepts
strict cost reductions in deterministic neighbour order. Integer cell storage and merge candidates
have explicit memory limits.

After scene inspection, the native controller plans regions, writes `seed-plan.json`, derives
padded coarse bounds and cell-diagonal visibility
limits, then bakes and verifies the grids. It writes both region registries and `bake-result.json`
before reporting completion. A changed seed plan invalidates only grid-owned resume files.

The inspection-only `region_plan` request still returns UTF-8-safe 16 KiB report fragments,
bounded to 16 MiB by the client. The Python planner remains a standalone reference for comparison
tests. Automatic baking does not send the completed region report through Python.

Intended coverage stays full strength; fading occurs outside it in the apron. Partitioning
preserves core coverage and fade support but may leave more than two overlapping bands. Probe
budgets can split boxes, so greater density can increase region count.

### Native bake order

`"strategy": "scene"` in `probe-bake.json` bakes one coarse volume over scene bounds using
`sparse_spacing`, without gameplay, local regions or floor-based reflections. The default
`"gameplay"` strategy runs the placement stages below. Both share the same GI transport.

Floor analysis is shared by local GI fitting and reflection placement. Turning off generated
interior lights does not disable that analysis. Reflection floor anchors also remain available
when local GI regions or moby-based GI seeding are disabled.

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

### Emission preparation

Automatic baking runs `--prepare-level-emission LEVEL_DIR CATALOG.json DEFAULTS.json` before
checking cached lighting or loading a scene. Matching uses the SHA-256 of exported PNG bytes,
with one bounded streaming buffer. Tracked level settings override catalog fields; independent
local edits override both. Removed or updated defaults are identified by `_catalog_defaults`.
Equivalent existing JSON keeps its exact bytes and modification time. Changed profiles are
published atomically so runtime and baking read the same definitions.

Batches write `emission-audit.json` from already decoded terrain, ties and shrubs, including each
instance's original light slots. The audit retains original glTF emission intent even when a
profile changes its visible intensity. It reports bright candidates without assigning emission.
The editor's Python resolver and audit remain references for export and comparison tests.

### Adaptive local density

`regions.adaptive_density` enables error-bounded spacing changes; Novalis enables it. The default
is off for other levels. `adaptive_relative_error` defaults to `0.1` and
`adaptive_absolute_error` to `0.02`, in linear irradiance. Dark references cap the absolute
tolerance at `0.003`.

Narrow corridor cross-sections can use half spacing within the original volume's probe budget.
After tracing the fine grids, each axis can double its spacing where exact lattice endpoints
permit it. Kept probes retain their captured directional maps, relocation and distance moments.
Exterior heights and confirmed interior ceiling coverage remain unchanged.

Checks cover player/jump heights and nearby walls/ceilings using two-sided geometric normals,
five view directions, packed texture precision, visibility and sparse/local boundary blending.
A final check includes selection and blending of three local volumes; failed reductions restore
the fine data. The tests sample reachable floors and surfaces rather than proving every pixel.

Final field validation shares the bake worker pool for its five-view GI queries. The controller
reduces results in receiver/view order, preserving error maxima and restoration decisions.
Candidate comparisons keep at most 512 receivers in temporary storage. Reflections reuse the
prepared playable-floor graph; standalone calls without prepared floors still load it normally.

`probe-density-report.json` records the final count and measured error. `adaptive-reference/`
keeps fine captures only for reduced volumes, for verification and resume; runtime loads only the
final region files. This pass reduces runtime storage, but still traces the fine reference first.

Each worker caches up to 65,536 exact byte/tint conversions per rounding mode, about 1 MiB,
rather than full linear textures. Tint/emission/terrain retain their colour-conversion order;
visibility reads alpha only. Reports reuse scene/material text but retain per-grid coverage
and runtime validation.

Grid alignment shares the bake worker pool. Clearance and occupancy samples run in parallel;
the controller sums each phase in the original order and resolves ties before applying offsets.
Workers inherit the caller's floating-point environment. Temporary score storage is bounded by
28 small preview phases or two full-grid phases; alignment does not change the bake identity.

## Environment captures

An enabled `environment_probes` section in `probe-bake.json` runs reflection capture after all GI
grids verify. Novalis enables it by default. `environment_stage.cpp` loads the completed GI and
the gameplay-anchored floor graph. `room_layout.cpp` supplies geometry-based cores, clearance
centres and upright principal axes for both bakers. `room_grid.cpp` fits local GI footprints without
growing their probe budgets. `environment_floor_map.cpp` prepares reachable reflection spaces;
`environment_placement.cpp` centres captures above their actual floors and fills coverage gaps.
`environment_probe_merge.cpp` coordinates receiver-preserving unions;
`environment_capture_search.cpp` tests replacement positions, `environment_capture_relocation.cpp`
repairs spacing, and `environment_merge_policy.cpp` compares lighting and coverage budgets.
`environment_bake.cpp` shades Embree hits with the runtime GI reference. `environment_filter.cpp`
builds seam-aware GGX levels and a matching BRDF lookup.
The versioned `environment.orpe` is consumed by enhanced water and all authored chrome/glass packets. See the
[lighting guide](../../lighting/README.md#environment-reflections) for defaults and limitations.

GI orientation and reflection placement share a connected floor graph. Short sampling gaps are
bridged only where actual floor support and body/head clearance agree. Physical reconnection
searches up to 12 units, following bounded floor-height dips instead of a straight height chord.
Intermediate samples keep
the distance field and medial paths continuous; collision component IDs do not define rooms.
Authored lighting labels preserve mood boundaries. Reflection placement then checks doorway
clearance and baked GI colour before combining spaces. Walls and separate storeys remain barriers.
The placement report records bridged edges and merged floor components. Influence covers the
playable floor footprints and player-height samples, independently of direct visibility from the
capture. Small occluders inside one lighting space do not require additional captures. Supported
low passages remain receivers even when a capture cannot fit there. Merging preserves every
receiver; incomplete coverage fails the bake. Parallax bounds remain separate from influence bounds.

Minimum reflection-capture spacing is six units. The closest visible pair is repaired first,
including pairs with different fragment IDs or local GI brightness. A shared replacement must
retain every receiver; otherwise the captures are relocated farther apart. Walls, separate storeys
and different water media remain separate. Unresolved spacing conflicts fail the bake.

The playable graph has separate walking, jumping and swimming links. Jump arcs sweep the player's
body against two-sided geometry and connect supported landings without filling the gap with floor
samples. `playable_space` in `probe-bake.json` configures `jump_distance` (12), `jump_rise` (4),
`jump_drop` (8), `player_radius` (0.5) and `player_height` (2.5). These are planning approximations,
not a gameplay-physics replay. The debug view draws the medial axis in orange and checked jump
arcs as dashed orange lines.

Authored swimmable collision adds surface and submerged gameplay domains. When `water.bin` is
available its wet mask refines coastlines; it is optional. Basin floors come from collision,
and sealed rooms below a basin remain dry. Reflection placement and merging retain separate
above-water and underwater captures. Their influence covers the water column, with a narrow
transition beneath the surface. Placement reports identify submerged captures and failed bounds.
Water shallower than `playable_space.minimum_swim_depth` (default 1) instead uses supported wading
floors and shares reflection spaces with dry land. Novalis sets 0.85, following the wading-depth
test in `update_hero_ground_material_effects` (seen in game code). It does not create an underwater
capture layer in shallow water.

## Depends on

Scene, asset, common and enhanced lighting modules, plus offline Embree. Python uses the standard
library and repository readers.

## State and threads

The native coordinator owns one thread pool shared by admitted levels' GI and reflection work.
Workers claim probes dynamically; progress callbacks remain on the calling thread. Tasks drain
before failure returns, and each stage restores its caller's floating-point environment.
Local-grid file writes and readback checks overlap the next grid on that same pool, with at most
two pending outputs per level. The coarse grid verifies synchronously before local bakes begin.
Diagnostics retain grid order; all pending checks drain before density checks, reflections or
publication. A one-worker pool executes output inline.

CPU decoding validates packed channels while expanding maps in one pass. Cached grid-header
recovery uses packed-only validation. Coverage checks still need expanded maps; density validation
shares unchanged volumes between reference/candidate checks and passes its final field directly to
reflections. Only reduced grids need separate fine-reference storage; no persistent cache is kept.
Expanded volumes release their temporary GPU byte buffers.

Standalone batches own a private pool. Controller threads perform serial scene preparation and
wait for preparation and final grid requests without occupying tracing workers.

`--jobs` limits admitted levels. `--memory-mib` limits estimated scene/planner reservations;
zero uses half physical RAM. Reservations are at least 1 GiB per level and four times exported
asset bytes; this is admission control, not a hard operating-system memory limit.

One `NativeBatch` owns a level's channel and log, sharing one prepared scene across grids.
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
Optional `travel` records in that profile define version/level, pvar byte offsets and validation
limits for moving seed bounds. Game-specific offsets stay out of the shared native planner.
Standalone bundles include all profiles/catalogs. The example profile stays beside the tools.
`probe-bake.json` configures transport/grids; `regions` controls seeds, coverage, bounds and budgets.
`probe-emission.json` supplies material overrides; CLI values override defaults. See the
[lighting guide](../../lighting/README.md).

Export fingerprints hash canonical file bytes, independent of runtime rounding. In-memory fixtures
use structural hashes. Solver hashes include sampling, grids, relocation and emission gains;
changing field order or interpretation domains invalidates data and needs version handling.

## Automatic interior lighting

Before scene fingerprinting, the native batch worker reads exported collision and retained gameplay,
infers rooms and generates interior sources in process. Unambiguous low-sky samples guide bounded
boosts and enclosed ceiling lights; missing authored evidence creates no sources.

`interior-input.json` remains a diagnostic geometry report used by swimming-surface analysis, not a
subprocess handoff. Interior reports and generated sources have separate source/algorithm cache
identities. Completed files replace prior outputs atomically.

`regions.interior_lighting` in `probe-bake.json` defaults to `true`, independently of
`regions.enabled`, including sparse-only bakes. Set it to `false` to remove generated sidecars
and skip preparation. Explicit `probe-interiors.json` files remain authoritative.

Generated sidecars record their generator identity and refresh when collision, gameplay or tools
change. They stay beside exports for runtime identity checks; reports stay with bakes. Changes
invalidate old bakes. The multi-level cache tracks explicit sidecars and records generated ones
after preparation, allowing unchanged runs to reuse verified data.

## Tests

`enhanced_baker_cli` checks multi-level admission, shared workers, disconnects, cold publication and
warm cache reuse on synthetic exports, including identical serial/queued grid files and resume.
`bake_workers` checks worker lifetime and failure draining. `bake_output_queue` checks queue bounds,
ordered reports, sticky failure, destruction and floating-point environment inheritance.
`lighting_probe_decode` compares single-pass expansion with the former path across rounding modes
and malformed data. `probe_density_stage` checks accepted reductions and returned restored fields.

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
