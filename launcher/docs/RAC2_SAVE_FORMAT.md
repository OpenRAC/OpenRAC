# Going Commando's save file (NTSC-U, SCUS_972.68)

What the launcher's Going Commando save editor (`core/src/save_edit_rac2.rs`,
`src/components/SaveEditorRac2.svelte`) reads and writes, and how each field was proven.

## The card

The port keeps the card in `<data home>/memcard/SCUS_972.68/BASCUS-97268RATCHET/`:
`BASCUS-97268RATCHET` (55,300 bytes), `icon.sys`, `static.ico` and `save0.bin` .. `save3.bin`.
The game accepts the folder only with exactly these files (card state 8 of `func_00298F00`, see
`IDENTIFIED.tsv` R2-L); the port hides dot files from its listing, so the editor's temporary
`.saveN.bin.editing` and the card's `.backup_*` folders beside the game folder do not count.

## The container

Each `saveN.bin` is 99,264 bytes, written by `SavePack` (0x0029ADA0) and read by `SaveUnpack`
(0x00298D28). It is rac1's container with 28 level sections:

```text
+0  u32 global section size  (7416)
+4  u32 level section size   (3280)
+8  the global section, then 28 level sections (level numbers 0..27)
section: u32 data size, u32 crc16(data), the chunks, then {s32 -1, u32 0}
chunk:   s32 id, u32 size, size bytes, zero padding to 4
```

- **Sizes**: the load fails ("Load failed") when either size differs from the tables'.
- **Checksum**: `SaveChecksum` (0x0029AEB0) is rac1's CRC-16: register seeded with 0xEDB88320,
  polynomial 0x1F45, most significant bit first, the low 16 bits kept. It returns 0 for data longer
  than the global section's size (7416), so rac1's limit (0x1800) cannot be reused: Going
  Commando's global data is 7408 bytes. A stored checksum of 0 always fails (`func_0029AF58`).
- **Chunks**: the save tables in the executable list `{address, size, id, status}`; a chunk is a
  copy of `size` bytes at `address`, a level's copy at `address + level * size`
  (`func_0029AF98`). Read from the retail ELF:

