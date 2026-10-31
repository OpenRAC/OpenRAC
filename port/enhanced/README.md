# Enhanced rendering

Probe GI, modern shadows, GTAO, stylised water, FSR temporal upscaling and edge-aware texture upscaling run through
Vulkan when enhanced rendering is enabled. With it off, or on OpenGL, the game
uses original lighting. Embree is needed only for offline baking.

Start with the [lighting guide](lighting/README.md) for builds, bakes and controls.

| Directory | Contents |
| --- | --- |
| `lighting/` | Probe data, sampling, emission and transport |
| `renderer/` | GTAO, water, wetness, texture upscaling and shaders |
| `fsr3/` | Temporal world reconstruction, motion and reactive masks |
| `viewer/` | Probe lighting, probe selection and early lighting of a level, library `openrac_enhanced_viewer`, namespace `openrac::lighting`; `scene_lighting.h` registers them with the scene's lighting hook ([scene/README.md](../scene/README.md)) |
| `game/` | Authored brightness tracking |
| `tools/probes/` | Offline baker and bake tools |
| `tests/`, `docs/` | Tests and feature documentation |

## Integration

GTAO (`renderer/gtao.h`) implements `port/renderer/ambient_occlusion.h`, registered in
`game/common/frontend/world.cpp`. It uses `port/renderer/gtao_settings.h` and builds with its
shaders in `openrac_renderer`.

`viewer/scene_lighting.h` implements `port/scene/level_lighting.h`; register a `LevelScene`
through `lighting::use_scene_lighting`. Loader and baker share explicit emission definitions
in exported `probe-emission.json`.

Shared integration and core Vulkan optimisations stay in `port/renderer/` and
`port/scene/`. Editor bake inputs live in `editor/enhanced/`.

Texture upscaling and its 64 MiB cache live in `renderer/texture_upscale.*`.
Edge-aware mode combines 2x colour reconstruction with conservative directional
debanding during cache creation.
The normal texture pool accepts an optional `UpscaledTextureColours` provider; it
keeps original alpha/mip sampling and notifies the provider when source textures
are retired. Only the enhanced texture library depends on the Vulkan copy helper.

## Ratchet fur

Enhanced graphics > Diagnostics > Ratchet fur view shows the authored coverage mask or
UV growth directions with arrows. Detail normals shows the shaded surface orientation.
Enhanced graphics > Materials & water > Ratchet fur controls directional detail, strength,
strand contrast and fade distance. Its Lighting controls adjust highlight strength, softness,
directionality, scattering and Environment response separately. Controls apply live and are session-only.

Short strands are generated deterministically along the groom into a mipmapped 512-square
RGBA8 data texture. RG stores signed UV slopes around byte 128; B stores low-contrast strand
variation around the same neutral value. World-position/UV derivatives carry those slopes through
skinning, mirrored UVs and scale. Minification, distance and grazing-angle fades suppress unresolved
detail. Wetness reduces the surface detail. FP16 handles bounded amplitudes and grain; derivative
frames and normal reconstruction remain FP32.

Fine normals affect direct material lighting and the existing wet coat. Shadow comparisons,
probe bias, cached GI and prepass normals retain the smooth surface. Subtle colour grain keeps
some texture under flat indirect lighting. No geometry or rendering pass is added. This runs on
enhanced Vulkan; original rendering and OpenGL retain their existing material path.

