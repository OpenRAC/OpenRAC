# assets/disc

The disc, read straight from the player's image: library
`openrac_assets_disc`, namespace `openrac::assets`. It is what the extractor
copies and checks, and what the other readers get their bytes from. Converted
from [ReRAC](https://github.com/re-rac/rerac)'s `rc-formats` (ISC License,
Copyright (c) 2026 ReRAC contributors), re-organised for every game. Each
file names its source and the spec it follows.

| File | |
|---|---|
| `iso9660.h` | the ISO 9660 file system and raw sector reads; plain 2048-byte images and raw 2352-byte dumps; reads only what is asked for |
| `boot.h` | `SYSTEM.CNF` (the boot executable, the serial, the disc version, the video mode) and reads from the boot executable's segments |
| `checksum.h` | SHA-1 and SHA-256, from the published algorithms: builds are named and extracted files checked by them |
| `toc.h` | the table of contents per version: the global header's fields, the level table, the level headers and their scene records |
| `disc.h` | a disc as a whole: its lumps, each level's files, and the plan of the extracted tree (`archive_files`: `boot/`, `toc.bin`, `global/`, `levels/NN/`), every file one byte range of the image |
| `wad.h` | Insomniac's WAD compression (LZ77 behind a 16-byte header), which every compressed lump uses |
| `level.h` | a level's data container, its core index and the named blocks of its core data |
| `overlay.h` | a level's program (`overlay.bin`): its sections, class tables, and the relocator that finds in any level the copy of a function another level's address names |
| `messages.h` | a level's messages (help boxes, banners, subtitles) per language |
| `scene.h` | the in-engine scenes (cutscenes): the scene table and its chunks |
| `transition.h` | the flight between planets' lump |
| `frontend.h` | the title world and the boot pictures |
| `save_game.h` | the saved game on the memory card: chunk tables, the file, its CRC-16, the disc's template |
| `volumes.h` | the gameplay file's volumes: cuboids, spheres, cylinders, pills, paths, grind paths |

## Games

| Version | Layout |
|---|---|
| rac1-ntsc | known: ReRAC's `docs/formats/disc_layout.md`, `wad_layouts_rac1.md` |
| rac1-pal | known: the same table of contents at sector 1500, as OpenRAC's own reader of the PAL disc gives it ([editor/disc.py](../../../editor/disc.py), [level.py](../../../editor/level.py)) |
| rac2-ntsc, rac3-ntsc, rac4-ntsc | identified (serial and boot executable) but not readable: their tables of contents are not known yet, and `disc_layout()` refuses them |

Addresses in the comments are NTSC-U (SCUS_971.99) unless they say PAL.

## Tests

`tests/disc/`: synthetic images built in memory (`synthetic_disc.h`): the
file system, `SYSTEM.CNF`, the table of contents, level and global lumps, the
extracted tree's plan, WAD round trips, and every format above. Nothing is
read from a disc; nothing from a disc is in the tests.
