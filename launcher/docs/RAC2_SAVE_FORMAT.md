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
bolts found (u8[4], 0x19B278, `g_platinumBoltFlags`), 3004 object-group states (16, 0x19B2E8),
3005 moby counts (1024, 0x19B4A8: below, "Electrolyzer puzzles"), 3007 (12),
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

### The four versions

+0x04 of a record is its version, 0 to 3. The sixteen weapons that upgrade by experience have
four records (the Lancer 30, 60, 79, 80):

| version | how the game gets there                                                                                        | experience after        | ammo             |
| ------- | -------------------------------------------------------------------------------------------------------------- | ----------------------- | ---------------- |
| V1      | given                                                                                                          | under the V1 step       | V1's full        |
| V2      | V1's step reached (`func_00287EB0`)                                                                            | at least the step       | set to V2's full |
| V3      | bought in challenge mode, the Mega upgrade (vendor list `func_002F6608`, purchase 0x002F8F78, `func_002887B0`) | 0, then under V3's step | set to V3's full |
| V4      | V3's step reached, in challenge mode                                                                           | at least the step       | set to V4's full |

Challenge mode is progress +0x12 (`func_00288D58` tests it non-zero; `func_002DE990`, which
starts it, adds 1). The vendor offers the V3 only while it is set, for an owned weapon on a record
with no experience step left (V2). The Bomb Glove, Visibomb Gun, Decoy Glove, Tesla Claw and
Walloper have two records and no step: their second is the challenge-mode purchase. The Clank
Zapper's second comes by experience. The Sheepinator is left out of the vendor's upgrade; its V3
comes when challenge mode starts (V1 to V3, V2 to V4) or when it is given in challenge mode. The
load (`SaveUnpack`) checks only the sizes and checksums, so the editor offers V3 and V4 (and the
two-record purchases) on every save (outside challenge mode a V3 still earns experience, which
`func_002A6C58` adds with no mode test, but never steps to V4: the tick `func_L00_002D5EB8` skips
the row; a V3 at exactly V4's step stayed V3 800 frames in a hidden run; V4 has no step), and keeps experience in each
version's range: `core/src/save_edit_rac2.rs`, "Weapon versions".

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

- **The version of the weapon in hand.** Setting the held weapon's record to its upgraded one
  (Lancer to Heavy Lancer) crashed the port at the level start in the held Lancer's update
  (`func_L00_003A6C30`, moby class 1490). The editor changes the version of the other weapons
  only (below), with the experience and ammo the game's own upgrade leaves.
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

## Advanced: progress flags (L-2)

The editor's Advanced switch (off whenever a slot opens, with a warning) shows the story and
progress flags, grouped by planet (`launcher/core/src/rac2_flags/`: `planets_a.rs` levels 0-8, `planets_b.rs` 9-16, `planets_c.rs` 17-30 and flags of no one planet; each flag is `Proven` (honoured at load) or `FromCode` (from the code, shown as unverified)). Other Advanced sections (the saved level and position, weapon mods,
platinum bolts spent, ship upgrades) are a separate component and module, and their notes a
separate section of this document.

**Method.** Two hidden runs on box1 from the same start, one with the event and one without, then
a RAM dump at the same frame; `ramdiff.py` compares the dumps inside the save's chunks (every
chunk is a copy of RAM by the save tables, so this is the save's own difference). Port faaacf206
(it has the Maktar arena sound-bank fix), format 20 data.

| Planet        | Flag                   | Place                                         | Proof                                                                                                                                                                                               |
| ------------- | ---------------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Maktar Nebula | Arena entered          | chunk 5 (0x1395B8) byte 5                     | entering the arena (`--level 2`, teleport 485.6 700.1 123.0, `OPENRAC_WRITE=c300:1A7904:2`) set it 0 → 1; `func_L02_003F9C10` opens the gate and plays the first-visit scene only while it is clear |
| Maktar Nebula | Arena intro scene seen | chunk 43 (0x139768) bit 331 (byte 0x29, 0x08) | set by the same entry                                                                                                                                                                               |

The same entry also set chunk 5 byte 0x9C to 1, the "once" bit field at chunk 5 +0xA7 (bit 2,
set through `func_00294E48`) and bit 7 of level 2's byte 7 in chunk 3004 (the per-level object
group states `D_0019B2E8[group + level * 16]`); their meanings are not proven, so they are not
offered.

