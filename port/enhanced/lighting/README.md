# Baked probe lighting

Probe GI requires Vulkan, enhanced rendering enabled at startup and a verified level bake.
Otherwise original lighting applies and Enhanced controls are disabled, retaining saved settings.
Core Vulkan optimisations work in either mode. Interiors can still be too dark.
See the [probe baking tools](../tools/probes/README.md) for pipeline details.

## Build

`OPENRAC_ENHANCED_RENDERING` builds in by default; the `enhanced` preset uses release settings
in a separate folder. Enable it through Mods > **Enhanced graphics**, `enhanced_rendering = 1`,
`OPENRAC_ENHANCED=1` or `OPENRAC_PROBE_GI`. It is off at runtime by default; OpenGL uses original lighting.

`python3 tools/build_port.py --probe-baker` statically links pinned Embree 4 into
`openrac-enhanced-baker`, as shipped by the launcher, without separate Embree/TBB libraries.
Alternatively use `-DOPENRAC_PROBE_BAKER=ON` with an installed Embree 4; Windows needs its DLLs
on PATH. Python needs no extra packages; the game does not use Embree.

## In the launcher

Mods > **Enhanced graphics** selects Vulkan for the current disc and runs `probe-bake`
on first use and after re-exporting levels:

```sh
python port/enhanced/tools/probes/bake_all.py --levels-dir <levels> \
  --output-dir <data>/probes/<version> --baker <openrac-enhanced-baker> --disc <disc> --jobs 0 --update
```

`--jobs 0` admits up to one level per four available bake threads. One native process shares its
tracing workers across admitted levels; estimated scene reservations are capped by half physical
RAM, or `--memory-mib`. This controls admission rather than enforcing a hard resident-memory cap.
`--update` compares exports, level format, baker and options against `openrac-bake.json`, replacing
stale data only after verification. The game uses `--probes <data>/probes/<version>` to load
`level_NN/sparse.orpg`; missing/currently invalid bakes retain original lighting.

## Bake a level

After extracting your own disc, run from the repository root:

```sh
python port/enhanced/tools/probes/bake_level.py \
  --level-dir build/native-ntsc-data/port/ntsc/level_01 \
  --output-dir build/probe-gi/novalis-new-batch
```

Use a fresh output directory outside the source level. Settings, emission and planning feed one
native process, which expands enclosed coverage with two-sided collision rays and shares its
converted scene/Embree data across all grids. Registration waits
for every file's readback, runtime-rounding and sparse-grid comparison checks. Source changes
before loading, after planning or after baking abort the batch.

Failures retain logs without registering partial results; existing batches stay intact. Restart
running games to load new data. `batch.log` contains diagnostics; each probe file has a `.txt` report.

Outputs include `sparse.orpg`, `region-NNN.orpg`, `sparse.orpg.regions.json`,
planning reports, logs and `bake-result.json` with format/bake versions.
`--rays` and `--bounces` set quality for the whole batch.

Clustering caches eligible pair scores in a priority queue, recomputing only changed pairs.
Stable IDs preserve best-first ties; generations reject stale entries without changing bounds,
source assignments or coverage checks.

To bake every exported level in one version, use a fresh output root:

```sh
python port/enhanced/tools/probes/bake_all.py \
  --levels-dir build/native-ntsc-data/port/ntsc \
  --output-dir build/probe-gi/all-levels
```

Each `level_XX` uses its own settings. Runs are sequential by default; failures retain logs and
allow remaining levels to continue. `bake-all-result.json` records results; any failure returns
nonzero. `--dry-run` lists levels without writing. Options include `--baker`, `--disc`, `--rays`,
`--bounces` and `--threads`.

`--threads N` limits `bake_level.py`, `seed_regions.py` and `openrac-enhanced-baker`: 1 is serial,
0 automatic. Workers share acceleration data and preserve bytes/identity.
Whole-field GI validation uses the same workers, with ordered error reduction and bounded comparison buffers.
`probe_bake_parallel_test --benchmark` checks equality and timing with one, two, four and automatic
threads on synthetic data, without launching the game.

Embree separates opaque/cutout geometry. Unit-alpha opaque textures skip callbacks at cutoffs
up to 0.5; other cutouts filter bilinear alpha during traversal. Zero-alpha textured triangles
are omitted only for positive cutoffs. Untextured acceptance, materials and emitter order stay intact.

