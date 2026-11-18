# openrac-rac1-jp: Ratchet & Clank, NTSC-J

Ratchet & Clank's NTSC-J disc (`SCPS_150.37`, v1.02, 60 Hz), built from PAL v2.00's C
([rac1-pal](../rac1-pal/README.md)) with rac1-decomp's own C where this disc differs
(`games/rac1/decomp/src/versions/jp/`).

Its text and fonts are Japanese. The shared FontPrintWindow replacement calls this disc's own
routine (`RAC1_FONT_PRINT_WINDOW`); the small font's replacement and the loading picture's wrapper
are left out (`omit` in `release.json`).

```sh
python3 tools/build_port.py rac1-jp
port/build/release/openrac-rac1-jp --data <data> --levels <its levels> --window
```

Its levels are its own disc's (the launcher's "Get the levels ready" writes
`assets/port/rac1/jp`); the options are rac1-pal's.

Everything here except `release.json` and this file is generated: how, and what each file is, is in
[rac1: carried releases](../rac1/README.md#carried-releases).
