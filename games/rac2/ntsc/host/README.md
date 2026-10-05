# Building on macOS and Linux

The project's scripts were written for Windows with WSL. These files run the
same pipeline, unchanged, on macOS (Apple Silicon included) and Linux. They
were added in OpenRAC; nothing in the project reads or hashes them.

On an Apple Silicon Mac on 2026-10-03, at the project's commit `0fd4b77`, this
recipe produced the results below. Rerun on 2026-10-04 at `b300787`, after the
project moved its C to `src/` fragments: the boot and all 27 overlays again
rebuilt byte for byte (79,486,851 bytes), and 179/179 boot bodies matched
(9,456 bytes).

| Step | Result |
|---|---|
| `scripts/setup.py` | disc and boot verified; Wrench extracted 27 overlays, all equal to `config/overlays.json` |
| `scripts/build.py --all-levels` | boot and all 27 overlays rebuilt from reconstructed assembly, every loaded byte equal (79,486,851 bytes) |
| `scripts/check_candidates.py` | 178/178 boot bodies match (9,336 bytes); object identical to `progress/candidates.json` |
| `scripts/check_level_candidates.py`, all 27 levels | 58/58 match (2,896 bytes); objects identical to `progress/level-candidates/` |
| C integration, boot and all levels | 214,344 bytes integrated, all gates passed; every function row and object hash equal to `progress/integration.json` and `progress/levels/` |
| `python -m unittest discover -s tests` | 176 tests pass (240 at `b300787`; one of them fails on macOS only, where `/var` is a link) |

## The files

