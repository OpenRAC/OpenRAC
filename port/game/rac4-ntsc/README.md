# openrac-rac4-ntsc: Ratchet: Deadlocked, NTSC-U

Deadlocked's NTSC-U disc (`SCUS_974.65`), built by [hostgen](../../tools/hostgen/README.md) from
[games/rac4](../../../games/rac4/decomp). The main menu draws and answers the pad; its 3D background
and sound are not done, there are menu drawing faults, and gameplay is not reached.

```sh
python3 tools/build_port.py rac4-ntsc
port/build/release/openrac-rac4-ntsc --data <data> --window --no-card --skip-movies --stop-on-missing
```

A strict test run: `OPENRAC_STRICT=1 OPENRAC_UNCAPPED=1 OPENRAC_AUDIO_TICKED=1`,
`OPENRAC_PRESS=300:8:10,600:8:10,900:8:10,1200:8:10,1400:40:5,1600:10:5`, `--frames 1800`.
`OPENRAC_SHOT=500:<prefix>` saves frames; `--debug-dump <path>` writes frame metrics.

## Files

| File | |
|---|---|
| `hostgen.json` | `skip` leaves out `src/pal` (PAL's C, not this disc's) and 35 files whose matching C drops registers the callers use (below); `returns_64` and `asm.signatures` keep 64-bit TEX0 values whole |
| `libraries.tsv` | the library calls the port answers |
| `profile.cpp` | the game's profile: it sends its display list |
| `host/entry.c`, `heap.c`, `dma.c`, `wad.c`, `trig.c` | startup, memory, DMA, the WAD decompressor and VU0 sine (shared bodies in `common/lib/`) |
| `host/pad.c` | the older libpad API over the shared pad: port lifetime, digital/analog modes, the 32-byte buffer; slot 0 only, no motors |
| `host/sound.c` | the synchronous 989snd command at `0x00158A30`, answered by the native sound service with no IOP wait |
| `host/movie.c` | the movie entry at `0x004EEEB0`, played by the shared player (so `--skip-movies` works) |

## Native boundaries

- Memory card: the query at `0x0013A8C0` (RPC command 1) and its completion at `0x0013A7A0` use the
  shared card service. Binding only `sceMcInit` leaves the card state at `0x00171B0C` stuck at 0.
- `sceSifLoadModuleBuffer` (`0x0012CBC0`) uses the native service, not the IOP loader.
- The 24 module records at `0x003D7EB8` (id, pre-load, post-load, cleanup callbacks) give 14
  hostgen roots; module 5's post-load (`0x00460138`) starts the pad.
- The front end's loop (`0x00157C58` into `0x00437EB0`) runs until the word at `0x0021EDBC` changes.
  Its seven nine-slot method tables (`0x003D35D8` to `0x003D3768`, slots at base + 8) and the
  helper at `0x003D37A0` are rooted (52 methods), with `0x0050E2A8`, which the catalogue lacks.
- Splash and card state: `0x0021E1F0`, its timer `0x0021E1F4`, the startup gate `0x0021E1EC`.

## Register-forwarding wrappers

The 35 skipped files are translated from their instructions, because their matching C drops a
register the callers rely on (the decompilation stays as it is). For example `005005E8` forwards a
second argument to `00500AE8` and returns its result; without it a null method is called.
`00500D30` also forwards a float register to `00453C68`. The TEX0 producers (`00448F98`,
`004FB4E0`, `00449350`) and the glyph builders (`0044A410`, `0044A5C8`, `0044A7D0`) pass 64-bit
values; hostgen's `test_catalogue` checks both halves.

ctest `rac4_io` tests the pad and sound adapters with mock devices (no disc). Changing a startup
state word is not a fix for a missing input service.
