# Handoff: testing and picking up OpenRAC

This is the document to test from and to come back to. It was last updated on **2026-11-25**, after
the wrap-up of 2026-11-23 to 25.

| Section | What is in it |
|---|---|
| 1. Set up | Get the code and tools running on your machine |
| 2. Where things stand | What works, per game, and what was done in the wrap-up |
| 3. What to test | A checklist for a tester |
| 4. Open work | What is known unfinished |
| 5. How we work | Conventions, merge queue, probes |

The working notes behind this are in [handoff/notes/](handoff/notes/). They were live status
boards, so they mention machine names (box1, box2) that only existed on the previous maintainer's
machines; the technical content is what matters.

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

Words used below: a **PR** is a pull request on GitHub; the **frame recorder** is our native
replacement for the PlayStation's drawing hardware; **hostgen** is the tool that turns the retail
code into C for the PC; **PAL** and **NTSC-U** are the European and North American discs.

### 2.1 Summary

| Game | State |
|---|---|
| Ratchet & Clank (rac1) | Plays through; the most mature |
| Going Commando (rac2) | Plays through; many fixes and new mod hooks this week |
| Up Your Arsenal (rac3) | Single player plays through on PAL and NTSC-U; local and online multiplayer work; many visual fixes this week; a few features are still landing (2.7) |
| Server | Hub and game servers work; **not deployed** (section 4) |

### 2.2 Up Your Arsenal (rac3): fixes merged in the wrap-up

Loading screen and flight

| PR | What it fixed | What to look for |
|---|---|---|
| #832, #844 | World, stars and galaxy on the flight that leaves a planet; smooth approach | Stars fill the whole picture, the planet grows smoothly |
| #836 | Start skips the loading flight and shows the destination planet | Press Start during a flight: it skips, the right planet shows |
| #871 | The flight's exhaust moves between game ticks | No stutter in the ship's flame |
| #876 | Veldin's getters and wrappers paired with their real copies | The flight to Veldin shows Veldin |
| #829 | Ship glass stays on the hull | Glass does not slide off the ship |

Dropship and effects

| PR | What it fixed | What to look for |
|---|---|---|
| #823, #834, #860, #861 | Thruster heat haze drawn natively, as round patches, from the decomp's whole texture setting | Haze under the dropship is round and soft, no squares or wrong colours |
| #869 | Dropship hull drawn opaque (levels format 18) | Hull is solid, not see-through |
| #877 | The Veldin dropship was missing (hostgen dropped it) | Dropship is there on Veldin |
| #839, #868 | Open water drawn natively, as the original's microprogram 10 does (Florana's rivers and eleven more levels) | Rivers and lakes look like water, with the right movement |
| #903 | Water surface textures get mipmaps | No shimmering far away |
| #833 | Inferno Mode draws Ratchet as the Infernox class | Inferno Ratchet looks right |
| #830 | Wrench sweep and camera line tests work | Wrench hits and the camera behave near walls |
| #831 | The 2D layer's flat covers reach the screen's right and bottom edges | Pause and fade borders touch the edges |
| #898 | Synchronous rendering keeps its smoothing across ticks | Motion smooth on OpenGL |

Menus, popups and crashes

| PR | What it fixed | What to look for |
|---|---|---|
| #837 | The Vid-comic password panel stays shut on the Phoenix | No password popup when boarding the Phoenix |
| #899 | The Phoenix "Enter ship" box | The box shows its text properly |
| #896 | Veldin crash: calls through an address hand on the stack arguments | Veldin no longer crashes (message boxes) |
| #897, #917, #921 | Functions filed under the wrong level copies (rac3-decomp catalogue root fix, 2.6) | Fewer odd per-level bugs |
| #906 | Signatures for the map 40–53 head point wraps | Builds without a disc |
| #778 | Map 48 (Aquatos Sewers) camera starts at the head point | No blue picture at the start of the map |
| #809, #817 | Windows and no-disc builds work again | Windows build starts; multiplayer input responds |

Multiplayer and ports

