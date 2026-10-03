# Original assembly classification

Some retail code was written as assembly, and some four-byte words are
leftovers from functions stripped by the linker. Matching those bytes by
compiling C would misrepresent their origin. The tracked lists are:

- `config/handwritten_asm.txt`: 212 functions marked handwritten by the
  disassembler (83,520 retail bytes).
- `config/linker_remnants.txt`: 69 dead-strip remnants identified by
  `tools/triage.py` (276 retail bytes).

These names were taken from the two corresponding `tools/triage.py` buckets
on 2026-09-27. The `fallthrough fragment` and `epilogue fragment` buckets
remain unmatched because they are incorrect function boundaries to fix.

`tools/setup_asm.sh` regenerates `asm/` from the contributor's own baserom,
then `tools/organize_asm.py` moves the listed files into
`asm/handwritten/<segment>/` and `asm/remnants/<segment>/`. Compatibility
links remain under `asm/nonmatchings/<segment>/` for the differ and retail
inventory. No retail assembly bytes are committed to Git.

The source uses `ASM_FUNC` and `LINKER_REMNANT` to include these files
without changing the linked image. The progress report counts the listed
original assembly as finished work while the build audit continues to
count exact C matches separately. Changing this published progress policy
requires upstream review.
