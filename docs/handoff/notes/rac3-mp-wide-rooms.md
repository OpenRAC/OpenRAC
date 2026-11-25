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

Still open: the items 2-5 of the PR (results for slots 8-23, channel 5's end message, channel 9, the
native scoreboard), a match started from a room of more than 8 (not run yet), and the NTSC-U carry (item 6).
