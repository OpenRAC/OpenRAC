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

Novalis NTSC [enhanced water](docs/water.md) needs local `water.bin` and `water.mesh` files beside
the manifest. Its separate bake stores banks, rest heights, dense mesh and outlets. Compute adds
Ratchet's ripples and bank reflections; cached lighting preserves the painted blue textures.
