# rac3 online rooms past 8 players (16, then 24)

2026-11-22/23, mp24 agent. Branch `feat/rac3-mp-24` (clone `~/RACSwarm/rac3-mp24`, box2 same path).
Tooling and raw findings: `wide/` next to this file (`sites.py` the dataflow, `gen.py` writes
`mp/wide.json`, `*_sites.txt` the findings per program).

## 1. Design

The game's per-player state is sized for 8 in fixed tables (protocol.md 7.4). The port widens them
without touching the game's logic, which stays the translated retail code:

- **Split tables.** Entries 0-7 of every table stay where the game keeps them; entries 8-23 live in
  tables the port owns. A room of 8 therefore touches exactly the memory it always did (8-player
  rooms stay byte-compatible by construction), and code not yet widened still sees slots 0-7.
- **hostgen sites** (`asm.wide` in `mp/hostgen.json` -> `mp/wide.json`, `asmc.parse_wide`). At each
  listed instruction the translation calls the host:
  - `ea TABLE`: a load/store's address -> `rac3_mp_wide_address(addr, table)`: if `addr - base`
    lies in `[8*stride, 24*stride)` it moves to the port's table, else unchanged (idempotent, so a
    pointer mapped once is never moved twice);
  - `value TABLE rN` / `arg TABLE rN`: an element pointer formed by the instruction, or handed to a
    call (after the delay slot), mapped the same way (the remote players' table, names to strcpy);
  - `bound`: a slot count (slti/sltiu 8, a countdown from 7) -> `rac3_mp_wide_bound(game)` =
    game + slots - 8;
  - `span`: the end pointer of a loop over a table (8 * stride) -> slots * stride.
- **Room width.** `rac3_mp_wide_set_slots(n)`, n = the game's MaxPlayers clamped to 8..24: the host's
  from its create request (or `OPENRAC_RAC3_MAX_PLAYERS`, 9-24, until the CREATE page offers more),
  a joiner's from the listed game's advert. 8 when the party ends. PAL only (NTSC-U has no sites).
- **Net block and remote players.** The port's tables in its statics (`openrac_guest_static`).
- **OBJECT_GAME_SETTINGS.** Its arrays' slots 8-23 are OBJECT_GAME_SETTINGS_WIDE, a port-owned
  companion (0x2E0 bytes: names 16x16, clans 8x16, skins/teams/clients/states/reservations 2x16,
  values A/B 4x16, accounts 4x16), relayed as `RELAY_OBJECT_WIDE` (kind 4: object id, bytes) just
  before the settings object, only in rooms of more than 8, and with the same owner/release rules
  (`mp_net.c` `gs_wide`). The game's DME type registry (475-byte limit) is not involved: the port's
  object store carries it.
- **DME client indices** 0..slots-1 (`GAME_CLIENTS` = slots in `mp_net.c`).

## 2. What the sites cover (step 1, landed in the PR)

| Program | Functions | Sites | What |
| --- | --- | --- | --- |
| core (I5BOOTN) | 33 | 152 (23 bounds, 2 spans) | NWUpdate's per-slot health (0x001959F8), session add/remove and the settings' slot init (0x001970F8), net_lib_start's clears (0x00198478), the 16 channel receive wrappers' slot stamp (0x00198A38..0x00198FF0), remote players' init/clear/create (0x001990D8, 0x00199BE8, 0x0019A180, callbacks 0x00199A60..), slot update (0x0019A4D0), the room join request (0x0019A6F8, its loops already run to net+0x134), my-team lookup (0x0019CA28) |
| level 39 | 35 | 116 (8 bounds) | every indexed access to the settings object's arrays and the net tables; bounds only in the lobby/staging slot searches (by client, by name, the ratings' sum, the create page's slot clear) |

Not widened on purpose (stay 8, documented in `gen.py`): the game-mode/results loops of level 39
(0x00670xxx, 0x006A6938/0x006A74F8/0x006A83B8: they write the 8-wide results block 0x001FD7E8 and
screen widget arrays), the staging list's own widget loop (0x006C30A0; the native list draws the
rows), bit-mask loops over a byte (0x0069AEF0, 0x006C4568), the talking mask (bit per client in one
byte: clients 8-23 have no voice icon), the voice streams' countdown (voice never runs in the port).

## 3. Step 2 (maps: in-game state past 8) - inventory, not yet done

The maps (40-53) touch the same net tables (11 functions / 45 sites a map, same shapes as level
39: `wide/lvl_sites_40.txt`), plus tables of their own that the analyzer does not list yet:

- the players' array (level 39 `D_L39_0024CE20`, 8 pointers) and each map's copy;
- the game-mode struct (per map, `ui-scoreboard.md` 2.3): kills/deaths s16[8][2] (+0x18),
  +0x38/+0x40/+0x48/+0x50 u8[8], +0x58 f32[8], +0x78 per-weapon 0x24 x 8, +0x198 suicides s16[8];
- the results block 0x001FD7E8 (8 x every column) and screen 0x12 (8 rows): the native scoreboard
  reads it, so it needs its own wide companion;
- channel 5 (0x68 bytes): per-player bytes +0x12[8] and the 0x82 score blocks: a room past 8 needs
  the extra players' bytes in a port-side companion message (relay kind, like RELAY_OBJECT_WIDE);
- channel 9's player index (bits 28-31 of word +0x14): 16 fits; 24 needs the fifth bit carried in
  the relay framing (a relay kind with one extra byte), restored before 0x0019AF90 decodes it;
