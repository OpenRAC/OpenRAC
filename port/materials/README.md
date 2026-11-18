# Materials

CPU-only material catalogs: strict JSON, source layering, selectors, provenance and transport projection.

## Interface
`catalog.h` declares property metadata, catalog parsing, merging, resolution, stable serialization and atomic saves.

## Depends on
`openrac_common` and `openrac_assets_disc` for streaming SHA-256; no renderer, game or GPU dependencies.

## State and threads
Callers own catalogs and resolved records. Publish immutable snapshots at the render handoff.

## Errors
Fallible APIs return `Result`; malformed or ambiguous authoring cannot partially replace live bindings.

## Configuration
Version 1 `material-overrides.json` contains named `presets` and an `overrides` array. Sources merge
from shared game through disc, level and ordered mods. Preview applies edits at their save-file priority.
Rules use stable ids and explicit layer, class, material-slot, packet-pass or content-hash selectors.
Fields merge independently. More specific matching rules win within a source layer; equally ranked
conflicting assignments are errors. Missing fields inherit. A disabled rule is an explicit tombstone.

Named definitions remain in `presets` for compatibility. A rule with `"material": "paint"` assigns
one complete shared definition to its matching mesh slots, replacing earlier finish properties.
The older `"preset"` spelling continues to contribute sparse inherited properties. A rule cannot
use both references. Textures, coverage and geometry remain properties of the original mesh slots.
Equivalent resolved render records are stored once and referenced by multiple bindings.

## Tests
`material_catalog` checks strict syntax/types/ranges, selector isolation, ambiguity, sparse source
priority, provenance, save/read stability and separation of runtime controls from transport inputs.

## Further reading
The enhanced module documents material consumers and its authoring UI.

```json
{
  "version": 1,
  "presets": {"leaf": {"shading": "foliage", "foliage": {"transmission": 0.35}}},
  "overrides": [
    {"id": "plant-body", "match": {"layer": "shrub", "class_id": 7}, "material": "leaf"},
    {"id": "fern-body", "match": {"layer": "shrub", "class_id": 8}, "material": "leaf"}
  ]
}
```

The scene owns immutable original materials and resolved model/slot bindings. Enhanced colour draws
use cached surface records and typed optical controls. Textures, cutout masks and simulation state
remain with their mesh slots. Glass Lit selects a transparent pass; Original restores retail coverage.
Baking reads the same resolved surface records.
Runtime-only fields are excluded from bake fingerprints. Effective static emission or diffuse changes
require a rebake. Materials can set `emission.bake_mode` to `"texture"` to sample their emission
texture and vertex colours, or to `"mesh"` to use a constant linear HDR `emission.bake_mesh_rgb`
on the selected model and material slot. Texture emission uses the cutout mask by default. Mesh
emission spreads across the full triangle by default. Set `emission.bake_use_cutout` to explicitly
choose either behavior for emission. Visibility and runtime coverage still use the original cutouts.
Older materials with no explicit mode retain their previous `emission.average_for_bake` behavior.

Run `openrac-enhanced-baker --migrate-mesh-emission LEVEL_DIR MATERIAL_OVERRIDES.json` on a prepared
export to write a mesh-emission override for each emitting static model and material slot. It
integrates the old linear RGB over the original emitting area and divides by the full mesh area
or, when explicitly retaining cutouts, the covered area. Runtime emission stays in the existing
catalog. Use the level's root `material-overrides.json` to apply the migrated settings to that export.

To reproduce the area-weighted `bake_mesh_rgb` values in a separate JSON catalog, run:

```sh
openrac-enhanced-baker --calculate-mesh-emission LEVEL_DIR > mesh-emission.generated.json
```

The calculator ignores stored mesh RGB values. Migrated exports must still have the original
geometry, textures and source emission settings.

