# openrac-rac2-ntsc2: Going Commando, USA v2.00 (Greatest Hits)

Going Commando's USA v2.00 disc (`SCUS_972.68`, Greatest Hits): rac2-ntsc's program at this
release's addresses, read through the release view of [games/rac2](../../../games/rac2/decomp) (`src/` and `src/us2/`). This
disc: same layout as v1.01 with its fixes, its own text ids and about 1,100 functions of its own ([RAC2_NOTES.md](../../../docs/port/RAC2_NOTES.md#what-differs-between-the-discs)).

```sh
python3 tools/build_port.py rac2-ntsc2 --disc <your disc image>
```

Play it as rac2-ntsc's, or from the launcher, which picks this program for this disc.

Everything here is generated except `release/release.json` and the release's own hand C it names in
`hand/`: how, and what each file is, is in
[rac2: carried releases](../rac2/README.md#carried-releases).