- OBJECT_PLAYER_DATA +0x54 slot (s32: fine), the player UID's client nibble (16: fine; 24 needs the
  UID's counter bits).

Each map needs its sites generated (`sites.py N level` with the extra tables) and its bounds reviewed
like level 39's.

### Players' array per map (8 pointers; from the copy of L39_0066CDD0, catalogue pairing)

| 39 | 40/50 | 41/51 | 42/52 | 43/53 | 44 | 45 | 46 | 47 | 48 | 49 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0x0024CE20 | 0x00249330 | 0x00249530 | 0x00249420 | 0x002492A0 | 0x00249320 | 0x002492A0 | 0x00248EA0 | 0x00248FA0 | 0x00248FA0 | 0x00248FA0 |

Step 2 adds these, the game-mode struct's arrays (2.3 of ui-scoreboard.md) and the results block to
`sites.py`'s tables, reruns it per map, and reviews each map's bounds as level 39's were.

## 4. State at hand-off (2026-11-25)

Step 2 is on `feat/rac3-mp-24-step2` (PR #781, draft). Built and run on box2 (PAL, hidden, Server master
`--mode all`, a game server on 7770, clients on `OPENRAC_SERVER=127.0.0.1:7770`).

**The 9th joiner (found 2026-11-25).** It did not back out: it started the match. The host's join
handler (0x0019A6F8, loop at 0x0019ABA8) finds a free slot with `lh client[slot]` (0x0019ABB8, a wide
site) and then stores the joiner's DME client with `sh` through the same register (0x0019ABD4), which
the dataflow had not marked: slot 8 went into the settings object in place, onto `state[0]` (the client
array is followed by the state array), and "state[0] >= 7" is the game's "started". Everyone saw
"Starting...", the host loaded the map, and the joiner's wait ended. `wide.json` now also names the
second uses of an address register that a site already translates: 17 sites (the join store, the
send's destination 0x006C442C, level 39's and each map's third `lw` of the players array in
0x0061CE30 and its copies). Found by scanning each `ea` site for a later load or store with the same base register and offset
before the register is written (a branch-likely's delay slot does not count).

**The CREATE page.** The page is built when the multiplayer menu opens, before the login, so the
spinner's maximum cannot be set from its constructor by the connection state. `mp_net.c` keeps the
spinner's address and `create_page_sync()` (each MediusUpdate) sets its maximum to the room's most
while connected, else 8. The spinner then counts 2..24 online.

**Run results (box2, hidden llvmpipe clients, one joiner every 25 s):** host plus 10 joiners: STAGING
shows PLAYER 11/16, no start; host plus 15: PLAYER 16/16; host (spinner at 24) plus 23: PLAYER 24/24 on
the host and on joiners 12 and 23. Before the fix the 9th joiner made the host load the map (and crash
in a map's `sprintf` on a players entry past 8). A match from STAGING with more than 8 was not started
in these runs: the scripted READY of the joiners did not take, so the host's START stayed greyed.
NTSC-U widening is off (`rac3_mp_wide_available()` is PAL-only again).

Test recipe: Server master with `FAMILY_PARTY_SIZES=rac3-pal-1:24 MAX_CONNECTIONS_PER_ADDRESS=64
--mode all`; clients `OPENRAC_SERVER=127.0.0.1:7770 OPENRAC_RAC3_PROFILE=<name> OPENRAC_RAC3_NET_LOG=1`,
hidden (`SDL_VIDEODRIVER=offscreen OPENRAC_HIDDEN=1 OPENRAC_SOFTWARE_GL=1 LP_NUM_THREADS=2`), presses by
`OPENRAC_PRESS`: MULTIPLAYER (down, down, cross at frame 580), ONLINE PLAY (cross at 1300), the
configuration (cross 1400), the city (cross 1750), licence (Square 1850), announcements (cross 1950),
then the host: down, cross (CREATE GAME, 2050/2090), down x3 and right x(N-8) on PLAYERS, Circle;
joiners: cross, cross (QUICK PLAY, Any), cross (JOIN). The host's START is greyed until it and every
joiner is READY (Circle). The server must be named by its game port (7770), not the hub's, or CREATE is
refused ("sent PARTY, which the hub does not take").

## 5. Step 2b (2026-11-25, later): the match's end past 8 players

Written, built (box2, PAL) and run on box1 and box2:

- **Results for slots 8-23.** Each map's copy of `func_L39_00670D90` (the end routine, 15 programs) is
  wrapped (`mp/fixes.py` FIXED, `rac3EndResultsMap<N>`, `host/mp_fixes.c` `fix_end_results`): the game's
  routine runs, then `rac3_mp_wide_end_results` (mp_wide.c) fills the results block for slots 8 and up
  from the port's companions: the name (from the settings' wide half), the local flag (with the game's
  own profile reset, `func_L01_003F1F60`, called through the program's copy, level 39's 005778C8), the
  team, each team record's member list (capped at the record's 8), and every mode array (scores,
  suicides, the three columns, share, weapons). For the deathmatch types (reason 1 or 5, game type 2)
  the winner at results+0x1A8 is chosen again over every slot (`wide_winner`, the game's own rules:
  best kills less suicides, fewer deaths, a tie of both is -1; teams by their sums). A room of 8 is the
  game's own (the wrap returns at once).
