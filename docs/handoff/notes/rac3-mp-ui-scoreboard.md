# RAC3 PAL multiplayer: the end-of-match scoreboard (for the native MP UI)

2026-11-22. Read-only research, PAL SCES-52456. V = read in code (disassembly of the level ELFs
and core, matched C in `games/rac3/decomp/src/levels/39_multiplayer_menu/`), I = inferred.
Companion to [ui-screens.md](ui-screens.md) (level 39's lobby screens). Addresses are level 39's
unless a map column says otherwise; map places come from `config/levels/functions.tsv` column 7.

## 0. The short version

The scoreboard is **not drawn in the map**. A match ends in two stages:

| Stage | Where | What | |
|---|---|---|---|
| 1. End banner, 250 frames | the **map's own copy** of the game-mode code | "<team/player> WINS" / draw text in a box, drawn by a HUD callback, no rows | V |
| 2. Results table, until Cross/Triangle | **level 39, lobby screen id 0x12** (vtable 0x0047DA80) | 8 rows x 5 columns of GUI-element widgets, same screen manager as the lobby | V |

The map hands the results to level 39 in a **core BSS block at 0x001FD7E8** (0x82C bytes, same
address in every program), filled by the map's end routine just before the map exits to level 39
with return mode 4 (online) / 3 (local). So the native scoreboard binds to **level 39 screen 0x12**
with the hooks mp_ui.c already has (0x005D69C0, 0x00686138, 0x006862D0), gated on the current
screen id being 0x12, and reads 0x001FD7E8. The map stage only needs a banner overlay (optional).

## 1. Flow

### 1.1 In the map (each map's copy; L39 addresses, map places in section 5) — V

| L39 | role |
|---|---|
| 0x00670438 | game-mode per-frame update (ptr from 0x61CC70). Host only (`net+0x168`): time up -> end(1); type 0 `mode+0x10 != -1` -> end(3); type 1 team score >= `GS+0x198` -> end(4); type 2 kill limit `GS+0x197` (per team if `GS+0x19D`, else per player) -> end(2) |
| 0x00670C50(reason) | host: builds the 0x68-byte channel-5 payload, command **0x82**, sends with 0x0014AD08 (reliable, everyone), then calls 0x670D90(reason) itself |
| 0x0066FA90(msg) | channel-5 receive (via 0x677CB8): 0x82 copies the score blocks into the mode struct (table 1.3) then **0x670D90(msg[0])** |
| **0x00670D90(reason)** | the end: `mode+8 = 2`, `mode+0x1F0 = 1`, `mode+0x1EC = reason`, 0x672400(), **memset(0x1FD7E8, 0, 0x82C) and fills the results block** (section 2), decides the winner (`+0x1A8`), registers the banner 0x66FF08 with 0x58A328 |
| 0x00670438 (again) | while `mode+0x1F0 != 0`: re-registers the banner each frame, `+0x1F0++`; at **0xFA (250 frames, 5 s at 50 Hz, I)**: `*(s32*)0x00241440 = net+0x170 ? 4 : 3`, `*(s32*)0x00242914 = 0`, **0x59A578(0x27, 1)** (load level 39) |
| 0x0066FF08(arg) | the banner (a HUD callback, returns 0): text id 0x1B9F with the winner's team name (0x66C7D8(+0x1A8)) through sprintf 0x120048, or id 0x185E (draw) when `+0x1A8 == -1`; FontSelect(1) if `*(*(u32*)0x241120) >= 2` (two local views, I); box 0x58C7E0(H/2-120, H/2-100, W/2-hw, W/2+hw, 0x33, 0x331465B7) + outline 0x5A8CF8(0x33, 0x332299DE, ...), hw = (text width 0x5947A0 + 10)/2; FontPrint(0x8066CCFF, text, -1, 1, 1, 0x80000000, x=W/2, y=H/2-118, 1.0, 1.0); a second smaller layout (-60/-40/-58) when `*(u32*)0x242914 != 0`; restores the font `$gp-0x6B50`. W/H = `*(s32*)0x240300` / `0x240304` (0 in the image, set at run time; I = 512/448) |

End reasons (`results+0x818`, `mode+0x1EC`): 1 time limit, 2 kill limit, 3 type-0 objective
(`mode+0x10` = winning team), 4 type-1 team score limit. V (names I).

Game type `GS+0x180` (copied to `results+0x80C`): **0, 1, 2** with the end rules above and the
results columns of section 2.2. I: 0 = Siege, 1 = CTF, 2 = Deathmatch (team when `GS+0x19D`).

### 1.2 Back in level 39 (boot 0x0067C408, matched C L39_0067C408.c) — V

`mode = *(s32*)0x00241440` (then set to -1):
- **4** (online return): per local view 0x5DA820/0x5E0450/0x5D7360, 0x6774C8, 0x5D8DB8;
  **`mgr+0x284 = 0x12`**; frame callback 0x67CA48 (lobby, net pumped); `net+0x170 = 0`.
- **3** (local return): same init, **`mgr+0x284 = 0x12`**, frame callback 0x67CCB0.
- 5 / 6 leave (0x6A7928), 9 -> screen 0x19, 0 / 1 / 2 / 7 / 8 the other entries.

### 1.3 Screen 0x12 "results" (vtable 0x0047DA80, ctor 0x006A69C0, factory 0x686810) — V

| vt off | fn | |
|---|---|---|
| +0x14 update | **0x006A74F8** (decomp L39_006A74F8.c) | calls 0x6A8138(screen, 0) (highlight/focus refresh) first |
| +0x18 draw | 0x006A7908 = common draw 0x0068B058 only | |
| +0xEC Enter | **0x006A72B0**: 0x67F100, 0x68AAB8, 0x68A9B8(0), 0x6A79C0; if `!net+0x170` 0x19CA28(1); **0x006A83B8** (fills the widgets from 0x1FD7E8); if `!net+0x170` **0x6E0B38** (stats/ladder upload from the results); sets +0x48C=0, +0x470/+0x478/+0x47C = -1 | |
| +0xF0 Exit | 0x006A74B8 | |

Update events (0x686138 codes): **Cross (7)** = continue: online host (+0x474 && +0x470 == -1)
sends a rehost message (fmt 0x2488C8, 0x52) to every other `GS+0x150[i]` and after 0x1E frames
leaves (0x6D40B0) and goes to screen 0xC; online client with `+0x478 >= 0` joins the host's new
world (0x6D34D8) -> screen 0x21, else message boxes 0x12F5/0x12F6/0x1FCF; **local**
(`net+0x174`): 0x59A578(`GS+0x17C` (+10 when two local players and <= 0x2D), 0) replays the map.
**Triangle (5)** -> 0x6A7928 (leave). Up/Down go to the common nav 0x68AEF8; **L1/R1 (0xB/0xE)
are not used** by 0x6A74F8 (it returns 0, default handler 0x68AF68 ignores them, I), so they are
free for scrolling.

## 2. The data

### 2.1 Results block **0x001FD7E8** (core BSS, 0x82C, filled by 0x670D90) — V

Index i = player slot 0..7 (the tNW_GameSettings slot). Every array is **8 wide**.

| off | type | from | meaning |
|---|---|---|---|
| +0x000 | char[8][0x20] | `GS+16*i` (empty -> "") | name |
| +0x100 | s32[8] | `player[i]+0x1A14` (players `*(u32*)(0x24CE20+4i)` in L39) | flag (I: local/bot) |
| +0x120 | s32[8] | `player[i]+0x2548` | **team** (colour 0x66C788(team), name 0x66C7D8(team)) |
| +0x140 | s16[8][2] | `mode+0x18` | **[i][0] kills, [i][1] deaths** |
| +0x160 | u8[8] | `mode+0x38` | type-0 column (I: siege nodes) |
| +0x168 | u8[8] | `mode+0x40` | (not sent in 0x82: host/local only) |
| +0x170 | u8[8] | `mode+0x48` | type-1 column A (I: flags captured) |
| +0x178 | u8[8] | `mode+0x50` | type-1 column B (I: flags saved) |
| +0x180 | f32[8] | `mode+0x58` | type-0 share (shown as % of the team total; not sent in 0x82) |
| +0x1A0 / +0x1A4 | f32 x2 | `mode+0x20C` obj +0x108/+0x10C (swapped by +0x88) | per-team totals for the % |
| +0x1A8 | s32 | computed | **winner: a team id**, -1 = draw (FFA: the team value of the best player) |
| +0x1AC | s32 | `(*$gp-0x6980)+0x198` | |
| +0x1B8 | s16[8] | `mode+0x198` | **suicides** (deaths with killer -1); score = kills - suicides |
| +0x1C8 | 0x120 | `mode+0x78` | per slot 0x24: s16 kills/deaths per weapon (`+4w`, `+4w+2`) |
| +0x2E8 | 0xA4 x team | built | team groups: +0 slot[8], +0x20 f32 `GS+0x110`, +0x40 f32 `GS+0x130`, **+0x8C count** |
| +0x808 | s32 | `GS+0x184` | |
| +0x80C | s32 | `GS+0x180` | **game type** 0/1/2 |
| +0x814 | s32 | net clock - `mode+4`, capped at `mode+0` | **match time, ms** (shown m:ss, text 0x1100) |
| +0x818 | s32 | arg | end reason |
| +0x81C..+0x824 | s32 x3 | `mode+0x200..0x208` | |
| +0x828 | s32 | `GS+0x19D` | **teams on** |

Winner rule (V, 0x670D90): with teams, the team with the highest sum of (kills - suicides) wins,
ties broken by fewer deaths (+0x142), still tied = -1; without teams the same per player; type 2
non-team also compares the team objects' scores.

### 2.2 Screen 0x12's widgets (0x6A83B8; widget k = `*(screen+0x110+4k)`) — V

| widget | content |
|---|---|
| title / subtitle | 0x689BD8(text 0x11F5), 0x689DB8(text 0xCD5) |
| 0 | own name (`net+0xBE`, trailing spaces cut), online only, vt+0xB8 |
| 1 | **winner line**: text 0x1334 formatted with the winner's name (team name 0x66C7D8, or FFA the player name from screen+0x2A8, or clan name 0x6D31B8), or 0xF3A (draw); vt+0x24 |
| 2 | match time "%d:%02d" (text 0x1100, `+0x814`) |
| 4 | vt+0xDC(clamp(`*(s32*)0x240400`, 40, 49) + 7) (I: a rank/skill value) |
| 5+r | row r shown (SetState 1) |
| 0xD+r | team colour bar (SetColour 0x66C788(team)) |
| 0x15..0x19 | column headers: 0x1332, 0x1336, 0x1337, then type 1: 0x1B73 / 0x1765, type 2: none, type 0: 0xF37 / 0x133D |
| **0x1A+5r** | name |
| 0x1B+5r | score = kills - suicides ("%d", fmt 0x2488F8) |
| 0x1C+5r | deaths |
| 0x1D+5r | type 0: `+0x160`; type 1: `+0x170`; type 2: blank |
| 0x1E+5r | type 0: `+0x180` as 0..100 % (0x1321C8, fmt 0x248900); type 1: `+0x178`; type 2: blank |

Rows: **8, fixed** (r = 0..7; widgets 0x1A..0x41). Order: screen+0x450[8] sorted by 0x6A7178
(bubble sort with compare 0x6A6FC8 on the results), names copied to screen+0x2A8[8][16]; a row
whose name is empty is skipped. Text ids are the game's string table (0x59A970); not resolved
statically.

Layout (ctor tables, fractions of 512 x 448; element x/y are centres, I as in ui-screens.md):
- header row y **0.465** (208 px), column x 0.105 / 0.35 / 0.465 / 0.615 / 0.745
  (54 / 179 / 238 / 315 / 381 px), table 0x004B05C8.
- rows y = **0.5125 + 0.0468 r** (230 + 21 r px: 230 ... 377), column x -0.065 (w 0.2) / 0.38 /
  0.515 / 0.645 / 0.785, table 0x004B0638 (+0 y0, +8 x[5], +0x1C w[5], +0x30 pitch). The name
  column's x -0.065 suggests another origin for that widget type (I; check live).
- widget 0 template 0x4B0490: x 0.5 y 0.162 w 0.82 h 0.045; panel 0x4B0580: x 0.5 y 0.648 w 0.82
  h 0.427 (I: the table's frame).

### 2.3 Game-mode struct (per program, `mode` below) — V

| program | mode |
|---|---|
| 39 | 0x00352940 |
| 40 / 41 / 42 / 43 / 44 / 45 | 0x00357260 / 0x003573A0 / 0x0034F2A0 / 0x0034F120 / 0x0034F160 / 0x0034F120 |
| 46 / 47 / 48 / 49 | 0x0035FCE0 / 0x0035FE20 / 0x0035FE60 / 0x0035FDE0 |
| 50 / 51 / 52 / 53 | 0x00368260 / 0x003683A0 / 0x003602A0 / 0x00360120 |

| off | meaning |
|---|---|
| +0x00 | time limit ms (-1 none) |
| +0x04 | start time (net clock `net+0x128`; -1 before start) |
| +0x08 | state: 1 running, **2 ended** |
| +0x0C | `net+0x12C` |
| +0x10 | winning team (-1 none) |
| +0x18 | s16[8][2] kills, deaths (0x670AE0: killer +0x18+4k, victim +0x1A+4v) |
| +0x38 / +0x40 / +0x48 / +0x50 | u8[8] mode columns |
| +0x58 | f32[8] |
| +0x78 | [8][0x24] per-weapon kills/deaths |
| +0x198 | s16[8] suicides |
| +0x1EC | end reason; **+0x1F0** end frame counter (0 = match on) |
| +0x200..0x208 | copied to results +0x81C |
| +0x20C | ptr: per-team object (+0x88 flag, +0x108/+0x10C f32) |
| +0x210 | ptr: team scores (+0x8 team 0, +0xC team 1) |
| +0x218 | [n][0x30] message lines, timers `*(s32*)0x24CF80[n]`, count 0x24CFA0 (L39; I: kill feed, drawn by HUD cb 0x6702A0) |

Channel 5, command 0x82 payload (0x68, 0x670C50 / 0x66FA90): +0x00 reason, +0x04 mode+0,
+0x08 mode+4, +0x0C 0x82, +0x0D mode+0xC, +0x0E / +0x10 s16 team scores (+0x210 obj +8/+0xC),
+0x1C mode+0x10, **+0x20 mode+0x18 (32)**, **+0x40 mode+0x198 (16)**, +0x50 mode+0x38 (8), +0x58
mode+0x48 (8), +0x60 mode+0x50 (8). V. So on clients `+0x168` (mode+0x40), `+0x180` (mode+0x58)
and the per-weapon block stay local values (I).

Net block 0x001A5CD0 fields used: +0x8 my client id, +0x128 clock, +0x12C, +0x168 host,
+0x170 online game (0 after the return), +0x174 local game, +0xBE own name, +0x58C7 (type),
+0xBC. GS fields: +0x150 account ids, +0x17C map level, +0x180 type, +0x184, +0x197 kill limit,
+0x198 score limit, +0x19D teams. V

## 3. Hook points

### 3.1 Results screen (level 39; preferred) — V

- Draw: existing wrap of **0x005D69C0**, gate on `mgr->screens[mgr+0x270]` being screen id
  0x12 (`mgr+0x1C0[mgr+0x270] == 0x12`), draw after the original. Hide the game's rows by setting
  widgets 0x1A..0x41 (and 5..0x14) to state 0 through vt+0x1C after 0x6A83B8 has run (wrap
  0x006A72B0 Enter, or 0x006A83B8), or draw a panel over them.
- Pad: existing wrap of **0x00686138**; take Up/Down/L1/R1 (codes 1/2/0xB/0xE) while screen 0x12
  is current and return 0 for them; leave Cross/Triangle to the game (continue / leave).
- Data: read 0x001FD7E8 directly (it stays valid until the next match ends: only 0x670D90
  memsets it). For more than 8 players this block (and the 0x82 payload, mode arrays) are 8 wide.
- Text: FontPrint 0x592238, FontSelect 0x591BA0 with the element font `*(u32*)0x0024CD5C`,
  colours as ui-screens.md section 6; team colour 0x66C788(team) = 0x80BBGGRR from the 3-byte
  table 0x002462A8 (L39; maps 0x002462C8), team name id table 0x002462E8 (maps 0x00246308).

### 3.2 In the map (banner stage, 250 frames; optional) — V

- Per frame 2D: the map's copy of **0x00588CE0** opens the HUD list (0x589538), calls the HUD
  callback runner **0x0058A378** only when the count `*(s32*)0x24BA38 != 0` (L39 numbers), then
  0x57C4D8(view), closes (0x589668); later the GUI elements 0x5D69C0 and closes again.
- Easiest draw-on-top: wrap the map's **0x0066FF08** (banner) and draw after it; it runs only
  while the match has ended (`mode+0x1F0 != 0`). For drawing during the match too, wrap the map's
  0x5D69C0 copy (always called) like the lobby hook.
- HUD callbacks: 0x0058A328(fn, arg) appends to fn[4] at 0x24BA18 / arg[4] at 0x24BA28 (L39),
  max 4; 0x587E08 / 0x5FBDD8 reset the count; the game re-registers every frame (I).
- Pad: no screen manager in a map (0x686138 is level 39 only). Read pad records directly: pad 0
  at 0x00225780 (`*(u32*)0x241164`), +0x1C4 pressed (core data, same in every program, V for
  the addresses, I that the map's pad update 0x5C34A8 copy keeps them current).

### 3.3 In-match score display (if wanted) — I

No separate in-match player table was found. The in-match numbers are HUD element pages built per
view by 0x5D6730 through 0x610448(page, ...): page A init 0x5DC648 / 0x5DE6B8 / update 0x5DD810,
page B init 0x5DF060 / 0x5E02F0 / update 0x5DF8F8 (team names and colours, mode+0x48),
and the score counter 0x005E0FCC (called from 0x4CF248; element 0x30017 = "%d/%d" of the view
player's kills - suicides vs `GS+0x197`). All GUI-element based (handles 0x3000C..0x30017).

## 4. Maps 40-49 vs 50-53

Every map has **its own copy** of the game-mode and HUD code at its own address (catalogue: one
fingerprint, 15 places 39..53); 50-53 are 40-43 shifted by +0x11000 code. The results block
0x1FD7E8 and the net block are core and shared; the mode struct and the team tables are per
program (2.3). The results screen exists only in level 39 (0x6A74F8 / 0x6A83B8 / 0x686138 have
no map places). V

## 5. Map places (catalogue column 7) — V

| L39 | role | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 | 51 | 52 | 53 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 670D90 | end: fill results | 544738 | 546900 | 53C1D8 | 5398C0 | 538CC0 | 5364A8 | 535D00 | 538580 | 537880 | 537200 | 555738 | 557900 | 54D1D8 | 54A8C0 |
| 670C50 | host: send 0x82, end | 5445F8 | 5467C0 | 53C098 | 539780 | 538B80 | 536368 | 535BC0 | 538440 | 537740 | 5370C0 | 5555F8 | 5577C0 | 54D098 | 54A780 |
| 66FA90 | ch5 receive | 543438 | 545600 | 53AED8 | 5385C0 | 5379C0 | 5351A8 | 534A00 | 537280 | 536580 | 535F00 | 554438 | 556600 | 54BED8 | 5495C0 |
| 670AE0 | kill applied | 544488 | 546650 | 53BF28 | 539610 | 538A10 | 5361F8 | 535A50 | 5382D0 | 5375D0 | 536F50 | 555488 | 557650 | 54CF28 | 54A610 |
| 670438 | mode update | 543DE0 | 545FA8 | 53B880 | 538F68 | 538368 | 535B50 | 5353A8 | 537C28 | 536F28 | 5368A8 | 554DE0 | 556FA8 | 54C880 | 549F68 |
| 66FF08 | end banner | 5438B0 | 545A78 | 53B350 | 538A38 | 537E38 | 535620 | 534E78 | 5376F8 | 5369F8 | 536378 | 5548B0 | 556A78 | 54C350 | 549A38 |
| 58A328 | HUD cb register | 457240 | 458DC0 | 44FBC0 | 44D780 | 44CAC0 | 44A2C0 | 44AF40 | 44D7C0 | 44CAC0 | 44C440 | 468240 | 469DC0 | 460BC0 | 45E780 |
| 58A378 | HUD cb runner | 457290 | 458E10 | 44FC10 | 44D7D0 | 44CB10 | 44A310 | 44AF90 | 44D810 | 44CB10 | 44C490 | 468290 | 469E10 | 460C10 | 45E7D0 |
| 588CE0 | frame 2D render | 455BF8 | 457778 | 44E578 | 44C138 | 44B478 | 448C78 | 4498F8 | 44C178 | 44B478 | 44ADF8 | 466BF8 | 468778 | 45F578 | 45D138 |
| 5D69C0 | GUI element render | 4A91D8 | 4AB2F0 | 4A0BC8 | 49E360 | 49D6B0 | 49AF48 | 49AF40 | 49D7C0 | 49CAC0 | 49C440 | 4BA1D8 | 4BC2F0 | 4B1BC8 | 4AF360 |
| 592238 | FontPrint | 45F4A8 | 461058 | 457E58 | 4559E8 | 454D28 | 452558 | 452E50 | 4556D0 | 4549D0 | 454350 | 4704A8 | 472058 | 468E58 | 4669E8 |
| 591BA0 | FontSelect | 45EE10 | 4609C0 | 4577C0 | 455350 | 454690 | 451EC0 | 4527B8 | 455038 | 454338 | 453CB8 | 46FE10 | 4719C0 | 4687C0 | 466350 |
| 5947A0 | text width | 461A10 | 4635C0 | 45A3C0 | 457F50 | 457290 | 454AC0 | 4553B8 | 457C38 | 456F38 | 4568B8 | 472A10 | 4745C0 | 46B3C0 | 468F50 |
| 59A970 | text by id | 467360 | 468F10 | 45FD10 | 45D8A0 | 45CBE0 | 45A410 | 45AD08 | 45D588 | 45C888 | 45C208 | 478360 | 479F10 | 470D10 | 46E8A0 |
| 58C7E0 | filled rect | 4596F8 | 45B278 | 452078 | 44FC38 | 44EF78 | 44C778 | 44D3F8 | 44FC78 | 44EF78 | 44E8F8 | 46A6F8 | 46C278 | 463078 | 460C38 |
| 5A8CF8 | outline | 4754A8 | 477058 | 46DE58 | 46B9E8 | 46AD28 | 468558 | 468E50 | 46B6D0 | 46A9D0 | 46A350 | 4864A8 | 488058 | 47EE58 | 47C9E8 |
| 58B2C8 | flat box | 4581E0 | 459D60 | 450B60 | 44E720 | 44DA60 | 44B260 | 44BEE0 | 44E760 | 44DA60 | 44D3E0 | 4691E0 | 46AD60 | 461B60 | 45F720 |
| 59A578 | load level | 467050 | 468C00 | 45FA00 | 45D590 | 45C8D0 | 45A100 | 45A9F8 | 45D278 | 45C578 | 45BEF8 | 478050 | 479C00 | 470A00 | 46E590 |
| 66C7D8 | team name | 5400C8 | 542290 | 537B68 | 535250 | 534650 | 531E38 | 531690 | 533F10 | 533210 | 532B90 | 5510C8 | 553290 | 548B68 | 546250 |
| 66C788 | team colour | 540078 | 542240 | 537B18 | 535200 | 534600 | 531DE8 | 531640 | 533EC0 | 5331C0 | 532B40 | 551078 | 553240 | 548B18 | 546200 |

All addresses are 0x00xxxxxx. Data the map routines use at level-39 addresses (0x24BA18 HUD cb
table, 0x24CE20 player table, 0x240300 W/H) are relocated per map (mp_maps.c's OPENRAC_LDATA
pairing); find each map's by its copy's lui/addiu, as done for the mode struct.

## Open questions

1. The text ids (0x11F5, 0xCD5, 0x1334, 0x1332/0x1336/0x1337, 0x1B73, 0x1765, 0xF37, 0x133D,
   0x1B9F, 0x185E): read them at run time with 0x59A970 to name the columns and the modes.
2. Game type names (0 Siege / 1 CTF / 2 Deathmatch is I) and the meaning of mode +0x38/+0x40/
   +0x48/+0x50/+0x58 per type.
3. Screen 0x12 name column x -0.065: confirm the widget origin in a live dump.
4. `*(s32*)0x242914` (the banner's second layout) and `*(*(u32*)0x241120)` (the font switch): local
   view count vs split screen, check in maps 50-53.
5. Widget 4's vt+0xDC value from `*(s32*)0x240400` (rank? level?).
6. On clients, results `+0x168`, `+0x180` (the type-0 %) and the per-weapon block are not in the
   0x82 payload: for a native board, show only what 0x82 carries, or have the server send more.
