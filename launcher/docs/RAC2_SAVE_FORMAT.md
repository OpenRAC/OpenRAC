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
- **The level the save resumes on.** Moving a save needs the level's arrival state; not tried.
- **Weapon mods** (chunk 42), **platinum bolts spent** (chunk 5 +0x90), **ship upgrades** (the
  other bits of chunk 44), **crystals and moonstones** (+0x22, +0x24): places known, meanings not
  proven by an experiment.
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

| Planet         | Flag                       | Place          | From                                                                                                                                                                                                                       |
| -------------- | -------------------------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Maktar Nebula  | Arena: first challenge won | chunk 5 +0x31  | `func_L02_003F9B70` maps challenge 0 to it (0..13 to 0x31, 0x34, 0x32, 0x33, 0x35-0x38, 0x91, 0x39-0x3B, 0x92, 0x93); `func_L02_003FCCA0` adds 1 per win and gives item 38 on the first; each frame byte 2 = (0x31 != 0) |
| Endako         | Swingshot area passed      | chunk 5 +0x0D  | `func_L03_003F09C0`: at level start, with item 13 owned and 0x0D clear, places the hero at its cuboid; set when the hero enters its volume                                                                                  |
| Barlow         | Thermanator area passed    | chunk 5 +0x12  | `func_L04_00456628`: the same with item 39 and its own volume                                                                                                                                                              |
| Feltzin System | Space mission 1..4 won     | chunk 5 +0x61..+0x64 | `func_L05_0041DA20` loads them into the mission win counts at level start; `func_L05_00420788` adds 1 per win (bolt prize by count) and `func_L05_004222D8` stores them back, capped at 127                       |

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
"already said" marks or have no effect a player would name. Maktar's arena challenges other than
the first are counted at the bytes above, but the challenge each index is was not read from the
game's text. Notak's 0x13/0x14 (set together when a probe accepts the hero) and the tutorial's
byte 0 have no clear meaning. Chunk 3004's group states (bit 0x80 of `0x19B2E8[group + level *
16]`, set through `func_002A0430(group)` when a level's object group is done) depend on each
level's object data and are not named. Siberius has no constant use of chunk 5 in its own code.
