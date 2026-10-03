# Open questions

Decisions the group still has to make. Each says where things stand today and
what is affected. Settle one in a pull request that updates this file and the
docs it names.

## 1. Licensing

| Directory | License today |
|---|---|
| `games/rac1/pal` | MIT, Kryštof "Lynder063" Malinda ([LICENSE](../../games/rac1/pal/LICENSE)); libgcc sources under the GPL with the runtime exception |
| `games/rac1/ntsc` | MIT, Mateusz Kłysz ([LICENSE](../../games/rac1/ntsc/LICENSE)); GPL-2.0 libgcc and EE-GCC patches, newlib's license ([licenses/](../../games/rac1/ntsc/licenses)) |
| `games/rac2/ntsc` | MIT, llesieur99 ([LICENSE](../../games/rac2/ntsc/LICENSE)) |
| `games/rac3/ntsc` | **none stated**: the project has no license file |
| `editor/` | MIT, as part of rac1-decomp where it was written ([editor/LICENSE](../../editor/LICENSE)) |
| rest of the top level (`tools/`, `docs/`) | not chosen yet; see [LICENSE.md](../../LICENSE.md) |

To decide: a license for rac3 (its authors' call), and one for the top level.
MIT would match three of the four projects. Lombyte's
[THIRD_PARTY_NOTICES.md](../../games/rac1/ntsc/THIRD_PARTY_NOTICES.md) still
says rac1-decomp has no license; rac1-decomp has had an MIT license since
2026-09, so that note is out of date.

## 2. Credit lines for AI assistants in commits

The projects disagree:

- rac1/pal keeps `Co-Authored-By:` trailers for AI assistants.
- rac1/ntsc's optional commit hook ([scripts/commit-msg.py](../../games/rac1/ntsc/scripts/commit-msg.py))
  strips co-author and session trailers.
- rac3's pull-request rules ([docs/wiki/Pull-Requests.md](../../games/rac3/ntsc/docs/wiki/Pull-Requests.md))
  ask contributors to strip AI co-author lines.

OpenRAC's own commits so far keep the trailer. To decide: one rule for the
repository, written into [CONTRIBUTING.md](../../CONTRIBUTING.md).

## 3. Names that come from prerelease builds

Some committed names and layouts come from demo, preview or prototype discs
rather than retail ([SOURCING.md](SOURCING.md#prerelease-builds)):

- rac1/pal [include/structs.h](../../games/rac1/pal/include/structs.h): the
  IOP stash structures are attributed to debug symbols in a June 2002
  prototype, and are used by `src/game/stash.c` and
  `src/overlays/shared/stash_00295010.c`. The `BSphere`, `vec4` and
  `MobyInstance` blocks are attributed to STABS debug information without
  naming the build; no C file uses them. The `MobyInstance` field list is
  the same as the one left out of rac2 ([SOURCES.md](../SOURCES.md)),
  whose copy said it came from a leaked build. It also disagrees with
  matched retail code: `InitMobyInstance` (`func_0020D440`,
  [src/game/mobyfunc.c](../../games/rac1/pal/src/game/mobyfunc.c)) stores a
  float at +0x2C and a short at +0xA6, where the header has bytes, so it may
  describe another build of the engine.
- rac1/ntsc [docs/engine-source-layout.md](../../games/rac1/ntsc/docs/engine-source-layout.md):
  original source file names and structures from 2002 development and demo builds.
- rac2 [include/flags.h](../../games/rac2/ntsc/include/flags.h) and
  [include/gadgets.h](../../games/rac2/ntsc/include/gadgets.h) (prototype strings),
  and the class names in [docs/moby-dispatch.tsv](../../games/rac2/ntsc/docs/moby-dispatch.tsv)
  (a prototype's extracted tables). No C file includes rac2's headers.

To decide: whether names from prerelease builds may stay in code, or should be
replaced by names derived from retail alone.

## 4. One Ratchet & Clank tree for both regions

rac1/pal and rac1/ntsc decompile the same game, and have long shared and
ported functions ([games/rac1/pal/docs/SIBLING_DECOMPS.md](../../games/rac1/pal/docs/SIBLING_DECOMPS.md),
[config/overlays/us_map.tsv](../../games/rac1/pal/config/overlays/us_map.tsv)
maps 99.7% of the US code to PAL). Merging them into one tree that builds
both regions needs one answer first: which compiler built the game code. The
PAL project matches it with SN GCC 2.95.3; the US project with a patched GNU
EE-GCC 2.9 ([docs/toolchains](../toolchains/README.md)).

## 5. Continuous integration and decomp.dev

Each project had its own workflows; they are kept, inert, under
`games/*/*/.github/` (GitHub only runs workflows at the repository root).
To decide: which checks run for OpenRAC, how each game's report reaches
decomp.dev from one repository, and what happens to Lombyte's progress
branch and its private scoring repository.

## 6. Code shared between the games

The games share engine code and Sony's libraries. To decide, after measuring
it with a cross-game function map: whether identical functions live once in
a shared tree that several game builds compile, or stay copied per game with
their provenance noted.

## 7. Build hosts

- rac1/pal builds on macOS and Linux through Docker, and natively on Windows.
- rac1/ntsc builds on Linux or WSL, and on macOS through `./setup.sh --docker`.
- rac2 builds on Windows with WSL only (`scripts/wsl_chain.py` has
  machine-specific defaults).
- rac3 builds on Windows, or on Linux and macOS with wibo; several tools
  default to `C:\tools\...` paths.

To decide: whether OpenRAC aims for one container image that builds every
game on any host.

## 8. Project names and branding

The imported directories keep their projects' names and artwork (Lombyte's
logo, the rac2 logo). To decide: whether they stay as they are inside OpenRAC.