| id               | size | address                      | what (names from the decomp's symbols)                                        |
| ---------------- | ---- | ---------------------------- | ----------------------------------------------------------------------------- |
| 0                | 4    | 0x1A79F0                     | level the save resumes on (`g_playerProgress`), -1 in an empty slot           |
| 39               | 64   | 0x1A79F8                     | progress block, below                                                         |
| 3                | 4    | 0x1A7BC0                     | time played                                                                   |
| 4                | 8    | 0x1A7360                     | clock when saved, BCD (`g_saveClock`)                                         |
| 1003             | 4    | 0x1A7390                     |                                                                               |
| 5                | 205  | 0x1395B8                     | progress flags (+0x8D armour shop bits, +0x90 platinum bolts spent)           |
| 7                | 12   | 0x1A7A40                     | cheats (`g_cheatFlags`)                                                       |
| 8                | 32   | 0x1A7A60                     | skill points (`g_skillPointFlags`), 30 counted                                |
| 9                | 224  | 0x139688                     | ammo, s32 per item (`g_weaponAmmo`)                                           |
| 10               | 56   | 0x1A7AF8                     | owned, u8 per item (`g_inventoryOwned`)                                       |
| 11               | 56   | 0x1A7B30                     | "new" mark per item (`g_inventoryNewFlag`)                                    |
| 40               | 224  | 0x139868                     | weapon experience, s32 per item (`g_weaponXp`)                                |
| 12               | 32   | 0x1A7B68                     | weapon list order: low 6 bits item, 0x40 owned, 0xFF end (`g_inventoryOrder`) |
| 13               | 32   | 0x1A73B8                     | quick select, s32[8] item ids (`g_equippedItemSlots`)                         |
| 14               | 28   | 0x1A7BC8                     | levels on the ship's map, u8 per level (`g_abLevelAvailableFlags`)            |
| 20               | 112  | 0x139948                     | galactic map order, s32[28] (`g_anAvailableLevelOrder`)                       |
| 15               | 1824 | 0x139A28                     | map blips                                                                     |
| 16, 17, 18       |      | 0x18C958, 0x18C750, 0x18C4B0 |                                                                               |
| 38               | 40   | 0x1A7B90                     | options (sound +8, language +0x24)                                            |
| 30               | 80   | 0x139568                     | each item's record in the record table, u8 (the version: see below)           |
| 32               | 28   | 0x1A7398                     | what the hero has in hand or on, s32[7] (`g_itemEquipSlotTable`)              |
| 37               | 12   | 0x1A7A50                     | special skins                                                                 |
| 41               | 76   | 0x188610                     |                                                                               |
| 42               | 56   | 0x1A7A80                     | weapon mods bought, bits per item (`g_itemStateFlags`)                        |
| 45               | 56   | 0x1A7AB8                     |                                                                               |
| 43               | 256  | 0x139768                     | cinematics seen (`g_cinematicUnlockedFlags`)                                  |
| 44               | 4    | 0x1A7AF0                     | ship customisation (`g_shipCustomization`), bits 16..20 the paint job         |
| 46               | 40   | 0x1A7368                     |                                                                               |
| 47               | 256  | 0x188510                     | bestiary kill counts                                                          |
| 1005, 1010, 1011 |      |                              | deaths, help log and its position                                             |

Per level (table 0x00256368): 3001 visited (u8, 0x1A7BE8), 3002 the map (2048), 3003 platinum
bolts found (u8[4], 0x19B278, `g_platinumBoltFlags`), 3004 missions (16), 3005 (1024), 3007 (12),
4002, 4005 (64), 4007, 4008.

The progress block (chunk 39, 0x1A79F8):

| offset | address  | what                                                          |
| ------ | -------- | ------------------------------------------------------------- |
| +0x00  | 0x1A79F8 | bolts, u32 (`g_boltCount`)                                    |
| +0x04  | 0x1A79FC | raritanium, u32                                               |
| +0x08  | 0x1A7A00 | maximum health (`g_maxHealth`): the game sets it each tick    |
| +0x12  | 0x1A7A0A | challenge mode counter (`g_miscExtras`, +1 at each New Game+) |
| +0x14  | 0x1A7A0C | nanotech experience, u32                                      |
| +0x20  | 0x1A7A18 | armour worn, u8 0..4 (`g_equippedArmor`)                      |
| +0x30  | 0x1A7A28 | the ten Nanotech Boosts, u8 each                              |

The game works the maximum health out at each tick (`func_L00_002D5EB8`): 4, plus the health
steps reached by `nanotech experience >> 5` in the table `D_L00_00262260`, plus one per
non-zero boost byte.

## The record table and weapon versions

Each level's program holds a table of 0xE0-byte item records (in the port's RAM of level 3 at
0x266DA0; the executable's code names it `D_00239BA0`). Chunk 30 gives each item its record:
the item's own number for the first version, a later record once upgraded. Records link to
their next version at +0x4A and back at +0x4C; +0x6C is the experience step (compared with
`experience >> 5`; 0 none), +0x8E the full ammo. The editor's item list (ids, names, version
chains, ammo, steps) is that table: names match the records' moby classes and HUD icon ids to
the APRac2 randomizer's item list (`oclass_id`, `icon_id`), and the upgraded names to its list by
record number. Gadget and item ids are the same table's.

When `experience >> 5` reaches the step, `func_L00_002D5EB8` upgrades the weapon itself
(`func_00287EB0`, `func_00288508`) at its next tick in a level.

## How each field was proven

Hidden runs of the port (NTSC-U, `--cards` a scratch copy of a card, scratch `--settings`):
Load slot 0 from the title, then `OPENRAC_WRITE` (or `OPENRAC_GIVE`) 50 frames after control,
then pause, Options, Save, slot 0, Yes. The save then differs from a save made the same way
without the change only in the clock and play time, and in:

- **e1, writes**: raritanium 777 (0x1A79FC), nanotech experience 20,000 (0x1A7A0C), skill point
  0 (0x1A7A60), platinum bolts 0 and 1 of level 1 (0x19B27C), boosts 0 and 2 (0x1A7A28), armour 2
  (0x1A7A18), paint 5 (0x1A7AF0 = 0x50000), levels 0..3 available (0x1A7BC8), level 3 visited
  (0x1A7BEB), ammo of item 28 (0x1396F8): each landed at the place above, and the maximum health
  (+0x08) went from 4 to 8 (the game's own recount).
- **e2, the game's grants**: `OPENRAC_GIVE=bolts:123456,24`: bolts at +0x00 of chunk 39; the
  Seeker Gun (24) owned (chunk 10), marked new (chunk 11), ammo 25 (chunk 9), appended to the
  weapon list as 0x58 (chunk 12) and put in quick select slot 2 (chunk 13).

The editor's unit tests check the same places (`each_field_lands_where_the_game_keeps_it`,
`giving_a_weapon_writes_what_the_game_writes`), on a save from e2 (`tests/fixtures/rac2/`).