| PR | What it fixed | What to look for |
|---|---|---|
| #769 | Online rooms past 8 players: net layer and the room | Larger rooms can be made (play is still open, section 4) |
| #774 | Split-screen views and the game's 2D screen on the frame recorder; movies on it | Split screen (maps 50–53) shows both players; movies play |
| #779 | rac1 and rac2 hand-off on the frame recorder | rac1/rac2 still draw correctly (opt-in, see section 4) |
| #815 | Sound at the 989snd API and a native float mixer (P11) | Sound effects and music play; no crackle |
| #761, #764, #765, #773, #775 | Menu pictures, text, HUD sprites, panels and map prints through the native 2D layer | Menus, HUD and multiplayer screens look as before |
| #763, #770, #772, #780 | Match seeding from the clock; stock-bug fixes and no-disc signatures | Each match spawns differently; no stuck game time |
| #786, #787 | One OpenRAC server row on Network Startup; NTSC-U multiplayer config | Server row reads right; NTSC-U lobby opens |
| #883 | Leaving multiplayer no longer freezes on the adaptor's power-off | Leave a lobby: no freeze |
| #771, #795 | Scratchpad copies replace the DMA model | No visible change expected |
| #776, #798, #799 | VU0 microprograms as native functions; `$gp` and address fixes | No visible change expected |

Fixes by llesieur99 (their commits, authorship kept)

| PR | What it fixed | What to look for |
|---|---|---|
| #882 | NTSC-U reads frontbin's own scene tables on New Game | New Game starts on NTSC-U |
| #883, #884 | Leaving multiplayer; frontbin's MobyProc and draw side as the boot program's | Menus and mobys draw right |
| #885, #892 | Two functions' return values; an allow entry dropped | Nothing visible; keeps checks green |

### 2.3 Going Commando (rac2) and Ratchet & Clank (rac1)

| PR | Game | What it did | What to look for |
|---|---|---|---|
| #767, #797, #808, #821, #822 | rac2 | Auto-targeting weapons pick enemies off screen; only checked pointers; Siberius cart copies | Zodiac and drones hit off-screen enemies; Siberius cart riders behave |
| #814 | rac2 | Siberius cart: enemy copies stay on their platform; strips follow the hero | Siberius cart fight |
| #794, #796, #810 | rac2 | Mouse turns first-person directly; interpolated mouse camera by default; keyboard and touch leave the free camera | Mouse look feels smooth |
| #811, #872, #825 | rac2 | Bolt totals past 9,999,999 as millions; gun in the right hand in first person; death trace builds | Large bolt counts; gun position |
| #874, #875 | rac2 | Weapon keys, aim, dodge, strafe and a Modern preset; Keyboard and mouse page in the pause menu | Rebind keys; Modern preset |
| #891 (replaces #788) | rac2 | Flight ship parts follow the selected paint (llesieur99). **Export stamp 21** | Flight wings match the hull colour |
| #802, #806, #843, #862, #886 | rac1 | Mouse turrets; imported class updates; particles in expanded memory; freeze screens keep their Field of View; an added planet's name, picture and caption | Turrets aim with the mouse; added planets show a name |
| #866, #873 | both | Mouse selects a menu item only when over it; keys named as the keyboard layout prints them | Menus with a mouse; key names |
| #805, #801, #777, #812, #800, #813, #824 | launcher | Vibration greyed out in rac1; drawer rows; galactic map list; rac1 Android unpack; Mods follows the picked game; save editor caps and NG++ layouts 1 to 4 | Launcher pages |

### 2.4 Mods and SDK (openrac-mods and the SDK)

| Item | What it did | What to look for |
|---|---|---|
| NG++ 0.6.21 | A real Ice Gun and faster Bouncer bomblets each tier | Bouncer bomblets speed up per tier |
| NG++ 0.6.22 | The Ice Gun drawn as a liquid-nitrogen stream that sets enemies in ice (RAC3 style) | Ice Gun looks like a freezing stream |
| NG++ 0.6.23 | Lines at the top of the picture clear after 2.5 seconds (popup timer) | Pickup messages vanish by themselves |
| #826 | Armour tiers keep Carbonox's visor and lamps as an accent (SDK 1.38) | Visor and lamps keep their colour |
| #828 | Paint draws a moby's effects in its colour (SDK 1.39) | Painted effects match the paint |
| #766, #803, #804, #807, #863, #864, #870 | Armour vendor and paint material (1.34); sharper 2D cells (1.36); motors and menu options (1.37); freeze and play hook points; ride-mod API; pick a mod version whose hooks exist | For mod authors; no player change |

