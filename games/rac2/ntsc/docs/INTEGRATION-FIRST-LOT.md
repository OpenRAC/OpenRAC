# First C integration - 2026-10-01

The seven reviewed C functions are now used in the complete boot link, with
**2,521,763 file-backed bytes matching across both original PT_LOAD segments**.
Entry point, load addresses, lengths, memory extents and flags also match.
All seven defined C STT_FUNC symbols have their reviewed address and full size,
and all 72 bytes still match after the complete link.

The remaining assembly uses the established ProDG 2.0 Ps2EeAs reconstruction
route. The C uses the separately measured SN 2.95.3 compiler and GNU ee-as,
with `-O2 -G0 -ffunction-sections`. The original assembly inputs are fragmented
at reviewed function boundaries. Only the selected bodies are removed; padding
and the rest of each text section remain. The linker places C sections among
those fragments and maps original call names to the defined C symbols.

There is no reference-byte patching, post-link trimming or assembly fallback
for a promoted C body. A copied, stale, zero-sized or prefix-only function does
not satisfy the gates. Snapshot/source hashes prevent a source change during
the build from being attributed to an earlier object.

Object-container hashes depend on STT_FILE metadata, including the source path.
The exact new snapshot object is therefore qualified in its own standalone link,
then that **same object hash** is used in the complete link. The exporter checks
this identity instead of comparing it to an object from an unrelated earlier run.

Reproduction: use `scripts/build.py` with both `--toolchain` (assembly) and
`--c-toolchain` (C). The private run produces `object-qualification.json`,
`integration.json`, the link map and gate report. Public evidence is in
`progress/candidates.json` and `progress/integration.json`.

Progress is 72 / 48,788,176 executable bytes: about **0.000148%**, not 100%.
decomp.dev currently displays 0.01% for this small positive value; the JSON
retains the precise fraction and unchanged full-game denominator.
The 27 overlay identities and measured assembly reconstruction remain in scope;
no overlay C is claimed. Seven complete C units are carved out of the former
assembly units without duplicating their bytes. The native runtime and gameplay
remain unverified, and other compiler families still need qualification.