| File | What it does |
|---|---|
| [shim/sitecustomize.py](shim/sitecustomize.py) | Loaded through `PYTHONPATH`; changes only how programs start: `*.exe` through `RAC2_EXE_RUNNER` (default `wine`), `wsl.exe ... bash -lc S` through `RAC2_LINUX_RUNNER` (default `bash -c`), and `wsl_chain.wsl_path` returns the POSIX path. The tools the scripts hash are the real ones. |
| [linux-amd64.Dockerfile](linux-amd64.Dockerfile) | Ubuntu 24.04 amd64 image that plays WSL's role: Wrench, building the compiler, running its 32-bit `cpp`, `cc1` and `as` |
| [build-chain.sh](build-chain.sh) | Builds the patched GNU EE-GCC 2.9-ee-991111b chain (bison 1.28, the source archive, rac1/ntsc's patch stack without `0001`, then the RAC2 adjustments) |
| [rac2_recipe.py](rac2_recipe.py) | The RAC2 adjustments: the published scripts in `scripts/compiler/`, plus four earlier steps the project has not published, reconstructed from [docs/COMPILER-NOTES.md](../docs/COMPILER-NOTES.md) and the project's history |

## Recipe

From the top of OpenRAC, with Docker (OrbStack or Docker Desktop on a Mac):

```sh
O=$(pwd); R=$O/build/rac2; H=$O/games/rac2/ntsc/host
mkdir -p $R/downloads $R/toolchain/sn-prodg-2.0/ee/bin && cd $R/downloads

# Inputs, each checked by SHA-256
curl -L -o gnu-ee-binutils-gcc-1.1.tar.gz \
  "https://web.archive.org/web/20060518234647id_/http://ps2dev.sourceforge.net:80/downloads/ee/gnu-ee-binutils-gcc-1.1.tar.gz"
curl -L -o bison-1.28.tar.gz https://ftp.gnu.org/gnu/bison/bison-1.28.tar.gz
curl -L -o eegcc-sn-v2.73a.tar.gz \
  https://github.com/AngheloAlf/SN-Systems-ProDG_for_PS2_2.0/releases/download/1/eegcc_sn_v2.73a.tar.gz
mkdir -p lombyte-c260794 && curl -L -o lombyte-c260794/0020-gas-absolute-unknown-symbol.patch \
  https://raw.githubusercontent.com/mateuszklysz/Lombyte/c260794/patches/sce-991111b/0020-gas-absolute-unknown-symbol.patch
curl -LO https://github.com/chaoticgd/wrench/releases/download/v0.5/wrench_v0.5_linux-glibc2.39-0ubuntu8.5.zip
shasum -a 256 *.tar.gz lombyte-c260794/*.patch
#   1f518043e252d6eda726386971d52eda26541ab936ea73a9783d73712b595f92  gnu-ee-binutils-gcc-1.1.tar.gz
#   c5d3e4858e17cb440cee9de7837f07277bcfb03507e9d2f0c506cab5efe36c3a  bison-1.28.tar.gz
#   293903acfb0c8aee3b7766214119be933d80eca01fb7a3c2266287d91455962f  eegcc-sn-v2.73a.tar.gz
#   4a1726ac272b83648eea77e07e3fb13c4440a378cb3e69831c71f596838392dc  0020-gas-absolute-unknown-symbol.patch
mkdir -p ../tools && unzip -q wrench_v0.5_linux-glibc2.39-0ubuntu8.5.zip -d ../tools

# SN ProDG 2.0's assembler (pinned c839dd63..., named ps2eeas.exe in the archive)
# and the linker (pinned 80f3724a..., the same file as the SN ProDG 3.01 mirror's)
tar xzf eegcc-sn-v2.73a.tar.gz ee/bin/ps2eeas.exe
cp ee/bin/ps2eeas.exe $R/toolchain/sn-prodg-2.0/ee/bin/Ps2EeAs.exe
cp $O/toolchains/sn-prodg-3.01/usr/local/sce/ee/gcc/ee/bin/ld.exe $R/toolchain/sn-prodg-2.0/ee/bin/
cd $O

# Containers: rac2-gnu plays WSL; rac2-wine runs the Windows tools (rac1/pal's
# linux/386 image, because Rosetta cannot run 32-bit Wine). Mount OpenRAC at the
# same path in both: manifests and proofs store absolute paths.
docker build --platform linux/amd64 -t rac2-linux -f $H/linux-amd64.Dockerfile $H
docker run -d --init --name rac2-gnu --platform linux/amd64 --tmpfs /rac2tmp:exec -v $O:$O rac2-linux sleep infinity
docker run -d --init --name rac2-wine --platform linux/386 -v $O:$O rac1-build:latest sleep infinity
docker exec -d rac2-wine wineserver -p

# The compiler (Lombyte changed its 0020 after RAC2 took the stack; use the earlier one)
docker exec -e PATCH_0020=$R/downloads/lombyte-c260794/0020-gas-absolute-unknown-symbol.patch \
  rac2-gnu bash $H/build-chain.sh $R/compiler/chain

# Python 3.12 and the pinned packages
python3.12 -m venv $R/venv && $R/venv/bin/pip install -r games/rac2/ntsc/requirements.txt

# Setup: verify the disc, extract the boot and the 27 overlays
docker run --rm --init --platform linux/amd64 -v $O:$O -w $O/games/rac2/ntsc rac2-linux \
  python3 scripts/setup.py --iso $O/baserom/SCUS_972.68.iso --runtime $R/runtime \
  --wrench $R/tools/wrench_v0.5_linux-glibc2.39-0ubuntu8.5/wrenchbuild

# Build and checks
cd games/rac2/ntsc
export PYTHONPATH=$H/shim RAC2_EXE_RUNNER="docker exec -w {cwd} rac2-wine wine" \
       RAC2_LINUX_RUNNER="docker exec rac2-gnu bash -c" \
       RAC2_WSL_TOOLS=$R/compiler/chain/tools RAC2_WSL_TMP=/rac2tmp
RUN=$(ls -d $R/runtime/runs/* | tail -1)
$R/venv/bin/python scripts/build.py --manifest $RUN/manifest.json --toolchain $R/toolchain/sn-prodg-2.0 --all-levels
$R/venv/bin/python scripts/check_candidates.py --reference $RUN/reference/boot.elf \
  --toolchain $O/toolchains/sn-prodg-3.01/usr/local/sce/ee/gcc --runtime $R/runtime
$R/venv/bin/python -m unittest discover -s tests
```

`check_level_candidates.py --level <L> --reference $RUN/reference/levels/<L>/overlay.elf`
checks one level's bodies the same way. On Linux x86-64 you need no
containers: install 32-bit Wine and `gcc-multilib`, leave `RAC2_EXE_RUNNER`
and `RAC2_LINUX_RUNNER` at their defaults, and run `build-chain.sh` directly.

## What to know

- **A rebuilt compiler has its own hashes.** The committed proofs record the
  author's binaries (`progress/integration.json`), so `build.py --c-toolchain`
  against them refuses with "C integration instruments differ". The bodies and
  objects come out identical; check with fresh reviews instead
  (`--candidate-review <your check_candidates report>`, and level reviews from
  `check_level_candidates.py --write-review` in a scratch copy, never over the
  committed ones). The binaries also depend on the build host and path.
- **Do not edit `scripts/wsl_chain.py`**: the level reviews hash it. The shim
  exists so nothing in the project changes.
- **Linked ELF hashes vary by machine**, because Ps2EeAs records absolute
  source paths in objects; the gates compare loaded bytes only.
- **Not yet published by the project**: four compiler steps (rebuilt in
  `rac2_recipe.py`; the resulting `tc-mips.c` hashes to the qualified
  `61e51c1e…`, `mips.c` does not), an addition to `mips.md` that the zero-store
  patch's line numbers imply, and which Lombyte commit the stack is pinned to.
- The project's README, START-HERE and `doctor.py` still name the SN ProDG
  3.01 compiler for C; the pipeline uses only `ld.exe` from that directory,
  plus the GNU chain.