Sunlight and emitter visibility use occlusion queries that stop at the first
accepted blocker. Sun rays are unbounded; emitter rays end just before the
sampled light. Transport and probe placement still use closest hits. Each
primary ray shares its first hit between GI and distance moments: transport
stays unbounded, while moments clamp distance to the grid's visibility range
(normally 1.5 times the cell diagonal).

Exports need `bake-inputs/`. Re-extract older exports or pass
`--disc <owned-local-disc.iso>` for planning. Gaspar lava also needs exported
overlay/core inputs or a prepared legacy surface. Keep inputs and bakes local
and untracked; PAL and NTSC need their own exports.

## Level settings

`probe-bake.json` overrides defaults; only `"version": 1` and changed values are needed.
The [example](../tools/probes/probe-bake.example.json) lists defaults. Delete it and any mod profile
to reset. CLI/runtime overrides win; `--print-profile` shows resolved settings. Baking never edits
the profile: retain it as an input record and rebake sparse/dense together after input changes.

`sunlight_direction` normalises a world-space travel vector, e.g. `[0, 1, -1]` points downward
toward +Y (Z is height). Zero/nonfinite vectors fail; omission or `null` keeps authored directions.
Runtime direct sun, cascades, grounded Ratchet shadows and baked sun bounces share it. Fill and
airborne shadows retain their directions. Rebake after changes.

The [Novalis NTSC profile](profiles/rac1-ntsc/level_01/probe-bake.json) sets sky intensity 0.5,
sky weight 0.3, sunlight direction and 8/3-unit dense spacing, two-thirds of the standard 4 units.
The PAL profiles (`profiles/rac1-pal/`) start as copies of NTSC-U's: the levels' classes, textures
and coordinates are expected to match; check a PAL bake before tuning them apart.
Export copies `profiles/<game>-<version>/level_NN/probe-bake.json`; launcher bundles include these
profiles. Baker/runtime read the exported copy without a checkout. Missing profiles use defaults.
Re-export and rebake after tracked settings change. Profiles contain no extracted meshes/bakes.

| Setting | Default |
| --- | --- |
| Placement strategy | `gameplay` |
| Sparse / dense spacing | 256 / 4 units |
| Probe map | 6x6 texels, 8x8 with border |
| Rays / bounces | 1,024 / 3 |
| Sky intensity / sky colour weight | 1 / 0.7 |
| Sunlight RGB scale | `[1, 1, 1]` |
| Sunlight direction | Authored light sets |
| Terrain vertex albedo | `true` |
| Distance exponent / inside fraction | 32 / 0.75 |
| Relocation | 4 attempts within 45% of minimum spacing |

`sunlight_scale` multiplies primary sun RGB in runtime lighting and baked
bounces; each channel accepts 0 to 4. Equal channels change strength without
changing colour. `terrain_vertex_albedo` keeps authored terrain lighting as a
material tint. Both settings require a rebake.

`"strategy": "scene"` selects one coarse GI volume around all baked geometry and initial moby
receivers, padded by one cell. It uses `sparse_spacing` and the normal lighting, emission,
relocation and visibility settings. It skips gameplay seeds, local volumes, ceiling-light
generation and floor-based reflection capture placement, even if those features are enabled
in other profile fields. Background scenes use this strategy. Sky-only scenes without static
geometry produce a verified empty result because they have no surfaces to light.

The default `"strategy": "gameplay"` uses floor-seeded placement. The optional `regions`
section in `probe-bake.json` controls its local probe placement:

```json
{
  "version": 1,
  "regions": {
    "enabled": true,
    "coarse_bounds": "local_regions",
    "coarse_padding": 128,
    "padding": 8,
    "vertical_padding": 4,
    "max_probes": 4096,
    "max_regions": 1024,
    "no_moby_seeds": false,
    "no_geometry_coverage": false,
    "geometry_max_rise": 32,
    "geometry_padding": 3
  }
}
```

Only `version` is required. Coarse allocation uses local-region bounds plus 128-unit padding,
snapped outward; `"scene"`, or no local regions, uses scene bounds. Tracing remains unbounded.
`regions.enabled: false` selects sparse-only; space levels can widen sparse spacing.

Seeds include lighting volumes, markers, crates, bolts, pistons and enemies, covering piston
travel/jump clearance. Airborne markers use separate ground/actor bands; known piston travel
stays continuous. Height-preserving merges before/after classification keep taller regions
from expanding lower neighbouring footprints.

