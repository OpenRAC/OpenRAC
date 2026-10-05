# Licensing

OpenRAC holds several projects, and each keeps its own license. A file is
covered by the license of the closest directory below that has one.

| Directory | License |
|---|---|
| [`games/rac1/pal`](games/rac1/pal/LICENSE) | MIT, Copyright (c) 2026 Kryštof "Lynder063" Malinda. The libgcc sources in `src/libgcc/` keep the GPL with the runtime exception their headers state, and `include/moby_pvars.h` is adapted from Wrench (GPL-3.0-or-later); see its [THIRD_PARTY_NOTICES.md](games/rac1/pal/THIRD_PARTY_NOTICES.md). |
| [`games/rac1/ntsc`](games/rac1/ntsc/LICENSE) | MIT, Copyright (c) 2026 Mateusz Kłysz. Code reconstructed from libgcc and the EE-GCC patches is GPL-2.0 ([licenses/GPL-2.0.txt](games/rac1/ntsc/licenses/GPL-2.0.txt)); newlib code keeps newlib's licenses ([licenses/COPYING.NEWLIB.txt](games/rac1/ntsc/licenses/COPYING.NEWLIB.txt)); see its [THIRD_PARTY_NOTICES.md](games/rac1/ntsc/THIRD_PARTY_NOTICES.md). |
| [`games/rac2/ntsc`](games/rac2/ntsc/LICENSE) | MIT, Copyright (c) 2026 llesieur99. |
| [`games/rac2/ntsc/ports/pal-functional`](games/rac2/ntsc/ports/pal-functional/LICENSE) | MIT, Copyright (c) 2026 platypet2217-star; experimental port with a [provenance review item](games/rac2/ntsc/THIRD_PARTY_NOTICES.md). |
| [`games/rac3/ntsc`](games/rac3/ntsc/LICENSE) | GNU General Public License v3 (since 2026-10-04). Code from this directory can only move into another under the GPL, or with its authors' permission. |
| [`games/rac4/ntsc`](games/rac4/ntsc/LICENSE) | MIT, Copyright (c) 2026 Kryštof "Lynder063" Malinda. `src/libgcc/` keeps the GPL with the runtime exception, and `src/libm/` newlib's fdlibm notice ([THIRD_PARTY_NOTICES.md](games/rac4/ntsc/THIRD_PARTY_NOTICES.md)). |
| [`editor`](editor/LICENSE) | MIT, as part of rac1-decomp where it was written ([editor/LICENSE](editor/LICENSE)). |
| Everything else (`tools/`, `docs/`, the top-level files) | **Not chosen yet.** |

Choosing the missing licenses is an [open question](docs/policy/OPEN_QUESTIONS.md#1-licensing).

None of these licenses cover the games. OpenRAC holds no game code or assets:
you supply your own discs ([docs/policy/SOURCING.md](docs/policy/SOURCING.md)).
Third-party code and data used in OpenRAC are listed, with their licenses, in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and in each game's own notices.
