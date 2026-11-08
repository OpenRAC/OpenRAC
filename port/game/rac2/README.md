# Going Commando: one port for three discs

The port builds a program for each release rac2-decomp builds, from the same C
(`games/rac2/decomp`, the mirror of rac2-decomp: `src/`, and `src/pal/`, `src/us2/` for the
releases' own functions; games/rac2/decomp/docs/VERSIONS.md):

| Program | Disc | Serial | Its folder |
|---|---|---|---|
| `openrac-rac2-ntsc` | USA v1.01 | `SCUS_972.68` | [rac2-ntsc](../rac2-ntsc/README.md) |
| `openrac-rac2-pal` | Europe/Australia v1.00 (*Locked and Loaded*) | `SCES_516.07` | `rac2-pal` |
| `openrac-rac2-ntsc2` | USA v2.00 (Greatest Hits) | `SCUS_972.68` | `rac2-ntsc2` |

The player inserts any of them; the launcher reads the disc's serial and executable's hash and
picks the program (games/rac2/game.json). Everything here is shared:

```
port/game/rac2/host/*.c        the port's own C, with v1.01's names and addresses (rac2.h)
port/game/rac2/profile.cpp     what the window reads of the game's memory, one table per disc
port/game/rac2/differences.json the game fixes, the same for the three discs
port/game/rac2-<release>/      hostgen.json and the release's tables (below)
```

## Writing shared C

- **Functions by v1.01's names**: `func_0027F538`, `func_L05_0041DF58`, `func_0027F640__game`.
  Each release's `host/rac2_version.h` renames them (`#define func_0027F640 func_0027FB48`).
- **Addresses in `RAC2_ADDR()`**, written `0x%08Xu`: `OPENRAC_DATA(RAC2_ADDR(0x001B1614u))`; a
  level program's in `RAC2_LADDR(NN, 0x...u)`. A release without an entry does not compile.
- **The game's constants that move** (a field of the menus' objects, a text id) in
  `RAC2_K(NAME, v1.01's value, where)`, `where` the function whose instruction holds it (or the
  table `D_...` whose word it is): `#define TEXT_SPEED RAC2_K(TEXT_SPEED, 0x2F18, func_0033C738)`.
- **The layout**: `RAC2_LEVEL_BASE`, `RAC2_GP`, `RAC2_EXE_SIZE`.

v1.01's table (`rac2-ntsc/host/rac2_version.h`) renames nothing. The others are written by
[release_carry.py](../../tools/hostgen/release_carry.py), never by hand.

## Another release's port

hostgen and the port's tables know a function or a global by the address its name states, and
rac2-decomp's names state v1.01's. So a release's port reads the decompilation through its
**release view** ([release.py](../../tools/hostgen/release.py), hostgen.json `"release"`): every
address name renamed to the release's address, from what rac2-decomp proves (its catalogues, which
name each release function by the v1.01 function whose code it is, and the values its build read
from the release's bytes for every name its C uses), and from versionmap's pairs (the same
instruction of the same code) where its C does not. A name with neither is not guessed: it is
marked (`func_V1_...`, `D_V1_...`) and what uses it is a stub, or the release's own assembly.
Then the release's port is v1.01's on the release's addresses, and every rule of hostgen holds.

To write the tables again (after a sync of games/rac2/decomp), with a checkout of rac2-decomp at
the synced commit built for v1.01 and the release (`tools/build.py all`, `--version pal`,
`--version us2`; `tools/carry.py --version pal|us2` for `build/<version>/carry.json`, restoring
its catalogues), the dumps of the three discs in its `baserom/`, and versionmap's map:

```sh
python3 port/tools/versionmap/versionmap.py build rac2/ntsc rac2/pal --out build/versionmap/rac2-ntsc--rac2-pal.json \
    --entries port/game/rac2-ntsc/hostgen.json --entries port/game/rac2-ntsc/libraries.tsv
python3 port/tools/hostgen/release.py build --game port/game/rac2-pal/hostgen.json --source <rac2-decomp> \
    --map build/versionmap/rac2-ntsc--rac2-pal.json
python3 port/tools/hostgen/release_carry.py --game port/game/rac2-pal/hostgen.json \
    --primary port/game/rac2-ntsc/hostgen.json --source <rac2-decomp> \
    --map build/versionmap/rac2-ntsc--rac2-pal.json --version pal \
    --primary-protos port/build/release/games/rac2-ntsc/gen/game_protos.h
python3 port/tools/hostgen/copycalls.py --source games/rac2/decomp --version pal \
    --release-table port/game/rac2-pal/release_names.json --out port/game/rac2-pal/call_copies.tsv \
    --data port/game/rac2-pal/data_copies.tsv --config port/game/rac2-pal/hostgen.json \
    --twins port/game/rac2-pal/level_twins.tsv
```

and the same for `rac2-ntsc2` (`--version us2`). `--primary-protos` takes a v1.01 build's
prototypes: without it the host C's signatures for the functions it wraps (`asm.signatures`, the
ship HUDs among them) are dropped and the release's host C no longer compiles. What each writes:

| File | |
|---|---|
| `release_names.json` | the release view: each v1.01 name and the release's, per file for the short names (`func_2F5D68`), the values the release's build derived (hostgen's `"derived"`), the candidates left out, and how many of each kind of evidence |
| `hostgen.json` | rac2-ntsc's, each function and address the release's (the release's catalogues, `$gp`, the levels' base, the dumps) |
| `libraries.tsv`, `split_places.tsv` | rac2-ntsc's rows by the release's names |
| `call_copies.tsv`, `data_copies.tsv`, `level_twins.tsv` | written from the release's own programs, as rac2-ntsc's are |
| `host/rac2_version.h`, `host/rac2_addresses.h` | the shared host C's names, addresses and constants |
| `../rac2/profile_<id>.h` | the profile's table for the disc |
| `release_carry.json` | what rests on less than the same code: functions paired as changed, `near` or `called`, globals between moved neighbours, and what was found nowhere |
| `release_pairs.tsv`, `release_constants.tsv` | the few pairs and constants checked by hand, with the instructions that show them |

`asm_interface.json` (the build without the disc) is written as rac2-ntsc's is, with the release's
dumps (`--retail-exe`, `--retail-dump`).

## What differs between the discs

Measured from the programs, not assumed (release_carry.json has each):

- **PAL** runs at 50 Hz and its text is localised: the menus' text ids are others (`0x2DE2` is
  `0xBF3`), the front end's objects are larger (the Options page's list at the front end's block
  + `0x3E810`, not `+ 0x3E1D8`; a list's `+ 0x220` is `+ 0x230`), its pause Options list has seven
  rows, not eight, and the font routine takes a scale (v1.01's `func_0027FC70` is a thunk that passes
  1.0 to the release's own). The executable's data from `0x001A79F0` is 0x88 bytes on.
- **USA v2.00** has v1.01's layout with its fixes: the same menu objects as PAL's (larger), its own
  text ids, 1,117 functions of its own.
- In all three the moby, hero, camera and view records are the same, read by the same code.
