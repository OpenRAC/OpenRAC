# Shared maps

A shared map (`.openrac-map`) is a level someone made in OpenRAC Editor, ready to play. It holds
no bytes of the game: those come from the player's own disc and extracted levels.

**Adding one.** In **Mods**, pick or drop the file, on a computer or on Android (a file saved as
`.openrac-map.zip`, as Android's downloads do, is a map too). The launcher makes the map's planet
pack from the player's files (`launcher/core/src/maps.rs`) and adds it to the packs, on: a new
planet the ship can fly to. It takes a few seconds and needs no editor and no Python. Adding the
same map again replaces it in its place; the same map for another disc is a pack of its own.

**Cutscenes and moving objects.** The game has their player built in (Ratchet & Clank,
`port/game/rac1/editor_runtime`; the editor's formats are in [CUTSCENE_FORMAT.md](https://github.com/OpenRAC/OpenRAC-Editor/blob/main/docs/CUTSCENE_FORMAT.md) and
[BEHAVIOUR_FORMAT.md](https://github.com/OpenRAC/OpenRAC-Editor/blob/main/docs/BEHAVIOUR_FORMAT.md)). It reads only what a map's pack holds, so it does
nothing on the game's own levels.

## The file

A zip with `map.json` and the maker's own bytes, `data/<n>.bin` for file `n`:

```json
{
  "format": "openrac-map", "version": 2,
  "id": "openrac-museum", "name": "OpenRAC Museum", "author": "...", "about": "...",
  "credits": [], "made": "2026-11-28", "game": "rac1", "disc": "pal",
  "sources": [
    {"port": "level_11/terrain.glb", "sha256": "..."},
    {"disc": [1984642, 6744], "sha256": "..."},
    {"wad": 1, "at": 576, "sha256": "..."}
  ],
  "files": [{"path": "level_11/core_pal.bin", "sha256": "...", "size": 13811712, "parts": []}]
}
```

A source is a file of the extracted levels (`port`), sectors of the disc (`disc`: first, count,
read through `iso_data/<game>/sectors.tsv`), or the WAD stream at a place in another source,
unpacked (`wad`). Each file of the pack is its parts, in order:

| Part | Makes |
| --- | --- |
| `["data", at, length]` | bytes of the map's own `data/<n>.bin` |
| `["copy", source, at, length]` | bytes of a source |
| `["wad", [header parts], [parts]]` | what the parts make, packed as the game's WAD stream (`launcher/core/src/wad.rs`, byte for byte as the editor packs it) |
| `["point", source, at, count, matrix]` | `count` positions of a source moved by a 4 x 4 matrix (16 doubles as hex, column-major) |
| `["normal", source, at, count, matrix]` | `count` normals turned by the inverse matrix given, made one long |
| `["slot", source, at, count]` | `count` floats, each followed by a zero |
| `["index", source, at, count, width, flip]` | `count` indices of `width` bytes as 32-bit ones, each triangle's last two swapped when `flip` is 1 |

The point, normal, slot and index parts place a model of the disc in the level, as the editor's
bake writes it into the terrain. Every source and every file made is checked against its SHA-256:
a map made from another disc, or from levels another OpenRAC extracted, is refused with what to do.
