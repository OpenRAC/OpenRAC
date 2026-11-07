#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# Runs a build's Metal tests, and a game on Metal and on OpenGL, on another Mac over ssh, and
# brings the logs and screenshots back. Usage, from port/:
#
#   tools/remote-mac.sh USER@HOST [--build DIR] [--data DIR] [--frames N]
#
#   --build DIR   the build to send (default build/release-x86_64; build/release-universal for
#                 either kind of Mac)
#   --data DIR    a game data folder (<install>/active/rac1/data) to send too, then the game is run
#                 with it on both APIs; sent again only where it changed
#   --frames N    how long each game run lasts (default 1500)
#
# The other Mac needs Remote Login (System Settings, General, Sharing) and your key
# (ssh-copy-id USER@HOST), and its user logged in at its screen, for the tests' windows. Results
# land in build/remote/HOST/. Everything sent lives in ~/openrac-remote there.
set -eu

host=""
build=build/release-x86_64
data=""
frames=1500
while [ $# -gt 0 ]; do
    case "$1" in
        --build) build=$2; shift 2 ;;
        --data) data=$2; shift 2 ;;
        --frames) frames=$2; shift 2 ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *) host=$1; shift ;;
    esac
done
[ -n "$host" ] || { sed -n 5,20p "$0" >&2; exit 2; }

tests="mtlgl imgui_gl overlay_draw renderer_direct renderer_shader touch_overlay viewer world_gl_smoke"
app=$(ls -d "$build"/apps/openrac-rac1-ntsc/*.app)
remote=openrac-remote
results=build/remote/${host#*@}
mkdir -p "$results"

set --
for t in $tests; do
    set -- "$@" "$build/${t}_test"
done
ssh "$host" "mkdir -p $remote/data"
rsync -a "$@" "$app" "$host:$remote/"
if [ -n "$data" ]; then
    rsync -a --delete --exclude '*.log' --exclude decompiler_out "$data/" "$host:$remote/data/"
fi

# On the other Mac: each test as ctest runs it (the _metal ones in Cocoa windows with Metal's API
# validation; 77 is a skip), then the game. The GPU line of each log says which GPU it ran on.
# ssh hands the other Mac's shell one command line: each argument is quoted for it.
q() { printf "'%s'" "$(printf %s "$1" | sed "s/'/'\\\\''/g")"; }
ssh "$host" "sh -s $(q "$remote") $(q "$frames") $(q "$(basename "$app")") $(q "$tests") $(q "${data:+yes}")" <<'EOF' | tee "$results/summary.txt"
set -u
cd "$HOME/$1"
frames=$2; app=$3; tests=$4; data=$5
rm -rf out && mkdir -p out/shots
sw_vers -productVersion
system_profiler SPDisplaysDataType | grep -E 'Chipset Model|Metal'
for t in $tests; do
    if [ "$t" = mtlgl ]; then
        MTL_DEBUG_LAYER=1 ./mtlgl_test > "out/$t.log" 2>&1
    else
        args=""
        [ "$t" = viewer ] && args=./viewer_test_level_metal
        OPENRAC_GL_API=metal MTL_DEBUG_LAYER=1 SDL_VIDEO_DRIVER=cocoa ./${t}_test $args > "out/${t}_metal.log" 2>&1
    fi
    status=$?
    case $status in 0) r=passed ;; 77) r=skipped ;; *) r="FAILED ($status)" ;; esac
    echo "$t: $r"
done
if [ -n "$data" ]; then
    game="$app/Contents/MacOS/$(defaults read "$PWD/$app/Contents/Info" CFBundleExecutable)"
    for api in metal desktop; do
        OPENRAC_GL_API=$api MTL_DEBUG_LAYER=1 OPENRAC_SHOT=300:out/shots/${api}_ \
            "$game" --data "$PWD/data" --window --frames "$frames" --skip-movies > "out/game_$api.log" 2>&1
        echo "game on $api: exit $?, $(grep -m1 'graphics:' "out/game_$api.log" | sed 's/.*graphics: //')"
        grep -m1 'could not be used' "out/game_$api.log" || true
    done
fi
EOF
rsync -a "$host:$remote/out/" "$results/"
echo "logs and screenshots: $results"
