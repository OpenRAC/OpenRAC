# Ratchet & Clank: Up Your Arsenal (2004)

Released in Europe as *Ratchet & Clank 3*.

| Version | Directory | Came from | Builds on |
|---|---|---|---|
| NTSC-U, `SCUS_973.53` (disc v1.00) | [ntsc/](ntsc/README.md) | [ratchet-uya-decomp](https://github.com/vetusmagnus/ratchet-uya-decomp) | Windows; Linux and macOS with wibo |

The project decompiles `frontbin.elf`, the frontend and menu program, which
lives inside the game's data rather than as a file on the disc; you extract it
with [Wrench](https://github.com/chaoticgd/wrench). The build reproduces it
byte for byte. The level programs and the other executables are counted for
progress, and a first set of common level functions is matched in C
([ntsc/docs/common_level_c.md](ntsc/docs/common_level_c.md)). The main
executable, `SCUS_973.53`, is not decompiled yet.

Its compiler research, 15 compilers against 8 flag sets, is in
[ntsc/docs/compiler_matrix_findings.md](ntsc/docs/compiler_matrix_findings.md);
it is the most systematic compiler study among the projects
([docs/toolchains](../../docs/toolchains/README.md)).

Disc checksums are in [baserom/README.md](../../baserom/README.md); progress
in [progress/](../../progress/README.md). The project has no license file yet
([open questions](../../docs/policy/OPEN_QUESTIONS.md#1-licensing)).

## Getting started

On Windows, follow [ntsc/docs/wiki/Setup.md](ntsc/docs/wiki/Setup.md). On
macOS and Linux, from the top of OpenRAC (tested on an Apple Silicon Mac,
2026-10-03; the build matched retail byte for byte in about 77 seconds):

```sh
# The compiler: SN ee-gcc 2.95.3 v1.36 is the ee/gcc tree of the SN ProDG 3.01
# mirror (toolchains/README.md); setup links toolchains/eegcc_2.95.3_sn_v1.36 to it.
python3 tools/openrac.py setup rac3/ntsc

# wibo runs the Windows compiler: wibo-macos natively (needs Rosetta 2;
# xattr -d com.apple.quarantine wibo-macos if a browser downloaded it), or
# wibo-i686 on Linux. Wrench unpacks the disc; it has Linux and Windows builds.
mkdir -p build/rac3 && cd build/rac3
curl -sSLO https://github.com/decompals/wibo/releases/download/1.2.0/wibo-macos && chmod +x wibo-macos
curl -sSLO https://github.com/chaoticgd/wrench/releases/download/v0.5/wrench_v0.5_linux-glibc2.39-0ubuntu8.5.zip
unzip -q wrench_v0.5_linux-glibc2.39-0ubuntu8.5.zip -d wrench && cd ../..

# frontbin.elf lives inside the game's data. On a Mac, run Wrench's Linux build
# in an amd64 container (Rosetta runs it); the unpack writes about 4.8 GB.
docker run --rm --platform linux/amd64 -v "$PWD:$PWD" -w "$PWD" ubuntu:24.04 \
  build/rac3/wrench/wrench_v0.5_linux-glibc2.39-0ubuntu8.5/wrenchbuild \
  unpack baserom/SCUS_973.53.iso -o build/rac3/wrench_out -s
cp build/rac3/wrench_out/uya_scus_973_53/globals/misc/frontbin.elf games/rac3/ntsc/
shasum games/rac3/ntsc/frontbin.elf    # 3bc94ee895e4b4af9b5602a229af599c1103b542

# Python packages, the assembly, the build, the pre-PR check
python3 -m venv build/rac3/venv && build/rac3/venv/bin/pip install -r games/rac3/ntsc/tools/requirements.txt
cd games/rac3/ntsc
../../../build/rac3/venv/bin/python tools/setup_asm.py
../../../build/rac3/venv/bin/python tools/build.py \
  --toolchain ../../../toolchains/eegcc_2.95.3_sn_v1.36 --runner ../../../build/rac3/wibo-macos
../../../build/rac3/venv/bin/python tools/pr_check.py
```

The build ends with `MATCH: build/frontbin.bin sha1 3bc94ee... (0x218924
bytes)`. To try one function, `tools/try_func.py` takes `UYA_TOOLCHAIN` and
`UYA_RUNNER` set to the same two paths. `localdecomp/server.py` assumes
Windows. `pr_check.py --obj build/src/text.c.o` still rejects the `.rodata`
that the switch jump tables put in `text.c.o`, which the linker script places
on purpose; the check itself is out of date.

The wiki's other pages ([ntsc/docs/wiki/](ntsc/docs/wiki/Home.md)) cover the
workflow, matching patterns and pull requests.
