# Brief shared by every agent porting ReRAC into OpenRAC

OpenRAC (/home/user/OpenRAC) is a monorepo of the PS2 Ratchet & Clank
decompilations whose goal is a native PC port in the manner of OpenGOAL, with no
emulation. The port lives in /home/user/OpenRAC/port. It is C++20 with CMake and
Ninja, built with clang, and uses OpenGL 4.1 core and SDL3. ReRAC
(/home/user/re-rac/rerac, read-only, ISC license, Rust/Bevy) is a native port of
RAC1 NTSC-U (SCUS_971.99). We are porting what it knows into OpenRAC as C++.

The user's words: "stay C++ and use ReRAC as reference, but building a
differently formatted system with launcher and support across all 4 games.
Take everything you can from there, convert to C++, refactor, reorganize,
change function names, etc."

## Conversion rules

- Convert, don't transliterate. Use idiomatic C++20 with OpenRAC's names and
  structure:
  - namespaces `openrac::<module>`;
  - types in CamelCase, functions and variables in snake_case, members `m_x`,
    constants `kName`;
  - four-space indent, 100 columns, `port/.clang-format`. Run
    `clang-format -i` on every file you write.
  - Do not keep ReRAC's names or file names where a clearer one exists.
- Every new file starts with:
  ```
  // SPDX-License-Identifier: GPL-3.0-or-later
  // Copyright (c) 2026 the OpenRAC contributors
  //
  // Adapted from ReRAC (https://github.com/re-rac/rerac), <crate/path>:
  // ISC License, Copyright (c) 2026 ReRAC contributors.
  //
  // <what the file is, in OpenRAC's plain prose style>
  ```
- Comments say why and where knowledge comes from. EE addresses ReRAC cites
  are NTSC-U (SCUS_971.99) addresses: say so ("NTSC-U 0x...").
- Multi-game: OpenRAC supports rac1 (pal, ntsc), rac2, rac3 and rac4.
  - ReRAC only knows RAC1 (NTSC-U). Shape APIs so another game or version can
    be added: a `Game`/`GameVersion` parameter (port/assets/version.h), or
    per-game tables, where the format is known to differ.
  - Put RAC1's layout in clearly named RAC1 code paths.
  - Never invent facts about rac2–4 formats. Leave a clear "not known yet"
    (an error or an empty table), not a guess.
- Use the foundation:
  - `port/assets/bytes.h`: `ByteView` with checked LE reads, `AssetError`,
    `fail(...)`, `ByteWriter`;
  - `port/assets/version.h`: `Game`, `GameVersion`, `versions()`,
    `find_version*`, `game_of_serial`;
  - the log, `port/common/log.h`: `openrac::log::info/warn/...` with
    std::format;
  - the test harness, `port/tests/check.h`: `CHECK(cond)`;
    `return openrac::test::result();`.
- No third-party dependencies beyond the standard library unless your brief
  says so.
- Include style is relative to port/ (`#include "assets/bytes.h"`).

## Legal rules (hard)

- Never commit or embed game data. That includes:
  - disc bytes, extracted assets or VU microcode bytes;
  - tables of retail bytes, generated assembly or disassembly listings.
- Facts are fine: addresses, sizes, offsets, checksums, field layouts,
  descriptions of formats.
- If ReRAC has a table that is clearly bytes copied from the disc or ELF
  (beyond small constants that are format magic numbers), do not copy it. Say
  so in your report.
- Never use Sony SDK source, samples or headers.
- Never name people. Strip every personal name and handle that appears in
  ReRAC's text, notably "Lombyte". If you must refer to the third-party NTSC
  decompilation of RAC1, call it "the NTSC decomp". Tools and projects may be
  named: Wrench, OpenGOAL, Ghidra, PCSX2, ReRAC.
- Do not mention session links anywhere.

## Working rules

- Work only in the directories your brief assigns you. Other agents are
  writing other directories of the same checkout at the same time. Never
  touch, revert or reformat files outside yours.
- Do not run `git commit`, `git add`, `git stash` or `git checkout`. The lead
  commits.
- Build in your own build directory so you don't collide with others:
  ```
  cmake -S /home/user/OpenRAC/port -B /home/user/OpenRAC/port/build/agent-<you> -G Ninja \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DOPENRAC_PORT_GAMES=OFF \
    -DOPENRAC_PORT_WINDOW=OFF        # ON only for the renderer agent
  cmake --build <dir> --target <your targets>
  ctest --test-dir <dir> -R <your tests>
  ```
  - Build only your targets: other agents' files may be half written.
  - Code must compile with no warnings under `port/cmake/Warnings.cmake`
    (-Wall -Wextra -Wshadow -Wpedantic -Wconversion).
- CMake is set up by the lead (`port/cmake/Assets.cmake`, `Window.cmake`):
  - every .cpp in your module directory is compiled into your module's
    library;
  - every `port/tests/<module>/*.cpp` is a test program
    (`<module>_<file>_test`);
  - exit code 77 means skipped. Use it for tests that need a disc: read the
    path from the env, e.g. OPENRAC_DATA_RAC1_NTSC.
  - You need not edit CMake. If you truly must, say exactly what in your
    report instead of editing.
- There is no game disc in this container. Write tests with synthetic data:
  build byte buffers with ByteWriter from the documented layout and check what
  the readers return. Port ReRAC's own unit tests where they use synthetic
  data.
- Write a README.md in your module directory, in OpenRAC's style:
  - plain, factual prose that says what is there, what each file is and where
    knowledge comes from;
  - which games and versions are supported and what is not known yet.
- Finish with a report (it is your final message):
  - the files you wrote;
  - what you converted from where;
  - what you deliberately left out and why;
  - legal concerns found;
  - test results;
  - anything the lead must do (CMake, follow-ups).
