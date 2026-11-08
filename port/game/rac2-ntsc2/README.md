# Going Commando, USA v2.00 (SCUS-97268, Greatest Hits)

`openrac-rac2-ntsc2`: Going Commando's port for this disc, the same program as
[rac2-ntsc](../rac2-ntsc/README.md)'s on this release's addresses. Its decompilation is rac2-decomp's
(`games/rac2/decomp`), read through the release view; the host C, the profile and the game fixes are
shared ([port/game/rac2](../rac2/README.md), which also says how these files are written again).
Nothing here is edited by hand except `release_pairs.tsv`, `release_constants.tsv` and `release_extra.json`.

Build and play it as rac2-ntsc's: `python3 tools/build_port.py rac2-ntsc2 --disc <your disc image>`, or the
launcher, which picks this program when the disc is this release.