- **Channel 5's end message (0x82).** `dme_relay` sees the 0x68-byte channel 5 message with command
  0x82 in a room of more than 8 and first sends `RELAY_MATCH_END_WIDE` (kind 5: u8 the room's width,
  then slots 8 up of mode +0x18 (4 bytes a slot), +0x198 (2), +0x38, +0x48, +0x50 (1 each)), to the same
  target; the receiver's `rac3_mp_wide_end_unpack` puts them into the port's mode companions before the
  message itself reaches the game, so the end routine's wrap finds them. The other mode arrays (+0x40,
  +0x58 share, +0x78 weapons) stay each console's own, as in the game.
- **Channel 9.** Read, not changed. The receiver (`func_L39_00677740`) finds the sender's player by the
  sender's DME client (`func_L39_0061BED0(sender, bit 25)`, whose players-array loads are sites), and
  hands bits 28-31 of word +0x14 to `func_L39_0066F260` as the bit mask of the packet's four history
  blocks, not as a player index. So the fifth index bit of a slot past 15 is never needed to pick a
  table entry; no framing was added. Whether the stream decodes right for clients 16-23 is not shown
  yet (the run below has 10 players).
- **`mp_ui.c`** loops to `rac3_mp_wide_slots()`: the staging table (settings arrays through
  `rac3_mp_wide_settings`) and the scoreboard (the results block through `rac3_mp_wide_results`); the
  ordering takes up to 24 rows. The game's own widgets stay 8 rows (hidden while the native screen draws).
- `OPENRAC_RAC3_NET_LOG=1` also logs the settings' slot client/state as each console sees it, the end
  message's companion on both ends, and the wide results.

NTSC-U widening stays off (`rac3_mp_wide_available()`); `release_carry_rac3.py` was rerun so NTSC-U's
`mp_fixes.h` and rows carry the new wrap.


## 6. Match test, 2026-11-25 evening: READY and START in rooms past 8

Binary: PAL release build on box2 (this branch), copied to box1; Server master `--mode all` on the Mac with
`FAMILY_PARTY_SIZES=rac3-pal-1:24 MAX_CONNECTIONS_PER_ADDRESS=64`; clients hidden, muted, llvmpipe, all on
box1 (started 8 s apart), scripted by `OPENRAC_PRESS` (host: CREATE with PLAYERS right x(N-8) and TIMED 5
Minutes, READY, up + cross for START; joiners: QUICK PLAY, join, READY). The scripts are
`~/RACSwarm/k3-wide.sh` (orchestrator) and `k3-mp24-run.sh` (one client) on the Mac. `OPENRAC_RAC3_NET_LOG=1`
logs each console's view of the slots' client/state, the channel 17 slot updates it delivers, the wide
end message and results, and `N ready of M players: START ...`.

Proven (screenshots in `~/Projects/RAC/mp24-progress/`): a room of 10 (host + 9 joiners) fills, every console
shows the same 10 slots (8 and 9 included), every slot reaches READY (6) and the host's START enables
with 10 of 10 ready.

What was wrong, and what fixes it:
- **Box2's software-GL clients** run far below 50 fps (about 190% CPU each; box1's use 6%), so the scripted
  presses, which are by frame, drift by minutes. All hidden clients of a room run on box1.
- **Joiners with no name in the game's net block** sent "(null)" and an uninitialised skin in the join request
  and slot updates; `name_fix` (mp_net.c `dme_relay`) puts the account's name and skin 0 in. (A pre-widening
  build, box1 `~/RACSwarm/mp-ui-run/bin`, shows the right names and skins, so this is a regression from the
  widened or merged code, cause not found; the rows read "j1", "j2" ... now.)
- **READY past 3 joiners reset itself.** The staging update (L39_006C20B0) reads the pad's pressed bits each
  game tick; a client too slow to draw runs several ticks in the window frame of one press, so it sends
  ready (state 6), then unready (state 1) once per extra tick (client 1 six times, client 7 none: the count
  depended on how far each console lagged). Rooms of 8 do the same on such clients; a real GPU client does
  not tick more than once a frame. With `OPENRAC_RAC3_PORT_READY=1` the port takes every slot's Circle press
  (`rac3_mp_ui_pad_filter` in pad.c's scePadRead: the press is left out of the pad word, so the game never
  sees it, and `rac3_mp_net_toggle_ready` sends the slot update, or for the host writes its own state).
- **Slots 8 and up** have no row in the game's staging widgets, so their Circle never reached a READY handler;
  the same function takes their press without the variable.
- **START stayed greyed** with all 10 ready: the host's per-frame check (L39_006C30A0) counts the ready among
  slots 1-7 and itself and compares with GameSettings +0x178 (the players), and the START handler reads its
  byte right after that check in the same call. `mp_func_L39_006C30A0` (a new wrap, mp/libraries.tsv) sets
  the byte again from every slot when the room is wider than 8.
- The staging header reads `PLAYER n/m` from net+0x130 and net+0x134: `m` is the room's maximum
  (`rac3_mp_wide_set_slots`), so 10/10 is a full room of 10 (screenshots show 2/8, 5/10, 3/10 on the way).

