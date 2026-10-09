# extract

`openrac-extractor`: sets a game up from the image of the player's own
disc, the way OpenGOAL's extractor does for Jak and Daxter. It is the C++
form of [tools/extractor.py](../../tools/extractor.py), built with the port
so a player needs no Python: the same command line, the same layout and the
same failures (OpenGOAL's numbers), and it goes further, after
[ReRAC](https://github.com/re-rac/rerac)'s `rerac-extract` (ISC License,
Copyright (c) 2026 ReRAC contributors).

```sh
openrac-extractor IMAGE --game rac1 [--proj-path DIR] [--extract] [--validate] \
    [--decompile] [--compile] [--json]
```

| Step | What it does |
|---|---|
| `--extract` | copies the disc's files into `DIR/iso_data/GAME/`, hashing each as it is copied, and keeps the image there as `disc.iso` (a hard link where it can): the games read most of their data by sector. Writes `buildinfo.json` as tools/extractor.py does |
| `--validate` | refuses an image whose boot executable is not a build OpenRAC knows (the table in [assets/version.h](../assets/version.h), written from `games/*/game.json`); implied by `--extract`. An unknown build prints the line a maintainer would add |
| `--decompile` | turns the disc into what the port reads, in `DIR/decompiler_out/GAME/`: `toc.bin`, every level's own files (`levels/NN/level_header.bin`, `overlay.bin`, `core_data.bin`, ...), their WAD-compressed lumps unpacked (`levels/NN/unpacked/`), and `lumps.tsv`, the place in `disc.iso` of every other lump (movies, music, speech, global lumps), which stay where they are. Needs the game's disc layout ([assets/disc](../assets/disc/README.md)): RAC1 today |
| `--compile` | builds the native port: not available yet (4060) |

With no step named it runs them all. `--json` prints one JSON object per
line for the launcher (progress, info, the disc, one final `done` or `error`;
[report.h](report.h)); the exit status is 0, or 1 after `error NNNN: ...`
(2 for a wrong command line).

Every file is written as `<name>.partial` and renamed when complete, and a
step's folder is built as `_temp` and renamed at the end: a run that is
killed never leaves a complete-looking wrong folder. Free space is checked
before anything is written.

| Code | |
|---|---|
| 4000 | no boot executable (no `SYSTEM.CNF`, or the file it names is missing) |
| 4001 | not a version of this game OpenRAC knows (another game, or another game of the series) |
| 4002 | the serial is known, its boot executable is not (another revision, or a damaged image) |
| 4020 | not an ISO 9660 image |
| 4030 | the disc's data is not what its layout says |
| 4040, 4041 | not a `.iso` file; too small for a PS2 disc |
| 4060 | a step that does not exist yet for this game (OpenRAC's own) |
| 4061, 4062, 4063 | not enough free space; cannot write; cannot read the image or it is truncated (OpenRAC's own) |

## Files

| File | |
|---|---|
| [report.h](report.h) | the failures and their numbers; output for a person or as JSON lines |
| [extractor.h](extractor.h) | identify, validate, extract, decompile, and `run` |
| [main.cpp](main.cpp) | the command line |

Tested by `tests/extract/` on synthetic discs written to a temporary folder,
against a table of builds made for them.

Not done yet: the per-file SHA-1 table of a whole disc (ReRAC has one for
NTSC-U) to check every lump on `--decompile`; the launcher still runs
tools/extractor.py (`launcher/actions.json`) and moves to this program once
the port is packaged with it.
