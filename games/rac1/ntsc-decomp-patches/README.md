# OpenRAC's changes to Lombyte

Applied to [Lombyte](https://github.com/lombyte-project/Lombyte) (`games/rac1/ntsc-decomp`) by `python3 tools/sources.py apply`; how to change them: [docs/SOURCES.md](../../../docs/SOURCES.md).

Each patch says what it changes and why; most make C that relied on a register explicit, so
that hostgen translates it as the console runs it. The PS2 build is unchanged by them.

The files are OpenRAC's own, read only by the port or by OpenRAC:

- `config/us/port_symbols.txt`: the address of 49 readable names that some files call by a plain
  name and others label with `__asm__` (port/game/rac1-ntsc/hostgen.json "symbols"). Without them
  the port had no function for those names.
- `src/ui/menus/confirm_prompt_input.c`: C for `FUN_00221ab8`, the memory card prompt's Yes/No
  input, which splat has as a text gap (rac1-decomp's `Menu_ConfirmPromptInput`, the same 148 bytes).
- `README.md`: Lombyte's README as OpenRAC keeps it, with the NTSC decomp named as OpenRAC names
  it (a patch would carry the old line).
- `host/`: a runner that builds the project on an Apple Silicon Mac under rac1/pal's Wine.

<!-- patches:begin -->
<!-- Written by tools/sources.py save; do not edit. -->
| Patch | What |
|---|---|
| [0001-register-arguments.patch](0001-register-arguments.patch) | Pass the arguments the console left in registers. |
| [0002-unused-parameters.patch](0002-unused-parameters.patch) | Drop parameters the retail code never reads. |
| [0003-tex0-64-bit.patch](0003-tex0-64-bit.patch) | Keep TEX0 words 64 bits wide. |
| [0004-named-addresses.patch](0004-named-addresses.patch) | Name globals and functions instead of literal addresses. |
| [0005-big-modded-levels.patch](0005-big-modded-levels.patch) | Let a modded level use more than 32 MiB. |
| [0006-rac1-decomp-licence.patch](0006-rac1-decomp-licence.patch) | Name rac1-decomp's licence as GPL-3.0. |

Files under [files/](files) (copied as they are):

- [`README.md`](files/README.md)
- [`config/us/port_symbols.txt`](files/config/us/port_symbols.txt)
- [`host/README.md`](files/host/README.md)
- [`host/run.sh`](files/host/run.sh)
- [`host/wine8-chroot.sh`](files/host/wine8-chroot.sh)
- [`src/ui/menus/confirm_prompt_input.c`](files/src/ui/menus/confirm_prompt_input.c)
<!-- patches:end -->
