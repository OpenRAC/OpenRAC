# Ratchet and Clank PAL native PC campaign

Complete the decompilation of Ratchet & Clank (2002), PAL SCES_509.16, and
make the full game playable natively on Windows. Implement, validate and
commit each logical fix locally, then continue with the next task during
the active session. The owner authorized these commits on 2026-10-10.
Pushing and publishing need a specific request.

## Repository and startup map

The configured parent workspace contains two independent Git repositories:

| Work | Repository and location |
|---|---|
| Retail matching C, declarations and match reports | `rac1-decomp/` |
| Native runtime, renderer, hostgen and platform fixes | `OpenRAC-native/port/` |
| Native implementations awaiting matching C | `OpenRAC-native/port/game/rac1-pal/hand/`, registered in `hand.tsv` |
| Shared campaign handoff | This file |
| Machine setup and launchers | Parent `SETUP.txt`, `Enter-Decomp.ps1`, `Build-Native.ps1`, `Run-Native.ps1` |

`OPENRAC_RAC1_PAL_SOURCE` in `port/build/release/CMakeCache.txt` points to the
sibling `rac1-decomp/`. Check that binding at startup. Editing the imported
`games/rac1/pal/` does not change this local build. On a different machine,
use the actual configured source path and its instructions.

Before editing, read both repositories' instructions, inspect their Git
status/staged diffs and recent commits, then read the latest hostgen report
and relevant run log. Check for concurrent work before rebuilding shared
outputs or choosing a function someone is already implementing. Preserve
other work. Use a topic branch if starting from `main`.

## Task selection and acceptance

1. Reproduce the earliest blocker on the normal boot, New Game and first
   level path. Diagnose the missing function, bad translation, invalid
   state or rendering fault from current evidence. Do not assume an old
   log still identifies a missing function: check source and registrations.
2. Prefer matching C in `rac1-decomp/` when recovering game logic. A native
   implementation can unblock play before a match, but must be based on
   retail behavior, tested and explicitly labeled nonmatching. Keep it in
   the native tree. Do not alter PS2 matching code just for host ABI needs.
3. Fix missing callees and state transitions in dependency order. A stub,
   forced success, skipped dialog or disabled stop-on-missing is not a fix.
   Test the behavior and failure/edge cases relevant to the change.
4. Once the current path runs, cover movement, camera, collision, combat,
   gadgets, enemy behavior, death/restart, saves and planet transitions.
   Extend coverage through every level and the ending. Investigate audio,
   cutscenes and rendering defects as game behavior, not just build issues.
5. Continue the remaining executable and overlay decompilation even after
   a playable path exists. Use rank/triage tools and reusable function
   families; preserve evidence when a function is blocked and move to an
   independent task instead of repeating unproductive attempts.

Keep the missing-function trap enabled. `--keep-going` and memory-write
shortcuts must never count as gameplay validation. A frame-count exit, a
title screenshot, and a free-camera level viewer do not prove playability.

## Validation and commits

Run commands from their documented directory and stop on a failed exit
code. PowerShell does not automatically throw on native command failure.

- Matching work: use `rac1-decomp/AGENTS.md` and its workflow. Require
  strict per-function/file checks, executable layout/image checks where
  applicable, and a regenerated `progress/report.json` committed with the
  matching source. Do not weaken build-fidelity or existing test gates.
- Native work on this machine: from the workspace run
  `.\Build-Native.ps1`. It configures the external PAL source, builds the
  release preset and runs CTest with `SDL_VIDEO_DRIVER=windows`. Read and
  address failures; record pre-existing failures separately with evidence.
- Translator changes: also dot-source `Enter-Native.ps1`, then from
  `OpenRAC-native/` run `python -m unittest discover -s port/tools/hostgen`.
  Report skipped tests as skipped, not as validation on Windows.
- Reproduce the affected native scenario using `Run-Native.ps1` or the
  executable with documented inputs. Record exact arguments, input timing,
  environment and level/save state, exit code, log and screenshot paths.
  Save local artifacts under the workspace `.tools/` directory. Never
  overwrite a player's saves; use an isolated test card directory.
