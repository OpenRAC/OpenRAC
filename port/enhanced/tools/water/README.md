# Water baking tools

`openrac-enhanced-baker` decodes retained source tables, prepares water fields and rest meshes,
then bakes GI and reflections. The launcher flow uses no Python process. Python implementations
remain checkout-only references for regression comparisons.

## Interface

- `water_source.h`: `decode_sources(level)` reads the level profile and retained source image.
- `water_bake.h`: `bake(request, output)` writes matching `water.bin` and `water.mesh` assets.
- `--decode-water LEVEL` prints decoded surfaces for inspection without numerical baking.
- `--bake-water INPUT.json OUTPUT.bin` runs the numerical stage directly.
- `--bake-pokitaru-coast LEVEL_DIR X Y SIZE CELL` emits the experimental NTSC-U coastal
  patch as JSON. The patch uses the ocean's authored mean height and exported collision.
  Bank flags and surveyed depths are row-major, with null depths for unsurveyed or dry cells.
  The waterline raster conservatively marks collision intersections with the mean ocean plane.
  Closed contours are filled so the wave solver treats structures as solid obstacles.
- `--bake-coastal-wave PATCH.json PERIOD HEADING OFFSHORE_DEPTH` emits one linear
  wave response and its uniform-depth reference. Period is in seconds and heading is in
  degrees counterclockwise from world +X. The response assumes one world unit per metre,
  a fixed mean waterline and a patch of 8 to 96 cells per side. Unknown wet depths use the
  offshore depth. Banks reflect and the outer fringe absorbs scattered waves.
  Shallow cells include an approximate breaking loss. The exported correction fades the
  coastal-minus-reference response across the outer fringe and is zero on banks.
  All fields store row-major real/imaginary pairs. Evaluate height as
  `real(H * exp(-i * 2*pi*t/period))`. The incident phase is zero at the patch origin.
- `--bake-ocean-spectrum PATCH.json OFFSHORE_DEPTH` emits the Pokitaru spectrum.
  Version 3 bakes a separate coastal correction for the 3.2 s and 2.65 s swells.
  At runtime, two seeded 64 × 64 inverse FFTs run in compute. The 96-unit tile supplies
  broad swell height, slope and horizontal displacement. The 17-unit tile adds finer
  normal detail. Unresolved slope variance broadens reflections with distance.
  Wave compression and shallow contact depth add sparse foam, while the shallow-water
  simulation supplies local disturbances.

### Ocean profile

Put the editable defaults in `ocean-profile.json` beside the version 3 `ocean.wave` bake.
Earlier version 3 assets can still carry the profile in their embedded `ocean` object.
Distances are in world units, and wind heading is in degrees counterclockwise from +X.

```json
{
  "horizon": 4096,
  "near_radius": 32,
  "wind_heading": 163.74,
  "choppiness": 0.85,
  "chop": {"tile": 96, "rms": 0.085},
  "fine": {"tile": 17, "rms": 0.006},
  "material": {
    "colour": [0.025, 0.19, 0.30],
    "absorption": [0.85, 0.24, 0.12],
    "roughness": 0.22,
    "reflectivity": 0.02037,
    "reflection_strength": 1
  },
  "dry_rects": []
}
```

`dry_rects` accepts up to eight visual exclusion rectangles as
`[min_x, min_y, max_x, max_y]`. The game owns swimming and tide height.
A level without surveyed coastal data may omit `patch` and `modes` and provide
`mean_height` in the profile and the top-level `offshore_depth` in `ocean.wave`. An adapter must identify its
authored ocean draw packets. Pokitaru is the first adapter.

The optional material sets the ocean's linear RGB body colour, absorption per world unit,
roughness, normal-incidence reflectivity, and reflection strength. Omitted fields use the
values shown above. Edit the profile and reload the level to change its defaults.
**Enhanced Graphics → Materials & water → Ocean** previews live overrides for the loaded level.
**Reset ocean material to level profile** restores the file values.
New bakes write coastal coefficients at round-trip float precision and use null for
zero pairs, preserving the renderer's float values while reducing the bake size.

The exporter copies the [Pokitaru ocean profile](../../water/profiles/rac1-ntsc/level_11/ocean-profile.json)
into fresh NTSC-U exports. The launcher bake uses its `coast` origin and offshore depth to
prepare `ocean.wave` before lighting. Changes to material tuning reuse both bakes.
Re-export an older level to install the profile. Enhanced Vulkan renders the ocean out to
the horizon at the game's tide height. Toggle **Baked coastal response** under **Ocean** to
compare the same spectrum with and without the baked response.

## Configuration

The existing per-level `probe-bake.json` contains `water_sources`, copied automatically by level
export. Novalis supplies NTSC-U and PAL profiles. No source addresses are hardcoded in the decoder.
Missing profiles on exports declaring water require a fresh export.

| Field | Meaning |
|---|---|
| `version` | Source schema version, currently 1 |
| `format` | `sectioned_le32`: four-word little-endian section headers and their payloads, optionally WAD-compressed |
| `image_bytes` | Bounded source address space, at most 64 MiB |
| `pool_spacing` | Two positive rest-mesh subdivision dimensions |
| `tables` | Ordered `masked_tiles` or `triangle_strips` operations |

A `masked_tiles` table describes origin `address`, `count`, `record_stride`, authored
`mask_address`, `mask_stride`, mask `columns` and `rows`, first-tile XY `offset`, positive XY
`step`, and runtime `source` ID. Bits expand into rectangles. Shared edges join rectangles at the
same elevation; disconnected ponds remain separate bodies.

A `triangle_strips` table describes `address`, `count`, `record_stride`, embedded `pointer_offset`
and `count_offset`, `vertex_stride`, three float32 `position_offsets`, runtime `source` ID,
`flowing` and `flow`. A single strip with a direct vertex array uses `vertices_address` in place
of `pointer_offset`. Optional `second_height_offset` averages two endpoints. Optional
`height_offset` adjusts rest height and rounds to source float32 precision. Optional `scale` and
`translation` transform XYZ afterward. Translated positions round toward zero to match the
game's float32 world vertices. Record and triangle order remain stable.

This is a closed set of table formats, not a scripting language. Unknown fields and operations,
unloaded pointers, invalid counts and nonfinite coordinates fail before output publication.
`water-boundaries.json` continues to configure explicit absorbing outlets.

## State and threads

Each controller owns its bounded source image, decoded collision, rasters and output buffers.
Water stages use its thread without another worker pool. `Decode water source tables` and the
numerical stages have Tracy zones. Publication stages both assets privately and writes the cache
stamp after installation. Their shared fingerprint detects interrupted pair updates.

## Cache and errors

Water cache version 3 checks the source image, bake profile, collision, boundaries and both output
hashes. Earlier stamps require regeneration. Public native functions return `Result`; malformed
inputs and persistent write failures leave the previous completed bake available for retry.
No new dependencies or environment settings are needed.

## Tests

The fast `enhanced_water_sources` CI test checks synthetic source parity, different layouts and
transforms, malformed profiles and invalid source images. It compares decoded surfaces and both
finished assets against the reference, substituting only the new fingerprint. `enhanced_baker_pipeline`
checks automatic preparation, cache reuse and old-stamp rejection. `enhanced_baker_cli` checks
water numerics and overwrite rejection.

## Further reading

[Probe baker](../baker/README.md) and [Tracy profiling](../../../../docs/port/PROFILING.md).
