# Ratchet & Clank (2002)

Two decompilations of the same game, one per region, each with its own
toolchain and conventions. Both cover the boot executable and the 19 level
programs (code the game loads with each planet).

| Version | Directory | Came from | Builds on |
|---|---|---|---|
| PAL, `SCES_509.16` (disc v2.00) | [pal/](pal/README.md) | [rac1-decomp](https://github.com/Lynder063/rac1-decomp) | macOS and Linux through Docker; Windows natively |
| NTSC-U, `SCUS_971.99` (disc v1.00) | [ntsc/](ntsc/README.md) | [Lombyte](https://github.com/mateuszklysz/Lombyte) | Linux or WSL; macOS through `./setup.sh --docker` |

Disc checksums are in [baserom/README.md](../../baserom/README.md); progress
in [progress/](../../progress/README.md).

## How the two relate

- **Shared work.** The projects have ported functions to each other for a
  while: Lombyte credits code ported from the PAL project throughout its
  source and in its [THIRD_PARTY_NOTICES.md](ntsc/THIRD_PARTY_NOTICES.md), and
  the PAL project credits Lombyte in [its own](pal/THIRD_PARTY_NOTICES.md).
- **A map between them.** [pal/config/overlays/us_map.tsv](pal/config/overlays/us_map.tsv)
  pairs 99.7% of the US code with its PAL counterpart, from the code alone
  ([pal/docs/OVERLAYS.md](pal/docs/OVERLAYS.md)); `pal/tools/lombyte.py`
  uses it to find functions matched in one project and not the other; inside
  OpenRAC it reads `games/rac1/ntsc` and the US report kept in `progress/sources/`.
- **Different names.** PAL names functions by address (`func_0012D8F8`,
  `func_L05_002D48B8` in level code) with readable names in
  [pal/include/names.h](pal/include/names.h); the US project keeps one file per
  function under a subsystem path, with recovered names in
  [ntsc/config/us/recovered_names.json](ntsc/config/us/recovered_names.json).
- **Different compilers.** The PAL project matches the game code with SN GCC
  2.95.3; the US project with a patched GNU EE-GCC 2.9. Settling which one
  built the game comes before merging the two into one tree
  ([docs/toolchains](../../docs/toolchains/README.md),
  [open questions](../../docs/policy/OPEN_QUESTIONS.md#4-one-ratchet--clank-tree-for-both-regions)).

## Getting started

```sh
python3 tools/openrac.py setup rac1/pal rac1/ntsc   # from the top of OpenRAC
```

Then follow [pal/README.md](pal/README.md) or [ntsc/README.md](ntsc/README.md).
On an Apple Silicon Mac, `ntsc/`'s Docker build needs QEMU rather than Rosetta
for amd64 containers, because its Windows tools run under 32-bit Wine
([build hosts](../../docs/policy/OPEN_QUESTIONS.md#7-build-hosts)); `make check`
and all but two build steps also work with `RNC_WINE` set to wibo's i686 build.
The level editor ([editor/](../../editor/README.md)) reads the PAL disc.