- Review the diff and stage only explicit owned files. One coherent fix
  per local commit, with why it changed, validation and limitations in the
  body, plus the assistant co-author trailer required by `CONTRIBUTING.md`.
  Include related tests; keep unrelated fixes in separate commits. Commit
  corresponding changes separately in each repository and cross-reference
  the prerequisite commit when a native change needs decomp changes.
- Update this handoff with the completed task, evidence, remaining blocker
  and exact next action. Use commit subjects for the current commit and
  hashes of earlier commits; do not try to embed a commit's own hash in it.

## Evidence at setup on 2026-10-10

These are observed artifacts, not a fresh full-game validation. Both
repositories already contain uncommitted implementation work; the setup
commit deliberately does not absorb it.

- Decomp baseline: `4b86e4fc`. `progress/report.json` reports 4,214 of 5,109
  functions matched/finished (82.48%) and 78.50% matched code. The report
  includes classified original assembly; these numbers are not pure C
  coverage or proof that all remaining native paths work.
- Native baseline: `6321d39`, branch `setup/windows-native`. The observed
  generated report at `port/build/release/games/rac1-pal/gen/report.md`
  lists 3,798 translated functions, 187 from candidates, 3 translation
  stubs and 1,158 functions without C. Its call frontier is a prioritization
  aid, not exhaustive dynamic coverage.
- Parent `.tools/native-gameplay-tests.log` records 39/39 tests passing.
  Treat this as prior evidence for that working tree, not a test of later
  edits. Older `SETUP.txt` counts and blocker descriptions have been
  superseded by ongoing source changes.
- `.tools/native-run/zone.log` stopped at `func_L00_002E74B0`. A local
  `level_camera_hero.c` and its `hand.tsv` registration now exist.
  `game_freeze.c` also implements the former `func_001FBE80` blocker.
  Review and reproduce before implementing either again. The observed
  `camera.log` was still advancing and does not establish completion.

## Next session handoff

### Crate initialization, 2026-10-10

Local commit subject: `feat(rac1/pal): implement native crate initialization`.
Recovered `func_L00_002D1168` from the PAL retail instructions in a native
hand implementation. It handles independent crates, saved-counter removal,
group support links, moving-platform attachment, stack state propagation,
and list transitions. No matching source or matching report was changed;
this is not an exact PS2 match. Helpers receive overlay globals from the
level entry point so hostgen retains the correct level relocation context.

Validation on the existing working tree:

- `Build-Native.ps1`: 42/42 CTest tests pass; log
  `.tools/native-crate-build-final.log` in the parent workspace.
- `crate_init_test.c` also compiles and passes with the local
  `i686-w64-mingw32-clang.exe -std=c11 -O2 -fno-strict-aliasing
  -ffp-contract=off`, checking EE structure offsets as well as synthetic
  ground, miss, saved-counter, stack, platform and list scenarios.
- Hostgen: 378 units, 3,808 translated functions (includes new helpers),
  192 candidates, 3 stubs, 1,153 without C. Both `units_unreadable` and
  `index_problems` are empty.
- Before: `.tools/native-run/crate-before.log`, exit 2 at frame 2809,
  missing `func_L00_002D1168`. After: `crate-verified.log`, exit 2 at frame
  2809, missing `func_002116A0`. The crate call is passed; first-level
  gameplay is still blocked. Screenshots use the corresponding prefix,
  including `crate-verified-2800.png` (opening movie, not gameplay).

Exact reproduction, from the parent workspace in PowerShell:

```powershell
$p = .\.tools\Invoke-NativeProbe.ps1 -Name crate-verified -Frames 5200 -Press '100:4000:5,1200:8:5,1400:4000:5,1700:4000:5,2200:8:5,2500:8:5,2800:8:5,3700:4000:5,4000:0:150:128:0'
$p.WaitForExit()
$p.ExitCode
```

The local helper launches `port/build/release/openrac-rac1-pal.exe` with
`--data <native>/build/native-data --cards <native>/build/native-test-cards
--window --levels <native>/build/native-data/port --frames 5200 --no-card`.
It sets `OPENRAC_DEBUG=1`, `OPENRAC_UNCAPPED=1`, `OPENRAC_PRESS` to the
sequence above and `OPENRAC_SHOT=200:<workspace>/.tools/native-run/<name>-`.
This is fresh New Game, skips three opening movies with Start, uses no
player save, and keeps stop-on-missing enabled. Exit codes were captured
from the process object after `WaitForExit`.

