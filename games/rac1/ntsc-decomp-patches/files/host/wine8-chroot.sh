#!/bin/sh
# Runs a Windows program with the 32-bit Wine 8 of rac1/pal's image, whose root
# file system is mounted at /wine8root, from inside rac1/ntsc's container.
# The caller's tree must be mounted at the same path under /wine8root (/work
# and /wine8root/work). rac1/ntsc uses it as RNC_WINE (host/run.sh): the
# container's own Wine 9 cannot start 32-bit programs when an Apple Silicon
# Mac emulates the container (status c0000018); this Wine can.
[ -e /wine8root/proc/self ] || mount -t proc proc /wine8root/proc 2>/dev/null
[ -e /wine8root/dev/null ] || mount --bind /dev /wine8root/dev 2>/dev/null
exec chroot /wine8root /usr/bin/env WINEPREFIX=/opt/wineprefix WINEDEBUG=-all HOME=/root \
  sh -c 'cd "$1" && shift && exec wine "$@"' sh "$PWD" "$@"
