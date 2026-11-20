# Up Your Arsenal's save file (PAL, SCES_524.56)

What the launcher's Up Your Arsenal save editor (`core/src/save_edit_rac3.rs`,
`src/components/SaveEditorRac3.svelte`) and the port (`port/game/rac3-pal/host/save_format.h`,
`play.c`) read and write, and how it was found. Addresses are the PAL executable's (`boot_elf`);
the functions are in `port/game/rac3-pal/IDENTIFIED.tsv`. Ratchet & Clank 3 and Up Your Arsenal
are the same game; the NTSC-U disc (`SCUS_973.53`) is not checked here.

## The card

On a fresh card the game asks to create its save file (dialog 15: "No Ratchet and Clank 3 save
data... Would you like to create a Ratchet and Clank 3 save file?"; **Yes is Circle**, No is
Triangle) and needs 600 KB free. It then writes, in `<card>/BESCES-52456RATCHET/` (the port's
`<data home>/memcard/SCES_524.56/BESCES-52456RATCHET/`):

| file                     | bytes   | what                                                      |
| ------------------------ | ------- | --------------------------------------------------------- |
| `icon.sys`               | 964     | the card browser's title and icon settings                |
| `static.ico`             | 33,112  | the icon (read from the disc into a scratch buffer first) |
| `net.bin`                | 4,028   | the network configuration                                 |
| `save0.bin`..`save3.bin` | 134,992 | the four save slots, all written whole (empty: level -1)  |
| `BESCES-52456RATCHET`    | 28,672  | the game's own file beside them                           |

The folder name is the card code's pattern (twelve stars before `RATCHET`) filled with `B`, the
region's letter and the serial. The slot list takes `save` and one digit under 4. When it starts,
the front end also opens Ratchet & Clank's and Going Commando's saves (`BASCUS-97199RATCHET`,
`BESCES-50916RATCHET`, `BISCPS-15037RATCHET`, `BASCUS-97268RATCHET`, `BESCES-51607RATCHET`,
`BISCPS-15056RATCHET`, `BASCKA-20011RATCHET`); the port's card shows them read only from the
other games' card folders (port/game/common/lib/mc.c).

## The container

Each `saveN.bin` is 134,992 bytes, written by SavePack (`func_0039E9C0`) and read back section by
section (`func_0039ED78`) by the card machine (`func_0039C7F0`). It is Going Commando's container
(docs/RAC2_SAVE_FORMAT.md) with other tables and 37 level sections:

```text
+0  u32 global section size  (14216)
+4  u32 level section size   (3264)
+8  the global section, then 37 level sections (level numbers 0..36)
section: u32 data size, u32 crc16(data), the chunks, then {s32 -1, u32 0}
chunk:   s32 id, u32 size, size bytes, padding to 4
```

- **Sizes**: `func_0039EA88` of each table (8, each chunk's 8 and size rounded up to 4, 8).
- **Checksum**: `func_0039EAC0` is rac1's CRC-16 (register seeded with 0xEDB88320, polynomial
  0x1F45, most significant bit first, the low 16 bits kept); it returns 0 for data longer than
  the global section (14216). A stored 0 always fails (`func_0039EB68`).
- **Chunks**: the tables at 0x00332388 (global, 40 entries) and 0x00332618 (per level, 9) list
  `{address, size, id, status}`; a chunk is a copy of `size` bytes at `address`, a level's at
  `address + level * size` (`func_0039EBA8`; an id of 0x1770 would be written as zeros, none is
  listed). On load a chunk with an unknown id or another size counts as an error.