Mods build against SDK 1.34.0 as pinned in `sdk.lock` (the mods above use hook points that are
newer, up to 1.39).

### 2.5 Rendering and performance

| PR | What it did | What to look for |
|---|---|---|
| #818 | Cloud shadows and Early Lit cache quality | New lighting options in Enhanced |
| #820, #894, #895 | Fewer per-frame allocations; hero shadow only where its volumes reach; two-texture quads in one draw | Same picture, higher frame rate |
| #816, #879 | Probe baker keeps authored regions; Vulkan shaders compiled during the build | Lighting stable; faster first start |
| #909 | Debug inspector reads each game's own moby fields | F3 inspector values |

### 2.6 rac3-decomp

- Merged PRs #142–#156 (matches in shared level code, frontbin, NTSC-U natives, campaign names).
- PAL report (`games/rac3/decomp/progress`): **16,067 of 16,589 functions** (96.85%), 87.1% fuzzy code match, 540 of 584 units complete.
- NTSC-U report: **15,590 of 16,582 functions** (94.02%), 521 of 572 units complete.
- **Catalogue root fix** (decomp commits 7453660c, 0bf00a6a): `copy_groups()` in `tools/level_catalogue.py` files a level's copies of identical code by their callers' call sites. This removed the per-level workarounds in OpenRAC (#917).
- Pin moved by #762, #840, #841, #842, #880, #881, #889, #917; **#921 (landing)** moves it to 7a8149b8 with #156 and the NTSC-U carry.

### 2.7 Landing (in the merge queue or waiting on CI)

These have passed review and will merge on their own once checks are green. Test them after they land.

| PR | What it does | What to look for |
|---|---|---|
| #783 | rac3 native UI rest: textures by the game's tables, Widget3D from the posed mesh, fade fill, HUD placement as item data | Menus, pause menu, HUD edges, memory-card icon and the fade look unchanged |
| #781 | rac3 rooms past 8 players, step 2: the 9th joiner is rejected properly, second-range table sites | A 9th player cannot slip in; 8-player matches unchanged |
| #819 | P10b: VU0 inline assembly as native C on locals (PAL 767 to 599 forced assembly routes) | No visible change; games build and run |
| #789 | P12/P14 native kernel and pads (already merged) | Pads and start-up still work |
| #790 | P4b/P5/P6 effects: frame copies and screen effects done natively | Haze and Nitro effects (not confirmed; section 4) |
| #700 | Audio recipes: shareable sound packs prepared from your own disc (desktop, RAC1 PAL) | Mods > Audio: make a recipe, enable it, hear it |
| #921 | rac3-decomp pin 7a8149b8 (14 level functions, NTSC-U carry) | Nothing visible |
| #887 | Android launcher CI: host shader compiler without X11 | Android build is green |
| #891 | rac2 flight ship paint (above) | See 2.3 |

## 3. What to test

Before you start: **open the launcher and press "Get the levels ready"** for each game. Levels
are exported in a format that changed this week (rac3 format bumps from #844 and #869; rac2
format 21 from #891), and old exports must be redone.

**Up Your Arsenal**
- [ ] Start New Game on PAL and NTSC-U: loading flight shows stars and the planet; Start skips it.
- [ ] Veldin: the dropship is there and opaque; thruster haze is round; no crash on message boxes.
- [ ] Water on Florana and other river levels looks like water and does not shimmer.
- [ ] Phoenix: no password popup; the "Enter ship" box shows its text.
- [ ] Inferno Mode, wrench, pause menu borders, the loading ship's glass.
- [ ] Map 48 (Aquatos Sewers) multiplayer: no blue picture at the start.
- [ ] Split screen (maps 50–53) with two pads.
- [ ] Online: log in, create or join a room, play a match to its end; leave a lobby without a freeze.
- [ ] Sound and music, in several levels.

**Going Commando**
- [ ] Siberius cart and Zodiac or drone off-screen targeting.
- [ ] Flight ship wings match the selected paint.
- [ ] NG++ on all three discs: Ice Gun stream, Bouncer speed, popups clear after 2.5 s, armour visor accent.
- [ ] Mouse look, weapon keys and the Keyboard and mouse pause page.

**Ratchet & Clank**
- [ ] Mouse turrets; menus with the mouse; particles in large levels; an added planet's name and picture.

**Windows**
- [ ] A build without a disc starts and multiplayer input responds (#809, #817).

**Launcher**
- [ ] Mods tab follows the picked game; save editor opens NG++ saves; after #700 lands, the Audio page.

Report bugs with the launcher's crash file or a screenshot.

## 4. Open work

| Area | What is unfinished | Where to start |
|---|---|---|
| rac3 rooms past 8 | The 9th-joiner fix is in (#781); the match results, the match's end and NTSC-U widening are not done | [rac3-mp-wide-rooms.md](handoff/notes/rac3-mp-wide-rooms.md) |
| rac3 Solana sequence | The first flight's galaxy sequence: the boot flight has no first-flight variant (probably the New Game intro movie, sector 18444); not confirmed | [rac3-port-NO-EMULATION-STATUS.md](handoff/notes/rac3-port-NO-EMULATION-STATUS.md) |
| rac3 blank weapon slot | Blank first-weapon slot after loading a save, one report; not reproduced in six runs | same note |
| #790 effects | Haze and Nitro checks not confirmed in a real run | PR description, audit P4b/P5/P6 |
| Recorder for rac1/rac2 | R1–R3 runs not finished (#779 is in; rac1's title screen drops 2 of 3 commands) | [FRAME_RECORDER.md](port/FRAME_RECORDER.md) |
| Misfiled multiplayer rows | Five `func_L39_*` rows for maps 40–53 (func_L01_00436070, 00443948, 004AA948, 004AAD58, 004D2470) pair through `level_copies`, not names | [rac3-port-NO-EMULATION-STATUS.md](handoff/notes/rac3-port-NO-EMULATION-STATUS.md) |
| NTSC-U-only twins | `func_L01_0037DC70` and `func_L39_004B5BE8` exist only in NTSC-U | rac3-decomp notes |
| No-emulation packages | P2 (markers, per-player views), P4b/P5/P6, P10b, P11, P12, then P7/P8 (DMA-chain walk, GS interpreter) | [audit](handoff/notes/rac3-port-no-emulation-audit.md) |
| Hub | Server not deployed: the launcher defaults to `hub.openrac.dev:7177` and nothing runs there; set `REGION` and `SERVER_NAME`, tag a Server release | Server `docs/README.md` |
| Decomp matching | rac3-decomp: 522 PAL and 992 NTSC-U functions are still unmatched; about 90% of rac3's code by size is still translated from the retail binary | rac3-decomp README and CONTRIBUTING |

Other open PRs (not part of this wrap-up): #912, #918, #907, #901, #865, #655, #639, #867 (drafts
and feature work from other contributors); check `gh pr list --state open` for anything newer.

## 5. How we work

- Conventional Commits. PRs go into OpenRAC `develop`; Server and openrac-mods take commits on `master`; rac3-decomp on `master`.
- **Coding conventions** are in [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md) (moved there in #890), with a reuse and design section and `tools/dup_check.py`, a check for repeated code.
- **No game data, no Sony SDK source, no leaked material, ever.** Every new piece of drawing, sound or input is native; nothing emulates PS2 hardware.
- **Merge queue (#893).** `develop` has a merge queue and auto-merge is off. Pull request checks are fast; the full checks run in the queue. When a PR's required checks pass, enqueue it:
  `gh api graphql -f query='mutation($id:ID!){enqueuePullRequest(input:{pullRequestId:$id}){mergeQueueEntry{position}}}' -f id=$(gh pr view <N> --json id -q .id)`
- **Games are built by hand on PRs** (#905, #900): a PR builds one version per game, and the dev release (#916) builds everything on its own PR. Run the game builds yourself when you touch drawing, sound or hostgen.
- Keep every version building (PAL, NTSC-U, v2.00 ...); the release carry tools under `port/tools/hostgen/` regenerate the other versions' tables.
- Fix others' PRs with commits on top; never rewrite their commits.
- **Probes** for debugging: `OPENRAC_HAZE_LOG` (dropship haze) and `OPENRAC_MOTION_LOG` (tick-boundary motion and smoothing). More switches in [port/DEBUGGING.md](port/DEBUGGING.md).

FINALIZED
