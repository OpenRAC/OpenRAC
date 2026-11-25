# openrac-rac2-pal: Going Commando, Europe/Australia (*Locked and Loaded*)

Going Commando's Europe/Australia disc (`SCES_516.07`): rac2-ntsc's program at this release's
addresses, read through the release view of [games/rac2](../../../games/rac2/decomp) (`src/` and `src/pal/`). This
disc: 50 Hz, localised text, larger front-end objects ([RAC2_NOTES.md](../../../docs/port/RAC2_NOTES.md#what-differs-between-the-discs)).

```sh
python3 tools/build_port.py rac2-pal --disc <your disc image>
```

Play it as rac2-ntsc's, or from the launcher, which picks this program for this disc.

Everything here is generated except `release/release.json` and the release's own hand C it names in
`hand/`: how, and what each file is, is in
[rac2: carried releases](../rac2/README.md#carried-releases).