Proven: the files the game writes on a fresh card have these sizes and checksums (the
launcher's fixture `core/tests/fixtures/rac3/save-empty.bin` is slot 0 of one), and the game's
own Load page lists a slot made from it by the editor's code with level 1, 12,345 bolts and
223,380 ticks as "Veldin · 19.11.26 · 01:02 · 12345" and loads it (a hidden run of the port).

### The global chunks

Names in brackets are rac3-decomp's `config/names.tsv` (NTSC-U addresses, the same here).

| id               | size     | address                      | what                                                                        |
| ---------------- | -------- | ---------------------------- | --------------------------------------------------------------------------- |
| 0                | 4        | 0x1D545C                     | level the save resumes on (`level_current`), -1 in an empty slot            |
| 39               | 128      | 0x142660                     | progress block: +0 bolts (`player_bolts`); +0x32 a byte the slot list keeps |
| 3                | 4        | 0x1D5528                     | play time in ticks, 60 a second (the slot list: ticks / 216000 hours)       |
| 4                | 8        | 0x1D4BE0                     | clock when saved, BCD (stat, s, m, h, -, day, month, year)                  |
| 1003             | 4        | 0x1D4C38                     | a frame counter                                                             |
| 5                | 272      | 0x1426E0                     |                                                                             |
| 7                | 19       | 0x1D5468                     |                                                                             |
| 8                | 32       | 0x1D54B0                     |                                                                             |
| 9                | 624      | 0x1427F0                     | ammo, s32 per item (`item_ammo`)                                            |
| 10               | 156      | 0x142CA0                     | owned, u8 per item (`item_owned`)                                           |
| 11               | 156      | 0x142D40                     | "new" mark per item (`item_new`)                                            |
| 40               | 624      | 0x142DE0                     | experience, s32 per item, 1/32 points (`item_xp`)                           |
| 12               | 64       | 0x1D54D8                     | the vendor's list, u16 x 32 (`vendor_stock`)                                |
| 13               | 32       | 0x1D4C60                     |                                                                             |
| 14               | 60       | 0x1D5530                     | a byte per level (`func_0039F728` kind 1)                                   |
| 20               | 240      | 0x143050                     |                                                                             |
| 15               | 1824     | 0x143230                     | 0x72 entries of 16 bytes (`func_0039F728` kinds 4 and 5 test +0x0C)         |
| 16               | 4272     | 0x1A7E18                     |                                                                             |
| 17               | 516      | 0x1A7C10                     |                                                                             |
| 18               | 1872     | 0x1A74C0                     |                                                                             |
| 38               | 188      | 0x143950                     | options                                                                     |
| 30               | 160      | 0x1425C0                     | each item's row in the weapon table, u8 (`item_weapon_rows`)                |
| 32               | 28       | 0x1D4C40                     |                                                                             |
| 37               | 19       | 0x1D5498                     |                                                                             |
| 41               | 268      | 0x1A2018                     |                                                                             |
| 42               | 156      | 0x142A60                     | weapon mods, u8 per item (`item_flags`)                                     |
| 45               | 156      | 0x142B00                     |                                                                             |
| 43               | 256      | 0x142BA0                     | bit flags (`func_0039F728` kind 9)                                          |
| 44               | 4        | 0x1D54D0                     |                                                                             |
| 46               | 40       | 0x1D4BE8                     |                                                                             |
| 47               | 256      | 0x1A1F18                     |                                                                             |
| 1005             | 4        | 0x1D4CA0                     |                                                                             |
| 1010             | 800      | 0x1A8EC8                     |                                                                             |
| 1011             | 4        | 0x1D5598                     |                                                                             |
| 7002             | 32       | 0x1D4C80                     |                                                                             |
| 7003             | 12       | 0x1D55A0                     |                                                                             |
| 7004             | 6        | 0x1D55B0                     |                                                                             |
| 7005, 7006, 7007 | 192 each | 0x143B30, 0x143BF0, 0x143CB0 | read by the progress checks (`func_0039F3B8`)                               |

### The level chunks

| id   | size | address  | what                                                                |
| ---- | ---- | -------- | ------------------------------------------------------------------- |
| 3001 | 1    | 0x1D5570 | a byte per level (`func_0039F728` kind 11)                          |
| 3002 | 2048 | 0x1A9318 | per level, 2 KB (`func_0039F008` packs the level's block from here) |
| 3003 | 8    | 0x1BBB18 | 8 bytes per level (`func_0039F728` kind 8)                          |
| 3004 | 16   | 0x1BBC40 |                                                                     |
| 3005 | 1024 | 0x1BBE90 |                                                                     |
| 4002 | 4    | 0x143D70 |                                                                     |
| 4005 | 64   | 0x1C5290 |                                                                     |
| 4007 | 4    | 0x1A91E8 | frames played while the bolt rate counts (`bolt_rate_frames`)       |
| 4008 | 4    | 0x1A9280 | bolts collected meanwhile (`bolt_rate_bolts`)                       |

## The weapons

Chunk 30 holds each item's row in the weapon table (0xF0 bytes a row, in each story level's
program: rac3-decomp `include/structs/weapon.h`); a new game's chunk is 0, 1, 2 ... 159, so an
item's V1 row is its own number, and levelling up follows the rows' `+0x46`. The editor's list
(`WEAPONS` in `save_edit_rac3.rs`) is read from the PAL disc: Veldin's weapon table (`+0x04` the
name's text, `+0x8A` the full ammo) and the English text. The twenty weapons and the OmniWrench:

| item | V1            | item | V1                | item | V1               |
| ---- | ------------- | ---- | ----------------- | ---- | ---------------- |
| 9    | OmniWrench    | 71   | Spitting Hydra    | 143  | Qwack-O-Ray      |
| 16   | Plasma Coil   | 79   | Disc Blade Gun    | 151  | RY3NO            |
| 17   | Lava Gun      | 87   | Agents of Doom    | 21   | Miniturret Glove |
| 19   | Bouncer       | 95   | Rift Inducer      | 22   | Shield Charger   |
| 39   | Shock Blaster | 103  | Holo Shield Glove | 47   | N60 Storm        |
| 55   | Infector      | 111  | Flux Rifle        | 119  | Nitro Launcher   |
| 63   | Annihilator   | 127  | Plasma Whip       | 135  | Suck Cannon      |

Each has eight rows (V1 to V4, then the fifth version's own name and its Mega, Giga and Omega;
RY3NO five): the Shock Blaster's are rows 39 to 46 (Shock Blaster, V2, V3, V4, Shock Cannon,
Mega, Giga and Omega Shock Cannon), the Lava Gun's 17, 161, 174 to 179.

## What the editor changes

The bolts (progress +0, up to 9,999,999) and the ammo of an owned weapon (up to its version's full
ammo). Owning a weapon, its version and its experience are shown, not changed: a save the game
wrote in a level (none yet: the port loads no level) will show what else the game sets with them
(the weapon list's order, the quick select, the new mark). The write keeps every other byte and
the chunks' padding, makes each section's checksum again, reads the result back, backs the card
up first and refuses while the game runs with the card (rac1's guards, `save_edit.rs`).