Indoor cells need walkable support, mostly overhead coverage and walls in five of eight
directions within 32 units. They extend to the first ceiling with at least one probe cell of
surface support. Connected growth can fill marker gaps within the seed's floor-height band,
checking walls. Outdoor/unresolved seeds keep their heights; roof-only overhangs do not qualify.

Indoor/outdoor boundaries partition regions before bounded merging and overlapping budget splits.
Spacing, height/probe limits, 1,024 regions and 256 MiB remain fixed. Enclosure work is capped at
4,096 samples per seed. `geometry_max_rise` limits added height; higher ceilings report incomplete
coverage and missing ceilings stay unresolved. Classification changes neither transport/lighting
inputs nor creates lights from light-bank IDs.

`seed-plan.json` includes indoor and unresolved/outdoor sample counts,
connected cells added, height caps and sampling-cap diagnostics. Dense regions
are split with overlap. `seed-plan.json` records skipped seeds and limits;
enemy patrol routes are excluded. The all-level bake cache includes planner
scripts, per-level region/bake settings and emission/interior sidecars, so changes invalidate cached
layouts even when the native baker executable has not changed.
`seed_regions.py` can inspect or replace regions for an existing sparse bake, saving the previous list.

## Formats and compatibility

Only **file format 5, bake revision 4** is supported. Rebake older data.
Each texel integrates a regular 2x2 angular footprint, including emission.
Irradiance and distance moments combine the four directions before normalising.
The footprint softens visibility boundaries; thin walls can still leak.

Patterns cache by resolution/rounding mode; rotations and bounce/emitter directions stay
per-probe. Portable C++ integrates four texels together in unchanged accumulation order.
Exponent 32 uses repeated squaring, others `pow`; no SIMD library or fast-math flags are needed.

Upright grid frames rotate XY cell coordinates while rays and irradiance directions stay in world space.
Relocation is stored in the grid frame for direct GPU upload; CPU reference offsets are world-space.

Headers contain game/level identity, scene/settings hashes and grid/atlas
layout. Values are little-endian; texture planes upload without repacking:

| Plane | Format | Contents |
| --- | --- | --- |
| Irradiance | R11G11B10F | E^(1/5), including map borders |
| Distance | RG32F | Mean distance and mean squared distance |
| Metadata | RGBA16F | Total offset XYZ, validity and individual-relocation status |

Volumes use rows of 64 probe tiles, with unused tiles zeroed. Runtime keeps
file bytes for upload/streaming; Vulkan still needs a staging copy. Inspection reports
count these padded GPU planes separately from the file header. CPU tools
can expand maps. Sampling blends sqrt(E), squares back to E and divides by pi
once for diffuse shading. Distances and biases use game units; Z is height.

## Bake lighting

Default sky light is `0.3 * primary directional colour + 0.7 * sky_intensity * sky`.
`sky_weight` changes that mix. Terrain, ties, shrubs and selected static mobys
block and bounce light. Moving objects receive GI without updating the bake.

Runtime and bake share emission textures and colours for lamps, energy
spheres, Orxon ooze/windows and Gaspar lava. `probe-emission.json` overrides
emission; ordinary prelighting is not emission. Animated liquids are averaged,
and prepared legacy lava takes priority to avoid double counting.

Export and standalone bundles include `probe-emission.json`. `static_mobys` permits reviewed
initial scenery meshes; manifest/placement `bake_static` flags take precedence. Animated meshes
stay excluded. Scalar `runtime_intensity` multiplies the whole texture, without masks or RGB tint.
The old `intensity` key is rejected: refresh old exports and rebake.

Optional `bake_intensity` overrides transport only, defaults to runtime intensity and bypasses
automatic interior boosts. Novalis mushrooms use runtime intensity 1 and bake intensity 4:
the rigid underside emits into the bake; the animated cap retains runtime emission.

Per-material `contrast` in `probe-emission.json` ranges from 1 to 4 (default 1). After linear
filtering/optional vertex tint, emission RGB gains `(luminance / 0.18)^(contrast - 1)` before
intensity. Higher values darken below-middle-grey emission and brighten highlights, preserving
hue, black, alpha and diffuse colour. Runtime/bake use the same curve; Novalis mushrooms use 1.5,
others 1. Rebake/reload after changes, e.g. `{"runtime_intensity": 1, "bake_intensity": 4, "contrast": 1.5}`.

