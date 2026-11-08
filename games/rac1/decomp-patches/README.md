# OpenRAC's changes to rac1-decomp

Applied to [rac1-decomp](https://github.com/OpenRAC/rac1-decomp) (`games/rac1/decomp`) by `python3 tools/sources.py apply`; how to change them: [docs/SOURCES.md](../../../docs/SOURCES.md).

The files are OpenRAC's own:

- `nonmatching/functional/*.c`: functional C the port runs for functions with no matched C yet
  (listed in `nonmatching/functional.tsv` by patch 0004).
- `versions/ntsc/`: NTSC-U's splat config and the C where its code differs from PAL's, so
  `tools/version_build.py --version ntsc` links NTSC-U from this tree ([VERSIONS.md](../VERSIONS.md)).

<!-- patches:begin -->
<!-- Written by tools/sources.py save; do not edit. -->
| Patch | What |
|---|---|
| [0001-tools-inside-openrac.patch](0001-tools-inside-openrac.patch) | Run the tools from inside OpenRAC. |
| [0002-overlay-triage.patch](0002-overlay-triage.patch) | Triage overlay assembly from its own directory. |
| [0003-c-for-the-port.patch](0003-c-for-the-port.patch) | Write the C so the port's translation reads it. |
| [0004-functional-c.patch](0004-functional-c.patch) | Add functional C the port runs for unmatched functions. |
| [0005-big-modded-levels.patch](0005-big-modded-levels.patch) | Let a modded level use more than 32 MiB. |

Files under [files/](files) (copied as they are):

- [`nonmatching/functional/func_L00_002C5688.c`](files/nonmatching/functional/func_L00_002C5688.c)
- [`nonmatching/functional/func_L01_002E1FE0.c`](files/nonmatching/functional/func_L01_002E1FE0.c)
- [`nonmatching/functional/func_L02_002DA820.c`](files/nonmatching/functional/func_L02_002DA820.c)
- [`nonmatching/functional/func_L06_003006F8.c`](files/nonmatching/functional/func_L06_003006F8.c)
- [`nonmatching/functional/func_L07_0030C3E0.c`](files/nonmatching/functional/func_L07_0030C3E0.c)
- [`nonmatching/functional/func_L10_002EA578.c`](files/nonmatching/functional/func_L10_002EA578.c)
- [`nonmatching/functional/func_L13_00308860.c`](files/nonmatching/functional/func_L13_00308860.c)
- [`nonmatching/functional/func_L15_0029B1D8.c`](files/nonmatching/functional/func_L15_0029B1D8.c)
- [`tools/tests/test_triage.py`](files/tools/tests/test_triage.py)
- [`versions/ntsc/splat.yaml`](files/versions/ntsc/splat.yaml)
- [`versions/ntsc/src/game/bmain.c`](files/versions/ntsc/src/game/bmain.c)
- [`versions/ntsc/src/game/menu.c`](files/versions/ntsc/src/game/menu.c)
- [`versions/ntsc/src/game/missionfunc.c`](files/versions/ntsc/src/game/missionfunc.c)
- [`versions/ntsc/src/game/pause.c`](files/versions/ntsc/src/game/pause.c)
- [`versions/ntsc/src/game/space.c`](files/versions/ntsc/src/game/space.c)
- [`versions/ntsc/src/overlays/shared/menu_00249720.c`](files/versions/ntsc/src/overlays/shared/menu_00249720.c)
- [`versions/ntsc/src/overlays/shared/missionfunc_0024EEF0.c`](files/versions/ntsc/src/overlays/shared/missionfunc_0024EEF0.c)
- [`versions/ntsc/src/overlays/shared/mobyutil_00261B00.c`](files/versions/ntsc/src/overlays/shared/mobyutil_00261B00.c)
- [`versions/ntsc/src/overlays/shared/partupd_00272158.c`](files/versions/ntsc/src/overlays/shared/partupd_00272158.c)
- [`versions/ntsc/src/overlays/shared/pause_00277208.c`](files/versions/ntsc/src/overlays/shared/pause_00277208.c)
- [`versions/ntsc/src/overlays/shared/tieproc_00299108.c`](files/versions/ntsc/src/overlays/shared/tieproc_00299108.c)
- [`versions/ntsc/src/overlays/shared/vendor_002C99E0.c`](files/versions/ntsc/src/overlays/shared/vendor_002C99E0.c)
- [`versions/ntsc/src/overlays/shared/vendor_002CF2C0.c`](files/versions/ntsc/src/overlays/shared/vendor_002CF2C0.c)
<!-- patches:end -->
