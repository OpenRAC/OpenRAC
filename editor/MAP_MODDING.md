# Map mods

A map mod is a named world installed in **Mods**, alongside texture packs and
gameplay mods. The original disc and the extracted campaign are kept. Turning
the map off brings the campaign back. You can install several maps; enable one
at a time, then play with **Mods** on.

This workflow is for Ratchet & Clank PAL and NTSC-U. A map uses an existing
level's program and available object classes as its template. Its identity is
the mod's name and installed id, not a new entry in the disc's level table.
Each disc needs its own export of the gameplay and collision files.

## Create and export

1. Open a level in Godot through **Mods → Level editor**. Work in a copy of
   its project. The existing [level guide](LEVEL_MODDING.md) covers objects,
   their properties, and collision. Supply your terrain as `terrain.glb` in
   the project's `levels/level_NN/` folder. Its coordinates and the collision
   must agree: both use the game's units, with Z up.
2. Pack the edited project with **Pack** or `editor/extract.py pack`. This
   produces the edited gameplay/collision files and `patches.tsv`. The terrain
   is drawn directly by the native port; it does not need an ISO repacker.
3. Export a named map from that project and the packer's output:

   ```sh
   python editor/map_mod.py --packed PATH_TO_PACKED --project PATH_TO_PROJECT \
       --level 0 --name "My map" --spawn 132 115 32 --heading 0 \
       --out assets/my-map
   ```

   `--level` selects the template program. `--spawn` is the hero's start in
   game coordinates, and `--heading` is in radians. Choose a new output
   folder: the exporter refuses to overwrite one. It copies only this level's
   edited files, its terrain and available terrain textures. The source
   project, packed files and disc are not changed.
4. Use **Mods → Add mod → Folder** to install the exported folder. A zip with
   the same contents also works. Enable the map and turn on **Mods** under
   Play. The existing stack step includes its data and starts the map directly.

Map sessions use a fresh game with memory cards disabled, so they cannot
overwrite a campaign save. The hero's start is applied before the game's hero
and camera initialization. The window title shows the map's name. Disc movie
sequences, including the original introduction, are omitted by default;
`--keep-movies` retains them.

## Pack metadata

The export is a format-3 map pack. Older builds refuse this format instead of
silently installing the world as a replacement campaign level:

```json
{
  "format": 3,
  "game": "rac1",
  "version": "pal",
  "name": "My map",
  "map": {
    "level": 0,
    "spawn": [132, 115, 32],
    "heading": 0,
    "skip_movies": true
  }
}
```

`level` is an integer from 0 to 18. `spawn` has three finite coordinates;
`heading` defaults to zero, and `skip_movies` defaults to true. Ordinary packs
without `map` keep their existing behavior. The stacker generates `map.json`
for the port; do not hand-edit the stacked folder, which is replaced on each
play. The launcher passes `--map` when one is selected, so a port executable
without map support stops instead of running the map as the campaign.
Two enabled maps, or a map made for another disc, stop stacking with an
explanation. The previous successful stack is kept on a validation failure.

The map's template still supplies its game logic, object behaviors, original
in-game planet labels and any unedited resources. This does not create a new
planet on the campaign's ship map. Remove unwanted original triggers in the
editor; custom behavior can use the existing [mod SDK](../sdk/README.md).
The [sourcing policy](../docs/policy/SOURCING.md) applies to exports as to other
editor output: never commit or upload disc-derived game data with this code.
