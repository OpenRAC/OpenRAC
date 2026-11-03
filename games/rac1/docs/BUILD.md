# Build

You need your own disc dump in `baserom/` and the toolchain in `toolchain/`. The [README](../README.md) has the setup.

Run commands from the repository root.

## Build

```sh
bash tools/docker/run.sh bash tools/build_sn.sh
```

A good build ends with:

```
every function is at its retail address
image matches retail outside the decompiled near-misses (0 bytes differ inside them)
```

## Check everything

Run this before a PR. It rebuilds, checks every version and refreshes the reports. It fails if a match number drops.

```sh
bash tools/check_all.sh
```

For a batch of changes, use the gate loop. It runs the same checks, and reverts any file that breaks a match. Exit 0
means it passed. Anything else means stop and read the output.

```sh
python3 tools/gate_loop.py
```

To merge a PR once every check of its newest commit has passed:

```sh
bash tools/land_pr.sh BRANCH
```

## Quick checks

These need no build and no disc.

```sh
python3 tools/gen_progress_report.py --check
python3 tools/move_src.py --check
python3 tools/check_docs.py
python3 tools/fakematch_scan.py --check
python3 tools/check_no_retail.py
python3 tools/check_syntax.py
python3 tools/ratchets.py --check
python3 tools/format.py --check
python3 -m unittest discover -s tools/tests -p 'test_*.py'
```

## Data in C

A file in `src/data/` names a block of retail's data and says which source file owns it. It holds no numbers: those
are generated from your own baserom into `build-sn/data_inc/`, and are never committed. A table of only symbols and
zeros, or a block of only zeros, is written out in the file, since it holds none of retail's numbers.

To give a block a struct type, record the layout, then set it. A block that does not fit stays as it is, and says why:

```sh
python3 tools/data_types.py learn "Camera:structs/camera.h"     # needs clang
python3 tools/data_in_c.py set-type D_00187510=Camera
```

```sh
python3 tools/data_in_c.py add D_00165600:src/game/ui/pause.c
```

## Move a source file

Never use `git mv` on a `.c` file. The object lists, version folders and tools all name its path.

```sh
python3 tools/move_src.py src/core/libc/errno.c src/core/libc/error_number.c
```

After a move, the ELF's SHA changes because file names are stored in it. Compare the loaded bytes, or run `check_all.sh`.

## Format

```sh
python3 tools/format.py
```

To hide formatting commits in `git blame`:

```sh
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

## More

- Toolchain: [reference/TOOLCHAIN.md](reference/TOOLCHAIN.md)
- Build steps: [reference/BUILD_FIDELITY.md](reference/BUILD_FIDELITY.md)
- Containers: [reference/CONTAINERS.md](reference/CONTAINERS.md)
