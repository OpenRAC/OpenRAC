# Third-party notices

Third-party code and data used by OpenRAC's shared components. Each game
directory lists its own in its `THIRD_PARTY_NOTICES.md` and source comments:

- [games/rac1/pal/THIRD_PARTY_NOTICES.md](games/rac1/pal/THIRD_PARTY_NOTICES.md):
  functions adapted from Lombyte, ReRAC's notes in `config/overlays/rerac_notes.tsv`.
- [games/rac1/ntsc/THIRD_PARTY_NOTICES.md](games/rac1/ntsc/THIRD_PARTY_NOTICES.md):
  libgcc and soft-float, newlib, David Gay's dtoa, code ported from rac1-decomp,
  the patched EE-GCC.
- rac2 and rac3 credit their sources in their docs
  ([games/rac2/ntsc/docs/SECOND-C-LOT.md](games/rac2/ntsc/docs/SECOND-C-LOT.md),
  [games/rac2/ntsc/docs/COMPILER-NOTES.md](games/rac2/ntsc/docs/COMPILER-NOTES.md),
  [games/rac3/ntsc/docs/wiki/Cross-Repository-Resources.md](games/rac3/ntsc/docs/wiki/Cross-Repository-Resources.md)).

The level editor (`editor/`) is distributed under rac1-decomp's MIT license
([editor/LICENSE](editor/LICENSE)). It uses the following.

## ReRAC

[ReRAC](https://github.com/re-rac/rerac), a native PC port of the US build,
documents the game's formats and systems. The following adapt its format code
and notes:

- `editor/mobys.py`: the moby instance record and what the level loader does
  with each field, from `crates/rc-formats/src/gameplay.rs` and
  `docs/plan/moby_render_notes.md`; described in [ASSETS.md](games/rac1/pal/docs/ASSETS.md) ("Mobys").
- `editor/moby_class.py`: the moby class header and model format (packets,
  vertex cache, skinning slots, normals, untextured faces), from
  `docs/formats/moby_rac1.md` §1-§2 and `crates/rc-formats/src/moby.rs`;
  described in [ASSETS.md](games/rac1/pal/docs/ASSETS.md) ("Moby classes").
- `editor/moby_anim.py`: moby skeletons and animation sequences, from
  `docs/formats/moby_rac1.md` §3-§4, `docs/plan/moby_animation.md` and
  `crates/rc-formats/src/moby_anim.rs`.
- `editor/collision.py`: the collision block (cell tree, packed vertices,
  faces and surface bytes), from `docs/formats/collision_rac1.md`,
  `docs/plan/collision_queries.md` and `crates/rc-formats/src/collision.rs`;
  described in [ASSETS.md](games/rac1/pal/docs/ASSETS.md) ("Collision").

ISC License

Copyright (c) 2026 ReRAC contributors

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## Lombyte

[Lombyte](https://github.com/mateuszklysz/Lombyte), the US decompilation, now
`games/rac1/ntsc`:

- `editor/moby_classes.tsv`: the moby class names the editor shows, from its
  `config/overlays/us/names/level-NN.json` ("moby-class-record" evidence),
  which joins each level's class dispatch table with the class names of
  [Wrench](https://github.com/chaoticgd/wrench)'s moby class unpack.

MIT License

Copyright (c) 2026 Mateusz Kłysz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Wrench and Replanetizer

Consulted for what formats and fields mean; no code from them is included
([editor/README.md](editor/README.md#credits)):
[Wrench](https://github.com/chaoticgd/wrench) (GPL-3.0-or-later) and
[Replanetizer](https://github.com/RatchetModding/Replanetizer).