Tracked profiles live in `enhanced/lighting/profiles/<game[-version]>/[level_NN/]material-overrides.json`.
Export/bake preparation writes separate `materials/shared`, `materials/version` and `materials/level`
catalogs inside each level. User edits save to the highest-priority mod/override/level root catalog;
export never overwrites that file. A parent-folder catalog supplies user defaults across levels.
To ship Studio edits, merge the saved root catalog into the matching tracked level profile,
preserving its existing rules and shared definitions. New exports and bake preparation then publish
those settings into `materials/level`; local Studio saves continue to override the tracked defaults.
Legacy emission sidecars remain readable; unchanged generated defaults are not imported as local edits.
Fur coverage, water boundaries/simulation, bake geometry and authored lights keep their own formats.

Developer settings > Material Studio edits reusable definitions and their mesh-slot assignments.
The click picker's Edit material selects the exact binding. Saved slot selectors
use the exported `model` name and zero-based `material_slot` (`-1` for implicit material). Hover a control to
see its source; edits preview live, Save writes a sparse overlay atomically, and Discard restores the
last loaded/saved values. Saving refuses external file changes. Importing new image references and
changing verified fur coverage require editing the JSON and reloading the level.

Material Studio separates the **Materials** library from the **Surfaces** browser. Each surface shows
its shared material name. Untouched surfaces and legacy overrides are imported into shared definitions;
equivalent appearances share one definition, while textures and geometry remain with their surfaces.
Tint, brightness and emission controls show inherited defaults until edited; opening Studio does not save them.
The surface browser shows every editable surface; **Local changes only** filters assignments and
imported local edits. Pick a surface, then
choose a shared material, **Create shared material**, or **Choose a finish**. Library finishes create
reusable definitions instead of copying values into each surface. Editing a linked material updates
every user; **Make independent copy** creates a separate material for the selected surfaces.
Every definition has a **Material type** selector: **Lit**, **Glass Lit**, **Lit Fur Surface**,
**Lit Wet Fur Surface**, **Lit Foliage**, or **Original**. Types select the active controls and shader
response. Water is a Lit option for the level's water binding. Fur and wet fur retain verified coverage
and the existing wetness simulation. Glass Lit moves assigned geometry into the transparent pass;
switching between opaque and glass materials invalidates static GI, while reflection and transmission
tuning stays runtime-only. The original texture cutout mask remains in effect for Glass Lit.
Original participates in preparation fingerprints because it can suppress inherited tint and emission edits.
Original retains the original appearance without enhanced material lighting. On chrome surfaces it
restores the retail reflective geometry and texture; Lit uses the colour/replacement geometry instead.
Each surface controls its own chrome coverage, including when several surfaces share a retail packet.
Legacy chrome and stylised families migrate to Lit metallic properties; the glossy-plastic library entry is removed.
**Restore level material** clears saved and unsaved exact-slot
overrides and assignments for the selection, retaining inherited defaults, shared definitions and
other users. The reset previews immediately and persists with **Save changes**. Field **Revert** only
discards unsaved field edits, restoring a new material's initial definition before its first Save.
**Discard unsaved changes** restores the saved definitions and assignments
together. **Details > Material sources** shows the underlying rules and files for the selected surface.
Shared optical controls live in the Vulkan material table, preserving compatible batches.

The serialized `type` ids are `lit`, `glass-lit`, `lit-fur`, `lit-wet-fur`, `foliage`, and `original`.
Generated definitions use stable content-derived ids, so shared edits persist across reloads without
writing one assignment per untouched mesh. The source catalog remains separate from the expanded
editor definitions; legacy sparse rules are import inputs and no longer form a second editing workflow.

Metal finishes retain their measured F0 magnitude in `surface.reflectivity`; the base artwork
modulates the metallic colour and `reflection.tint` supplies the finish's hue. Switching to an
opaque or foliage library finish resets `surface.metallic` to zero. Foliage also supports metallic,
reflection strength and reflection tint for its direct highlights and broad indirect reflection.
Increasing metallic reduces both reflected and transmitted diffuse light while retaining the
leaf shader's partial wax coverage.
