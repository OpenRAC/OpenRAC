# Enhanced baker

`openrac-enhanced-baker` prepares exported levels for enhanced rendering. It owns water preparation,
interior source generation, diffuse GI and reflection capture in one native process. The launcher
uses this same automatic pipeline; baking needs no Python executable or game window.

```sh
openrac-enhanced-baker --bake-all --levels-dir /exports/ntsc --output-dir /cache/ntsc --jobs 0 --update
```

Use an output directory outside the exported levels. `--update` retains verified matching bakes
and replaces stale data only after a successful bake. `--dry-run` lists validated destinations.
`--threads`, `--jobs` and `--memory-mib` control resources; `--rays`, `--bounces` and `--compression`
override transport or encoding. Source inputs are retained by level export. Missing inputs or a
required water profile need a fresh export.

Lossless LZ4 storage compression defaults on in the baker and launcher. Each published
`.orpg` or `.orpe` file uses a checksummed LZ4 frame when it reduces the file size.
Filenames and GPU payloads are preserved, and readers accept both plain and wrapped assets.
Use `--no-lz4-assets` to write plain assets, or configure `-DOPENRAC_BAKE_LZ4=OFF`
to build without the codec. Cache readiness follows the current baker's storage default.

GI and reflection placement share the **Layered watershed** graph. Each reachable free-space
basin owns its GI allocations and one reflection photograph. The production graph and fitted
volumes are described in `spatial-watershed.json`. Its `connections` index points at
`spatial-connections-…` chunks, which the preview loads when picking cells. Keep that directory
beside `bake-preview.html` for offline picking. Older bakes can still supply inline edges or
generate an explicitly labelled prototype for comparison.

Spans follow reachable floor/water evidence, with separate elevations in each XY column.
Collision bounds free intervals and checks graph edges; air analysis extends to four player
heights (capped at 16 units), while submerged spans extend towards the trusted waterline.
`ceiling: null` means no roof was found in the analysis range, not that a physical cap exists.
Lateral clearance, edge apertures, peak persistence and shared-view refinement define basins,
independently of authored lighting labels. Portals are connected patches of checked crossings.

Select **Layered watershed** in **Pick** to inspect cells, spans, ceiling limits and basin
contacts, or choose a basin from the layered-region controls. **Height bands** offers middle,
all and absolute-height slices; **Span columns** exposes the vertical intervals. **F**
frames the selected basin. Copy JSON is scoped to that basin and the picked cell/span.
Production island IDs are the same basin IDs used by the fitted volumes.

To inspect reflection captures, influence boxes and linked receivers over the collision mesh:

```sh
openrac-enhanced-baker --debug-preview-from /cache/ntsc/level_01 --level-dir /exports/ntsc/level_01
```

Open `bake-preview.html` in the bake directory. **View** switches between **Reflection**, **GI**
and **Layered watershed**. The page works offline and includes exported collision geometry,
so keep the preview and its data local. Use `--no-prototypes` to skip regenerating experimental
placement overlays. A native `--batch-output` run can generate the page with `--debug-preview`.

Use **Pick** to select captures, GI grids, watershed islands, collision pieces, area routes,
cells or portals. **F** frames the selection. The JSON copy controls export the selected record,
including probe dimensions and classification counts for GI, and influence and projection bounds
for reflection. **Config** adjusts overlay opacity and restores the default camera. The camera
and movement speed are remembered per preview path. Hold the right mouse button and scroll to
adjust fly speed. Ordinary scrolling zooms.

**Geometry** switches between solid shading, feature edges, wireframe and hidden geometry.
Overlays behind collision fade by default. **Playable evidence** shows gameplay anchors,
manual evidence boxes and swimming entries. Hollow markers identify water surface samples.

**Area routes** shows the diagnostic room-and-entrance graph. Open areas have reachable centres
and routes to checked entrances. Isolated areas appear as outline circles. Collision and clearance
checks constrain walking, swimming and shore connections. The medial distance field helps centre
narrow passages. Route generation is separate from production probe placement.

Failed batches can be previewed from their `.level_NN.partial` directories. **Placement failures**
marks uncovered receivers in red, locality conflicts in amber and incompatible ownership in
purple. Use **Previous/Next failure** to navigate the full list and **Copy failure JSON** to inspect
the receiver and candidate volumes. Candidates can appear before a reflection capture is baked.

A profile can supply optional `playable_areas` boxes with world-space `minimum` and `maximum`
triples. Only supported floor samples inside the boxes can become anchors. Keep the height bounds
tight to the intended floor. An empty array clears inherited boxes. Authored swimming areas
require a collision-checked entry from reachable land or a reachable basin floor.

For emissive review, look up a saved task against the level exports:

```sh
openrac-enhanced-baker --inspect-emissive-task EXPORTS REVIEW.json TASK_ID > picks.json
python label_emissive_capture.py picks.json capture.png labelled.svg
```

