# Going Commando: what every disc shares

One copy of the port's own C for Going Commando, built into each disc's program. All three are
built from the same decompilation (`games/rac2/decomp`: `src/`, plus `src/pal/` and `src/us2/` for
each release's own functions). The launcher picks the program by the disc's serial and the
executable's hash (`games/rac2/game.json`).

| Program | Disc | Serial |
|---|---|---|
| [rac2-ntsc](../rac2-ntsc/README.md) | USA v1.01 | `SCUS_972.68` |
| [rac2-ntsc2](../rac2-ntsc2/README.md) | USA v2.00 (Greatest Hits) | `SCUS_972.68` |
| [rac2-pal](../rac2-pal/README.md) | Europe/Australia (*Locked and Loaded*) | `SCES_516.07` |

| File | |
|---|---|
| `host/*.c` | the shared host C, in v1.01's names (below) |
| `profile.cpp`, `profile_*.h` | what the window reads of the game's memory, one table per disc |
| `script_binding.c` | the mods' script binding |
| `differences.json` | what the port does differently from the PS2 game ([SETTINGS.md](../../../docs/port/SETTINGS.md)) |
| `parity.tsv` | why the discs' programs bind a function differently, where they do ([rac1](../rac1/README.md#parity)) |
| `data_moves/` | how the NTSC-J and Korean discs move v1.01's data, kept for when they get programs |

Game facts the port relies on (level hooks, layouts, flights, credits, the Barlow vendor):
[RAC2_NOTES.md](../../../docs/port/RAC2_NOTES.md).

## Writing shared C

- **Functions** by v1.01's names: `func_0027F538`, `func_L05_0041DF58`, `func_0027F640__game`.
  Each disc's `host/rac2_version.h` maps them (v1.01's maps nothing).
- **Addresses** in `RAC2_ADDR(0x...u)`; a level's in `RAC2_LADDR(NN, 0x...u)`. A disc with no entry
  does not compile.
- **Constants that move** (a field offset, a text id): `RAC2_K(NAME, v1.01's value, where)`, `where`
  the function or table that holds it.
- **The layout**: `RAC2_LEVEL_BASE`, `RAC2_GP`, `RAC2_EXE_SIZE`.

| File | |
|---|---|
| `entry.c`, `level.c` | crt0 without the thread and heap system calls; ParseBin wrapped (which level's program is loaded) |
| `dma.c`, `spr_dma.c` | `sceDmaSend`, the scratchpad DMA the hand-written kernels poll |
| `font.c`, `ui2d.c`, `picture.c` | text, 2D quads and the menus' picture, drawn by the window at its own resolution |
| `hud.c`, `wide_view.c`, `options.c` | the HUD at a wide picture's edges, culling for the wider view, the game's 16:9 kept off |
| `movie.c`, `sound.c`, `io.c`, `print.c` | movies, 989snd commands, file open, printing |
| `handasm.c` | hand-written assembly the decompilation keeps as assembly, in C |
| `variants.c`, `callback_lists.c` | places the catalogue folds into one function, each with its own C |
| `trig.c`, `wad.c` | VU0 sine and cosine, the WAD decompressor (shared bodies in `common/lib/`) |
| `render.c`, `camera_latch.c` | the game's own renderers left out; the camera turn for a late picture |
| `tools.c`, `touch.c`, `play.c`, `mod_pages.c` | developer tools, on-screen pad hints, the mods' calls and pages |

## Carried releases

USA v2.00 and PAL read the decompilation through a *release view*: every v1.01 address name renamed
to the release's, from what rac2-decomp proves (its catalogues and its build's values read from the
release's bytes), then versionmap's pairs (same instruction, same code). A name with neither is
marked (`func_V1_...`, `D_V1_...`), never guessed. The release's build says what its C is
(`build/<v>/*/placed.json`): `placed` lists the functions it put in the release, and `gone_in` the
primary's definitions it left out, by file, which the view drops under any spelling, so no place
gets two bodies. An empty level hook its callers pair with one of the release's (`hooks`) keeps its
body, so every hook place has C. Their tables are generated: only
`release/release.json` is written by hand. It holds the release's facts (ids, title, frame and tick
rates, the decompilation's catalogue, build and dump folders, `level_base`, `$gp`, the primary
release's folder) and the pairs checked by hand, each with its evidence: `pairs` (functions),
`constants` (`RAC2_K`), `hand_names` (globals of the hand C the release table has no value for),
`hand` (the release's own hand C in `hand/`), `overrides` (the port's C for the release's own matched
functions, in `hand/`) and `extra` (additions to the carried `hostgen.json`).
The primary's hand C is written into `hand/` in the release's names, as rac1's releases do.

After a sync of `games/rac2/decomp`, with rac2-decomp built for v1.01 and the release
(`tools/build.py all`, `--version pal|us2`; `tools/carry.py --version pal|us2`), the three discs'
dumps in its `baserom/`, and `<r>` `pal` or `ntsc2` (`<v>` `pal` or `us2`). `tools/carry.py`
rewrites the decompilation's `config/<v>/*/functions.tsv`: put them back
(`git -C games/rac2/decomp checkout -- config/`) before `release.py build`, or the tables follow
catalogues CI does not have. Then publish the build's facts (`python3 tools/facts.py publish
rac2/decomp`, docs/SOURCES.md "Build facts"). Without a build, the same commands read the
published facts instead: only the discs are needed.

```sh
python3 port/tools/versionmap/versionmap.py build rac2/ntsc rac2/<r> --out build/versionmap/rac2-ntsc--rac2-<r>.json \
    --entries port/game/rac2-ntsc/hostgen.json --entries port/game/rac2-ntsc/libraries.tsv
python3 port/tools/hostgen/release.py build --game port/game/rac2-<r>/hostgen.json --source <rac2-decomp> \
    --map build/versionmap/rac2-ntsc--rac2-<r>.json
python3 port/tools/hostgen/release_carry.py --game port/game/rac2-<r>/hostgen.json \
    --primary port/game/rac2-ntsc/hostgen.json --source <rac2-decomp> \
    --map build/versionmap/rac2-ntsc--rac2-<r>.json --version <v> \
    --primary-protos port/build/release/games/rac2-ntsc/gen/game_protos.h
python3 port/tools/versionmap/data_moves.py write build/versionmap/rac2-ntsc--rac2-<r>.json port/game/rac2-<r>/release/data_moves.tsv
```

`release_carry.py` also writes `tables/call_copies.tsv`, `data_copies.tsv` and `level_twins.tsv`
(`copycalls.py`, which still runs alone; `--no-copycalls` leaves them as they are).
`--primary-protos` takes a v1.01 build's prototypes; without them the wrapped functions' C types
are dropped and the release's host C no longer compiles.

| File | |
|---|---|
| `release/release_names.json` | each v1.01 name and the release's, the values the release's build derived, and the evidence |
| `release/release_carry.json` | what rests on less than the same code (`near`, `called`, between moved neighbours, not found) |
| `release/data_moves.tsv` | how the release moved v1.01's data, from versionmap's map (addresses only) |
| `hostgen.json`, `libraries.tsv`, `hand/`, `tables/`, `host/` | rac2-ntsc's, in the release's names and places |
