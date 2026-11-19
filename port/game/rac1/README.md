# Ratchet & Clank: what every disc shares

One copy of the port's own C for Ratchet & Clank, built into every disc's program
(`openrac-rac1-pal`, `-ntsc`, `-pal1`, `-jp`). A fix here holds for every disc; a disc decides
only its own names and addresses.

| File | |
|---|---|
| `host/*.c` | the shared host C, in games/rac1's names: overlay hook, direct start, 2D and world hand-off, collision, hardware routines, folded-function variants, HUD, options |
| `game_state.*`, `profile.cpp` | the game's profile (below) |
| `profile_rac1-pal1.h`, `profile_rac1-jp.h` | the profile's address tables for those discs (generated) |
| `differences.json` | what the port does differently from the PS2 game ([SETTINGS.md](../../../docs/port/SETTINGS.md)) |
| `parity.tsv` | why the discs' programs bind a function differently, where they do (below) |

Each disc's own folder holds only what differs: `hostgen.json`, `libraries.tsv`,
`split_places.tsv`, `hand/`, and in `host/` its table (`rac1_version.h`) and C only it needs.

## Writing shared C

Each file includes [rac1.h](host/rac1.h), which reads the disc's translation and then its table.

- **Functions** by games/rac1's names: `func_001F8B6C`, `func_L00_002A8A20`, and
  `func_0022C9A8__game` for the game's C under a `wrap`. A disc that names them otherwise maps them
  in its table.
- **Addresses** in `RAC1_ADDR(0x...u)`, one literal each; a level's in `RAC1_LADDR(NN, 0x...u)`.
  A disc with no entry for an address does not compile.
- **Left out on a disc**: `#if !RAC1_OMIT(func_...)` around the code, and the reason in that
  disc's table (or `release.json` `omit`).
- **Other C types**: `RAC1_TYPE_<name>_<what>` in each table. **Names in strings**:
  `RAC1_NAME(func_...)`.

[hostnames.py](../../tools/versionmap/README.md) checks a table against the shared C: every name
and address used has an entry, every entry is used, no address sits outside `RAC1_ADDR`. ctest
runs it as `rac1-*_host_table`.

## The game's profile

The host's C++ reads the running game (camera, fog, live mobys, display list) through this
profile ([host::GameProfile](../common/profile.h)):

- `game_state.h`, `game_state.cpp`: the address tables (one per disc for the executable, one per
  level) and the reads (`read_game_state`, `read_live_moby`, `shown_chain`, ...).
- `profile.cpp`: one profile per disc over those tables, picked by id in `host::game_profile()`.

Programs, the viewer and its tests link them as `openrac_game_rac1_profile`
([Window.cmake](../../cmake/Window.cmake)). A new disc adds its table and profile here; nothing else
changes.

## Carried releases

PAL v1.00 (`rac1-pal1`) and NTSC-J (`rac1-jp`) are built from PAL v2.00's C, with rac1-decomp's
own C where their code differs (`src/versions/<v>/`). Their tables are generated: only
`release.json` and the README are written by hand. Never edit the rest; CI checks them with
`release_rac1.py --tables --check` (no disc needed).

From a built rac1-decomp (`tools/build_sn.sh`, `tools/gen_progress_report.py --no-build`,
`tools/version_src.py build --version <v>`), with `<disc>` `pal1` or `jp` and `<v>` its rac1-decomp
version (`pal_v1` or `jp`):

```sh
python3 port/tools/hostgen/version_relocs.py --source <rac1-decomp> --version <v> --out build/relocs-<v>
python3 port/tools/versionmap/versionmap.py build rac1/pal rac1/<disc> --out build/versionmap/rac1-pal--rac1-<disc>.json \
    --entries port/game/rac1-pal/libraries.tsv --entries port/game/rac1/host/hand.c
python3 port/tools/hostgen/release.py build --game port/game/rac1-<disc>/hostgen.json --source games/rac1/decomp \
    --relocs build/relocs-<v> --map build/versionmap/rac1-pal--rac1-<disc>.json
python3 port/tools/hostgen/release_rac1.py --game port/game/rac1-<disc> --primary port/game/rac1-pal \
    --map build/versionmap/rac1-pal--rac1-<disc>.json
python3 port/tools/versionmap/data_moves.py write build/versionmap/rac1-pal--rac1-<disc>.json port/game/rac1-<disc>/release/data_moves.tsv
```

| File | |
|---|---|
| `release.json` | by hand: ids, `$gp`, main segment start, save folder, rows for functions only this disc has, `omit` |
| `release_names.json` | each PAL v2.00 name and this disc's, and what it rests on (`how`) |
| `release_carry.json` | what rests on less than the check or the same code |
| `data_moves.tsv` | how the disc moved PAL v2.00's data, from versionmap's map (addresses only) |
| `hostgen.json`, `libraries.tsv`, `split_places.tsv`, `port_symbols.txt`, `hand/`, `host/` | rac1-pal's, in this disc's names and places |

Names come from rac1-decomp's check first: `tools/version_check.py` places each C function at its
counterpart and compares bytes, and every relocation of an exact function gives a name's address.
Then versionmap's `exact` pairs (same instruction, same code), then `between` (two neighbours that
moved alike). Anything else is marked `D_V2_...`. `func_E1_...` (PAL v1) and `func_J_...` (NTSC-J)
are the disc's own functions.

## Parity

[parity.py](../../tools/versionmap/README.md) compares every rac1 program: what each binds per
function, each row's C type, and every carried data address (against `release_names.json` and
`data_moves.tsv`, each on its own). A difference not in `parity.tsv` fails the build and CI.
