# Extractor

Reads the levels, models, textures and sounds of your own Ratchet & Clank disc (and, in early
access, Going Commando's, Up Your Arsenal's and Deadlocked's) and writes them the way the native
port uses them. The launcher runs it for you; nothing here needs Godot.

```sh
python3 extractor/extract.py survey ISO                 # the disc's layout, as JSON
python3 extractor/extract.py port ISO assets/port/rac1/pal  # the levels for the port
python3 extractor/extract.py raw ISO OUT --level N      # one level's sections, as stored and decoded
python3 extractor/occlusion.py ISO assets/port/rac1/pal # each level's visibility (PVS)
```

Output goes under `assets/` or `build/` (both ignored by git), or anywhere with `--anywhere`. Never
commit or share it: see [LEGAL.md](../LEGAL.md).

**Packs for the game:** [models and textures](MODDING.md) (`assets.py`), [sound](AUDIO_MODDING.md)
(`audio.py`), and how the launcher stacks packs (`overrides.py`). How a moby's behaviour is made,
and where modders change it: [BEHAVIOUR.md](BEHAVIOUR.md).

## The level editor

The level editor, its Godot projects, the level packer and their tests live in
[OpenRAC Editor](https://github.com/OpenRAC/OpenRAC-Editor). Its tools read discs and levels with
this folder's modules, so what they may use is a contract:

- [extractor_api.json](extractor_api.json): the modules the editor's tools import, and a version
  raised when one of them changes in a way those tools would notice. `test_extractor_api.py` keeps
  the list honest: none of them may import a module the editor took with it.
- `packed_format` (in the same file): the format of the packed levels the editor writes and the
  port and `overrides.py` read (`packed.json`'s `format`).
- The port's `--moby-states` run, which the editor uses to see how the game starts a level's mobys
  ([docs/port/MOBY_STATES.md](../docs/port/MOBY_STATES.md)).

## Code

Python 3 standard library only. Tests: `python3 -m unittest discover -s extractor`.

## Credits

- **[Wrench](https://github.com/chaoticgd/wrench)** by chaoticgd and contributors
  (GPL-3.0-or-later). Most of what the extractor knows about terrain, ties, shrubs, the sky,
  textures and placements comes from reading its source
  ([ASSETS.md](../games/rac1/decomp/docs/ASSETS.md#sources-and-credits) lists which files each
  format came from), and so does what the later games change
  ([LATER_GAMES.md](../docs/engine/formats/LATER_GAMES.md)).
- **[Replanetizer](https://github.com/RatchetModding/Replanetizer)** by RatchetModding
  contributors, consulted for what fields mean.
- **[ReRAC](https://github.com/re-rac/rerac)** (ISC): the moby instance record, the moby class
  format and the collision block ([ASSETS.md](../games/rac1/decomp/docs/ASSETS.md#collision)).
- **[Lombyte](https://github.com/lombyte-project/Lombyte)** (GPL-3.0): the moby class names
  (`moby_classes.tsv`, see its header).
- **[OpenGOAL's jak-project](https://github.com/open-goal/jak-project)**, whose extractor was the
  model for extracting from your own disc.

No code from these projects is included; the extractor is newly written.
