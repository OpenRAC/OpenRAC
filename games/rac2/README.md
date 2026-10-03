# Ratchet & Clank: Going Commando (2003)

Released in Europe as *Ratchet & Clank 2: Locked and Loaded*.

| Version | Directory | Came from | Builds on |
|---|---|---|---|
| NTSC-U, `SCUS_972.68` (disc v1.01) | [ntsc/](ntsc/README.md) | [rac2-decomp](https://github.com/llesieur99/rac2-decomp) | Windows with WSL |

The whole program, the boot executable and its 27 level overlays, already
rebuilds byte for byte from reconstructed assembly. C replaces that assembly
function by function, each body proven against retail before it is
integrated ([ntsc/README.md](ntsc/README.md), [ntsc/docs/START-HERE.md](ntsc/docs/START-HERE.md)).
Some of that C was carried over from Ratchet & Clank, where the two games
share byte-identical functions ([ntsc/docs/SECOND-C-LOT.md](ntsc/docs/SECOND-C-LOT.md),
[ntsc/docs/RAC1-TO-RAC2.md](ntsc/docs/RAC1-TO-RAC2.md)).

Only v1.01 is supported; the Greatest Hits release and other regions differ.
Disc checksums are in [baserom/README.md](../../baserom/README.md); progress
in [progress/](../../progress/README.md).

## Getting started

```sh
cd games/rac2/ntsc
python -m unittest discover -s tests       # 176 tests; Python only, any host
python scripts/doctor.py                   # what your environment has and lacks
```

The setup and build steps, and the toolchains they need, are in
[ntsc/README.md](ntsc/README.md) (Windows with WSL). On macOS and Linux,
[ntsc/host/README.md](ntsc/host/README.md) runs the same pipeline unchanged; on
an Apple Silicon Mac it rebuilt the boot and all 27 overlays byte for byte and
passed every C check (2026-10-03). `scripts/setup.py --iso` takes the image in
OpenRAC's `baserom/` directly. Its runtime directory must be outside
`games/rac2/ntsc`; OpenRAC's ignored `build/` works (`--runtime ../../../build/rac2-runtime`).
