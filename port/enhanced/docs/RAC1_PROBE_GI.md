# RAC1 baked probe GI

See the [lighting guide](../lighting/README.md) for setup, controls and formats.

## Runtime lighting

Probe GI needs an enhanced Vulkan build and a valid level bake; otherwise
original lighting applies. It replaces ambient light on terrain and lit mobys,
including Ratchet, enemies and attachments:

```text
indirect = probe_irradiance / pi
diffuse = albedo * (direct_light + indirect * ambient_visibility)
surface = diffuse + emission + separate_specular_effects
output = existing_fog_and_output_conversion(surface)
```

GTAO mainly affects indirect light. Direct light, emission, metal effects, fog
and alpha remain separate. Camera-facing normals handle double-sided surfaces
but can change lighting when the camera crosses a surface's tangent plane.

## Offline bake

Embree traces terrain, ties, shrubs and selected static mobys with shared texture, opacity,
tint and emission rules. Terrain vertex tint defaults on. Animated liquid emission is averaged;
direct emission sampling is not counted again on path hits.

Escaping rays default to 70% scaled skybox and 30% primary directional colour.
Static lights and emissive surfaces add diffuse bounces; runtime direct light
stays separate. Each volume traces the surrounding scene beyond its bounds.
Moving objects, destruction and doors do not update baked light or visibility.

## Probe placement

Bounce normals come from triangles and face the incoming ray. Inside tests
use ray crossings: at least 75% of 32 direction pairs must have odd counts in
both directions. Inside probes get up to four relocation attempts within 45%
of the smallest grid spacing; unusable probes become invalid.

Open surfaces, sealed single-shell rooms and intersecting shells remain
ambiguous. Correcting triangle winding alone cannot resolve them.

## Probe data and sampling

Format 5, bake revision 4 stores bordered octahedral irradiance/distance-moment maps,
relocation and validity as directly uploadable planes. Older formats or stale scene/settings
hashes are rejected.

Sampling applies normal/view bias and blends eight neighbours by relocation, validity,
position, surface direction and visibility. Variance is nonnegative; sparse probes halve it
beyond the mean blocker without reducing unobstructed fill. A receiver-plane guard limits
leaks, but finite ray sampling can miss thin walls.

Irradiance is stored and texture-filtered as E^(1/5), raised to 2.5, blended as
sqrt(E), then squared back to E. Diffuse shading divides by pi once. CPU checks
use the same arithmetic.

## Sparse and dense volumes

A sparse grid provides base coverage. Dense regions follow authored lighting volumes
and whitelisted gameplay mobys, extending to nearby walls and ceilings. Large
regions split with overlap; planning reports record coverage limits. Halving
spacing on all axes costs about eight times as many probes.

Vulkan chooses up to three regions per fragment using tiles/depth; the CPU fallback uploads
three camera-prioritised regions with fades. Each region tests visibility before blending.
Missing support can fall back to sparse GI; blocked or weak support stays/fades dark.
Boundary weights and an outer apron keep intended interior coverage full strength, fading to
sparse lighting outside it.

## Limits and troubleshooting

Green coverage shows usable support, not accurate lighting or occlusion. Sparse fallback
cannot replace blocked light. More probes alone cannot fix invalid placement, room/solid
ambiguity or the wrong visibility range.

Dense regions need to cover receiver surfaces, walls, ceilings and motion paths.
Enemy patrols and dynamic geometry can leave planned coverage. Check coverage
and GI-only views before adjusting brightness. Distance moments use a local
range so sky misses do not overwhelm nearby wall hits. Point lights are not
shadowed, and static baked emission remains after destruction.

A valid bake confirms scene/settings compatibility, not image quality. Rebake
sparse and dense volumes together after changing inputs or transport settings.

## Verification

CPU/GPU fixtures cover formats, placement, transport, sampling, region selection,
streaming and shadow/GTAO integration. Gameplay checks still need interiors,
doorways, thin walls, terrain LODs, skinning, attachments and emission. Compare
original, GI-only and combined views; record time, memory and coverage.

Exports and bakes stay local. PAL and NTSC need separate data and identities.

## References

- [Original lighting](../../../docs/engine/systems/LIGHTING.md) and
  [moby rendering](../../../docs/engine/systems/MOBY_RENDERING.md).
- [Terrain vertex shader](../../renderer/shaders/world/tfrag.vert),
  [fragment shader](../../renderer/shaders/world/tfrag.frag) and
  [point lights](../../renderer/shaders/world/world_lights.glsl).
- [Embree overview](https://github.com/RenderKit/embree/blob/master/doc/src/overview.md).
- NVIDIA DDGI [algorithms](https://github.com/NVIDIAGameWorks/RTXGI-DDGI/blob/main/docs/Algorithms.md)
  and [sampling](https://github.com/NVIDIAGameWorks/RTXGI-DDGI/blob/main/rtxgi-sdk/shaders/ddgi/Irradiance.hlsl).


## Preparation failures and logs

Lighting preparation writes `probe-bake.log` in the version's probe-cache root.
It records the tool/input locations, preparation stages, native error output,
Python tracebacks and suggested recovery steps. Per-level `batch.log` and
`interior-inference.log` retain the native tools' full output in the level's
partial folder. Logs are local files and may contain local paths.

A failed launcher task shows the reason in its persistent error notice, with a
plain-language explanation and a suggested fix. **Error details** expands the
native reason (paths are shortened); **View details** opens Activity, where the
whole task output can be copied. On desktop, **Open log** opens the associated
text log. The launcher only opens `.log` files inside that version's lighting
cache, with canonical-path checks against traversal and external symlinks.
Android uses Activity for the output rather than a desktop log viewer.

Failures also emit a versioned `openrac-bake-error: {JSON}` line, retained in
`bake-all-result.json` for each failed level. The error categories and recovery
text live in [diagnostics.py](../tools/probes/diagnostics.py). Older tools without
that record still get an explicit fallback notice and Activity access.

A failed level does not discard a previously verified bake. Partial verified
grids and native logs remain available for retry. Installing a replacement uses
a recoverable `.level_NN.previous` backup instead of deleting the previous bake
first. A failed rollback reports the backup location. A cache lock prevents two
CLI preparations from writing the same cache at once; the OS releases it when
the process stops. Empty menu/flight levels are successful no-bake results,
not errors. Incomplete or unverified cached results are rebuilt.
