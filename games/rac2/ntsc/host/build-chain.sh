#!/usr/bin/env bash
# Build the RAC2 GNU-EE 2.9-ee-991111b chain (cpp, cc1, as) in a 32-bit host
# configuration, following Lombyte's scripts/build-game-compiler.py recipe and
# the RAC2 compiler notes. Run inside the rac2-linux (Ubuntu 24.04 amd64) image,
# with OpenRAC mounted at the same path (README.md next to this file).
#   build-chain.sh <work-root>      downloads come from $DOWNLOADS (default build/rac2/downloads)
set -euo pipefail
ROOT=$(cd "$1" && pwd)
HERE=$(cd "$(dirname "$0")" && pwd)
OPENRAC=$(cd "$HERE/../../../.." && pwd)
DOWNLOADS=${DOWNLOADS:-$OPENRAC/build/rac2/downloads}
LOMBYTE=$OPENRAC/games/rac1/ntsc/patches/sce-991111b
RAC2=$OPENRAC/games/rac2/ntsc/scripts/compiler
JOBS=${JOBS:-$(nproc)}
export GIT_CEILING_DIRECTORIES=$ROOT

echo "$(sha256sum "$DOWNLOADS/gnu-ee-binutils-gcc-1.1.tar.gz")" | grep -q '^1f518043e252d6eda726386971d52eda26541ab936ea73a9783d73712b595f92 '
echo "$(sha256sum "$DOWNLOADS/bison-1.28.tar.gz")" | grep -q '^c5d3e4858e17cb440cee9de7837f07277bcfb03507e9d2f0c506cab5efe36c3a '

# bison 1.28 (the 991111 grammar needs the 1.x skeleton)
if [[ ! -x $ROOT/bison/bin/bison ]]; then
  rm -rf "$ROOT/bison-1.28"
  tar xzf "$DOWNLOADS/bison-1.28.tar.gz" -C "$ROOT"
  (cd "$ROOT/bison-1.28" && CC="gcc -std=gnu89" CFLAGS="-O2 -fcommon" ./configure --prefix="$ROOT/bison" \
     && make -j"$JOBS" && make install) > "$ROOT/bison-build.log" 2>&1
fi
export BISON_SIMPLE=$ROOT/bison/share/bison.simple BISON_HAIRY=$ROOT/bison/share/bison.hairy

# source: archive + Lombyte stack minus 0001 + RAC2 recipe
rm -rf "$ROOT/source" "$ROOT/source.extract"
mkdir "$ROOT/source.extract"
tar xzf "$DOWNLOADS/gnu-ee-binutils-gcc-1.1.tar.gz" -C "$ROOT/source.extract"
mv "$ROOT/source.extract/gnu-ee-binutils-gcc" "$ROOT/source"
rmdir "$ROOT/source.extract"
cd "$ROOT/source"
for p in 0000 0015 0016 0019 0020 0021 0022 0025 0026 0027 0028 0029 0030 0031 0032 0033 0034 \
         0037 0036 0044 0045 0046 0047 0048 0049 0050 0051 0052 0053 0054 0055 0056; do
  patch_file=$(ls "$LOMBYTE/$p"-*.patch)
  # Lombyte 3f622bf (2026-10-01) changed 0020 after RAC2 took its stack;
  # PATCH_0020 selects the earlier version (Lombyte c260794).
  if [[ $p == 0020 && -n ${PATCH_0020:-} ]]; then patch_file=$PATCH_0020; fi
  git apply --whitespace=nowarn "$patch_file"
done
python3 "$HERE/rac2_recipe.py" "$ROOT/source" "$RAC2"
sha256sum gcc/config/mips/mips.c gcc/config/mips/mips.h gcc/config/mips/mips.md gas/config/tc-mips.c > "$ROOT/source-hashes.txt"

# build
HOST_CFLAGS="-O2 -fno-strict-aliasing -fcommon -std=gnu89 -D_GNU_SOURCE"
rm -rf "$ROOT/build" && mkdir "$ROOT/build" && cd "$ROOT/build"
printf 'obstack.o gcc.o mkstemp.o: override CFLAGS = -g\n' > host-flags.mk
export CC="gcc -m32" CXX="g++ -m32" CFLAGS="$HOST_CFLAGS"
{
  bash "$ROOT/source/configure" --target=mips64r5900-sf-elf --host=i686-linux-gnu \
       --build=i686-linux-gnu --disable-nls --enable-languages=c --without-headers \
       --prefix="$ROOT/install"
  make -j"$JOBS" all-libiberty
  # The 1999 Makefile lacks flow.o -> insn-flags.h; generate the headers first.
  make -C gcc -f Makefile -f "$ROOT/build/host-flags.mk" LANGUAGES=c "CC=gcc -m32" "CFLAGS=$HOST_CFLAGS" \
       "BISON=$ROOT/bison/bin/bison" insn-flags.h insn-codes.h insn-config.h
  make -C gcc -f Makefile -f "$ROOT/build/host-flags.mk" -j"$JOBS" LANGUAGES=c "CC=gcc -m32" "CFLAGS=$HOST_CFLAGS" \
       "BISON=$ROOT/bison/bin/bison" cc1 cpp xgcc
  make -j"$JOBS" all-gas
} > "$ROOT/build.log" 2>&1

mkdir -p "$ROOT/tools"
cp gcc/cc1 gcc/cpp "$ROOT/tools/"
cp gas/as-new "$ROOT/tools/as"
cd "$ROOT/tools" && sha256sum cc1 cpp as | tee "$ROOT/tool-hashes.txt"
