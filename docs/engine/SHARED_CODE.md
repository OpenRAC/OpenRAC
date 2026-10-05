# Code shared between the games

How much machine code the five versions have in common, measured from the
discs, and what each project can take from the others today. The tables are
generated: [shared/xmap/README.md](../../shared/xmap/README.md). This page
explains the method, reads the first results (2026-10-04) and says what to do
with them.

## What was measured

[tools/xmap.py](../../tools/xmap.py) reads every program a version has: the
boot executable's `core.text`, `.text` and `net.text`, RAC3's `frontbin.elf`,
and each level's code (19 in RAC1, 27 in RAC2, 51 in RAC3, 47 in Deadlocked).
It cuts each code section into functions the same way for every game, and a
function that repeats in several levels counts once. Each function gets two
fingerprints ([tools/mips.py](../../tools/mips.py)):

- **strict**: only address fields are masked (jump targets, the halves of
  RAM addresses, `$gp` offsets). Two functions share it only when they are the
  same function at another address.
- **shape**: constants and struct offsets are masked too. Two functions that
  differ only in a number share it; they are relatives, not copies.

It also fingerprints the functions as each project itself cuts and lists
them, with which ones the project has matched in C, so "matched there, open
here" can be answered whatever the two projects call a function.

The rules are the ones that built rac1/pal's US to PAL map
([OVERLAYS.md](../../games/rac1/pal/docs/OVERLAYS.md)). The strict figure is a
floor: that map pairs 84% of the US code by identical fingerprint and most of
the rest by similarity between those anchors, a step this tool does not take
yet.

## Results

**How much distinct code there is.** Far less than the totals the projects
count, because level code repeats: RAC2's 27 overlays hold 5.05 MB of
distinct code against the 48.8 MB its progress scope sums.

| Version | Distinct functions | Distinct code |
|---|---:|---:|
| rac1/pal | 4,827 | 3.66 MB |
| rac1/ntsc | 4,827 | 3.65 MB |
| rac2/ntsc | 8,056 | 5.05 MB |
| rac3/ntsc | 14,436 | 7.02 MB |
| rac4/ntsc | 15,235 | 5.39 MB |

**What is shared.**

- *RAC1's two regions*: 75.6% of the bytes are identical functions, and 98%
  of the core. The rest differs in more than addresses, which is why the PAL
  project pairs it by similarity.
- *The engine core carries over; the game does not.* RAC1 and RAC2 share 70%
  of RAC1's `core.text` (81 KB) but 8% of the game code and almost no level
  code. RAC3 and Deadlocked share 69% of RAC3's core (106 KB) and 567 KB in
  all, 8 to 10% of each, the largest overlap between two games. RAC1 shares
  27% of its core with RAC3 and with Deadlocked.
- *In every version*: 319 functions, 38 KB, of which 30 KB is in the core,
  at the addresses of Sony's libraries and the C runtime. That is the code a
  shared library tree would hold first.
- *Level code is each game's own*: under 1% is identical between two games,
  except RAC3 and Deadlocked (6 to 10%).

**What each project can take from the others.** 3,765 distinct functions
(960 KB) are matched in at least one project. Matched elsewhere, identical in
this version's code, and still open here:

| Version | Functions | Bytes | Compared with what the project has matched |
|---|---:|---:|---|
| rac1/pal | 245 | 125,876 | +19% |
| rac1/ntsc | 186 | 60,020 | +8% |
| rac2/ntsc | 437 | 62,472 | 4.6 times as much |
| rac3/ntsc | 192 | 22,252 | +25% |
| rac4/ntsc | 282 | 29,872 | 1.1 times as much |

Most of rac1/pal's come from Lombyte (228 functions, 124,888 bytes, among
them `_dtoa_r` at 4.5 KB), and most of RAC2's from the two RAC1 projects
(about 295 functions, 48 KB). The lists, one per pair, are in
[shared/xmap/ports/](../../shared/xmap/ports): source name and address, target
program and address, size, and whether it is the same function or a relative.

## Using a port candidate