Result of the run with the START wrap (host + 9 joiners on box1, 10 players): START pressed with all 10 ready,
every console loaded map 40 (Bakisi Isles) and drew its HUD with the other players' name tags (screenshots 12, 13
in `~/Projects/RAC/mp24-progress/`), and channel 9 (the player state stream) flowed from every client, slots 8
and 9 included. The match then did not progress: the HUD clock stayed 00:00 and the frame did not change for
the 40,000 frames the run lasted (about 13 minutes), so the 5 minute time limit never ended it and the end
message (0x82), the wide results and the scoreboard have not run. No joiner sent the map-loaded slot update
(state 8) to the host; only the host's own state 8 went round (the host's settings read 0:8, the joiners' 1-9 stay
6). Whether the map's start barrier waits for those states, and why the joiners' L39_00575CC8 / map-load update
(channel 17, state 8) is not sent, is the next thing to find: compare the same run at 2 players, where the
earlier notes say the barrier opens, and read what the barrier counts (a bound or slot table of the map that
the wide.json sites do not cover is the first suspect).

Also left: clan tags print as "(null)" after the name tags (the join request's clan, +0x50, and the slot
update's, +0x17, should get an empty string the way `name_fix` gives the name); the map shows no scenery in
box1's data dir (no level exports there: extract the PAL levels with the launcher's "Get the levels ready"
pipeline, tools/extractor.py, to see the world); 16 and 24 players, headless joiners (a window-less run
`--window` left out: not yet tried, its tick pacing decides whether the frame-scripted presses still line
up) and clients on box2 and the Mac; the staging list shows 8 rows (the native staging table is not up in these
runs: the server serves no UI description) and has no scroll bar; channel 9 for clients 16-23; the NTSC-U
carry of the new wraps' places (release_carry_rac3.py was rerun for the first batch).

## 6b. Earlier state of the same match test (before the READY/START fixes above)

Binary: PAL release build on box2 (this branch's step 2b plus `name_fix`, built from the tree before the
merge of develop and the other agent's push), copied to box1; Server master `--mode all` on the Mac with
`FAMILY_PARTY_SIZES=rac3-pal-1:24 MAX_CONNECTIONS_PER_ADDRESS=64`; clients hidden, muted, llvmpipe,
scripted by `OPENRAC_PRESS` (host: CREATE with PLAYERS right x(N-8) and TIMED 5 Minutes, READY, up + cross
for START; joiners: QUICK PLAY, join, READY). `OPENRAC_RAC3_NET_LOG=1` now also logs each console's view
of the settings' slot client/state, the channel 17 slot updates it delivers (first 10 payload bytes),
and the wide end message and results.

Proven: rooms of 10 (host + 9 joiners) fill and every console shows the same 10 slots (`settings (sent)`
on the host, `(received)` on the joiners, slots 8 and 9 included); 2-3 player rooms ready and START as before.

Findings:
- Box2's software-GL clients run far below 50 fps (about 190% CPU each), box1's use about 6% each; the
  presses are by frame, so a script that spreads clients over both boxes drifts by minutes. Run all
  hidden clients of one room on box1 (10 clients, host included, fit with room to spare) and start them
  8-10 s apart.
- A joiner that joins with no name in the game's net block sent "(null)" as its name and an
  uninitialised skin (staging rows "(null)" / "Paradox ERR"); `name_fix` (mp_net.c `dme_relay`) puts the
  account's name into the join request and slot updates. With it the rows show `j1`, `j2` ... and the skins
  are valid. This did not change the ready problem below.
- **READY does not hold with more than 2-3 joiners, in a room of 8 as well as of 10** (so not a
  widening effect): every joiner's READY (channel 17, state 6) reaches the host and is applied, then the
  same joiner sends channel 17 state 1 ("leave", L39_006C4308), which the host applies as state 0, once
  for every joiner that readies after it (client 1 sends six, client 2 five, ... client 6 one, client 7
  none: the last stays ready). Cause not found. Not yet compared with a build from before step 1
  (box1 `~/RACSwarm/mp-ui-run/bin` is such a build but has no settings logging and needs a disc image
  in its data dir). Start there next: whichever of the stock staging update (L39_006C20B0) or the
  port's object update path (`obj_receive` calls the type's on_update with the whole object, field -1)
  makes an already-ready joiner leave when another slot's state changes.
- Joiners in slots 8 and up send no READY at all (their Circle press produces no channel 17 message):
  the staging screen's own widget loop (L39_006C30A0) stays 8 rows, so there is no row for them to
  have the cursor on. Needs a port-side READY for slots 8+ (a native handler for Circle in the
  staging screen that sends the slot update from the wide settings), in `mp_ui.c`.
- So no room of more than 8 could be started: the host's START stays greyed. The end message
  (0x82), the wide results and the scoreboard are written and built but have not run in a match.

## 7. The map's start barrier past 8 players, 2026-11-25 night

What held the match at 00:00 (all found by reading the retail assembly of the map's main loop, the generated C and
the net log of a 10-player run; every site is in `mp/wide.json`, for level 39 and the maps 40-53, whose copies of the
code are the same instructions at other addresses):

- **The map's main loop** (`func_L39_00575CC8`, L40 0x004425C8, state 17) counts the players that exist
  (`D_players[0..7]`) against the room's player count (GameSettings +0x178) before it sends its map-loaded slot update
  (state 8), the host counts the joiners' state 8 over slots 1-7, and the host sets its own slots' state. Its loops are
  pointer walks with a counter (`li rN,7 ... bgez`), so the dataflow had not found them: `bound` sites on the counters,
  `ea` sites on the walked loads (players array, slot clients, slot states), and the walk over the remote players
  (stride 0xC70: `span` at its end pointer, `ea` on its field loads, `arg` on the call of 0x00199878 handing the
  element: **an `arg` site is the address of the call, not of its delay slot**; that mistake left slots 8 and 9
  without a hero and the barrier at 8 of 10 players).
