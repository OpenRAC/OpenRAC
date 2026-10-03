# Ratchet & Clank: Up Your Arsenal (2004)

Released in Europe as *Ratchet & Clank 3*.

| Version | Directory | Came from | Builds on |
|---|---|---|---|
| NTSC-U, `SCUS_973.53` (disc v1.00) | [ntsc/](ntsc/README.md) | [ratchet-uya-decomp](https://github.com/vetusmagnus/ratchet-uya-decomp) | Windows; Linux and macOS with wibo |

The project decompiles `frontbin.elf`, the frontend and menu program, which
lives inside the game's data rather than as a file on the disc; you extract it
with [Wrench](https://github.com/chaoticgd/wrench). The build reproduces it
byte for byte. The level programs and the other executables are counted for
progress, and a first set of common level functions is matched in C
([ntsc/docs/common_level_c.md](ntsc/docs/common_level_c.md)). The main
executable, `SCUS_973.53`, is not decompiled yet.

Its compiler research, 15 compilers against 8 flag sets, is in
[ntsc/docs/compiler_matrix_findings.md](ntsc/docs/compiler_matrix_findings.md);
it is the most systematic compiler study among the projects
([docs/toolchains](../../docs/toolchains/README.md)).

Disc checksums are in [baserom/README.md](../../baserom/README.md); progress
in [progress/](../../progress/README.md). The project has no license file yet
([open questions](../../docs/policy/OPEN_QUESTIONS.md#1-licensing)).

## Getting started

Follow [ntsc/docs/wiki/Setup.md](ntsc/docs/wiki/Setup.md): the SN ee-gcc
2.95.3 v1.36 toolchain ([toolchains/](../../toolchains/README.md)), your own
`frontbin.elf` in `games/rac3/ntsc/`, then `python3 tools/setup_asm.py` and
the build. The wiki's other pages ([ntsc/docs/wiki/](ntsc/docs/wiki/Home.md))
cover the workflow, matching patterns and pull requests.
