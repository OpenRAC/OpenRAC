# Create and add a ship destination

A destination pack **adds a world** to RAC1's ship computer. It does not replace
its template planet. Enable one pack to add the twentieth destination; enable
another to add the twenty-first. Each has its own name, arrival position and
resource folder, even when several use the same template program.

The creator edits a Godot project and exports a pack. They do not edit the game's
C, choose unused memory addresses, or manually assign destination numbers.
This workflow currently targets the **native RAC1 PAL port**.

## 1. Prepare a project of your own

Use your own matching disc and the complete tools described in
[the level editor guide](LEVEL_MODDING.md). In **Mods → Level editor**, extract
a level and make a separate copy of its Godot project before editing it.
Choose a template with the objects and working ship your world needs. **Aridia
(level 2)** is verified for the ship roundtrip; Novalis's first-arrival story
sequence hides its ship until its quest progression is satisfied.

The template number means **which program and resources the map uses**. For
example, a new destination can use template 2 while original Aridia remains
accessible. Do not change `--template-level` to 19 or 20 to request a new slot.

## 2. Build the world in Godot

- Move, add or remove objects using the template's available classes.
- Author your terrain with the [model/terrain workflow](MODDING.md), placing the
  result at `levels/level_NN/terrain.glb`. Imported GLB children are not ordinary
  editable Godot nodes: edit the mesh in your modeling tool, then import it again.
- Edit or generate collision with the Collision dock. Visible geometry alone
  does not make a floor. Keep terrain, collision, paths and cameras aligned.
- Place **Ratchet** where the player should arrive and set his heading. The
  exporter reads this placement automatically; keep exactly one Ratchet start.
- Keep **Game/LevelSettings/Ship**, its landing space, required class resources,
  flight path and camera volumes. The ship is the return route. Export checks
  that a ship placement exists; a playtest must verify its behavior and access.
- Remove unwanted story triggers. A template supplies game logic as well as
  assets; importing another planet's enemy model does not import its behavior.

The game uses Z up. Surface type **31 / 0x1f** is ordinary ground; type **0** is
water. The editor's surface selector documents these meanings.

### Import static scenery from another level or your own model

Use a static `.glb` from an extracted donor level or make your own in Blender or
another modeling tool and import it into the Godot project. Static imports add
geometry to the map; they do not add a moby class or enemy behavior. The model
must use triangle meshes, PNG textures, and supported glTF materials. Texture
files must be inside the donor level or creator project; unsupported animation,
skinning and behavior are refused.

For a model already in the Godot project, open the extracted level scene and
choose **Add static GLB to map** in the Collision dock. The editor instances it
under **Game/StaticAssets**, where you can position, rotate and scale it. This
is undoable and keeps the model linked to its project asset. For a donor model
outside the project, use the import command below to copy it and its textures
into the project first.

```sh
python editor/static_assets.py --source DONOR_LEVEL/ties/tie_123.glb --project YOUR_PROJECT --level 2 --name MyDecor --at X Y Z --collision mesh
python editor/static_assets.py --source YOUR_PROJECT/models/my-building.glb --project YOUR_PROJECT --level 2 --name MyBuilding --at X Y Z --collision mesh
```

The tool copies the mesh and its referenced PNG textures, adds an instance under
**Game/StaticAssets**, and backs up the level scene in `.openrac-backups/`.
Texture names are based on content, so two levels' `tie_0001.png` files cannot
silently overwrite each other. Source files are copied into the project; the
donor and original model files are never changed.

Reopen the scene in Godot. Move, rotate, scale or duplicate the imported instance;
groups beneath Game/StaticAssets can be transformed too. Keep the instances under
that group. Imported geometry and optional collision use the same transforms,
including mirrored/non-uniform scale. Changing, duplicating or removing an instance
requires **Pack** again before export; stale exports are refused.

`--collision mesh` makes the visible triangles solid with the ordinary ground
surface. Omit it for decorative objects such as foliage. You can also change the
instance's `rc1_static_collision` metadata in Godot. This generates collision from
the model, not a reconstruction of the donor level's separate collision mesh:
transparent foliage and detailed models may need simpler authored collision.

Packing combines the static instances into the map's terrain and copies their
texture dependencies. No new runtime object class or donor-level code is required.
Vertex colours are preserved; models without them get neutral vertex colours,
so donor per-instance lighting is not transplanted. The imported result is
static: skeletons, animations and behaviors are refused. Generated meshes,
textures and packed collision remain private like the rest of the map.

## 3. Pack and export an added destination

Save the project, then use **Pack** or the command below. Substitute your own
paths; keep outputs in a private ignored folder such as `build/`.

```sh
python editor/extract.py pack YOUR_ISO YOUR_PROJECT build/my-world-packed
python editor/map_mod.py --destination --project YOUR_PROJECT --packed build/my-world-packed --template-level 2 --name "My World" --map-id "yourname.my-world" --out build/my-world-pack
```