- **The per-frame player update** (L39 0x0066AFA0) ends with a loop over the players array: widened the same way.
- **The host's slot update handler** (core 0x0019A4D0) stores the new state of a slot in its in-game branch with a
  `sh` at 0x0019A5FC that had no site: slots 8 and up never got state 8 on the host, so the barrier of the host
  (`n == players - own`) stayed shut. Its clan copy (the call at 0x0019A628) had no `arg` site either: a clan tag of a
  slot past 8 was written over the next table.
- **The match clock never ran in any multiplayer match**: the programs add the EE's timer 1 count (0x10000800) to
  their frame clock and write 0 back; the port has no timer page, so it read 0 (a 2-player run showed the same frozen
  00:00 with the bots moving). `timer1_feed` (mp_net.c) puts the real time since the last net update there, as a
  16-bit counter at 576 ticks a millisecond.

Run tooling: `OPENRAC_NO_RENDER=1` (docs/port/DEBUGGING.md) skips the whole frame (built, drawn, presented) and keeps
the tick, the net and the scripted pad at full speed: a no-render client uses about 8% of a core against 190% for a
software-GL one, and its window-frame counter (`OPENRAC_PRESS`) is the same, so scripts written for rendering clients
run unchanged. A room's fleet: host and one joiner render (box1, levels extracted with
`extractor/extract.py port DISC assets/port/rac3/pal`, `--levels`), the rest do not (box1 up to 10, the Mac, box2).
`HOSTGEN_JOBS` sets hostgen's default worker count (a box's memory).

### Where the 16/24-player match stands (end of the 2026-11-25/26 session)

Proven: a 10-player room (host + 9 joiners, spread over box1 and the Mac) loads map 40, every console passes the start
barrier (all ten slots at state 8, the host's included), draws the world with the other heroes, and plays (a 2-player
run on the same build shows bots moving and shooting). Not yet seen: the match clock running (the timer 1 feed is in,
untested in a full match), the 0x82 end message and the scoreboard for a wide room (the host's end path did run once
in a corrupted 16-player run: "the end message's slots 8+ sent", "results for slots 8-15").

Open, in the order they bit:
- **The server's keep-alive** (`KEEP_ALIVE_IDLE_SECONDS`, 30 s, "covers a map load") dropped a host whose map load
  took longer than that on a loaded box: "Session (host) stopped sending, dropping it", then the joiners' "the party
  is gone". The test Server runs with 240.
- **A fatal error at START in a 12-player run** (host, level 39, frame 22051): `func_L39_005B0AE0 at 005B0BD0 reached
  the scratchpad DMA register 1000D400`, with the host's own slot at state 0 (not ready) when START was pressed: the
  scripted host READY had been undone (a drifted press), so this may be the greyed-START path, not the wide code.
  Not diagnosed.
