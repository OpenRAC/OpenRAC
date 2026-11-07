# Building on an Apple Silicon Mac

The project builds on Linux and WSL, and on other hosts in the container its
`setup.sh --docker` makes. On an Apple Silicon Mac that container is emulated,
and its own Wine (Ubuntu 24.04's Wine 9, with `wine32:i386`) cannot start the
project's Windows tools there: Wine stops with status c0000018, under Rosetta
and under QEMU alike. The two files here run those tools under another Wine
instead. They were added in OpenRAC; nothing in the project reads them.

On 2026-10-04, at the project's commit `e1db4cd`, with OrbStack and Rosetta
left on:

| Step | Result |
|---|---|
| `bash host/run.sh bash setup.sh --elf /input/SCUS_971.99` | toolchain built, 19 overlays extracted and disassembled, `PASS: reconstructed boot ELF matches retail` |
| `bash host/run.sh make elf` | 4,368 build steps, built SHA-256 `e050581032e4bb3f20341307da5b69b76f1574910519155380ea771e55c3c0c9`, `PASS: reconstructed boot ELF matches retail` (about 3 minutes) |
| `bash host/run.sh make overlays` | `PASS: all 1540 overlay functions in C match retail` (about 1 minute) |

## The files

| File | What it does |
|---|---|
| [wine8-chroot.sh](wine8-chroot.sh) | Runs one Windows program with the 32-bit Wine 8 of rac1/pal's image, whose file system is mounted in the container. The project takes it through `RNC_WINE`, its own setting for the Wine to use. |
| [run.sh](run.sh) | Runs a command in the project's container with that Wine in place: copies rac1/pal's image into a Docker volume the first time, mounts it, and sets `RNC_WINE` |

The tools that run are the project's own (its patched Ps2EeAs, SN's `ee-gcc.exe`
driver and its compilers); only the Wine underneath differs. The container
runs with `--privileged`, because the wrapper mounts `/proc` and `/dev` inside
the Wine file system and enters it with `chroot`.

## Recipe

From `games/rac1/ntsc`, with Docker (OrbStack or Docker Desktop):

```sh
python3 ../../../tools/openrac.py setup rac1/ntsc     # config/us/SCUS_971.99 from your disc
bash ../pal/tools/docker/run.sh true                  # rac1/pal's image, rac1-build (Wine 8, linux/386)
./setup.sh --shell                                    # builds the lombyte-dev image; leave the shell it opens
bash host/run.sh bash setup.sh --elf /input/SCUS_971.99
bash host/run.sh make elf
bash host/run.sh make overlays
```

`make check` (the project's tests and report) needs no Wine and runs in the
same container: `bash host/run.sh make check`.

## What was tried before this

- The container's own Wine 9: fails as above, with `WINEARCH=win32`,
  `WINELOADERNOEXEC=1` and `setarch` as well.
- wibo's i686 build as `RNC_WINE`: 4,366 of the 4,368 build steps pass; the
  two units SN's `ee-gcc.exe` driver compiles fail, because wibo cannot start
  the driver's child processes under 32-bit emulation (`clone: Invalid
  argument`).
