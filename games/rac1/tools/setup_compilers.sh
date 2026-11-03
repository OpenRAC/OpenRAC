#!/usr/bin/env bash
# Fetch the two compilers the SN ProDG clones do not include. Run from the repository root:
#
#   bash tools/setup_compilers.sh
#
# Both come from the decompme/compilers release, and each archive's SHA-256 is pinned. toolchain/ is gitignored, so nothing
# here is committed. See docs/reference/TOOLCHAIN.md.
set -euo pipefail
cd "$(dirname "$0")/.."

RELEASE=https://github.com/decompme/compilers/releases/download/compilers
GCC_ARCHIVE=ee-gcc2.9-991111-01.tar.xz
GCC_SHA256=ed684fd98f89d36b0121caab311052089103e3b36241fcef4338cc9ea41c75b8
AS_ARCHIVE=ee-gcc2.9-991111-01-dtls13010.tar.gz
AS_SHA256=3ab52a7ea962100f2a7df3e04d3e7a758aad669eb365251682e769b755d9d586

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

fetch() {
  curl -fsSL -o "$work/$1" "$RELEASE/$1"
  echo "$2  $work/$1" | sha256sum -c --quiet -
}

if [ -x toolchain/ee-gcc-2.9-991111-01/bin/ee-gcc ]; then
  echo "toolchain/ee-gcc-2.9-991111-01 is already here"
else
  fetch "$GCC_ARCHIVE" "$GCC_SHA256"
  mkdir -p toolchain/ee-gcc-2.9-991111-01
  tar xJf "$work/$GCC_ARCHIVE" -C toolchain/ee-gcc-2.9-991111-01
  echo "installed toolchain/ee-gcc-2.9-991111-01"
fi

if [ -f toolchain/ee-as-dtls13010/as.bin ]; then
  echo "toolchain/ee-as-dtls13010 is already here"
else
  fetch "$AS_ARCHIVE" "$AS_SHA256"
  mkdir -p toolchain/ee-as-dtls13010/lib "$work/as"
  tar xzf "$work/$AS_ARCHIVE" -C "$work/as"
  cp "$work/as/ee/bin/as.bin" toolchain/ee-as-dtls13010/
  cp -L "$work/as/lib/ld-2.3.6.so" "$work/as/lib/libc-2.3.6.so" "$work/as/lib/libc.so.6" toolchain/ee-as-dtls13010/lib/
  echo "installed toolchain/ee-as-dtls13010"
fi
