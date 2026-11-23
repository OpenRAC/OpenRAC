# Handoff: picking up OpenRAC

This is the starting point for team members taking over. It assumes nothing about your machines:
you set up from the public repositories, your own discs and ordinary tools. Read section 1 to get
running, section 2 to see where things stand, then pick something from section 3.

The working notes from the last stretch of work are in [handoff/notes/](handoff/notes/). They were
written as live status boards, so they mention machine names (box1, box2) and folders that only
existed on the previous maintainer's machines; the technical content (addresses, causes, designs)
is what matters.

## 0. Known blocker: rac3 falls through the ground

On develop since #771 (P9, scratchpad DMA removal, merged 2026-11-23), Ratchet falls straight
through the ground in rac3: there is no floor collision. Fix this first; most rac3 testing depends
on it.

Likely cause: rac3's collision checks copy the nearby collision data into the scratchpad with DMA
transfers and test against it there. #771 replaced the modelled DMA with plain copies made at
translation time, and made DMA-register stores from decompiled C do nothing. If a collision
transfer is started from C (not from the assembly hostgen rewrites), the copy never happens and the
collision code finds no triangles. A second, less likely suspect is #771's new collision-line cut
(`port/game/rac3-pal/host/coll_line.c`) shortening ground probes.

First steps: build develop at the commit just before #771 (`553c51d5a^`) and confirm the ground
holds there; then look at which collision transfers lost their copy (search the generated output
and the C for stores to the scratchpad DMA channel registers) and give them a native copy, the way
#771 did for the assembly kernels.

## 1. Set up

### 1.1 What you need

- Your own disc images for the games you work on (PS2 discs you own; see
  [../baserom/README.md](../baserom/README.md) for the supported discs and file names). Nothing in
  these repositories contains game data, and nothing may be added.
- macOS, Linux or Windows with: git, Python 3.11+, CMake 3.28+, Ninja, Clang (or MSVC on Windows),
  Node 22 with corepack (for the launcher), Rust stable (for the launcher), and for Server a JDK 25
  and Docker (optional).
- Disk: about 10 GB per game set up, more if you build the decomps.

### 1.2 The repositories