A candidate marked `same` is the same machine code in both games, so the C
that produces it in one game produces it in the other **when built the same
way**. That last part is the catch: the projects do not agree on compilers
([docs/toolchains](../toolchains/README.md)). So:

1. Take the C from the project that matched it, with its license and a
   provenance note, as the projects already do
   ([workflow](../workflow/README.md#6-porting-between-versions-and-games)).
   RAC3's code is GPL v3; it can only move into an MIT tree with its authors'
   agreement ([LICENSE.md](../../LICENSE.md)).
2. Rename its symbols to the target's (addresses differ, and each project
   names by its own).
3. Build it with the target project's own tools and pass that project's own
   check. If it does not match there, the two projects compile that code
   differently: that is evidence for the compiler question, worth writing
   down, not a failed port.

A `shape` candidate needs its constants and offsets adjusted; treat it as a
strong draft.

Byte-identical library code deserves a note: where a function comes from a
prebuilt archive (Sony's libraries, libgcc), every game that linked the same
archive has the same bytes, whatever compiled the game itself. Those ports
depend least on the compiler question.

## Running it

`scan` needs each game's code on disk, which each game's setup produces
(nothing is committed; `build/xmap/` is ignored):

| Version | What it reads | Produced by |
|---|---|---|
| rac1/pal | `games/rac1/pal/baserom/SCES_509.16`, `baserom/overlays/level_*` | `openrac.py setup rac1/pal`, then `python3 tools/overlays.py dump` there |
| rac1/ntsc | `games/rac1/ntsc/config/us/SCUS_971.99`, `config/us/overlays/level_*` | `openrac.py setup rac1/ntsc`, then its `setup.sh` |
| rac2/ntsc | `build/rac2/runtime/runs/*/reference/` | `scripts/setup.py --runtime build/rac2/runtime` ([host recipe](../../games/rac2/ntsc/host/README.md)) |
| rac3/ntsc | `build/rac3/wrench_out/uya_scus_973_53/` | the Wrench unpack on [rac3's page](../../games/rac3/README.md#getting-started) |
| rac4/ntsc | `games/rac4/ntsc/baserom/SCUS_974.65.elf`, `build/rac4/wrench_out/levels/` | `bash tools/setup_asm.sh` there, and `wrenchbuild unpack ... -o build/rac4/wrench_out -g dl -r us` |

```sh
python3 tools/xmap.py scan                       # all five take about five seconds
python3 tools/xmap.py report                     # shared/xmap/summary.json and README.md
python3 tools/xmap.py ports rac1/ntsc rac1/pal   # one pair's candidates
```

Rerun after a project lands matches or changes how it lists functions, and
commit `shared/xmap/` as `chore(shared)`.

## Limits

- **A floor, not the whole overlap.** Only identical functions and exact
  shapes are found. Functions that changed by an instruction or two between
  games, the next largest group, need the similarity step.
- **Cuts differ.** A function one project cuts in two is not found whole in
  another. Each version is indexed both ways (uniformly, and as its project
  cuts it), which recovers most cases, not all.
- **"Matched" means what each project says.** RAC2 lists only the functions
  it has matched, so nothing is known about the rest of its list. RAC3's 33
  common level functions are not read yet. Deadlocked's matches rest on a
  looser check ([games/rac4](../../games/rac4/README.md#what-matched-means-here)).
- **Small functions prove little.** Fingerprints under 16 bytes are ignored,
  shapes under 48.

## Files, not only functions

The same comparison on the source trees finds 16 files that two games build
with unchanged: GCC's libgcc sources and build helpers in rac1/pal and rac4,
and label macros in rac3 and rac4. [shared/files.json](../../shared/files.json)
lists them, and `python3 tools/shared.py check` keeps their copies identical
([shared/README.md](../../shared/README.md)).

## What comes next

In order ([open question 6](../policy/OPEN_QUESTIONS.md#6-code-shared-between-the-games)):
port the candidates above, each proven in its target; add the similarity step
for the functions that changed slightly; then give the library code that is
in every version one home that every game's build compiles and checks.