The fur lighting adds a broad anisotropic dielectric highlight along the groom and a restrained,
warm forward/wrap scattering response. It uses the existing directional sets and eight point lights,
with the same light blending, sunlight override and point falloff. Added directional lighting uses
the full shadow visibility without the legacy diffuse floor. Local prefiltered reflection captures
supply indirect highlights through a groom-dependent lookup normal; the BRDF retains the original
shading cosine. Cached GI supplies warm scattering across the coat and a dielectric reflection
fallback where captures are absent. This remains light-driven in shade and interiors, without an
unshadowed sun contribution or a constant ambient glow. It adds no GI or shadow lookup, but samples
the existing reflection atlas on fur. Wetness reduces the dry-fur response.
The implementation uses the [anisotropic GGX equations](https://google.github.io/filament/Filament.html#materialsystem/anisotropicmodel),
with FP32 frames, distributions, visibility denominators and HDR accumulation; bounded Fresnel,
scattering and tint calculations use FP16 where supported. Static world shader variants exclude
the fur declarations and functions entirely. This is a short-coat material approximation, without
strand geometry or transport through the body.

[`lighting/profiles/rac1/fur.json`](lighting/profiles/rac1/fur.json) labels fur triangles in the
verified body material, grouped by UV island. Groom directions use +U right and +V down. Small
UV polygons refine the groom and exclude the nose and claws in original atlas pixels. The profile
covers fur on the head, ears, arms, torso, feet and tail, excluding eyes, nose, mouth, teeth,
claws and equipment. Directions are an initial atlas-space groom; shared and mirrored UVs still
can be refined independently of the strand texture.

At load, `renderer/fur_map.cpp` generates an RGBA8 map: R is coverage, GB is a signed unit UV
direction multiplied by coverage and encoded as `0.5 + 0.5 * direction`, A is 255. No game pixels
are stored in the profile. Game identity, body class, material, atlas dimensions, vertex/index
counts and a UV signature must match. Overrides and unknown layouts opt out. A static vertex
selection excludes non-fur islands; two texels of map padding prevent bare UV seams without
spilling onto equipment. Shared vertices between fur and excluded islands are refused. The range
and live-palette checks exclude attachments, other materials and UI copies. Diagnostics are
session-only and default off.

## Normals

The shared prepass writes smooth view-space normals for early GI and GTAO. Depth supplies
positions/horizons, never reconstructed normals. Colour passes read cached GI/shadows and use
smooth normals for direct light, with consistent skinning, scaling and camera-facing orientation.
Invalid normals use a neutral fallback. Raster derivatives only orient normals to visible
triangles; grazing-angle constraints prevent flips beneath surfaces and GTAO self-occlusion.
No extra normal texture is needed.

The prepass uses RGBA8 normals (four bytes per render pixel), with matching depth and MSAA
coverage. It preserves the existing sky/background and reuses depth for world colour.
Original rendering and OpenGL retain the minimal depth-only path. Probe formats and bake transport
are unchanged.

## Gameplay rim lighting

Gameplay rim lighting uses cached GI colour and smooth normals, fading toward front-facing
surfaces within a brightness cap. It needs valid probe coverage, adds no samples/targets, and
leaves emission, diagnostics, UI and blended effects unchanged. Ratchet, attachments, pickups,
crates, enemies, NPCs and tool targets use the class list in `port/game/rac1/gameplay_lighting.h`.
Enhanced Settings toggles it; Advanced saves live strength/falloff controls. Higher falloff
narrows the edge. Reflections stay separate from diffuse texture/vertex tint, including chrome
overlays such as Clank's, keeping dark materials readable.
Optional Rim shadow lift in Advanced brightens rims in dim surroundings. It applies a shared
linear RGB gain `clamp((max(luminance / 0.18, 0.000001))^(exponent - 1), 1, maximum_gain)` before
the existing rim brightness limit. This preserves RGB ratios and leaves zero GI black.
The default exponent is 0.09 and maximum gain is 7.10; lift is on by default. Exponent 1 or maximum
gain 1 reproduces the unlifted rim. Lighting brighter than 18% linear grey is unchanged.

Level exports preserve authored normals for terrain, ties, shrubs and mobys. Older exports
flattened terrain and static meshes, so re-extract those levels to restore smooth shading.
Changed mesh files require a matching probe bake under the existing source-fingerprint checks.

## Water

Novalis (NTSC-U and PAL) [enhanced water](docs/water.md) prepares `water.bin` and `water.mesh`
beside the manifest during lighting preparation. The bake stores banks, rest heights,
dense mesh and outlets. Compute adds Ratchet's ripples and bank reflections; cached
lighting preserves the painted blue textures.
