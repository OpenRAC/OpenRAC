# OpenRAC's changes to rac4-decomp

Applied to [rac4-decomp](https://github.com/OpenRAC/rac4-decomp) (`games/rac4/decomp`) by `python3 tools/sources.py apply`; how to change them: [docs/SOURCES.md](../../../docs/SOURCES.md).

`files/src/game/00473A88.c` replaces the project's file whole, not as a diff: the five members
of its local struct are named by offset (`f20`, `f68`, `f69`, `f6A`, `fBE`), as the project's own
LEGAL.md asks, and a diff would carry the names it replaces
([open question 3](../../../docs/policy/OPEN_QUESTIONS.md#3-names-that-come-from-prerelease-builds)).
After an update, check that file against the project's version.

`files/README.md` and `files/docs/CREDITS.md` are the project's own, without a community credit
OpenRAC removed (#192); a diff would carry the line.

<!-- patches:begin -->
<!-- Written by tools/sources.py save; do not edit. -->
| Patch | What |
|---|---|
| [0001-tools-inside-openrac.patch](0001-tools-inside-openrac.patch) | Run the build tools from inside OpenRAC. |

Files under [files/](files) (copied as they are):

- [`README.md`](files/README.md)
- [`docs/CREDITS.md`](files/docs/CREDITS.md)
- [`src/game/00473A88.c`](files/src/game/00473A88.c)
<!-- patches:end -->
