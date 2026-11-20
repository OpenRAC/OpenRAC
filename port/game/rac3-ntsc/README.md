# openrac-rac3-ntsc: Up Your Arsenal, NTSC-U

Up Your Arsenal's NTSC-U disc (`SCUS_973.53`): rac3-pal's program at this disc's addresses, read
through the NTSC-U view of [games/rac3](../../../games/rac3/decomp) (`src/` and `src/us/`), as Going
Commando's PAL and USA v2.00 are rac2-ntsc's ([rac2: carried releases](../rac2/README.md#carried-releases)).
PAL is the primary release here: rac3-decomp's C is named by PAL's addresses, and the port's tables
and host C are written for PAL first.

**Runs as far as its main menu**, as PAL does: the boot screens (the legal notice, the credit
screen), then the main menu with its labels, frames and background picture. Levels are not
translated yet (no `src/levels`, no level catalogue): see [What is left](#what-is-left).

```sh
python3 tools/build_port.py rac3-ntsc --disc <your NTSC-U disc image>
# a data folder with iso_data/rac3/SCUS_973.53 (the disc's boot executable) and iso_data/rac3/disc.iso
SDL_AUDIODRIVER=dummy port/build/release/openrac-rac3-ntsc --data <data> --window --skip-movies
```

The launcher builds and plays it from a player's NTSC-U disc as it does PAL's (launcher/actions.json,
`rac3/ntsc`).

## One tree for both discs

Nothing of PAL's port is copied here. The NTSC-U program is built from:

- **the shared host C**, rac3-pal's [`host/`](../rac3-pal/host) (hostgen.json `host_shared`), each
  file written once in PAL's names and compiled again for NTSC-U with them renamed
  (`host/rac3_names.tsv`, tools/hostgen/release_host.py) and with this folder's
  [`host/rac3_version.h`](host/rac3_version.h) first on the include path. An address of the game's
  memory the shared C reads is written `RAC3_ADDR(0x...u)`, PAL's value; NTSC-U's is in
  `host/rac3_addresses.h`. A fix written in rac3-pal's host C holds for both discs;
- **the profile**, rac3-pal's [`profile.cpp`](../rac3-pal/profile.cpp), which holds both discs'
  tables (`kPal`, `kUs`) and picks them by the program's id (`openrac_game_rac3_profile`);
- **the tables**, carried from rac3-pal's by
  [`release_carry_rac3.py`](../../tools/hostgen/release_carry_rac3.py), never written by hand.

| File | |
|---|---|
| `release/release.json` | **by hand**: this disc's own hostgen.json keys (title, serial, 60 Hz, `src/us`, the retail image), the function pairs rac3-decomp's map does not make yet and the addresses no global names, each with its evidence |
| `release/release_names.json` | each PAL name and NTSC-U's (release_rac3.py, from rac3-decomp's `config/us/map.tsv` and `config/us/names.tsv`) |
| `release/release_carry.json` | what the carried tables rest on besides the same code: host replacements and wraps of a function NTSC-U has other code for, rows left out, addresses not found |
| `hostgen.json`, `libraries.tsv` | rac3-pal's, in NTSC-U's names |
| `host/rac3_names.tsv`, `host/rac3_addresses.h` | the shared host C's function names and addresses, NTSC-U's |
| `host/rac3_version.h` | the shared host C's switch to NTSC-U (by hand) |

After a change to rac3-pal's hostgen.json, libraries.tsv or the addresses its host C wraps, or a
sync of `games/rac3/decomp`, write them again:

```sh
python3 port/tools/hostgen/release_rac3.py --game port/game/rac3-ntsc/hostgen.json --source games/rac3/decomp
python3 port/tools/hostgen/release_carry_rac3.py --source games/rac3/decomp
```

## What differs from PAL

- **60 ticks a second**: NTSC-U counts in sixtieths (its settings word 0x00143950 stays 0, the
  branch PAL never takes). The boot screens last the same seconds: the legal notice 180 ticks
  (PAL 150), the credit screen 180 (PAL 150); the main menu shows at tick 389 (PAL 330).
- **The resident core** (below 0x001D5680) is at PAL's addresses with PAL's code, but for 46 moved
  functions and a few libraries; `.lit` starts at 0x001D5680 on both discs, `$gp` is 0x001DC8B0 on
  both, and the loader carries the game at the same file offset (0x7080).
- **From `.bss` up everything moved**: the boot program's `.text` starts at 0x00381180 (PAL
  0x00381080), its data 0x80 later (`D_001DA050` is NTSC-U's `D_001DA0D0`); frontbin's `.text` at
  0x0037D100 (PAL 0x00381080). The front end's entry is 0x00381280 (PAL 0x00381250).
- **Code of its own**: 203 functions of the executable and frontbin have other code on NTSC-U
  (map.tsv `differs`), and 745 more are assembly on both discs. Three of the port's wraps sit on
  such functions (release_carry.json): the front end's start-up (0x003982E0, PAL 0x003983C8), the
  Options rows' texts (0x003B6220, PAL 0x003B63D0) and DrawWidgetSprite (0x003AA630, PAL
  0x003AA780).
- **Functions rac3-decomp's NTSC-U map does not pair**: the Widget3D classes' 8-byte type-id
  getters, which NTSC-U's function list counts into the remnant before them (0x003F0140 holds
  0x003F0168's `jr ra; li v0, 2`). Without them the main menu's labels were never set (the setter
  is looked up by that type id): paired by hand in release.json.

## What is left

- menus.py's walk (`--version ntsc`), then New Game and a level: the levels' C (`src/levels`) needs
  the release view's level names and NTSC-U's level catalogue (`config/us/levels/functions.tsv`),
  and the profile's per-level addresses (`kUs.levels`) and render.c's level tables NTSC-U's values.
