# Third-party and earlier notices

This repository is licensed under the GNU General Public License v3 (LICENSE). Code
released earlier under the MIT licence keeps its notice below, as that licence requires.

## The project up to RAC2 d46a3539 (2026-10-11)

```
MIT License

Copyright (c) 2026 llesieur99

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
```

## Lombyte (tools/compiler/patches/, tools/compiler/sdk/build.py, src/boot/sdk/ from it)

MIT License, Copyright (c) 2026 Mateusz Kłysz. Same terms as the MIT text above.

## ports/pal-functional/

MIT License, Copyright (c) 2026 platypet2217-star: ports/pal-functional/LICENSE.

## Public runtime adaptations

- The shared EE transfer helpers in `include/common.h` are adapted from
  [OpenRAC/rac1-decomp](https://github.com/OpenRAC/rac1-decomp/blob/69c3e42d792adb90fabb90184a4fac915a48f5b2/include/common.h),
  licensed GPL-3.0. `qcopy` and `qcopy_nc` retain the R5900 transfer sequence;
  RAC2's `qzero` additionally declares a memory clobber for surrounding C accesses.
- `tools/regalloc.py` adapts the register-dump parsing approach from
  [RAC1 at 948d2252](https://github.com/OpenRAC/rac1-decomp/blob/948d2252ef6b7885418fa980db7ed9e4ec0fefa6/tools/regalloc.py),
  itself adapted from RAC3, under GPL-3.0. Its source header records the reference
  and the locally implemented isolation and comparison behavior.

- `src/core/00117278.c` adapts Newlib's `_strtol_r`, Copyright (c) 1990 The Regents
  of the University of California. The complete Berkeley three-condition notice,
  as retained by public Newlib after its advertising-clause removal, is in that file.
- `src/core/0011FC68.c` and `src/core/00121450.c` adapt public GNU GCC
  `libgcc2.c`/`longlong.h` at `31cf01446d`. Both files retain the original FSF
  copyright notices, GPL-2.0-or-later terms, disclaimer and libgcc linking exception.
- `src/core/0011C090.c` adapts RAC1's public `func_00119F38` under GPL-3.0;
  its pinned source and RAC2-specific changes are recorded in `CREDITS.md`.
- `src/core/00122630.c`, `src/core/00122800.c` and `src/core/00122B00.c` adapt GNU GCC's public
  `fp-bit.c`. Its complete FSF
  1994–1998 copyright, GPL-2.0-or-later terms, historical linking permissions,
  disclaimer and contributor credits are preserved in the source, as with the
  existing single-precision sibling `src/core/001232F0.c`.

Pinned source references and the extent of each adaptation are listed in
[CREDITS.md](CREDITS.md). These are public runtime sources, not proprietary SDK
sources or headers.