The version's `emission-materials.json` catalog identifies reviewed PNG content by SHA-256,
independent of level/texture numbering. Export/preparation resolves it into `probe-emission.json`;
runtime needs no catalog/checkout. Per-level values, including explicit zero, override defaults.
`_catalog_defaults` lets existing exports refresh tracked defaults while retaining local authoring.
Changed texture content needs a reviewed identity; brightness never implies emission.

`vertex_colour` uses ties'/shrubs' original instance light-slot colours in runtime and transport,
not base mesh colour. An empty object marks reviewed ordinary material without overriding glTF emission.

Each bake writes `emission-audit.json` listing fully bright static surfaces and whether
they have a definition. Undefined entries are review candidates, not proof of emission.
Dynamic scripted brightness and glow packets retain their runtime paths; moving geometry
is not silently converted into static bake sources.

### Emission GI gains

`surface_emission_scale` in `probe-bake.json` defaults to 1. This gain
multiplies direct and bounced GI from all geometric emissive surfaces, including
explicit `bake_intensity` values, after interior boosts. It leaves visible emission,
sky, sunlight, point lights and generated ceiling area lights unchanged.
Changing it requires a rebake.

`surface_emission_regions` in the same profile supplies room-local absolute gains:

```json
"surface_emission_regions": [
  {"minimum": [135, 83, 30], "maximum": [158, 105, 42], "scale": 8}
]
```
Only emissive triangles fully contained in a box use its gain; later matching boxes
win. Veldin uses this box for Ratchet's garage, including its green panels and blue
instruments. Reused textures outside the garage retain the global 1x gain. These
overrides affect neither visible glow nor generated ceiling area lights.

Probes include visible direct light from emissive surfaces, finite point emitters
and generated ceiling area lights, even with zero surface bounces. These sources
also illuminate bounce surfaces. Explicit source sampling applies finite shadow
visibility and the receiving cosine during probe-map integration; adding ray-hit
emission again would count the same direct energy twice. Analytic sun direct
lighting remains a runtime contribution.

The offline baker separates scene validation/Embree ownership (`bake_scene.*`),
materials and sky (`bake_material.*`), transport (`bake_transport.*`), and probe
workers (`bake.cpp`). `bake_integration.*` contains the portable texel kernel.

`bake_alignment.*` previews 27 shared offsets with up to 64 deterministic probes before
relocation. A full-grid check must improve mean capped squared clearance, penalising near/inside
placements. Short two-sided rays estimate clearance; paired crossing parity estimates occupancy,
ignoring authored normals. Anchored outer faces preserve coverage/counts/atlases. Alignment and
relocation share the cell-local offset limit; failed aligned probes retry their original positions.

The baker derives normals from triangles. Runtime uses camera-facing normals,
receiver biases and distance moments to limit leaks; thin walls can still leak.

## Interior lighting

[Interior inference](../../../docs/port/RAC1_INTERIOR_INFERENCE.md) uses transition-labelled
collision samples to boost emitters within bounds and generate finite ceiling lights.
`probe-interiors.json` is prepared beside the export. Setting `regions.interior_lighting` to
`false` in `probe-bake.json` removes generated sidecars and skips preparation; explicit sidecars
remain authoritative. Missing labels produce no lights. Runtime emission appearance is unchanged.

Ceiling targets convert reflected radiance to irradiance by pi, matching diffuse E/pi. Existing
emitter estimates use those units before reducing fill. Rectangles face inward along geometric
ceiling normals averaged over a 3 by 3 footprint, independently of winding/sunlight.

## Dense regions

Vulkan retains up to 1,024 local regions in 256 MiB, bounded by device texture limits.
Atlases start 128 tiles wide and widen to fit height, placing complete 64-tile baked rows side by
side without decoding/repacking. GPU frustum culling ranks squared camera-to-bounds distance,
breaking ties by region ID. The nearest 128 visible regions share a lookup; 16x16 tiles hold
128-bit candidate masks. Fragments visibility-test and normalise at most three local volumes.
Incomplete support and regions outside the lookup use the level's sparse grid.

Secondary cameras build their own lookup. Transparent receivers use that lookup
without opaque-depth tile rejection. When GPU selection or atlas capacity is
insufficient, the CPU chooses three camera-visible regions and fades changes.

`<sparse.orpg>.regions.json` loads automatically. `OPENRAC_PROBE_REGIONS` selects
another list (`0` disables it); paths are relative to the list.
`OPENRAC_PROBE_DENSE` adds one explicit region.

