<!--
Keep this short. A reviewer must be able to reproduce your result with the command you paste.
-->

## What this changes

<!-- One or two sentences: which file(s), and why. -->

## Type

- [ ] A matched function (C that reproduces retail bytes)
- [ ] A level placement / catalogue change
- [ ] Tooling or tests
- [ ] Documentation

## Proof

<!--
Every match needs a proof: the exact command, and its result.
Example:
  python scripts/check_candidates.py --reference <boot.elf> --toolchain <ProDG-3.01> --runtime <dir>
  -> all catalogued symbols "matched": true, "different_bytes": 0
-->

- Command:
- Result:

## Checks

- [ ] `python -m unittest discover -s tests -v` passes
- [ ] Every changed program still gates: all loaded bytes of both PT_LOAD segments compare
      equal to retail (boot, and each level overlay touched)
- [ ] No retail-derived file in the diff — no `boot.elf`, overlays, disc images, extracted
      `.bin`/`.o`, `asm/`, `build/`, runtime output
- [ ] Pinned identities untouched, or the catalogue hashes were regenerated together
      (`config/target.json`, `config/overlays.json`, `progress/candidates.json`)
- [ ] Provenance: everything here comes from the retail disc I own or from my own work; any
      external source is cited as reference only
