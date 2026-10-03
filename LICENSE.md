# Licensing

OpenRAC holds several projects, and each keeps its own license. A file is
covered by the license of the closest directory below that has one.

| Directory | License |
|---|---|
| [`games/rac1/pal`](games/rac1/pal/LICENSE) | MIT, Copyright (c) 2026 Kryštof "Lynder063" Malinda. The libgcc sources in `src/libgcc/` keep the GPL with the runtime exception their headers state. |
| [`games/rac1/ntsc`](games/rac1/ntsc/LICENSE) | MIT, Copyright (c) 2026 Mateusz Kłysz. Code reconstructed from libgcc and the EE-GCC patches is GPL-2.0 ([licenses/GPL-2.0.txt](games/rac1/ntsc/licenses/GPL-2.0.txt)); newlib code keeps newlib's licenses ([licenses/COPYING.NEWLIB.txt](games/rac1/ntsc/licenses/COPYING.NEWLIB.txt)); see its [THIRD_PARTY_NOTICES.md](games/rac1/ntsc/THIRD_PARTY_NOTICES.md). |
| [`games/rac2/ntsc`](games/rac2/ntsc/LICENSE) | MIT, Copyright (c) 2026 llesieur99. |
| `games/rac3/ntsc` | **No license stated.** The project has not chosen one; until it does, its authors keep all rights to their work. |
| [`editor`](editor/LICENSE) | MIT, as part of rac1-decomp where it was written ([editor/LICENSE](editor/LICENSE)). |
| Everything else (`tools/`, `docs/`, the top-level files) | **Not chosen yet.** |

Choosing the missing licenses is an [open question](docs/policy/OPEN_QUESTIONS.md#1-licensing).

None of these licenses cover the games. OpenRAC holds no game code or assets:
you supply your own discs ([docs/policy/SOURCING.md](docs/policy/SOURCING.md)).
Third-party code and data used in OpenRAC are listed, with their licenses, in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and in each game's own notices.
