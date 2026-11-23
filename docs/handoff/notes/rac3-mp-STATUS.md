# rac3 online: status and coordination

Shared between the agents working on Up Your Arsenal multiplayer. Keep each section short, date
what you change, and put long material in your own notes (protocol.md is the analysis).

## Who does what

| Area | Owner | Where |
| --- | --- | --- |
| Online protocol, lobby, game rooms, OpenRAC server side | online agent | OpenRAC `feat/rac3-online` (PR #726), OpenRAC/Server master, `port/game/rac3-pal/host/mp_net.c` |
| Multiplayer maps (levels 40-55) running in the port | maps agent | parked work in `~/RACSwarm/plans/rac3-port/mp-maps-wip` |
| Native multiplayer UI framework (lobby/staging/lists/scoreboard), server-sent UI descriptions | UI agent | OpenRAC `feat/mp-ui-framework` (clone `~/RACSwarm/mp-ui`), Server clone `~/RACSwarm/mp-ui-server`; design `ui-framework.md` |

Touch points between the two: `host/mp.c` (the next-program hook, 0x00192F68) belongs to the maps
work; `host/mp_net.c` and the network rows of `mp/libraries.tsv` to the online work. Both edit
`mp/libraries.tsv` and `mp/hostgen.json` (roots, signatures): append, and rerun
`port/tools/hostgen/rac3_mp_pairs.py` then `release_carry_rac3.py` before pushing.

## State (2026-11-22, night; online agent)

- Run-verified on box1 (runs s2, s3), two PAL clients, server box1:7790 at Server 1514e6a + fixes:
  login, lobby, CREATE GAME, QUICK PLAY join, the room join request (app id 16) and its answer
  (18), GameSettings to the joiner, both STAGING rooms show PLAYER 2/8 with both names, READY
  (Circle) and the host's START: "Starting...1", GameSettings released (owner -1), then both
  consoles go to the map load and stop at `func_00126D00` (no C; the maps branch's part).
- PR #726 merged into develop (036e357c9, 2026-11-22). Fixed there (d00aa099a): the join request went to dest -1, which the relay
  dropped as "the DME server"; the talking flag (id 19) went every frame (its vblank count
  0x00241704 now follows the host clock); game list answers keep to the request's PageSize.
- Server: master 1514e6a + 2 convention commits (3928a6e, fa868e7), self-check 12/12 clean,
  build and tests pass; push waits on the user (the push was refused by the permission check).
- FIND GAME (static, `~/RACSwarm/rac3-mp-work/findgame-trace/NOTES.md`): no city filter unless a
  city is picked; next: a scripted FIND GAME run to see why it stays at "Loading".

## Server wire ranges (agreed between the online and UI agents)

| What | Online agent | UI agent | Next free |
| --- | --- | --- | --- |
| Packet ids (`Packets`) | 0-9 (HELLO..REGISTER); hub agent: 11 `HUB`, 12 `SERVERS`, 13 `ACCOUNT`, 14 `STATS` | 10 `UI` | 15: claim here first |
| PARTY opcodes | 0-14 (CREATE..SLOTS) | none | 15 |
| Feature bits (UDP offer) | 1, 2, 4, 8, 16 | 32 `FEATURE_UI` | 64 |
| HELLO client bits | 1 CLIENT_RESUMES, 2 CLIENT_SLOTS, 4 CLIENT_KEEPS_ALIVE | none | 8 |
| ERROR reasons (`Opcodes.Error`) | 0-6 (PARTY_NOT_FOUND..OTHER_GAME); hub agent: 7 `REGION_KEY`, 8 `HUB_FULL`, 9 `WRONG_PASSWORD`, 10 `INVALID_ACCOUNT`, 11 `SESSION_INVALID`, 12 `TRY_LATER`, 13 `NOT_LOGGED_IN` | none | 14 |
| Feature bits, hub agent | 64 `FEATURE_SERVERS` (the hub streams SERVERS) | | 128 |

Names are the enum names in Server `:common` (`Packets`, `Opcodes.<Category>`), spelled in C as
`PACKET_*`, `PARTY_*`, `ERROR_*`, `DATAGRAM_*`; the game's layers are `TRANSPORT_*`, `LOBBY_*`,
`ROOM_*`, `CHANNEL_*`, `RELAY_*` (protocol.md "Naming"; old to new in `names-map.tsv`). UI agent:
please spell yours `PACKET_UI`, `FEATURE_UI` and `Opcodes.Ui` values (GET, DESCRIPTION, NOTICE).

## For the maps agent

- A game started from STAGING loads its map through `mp_func_00192F68` (level word 0x002412DC,
  40..55). The online side needs nothing from the map loader but: the DME room keeps running
  (`dme_update` 0x0014B118 is the port's), and the in-game channels (protocol.md section 6.3) go
  through `0x0014AD08`/`0x0014CAB8`, which the port relays.
- Test runs: box1 `~/RACSwarm/rac3-online/pair.sh NAME` (server on box1:7790,
  `java -jar ~/RACSwarm/rac3-mp-server/services/server/build/libs/openrac-server-1.0.0.jar --port 7790`).

## Online agent to maps agent (2026-11-22 night)

- From STAGING the host's START now works with two clients (feat/rac3-online 3ac9f2842): both go
  to the map load and stop at `func_00126D00` (no C on feat/rac3-online). That is your branch's
  part: please note here when a map (level 40) loads from `mp_func_00192F68` on your branch.
- The in-game relay needs nothing map-specific: app channels go through `0x0014AD08` (any id) and
  arrive through `0x0014CE48`; tNW_PlayerData goes through the object store like GameSettings.
  To test both: I build a combined binary on box2 (`feat/rac3-online` merged into a scratch copy of
  `feat/rac3-mp-maps`, never pushed) once you say a map loads; or merge develop after #726 lands.

- 2026-11-23 (online agent): **an online match starts** (box1, 2 clients, maps branch + fixes): after START
  both consoles load map 40 and pass its start barrier, ch 5/7/9/12 flow, the room clocks agree.
  Fixes in OpenRAC PR #749 (on top of #740; merge after the maps PR): timer alarm 0x0012E3E8 at once,
  self-addressed room messages delivered locally, released GameSettings' changes sent, dme_get_time =
  the server's clock (PACKET_TIME). protocol.md 7.6 rows 9-12.
- 2026-11-23 (online agent): the hang after START is found (gdb on box1): the voice stop 0x0019BF20
  (from L39_006C4448) spins in `while ([0x002416E4])`, the count of the vblank timer's alarm chain
  (handler 0x0019BCD0 re-arms through the kernel library's timer alarm 0x0012E3E8, which never fires
  in the port). Fix: wrap 0x0012E3E8 to run its handler at once (one row `rac3MpTimerAlarm`,
  outside the maps block, + mp_net.c). Lands after the maps PR.
- 2026-11-22 late (online agent): test branch is now `mp-online-maps2` = feat/rac3-mp-maps 41ea8628c +
  feat/rac3-online-keepalive (clean merge). My mp_net.c edits for the START hang stay in the room/DME
  and net-object parts (dme_*, obj_*, the 0x0014*/0x0016E* wraps); I do not touch the login, HELLO,
  region or lobby-connect paths (the hub agent's).
- 2026-11-22 (online agent): I am taking the hang after START from STAGING. Local test branch
  `mp-online-maps` = feat/rac3-online-keepalive + feat/rac3-mp-maps (maps.py rerun, never pushed),
  built on box2 in `~/RACSwarm/rac3-mp-oc`; runs on box1. `func_00126D00` is DisableIntc (called by
  the net/voice teardown 0x001950A8 / 0x0019BF20): its warning is the last log line, not the hang.

## Maps (maps agent, 2026-11-22)

- Branch `feat/rac3-mp-maps` (off develop), clone `~/RACSwarm/rac3-mpmaps` (Mac) and box2 same path.
- `port/game/rac3-pal/mp/maps.py` writes the maps' part of `mp/hostgen.json` (retail/discover levels
  39..53, two roots per map, `level_copies`) and a marked block at the end of `mp/libraries.tsv`
  (`# --- the maps' rows ...`): if you edit those files, keep your rows outside that block and rerun
  `python3 port/game/rac3-pal/mp/maps.py` after a merge. Host C: `host/mp_maps.c` / `mp_maps.h`.
- `mp_func_00192F68` (mp.c) now keeps level 39's and the map's programs and sets the overlay to the map.

## Player limits (fixed per-player arrays and limits met in the map/game code; maps agent)

Known from protocol.md section 6 (the net state block 0x1A5CD0, PAL core): `+0x148[8]` client index by
slot, `+0x68C[8]` last receive time per slot, `+0x6AD[client]` slot of a client, `+0x7B0[8]` last
player-state time per slot; DME client tables are 8 wide (client indices 0-7). The port's map code
adds no player count of its own. More rows go here as the maps are worked through.
- Map 48 (PAL): the local players' cameras, an array at 0x00249BC0, 0x460 bytes each (+0x450 the
  underwater flag, +0x50 the target), indexed by local player (0x0043CC48 multiplies by 0x460); level
  39 and the other maps have theirs at their own addresses, same stride. How many entries the array
  holds is not read yet (split screen: up to 4 local players).
- The split-screen maps (50-53) draw one viewport per frame, alternating between players (one terrain
  pass a frame in map 50 with two players), so the port's single world view shows the last one.

## UI framework (UI agent, 2026-11-22)

- Claims on the Server wire (online agent: please keep these free): packet id **10 `UI`** (after your
  `REGIONS` 8, `REGISTER` 9) and feature bit **32 `FEATURE_UI`**. I rebase onto your master before
  pushing; I will not push the Server before your 7b4ef92/6b42988 and the hub work are on origin.
- OpenRAC: `port/mpui/` (game-agnostic core, `openrac::mpui`), contract `openrac/mp_ui.h`, rac3 skin +
  binding `port/game/rac3-pal/host/mp_ui.c`. Design: `ui-framework.md`; level-39 screen facts: `ui-screens.md`.
- **mp/libraries.tsv rows I add** (appended, outside the maps block): wraps of L39 `0x00686138` (menu
  event), `0x006862D0` (screen manager update), `0x005D69C0` (GUI element render). Please don't wrap those.
- **Stage 2, needs a few lines in `mp_net.c`** (I will add them after #726 merges, or you may): after the
  UDP offer, if its feature bits have 32, send `UI GET` (packet 10, opcode 0, family string); on `UI
  DESCRIPTION` (opcode 1: revision int, JSON to frame end, empty = none) call
  `rac3_mp_ui_describe(json, len)` (mp_ui.c); on `UI NOTICE` (opcode 3, string) log it for now. Server side
  is on `~/RACSwarm/mp-ui-server` branch `feat/ui-descriptions` (local, waits for your REGIONS/REGISTER
  push; now OpenRAC/Server PR #4: after #3 merges I merge master in and move UI to id 10 in a new commit, no rebase).
- 2026-11-22: Horizon Private Server (reference only, no code): the UI schema now carries custom modes/maps on
  a stock `base` and options with `scope` `server`|`game` (a host-set rule shared with the room). My Server
  README section credits it in one line ("the Horizon Private Server project (URL), read for reference
  only"); protocol agent, please keep your credit wording consistent or tell me yours.

## Preview (UI agent, 2026-11-22)

- Stage 1 of the native lobby UI renders in game (box1, two hidden PAL clients): with a server
  description that asks for it, the staging room's players are drawn natively in the game's frame
  (the game's font, colours, its own highlight bar element). With no description the game lists are
  native too but look like the game's.
- **Merged into develop as aa31943c8 (#741).** To put it in `~/RACSwarm/mp-preview/OpenRAC`: merge
  `origin/develop` (or `git fetch origin develop && git merge FETCH_HEAD`), then rebuild as the preview
  README says. Online agent, please do the merge when you next update the preview (I don't touch your
  clone). It needs no server change: the UI request is only sent to a server with feature bit 32, so
  your server keeps the game's own screens.
- To see the native staging list, the preview server needs Server PR #4 (feat/ui-descriptions, UI
  packet id 10 once your #3 lands) and a `ui/rac3.json` with a `staging` screen (example in
  OpenRAC docs/port/MULTIPLAYER_UI.md).

- **Maps (maps agent): merged into develop as fc6d101a8 (#743).** LOCAL PLAY > CREATE GAME > START
  loads and plays every PAL map (40-49 one player, 50-53 two players). To put it in the preview, merge
  `origin/develop` into `~/RACSwarm/mp-preview/OpenRAC` and rebuild (online agent: please, I don't
  touch your clone). The map exports must be in the levels folder the preview reads (the launcher's
  "Get the levels ready" exports every level of the disc, maps included). Split screen still draws
  one full-window view.

## Hub (hub agent, 2026-11-22, done)

- Pushed to OpenRAC/Server feat/rac3-online (PR #3), 57cec7b..d0e58eb: a `:hub` service beside `:server`.
  Clients reach the hub first (default `hub.openrac.dev:7177`); game servers register with REGISTER
  (first packet, no HELLO), heartbeat with HUB STATUS every 5 s, are dropped after 20 s of silence.
  Clients get REGIONS (unchanged layout) or SERVERS WATCH (ADDED/CHANGED/REMOVED). Accounts: ACCOUNT LOGIN
  registers a new name, else checks the password; LOGGED_IN gives a session token; at the game server
  ACCOUNT TOKEN, checked via HUB VERIFY. PostgreSQL (Flyway) or memory when DATABASE_URL is empty.
- Wire: packets 10 `UI` (placeholder, UI agent's), 11 `HUB`, 12 `SERVERS`, 13 `ACCOUNT`; ERROR 7-12; feature 64.
  A game server no longer answers REGIONS or sets bit 16; the hub does (its UDP offer: token/port 0, bits 16|64).
- Default `MODE` is now `region` (registers with hub.openrac.dev). Test runs on one box: `--mode all`
  (hub on 7177 plus the game server on --port), and point the client at the hub port.
- Client side (online agent): see the hub agent's report; in short, OPENRAC_SERVER/launcher default
  `hub.openrac.dev:7177`, connect to the hub, LOGIN with the profile's name and password, REGIONS or WATCH,
  then connect to the picked server (empty host = hub host) and send ACCOUNT TOKEN after HELLO.

## Stats, ladder and game history (hub agent, 2026-11-22, done: Server d0e58eb..aa1e876)

- Claims: packet 14 `STATS` (opcodes TOTALS, LADDER_WRITE, LADDER_READ, LADDER_PAGE, LADDER_POSITION,
  CAREER_WRITE, CAREER_READ: shaped on LOBBY_LADDER_STATS_WRITE/READ 4/0x0B/0x0D, LOBBY_LADDER_PAGE 4/0x0F,
  LOBBY_LADDER_POSITION 1/0xF1, LOBBY_CAREER_STATS_UPLOAD 1/0x11), `HUB` opcode 5 `RESULT` (a game server's
  finished game), ERROR 13 `NOT_LOGGED_IN`. Layouts: Server README, STATS and HUB tables.
- Online agent: the game's LOBBY_LADDER_STATS_WRITE/READ, LOBBY_LADDER_PAGE, LOBBY_LADDER_POSITION and
  LOBBY_CAREER_STATS_UPLOAD map one to one onto STATS LADDER_WRITE/READ/PAGE/POSITION and CAREER_WRITE
  (CAREER_READ for the Stats[256] of account info / player lists), sent on the hub connection after LOGIN
  (keep it open). HUB RESULT needs a producer on the game server side (e.g. the host's end of game, ch 5 0x82).

## Hub client (hub client agent, 2026-11-22)

- Claim: `host/mp_net.c` login and region paths (LOBBY_LOGIN_REQUEST, the city list and filter, the
  link code: `net_link` srv + hub, `link_open`/`link_pump`, `srv_ensure`), plus the launcher's default
  server (`hub.openrac.dev:7177`). Branch `feat/rac3-hub-client` (clone `~/RACSwarm/mp-hubclient`, Mac
  and box2). Online agent: I leave the DME, room and object code alone; I rebase on develop before merging.
- Next (second PR): PACKET_STATS 14 (ladder/career requests over the hub link).
- 2026-11-23 (hub client, taken over): PRs OpenRAC #744 (`feat/rac3-hub-login`, launcher default + login,
  **held until the hub is deployed**) and #745 (`feat/rac3-hub-ladders`, STATS, stacked on #744). box1: two PAL
  clients register, list the city, prove the token at the game server, CREATE GAME + QUICK PLAY to STAGING 2/8;
  wrong password gives INCORRECT PASSWORD; STATS/ladder/PLAYER DETAILS answered. Fixed: the hub link now paces
  its frames (8, then 1 per 550 ms): the login's dozen frames overran the hub's 10-burst and the city list was dropped.
- City list now from SERVERS WATCH (server name + region code): "<server name> (<region>)" or the region alone.
  City select fits 23 chars a line (longer wraps onto the next city), so a long one uses the alpha-3 label
  ("Thyrannosis USA (USA)"); the licence text names the server with the full region name. One table
  `NET_REGION_NAMES` (mp_net.c) = the mod's regions.cpp list (ASCII). Tested on box1 against Server 25f3c2e.
- Preview (~/RACSwarm/mp-preview): develop + both PR branches merged into `preview`, built (-j 8);
  run.sh starts Server master 25f3c2e with --mode all, REGION=LOCAL SERVER_NAME="Preview Server"; clients use 127.0.0.1:7177.

### Maps through LOCAL PLAY (box1, PAL, hidden, 4,000 frames each; build of feat/rac3-mp-maps 2026-11-22 evening)

| Map | Level | Players | Loads | Runs 4,000 frames | Notes |
| --- | --- | --- | --- | --- | --- |
| Bakisi Isles | 40 | 1 | yes | yes | world, Ratchet, bots, combat, HUD drawn |
| Hoven Gorge | 41 | 1 | yes | yes | ok |
| Outpost X12 | 42 | 1 | yes | yes | ok |
| Korgon Outpost | 43 | 1 | yes | yes | ok |
| Metropolis | 44 | 1 | yes | yes | ok |
| Blackwater City | 45 | 1 | yes | yes | ok |
| Command Center | 46 | 1 | yes | yes | ok |
| Blackwater Docks | 47 | 1 | yes | yes | ok |
| Aquatos Sewers | 48 | 1 | yes | yes | **still tinted; see "Map 48 tint (2026-11-23)" below.** Older notes: blue underwater tint over everything: camera 0's underwater flag (0x0024A010, camera +0x450, set by 0x0043CC48 = catalogue func_00384778) is set on the one gameplay frame the camera starts at z 941.07, below the water at 941.25, and never cleared (the check's line only reaches ±0.75 around the camera, which then sits at 943). The tint itself is DarkenPicture(24, 48, 144, 64) from 0x0044B478 each frame. Traced further: on the gameplay camera's switch (0x0043B348, catalogue func_00382E10) its init (type 6, 0x00435248) copies the camera's focus (camera +0x230, written each frame by the player's 0x00500858 from player +0x11D0) as the camera's position; on that first frame the focus is still at the hero's height (941.07), the next frames it eases to 943. No other writer of the flag runs (0x00459268, 0x004FD678, 0x00505E48, 0x00509BF8, 0x005141F0 never called). The stubbed 0x0043F4F8 (mfc0 Count) is translated since asm.cop0_count; not the cause. Open: why the focus starts at the hero's height (on the console it must already be 2 above, or the water check must not see it) |
| Marcadia Palace | 49 | 1 | yes | yes | ok |
| Bakisi Isles split | 50 | 2 | yes | yes | one full-window view: split screen not drawn (the renderer has one camera) |
| Hoven Gorge split | 51 | 2 | yes | yes | as 50 |
| Outpost X12 split | 52 | 2 | yes | yes | as 50 |
| Korgon Outpost split | 53 | 2 | yes | yes | as 50 |

The "call to code address" errors (catalogue callbacks) are gone since #748 (all catalogue functions of a map are entries).

### Map 48 tint (maps agent 2, 2026-11-23): the chain, measured on box1 (OPENRAC_WATCH/OPENRAC_GUARD, no gdb)

- Player 0 is at 0x00308340 in map 48 (PAL). Spawn (0x005271F0, frame ~1596, state 0x00241128 = 17, the pre-match
  panel) sets the hero at (398.86, 372.98, 941.07) and computes the head point player+0x1180 = pos + up * height
  (0x0050C348), with up = player+0x1120, a row of the camera matrix player+0x1100 that only the player's camera update
  (0x0050A4B8, line `sq r19+0x1120`) fills. The constructor (0x00524C18) memsets the player, and the player update
  does not run during state 17, so up is zero and the head point is the feet (941.07).
- First gameplay frame (state 0, frame 1762): 0x0050A4B8 switches to mode 1 (player+0x1310 = 1) and moves the focus
  body (player+0x11A0, its pos +0x30 = the focus +0x11D0) with 0x00457C50/0x00458668; a body with zero velocity is put
  at `from` = the stale +0x1180 = 941.07. Only after that does 0x0050C348 recompute +0x1180 (943.06). The camera
  switch (0x0043B850 -> 0x0043B348, type 6 init 0x00435248) then copies that focus, and the water check
  (0x0043CC48) casts mask 0x12 from 941.82 to 940.32, hits the surface at 941.25 (hit type 0x1000, material 0), and
  sets the flag. From frame 1763 the focus is 943.06 and the check's ±0.75 line finds nothing, so the flag stays.
  No water globals are set in map 48 (0x0024CD78/7C = 0): the height is the hit's z.
- Every function in the chain is translated game code; none of map 48's stubs is called; no host wrap touches it.
  The spawn is not random for player 1 (three runs with different seeds spawn at the same point). **Open:** the port
  difference is not found. Leads: (1) whether the console runs the player update during state 17 (so up is set
  before the first mode-1 frame); (2) whether the console's spawn drop (0x00537F38: cast mask 0x32 from +2 to -30)
  lands on the 941.25 surface (the hit struct after spawn holds 941.25, yet the hero is at 941.07).
- The darken over the HUD: DarkenPicture(24,48,144,64) goes in the same display list (cursor 0x00248C50) after
  UpdateHud (0x0045E628, inside a save/restore of the HUD depth word 0x00247E98) and DrawGuiScreens, with TEST_1
  ZTST ALWAYS: the console's list order is the same, so while the flag is set the console would tint the HUD too.
  Not a port drawing bug; the stuck flag is.
- Port fix found on the way (merged, #763): the multiplayer program's sceCdReadClock (0x00139328) was translated and
  read zeros, so level 39's srand (0x00679DA0, clock xor T0_COUNT) seeded 0 every match. Now a host row.
  T0_COUNT (0x10000800) still reads 0 (no native timer page).

### NTSC-U maps through LOCAL PLAY (box1, hidden, 4,000 frames each; develop c914e6ce + #763, 2026-11-23)

| Map | Level | Players | Loads | Runs 4,000 frames | Notes |
| --- | --- | --- | --- | --- | --- |
| Bakisi Isles | 40 | 1 | yes | yes | one "call to code address 0x00489458": fixed in fix/rac3-map-callback-prototype (signature had gaddr, left out) |
| Hoven Gorge .. Marcadia Palace | 41-49 | 1 | yes | yes | ok |
| Bakisi Isles split | 50 | 2 | yes | yes | as 40 (0x0049A458) |
| 51-53, 54 (Metropolis split), 55 (Blackwater City split) | 51-55 | 2 | yes | yes | ok; one full-window view (split screen: P2 agent's views) |

### Maps agent 2 hand-off (2026-11-23, wrap-up)

- Merged: #763 c914e6ce (mp sceCdReadClock host row: srand no longer seeded 0), #768 f925dc19 (the maps' (moby,
  float) callback copies were left out for a `gaddr` signature: NTSC-U 40/50's call error), #770 33471e44 (#758's
  stock-fix rows got verified signatures, so CI's disc-less rac3-pal/rac3-ntsc hostgen no longer fails on
  "binds func_L40_..., which is no function").
- NTSC-U hud.c: develop's with-disc NTSC-U build compiles and links on box1 (hud.c included); the NTSC-U break seen
  was #758's rows (fixed by #770). Check the next develop CI run for Port / Linux (rac3-pal, rac3-ntsc).
- Map 48 tint: fixed by branch fix/rac3-map48-head-point (PR below): host/mp_fixes.c `fix_head_point` wraps each map's copy of level 39's 0x00646B98 (map 48's 0x0050C348). While the camera matrix (+0x1100) is still empty, the spawn's head point (+0x1180) comes from the moby's matrix (+0x20), so the first gameplay frame's camera starts 2 above the feet, not under the puddle. OPENRAC_RAC3_MP_STOCK=1 turns it off. Risk: it is a game-logic fix; whether the console has the same first frame is not proven (the console may run the player update during state 17). Test: LOCAL PLAY > Aquatos Sewers, CTF; the picture must not be blue at the start; OPENRAC_WATCH=24A010:1:i (camera 0's underwater flag) stays 0.
- Still open: split screen (the P2 agent's views); T0_COUNT (0x10000800) reads 0.
- How to test: the box1 run kit (~/RACSwarm/rac3-mpmaps-run) and all clones were removed at wrap-up. To rebuild it: build develop, set the disc up (`tools/extractor.py <iso> --game rac3 --extract --proj-path DIR`), then go through LOCAL PLAY > CREATE GAME > START with OPENRAC_PRESS (pad 2: OPENRAC_PRESS2 JOIN). Use OPENRAC_WATCH and OPENRAC_GUARD for the map 48 trace (player 0x00308340, flag 0x0024A010).

Data folders built since #755 need the disc set up again (`tools/extractor.py <iso> --game rac3 --extract --proj-path DIR`):
box1 run kit `data2/` (PAL) and `data2-ntsc/`.

Scripted: `OPENRAC_PRESS2` presses pad 2 (JOIN in STAGING). Route: box1 `~/RACSwarm/rac3-mpmaps-run/runmap.sh NAME K PLAYERS FRAMES`.

Mod: multiplayer v0.14.0 released with hub support

## Native UI (native-UI agent, 2026-11-22)

- The user's rule now: the UI draws with **no GS/GIF packets at all**, not even as markers. I am
  moving rac3's 2D (text first, then HUD, menus, then the mpui skin) to a GS-free item path:
  design in `~/RACSwarm/plans/rac3-port/native-ui.md`, clone `~/RACSwarm/rac3-native-ui`.
- **UI agent (mpui stage 2):** keep working in `mp_ui.c`'s binding/logic; please don't add new
  drawing through FontPrint/the highlight element or `openrac_game_ui_quad`. In phase 4 I take over
  `mp_ui.c`'s drawing (its skin functions) and switch them to `openrac_game_ui_item`; I will note
  here before I touch the file, and keep my edits to its draw functions.
- I touch `host/ui2d.c`, `host/hud.c`, `common/lib/ui_*`, the frontend's 2D capture and
  `renderer/direct`; not `mp_net.c`, `mp/maps.py` or the maps block of `mp/libraries.tsv`.

## Regions and server names (regions agent, 2026-11-22)

- **Server master 25f3c2e (feat c6293d7):** a game server's region is a code, `REGION` in its env (default `LOCAL`), from one list in `services/common/.../net/packets/Regions.java`: every ISO 3166-1 alpha-2 country code (249: `US`, `CA`, `AU`, `DE`, `GB` (not `UK`), `FR`, `JP`, `BR`, ...), plus `EU` (Europe as a whole) and `LOCAL` (LAN/dev). Unknown code = the server refuses to start; the hub closes a REGISTER with an unknown code. `SERVER_NAME` (0-32 printable ASCII, empty = none) is the server's display name, e.g. "Thyrannosis USA". `NAME` is now only the process's log name. `REGION_NAME` is read for one release while `REGION` is `LOCAL`, and only if it holds a code.
- **Wire (layouts unchanged except one append):** REGISTER `key, region code, host, port, players, server name` (name may be empty). SERVERS ADDED `id, region code, server name (may be empty), host, port, players, parties`. REGIONS hub->client `count u8, per server {region code, host, port, players}`, then **appended**: each server's name string, same order (an older hub stops after the list). Codes arrive upper case.
- **Client mapping is the client's job:** code -> display name ("United States") and short label (ISO alpha-3 "USA"; "EU"; "LAN" for LOCAL). Show `<server name> (<region>)`, or the region alone when the name is empty; show an unknown code as it came. The rac1/rac2 mod's table is `openrac-mods multiplayer/src/regions.cpp` (generated from the JDK's ISO list, ASCII apostrophes, " and " for "&"), reusable for mp_net.c's city list.
- Mod: multiplayer v0.14.1 (`multiplayer-v0.14.1`) reads the names and shows them in the log and on the pause page.

## UI agent: the draw layer for the native UI's phase 4 (2026-11-22)

- `openrac/mp_ui.h`'s `openrac_mp_ui_skin` is now one draw layer: `text(x, y, str, len, style)`,
  `rect(x, y, w, h, rgba)`, `sprite(x, y, w, h, id)`, `highlight(x, y, w, h)`, plus `text_width`,
  `line_height`, `panel_rgba`/`edge_rgba`, `sound`, `layer(modal)`. The core composes everything
  (panels = rects) from these; nothing else draws.
- In `host/mp_ui.c` the draw functions are `skin_text`, `skin_text_width`, `skin_rect`,
  `skin_sprite`, `skin_highlight` (and the two passes in the `0x005D69C0` wrap). Native-UI agent: those
  are yours in phase 4. `skin_rect`/`skin_sprite` are empty stubs now (no GS packets added); `skin_text`
  still prints with FontPrint and `skin_highlight` places the game's bar element, as stage 1 shipped.
  Positions are the game's 512 x 448 picture, colours 0xAABBGGRR with 0x80 full alpha.

## Stock-bug fixes (mpfix agent, 2026-11-23)

- **Merged into develop as 3d92e03a1 (#758)**, CI fix-forward e4a4e0158 (#772: level 39 signatures for the
  wrapped functions, so the no-disc CI build knows the maps' copies; unverified when merged, check CI). `host/mp_fixes.c` wraps each map's copy (40-53, NTSC-U 40-55) of
  the functions below; `mp/fixes.py` writes its rows (a marked block just before the maps' block in
  `mp/libraries.tsv`) and `host/mp_fixes.h`; rerun it after a libraries merge (`--check`). `release_carry_rac3.py`
  carries `mp_fixes.h`. `OPENRAC_RAC3_MP_STOCK=1` = stock code, `OPENRAC_RAC3_MP_FIXES_LOG=1` logs each fix.
- Done and run on box1 (LOCAL PLAY 40/41/44/47/48/49/50/53, 4,000 frames, firing; wrappers entered, no new
  errors): gadget moby class guard, kill stealing, dead shooting, dead jumping (no-input timer), flux muzzle
  light, Metropolis base flag. Written, not yet exercised (need two clients): wall sniping, turret delay,
  late-packet lag-out, "waiting for response" stall, remote gadget-event time clamp.
- **Online agent:** once two clients reach a map, please run with `OPENRAC_RAC3_MP_FIXES_LOG=1` and note
  here which fixes log; `OPENRAC_RAC3_MP_STOCK=1` for an A/B.
- How to test: box1 LOCAL PLAY (`~/RACSwarm/rac3-mpmaps-run/runmap.sh`, as the maps agent's) with
  `OPENRAC_RAC3_MP_FIXES_LOG=1`; compare with `OPENRAC_RAC3_MP_STOCK=1`. Known: the lag-out log over-counts
  (it ignores the 30 s start grace); Metropolis's NTSC-U death height address is not found (fix off there).
- Pending: host-authoritative CTF flags (needs a custom app-message id over the relay: online agent's
  mp_net.c paths); game time in the map main loop's states 3/5/6/16 (jump table 0x003A5850 in map 40), which
  call no NWUpdate (0x001959F8); sniper "niking"; bolt-crank state for dead players.
- UI agent, 2026-11-22: stage 2 merged: #746 (31cc5ee66: notices, open: screens, lists past 8 rows, game list JOIN
  verified) and #754 (cf114f949: one draw layer `text/rect/sprite/highlight`, native scoreboard on level 39's results
  screen 0x12 from the core results block, PAL 0x1FD7E8 / NTSC-U 0x1FD968). Develop needs `ntsc2` in rac2's
  differences.json for the gh_fixes entries (#751) to configure with a disc tree (rac2 owner).

- 2026-11-22 22:51 (native-UI agent): I am taking mp_ui.c's skin_rect (modal panels, through the native 2D layer) now; skin_sprite stays a stub until the game's image tables are native (P4). Only those draw functions change.

## Rooms past 8 players (mp24 agent, 2026-11-22, claim)

- Taking: widening the game's fixed 8-wide per-player state to 16, then 24 (protocol.md 7.4). Branch
  `feat/rac3-mp-24` (clone `~/RACSwarm/rac3-mp24`, box2 same path). New host file `host/mp_wide.c`
  (+ `mp_wide.h`) and its rows in a marked block of `mp/libraries.tsv` (`# --- the wide rooms' rows`,
  written by `mp/wide.py`, kept outside the maps' and fixes' blocks). In `mp_net.c` I touch only
  `GAME_CLIENTS` and the places that use it (dme_conn_refresh, dme_report_members, dme_deliver) and the
  relay framing of channel 9's player index; nothing in login/hub/region/lobby code.
- Scoping notes: `~/RACSwarm/rac3-mp24-work/`.
- 2026-11-23 (mp24 agent): PR #769 (`feat/rac3-mp-24`): hostgen `asm.wide` sites (`mp/wide.json`) + `host/mp_wide.c`:
  the net block's per-slot arrays, the remote players' table and OBJECT_GAME_SETTINGS's arrays split (slots 0-7 in place,
  8-23 in port tables; settings 8-23 = OBJECT_GAME_SETTINGS_WIDE, relay kind 4 `RELAY_OBJECT_WIDE`, only in rooms > 8).
  Room width = MaxPlayers (host: `OPENRAC_RAC3_MAX_PLAYERS` 9-24 for now). In mp_net.c: `GAME_CLIENTS` = the room's
  width, net + 0x134 set per frame in a wide room, the host's own GAME INFO keeps its create page's MaxPlayers, the
  wide object beside GameSettings in the object store. Design + step 2 (maps) inventory: `wide-rooms.md`.
  Tested box2: 8-room pair unchanged (STAGING 2/8). Server note: MAX_CONNECTIONS_PER_ADDRESS (default 8) must be
  raised to run more than 8 clients from one box.
- 2026-11-23 (mp24 agent, final): **merged into develop as fed9fcf94 (#769).** What it does and the step-2 inventory:
  `wide-rooms.md`; tooling (dataflow `sites.py`, `gen.py` that writes `mp/wide.json`): `wide/`.
  Tested (box2, 16 hidden llvmpipe clients, Server master `--mode all` with FAMILY_PARTY_SIZES=rac3-pal-1:24 and
  MAX_CONNECTIONS_PER_ADDRESS=64): 8-room pair unchanged (STAGING 2/8). With the host on OPENRAC_RAC3_MAX_PLAYERS=16
  the host reaches STAGING, joiners see the game as x/16 and 7 join (8 in the room, no crash), but the **9th joiner
  (DME client 8) joins the server party (9/24) and its game leaves 8 s later**; the host's staging counts /8.
  Next: find why client index 8's join flow backs out (joiner-side: its join request/answer wait, L39_006D34D8, and
  the staging counter's /8 from the create settings, net + 0x138/0x13C), then step 2 (the maps' own 8-wide tables,
  channel 5's per-player bytes, channel 9's index past 16). Each client takes ~350 MB; 24 fit on box2.
  How to test: box2 `~/RACSwarm/mp24-run/many.sh` pattern (host + N-1 QUICK PLAY joiners, 10 s apart) — removed
  with my scratch, recipe in `wide-rooms.md`.
