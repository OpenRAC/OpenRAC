# OpenRAC's changes to rac3-decomp

Applied to [rac3-decomp](https://github.com/OpenRAC/rac3-decomp) (`games/rac3/decomp`) by `python3 tools/sources.py apply`; how to change them: [docs/SOURCES.md](../../../docs/SOURCES.md).

<!-- patches:begin -->
<!-- Written by tools/sources.py save; do not edit. -->
| Patch | What |
|---|---|
| [0001-write-func-l07-003c8ba0-s-assembly-lines-with-n-esca.patch](0001-write-func-l07-003c8ba0-s-assembly-lines-with-n-esca.patch) | Its inline assembly strings ran across raw line breaks, which GCC 2.95 reads as part of the string and Clang rejects, so hostgen cannot read L07_003C7468.c. |
<!-- patches:end -->