| Repository | Branch to work on | What it is |
|---|---|---|
| [OpenRAC/OpenRAC](https://github.com/OpenRAC/OpenRAC) | `develop` (PRs in; `master` only by release PR) | Launcher, native PC port of rac1, rac2, rac3, mod SDK; decomps as submodules |
| [OpenRAC/rac3-decomp](https://github.com/OpenRAC/rac3-decomp) | `master` | Up Your Arsenal decompilation (PAL, NTSC-U) |
| [OpenRAC/Server](https://github.com/OpenRAC/Server) | `master` | Multiplayer hub and game servers (Java, Netty, PostgreSQL) |
| [OpenRAC/openrac-mods](https://github.com/OpenRAC/openrac-mods) | `master` | Mods on the SDK: NG++, the multiplayer mod, randomizers, … |

### 1.3 OpenRAC (launcher and port)

Follow [DEVELOPING.md](DEVELOPING.md); in short:

```sh
git clone https://github.com/OpenRAC/OpenRAC && cd OpenRAC && git checkout develop
python3 tools/sources.py init        # the decomp submodules at their pins; again after every pull
cp /path/to/your/discs/*.iso baserom/
cd launcher && corepack enable && yarn install && yarn tauri dev   # the launcher, from source
```

The launcher sets each game up from your disc (it unpacks the disc's files, exports the levels and
builds the game) the first time you press Play. To build the port by hand instead:
`cd port && cmake --preset release && cmake --build --preset release && ctest --preset release`.
A game's data folder needs the disc unpacked first: `python3 tools/extractor.py -g <game> -e
--proj-path <data folder> <disc.iso>`. Debugging switches (`OPENRAC_*` variables, test runs, input
scripts, screenshots) are in [port/DEBUGGING.md](port/DEBUGGING.md).

Building rac3 is heavy: its code generator (hostgen) takes 3–5 GB of memory per run. On a 32 GB
machine build one rac3 target at a time.

### 1.4 Server

```sh
git clone https://github.com/OpenRAC/Server && cd Server
./gradlew build                      # tests; PostgreSQL ones skip without a database
java -jar services/server/build/libs/openrac-server-*.jar --mode all   # hub on 7177 + a game server on 7770
```

Or `docker compose up` for PostgreSQL, the hub and a game server. Settings: `.env.defaults` and
`docs/configuration.md`. To play against it, set the launcher's multiplayer server (Settings >
General) to `127.0.0.1:7177`. Coding conventions are strict and checked by CI:
`docs/CODING_CONVENTIONS.md`, `scripts/check_conventions.py`.

### 1.5 openrac-mods

```sh
git clone https://github.com/OpenRAC/openrac-mods && cd openrac-mods
python3 tools/fetch_sdk.py           # the SDK version sdk.lock pins (1.34.0)
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
```

To build against your own OpenRAC checkout's SDK: `-DOPENRAC_SDK=/path/to/OpenRAC/sdk`. A built mod
goes in the game's data folder under `plugins/<mod>/` (the launcher's Mods tab does this for
released mods).

### 1.6 rac3-decomp

See its README. Matching needs the SN toolchain set up as its docs describe; you only need this if
you want to decompile functions (which also shrinks the code the port still translates from the
retail binary, section 2.3).

## 2. Where things stand (2026-11-23)

### 2.1 Up Your Arsenal (rac3)

- **Single player** plays through on PAL and NTSC-U. Much was fixed recently (crashes, functions
  that did nothing, cutscene and loading screens, effects); expect remaining visual bugs.
- **Multiplayer, local**: LOCAL PLAY loads and plays every map (PAL 40–53, NTSC-U 40–55); 50–53 are
  split screen with two pads.
- **Multiplayer, online**: login at the hub, region list, lobby, create/find/join, staging, START
  and the match run against our Server. Accounts, ladders and career stats go to the hub.
- **Lobby screens** are drawn by a game-neutral UI module, `port/mpui`
  ([port/MULTIPLAYER_UI.md](port/MULTIPLAYER_UI.md)); a server can describe them.
- **The protocol** is mapped completely: [handoff/notes/rac3-mp-protocol.md](handoff/notes/rac3-mp-protocol.md).
- Known open: split screen draws one view; map 48's blue tint (draft fix, section 3); rooms above
  8 players are half done (the net layer is in; in-match tables are not); the scoreboard was never
  seen at a real match's end; the stock multiplayer bugs listed in
  [handoff/notes/rac3-mp-horizon-patches.md](handoff/notes/rac3-mp-horizon-patches.md) are partly fixed.

### 2.2 Going Commando (rac2), Ratchet & Clank (rac1)

Both play through. rac2 had a long run of fixes (enemy copies, poses, vendor, Aranos Prison, wrench
models, Greatest Hits backports as optional settings). rac1 is the most mature.

### 2.3 The no-emulation work (all three games)

The rule: the games' logic is their decompiled C; nothing emulates PS2 hardware (no GS packets,
no VU/VIF/DMA models, no IOP/SPU models, no disc sectors), not even as an intermediate format.
The full plan with every remaining layer and its native replacement, as work packages:
[handoff/notes/rac3-port-no-emulation-audit.md](handoff/notes/rac3-port-no-emulation-audit.md);
package states: [handoff/notes/rac3-port-NO-EMULATION-STATUS.md](handoff/notes/rac3-port-NO-EMULATION-STATUS.md).

Done: the native frame recorder ([port/FRAME_RECORDER.md](port/FRAME_RECORDER.md)); files instead
of disc sectors; rac3 text, menus, pictures and HUD through the shared 2D core `port/ui`; texture
identity from the game's own data ([port/TEXTURES.md](port/TEXTURES.md)); the scratchpad DMA model
removed; VU0 microprograms as native functions.

Open (some have draft PRs, section 4): rac3's last markers and per-player views (P2), effect
materials, strips and screen effects (P4 phase 2, P5, P6), inline VU0 asm as C (P10b), native sound
(P11), kernel and IOP shims (P12), rac1/rac2 onto the recorder and the 2D core (R1–R3), then
removing the DMA-chain walk and the GS interpreter (P7, P8). About 90% of rac3's code by size is
still translated from the retail binary; only decompiling (rac3-decomp) shrinks that.

### 2.4 Server

Hub and game servers, accounts (bcrypt, session tokens), stats and ladders in PostgreSQL, region
codes (`REGION`: ISO country codes, `EU`, `LOCAL`) and server names (`SERVER_NAME`), server-described
lobby screens, CI and a release workflow (jars and a ghcr.io image on a `v*` tag; nothing deploys
automatically). Docs start at Server's `docs/README.md`.

**Not deployed yet.** OpenRAC's launcher now defaults to `hub.openrac.dev:7177`; until a hub runs
there, online play needs a self-hosted server (section 1.4).

### 2.5 Mods

- **NG++** 0.6.2 (Going Commando New Game++): weapon tiers Alpha–Omega, RYNO II splits and homing,
  armour tiers sold at the real armour vendor with painted Carbonox, challenge-mode scaling, ship
  tiers. Fixed for PAL and v2.00 (addresses, 50 Hz timing). Notes:
  [handoff/notes/ngpp-STATUS-0.6.1.md](handoff/notes/ngpp-STATUS-0.6.1.md),
  [ngpp-armour-vendor.md](handoff/notes/ngpp-armour-vendor.md),
  [ngpp-armour-models.md](handoff/notes/ngpp-armour-models.md).
- **Multiplayer mod** (rac1/rac2) 0.14.1: goes through the hub or straight to a game server.
- Mods build against SDK 1.34.0.

## 3. Pick something

You won't do all of this; choose what fits your interest and setup. Roughly from most to least
self-contained.

**Testing (no coding needed)**
1. Play rac3 PAL and NTSC-U through, especially menus, HUD, pause and vendor screens, loading and
   save screens, and other languages; report with the launcher's crash file or a screenshot.
2. rac3 multiplayer: local play on every map, and two clients against a local Server (section 1.4):
   log in, create, join, play a match to its end (scoreboard).
3. NG++ on all three Going Commando discs: save and reload with tiers, the Ship Shack, armour in
   fights, each weapon's tier perks, the Zodiac and drones hitting off-screen enemies.

**Fixes with a clear lead**
4. Map 48 blue tint: finish and verify draft PR #778 (the camera's first frame starts under the
   water line). Notes: rac3-mp-STATUS.md.
5. rac3 rooms above 8 players: the 9th joiner backs out, then the maps' 8-wide tables
   ([handoff/notes/rac3-mp-wide-rooms.md](handoff/notes/rac3-mp-wide-rooms.md)).
6. The remaining stock multiplayer bugs (rac3-mp-horizon-patches.md, class "a").
7. Split screen: per-player views in the frame recorder (P2).

**Larger engine work (the no-emulation packages)**
8. Any open package in the audit: effects and frame copies (P4b/P5/P6), sound (P11), kernel/IOP
   (P12), rac1/rac2 onto the recorder (R1–R3), inline VU0 (P10b). Each package section says what to
   replace with what; check section 4 for an existing draft first.

**Operations**
9. Deploy Server's hub at `hub.openrac.dev:7177` (or change the launcher default), set `REGION` and
   `SERVER_NAME`, tag a Server release.

**Decompilation**
10. Match functions in rac3-decomp (its README and CONTRIBUTING; batch your matches).

### Working agreements

- Conventional Commits; PRs into OpenRAC `develop`; Server and openrac-mods take commits on `master`.
- No game data, no Sony SDK source or leaked material, ever.
- Every new piece of drawing, sound or input is native; never route through PS2-shaped layers.
- For each game version you touch, keep the others building (PAL, NTSC-U, v2.00 …): the release
  carry tools under `port/tools/hostgen/` regenerate the other versions' tables.

## 4. Open drafts and branches

The state at handoff (2026-11-23). Check `gh pr list --state open` for anything newer. Each draft's
description says what is done, what is left and how to continue. None of the recent drafts has
been built and run in game; start by building one and checking the area it touches.

- #790 (draft) `wip/p4b-p5-p6-native-effects`: wip: p4b-p5-p6-native-effects (unfinished, unbuilt)
- #789 (draft) `wip/p12-p14-native-kernel`: wip: p12-p14-native-kernel (unfinished, unbuilt)
- #788 `fix/rac2-flight-paint-t111`: fix(rac2): bind flight ship parts to the selected paint
- #785 (draft) `feat/vu0-inline-asm-native`: feat(hostgen): VU0 inline assembly in matched C as C on locals (draft)
- #783 (draft) `feat/rac3-native-ui-rest`: feat(rac3): textures by the getters' tables, Widget3D from the posed mesh, fade fill, HUD placement as item data
- #782 (draft) `feat/p11a-sound-api`: feat(port): sound at the 989snd API and a native float mixer (P11)
- #781 (draft) `feat/rac3-mp-24-step2`: feat(rac3): online rooms past 8 players, step 2 (work in progress)
- #779 (draft) `feat/r1-rac12-recorder`: feat(port): rac1 and rac2 hand-off on the frame recorder (R1, draft)
- #778 (draft) `fix/rac3-map48-head-point`: fix(rac3): the maps' camera starts from the head point, not the feet (Aquatos Sewers' blue picture)
- #774 (draft) `feat/rac3-recorder-views`: feat(rac3): split-screen views and the game's 2D screen on the frame recorder; movies on it
- #700 (draft) `codex/audio-recipes-v1`: feat(audio): generate shareable recipes from the player's disc
- #655 (draft) `feat/mod-interfaces`: feat(sdk): tables mods offer each other by name, interface 1.29
- #639 (draft) `feature/editor-new-planet`: feat: OpenRAC Editor support: editor moved out, imported classes, AI control, SDK 1.25

What each recent draft is, in the plan's terms:

| PR | Package | State |
|---|---|---|
| #774 | P2: rac3's last markers, split-screen views, movies on the recorder | Last commit not seen to build |
| #778 | Map 48 (Aquatos Sewers) blue tint: the camera starts from the head point | Unbuilt; a game-logic fix, check against the console |
| #779 | R1: rac1 and rac2 on the frame recorder | Builds; rac1's title screen drops 2 of 3 recorder commands per frame |
| #781 | rac3 rooms past 8 players, step 2 | Unbuilt; the 9th joiner still backs out; turns NTSC-U widening on too early |
| #782 | P11: sound at the 989snd API, native float mixer | Unbuilt against the games |
| #783 | rac3 native UI rest: Widget3D, getter textures, fade, HUD placement | Unbuilt; loading stars and multiplayer images not started |
| #785 | P10b: VU0 inline asm as C | hostgen tests pass; games not built |
| #789 / #790 | P12/P14 kernel, IOP, pads (`wip/p12-p14-native-kernel`) and P4b/P5/P6 effects (`wip/p4b-p5-p6-native-effects`) | Raw work in progress, stopped mid-build |

Recently merged and only compile-checked (worth a first look when testing): #786 Network Startup
page (it keeps its own server-address parser with a `game.openrac.dev` fallback, which should use
the hub default from #744 instead), #787 NTSC-U multiplayer config, #780 stock-bug fixes and the
rac3 PAL link fix, #776 VU0 microprograms, #771 scratchpad DMA removal.

## 5. Status

FINALIZED
