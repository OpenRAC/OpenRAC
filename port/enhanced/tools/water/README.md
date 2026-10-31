# Water baking tools

`openrac-enhanced-baker` decodes retained source tables, prepares water fields and rest meshes,
then bakes GI and reflections. The launcher flow uses no Python process. Python implementations
remain checkout-only references for regression comparisons.

## Interface

- `water_source.h`: `decode_sources(level)` reads the level profile and retained source image.
- `water_bake.h`: `bake(request, output)` writes matching `water.bin` and `water.mesh` assets.
- `--decode-water LEVEL` prints decoded surfaces for inspection without numerical baking.
- `--bake-water INPUT.json OUTPUT.bin` runs the numerical stage directly.

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
`flowing` and `flow`. Optional `second_height_offset` averages two endpoints. Optional
`height_offset` adjusts rest height and rounds to source float32 precision. Optional `scale` and
`translation` transform XYZ afterward. Record and triangle order remain stable.

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

[Probe pipeline](../probes/README.md) and [Tracy profiling](../../../../docs/port/PROFILING.md).
