# Third-party notices

## Experimental PAL functional reconstruction

`ports/pal-functional/` originates in
[platypet2217-star/RAC2Decomp](https://github.com/platypet2217-star/RAC2Decomp)
at `3e804dc`, imported and subsequently updated by
[rac2-decomp PR #9](https://github.com/llesieur99/rac2-decomp/pull/9).
This snapshot is from rac2-decomp commit
`940aaf1b0e523e8a789b0a17746c7e90fd68a2f8`. The original authorship and full
history are retained in those repositories. The port keeps its complete
[MIT license and copyright notice](ports/pal-functional/LICENSE).

The port is experimental and contributes no matched C bytes. Some comments
in `ports/pal-functional/src/boot_init.c` attribute structures and prototypes
to the Sony SDK. Whether those references were obtained from permitted
binary analysis or from source/headers has not been verified. The existing
attributions remain intact; this is an explicit maintainer review item
under [OpenRAC's sourcing policy](../../../docs/policy/SOURCING.md), not a
certification of source provenance.

## Matching decompilation references

Existing matching-source attribution and toolchain provenance are recorded
in [SECOND-C-LOT.md](docs/SECOND-C-LOT.md),
[RAC1-TO-RAC2.md](docs/RAC1-TO-RAC2.md), and
[COMPILER-NOTES.md](docs/COMPILER-NOTES.md). The new RAC1 library evidence
notes reference upstream GPL files without copying their implementations.
