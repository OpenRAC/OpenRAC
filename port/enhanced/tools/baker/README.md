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

Local coverage starts from authored lighting transitions and whitelisted mobys. Two-sided
collision queries extend interior ceilings without including outdoor overhangs. Moby expansion
requires a supported floor connection. Coverage is merged, partitioned and fitted to playable
footprints before whole-grid alignment and individual-probe relocation. Partitioning preserves
coverage and blend borders; higher density can require more regions.

The default transport uses 256 primary rays and 256 independent emitter samples per probe.
Finite emitters use multiple importance sampling with primary and bounce rays. Authored normals
never decide collision sidedness. Distance moments stop at 1.5 cell diagonals; radiance and
occupancy rays see the full scene. Both sparse and local grids store filtered distance moments.

Emission is explicit: reviewed texture identities and per-level definitions supply runtime and
bake intensity. Bright-material audits do not automatically create emitters. Generated interior
lights use enclosure and lighting-set evidence with configurable brightness limits. See the
[interior lighting description](../../../../docs/port/RAC1_INTERIOR_INFERENCE.md).

## Threads and memory

One fixed worker pool serves all admitted levels. `--jobs 0` admits up to one level per four
workers; reservations are bounded by half physical RAM unless `--memory-mib` is supplied.
Reservations control admission, not a hard resident-memory limit.

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
selects the policy. Visibility remains RG32F and relocation/classification remains RGBA16F.

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