`--name` is the label in the ship computer. `--map-id` is the creator-qualified
identity used for updates: keep it unchanged when revising the same world.
`--spawn X Y Z` and `--heading RADIANS` optionally override Ratchet's placement.
Use `--keep-movies` or `--keep-cutscenes` only when those template sequences belong
in your world. Existing export folders are refused; choose a new output folder
for each export. The source project, packed inputs and disc are not overwritten.

The export writes the manifest and required level files automatically. No sample
map, fixed map name or hardcoded author geometry is part of the framework.

## 4. Install, play and return

1. Use **Mods → Add mod → Folder** or add a zip containing the exported folder.
2. Enable the pack. It is labelled **Added ship destination**. Several destination
   packs can be enabled together; ordinary texture/model packs can coexist.
3. Turn on **Mods** under Play and launch. Preparation builds a registry and an
   isolated folder for each destination. The launcher requests destination support
   automatically; an older executable refuses the unknown option.
4. Start a new game for this test session. It starts on the first destination's
   template unless a starting level was explicitly selected. Board the ship with
   Triangle, select your world and confirm with X.
5. Test the arrival, walking/jumping, collision and objects. Board its ship and
   return to the original planet. Also test travel between added worlds.

Disabling or removing a destination removes its ship entry on the next preparation;
its original template planet stays. Authoring output supplied through `--packed`
is not applied to the campaign when destination packs are enabled: only the
exported destination receives those files. To keep separate campaign edits,
install them as an ordinary data pack.

For a repeatable shell test (the same preparation the launcher performs):

```sh
python editor/overrides.py add build/my-world-pack --into build/my-world-installed
python editor/overrides.py stack --into build/my-world-installed --out build/my-world-session --levels YOUR_PAL_RENDERER_LEVELS
port/build/release/openrac-rac1-pal --data YOUR_PRIVATE_DATA --levels YOUR_PAL_RENDERER_LEVELS --mods build/my-world-session --destinations --window
```

The renderer export must include every template used. Names and positions in this
guide are placeholders; the export/install/prepare steps work with the creator's
own project. Test PAL resources against PAL, never another disc edition's binaries.

## 5. Iterate without duplicating the destination

Edit the same project, pack again, then export to a fresh folder with the **same
`--map-id`**. Add that export again. Installation replaces the previous version
of that identity while preserving its installed ID, enable state and list order.
A different identity creates an additional destination. Prepare and relaunch after
an update; do not replace files in a running session.

## Formats and compatibility

| Pack | Meaning | Prepared output |
| --- | --- | --- |
| Format 1 | Ordinary replacements for the campaign | Root `patches.tsv` and level files |
| Format 3 | One standalone map, launched directly | `map.json` |
| Format 4 | Added ship destination | `destinations.json` plus `destinations/<installed-id>/` |

Format 4 deliberately makes older installers refuse the pack. Its `map.level`
field is the **template**, and `map.spawn`/`map.heading` are the arrival. The stable
creator identity is in `export`; the installed folder ID identifies it in the
registry. Destination numbers start after the original program count and are
assigned per prepared registry, so do not use these numbers as persistent save IDs.
The current menu text range accommodates up to 256 added destinations.

Standalone format-3 maps remain supported: omit `--destination` when exporting.
Only one standalone map can be enabled, and it cannot be mixed with destination
packs. The old experimental `--ship-map` switch remains compatible with a single
standalone map; creators should use format 4 and normal launcher preparation.

## Current limits

- Destination sessions are **card-free**, including while visiting original
  planets. Separate saved progression for added worlds is not implemented.
- There is no Godot project byte budget. When edited gameplay or level-data
  streams exceed their original disc allocation, the packer stores the larger
  stream in the mod's virtual sector container. The disc image and no-mod path
  remain unchanged. The expanded runtime arena is currently 256 MiB, so very
  large streams can still exceed available guest memory.
- RAC1's object tables use signed 16-bit runtime indices. A level's source
  moby table is therefore limited to 32,768 records including the objects
  already present. The 2,048 native killed-state IDs are a separate limit:
  added objects use remaining IDs, and overflow objects are packed with ID
  `-1` and spawn flags `0`, so the game can use them but does not remember their
  killed state after a level reload. The packer's `changes.txt` reports this.
- Collision has separate runtime bounds: expanded mod collision covers the
  positive 131,072-unit world, and each cell and the total 256 MiB arena must
  hold its data. **Check the limits** in the Godot Collision dock estimates
  affected cells and world bounds before packing.
- Templates supply their own code, object classes and unedited resources. The
  framework does not automatically import behavior from another level or game.
- Galaxy previews still use template artwork/location; the added world has its
  own selectable name and resource mount. This does not add a retail ISO overlay.
- NTSC-U supports existing standalone maps, but ship destinations are PAL only.
- The [sourcing policy](../docs/policy/SOURCING.md) applies: do not commit or upload
  disc-derived terrain, models, textures, packed game files or ISOs with this code.
  Keep generated outputs private; the repository contains only tools, tests and
  documentation.
