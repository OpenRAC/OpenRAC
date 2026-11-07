#!/usr/bin/env bash
# Runs a command in rac1/ntsc's build container on a host that emulates amd64
# (an Apple Silicon Mac), with the project's Windows tools under the 32-bit
# Wine 8 of rac1/pal's image instead of the container's own Wine. README.md
# here says why and what it was measured to do.
#
#   bash host/run.sh bash setup.sh --elf /input/SCUS_971.99   # once: toolchain, overlays, the ELF gate
#   bash host/run.sh make elf                                 # the boot ELF, byte for byte
#   bash host/run.sh make overlays                            # every overlay function in C against retail
set -euo pipefail
HERE=$(cd -- "$(dirname -- "$0")" && pwd -P)
GAME=$(dirname -- "$HERE")
IMAGE=${LOMBYTE_IMAGE:-lombyte-dev}
WINE_IMAGE=${WINE8_IMAGE:-rac1-build:latest}
VOLUME=${WINE8_VOLUME:-wine8root}

docker image inspect "$IMAGE" >/dev/null 2>&1 || {
    echo "no $IMAGE image: ./setup.sh --shell builds it (leave the shell it opens)" >&2; exit 1; }
if ! docker volume inspect "$VOLUME" >/dev/null 2>&1; then
    docker image inspect "$WINE_IMAGE" >/dev/null 2>&1 || {
        echo "no $WINE_IMAGE image: games/rac1/pal/tools/docker/run.sh builds it" >&2; exit 1; }
    echo "copying $WINE_IMAGE's file system into the volume $VOLUME (once)" >&2
    docker run --rm --platform linux/386 -v "$VOLUME:/dst" "$WINE_IMAGE" \
        sh -c 'tar --one-file-system -cf - / 2>/dev/null | tar -xf - -C /dst 2>/dev/null; test -e /dst/opt/wineprefix'
fi

mounts=()
[ -f "$GAME/config/us/SCUS_971.99" ] && mounts+=(-v "$GAME/config/us/SCUS_971.99:/input/SCUS_971.99:ro")
# --privileged: the wrapper mounts /proc and /dev inside the Wine file system and enters it with chroot.
exec docker run --rm --init --privileged --platform linux/amd64 \
    -v "$GAME:/work" -v "$IMAGE-home:/root" -v "$VOLUME:/wine8root" -v "$GAME:/wine8root/work" \
    -v "$HERE:/opt/host:ro" ${mounts[@]+"${mounts[@]}"} -w /work \
    -e LOMBYTE_IN_CONTAINER=1 -e RNC_WINE=/opt/host/wine8-chroot.sh "$IMAGE" \
    bash -c 'export PATH=/work/.venv/bin:/work/tools/binutils-mips-ps2-decompals:$PATH; exec "$@"' bash "$@"