**Load proof.** The Oozla fixture moved to Maktar (chunk 0 = 2, chunk 14 byte 2 = 1) was loaded
twice, without and with the Arena entered flag, and the hero sent into the arena the same way.
Without it the arena's intro played (bit 331 set at frame 5900); with it the intro did not play
(bit 331 stayed clear): the game honours the edited flag.

**Not yet named.** Per-level chunk 3004 (16 object-group states per level, which move as missions
are done: level 0 after the tutorial `bf 83 0f 80 00 80`), chunk 5's other bytes (indexed by
constants in each level's code, e.g. 0x83, 0x86, 0x89, 0x9F on Damosel, Joba, Endako), Joba's
and Maktar's arena challenge results (chunk 5 at the index `func_L11_0043E108` and
`func_L02_003F9B70` give per challenge), races, and the rest of the cinematics bits. Each needs
its own before/after experiment.

### Planets 0 to 8 (L-6a)

`rac2_flags/planets_a.rs`. Found by scanning each level's assembly (levels 0-8) for constant
offsets into chunk 5 (0x1395B8), chunk 43 (0x139768) and chunk 3004 (0x19B2E8), then reading the
decompiled C of every level-only function that uses one.

**The cinematics bits.** The scene players (`func_002EB7E8`, `func_002F53C8`) set
`*(u32 *)(0x139768 + (id >> 2) * 4) |= 1 << (id & 31)` for scene ids under 0xB1: the word index
is `id >> 2`, not `id >> 5`, so scene `id` is bit `(id >> 2) * 32 + (id & 31)` of the chunk. The
Maktar arena intro, bit 331, is scene 43. The level code asks for scenes through a request
global, not by constant ids, so the other bits are not named here.

| Planet         | Flag                       | Place                | From                                                                                                                                                                                                                     |
| -------------- | -------------------------- | -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Maktar Nebula  | Arena: first challenge won | chunk 5 +0x31        | `func_L02_003F9B70` maps challenge 0 to it (0..13 to 0x31, 0x34, 0x32, 0x33, 0x35-0x38, 0x91, 0x39-0x3B, 0x92, 0x93); `func_L02_003FCCA0` adds 1 per win and gives item 38 on the first; each frame byte 2 = (0x31 != 0) |
| Endako         | Swingshot area passed      | chunk 5 +0x0D        | `func_L03_003F09C0`: at level start, with item 13 owned and 0x0D clear, places the hero at its cuboid; set when the hero enters its volume                                                                               |
| Barlow         | Thermanator area passed    | chunk 5 +0x12        | `func_L04_00456628`: the same with item 39 and its own volume                                                                                                                                                            |
| Feltzin System | Space mission 1..4 won     | chunk 5 +0x61..+0x64 | `func_L05_0041DA20` loads them into the mission win counts at level start; `func_L05_00420788` adds 1 per win (bolt prize by count) and `func_L05_004222D8` stores them back, capped at 127                              |

**Load proof** (box1, the same port and data as above, `OPENRAC_DUMP_RAM` at frame 5000). The
Oozla fixture moved to each planet (chunk 0 and chunk 14), loaded from the title twice, without
and with the flag:

- Maktar, 0x31 set with byte 2 left clear in the save: the game set byte 2 to 1 (0 without), so
  it reads the arena count.
- Feltzin, 0x61 = 3: the level's first mission count (0x1B1350) loaded as 3 (0 without), and its
  first-mission switch (0x1B13F4) was 0 (1 without).
- Endako, Swingshot owned, 0x0D clear: control at frame 1675 at (407.8, 232.8, 160.0); with 0x0D
  set, control at 1986 at the usual (300.2, 227.3, 128.0).

These three are `Proven`; Barlow and Feltzin missions 2 to 4 follow the same code and are
`FromCode`.