The geometry-aware planner preserves padding and only merges boxes whose
rectangular union introduces no extra spatial volume. This retains smaller local
coverage instead of filling empty corners between overlapping seed boxes. More
regions can duplicate support probes and incur atlas row padding; the planner's
resident-byte report includes that cost. Capacity changes require a new bake.

Level loading hashes the source scene once for the sparse/dense asset set. Each
asset still validates its version, level, bake settings and scene fingerprint.

## Controls

Set `OPENRAC_PROBE_GI` to the sparse bake before launching. With valid probes,
F4 toggles enhanced lighting. The Enhanced menu also provides:

- Enhanced graphics controls probe lighting; Modern shadows has its own toggle.
- GI/GTAO strength (default 1) and normal/view biases (default 0.5 units).
- Probe spheres: cyan sparse, green dense, gold relocated, magenta invalid.
  Gold marks individual relocation only; shared grid alignment retains cyan or green.
  Existing metadata stores this without extra GPU storage. Vulkan uploads records once;
  compute filters range/frustum/selection and compacts indirect draws. Fades and faint hidden
  spheres need no per-frame CPU scan/upload. A CPU reference supports checks/fallback;
  GPU debug triangles are not counted.
- Ceiling area lights: source-coloured rectangles and cyan emitting-direction
  arrows from the completed bake report, with faint hidden edges.

GTAO mainly darkens indirect light. Modern shadows retain 10% directional/fill
light. Ratchet casts along sunlight when grounded and downward when airborne,
with an extra footprint cue in shade. See the
[debugging guide](../../../docs/port/DEBUGGING.md) for environment options.

## Checks and references

```sh
python -m unittest discover -s port/enhanced/tools/probes
```

Native fixtures cover formats, baking, CPU/GPU sampling, regions, shadows,
GTAO and feature gating. CI checks normal/enhanced builds with synthetic data,
software Vulkan and Embree.