**Load proof** (box1, port 8b862810c): a scratch card edited through `write_save` (bolts 777,777,
raritanium 4,321, nanotech experience 20,000, boosts 1 and 2, armour 2, paint 5, skill points
1..3, Heli-Pack and Chopper given, Seeker Gun ammo 10, Lancer experience 100, Endako and Barlow
on the map, two platinum bolts on Oozla), then Load from the title: the hero gets control on
Oozla, and the RAM 900 frames later holds every value, with the game's reactions: maximum health
8, the armour pack loaded (0x1A7290 = 2), paint 5 loaded (0x152C30), the galactic map list 1, 3, 4.

## What the editor leaves out, and why

- **Choosing a weapon's version.** Setting a weapon's record to its upgraded one (Lancer to
  Heavy Lancer) crashed the port at the level start in the held Lancer's update
  (`func_L00_003A6C30`, moby class 1490). The version is shown, not edited.
- **Experience at or past the next step.** A save with the held Lancer at 9,000 (step 8,960)
  stopped at the level start in hero state 98 and never gave control. The game never saves that
  state, so the editor keeps experience under the step of the version in the save.
- **The level the save resumes on**, **weapon mods**, **platinum bolts spent**, **ship parts**,
  **crystals and moonstones**: now edited by the advanced fields, below.
- **Armour and skill point names**: the game's text was not read; armour is a level 0..4, skill
  points are numbered.
- **Platinum bolts per level**: each level has four places; how many a level really holds is not
  in the save, so all four are offered.
- **Gadgets the hero has on** (and the weapon in hand): chunk 32; taking them away is refused.

## The advanced fields

`core/src/save_edit_rac2_extra.rs` and `src/components/SaveEditorRac2Advanced2.svelte` (the "More"
part of the editor) read and write these, through the main editor's container and guards. Proven
on box1 (port 8b862810c, scratch cards under `~/RACSwarm/l5-scratch`) as above: a pause-menu save
after `OPENRAC_WRITE` (e1) against one without (b0), then a save written by `write_save` loaded from
the title, with `OPENRAC_STATE_DUMP` and a RAM dump after control.

| field                | place                       | type | what the game does with it                                    |
| -------------------- | --------------------------- | ---- | ------------------------------------------------------------- |
| resume level         | chunk 0                     | s32  | loads that level and starts the hero at its landing spot      |
| crystals             | chunk 39 +0x22 (`0x1A7A1A`) | s16  | Tabora's counter (`func_L08_003DB4A0`, the HUD)               |
| moonstones           | chunk 39 +0x24 (`0x1A7A1C`) | s32  | Grelbin's counter                                             |
| platinum bolts spent | chunk 5 +0x90 (`0x139648`)  | u8   | `func_002B0D70`: platinum bolts to spend = found less this    |
| weapon mods          | chunk 42, u8 per item       | bits | slot `s`: bought `1 << 2s`, switched on `2 << 2s` (see below) |
| ship parts           | chunk 44 bits 0..15         | bits | the Ship Shack's parts (below); bits 16..20 are the paint     |

**e1** (writes 50 frames after control, then the pause-menu save): crystals 25 and moonstones 13
landed at chunk 39 +0x22 and +0x24, platinum bolts spent 7 at chunk 5 +0x90 (the byte only), mod
bytes 01 04 15 3F at chunk 42 +0x1C..+0x1F, the ship word 0x2B at chunk 44. Nothing else changed
but the clock.

### Where Ratchet stands

The save keeps the level and **no position**. A save made after teleporting the hero to
(303.8, 421.6, 118) on Oozla (t1) differs from one made without moving (b0) only in the clock, the
play time and the statistics' timers (chunks 4, 1003, 17, 3007); loading t1 put the hero at
Oozla's start (293.84, 411.66, 115), as loading b0 does. There is no checkpoint either: every load
starts at the level's landing spot, so the editor offers the level only.

Changing chunk 0 alone and loading from the title reached control on every numbered level 0..20
and 22..26 (one hidden run each, `OPENRAC_STUCK` on, 7,000 frames, Start pressed through scenes):
the hero at the level's start, on foot, or in the ship (state 49) on the space levels (5, 10, 15, 25) and the orbits (22, 23, state 86). The Insomniac Museum (30) crashed the port at its load and
has no level section in the save, so it is not offered.

