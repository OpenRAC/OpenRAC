# Ratchet & Clank: Going Commando (2003)

Released in Europe as *Ratchet & Clank 2: Locked and Loaded*.

| Disc | game.json | rac2-decomp | Port |
|---|---|---|---|
| USA v1.01, `SCUS_972.68` (SHA-256 `9db2e33e…`) | `rac2/ntsc` | `--version us` (the default) | `openrac-rac2-ntsc` |
| USA v2.00 Greatest Hits, `SCUS_972.68` (SHA-256 `eda31b45…`) | `rac2/ntsc2` | `--version us2` | `openrac-rac2-ntsc2` |
| Europe v1.00, `SCES_516.07`, *Locked and Loaded* (SHA-256 `519f9fa7…`) | `rac2/pal` | `--version pal` | `openrac-rac2-pal` |

All three are built from one tree, [ntsc/](ntsc/README.md), OpenRAC's mirror of
[rac2-decomp](https://github.com/OpenRAC/rac2-decomp): the same C, with each release's own
functions in `src/pal/` and `src/us2/` ([ntsc/docs/VERSIONS.md](ntsc/docs/VERSIONS.md)). It
builds on macOS, Linux and Windows through Docker, and on Linux with Wine natively. The two US
discs boot the same serial and are told apart by their boot executable (size and SHA-1) and
their `SYSTEM.CNF` (`VER = 1.01`, `2.00`). A player inserts any of the three and gets the same
setup, build and play.

Since 2026-10-11 the project has rac1/pal's structure:

- **Catalogues.** `tools/levels.py` reads the boot executable and the 27 level programs from
  the disc and lists every distinct function once, with all its places
  ([ntsc/docs/LEVELS.md](ntsc/docs/LEVELS.md)).
- **Source.** `src/` holds every function in address order, as C or as an `INCLUDE_ASM`
  marker; a function shared by many levels is defined once.
- **Build.** `tools/build.py` links the C in place of splat's assembly and compares every
  loaded byte of all 28 programs with retail ([ntsc/docs/BUILDING.md](ntsc/docs/BUILDING.md)).
  Both compilers rebuild from public source (`tools/compiler/`).
- **Progress.** `progress/report.json` is objdiff's format and counts each distinct function
  once.

Some of its C was carried over from rac1/pal, where the two games share byte-identical
functions (the SDK and library code of the boot).

[ntsc/ports/pal-functional/](ntsc/ports/pal-functional/README.md) is a separate project inside
it: platypet2217-star's RAC2Decomp (MIT), a functional, not matching, reconstruction of the PAL
release (`SCES_516.07`) as C with SDL2 and OpenGL. It builds with CMake on its own and is not
part of the matching sources or the progress.

Each release has its own catalogues, build and report (`progress/`, `progress/pal/`,
`progress/us2/`). Disc checksums are in
[baserom/README.md](../../baserom/README.md); progress in [progress/](../../progress/README.md).

## Getting started

```sh
cd games/rac2/ntsc
python3 -m unittest discover -s tests       # tool tests, no game data needed
python3 tests/check_repository.py           # the catalogues and the layout
python3 tools/progress.py --check           # the report matches src/
```

To build, put the disc in `games/rac2/ntsc/baserom/SCUS_972.68.iso` and follow
[ntsc/docs/BUILDING.md](ntsc/docs/BUILDING.md); `bash tools/docker/run.sh <command>` runs any step
in the project's build image.
