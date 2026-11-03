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

| id | size | address | what (names from the decomp's symbols) |
|---|---|---|---|
| 0 | 4 | 0x1A79F0 | level the save resumes on (`g_playerProgress`), -1 in an empty slot |
| 39 | 64 | 0x1A79F8 | progress block, below |
| 3 | 4 | 0x1A7BC0 | time played |
| 4 | 8 | 0x1A7360 | clock when saved, BCD (`g_saveClock`) |
| 1003 | 4 | 0x1A7390 | |
| 5 | 205 | 0x1395B8 | progress flags (+0x8D armour shop bits, +0x90 platinum bolts spent) |
| 7 | 12 | 0x1A7A40 | cheats (`g_cheatFlags`) |
| 8 | 32 | 0x1A7A60 | skill points (`g_skillPointFlags`), 30 counted |
| 9 | 224 | 0x139688 | ammo, s32 per item (`g_weaponAmmo`) |
| 10 | 56 | 0x1A7AF8 | owned, u8 per item (`g_inventoryOwned`) |
| 11 | 56 | 0x1A7B30 | "new" mark per item (`g_inventoryNewFlag`) |
| 40 | 224 | 0x139868 | weapon experience, s32 per item (`g_weaponXp`) |
| 12 | 32 | 0x1A7B68 | weapon list order: low 6 bits item, 0x40 owned, 0xFF end (`g_inventoryOrder`) |
| 13 | 32 | 0x1A73B8 | quick select, s32[8] item ids (`g_equippedItemSlots`) |
| 14 | 28 | 0x1A7BC8 | levels on the ship's map, u8 per level (`g_abLevelAvailableFlags`) |
| 20 | 112 | 0x139948 | galactic map order, s32[28] (`g_anAvailableLevelOrder`) |
| 15 | 1824 | 0x139A28 | map blips |
| 16, 17, 18 | | 0x18C958, 0x18C750, 0x18C4B0 | |
| 38 | 40 | 0x1A7B90 | options (sound +8, language +0x24) |
| 30 | 80 | 0x139568 | each item's record in the record table, u8 (the version: see below) |
| 32 | 28 | 0x1A7398 | what the hero has in hand or on, s32[7] (`g_itemEquipSlotTable`) |
| 37 | 12 | 0x1A7A50 | special skins |
| 41 | 76 | 0x188610 | |
| 42 | 56 | 0x1A7A80 | weapon mods bought, bits per item (`g_itemStateFlags`) |
| 45 | 56 | 0x1A7AB8 | |
| 43 | 256 | 0x139768 | cinematics seen (`g_cinematicUnlockedFlags`) |
| 44 | 4 | 0x1A7AF0 | ship customisation (`g_shipCustomization`), bits 16..20 the paint job |
| 46 | 40 | 0x1A7368 | |
| 47 | 256 | 0x188510 | bestiary kill counts |
| 1005, 1010, 1011 | | | deaths, help log and its position |

Per level (table 0x00256368): 3001 visited (u8, 0x1A7BE8), 3002 the map (2048), 3003 platinum
bolts found (u8[4], 0x19B278, `g_platinumBoltFlags`), 3004 missions (16), 3005 (1024), 3007 (12),
4002, 4005 (64), 4007, 4008.

The progress block (chunk 39, 0x1A79F8):

| offset | address | what |
|---|---|---|
| +0x00 | 0x1A79F8 | bolts, u32 (`g_boltCount`) |
| +0x04 | 0x1A79FC | raritanium, u32 |
| +0x08 | 0x1A7A00 | maximum health (`g_maxHealth`): the game sets it each tick |
| +0x12 | 0x1A7A0A | challenge mode counter (`g_miscExtras`, +1 at each New Game+) |
| +0x14 | 0x1A7A0C | nanotech experience, u32 |
| +0x20 | 0x1A7A18 | armour worn, u8 0..4 (`g_equippedArmor`) |
| +0x30 | 0x1A7A28 | the ten Nanotech Boosts, u8 each |

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
8, the armour pack loaded (0x1A7290 = 2), paint 5 loaded (0x152C30), the galactic map list 1, 3,
4.

## What the editor leaves out, and why

- **Choosing a weapon's version.** Setting a weapon's record to its upgraded one (Lancer to
  Heavy Lancer) crashed the port at the level start in the held Lancer's update
  (`func_L00_003A6C30`, moby class 1490). The version is shown, not edited.
- **Experience at or past the next step.** A save with the held Lancer at 9,000 (step 8,960)
  stopped at the level start in hero state 98 and never gave control. The game never saves that
  state, so the editor keeps experience under the step of the version in the save.
- **The level the save resumes on.** Moving a save needs the level's arrival state; not tried.
- **Weapon mods** (chunk 42), **platinum bolts spent** (chunk 5 +0x90), **ship upgrades** (the
  other bits of chunk 44), **crystals and moonstones** (+0x22, +0x24): places known, meanings not
  proven by an experiment.
- **Armour and skill point names**: the game's text was not read; armour is a level 0..4, skill
  points are numbered.
- **Platinum bolts per level**: each level has four places; how many a level really holds is not
  in the save, so all four are offered.
- **Gadgets the hero has on** (and the weapon in hand): chunk 32; taking them away is refused.