### Weapon mods

The mod vendor's list (the code at 0x002F7538) offers slot `s` of an owned weapon when
its record has a price (+0x98, +0x9C, +0xA0, in platinum bolts) and `1 << 2s` is clear; the weapon
menu (`func_00347468`) shows the slot's icon from the same bit. The names are the game's text
(0x31CF Shock Mod, 0x31CE Acid Mod, 0x31D0 Lock On Mod, read from a level's RAM). The lock-on
targeting (`func_L00_002CB038`) tests `0x20`, the bit above Lock On's: the editor writes both bits
of a slot together, the mod bought and on. The prices are each base record's, read from level 1's
record table (0x2637A0 in the port's RAM):

| weapon           | Shock | Acid | Lock On |
| ---------------- | ----- | ---- | ------- |
| Chopper          |       | 2    | 1       |
| Pulse Rifle      | 1     |      |         |
| Seeker Gun       | 1     |      | 1       |
| Hoverbomb Gun    |       | 3    | 1       |
| Blitz Gun        |       | 2    | 1       |
| Minirocket Tube  |       | 2    | 2       |
| Plasma Coil      |       | 2    | 1       |
| Lava Gun         |       | 2    | 1       |
| Lancer           | 2     |      | 3       |
| Spiderbot Glove  |       | 2    |         |
| Bouncer          |       | 3    | 1       |
| Miniturret Glove | 2     |      |         |
| Gravity Bomb     |       | 2    | 1       |
| RYNO II          |       |      | 1       |

Chunk 45 (0x1A7AB8) holds, per item, which slots the vendor may offer (`1 << s`); the editor
leaves it alone.

### Ship parts

From the Ship Shack's tables (`func_002F7738`, text ids at 0x0026D528, masks at 0x0026D648,
values at 0x0026D6D8) and the ship's model (`func_002E6178`):

| bits   | part      | values                                                               |
| ------ | --------- | -------------------------------------------------------------------- |
| 0..1   | nose      | 0 Standard, 2 Scoop, 1 Split                                         |
| 2..3   | wings     | 0 Standard, 4 Hi-Lift, 8 Heavy Ordinance                             |
| 4..5   | engine    | 0x10 Triple Boost Acceleration Engine (the shop's second entry 0x20) |
| 6      | lasers    | Fusion Laser Cannons                                                 |
| 7..8   | mines     | 0x80 Electro Mine Launcher, 0x100 Mega-Mine Launcher                 |
| 9      | missiles  | Fast-lock Missile Launcher (0: the Standard one)                     |
| 10..11 | torpedoes | 0x400 Torpedo Launcher, 0x800 Multi-Torpedo Launcher                 |
| 12     |           | Nuclear Detonation Device                                            |
| 13     |           | Hyperspace Warp System                                               |
| 14..15 | shields   | 0x4000 Advanced, 0x8000 Ultra-Charge Shielding System                |

The game gives a skill point for 0xBB50 (every part at its best, `func_L00_002A67F8`). A two-bit
part at 3 is not in the shop: loading a save with the nose at 3 (ls2) or the wings at 3 (ls4)
crashed the port at the title's Load page, so the editor shows such a value as unknown and keeps
it unless it is changed. 0xBB59 (every part at its last choice) loaded (ls1), and on the Feltzin
System (level 5, lp5 against lq5 with the ship's own parts) the space level's weapon table
(`func_L05_00379968`, 0x2A1800 in its RAM) followed every bit: lasers +0x140/+0x158, mines +0x90/+0x1D0,
missiles +0x168/+0x180, torpedoes +0x68/+0x1A8, the bomb +0xB8, the warp +0xE0, shields
+0x1F8/+0x230/+0x248 and the engine +0x208/+0x220 set, the standard parts' +0x0, +0x28, +0xF0,
+0x118 cleared. The Ship Shack's
paint list has 17 entries (values 0..16).

**Load proof** (box1): the Oozla save, two platinum bolts found (the main editor), then through
`write_save`: level 3 (Endako), crystals 25, moonstones 13, platinum bolts spent 2, ship 0xBB59,
Lancer Shock and Lock On (0x33), Chopper Acid (0x0C). Loaded from the title: control on Endako at
its start (300.16, 227.28, 128), and the RAM 1,900 frames later holds every value.