The report samples 81 rays through the saved NTSC camera and orders the material hits by
distance from the screen centre. It includes stable model and slot selectors, texture hashes,
and matching bindings in the current level. The labelled capture uses the same camera and
the uncropped game picture. The lookup uses exported geometry, including initial moby
placements, so confirm animated or scripted surfaces in the rendered capture.

## Source layout

| Location | Responsibility |
|---|---|
| `main.cpp`, `batch_command.cpp`, `batch.cpp` | CLI dispatch and one resident scene per level |
| `pipeline/` | Scheduling, progress, JSON, cache ownership and atomic publication |
| `sources/` | Profiles, scene conversion, emission, interiors and source fingerprints |
| `planning/` | Gameplay evidence, coverage, region partitioning and floor matching |
| `output/` | Compression handoff, density checks, GI/reflection files and reports |
| `probe_device.cpp` | Headless compression-format selection using local Vulkan dispatch |
| `../water/` | Native water decoding and numerical baking |
| `../../lighting/` | Embree traversal, sampling, map integration, codecs and shared asset formats |

The Python files are checkout-only reference implementations and comparison tests. They are not
shipped in launcher bundles. `bake_all.py`, `bake_level.py` and `seed_regions.py` can still be run
from a checkout. Standalone native commands remain available for inspection, source preparation,
interior inference, single-grid verification and water baking; see `--help`.

## Pipeline

1. Prepare native water fields and explicit emission definitions.
2. Check source/profile identities and admit levels within the memory budget.
3. Infer enclosed spaces, interior emission boosts and ceiling lights.
4. Load one transport scene and plan local GI coverage from gameplay evidence.
5. Bake the coarse grid, then local grids; verify density reductions against the fine field.
6. Capture reflections using the verified GI field, then generate GGX roughness mips.
7. Check source content again, verify all registered assets and publish the level atomically.

`probe-bake.json` beside the exported manifest is the per-level configuration, copied from the
matching tracked profile during export. Shared emission and moby definitions come from
`port/enhanced/lighting/profiles`, or `--profiles-dir`. The [example profile](probe-bake.example.json)
shows transport controls. Runtime activation uses the same profile interpretation.

The default `"strategy": "gameplay"` plans local grids and reflection captures around reachable
floors. `"scene"` instead bakes a single coarse grid over scene bounds for background scenes.
Coarse bounds normally enclose local regions with 128 units of padding; scene bounds are used
when local coverage is absent. Allocation bounds do not limit ray visibility.

Whitelisted gameplay placements anchor the reachable collision-floor graph. Clearance samples
follow sloping floors at the player radius, including curved tunnel floors. The floor graph
uses the coarse grid with half-spacing refinement only around failed links between physical
components. Fine samples keep the same collision/clearance checks, sample budget and authored
lighting boundaries; already connected areas retain the coarse grid.
The layered graph defines region ownership for both GI and reflection placement. A basin starts
with one oriented box. Feasible boxes are never split for a smaller volume score. GI grids split
only to meet grid-dimension/resource limits; reflection boxes split only to meet their configured
maximum extent. Adjacent parts merge whenever one feasible box can contain their combined support.
Every graph cell remains assigned; allocation failures stop publication rather than dropping cells.

Fitting searches yaw, pitch and roll using geometric seeds and bounded angular refinement.
Overlap with other basins contributes to orientation selection and is reported as a diagnostic;
it does not independently create additional volumes. Physical ceilings and the exterior height
policy expand finalized reflection influence without adding boxes or changing the photograph.
Expansion is bounded where it would newly engulf neighbouring receiver regions.

Each basin selects a render-clear capture cell near its interior centre. All its influence boxes
reference that photograph and share a parallax proxy enclosing their geometry. The surface audit
below measures remaining gaps without changing this allocation. The same rules apply to all levels.
Authored non-floor GI allocations remain supported for shafts and other explicit coverage.

The debug view exposes influence-region selection, all regions served by a capture, a
separate parallax-proxy overlay and minimal capture JSON with region references. Clicking
an influence-box edge selects its region and associated photograph.
Fit trials use PCA, hull-edge and diameter seeds followed by deterministic angular refinement.
This is a bounded approximate fit, not a proof of the globally minimum box. Legacy floor-only
placement helpers remain exercised by reference tests; production uses the shared layered graph.

The default transport uses 256 primary rays and 256 independent emitter samples per probe.
Finite emitters use multiple importance sampling with primary and bounce rays. Authored normals
never decide collision sidedness. Distance moments stop at 1.5 cell diagonals; radiance and
occupancy rays see the full scene. Both sparse and local grids store filtered distance moments.

Emission is explicit: reviewed texture identities and per-level definitions supply runtime and
bake intensity. Bright-material audits do not automatically create emitters. Generated interior
lights use enclosure and lighting-set evidence with configurable brightness limits. See the
[interior lighting description](../../../../docs/port/RAC1_INTERIOR_INFERENCE.md).

