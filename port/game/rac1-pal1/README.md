# openrac-rac1-pal1: Ratchet & Clank, PAL v1.00

Ratchet & Clank's PAL disc v1.00 (`SCES_509.16`), built from PAL v2.00's C
([rac1-pal](../rac1-pal/README.md)) with rac1-decomp's own C where this disc differs
(`games/rac1/decomp/src/versions/pal_v1/`).

Both PAL discs boot the same file name; the launcher tells them apart by the executable's size
and SHA-1.

```sh
python3 tools/build_port.py rac1-pal1
port/build/release/openrac-rac1-pal1 --data <data> --levels <its levels> --window
```

Its levels are its own disc's (the launcher's "Get the levels ready" writes
`assets/port/rac1/pal1`); the options are rac1-pal's.

Everything here except `release.json` and this file is generated: how, and what each file is, is in
[rac1: carried releases](../rac1/README.md#carried-releases).
