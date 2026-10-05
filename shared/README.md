# shared: what the games have in common

| Here | What it is | Maintained by |
|---|---|---|
| [xmap/](xmap/README.md) | The function map across the five versions: how much code they share, a table of every function two or more games share ([functions.tsv](xmap/functions.tsv)), and the functions each project could port from another ([ports/](xmap/ports)) | `python3 tools/xmap.py scan`, `report`, `ports` (generated; never edit) |
| [files.json](files.json) | The files several games build with, copy for copy, and the files that began as copies and now differ on purpose | by hand; `python3 tools/shared.py check` holds the copies identical, in CI too |

[docs/engine/SHARED_CODE.md](../docs/engine/SHARED_CODE.md) explains the map
and how to use it.

## Shared files

Each project still also lives in its own repository, where it has to build on
its own. So a file two games share stays in both, and `files.json` says they
are one file:

```sh
python3 tools/shared.py check        # every group's copies are identical
python3 tools/shared.py find         # identical files across games that are not listed yet
python3 tools/shared.py sync PATH    # make the other copies equal to PATH
```

When a sync from a project's repository changes one copy, `check` fails. Then
either the change belongs to every game that has the file (`sync` it, and
verify each game's build), or the files have parted ways (move the group to
`related` with the reason). Today the groups are GCC's libgcc sources and
build helpers that rac1/pal and rac4 share, and the label macros rac3 and rac4
share.

Once the projects work here rather than in their own repositories, these
files, and the library code every game contains, get one home under `shared/`
that each game's build compiles and checks
([open question 6](../docs/policy/OPEN_QUESTIONS.md#6-code-shared-between-the-games)).