**Left out.** Oozla, Maktar, Endako, Barlow and Tabora's other constant bytes (0x75, 0x7B, 0x9B,
0x8F, 0x04, 0x09, 0x78, 0xA2, 0x74, 0x75, 0xA4, 0x19, 0x1B, 0x1C) are help-voice and hint
"already said" marks or have no effect a player would name. Notak's 0x13/0x14 (set together when a probe accepts the hero) and the tutorial's
byte 0 have no clear meaning. Chunk 3004's group states (bit 0x80 of `0x19B2E8[group + level *
16]`, set through `func_002A0430(group)` when a level's object group is done) depend on each
level's object data and are not named. Siberius has no constant use of chunk 5 in its own code.

### Levels 9 to 16 (L-6b)

`rac2_flags/planets_b.rs`. Found by reading every access to chunk 5 (0x1395B8) and chunk 43
(0x139768) in the levels' own code: in the split assembly (`rac2-decomp/build/<level>/asm`) the
accesses are `lui 0x14` plus a negative offset or a register based on `%lo(D_1395B8)`, so each
byte's index is exact; the C in `games/rac2/decomp/src/levels/<level>` then says what the code
does with it.

**Load proofs** (box1, hidden, scratch settings and cards, port binary of L-2's round, format 20
data). The Oozla fixture card moved to the planet (chunk 0 = level, chunk 14 byte level = 1), the
flags then set through `write_save` (an ignored scratch test calling the editor's own write
path), loaded from the title (`OPENRAC_PRESS=300:4000:10,600:40:5,700:4000:5,900:4000:5`), and
compared with the same card without the flags. A moby's state is read from a RAM dump
(`OPENRAC_DUMP_RAM`) by finding its update function's address at moby +0x64 (state +0x20,
class +0xAA).

| Planet       | Flag                             | Place                 | Code                                                                                                                                                                                                             | Proof                                                                                                                                                                                               |
| ------------ | -------------------------------- | --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Dobbo        | Joba coordinates: task given     | chunk 5 byte 0x1D     | `func_L09_0040A2E8` (class 3678) sets it with 0x2D after its first scene; set, the character starts in state 3, not 1                                                                                            | state 3 with it, 1 without (frame 2650)                                                                                                                                                             |
| Dobbo        | Joba coordinates: task done      | chunk 5 byte 0x2E     | the same character: in state 3 and set, it plays its hand-over scene and calls `func_002AD7B8(11)` (Joba on the map)                                                                                             | from the code                                                                                                                                                                                       |
| Dobbo        | Hrugis Cloud coordinates offered | chunk 43 bit 768      | `func_L09_00408478` (class 3600, sells Hrugis Cloud's place for bolts, `func_002AD7B8(10)`): set, it starts in the sale states 6/7                                                                               | state 7 with it, 1 without                                                                                                                                                                          |
| Hrugis Cloud | Ship challenge 1 to 4 won        | chunk 5 0x66..0x69    | wins per challenge: `func_L10_00421288` copies them to 0x1B1390..0x1B139C at the start, `func_L10_00425888` back after a win; challenge 1 at 0 sets the first-visit word 0x1B1444                                | 0x66..0x68 = 1: the counters load as 1 1 1 0 and 0x1B1444 stays 0 (1 without); challenge 4 from the code                                                                                            |
| Joba         | Arena entered                    | chunk 5 byte 0x4A     | `func_L11_0043E1A0`, as Maktar's byte 5: clear, the entry opens the gate and plays the first-visit scene                                                                                                         | with the arena-fix binary of L-2 and `OPENRAC_WRITE=c300:1A7904:2` (the menu's "enter"): clear, the game set it 0 → 1 at frame 2469 and the arena (class 3466) was in state 8 at 4100; set, state 1 |
| Joba         | Arena challenge 1 to 13 won      | chunk 5 61..72, 75    | wins per challenge, the byte from `func_L11_0043E108` (challenge 9 is 71, 10 is 69, 11 is 70, 13 is 75); `func_L11_004427B8` adds 1 per win; the menu (`func_L11_00445990`) and the arena's states read them     | challenge 1 (61) set with Arena entered: state 7 at 4100 (1 without); the others from the code                                                                                                      |
| Gorn         | Ship challenge 1 to 4 won        | chunk 5 0x6B..0x6E    | as Hrugis: `func_L15_0042DCE0` copies them to 0x1B1520..0x1B152C, `func_L15_00432008` back                                                                                                                       | 0x6B, 0x6C = 1: the counters load as 1 1 0 0 (0 0 0 0 without); 3 and 4 from the code                                                                                                               |
| Snivelak     | Boss beaten                      | chunk 5 byte 0x83 = 2 | 0 not met, 1 fight under way (`func_L16_00403900`), 2 beaten (`func_L16_00412E70`, which also puts Smolg on the map); at 2 the boss (`func_L16_0040ECC0` through `func_L16_00410618`, class 3961) removes itself | state 253 (removed) with it, 1 without                                                                                                                                                              |

Joba's and Maktar's challenge names come from the game's text. `func_L11_00443CE0` and
`func_L02_003FE1B0` turn the menu's selection into a text id, the same index the win code passes
to `func_L11_0043E108` and `func_L02_003F9B70`. Index 0 and 1 of Joba win the Gravity Boots
(item 0x13) and the Infiltrator (item 0x33), as their names say.

**Skipped** (place known, meaning or effect not): Dobbo 0x1E, 0x1F, 0x20, 0x2D (written by the
level's controller and the Hrugis seller, read only in passing), 0x79/0x7A/0x7D (hint counters:
every fifth visit plays a help line), 0xA3 (once per arrival, then an autosave), cinematic bits
767, 771, 807; Joba 0x49 (the arena menu's last choice) and 0x57, cinematic words +0x68, +0x70;
Todano 0x22 (set when a counted object dies), 0x23 (hero entered an area), cinematic +0x74;
Boldan 0x24..0x28 (five "hero reached area" bytes no level code reads), 0x7E; Aranos Prison 0x29,
0x2B (the flight's arrival scene to the prison checks it), 0x78, cinematic +0x7C; the per-level
object-group bits (chunk 3004) that each level reads. Each needs its own experiment.

## Advanced: progress flags, Smolg to the Museum (L-6c)

`core/src/rac2_flags/planets_c.rs`. Found statically: a scan of every level's disassembly
(`build/<level>/asm` of rac2-decomp) for loads and stores into chunk 5 (0x1395B8) and chunk 43
(0x139768), and for the scene queue calls (`func_00288AA0` with a constant scene id), then the C
of each function that hits. None is load-proven yet, so all are FromCode.

**Scene bits.** The game sets scene `id`'s bit as bit `id & 31` of the word `id >> 2`
(`func_002EB7E8`, `func_002F53C8`, `func_00288B38`; ids under 0xB1 only), so its place is byte
`(id >> 2) * 4 + (id & 31) / 8`, bit `id & 7`: Maktar's arena intro, proven above at bit 331, is
scene 43. Scene ids are numbered by level: `func_002EF010` subtracts the first id of the level's
scenes, from the table at 0x1AD030 (indexed by level number; Smolg 137, Damosel 143, Grelbin 150,
Yeedil 155, Dobbo Orbit 163, Damosel Orbit 164, Ship Shack 165, Wupash 166, Jamming Array 167-168).
Data-driven scene triggers (`func_L17_00417DB0` and its copies in other levels) skip a scene whose
bit is set, so clearing a bit can replay a scene where its trigger is not yet spent.

| Planet        | Flag                           | Place              | Code                                                                                                                                          |
| ------------- | ------------------------------ | ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------- |
| Smolg         | Hypnomatic part found          | chunk 5 byte 0x84  | set by `func_L17_0041ED58`; the maker (`func_L18_00440718`), the Grelbin trader and the inventory (`func_00337210`) count 0x84, 0x86, 0x89    |
| Smolg         | Damosel coordinates seller met | chunk 43 scene 137 | `func_L17_0041E208` (sells level 18 on the map, `func_002AD7B8(18)`): plays 137 first unless its bit is set                                   |
| Damosel       | Hypnomatic part found          | chunk 5 byte 0x86  | `func_L18_0043FC70`: the pickup sparkles and is taken (sets 0x86) only while clear                                                            |
| Damosel       | Hypnomatic maker met           | chunk 43 scene 144 | `func_L18_00440718`: introduction 144 unless seen; with the three parts and the price sells item 55 (scene 145)                               |
| Damosel       | Coming back from Giant Clank   | chunk 5 byte 0x9F  | set with 0x87 by the launch pad (`func_L09_00410200`, scene 146); `func_L18_003F90A0` at the level start moves the hero, clears it, plays 149 |
| Grelbin       | Hypnomatic part traded         | chunk 5 byte 0x89  | `func_L19_0042B058` sets it after scene 153; with it the trader goes to his moonstones-for-bolts states                                       |
| Grelbin       | Moonstone trader met           | chunk 43 scene 152 | `func_L19_0042B058` state 0: without 152 or 154 seen the trader stays inactive                                                                |
| Grelbin       | First moonstone trade made     | chunk 43 scene 154 | same function: with 154 seen he offers the part (scene 153)                                                                                   |
| Damosel Orbit | Giant Clank battle won         | chunk 5 byte 0xA1  | set with 0x88 by `func_L23_0041D9B0`; `func_L18_003F90A0` gives item 5 after the return scene if it is not owned                              |

**Left out.** No reader found in any level or the executable: 0x85 (a Smolg switch,
`func_L17_0041F8F0`), 0x87 (launch pad used on Damosel), 0x88 (with 0xA1). Not named from the
code: Damosel 0x94 and 0xA0 (bit fields of the level's controllers), 0xA6 (set by
`func_L18_00428F08` in Damosel, Grelbin and Yeedil), Jamming Array's 0x01, 0x71 and 0xA5
(`func_L02_00404410`, shared with Maktar), Yeedil's scenes 160 and 161, the per-level object-group
states (chunk 3004), and the bits of scenes under 32 and from 169 (no one planet: no "Story scenes"
group yet). Dobbo Orbit, Ship Shack, Wupash Nebula and the Museum have no constant-indexed story
byte or scene read in their own code. Space combat results (Feltzin 0x61..0x64, Hrugis 0x66..0x69,
Gorn 0x6B..0x6E) are counts, written by those levels, not flags.

## Advanced: Electrolyzer puzzles (L-7)

Each planet's Electrolyzer puzzles, one flag a board, in the planet files of `rac2_flags/` (made by
`rac2_flags::electrolyzer`). Clearing one puts the board back unsolved and undoes what solving it
saved for the thing it opens, so the puzzle can be played again (asked for after Dobbo's could not
be replayed: the Dobbo flags before it were only the coordinate sellers').

**Where the game keeps a solved board.** The board is class 2198, `func_L02_003C26F0`, the same
code in levels 2, 3, 4, 9, 12, 17, 20 and 30 (`moby-dispatch.tsv`). It keeps no story byte of its
own. When solved (its state 8) it

1. raises its moby count: `D_0019B4A8[(uid >> 1) + level * 0x400]`, three bits of a nibble per moby
   id (`uid`, moby +0xB2), the low nibble for an even id and the high one for an odd one, up to 7.
   This array is the per-level chunk **3005** (the per-level save table at 0x00256368 lists
   `{0x19B4A8, 1024, 3005}`); it is the same count the game raises when an enemy dies or a crate
   breaks (`func_002A6C58`), whose bit 3 is not part of it;
2. marks its object group done, `func_002A0430(moby +0xB0)`: bit 7 of chunk **3004**'s byte `group`
   (`D_0019B2E8[group + level * 16]`), when the board has a group (255 is none);
3. sets off its one or two linked mobys (pvar +0x10, +0x14): `func_L00_003294F0(moby, 1)` sets bit 2
   of moby +0xBE, which `func_002B38B8` reads.

At the level start (state 0) the board reads only its count (and the level's own, unsaved
`D_L02_0021F4F0` table): non-zero, it goes straight to state 5 (solved) and sets off its linked
mobys again; zero, it goes to state 1, ready to play. The linked mobys then do what they do when set
off, and some of them save it: those places are the flag's too.

**The boards** (moby ids and links read from RAM dumps of `--level N` runs, the moby table at the
pointer 0x18AA38, 0x100 bytes a moby; "nearest" is from the hero's start):

| Planet        | Flag                     | Board (uid, group) | Linked moby                                                                                                                                                                                                                    | Places                                                                                        |
| ------------- | ------------------------ | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------- |
| Maktar Nebula | Electrolyzer puzzle 1    | 593, group 6       | two class 2736 platforms (`func_L02_003EA4E0`); their pvar +4 is 1, so set off they also store chunk 5 byte 3 and autosave (`func_0029B3E0`)                                                                                   | 3005 byte 296 high, 3004 byte 6 bit 7, chunk 5 byte 3                                         |
| Maktar Nebula | Electrolyzer puzzle 2    | 40, group 9        | two class 2736 platforms, pvar +4 0                                                                                                                                                                                            | 3005 byte 20 low, 3004 byte 9 bit 7                                                           |
| Endako        | Electrolyzer puzzle 1    | 217                | the factory door, class 2467 (`func_L03_003CC238`): opens while set off                                                                                                                                                        | 3005 byte 108 high                                                                            |
| Endako        | Electrolyzer puzzle 2, 3 | 437, 466           | class 2377 (`func_L03_003C6A28`): a control the hero takes over, live only once set off                                                                                                                                        | 3005 byte 218 high; byte 233 low                                                              |
| Endako        | Electrolyzer puzzle 4    | 20                 | class 2568 (`func_L03_003D19C8`): set off, asks for scene 50 unless chunk 5 byte 7 is set; the scene's end (`func_L03_003D2018`) sets byte 7                                                                                   | 3005 byte 10 low, chunk 43 byte 0x32 bit 2 (scene 50), chunk 5 byte 7                         |
| Barlow        | Electrolyzer puzzle      | 561                | class 3725 (`func_L04_00441178`) stores chunk 5 byte 0x0F; class 4439 (`func_L04_00455520`), watching it, marks its group 7 done, plays scene 56 and removes itself                                                            | 3005 byte 280 high, chunk 5 byte 0x0F, 3004 byte 7 bit 7, chunk 43 byte 0x3B bit 0 (scene 56) |
| Dobbo         | Electrolyzer puzzle      | 182, group 5       | the Hrugis Cloud coordinates seller, class 3600 (`func_L09_00408478`): after his first scene (scene 95, chunk 43 bit 767) he waits in state 3 until set off, then stores chunk 5 byte 0x1F and offers the sale (scene 96 next) | 3005 byte 91 low, 3004 byte 5 bit 7, chunk 5 byte 0x1F                                        |
| Todano        | Electrolyzer puzzle      | 665, group 1       | the dam's lift, class 4056 (`func_L12_0041EA18`): runs only while set off                                                                                                                                                      | 3005 byte 332 high, 3004 byte 1 bit 7                                                         |
| Smolg         | Electrolyzer puzzle      | 32                 | a lift, class 2215 (`func_L17_003C3330`, `needsTrigger`)                                                                                                                                                                       | 3005 byte 16 low                                                                              |
| Yeedil        | Electrolyzer puzzle      | 26                 | a lift, class 4675 (`func_L20_00446FA8`): moves only while set off                                                                                                                                                             | 3005 byte 13 low                                                                              |

The Museum (level 30) has the board's code but no level section in the save. No other code stores the
story bytes above: a scan of every level's and the executable's disassembly for `lui 0x14` plus
their offsets (-0x6A45, -0x6A41, -0x6A39, -0x6A29) finds only these stores.

**Proof** (Mac, hidden, port binary of f571b4b47 and its format data). For each board, a `--level N`
run (`OPENRAC_DIRECT`) with `OPENRAC_WRITE` holding the board's count at 1 (and its group bit and
the linked mobys' story bytes) from frame 1, so the level starts with them as a load leaves them,
against the same run without; a RAM dump at frame 2400. With the count every board started solved
(state 5) and its linked mobys were set off (+0xBE bit 2); without it every board was in state 1
and nothing was set off. The save chunks of the two dumps (every chunk of both save tables, less
the clocks) differ only in the places of the table: Dobbo's seller and the Todano lift set off,
Barlow's watcher gone (state 253) with scene 56's bit and group 7 set, Endako's 2568 asking for
scene 50 (its bit set). The rest of the differences are left out: Maktar's first board's platforms
autosave, which writes the level map (chunks 3002 and 15) and chunk 5 +0xA7 bit 2; with Todano's
group 1 done, members of the group set bit 3 of their own nibbles (not counts). A save edited by
the editor writes the same bytes (`an_electrolyzer_puzzle_is_reset_with_what_it_opened`). Loading a
card from the title was not scripted on the Mac (the title presses of L-6b did not reach the load
there), so the proof is of the level start, which is what a load ends in.

**To replay Dobbo's puzzle**: Advanced, Dobbo, clear "Electrolyzer puzzle solved", write, then
load the save (the save must not be on Dobbo while Dobbo is loaded in a running game). The board
plays again; the seller waits for it if his first scene was seen, and "Hrugis Cloud coordinates
offered" (scene 96, chunk 43 bit 768) cleared too sends him back to before the offer.

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
