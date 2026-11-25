# Handoff: picking up OpenRAC

This is the starting point for team members taking over. It assumes nothing about your machines:
you set up from the public repositories, your own discs and ordinary tools. Read section 1 to get
running, section 2 to see where things stand, then pick something from section 3.

The working notes from the last stretch of work are in [handoff/notes/](handoff/notes/). They were
written as live status boards, so they mention machine names (box1, box2) and folders that only
existed on the previous maintainer's machines; the technical content (addresses, causes, designs)
is what matters.

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
scripts, screenshots) are in [port/DEBUGGING.md](port/DEBUGGING.md); the code rules are in [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md) (moved here from `port/docs`; section 22 is reuse and design, checked by `tools/dup_check.py`).

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
python3 tools/fetch_sdk.py           # the SDK version sdk.lock pins (1.34.0 today; bump it to build against a newer interface)
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
```

To build against your own OpenRAC checkout's SDK: `-DOPENRAC_SDK=/path/to/OpenRAC/sdk`. A built mod
goes in the game's data folder under `plugins/<mod>/` (the launcher's Mods tab does this for
released mods).

### 1.6 rac3-decomp

See its README. Matching needs the SN toolchain set up as its docs describe; you only need this if
you want to decompile functions (which also shrinks the code the port still translates from the
retail binary, section 2.3).

## 2. Where things stand (2026-11-25)

### 2.1 Up Your Arsenal (rac3)

- **Single player** plays through on PAL and NTSC-U. The last two days fixed the Veldin start
  (the dropship that never flew in, a crash after the first flight, misfiled level getters), the
  loading flight (stars, ship glass, exhaust, destination planet), the dropship's hull and heat
  haze, open water (Florana's rivers and eleven more levels, now with mipmaps), Inferno Mode's
  Ratchet and the Vid-comic panel; expect remaining visual bugs (list in
  [handoff/notes/rac3-port-NO-EMULATION-STATUS.md](handoff/notes/rac3-port-NO-EMULATION-STATUS.md),
  "Open after the 2026-11-24 playtest fixes").
- **Multiplayer, local**: LOCAL PLAY loads and plays every map (PAL 40–53, NTSC-U 40–55); 50–53 are
  split screen with two pads. Map 48's blue tint is fixed (#778). Leaving multiplayer no longer
  freezes on the adaptor's power-off (#883).
- **Multiplayer, online**: login at the hub, region list, lobby, create/find/join, staging, START
  and the match run against our Server. Accounts, ladders and career stats go to the hub.
- **Rooms above 8 players**: the net layer and the lobby's tables are widened; #781 (step 2) fixed the
  9th joiner starting the match and the CREATE page's spinner, but the maps' in-match tables
  (game-mode struct, results block, channel 5/9) are not widened. Design:
  [handoff/notes/rac3-mp-wide-rooms.md](handoff/notes/rac3-mp-wide-rooms.md).
- **Lobby screens** are drawn by a game-neutral UI module, `port/mpui`
  ([port/MULTIPLAYER_UI.md](port/MULTIPLAYER_UI.md)); a server can describe them.
- **The protocol** is mapped completely: [handoff/notes/rac3-mp-protocol.md](handoff/notes/rac3-mp-protocol.md).
- **The decompilation** (rac3-decomp, at pin `0ed13519`): PAL 15,920 of 16,589 functions matched
  (95.97%; 85.26% of code by size, 539 of 584 units complete); NTSC-U 15,517 of 16,582 (93.58%;
  79.38% of code, 513 of 571 units).
- Known open: split screen draws one view until #774 lands (it is in the merge queue); the
  scoreboard was never seen at a real match's end; the stock multiplayer bugs listed in
  [handoff/notes/rac3-mp-horizon-patches.md](handoff/notes/rac3-mp-horizon-patches.md) are partly fixed.

### 2.2 Going Commando (rac2), Ratchet & Clank (rac1)

Both play through. rac2 had a long run of fixes (enemy copies on Siberius's carts and platforms, poses,
vendor, Aranos Prison, wrench models, the gun in Ratchet's right hand in first person, flight ship paint,
bolt totals past 9,999,999, Greatest Hits backports as optional settings), a keyboard-and-mouse pause
page and a Modern control preset (#874, #875, #891 for the paint), and hooks for ride mods and story
queries. rac1 is the most mature: added planets get their own name, picture and caption (#886),
particles draw in levels mounted in expanded memory, and the mouse aims turrets. The hand-off's
recorder work for them (R1) is still a draft, so their checks are open (section 3).

### 2.3 The no-emulation work (all three games)

The rule: the games' logic is their decompiled C; nothing emulates PS2 hardware (no GS packets,
no VU/VIF/DMA models, no IOP/SPU models, no disc sectors), not even as an intermediate format.
The full plan with every remaining layer and its native replacement, as work packages:
[handoff/notes/rac3-port-no-emulation-audit.md](handoff/notes/rac3-port-no-emulation-audit.md);
package states: [handoff/notes/rac3-port-NO-EMULATION-STATUS.md](handoff/notes/rac3-port-NO-EMULATION-STATUS.md).

Done: the native frame recorder ([port/FRAME_RECORDER.md](port/FRAME_RECORDER.md)); files instead
of disc sectors (P13); rac3 text, menus, pictures and HUD through the shared 2D core `port/ui`;
texture identity from the game's own data ([port/TEXTURES.md](port/TEXTURES.md)); the scratchpad DMA
model removed (P9); VU0 microprograms as native functions (P10a); and, since the 23rd, rac3's
dropship heat haze, open water, effect-strip draws and frame copies as native passes (the effects
work, part of P4b/P5/P6, landed as individual fixes rather than as the wip PR).

Not done, by package (states from the status note; PR numbers are in section 4):

- **P2** split-screen views and the game's 2D screen on the recorder: #774, in the merge queue.
- **P10b** VU0 inline asm as C: #785 (hostgen only, tests pass) continued by #819; no game built
  through it yet.
- **P11** sound at the 989snd API and the float mixer: continued as #815 (merged, with fixes and tests);
  not yet played in all games.
- **P12/P14** kernel, IOP shims, pads and cards: #789 holds raw work in progress.
- **R1** rac1/rac2 on the recorder: #779 (draft). **R2/R3** after it.
- **Effects** (P4 phase 2, P5, P6): the native-material and strip work still in the wip PR #790;
  Ties' additive blends 0x58 and 0x68 are drawn as the normal blend.
- Then removing the DMA-chain walk and the GS interpreter (P7, P8).

#888 is a draft reference: one branch with #774, #779, #783, #789 and #790 merged with develop,
built and played on rac3 PAL and NTSC-U on Windows; use it to see that they work together, not to
merge. About 90% of rac3's code by size is still translated from the retail binary; only
decompiling (rac3-decomp) shrinks that.

### 2.4 Server

Hub and game servers, accounts (bcrypt, session tokens), stats and ladders in PostgreSQL, region
codes (`REGION`: ISO country codes, `EU`, `LOCAL`) and server names (`SERVER_NAME`), server-described
lobby screens, CI and a release workflow (jars and a ghcr.io image on a `v*` tag; nothing deploys
automatically). Docs start at Server's `docs/README.md`.

**Not deployed yet.** OpenRAC's launcher defaults to `hub.openrac.dev:7177`; until a hub runs
there, online play needs a self-hosted server (section 1.4).

### 2.5 Mods

- **NG++** 0.6.23 (Going Commando New Game++): weapon tiers Alpha–Omega, RYNO II splits and homing,
  armour tiers sold at the real armour vendor with painted Carbonox, challenge-mode scaling, ship
  tiers. Fixed for PAL and v2.00 (addresses, 50 Hz timing). Since 0.6.2: the Lava Gun's Ice Gun
  tiers are drawn as Up Your Arsenal's stream of liquid nitrogen that sets enemies in ice (painted
  with interface 1.39), the Bouncer's bomblets fly faster each tier, on-screen popups clear after
  2.5 seconds, and the Zodiac and RYNO tier paths were corrected. Notes:
  [handoff/notes/ngpp-STATUS-0.6.1.md](handoff/notes/ngpp-STATUS-0.6.1.md) (older than the version),
  [ngpp-armour-vendor.md](handoff/notes/ngpp-armour-vendor.md),
  [ngpp-armour-models.md](handoff/notes/ngpp-armour-models.md).
- **Multiplayer mod** (rac1/rac2) 0.14.1: goes through the hub or straight to a game server.
- **SDK**: interface 1.39 is on develop (paint that draws a moby's effects in its colour, an accent
  colour, pad motors and menu options, rac1 play-tick hook points, rac2 ride-mod and story hooks).
  Open: two PRs both claim interface 1.40 (#865 pictures of a mod's own in the overlay, #907
  services between mods), so the second to merge renumbers; #901 lets a mod pack set its mods' settings; #655 interface tables by name. The mods repository
  still pins SDK 1.34.0 in `sdk.lock`; NG++'s newer visuals need a launcher whose interface has them
  (the launcher now picks a mod version whose hook points it has, #870).

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
4. rac3 rooms above 8 players: finish #781 or its successor: the maps' in-match tables (game-mode
   struct, results block, channel 5 and 9 companions), then check a real match of 9 or more
   ([handoff/notes/rac3-mp-wide-rooms.md](handoff/notes/rac3-mp-wide-rooms.md), section 3).
5. The catalogue root fix in rac3-decomp's `tools/level_catalogue.py` (`copy_groups()`): it files
   levels' copies of the 8-byte class getters and font wrappers one row along. The port works
   around it with `more_places`/`release.json` rows; the multiplayer maps' func_L39_ rows for
   func_L01_00436070, 00443948, 004AA948, 004AAD58, 004D2470 are still unfixed. It renames rows
   `src/levels/` and the report use, so it needs its own catalogue PR (notes: "Open after the
   2026-11-24 playtest fixes").
6. rac1 and rac2 checks of the recorder work (R1, #779): rac1's title screen drops 2 of 3 recorder
   commands per frame; check every level's anchors and the HUD sides in both games.
7. Two things seen once and not reproduced: the first flight's Solana galaxy sequence (the boot
   flight has no first-flight variant in its transition table; probably the New Game movie) and a
   blank first-weapon slot after loading a save (not reproduced in six runs). Needs a player with
   the original disc to compare.
8. The remaining stock multiplayer bugs (rac3-mp-horizon-patches.md, class "a").
9. Landing the drafts in section 4 (#774 first, as the queue allows): each is written, none is
   fully run in game. Build one and check the area it touches.

**Larger engine work (the no-emulation packages)**
10. Any open package in 2.3: effects and the ties' blends (P4b/P5/P6), sound in all three games
    (P11), kernel/IOP (P12/P14), rac1/rac2 onto the recorder (R1–R3), inline VU0 (P10b). Each
    package section says what to replace with what; check section 4 for an existing PR first.

**Operations**
11. Deploy Server's hub at `hub.openrac.dev:7177` (or change the launcher default), set `REGION` and
    `SERVER_NAME`, tag a Server release.

**Decompilation**
12. Match functions in rac3-decomp (its README and CONTRIBUTING; batch your matches).

### Working agreements

- Conventional Commits; PRs into OpenRAC `develop`; Server and openrac-mods take commits on `master`.
- **Merging into `develop` goes through the merge queue** (#893): pull request checks are fast, the
  full ones run in the queue, and auto-merge is off, so `gh pr merge` is refused. Once the required
  checks pass, enqueue the PR with the GraphQL mutation:
  `gh api graphql -f query='mutation($id:ID!){enqueuePullRequest(input:{pullRequestId:$id}){mergeQueueEntry{position}}}' -f id=$(gh pr view <N> --json id -q .id)`.
- CI builds a game only by hand on a PR (#905: dispatch the workflow when your change needs it),
  with one version per game on PRs (#900); full builds run after merge.
- New code follows [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md); run `python3 tools/dup_check.py`
  before a PR (section 22: reuse before writing).
- Two probes for rendering work: `OPENRAC_HAZE_LOG` (frame-copy strips and the heat haze patches
  made from them) and `OPENRAC_MOTION_LOG` (per-frame interpolation: alpha, pacing, what snapped);
  both in [port/DEBUGGING.md](port/DEBUGGING.md).
- No game data, no Sony SDK source or leaked material, ever.
- Every new piece of drawing, sound or input is native; never route through PS2-shaped layers.
- For each game version you touch, keep the others building (PAL, NTSC-U, v2.00 …): the release
  carry tools under `port/tools/hostgen/` regenerate the other versions' tables.

## 4. Open drafts and branches

The state at the 2026-11-25 wrap-up. Check `gh pr list --state open` for anything newer. Each PR's
description says what is done, what is left and how to continue. Most of the native-port drafts
have not been built and played in game individually; start by building one and checking the area
it touches. "In the merge queue" below means enqueued and about to land.

**In the merge queue**
- #774 `feat/rac3-recorder-views`: split-screen views and the game's 2D screen on the frame recorder (P2); movies on it
- #899 `fix/rac3-memset-in-call-copy`: the executable's memset kept out of misnamed call copies (the Phoenix's "Enter ship" box)
- #809 `codex/rac3-windows-headless-pr`: Windows headless builds and multiplayer input checks

**Native-port packages, open**
- #888 (draft) `codex/native-all-validation`: the five drafts below merged together and played on rac3 PAL and NTSC-U (reference only)
- #819 `codex/p10b-native-vu0`: continues #785 (P10b, VU0 inline asm as C, hostgen)
- #785 (draft) `feat/vu0-inline-asm-native`: the original P10b draft
- #790 (draft) `wip/p4b-p5-p6-native-effects`: effects work in progress, unbuilt
- #789 `wip/p12-p14-native-kernel`: kernel, IOP, pads work in progress, unbuilt
- #783 (draft) `feat/rac3-native-ui-rest`: textures by the getters' tables, Widget3D, fade fill, HUD placement; never built
- #781 `feat/rac3-mp-24-step2`: online rooms past 8 players, step 2 (9th joiner and spinner fixed, maps' tables open)
- #779 (draft) `feat/r1-rac12-recorder`: rac1 and rac2 on the frame recorder (R1)

**Fixes and features waiting**
- #909 `fix/debug-moby-layout`: the debug inspector reads each game's own moby fields
- #891 and #788 `fix/rac2-flight-paint-t111`: bind flight ship parts to the selected paint (two PRs for one change; land one, close the other)
- #887 `fix/green-develop`: Android host shader compiler on a runner without X11/Wayland headers
- #875, #874 `feat/rac2-kbm-pause-page`, `feat/rac2-kbm-comforts`: rac2 keyboard and mouse page, weapon keys, Modern preset
- #867 (draft) `fix-textures`: shadows in front of HUD text
- #865, #907 (SDK 1.40: overlay pictures; services between mods) and #901 (mod packs give settings), #655 (draft) interface tables by name
- #700 (draft) `codex/audio-recipes-v1`: shareable audio recipes from the player's disc
- #639 (draft) `feature/editor-new-planet`: OpenRAC Editor support

Recently merged and worth a first look when testing: #898 synchronous rendering keeps alpha
continuous across ticks, #896 calls through an address hand on stack arguments (the Veldin crash),
#895/#894 effect and shadow draw performance, #893/#900/#905 the CI changes, #883 leaving
multiplayer, #882 NTSC-U New Game scene tables, #876/#897 Veldin level-copy pairings, #839/#868/#903
water, #823/#834/#860 dropship heat haze, #815 native sound (P11), #778 map 48's camera, #786 Network
Startup page (it keeps its own server-address parser with a `game.openrac.dev` fallback, which should
use the hub default from #744 instead).

## 5. Status

FINALIZED