- **The map's clock**: see above; whether the 5-minute limit now ends a match is the next thing to run.
- **Clan tags** still print "[(nul]" in the HUD name tags: the slot update's clan copy now goes through the wide
  table (0x0019A628), but the "(null)" text itself (the game's sprintf of a null clan) comes from the senders'
  messages; `clan_fix` (dme_relay) did not catch it in the join request (the request is logged under
  OPENRAC_RAC3_NET_LOG=1 to find the bytes).
- **The staging list** keeps the game's 8 rows: a server description with a `staging` screen
  (`~/RACSwarm/k3-ui/rac3-pal-1.json` in the test Server's UI_DIRECTORY, columns name/clan/team/skin/ready on the
  `players` table) makes it native with the scroll marks; not yet looked at.
- NTSC-U carry of the new sites (release_carry_rac3.py) not run.

### 2026-11-26: the START crash, and what the 12- and 16-player runs showed

- **START crash (fixed, #959):** not the greyed-START path. The menu program (level 39) carries its own copies of
  the moby evaluator (0x005B0AE0), draw side (0x005B24E8) and texture upload (0x005B2238); the maps' copies are
  replaced by the port's, level 39's were translated from assembly and wrote the DMA chain-mode registers. They now
  have `mp/libraries.tsv` rows (`rac3MobyChainMenus`, `rac3MobyDrawSideMenus`, `rac3MobyTexturesMenus`, host C in
  `mp_maps.c`). A 12-player run passes START with them (no crash; the host script then missed its START press).
- **16 players corrupt the settings tables:** with joiners in slots 14 and 15 the host's GS_CLIENT / GS_STATE
  words at slots 0-6 hold the text "(null)" (two copies, 14 bytes each from +0xE0 and +0xF0 = slot 14 and 15 of
  the names table at stride 16), and the joiners then die at a SIGSEGV at game address 0x70056420 when the host's
  next settings broadcast arrives. A writer of the name (or clan) of slots 14 and 15 bypasses the wide table
  (an unlisted copy site); rooms up to 13 are fine.
- **Clan:** the join request's clan text is at +0x40 of the payload (+0x30 is the name); `clan_fix` now empties every
  "(null)" in the join request and slot updates instead of looking at one offset.
- **Scripted fleet:** a new join clears the others' ready, so every joiner must press READY after the last join
  (join.sh computes it), and the host's START press must come after that; the staging screen's team/skin dialog
  opens from a drifted press (see ~/Projects/RAC/mp24-progress/19-host-12p-after-start.png).
- Build costs: the mp hostgen takes about 12 minutes on box1 (python 3.14) and 35 on box2 (3.12); a binary built on
  box1 (Manjaro) does not run on box2 (Ubuntu: glibc, libstdc++, SDL3), so each box builds its own.

### 2026-11-26 (later): the settings corruption found and fixed; where the 16-player match stands

- **Cause of the "(null)" in GS_CLIENT / GS_STATE:** found with a gdb breakpoint on the port's sprintf
  (`mp_func_00120048`) for buffers in the settings object past +0xC0. The writer is the host's session-add
  handler (`func_0019A6F8`): after the name's sprintf (site 0019ABDC) it prints the joiner's clan into
  `settings + 0x80 + 8 * slot` with a second sprintf (the call at 0x0019ABF4) that had no site. Clan slots 8-15
  are +0xC0..+0xFF, so slot 12 wrote the text over the clients (+0xE0), 13 over +0xE8 and 14/15 over the states
  (+0xF0..), and slots 8-11 over skins and teams. The text is "(null)" because the clan the request carries is
  that text (clan_fix empties it in the request). Fixed: `"0019ABF4": "arg GS_CLAN r4"` in `mp/wide.json`.
  Not slots 14 and 15 only, and not a name write.
- With the site: a 16-player room (host + 15 joiners on box1, no-render, scripted) shows every slot's client
  `n`, a joiner's state 6 (ready) and an empty clan in the net log (`settings (sent)` now prints each clan),
  "PLAYER 16/16" and all sixteen names in the native staging list (mp24-progress/21), no crash, no stray write
  into the settings (breakpoint over +0xC0..+0x17F never hit).
- **Not done:** START. The host's staging screen has "16 ready of 16: START enabled", but the scripted `up`/`left`
  then cross never reaches START: the stock list widget has 8 rows, `up` from the first row wraps to the eighth
  and cross there opens the team/skin dialog (mp24-progress/22); `left` does not move to the left pane. The
  native list also shows stale rows (j1, j2, j1, j3, j2, ...) after a scroll. A port-side START path (like
  READY for slots 8+) or a staging screen whose cursor reaches START is the next step. So no wide match has
  played to the scoreboard yet.
- Scripted host: READY by Circle once, after the last joiner joined and readied (a second Circle un-readies it);
  all joiners READY after the last join. The frame counter at no-render speed is about 50 a second.
- Build recipe on the boxes: the decomp must be a clean pin (`git archive` of the submodule's commit over a copy
  that keeps asm/ and baserom/): a tree with another session's edits fails hostgen with "its signature takes 0".
  `OPENRAC_RAC3_PAL_RETAIL_IMAGE` must be a file nobody removes. `sudo gdb -batch -p PID` works for
  `break mp_func_00120048 if buffer >= $gsp + 0xC0 ...` (do not name a gdb variable `$gs`: it is a register).

## 8. Static sweep for the remaining 8-player assumptions, 2026-11-26

Method (no fleet): the retail assembly of the core (`I5BOOTN`, 0x190000-0x1A8000), level 39 and map 40 (the other
maps are copies of the same code, so every map site below is found in all 14 by matching the instruction window)
was scanned for (a) `slti`/`sltiu` 7/8/9 and `li 7`/`li 8` + `bgez` loop bounds that are not in `mp/wide.json`,
(b) a register holding the settings object (`lw 0x241640` / `-0x6980($gp)`) or the net block (`0x1A5CD0`) that
forms an element address (`addiu`/`addu`) with no site within 16 instructions, and (c) loop bounds beside sited
loads. Raw lists: 523 unsited bound-looking instructions in the three programs (most are jump-table range checks
and state constants), 176 in functions that touch the settings or net tables, 51 of them beside sited loads, plus
16 + 12 unsited element addresses in levels 39 and 40 and 13 in the core. Reviewed by hand, the real ones:

Fixed (sites added, 125 in all; `mp/wide.json`):
- **core 0x0019A868..0x0019A8F0** (5 `ea GS_RESERVED`): the session-add handler's walk over the slots' reservation
  words (settings + 0x100, 2 bytes a slot, up to `net+0x134` slots) had no sites, so for slots 8 and up it read and
  wrote the next table (GS_VALUE_A, per-mode values from the join request). Its account walk (0x0019A7A0) was sited.
- **level 39, five "which slot is mine" searches** (`bound` at 0x0068D724, 0x006A0404, 0x006A5D34, 0x006CBE44,
  0x006D4F64): loops over `GS_CLIENT` whose loads were sited but whose limit was still `slti 8`, so a console in
  slot 8 or up never found its own slot (found = -1) in the staging, name and skin handlers.
- **name and clan formatting, 7 sites in level 39 and in each of the 14 maps (105)**: the in-game HUD name tag
  (clan `value GS_CLAN` and name `value GS_NAME` handed to the tag's sprintf and strcpy; 39 0x00635D04/08/24, 40
  0x00507BFC/C00/C1C), the kill messages (39 0x0066FD74, 0x0066FDA0, 0x0066FE4C) and the score caption's name
  copy (39 0x0066690C): the element pointers into the settings' 16-byte names and 8-byte clans were formed from
  the slot index with no site, so slots 8 and up printed whatever lies at settings + 0x80 + 8 * slot: the `[(nul]`
  of the HUD tags. The earlier clan-sprintf miss (0x0019ABF4) was the same class.

Port side (same PR): a START path for rooms past 8 (the pad's Start button, or Up from the first row of the native
list, `wide_start` in `host/mp_ui.c`: the game's cursor moves to its START widget, and Start presses it when the
game's own START byte is set; a greyed START plays the refusal), a scroll bar on a table longer than its page, and
the list's `joined/MAX` count (`openrac_mp_ui_table_capacity`).

Left at 8 on purpose or not reachable (still listed, none touched):
- level 39's staging widget loops (0x006C30A0 row widgets, 0x006C1730 draw, 0x006C20B0 update: their rows are the
  native list), the results screen loops (0x006A6938, 0x006A74F8, 0x006A83B8, 0x00670D90: widened by the end-results
  wrap), bit-mask loops over a byte (0x006C4568, 0x0069AEF0), the ratings table of 11 entries (0x006C4148: 0x28-byte
  entries indexed by account, an 11-entry table that 24 distinct accounts would overflow), the voice countdowns.
- Per-frame scans over the players array that count down from 7 (`li 7 ... bgez`) with the body's loads sited: the
  one that was missed (map 40 0x00426B88, the same function in maps 41-45 and 50-53) now has a `bound` (10 sites); the
  other `li 7`/`li 8` hits are return codes, not loops.
- 0x006C3670/0x006AF09C compare a count from `func_006CC238` (the lobby's player count) with 8; left.

### 2026-11-26 (night): the first 9-player tests, and the map-start crash that stops them

Harness (kept on box1/box2 under `~/RACSwarm/k4-mp`, moving to `~/RACSwarm/build/OpenRAC`): `fleet.sh N TAG` starts a host
on box1 (GPU, one screenshot every 300 frames) and N-1 no-render joiners, scripted by `press.py` (login; CREATE with
PLAYERS right x(N-8) and TIMED once; joiners QUICK PLAY and READY with `OPENRAC_RAC3_PORT_READY=1`; the host READY, then
the Start button). The Server ran on the Mac (`KEEP_ALIVE_IDLE_SECONDS=240`, `FAMILY_PARTY_SIZES=rac3-pal-1:24`).
Disc data: the extracted `iso_data/rac3` (with `sectors.tsv`) is needed, not just the boot file.

Proven with 9 players (host + 8 joiners, all on box1, build of this branch):
- the room fills, every console reaches READY ("9 ready of 9 players: START enabled", header "PLAYER 9/9");
- the native staging list (vanilla description, now bound past 8 players) lines up under the game's headings, shows
  the count `9/9` and a scroll bar; the host's **Start button reaches START** (`START (port path): enabled`), the
  match starts and every console loads map 40. (The first try found that the game's START byte is set after the event
  is read; the port now counts the room's readiness itself.)

Not proven: the match. All 9 consoles die together at the map's first ticks (frame ~13150) with `SIGSEGV at game
address 0x4014EB4C`, a store in `func_L40_0053D0A0` (`sw 0x787878, 0x78(r3)`, called by `004EEB68`, `003DB0E8`,
`00493A20`, `004CE660`): that function walks the 0x340-byte entries of `D_0033BE40` (a 16-entry array, count at
`-0x1DC8($gp)`) and takes the player pointer at entry + 0x2D0; for entry 4 that word holds the float bits
0x4014EAD4. A watchpoint on that word (gdb, `$b = openrac_guest_base`, `watch *(unsigned*)($b+0x33CE10)`) saw four
writes: the marker's own init (`func_L40_005390B8`, a valid pointer), a clear (`func_L40_0045D4C0`, a 0x4DC0-byte
memset of a0 = 0x33BE40 from `00534D30`/`0050C780`), then two float stores by `func_L40_00504C70` (this = 0x33BE40,
fields +0x110/+0x114 are sticks). So `D_0033BE40` is used as one big object (0x4DC0 bytes) by the pad/hero code and as an
array of 0x340-byte marker entries by `005390B8`/`0053D0A0`: the marker array's entry 4 overlaps what the hero code
writes. **It reproduces on the build from before this branch (baseline `04ae121f`, 9 players), so it is not from the sites
above.** (An 8-player control did not start a match in this harness: the stock START path, up + cross, drifted into
the team dialog; whether 8 players pass this code is therefore unproven.) Open: find why a 9th player makes
`005390B8` take entries past the first four (its caller `003FE8B8`, a0 = 0/1, a1 = player mobys, lifetime 3000), or
whether 8 players already use five or more entries only at other times; the 5390B8 call list (6 calls at the crash:
a0 0,0,0,1,1,1) is in `~/RACSwarm/k4-mp/out/h-t9f/gdb.out` on box1 while it lasts.

### 2026-11-26 (day): the map-start crash found; item 2 of section 8 closed

- **Cause of the SIGSEGV in `func_L40_0053D0A0`.** `D_0033BE40` is the hero-marker array (16 entries of
  0x340 bytes, 0x3400 in all, `D_0033F240` follows). The 0x4DC0-byte struct the hero code writes (`this` of
  `00504C70`, field +0xFD0 = marker 4's +0x2D0) is not that table's: it is **the remote players' struct
  array** `D_00315040` (map 40; `D_00310900` in level 39, one per program), 8 entries of 0x4DC0 indexed by
  slot, built by `func_L40_00538618` (`func_L39_00666518`: `D_00315040 + slot * 0x4DC0`, `addu` at +0x54).
  Its end is `D_0033BE40`: slot 8 (the 9th player) is at 0x33BE40, so the marker table and a player share
  memory. gdb on the host's `mp_func_L40_0050C780` (the struct's init) showed g = 0x319E00 + 0x4DC0 * k for
  slots 1..7 and 0x33BE40 for slot 8. The marker count (`D_L39_002461F8`) and size are the map's own
  (spawn points, not players): no change.
- **Fix:** table `MAP_PLYS` (`value MAP_PLYS r18` at the `addu`, 15 programs), the base per program in
  `wide_program[].prog[]` (mp_wide.c); slots 8-23 live in a 16 * 0x4DC0 port table, cleared when a program
  loads. A new `IN_PROG` table kind takes per-program bases.
- **Same class, found by a scan of the slot id (`lw x, 0x2544(player)`) and of every `%lo` formation of the
  candidate tables:** `SLOT_A` (sdata, `D_002410F8`: slot 10 overwrote `D_00241120`, the local players'
  count pointer), `SLOT_B` (`D_00249790`/L39 `0024D528`), `SLOT_C` (0x80 bytes a slot, the moby ids of a hero;
  `D_00290178`/L39 `00293F38`), `SLOT_D` (0x1C bytes a slot, the hero state machine; `D_003AA6E0`/L39
  `004840A0`) and `PLY_MODE` (profile struct + 0xA4, a byte a slot, core). Each is 8 entries (checked against
  the next data label). 81 sites a map, found in map 40 and carried to the other programs by function place.
- **Not done (leads, none touched):** the three [3][8] sensitivity tables `D_001A58D0/5930/5990` (index
  `row * 0x20 + slot * 4`: an address alone cannot tell slot 8 of row 0 from slot 0 of row 1, needs a
  row-aware site; a slot 8+ writes the rows of others), byte/word arrays inside one object indexed by slot
  (map 40 `00516E08`, `004255A0`, `00431768`), the pointer-walking loops over these tables (`00470428`
  clears `D_00290178` rows by a countdown: the port tables are zeroed on program change instead), and the
  tables' last uses reached with a slot passed as an argument.
- **Item 2, closed by reading:** `0x006C3670` and `0x006AF09C` test field 3 of a packed 32-bit lobby
  attribute word (`func_006CC2A8`/`006CC268` mask and shift at net + 0x5F4) against 8: that is a flag bit
  and an average rating, not a player count. `0x006C4568` is a CRC-32 bit loop and `0x0069AEF0`'s 7s are
  attribute field numbers: neither touches players. The 11-entry ratings cache (`D_001FD630`, 0x28 bytes,
  `func_006C4148`) is an LRU by account: with more than 11 distinct accounts it evicts, it does not
  overflow; nothing in a match or its end waits on it. The results screen's own widget loops stay 8 rows
  (the port's scoreboard replaces them).

### 2026-11-26 (afternoon): the match clock, 2 and 9 players to the scoreboard, the 16-player hang

- **Levels must be extracted for a match to run.** Without `data/port/pal/level_NN` (extractor `port ISO OUT
  --anywhere --level 39 ... --level 53`) the map's world never loads and a match sits frozen at 00:00; the
  earlier "frozen match" notes were this. ~/RACSwarm/k4-mp/data keeps them on box1 and box2.
- **The match clock was broken in every room** (a 2-player room too): `func_L39_0066FA90` (the channel 5
  handler, L39 `00677CB8`) copies 104 bytes from the pointer it is given and, for any command but 0x80-0x82,
  sets the mode struct's time limit (+0) and start (+4) from it. The core's per-message notifier
  `func_00198C18` calls that handler with the message pointer in a3 ($7 passes through from its caller);
  hostgen declared it with three arguments, so a3 was 0 and the handler read guest address 0, where the port
  keeps the text "OPENRACSESSION01": limit and start became text, the remaining time (limit - (now - start))
  overflowed negative and the HUD read 00:00; nothing ended the match. Fix: `"func_00198C18": "int (int,
  int, int, int)"` in mp/hostgen.json's signatures. Found with gdb: a breakpoint on the map's copy of the
  handler (`mp_func_L40_0054B3D0`) printed a3=0.
- **Results:** a 2-player and a 9-player room (host + 8 no-render joiners, box1) play the 5 minutes and end at
  "Game Over" with the scoreboard (the native list pages 1-7 of 9). Screenshots 31 and 32 in mp24-progress.
- **The 16-player hang** (host spinning in `func_L40_00542450` = `func_L39_0066EAA8`, the effect list's walk,
  whose state switch has no default): the effect list's head `D_0024948C` held a moby. `func_L40_0053D958`
  (`func_L39_0066B7C8`) creates a marker moby per player into `D_00249450[8]` with a loop bound the room's
  count had widened but no table site for the stores: slot 15 wrote `0x249450 + 15 * 4 = 0x24948C`, the
  head. Found with a gdb watchpoint on the head. New table `SLOT_E` (`D_00249450`, with the five lookups
  `003EDAB8`, `003F0B28`, `003F0B90`, `003F0C68`, `003F0CB0` of the same table: `ea` on their loads and `bound`
  on their `slti 8`s, in every map that has them; maps 39 and 45 have only the creation).
- **Still open (found by listing every `bound` site whose function forms a table that has no site):** the
  game mode's per-player records behind `D_00357260 + 0x20C` (a struct of 8-entry arrays at +0x40, +0x60, +0x88,
  +0xA8, +0xC8, +0xE8, read by map 40's `00544DF0`, `004AEBF0`) and `D_0024845C`, `D_00248500`, `D_00249FC0`
  in `003C8FF0`, `0053E1A8`: each loop's bound is widened and its tables are not; an in-object `u8 buf[8]` on
  the stack (`004255A0`, `00431768`) is written by slot.