Details: [GI](../docs/RAC1_PROBE_GI.md),
[sun shadows](../docs/RAC1_SUN_SHADOW.md),
[Ratchet's shadow](../docs/RAC1_HERO_SHADOW.md).
[NVIDIA RTXGI-DDGI](https://github.com/NVIDIAGameWorks/RTXGI-DDGI) informed
irradiance encoding and blending.

## Local region planning

The planner merges boxes only when their union adds no coverage. It then partitions
full-strength coverage into disjoint cores with a one-cell border. Adjacent volumes
share two-cell blend bands; thin grids retain their partial-strength support.
The baker reads integer `lattice_bounds` directly, avoiding floating-point expansion.
Shared grid alignment and individual probe relocation run after merging and partitioning.

Six world-axis orders are compared by duplicated coverage and probe count. A local
refinement favours larger boxes and penalises regions where three or more blend bands
meet. It preserves coverage, fade support and the per-volume budget, but does not
impose a strict two-volume limit. Seed reports include overlap, probe counts and cost.

With probe spheres enabled, yellow wireframes show each local volume containing the
player; cyan shows the sparse grid if none does. Bounds follow the player's render
snapshot independently of camera selection and relocation. Hidden edges are faint.

Optional [Tracy profiling](../../../docs/port/PROFILING.md) covers the native baker's
Embree construction, placement, transport and integration on each worker.

`OPENRAC_PROBE_SPHERES=all` shows every GI volume bound and nearby resident probe positions,
independently of lighting selection. It uses the CPU diagnostic filter; normal lighting is unchanged.

## Environment reflections

Enable `environment_probes.enabled` in a level's `probe-bake.json`. After all GI grids verify,
the baker captures static HDR surroundings using the runtime GI reference, shadowed sun,
visible emission and the skybox. Bake-only emission gains affect transport, not visible surfaces.
Water and authored chrome/glass packets use these captures in enhanced mode.

Captures sit near floor-space centres, normally 3 units above their actual supporting floor.
Low ceilings can lower them to 2.55 units. The shared playable-floor analysis keeps walls,
separate storeys, authored lighting boundaries and above/below swimming-water domains separate.
It connects supported walking and jumping routes; shallow wading water shares dry-floor spaces.
See [placement and adaptive density](../tools/probes/README.md#environment-captures).

Placement and pruning retain coverage at floor, body and head height. Close visible captures
must merge or relocate until at least `minimum_capture_spacing` (6 units) apart. Walls, separate
storeys and water media exempt pairs. Other merges require 85% shared receiver area and similar
GI signatures: Rec.709 luminance within 2:1 plus 0.02, and Oklab distance at most 0.06 at a common
18% luminance. These are placement heuristics, not comparisons of finished cubemaps.
Every accepted replacement preserves every receiver; unresolved coverage or spacing fails baking.

`maximum_probes` defaults to 512 as a safety limit. Captures use 128x128 source faces and 128 GGX
samples. `environment.orpe` version 5 stores a GPU-ready R11G11B10F atlas, oriented influence and
parallax bounds, and a split-sum BRDF lookup. Seven saved GGX levels span 64x64 to 1x1; the source
128x128 level is filtered but omitted from storage. Roughness is clamped to the first retained
level's 1/7 value. Runtime validates scene/settings and every GI payload before upload.

Conservative 16x16 screen tiles select candidates from all 512 captures. Pixels blend two
box-projected captures, fading to local GI outside influence bounds. Opaque depth does not cull
reflection candidates because water/glass can lie on either side. Missing compute selection uses
the full reference loop. Masks persist until the view or capture data changes; metadata uploads
once. Reflection bindings use units 21-23, separate from water deformation's units 18-20.

Captures omit moving objects, transparent effects and recursive reflections. Upright boxes
approximate irregular rooms, and floor inference does not replay gameplay physics. Local GI
provides broad fallback colour and brightness, without sharp reflected objects. Placement,
coverage and atlas details are recorded in `environment-placement-report.json` and
`environment-report.json`.

Enhanced > Diagnostics > **Environment probes** displays captured reflections on mirror spheres
and cyan influence boxes. **Medial axis** shows orange centre paths, dashed orange jump arcs,
pink junctions and white component centres. Hidden edges remain faintly visible. These controls
do not change lighting or selection.

## Reflection material controls

In enhanced mode, every authored chrome/glass packet uses a PBR specular layer: box-projected
GGX environment reflections plus shadowed GGX directional highlights and local point highlights.
The original painted/diffuse layer, packet alpha, distance fade and emission remain intact. Missing
reflection coverage uses local normal-dependent GI, converted from irradiance to a broad reflection
with the material's Fresnel response. Chrome keeps its original effect only when neither GI nor
reflection data covers the receiver. This does not add refraction or convert painted surfaces into
metals.

Enhanced > **Reflection materials** exposes session-only roughness, normal-incidence reflectivity
(F0), strength and reset controls for each profile. Chrome profiles also expose linear reflection
tint and an authored-opacity multiplier. Water uses its profile for reflections and direct highlights.

| Profile | Roughness | Reflectivity (F0) | Strength |
| --- | --- | --- | --- |
| Water | 0.22 | 0.02037 | 1 |
| Chrome | 0.25 | 0.65 | 1 |
| Glass | 0.08 | 0.04 | 1 |
| Stylised | 0.35 | 0.12 | 0.65 |
| Clank | 0.26 | 0.65 | 1 |
| Wrench | 0.18 | 0.75 | 1 |

`metal_glass` selects Glass. `metal_stylised`/`metal_stylized` select Stylised; other chrome uses
Chrome, with Clank/wrench defaults selected by authored object class. **Object overrides** lists
loaded objects with chrome packets and can force any chrome profile for that class, including
its glass packets. Automatic restores the authored selection; clearing overrides resets every class.
Stylised exceptions are explicit, never inferred from texture colour. Controls do not change bakes.


## Colour correction

Enhanced Settings > Post Processing enables AgX by default at 0.25 EV. Exposure spans -4 to +4
stops; Brightness adds `(brightness - 0.5) * 1.6` EV, defaulting to the neutral midpoint. The
settings are `agx_preview`, `agx_exposure` and `agx_brightness`. Original graphics, OpenGL and
loading/menu snapshots bypass the pass.

The shader transforms world colour after FSR/motion blur and before native HUD rendering. It
decodes sRGB, applies exposure and a log-domain sigmoid with 40% hue blending, then encodes sRGB
once, preserving alpha. A separate source image prevents attachment feedback; no CPU readback
is used. Existing RGBA8 buffers have already clipped highlights, so this SDR treatment cannot
recover HDR detail.

AgX matrices are adapted from Google Filament's `ToneMapper.cpp` at
`be858f42ab95cf0ba6e224ac2dc3950bdb172080` (Apache-2.0); see
[the retained licence](../renderer/agx-LICENSE.txt) and the third-party notices.
