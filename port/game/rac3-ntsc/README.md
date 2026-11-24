# openrac-rac3-ntsc: Up Your Arsenal, NTSC-U

Up Your Arsenal's NTSC-U disc (`SCUS_973.53`): rac3-pal's program at this disc's addresses, read
through the NTSC-U view of [games/rac3](../../../games/rac3/decomp) (`src/` and `src/us/`), as Going
Commando's PAL and USA v2.00 are rac2-ntsc's ([rac2: carried releases](../rac2/README.md#carried-releases)).
PAL is the primary release here: rac3-decomp's C is named by PAL's addresses, and the port's tables
and host C are written for PAL first.

**Runs as far as PAL does**: the boot screens (the legal notice, the credit screen), the main
menu with its labels, frames and background picture, and a level: Veldin loads and is drawn
natively from the disc's level data, Ratchet in his armour, as on PAL. Every level function is
translated from the disc's level programs (NTSC-U's own catalogue, `config/us/levels/functions.tsv`):
rac3-decomp's level C is PAL's, named by PAL's level addresses.

```sh
python3 tools/build_port.py rac3-ntsc --disc <your NTSC-U disc image>
# a data folder set up by python3 tools/extractor.py <image> --game rac3 -e --proj-path <data>:
# iso_data/rac3/ with the disc's files, sectors.tsv and programs/ (the two programs, unpacked)
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
| `checks/result_allow.tsv` | rac3-pal's, in NTSC-U's names |
| `release/release.json` `level_addresses` | a level's own globals the shared C writes as `RAC3_LADDR(level, 0x...u)`, from the same instruction of the same code ([`rac3_level_pairs.py`](../../tools/hostgen/rac3_level_pairs.py), which reads both discs' level dumps) |

After a change to rac3-pal's hostgen.json, libraries.tsv or the addresses its host C wraps, or a
sync of `games/rac3/decomp`, write them again (the second needs both discs' level dumps in the
decompilation's `baserom/`, for a new `RAC3_LADDR`):

```sh
python3 port/tools/hostgen/release_rac3.py --game port/game/rac3-ntsc/hostgen.json --source games/rac3/decomp
python3 port/tools/hostgen/rac3_level_pairs.py --source games/rac3/decomp --write
python3 port/tools/hostgen/release_carry_rac3.py --source games/rac3/decomp
```

`release_carry_rac3.py` names every literal address of rac3-pal's host C that NTSC-U has
elsewhere and that is not wrapped yet: wrap it. `parity.py rac3` (run by the build) checks that the
two programs bind the same functions the same way (`port/game/rac3/parity.tsv` for the known
exceptions).

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

- **Level programs**: the same 49 single-player and multiplayer levels plus two split-screen maps
  of NTSC-U's own (54, 55: builds of 44 and 45). Veldin's globals sit 0x80 higher (its `.lit` is
  0x40 longer); the multiplayer levels' `$gp` is 0x00248140 (PAL 0x00247FC0). Of Veldin's
  functions whose catalogue rows start there, 2,161 are PAL's C (`match`), 1,251 assembly on both
  discs and 781 other code on NTSC-U (`differs`).
- **The editor's export** reads Ratchet's looks, wrench, items and ship where PAL's does (the
  same table of contents): extractor/wads.py.

## What is left

- The pause and in-level menus, the HUD and the other levels on NTSC-U (menus.py's walk passes on
  NTSC-U: `--version ntsc`, the boot screens, card notices, Load Game, New Game with and without a
  save, Options with its PROGRESSIVE SCAN row, Adjust Screen and its three languages; New Game
  reaches Veldin through the loading screen).
- rac3-decomp's level C on NTSC-U (src/levels in the release view): its level names and its
  levels' data names would need NTSC-U's values per level (rac3-decomp's NTSC-U build reads them
  from the disc; they are not in its tables yet).
- `asm.signature_check` is off for NTSC-U (release.json says why): its levels are translated from
  the disc, and rac3-pal's signatures cover PAL's names only (1,994 findings in 706 functions).
