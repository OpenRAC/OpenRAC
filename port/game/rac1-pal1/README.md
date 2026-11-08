# openrac-rac1-pal1: Ratchet & Clank (PAL disc v1.00), native

The native program of Ratchet & Clank's first PAL pressing (`SCES_509.16`, disc v1.00), built
from the same C as PAL v2.00's ([rac1-pal](../rac1-pal/README.md)): rac1-decomp builds this
release from that C, with its own definitions in `games/rac1/decomp/src/versions/pal_v1/` where its code
differs ([games/rac1/decomp/docs/VERSIONS.md](../../../games/rac1/decomp/docs/VERSIONS.md)). The two PAL discs
boot the same file name; the launcher tells them apart by the executable's size and SHA-1.

```sh
python3 tools/build_port.py rac1-pal1
port/build/release/openrac-rac1-pal1 --data <data> --levels <its levels> --window
```

Its levels are its own disc's (the launcher's "Get the levels ready" writes
`assets/port/rac1/pal1`); the options are rac1-pal's.

## How it is made

Nothing here is written by hand but `release.json` (this release's ids and the facts its tables
are read from: `$gp`, the main segment's start, the save folder) and this file. The rest is
written by two tools from a built checkout of rac1-decomp:

```sh
# In rac1-decomp: tools/build_sn.sh, tools/gen_progress_report.py --no-build,
# tools/version_src.py build --version pal_v1 (the dumps in baserom/pal_v1/), then:
python3 port/tools/hostgen/version_relocs.py --source <rac1-decomp> --version pal_v1 --out build/relocs-pal_v1
python3 port/tools/versionmap/versionmap.py build rac1/pal rac1/pal1 --out build/versionmap/rac1-pal--rac1-pal1.json \
    --entries port/game/rac1-pal/libraries.tsv --entries port/game/rac1/host/hand.c
python3 port/tools/hostgen/release.py build --game port/game/rac1-pal1/hostgen.json --source games/rac1 \
    --relocs build/relocs-pal_v1 --map build/versionmap/rac1-pal--rac1-pal1.json
python3 port/tools/hostgen/release_rac1.py --game port/game/rac1-pal1 --primary port/game/rac1-pal \
    --map build/versionmap/rac1-pal--rac1-pal1.json
```

| File | Written by | |
|---|---|---|
| `release_names.json` | release.py | the release view: each PAL v2.00 address name and this release's, and how each rests (`how`) |
| `hostgen.json` | release_rac1.py | rac1-pal's, every name the release's; places from `config/versions/pal_v1/functions.tsv`; `"release"` splices `src/versions/pal_v1/` in |
| `libraries.tsv`, `split_places.tsv`, `port_symbols.txt`, `hand/` | release_rac1.py | rac1-pal's, in this release's names and places |
| `host/*.c` | release_rac1.py | rac1-pal's own host C, in this release's names and addresses |
| `host/rac1_names.tsv`, `host/rac1_version.h` | release_rac1.py | the shared host C's ([port/game/rac1](../rac1/README.md)) names (renamed in one pass, release_host.py) and addresses (`RAC1_ADDR`) |
| `../rac1/profile_rac1-pal1.h` | release_rac1.py | the profile's address tables for this disc |
| `release_carry.json` | release_rac1.py | everything that rests on less than the release's check or the same code (below) |

### Where the names come from

- **Functions**: the release's catalogue (`config/versions/pal_v1/functions.tsv`) names each of its
  functions by its PAL v2.00 counterpart, with its places; `func_E1_...` are its own. A PAL v2.00
  function it does not have is marked (`func_V2_...`, no address) or, where the release's checked
  C calls something else in its place, spelled `FUN_...` at that function, its PAL definition left
  out.
- **Globals**: the values rac1-decomp's check proved. `tools/version_check.py --version pal_v1`
  places every C function at its counterpart and compares the relocated bytes with the release's;
  version_relocs.py records, for every function that is EXACT, each symbol its relocations name
  and the address the release's bytes gave it. Names no checked C relocates come from
  versionmap's `exact` pairs (the same instruction of the same code), then its `between` pairs
  (two neighbours that moved alike), which `release_names.json` marks as such; anything else is
  marked `D_V2_...`, and what uses it would be a stub (none is: hostgen's report has one stub,
  rac1-pal's `__builtin_trap`).

`release_carry.json` lists what the carried tables rest on besides that: rac1-pal's entries for
functions this release has its own C for, addresses from versionmap's `between` or `guess`, and
what it left out (no counterpart here).
