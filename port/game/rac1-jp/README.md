# openrac-rac1-jp: Ratchet & Clank (NTSC-J), native

The native program of Ratchet & Clank's Japanese release (`SCPS_150.37`, NTSC-J v1.02, 60 Hz),
built from the same C as PAL v2.00's ([rac1-pal](../rac1-pal/README.md)): rac1-decomp builds this
release from that C, with its own definitions in `games/rac1/decomp/src/versions/jp/` where its code
differs ([games/rac1/decomp/docs/VERSIONS.md](../../../games/rac1/decomp/docs/VERSIONS.md)). Its text and fonts are its own
(Japanese): NTSC-J changed the font routines, so the shared host C's FontPrintWindow replacement
calls this release's own routine (`RAC1_FONT_PRINT_WINDOW` in `host/rac1_version.h`) and leaves out
the small font's, which this release has no counterpart of, as it leaves out the loading
picture's wrapper (`RAC1_OMIT`, with the reasons, in `release.json`).

```sh
python3 tools/build_port.py rac1-jp
port/build/release/openrac-rac1-jp --data <data> --levels <its levels> --window
```

Its levels are its own disc's (the launcher's "Get the levels ready" writes
`assets/port/rac1/jp`); the options are rac1-pal's.

## How it is made

Nothing here is written by hand but `release.json` (this release's ids and the facts its tables
are read from: `$gp`, the main segment's start, the save folder) and this file. The rest is
written by two tools from a built checkout of rac1-decomp:

```sh
# In rac1-decomp: tools/build_sn.sh, tools/gen_progress_report.py --no-build,
# tools/version_src.py build --version jp (the dumps in baserom/jp/), then:
python3 port/tools/hostgen/version_relocs.py --source <rac1-decomp> --version jp --out build/relocs-jp
python3 port/tools/versionmap/versionmap.py build rac1/pal rac1/jp --out build/versionmap/rac1-pal--rac1-jp.json \
    --entries port/game/rac1-pal/libraries.tsv --entries port/game/rac1/host/hand.c
python3 port/tools/hostgen/release.py build --game port/game/rac1-jp/hostgen.json --source games/rac1/decomp \
    --relocs build/relocs-jp --map build/versionmap/rac1-pal--rac1-jp.json
python3 port/tools/hostgen/release_rac1.py --game port/game/rac1-jp --primary port/game/rac1-pal \
    --map build/versionmap/rac1-pal--rac1-jp.json
```

| File | Written by | |
|---|---|---|
| `release_names.json` | release.py | the release view: each PAL v2.00 address name and this release's, and how each rests (`how`) |
| `hostgen.json` | release_rac1.py | rac1-pal's, every name the release's; places from `config/versions/jp/functions.tsv`; `"release"` splices `src/versions/jp/` in |
| `libraries.tsv`, `split_places.tsv`, `port_symbols.txt`, `hand/` | release_rac1.py | rac1-pal's, in this release's names and places |
| `host/*.c` | release_rac1.py | rac1-pal's own host C, in this release's names and addresses |
| `host/rac1_names.tsv`, `host/rac1_version.h` | release_rac1.py | the shared host C's ([port/game/rac1](../rac1/README.md)) names (renamed in one pass, release_host.py) and addresses (`RAC1_ADDR`) |
| `../rac1/profile_rac1-jp.h` | release_rac1.py | the profile's address tables for this disc |
| `release_carry.json` | release_rac1.py | everything that rests on less than the release's check or the same code (below) |

### Where the names come from

- **Functions**: the release's catalogue (`config/versions/jp/functions.tsv`) names each of its
  functions by its PAL v2.00 counterpart, with its places; `func_J_...` are its own. A PAL v2.00
  function it does not have is marked (`func_V2_...`, no address) or, where the release's checked
  C calls something else in its place, spelled `FUN_...` at that function, its PAL definition left
  out.
- **Globals**: the values rac1-decomp's check proved. `tools/version_check.py --version jp`
  places every C function at its counterpart and compares the relocated bytes with the release's;
  version_relocs.py records, for every function that is EXACT, each symbol its relocations name
  and the address the release's bytes gave it. Names no checked C relocates come from
  versionmap's `exact` pairs (the same instruction of the same code), then its `between` pairs
  (two neighbours that moved alike), which `release_names.json` marks as such; anything else is
  marked `D_V2_...`, and what uses it would be a stub (none is: hostgen's report has one stub,
  rac1-pal's `__builtin_trap`, and the release's own C of a function PAL v2.00 calls through a table only it has).

`release_carry.json` lists what the carried tables rest on besides that: rac1-pal's entries for
functions this release has its own C for, addresses from versionmap's `between` or `guess`, and
what it left out (no counterpart here).
