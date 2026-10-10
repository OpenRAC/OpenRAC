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

First inspect ongoing native changes and the latest logs. Determine the
current New Game / first-level blocker after the camera work, reproduce it
with the normal game path, and take one unowned fix through verification
and a local commit. If that work is still owned by another session, select
an independent missing decomp function from rank/triage instead. Update
this section when the state changes; do not keep following a stale target.

Full completion requires all recoverable game code accounted for, no
unimplemented required native calls, documented native replacements for
console-specific assembly, matching audits passing without new mismatches,
and recorded native gameplay validation from New Game through the ending
with save/reload, level transitions, controls, graphics, audio and cutscenes.
Neither matching percentages alone nor a passing CTest suite proves this.