Existing uncommitted camera, grid, dialog, vector, translator and renderer
changes were present at session start and remain separate. Only this
task's new implementation, test, registration, CMake target and handoff
are staged for the crate commit. The full runtime result depends on that
working-tree baseline; this commit alone does not supply those earlier
startup fixes. `rac1-decomp/tools/organize_asm.py` remains untouched.

### Joint matrix selection, 2026-10-10

Local commit subject: `feat(rac1/pal): implement native joint matrix selection`;
previous fix is `368205a`. Implemented `func_002116A0`, PAL's handwritten
chain selector, in native C. It builds dependency marks and terminal
indices, invokes the existing native `func_00211808`, and copies full
64-byte matrices in request order. Matching code/reports remain untouched.

- `Build-Native.ps1`: 43/43 CTest tests pass;
  `.tools/native-joint-build-final.log`. The selector fixture also passes
  with the 32-bit compiler, exercising duplicate requests, dependency
  unions, terminal bounds, untouched bytes and retail's zero-count loop.
- Hostgen: 3,809 functions, 193 candidates, 3 stubs, 1,152 without C;
  378 readable units and no index problems. An initial declaration conflict
  was caught by the unreadable-unit gate, corrected to the source's
  `void *, int, int *, void *` signature, and rebuilt before runtime testing.
- Same reproduction command/environment as above, with
  `-Name joints-verified`: exit 2, frame 2809, now at `func_00218928`.
  Artifacts: `.tools/native-run/joints-verified.log` and
  `joints-verified-2800.png`. This verifies passing the selector call, not
  complete pose fidelity or playable Veldin. Existing evaluator limitations
  (for example post-scale chain behavior) remain outside this selector fix.

### Particle allocation, 2026-10-10

Local commit subject: `feat(rac1/pal): implement native particle allocation`;
joint selection was committed as `ba91985`. Implemented the two public
handwritten entries `func_00218928` and `func_00218930` using the PAL body
through `00218A74`. Preserve allocation order, bitmap/high-water/count
updates, NULL on exhaustion and the original clearing of only 32 bytes of
each 64-byte record. The reverse reuse path and secondary bitmap stores
are unreachable behind unconditional retail jumps; no new fallback was
introduced. These functions remain classified assembly in the decomp.

- `Build-Native.ps1`: 44/44 CTest tests pass;
  `.tools/native-particle-build.log`. The fixture fills all 2,048 slots,
  checks sparse holes and byte-aligned rescan order, exhaustion, untouched
  secondary bitmap and preserved payload bytes.
- Hostgen: 3,811 functions (including helpers), 195 candidates, 3 stubs,
  1,150 without C; 378 readable units, no index problems.
- Same New Game probe with `-Name particles-verified`: exit 2, frame 2828,
  now at `func_L00_00217AE8`. Runtime logs entry into the native level-0
  draw path with 199 models, but this does not prove visible gameplay.
- A second probe with `-Name particles-level -ShotEvery 2820` (all other
  arguments unchanged) also exits 2 at 2828. Its frame-2820 PNG was viewed
  and is black, consistent with being early in the transition; no visible
  Veldin scene or player control has yet been demonstrated.

Next action: correct the native candidate for `func_L00_00267290`.
`rac1-decomp/nonmatching/game/func_L00_00267290.c` declares and twice calls
the nonexistent `func_L00_00217AE8`. Retail calls at `002673D4` and
`002674E8` name the existing `func_00217AE8` (its Veldin copy is at
`002677B8`, as `config/overlays/functions.tsv` records). Correct the call
binding while preserving the real stream/event processing; do not add a
no-op for the nonexistent symbol. Then rebuild and repeat the same probe.

Full completion requires all recoverable game code accounted for, no
unimplemented required native calls, documented native replacements for
console-specific assembly, matching audits passing without new mismatches,
and recorded native gameplay validation from New Game through the ending
with save/reload, level transitions, controls, graphics, audio and cutscenes.
Neither matching percentages alone nor a passing CTest suite proves this.