## Reflection surface coverage

After choosing reflection captures, the baker samples rendered triangles and water interfaces.
Collision and render visibility from reachable floor columns identify the relevant side of each
surface; sealed cavities and unseen background surfaces do not become coverage targets.
Adaptive triangle subdivision, alpha-cutout checks and conservative sample footprints reduce
missed narrow faces. This is sampled coverage, rather than a proof over every rendered pixel.

This pass is read-only. It never refits, subdivides, adds or moves influence boxes, photographs,
projection proxies or GI allocations. Gaps remain visible in the report instead of generating
independent surface patches. Opaque surfaces select influences half a unit toward the visible
geometric side; projection uses the actual surface position.

The placement report records area-weighted strong, weak and uncovered coverage using the runtime
0.75-unit blend margin, plus region and distinct-capture overlap counts. The preview's Reflection
mode has **Surface coverage** and **Include strongly covered surfaces** controls: red means missing,
orange means fading, blue marks strong samples inside more than two regions, and green means
strong coverage. The preview retains at most 50,000 diagnostic samples; aggregate statistics use
the complete sample set.

`--refresh-reflection-coverage BAKE_DIR --level-dir LEVEL_DIR` refreshes only the coverage audit report. It leaves the encoded reflection asset and all
placement metadata unchanged. Use the matching, unchanged level export for that bake.

## Threads and memory

One fixed worker pool serves all admitted levels. `--jobs 0` admits up to one level per four
workers; reservations are bounded by half physical RAM unless `--memory-mib` is supplied.
Reservations control admission, not a hard resident-memory limit.

`bake-all-result.json` records `elapsed_seconds` for each completed level, including preparation,
baking, verification and publication. Times begin at admission, exclude scheduler queue time, and
can overlap when multiple levels run together. Failed levels also retain their elapsed time.

The coarse grid verifies before local grids begin. Up to two joined output controllers per level
own traced grids while compression and density checks share the compute pool with later tracing.
The one-worker case serialises compute without nested pool waits. Reports retain submission order;
all work drains before whole-field verification, reflection capture or publication.

Each original grid and trial lattice is encoded once. Final output reuses the selected bytes;
only reduced grids retain a fine reference for whole-field checks. Source identities are shared
between stages, with full content checks before tracing and after baking. Metadata-only changes
cannot hide a source edit.

## Assets, verification and failure recovery

GI colour uses BC6H on supporting desktop adapters or ASTC HDR 8x8 on supporting adapters such as
Android devices. Unsupported hardware uses packed R11G11B10F. `--compression auto|bc6h|astc|none`
selects the policy. Canonical depth maps use RG16 UNORM mean/spread: 256 bytes per probe,
plus 16 bytes of FP32 ranges. Two hardware gathers retain stable FP32 variance interpolation;
relocation/classification remains RGBA16F.

A GI probe has a 6x6 interior plus one border texel on each side. BC6H shares endpoints across its
four physical blocks to preserve border pairs exactly. ASTC permits at most 0.5% linear RGB border
error relative to source luminance, with a floor for dark colours. Reflection faces have mip
padding, box-projection metadata and a separate uncompressed BRDF lookup. Stored payloads are
ready for GPU upload; the port does not recompress them.

Each level publishes `sparse.orpg`, `region-NNN.orpg`, `sparse.orpg.regions.json`, `environment.orpe`
when enabled, `bake-result.json` and `openrac-bake.json`. Diagnostic JSON, per-grid text reports,
`batch.log` and the version-wide `probe-bake.log` remain available for inspection. Format and
algorithm versions invalidate old caches; there is no result migration.

Saved files must match their expected size and SHA-256. In-memory format, identity, coverage and
runtime-rounding checks still run. Density verification uses the runtime three-volume selection
and restores fine grids when reduced overlap changes lighting beyond the error budget.

A failed level retains partial grids and logs while preserving the installed bake. Replacing a
level uses a recoverable previous directory; the cache lock prevents simultaneous writers. On
Windows, atomic renames tolerate brief sharing locks. Other admitted levels can continue after
one fails. Restart the game to load new data.

Finite probe density and angular resolution can still leak light. Interiors approximate the
original lighting mood and may remain too dark. Static bakes do not track changing geometry or
emission. Sparse coverage ends at its allocation bounds.

## Verification

Run the baker's Python tests and the native CTest groups after configuring the port:

```sh
python -m unittest discover -s port/enhanced/tools/baker -v
ctest --test-dir port/build/release -R 'probe_|bake_|enhanced_baker|enhanced_water_sources' --output-on-failure
```

The native launcher-flow tests use synthetic exports to cover source preparation, water, emission,
cache invalidation, interruption, background scenes and publication. Codec tests compare worker
counts, paired borders, runtime rounding and malformed input. Output-queue tests cover bounded
ownership, ordered completion, shared one-worker execution and failure draining.
