# To do

Larger pieces of work that are scoped and decided but not started. Each
section gives the goal, what is already known, how to do it, how to check it,
and where to begin. If you take one on, say so in its section (name and date),
and remove the section when it lands.

Known bugs that are guarded but not properly fixed are in
[port/KNOWN_ISSUES.md](port/KNOWN_ISSUES.md).

## 1. The rest of the bigger moby pool for Ratchet & Clank (port)

**Landed** (2026-10-22): the port gives every level at least 768 spare moby
slots (767 usable, against the disc's 255) except Rilgar (5), Blarg Station
(6) and Quartu (15), and caps the vanilla tick's target list at 511
(`port/game/rac1/host/moby_pool.c`; the limits checked are in
[port/KNOWN_ISSUES.md](port/KNOWN_ISSUES.md)). Left:

- **Check it in game.** A hidden Gaspar run with a temporary census of the
  live slots between the pool's two bounds, by class. The pool should stay
  well short of its new end, and neither "scene actor skipped" nor "moby
  target list full" should appear. Then a level with many placed mobys
  (Kalebo III) with the difficulty mod's enemy count at 2 or more.
- **Vanilla room on Rilgar, Blarg Station and Quartu.** Their level heap ends within
  0 to 65 KiB of 0x1FF8000, so they keep 255. Room would have to come from
  shrinking the per-level VU chain area (f4 to fC of the load state, 0x160000
  to 0x1C0000 twice; the port's high-water mark there is not measured, and f4
  also holds the decompressed gameplay file) or from guest memory past
  32 MiB (several port paths mask addresses with `& 0x01FFFFFF`). The expanded
  expanded path has room for the remaining 15-bit moby indices and is separate
  from this vanilla path.
- **Expanded-mode moby-grid stress.** Expanded RAM now has 32-bit cell
  populations and a 16 MiB block pool; vanilla keeps the original grid.
  Exercise a deliberately dense cell, move/remove colliders from it, and
  exhaust the pool to verify counts, collision queries and rollback. The pool
  exhaustion path keeps a moby's previous rectangle active and returns
  temporary cell-list growth; ordinary removals also shrink underused lists.
  Per-size next-fit cursors keep pool allocation from rescanning the used prefix
  for every cell.
  Dense-cell and allocator-stress gameplay proof is still outstanding.
- **Expanded static collision leaves.** The editor and PAL/NTSC-U runtime now
  support tagged `RLCX` cells with 32-bit counts and indices, and the packer
  selects expanded RAM for those levels. Run a Godot edit that pushes one cell
  past 255 vertices and 4,080 bytes, confirm floors/walls and face filters in
  gameplay, then exercise allocation failure and 0xFFFFFF-directory-entry
  refusal paths. The expanded directory indexes all leaves and carries 32-bit offsets. Mod collision trees now support a positive 131,072-unit world and an optional 32-bit slab-offset directory; large-world gameplay proof remains.

## 2. A refactor pilot for rac1-decomp

**Goal.** Readable C with real names and types, while every build stays
byte-identical. Start with a pilot on one subsystem (the camera is a good
first one), to measure how well the method works before scaling it up.

**The method:** design centrally, edit in parallel.

1. **A designer.** One strong model or person writes the subsystem's shared
   types (structs with field names) and a name registry for its functions,
   globals and fields. The names come from evidence: call sites, strings, and
   what the rac2 and rac3 decompilations call the same things.
2. **Workers on bounded jobs.** Each takes one file and applies the registry:
   use the structs, the registry's names and descriptive locals. Workers never
   invent shared names; a name that is missing goes back to the designer.
3. **Automatic gates.** Each file builds byte-identical (rac1-decomp's
   `tools/overlay_file_check.py` and `tools/build_sn.sh`). Nothing new appears
   outside the registry. Each batch gets a full build.

**Why this shape.** A byte-identical build can't tell a wrong name from a
right one, so names must be reviewed, and wrong ones are worse than
`field_1F8`. Shared types touched by many workers at once also cause merge
conflicts and duplicate declarations.

**Verify and review:**
- the gates above;
- a human review of the registry before the workers start, and a sample of
  their files afterwards;
- measure the hit rate (files that pass on the first try) and the
  review-correction rate, to decide whether to scale up.

**Where:** changes go to rac1-decomp through its usual PRs, then sync into
`games/rac1/decomp`.

## 3. Put the ReRAC conversion to use (port)

**Goal.** The parts of the port converted from
[ReRAC](https://github.com/re-rac/rerac) (the disc readers, the extractor, the
geometry and world data readers, sound, movies and audio, the world renderers)
are reviewed, tested and built by default, and the sound library already plays
through `openrac_audio`. What is left is to use the rest of them:

- **Bridge the asset readers to the world renderers.** `port/assets/geometry/`
  and `port/assets/world/` read a level; `port/renderer/world/draw_data.h` is
  the render-ready form the world renderers upload. Nothing builds one from
  the other yet. ReRAC gives RAC1 NTSC-U's `WorldConstants`; a bridge for
  another game or version supplies its own.
- **Renderers for the rest of the world.** Terrain (tfrag) and ties have
  theirs (tested by `world_lod` and `world_gl_smoke`); shrubs, mobys, the sky,
  particles, water and the HUD have draw data but no renderer yet.
- **Drive the viewer from the disc.** `openrac-viewer` draws the editor's
  `port` export ([port/viewer/README.md](../port/viewer/README.md)); it should
  read the level from the player's disc through the asset readers and draw it
  with the world renderers.
- **The launcher on the C++ extractor.** The launcher still runs
  `tools/extractor.py` (`launcher/core/src/install.rs`) and moves to
  `openrac-extractor` once the port is packaged with it
  ([port/extract/README.md](../port/extract/README.md)).

What the sound engine does not do yet (streams through the voices' own
streaming, reverb on streams, movie sound) is in
[port/audio/README.md](../port/audio/README.md). Code adapted from ReRAC
names its source in its header and is credited in
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

**Verify:** the port's tests (`ctest --preset release`) after each step; what
changes the picture, in the maintainers' regression run
([docs/port/REGRESSION.md](port/REGRESSION.md)).
