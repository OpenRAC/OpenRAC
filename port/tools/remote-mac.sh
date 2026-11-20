#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# Runs a build's MoltenVK tests, and a game on MoltenVK and on OpenGL, on another Mac over ssh, and
# brings the logs and screenshots back. Usage, from port/:
#
#   tools/remote-mac.sh USER@HOST [--build DIR] [--data DIR] [--frames N] [--levels "N ..."]
#
#   --build DIR   the build to send (default build/release-x86_64; build/release-universal for
#                 either kind of Mac)
#   --data DIR    a game data folder (<install>/active/rac1/data) to send too, then the game is run
#                 with it on both APIs; sent again only where it changed
#   --frames N    how long each game run lasts (default 1500)
#   --levels L    with --data, the levels run on both APIs with the same input at the same ticks,
#                 shot every 60 frames and compared (tools/shot_diff.py, which needs Pillow here);
#                 default "1 5 8 11": Novalis, Rilgar's water, Batalia's effects, Pokitaru's
#                 ocean, each with the HUD and, at the end, the pause menu. "" runs none.
#
# The other Mac needs Remote Login (System Settings, General, Sharing) and your key
# (ssh-copy-id USER@HOST), and its user logged in at its screen, for the tests' windows. Results
# land in build/remote/HOST/. Everything sent lives in ~/openrac-remote there.
set -eu

host=""
build=build/release-x86_64
data=""
frames=1500
levels="1 5 8 11"
while [ $# -gt 0 ]; do
    case "$1" in
        --build) build=$2; shift 2 ;;
        --data) data=$2; shift 2 ;;
        --frames) frames=$2; shift 2 ;;
        --levels) levels=$2; shift 2 ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *) host=$1; shift ;;
    esac
done
[ -n "$host" ] || { sed -n 5,20p "$0" >&2; exit 2; }

tests="imgui_gl overlay_draw renderer_direct renderer_shader touch_overlay viewer world_gl_smoke
       texture_filter vulkan_present vulkan_hdr enhanced_rendering render_equivalence"
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

# On the other Mac: run in hidden Cocoa windows with the game's bundled MoltenVK; 77 is a skip.
# The GPU line of each log says which GPU it ran on, as
# does the overlay in the *_overlay_ shots. Its display is kept on (caffeinate -u wakes it): asleep,
# a window's swap waits forever and a MacBook Pro stays on its integrated GPU.
# ssh hands the other Mac's shell one command line: each argument is quoted for it.
q() { printf "'%s'" "$(printf %s "$1" | sed "s/'/'\\\\''/g")"; }
remote_status=0
ssh "$host" "caffeinate -d -u sh -s $(q "$remote") $(q "$frames") $(q "$(basename "$app")") $(q "$tests") $(q "${data:+yes}") $(q "$levels")" > "$results/summary.txt" 2>&1 <<'EOF' || remote_status=$?
set -u
cd "$HOME/$1" || exit
frames=$2; app=$3; tests=$4; data=$5; levels=$6
rm -rf out && mkdir -p out/shots || exit
sw_vers -productVersion
system_profiler SPDisplaysDataType | grep -E 'Chipset Model|Metal'
export OPENRAC_VULKAN_LIBRARY="$PWD/$app/Contents/Frameworks/libMoltenVK.dylib"
export SDL_VIDEO_DRIVER=cocoa
test -f "$OPENRAC_VULKAN_LIBRARY" || { echo "No bundled MoltenVK"; exit 1; }
game="$app/Contents/MacOS/$(defaults read "$PWD/$app/Contents/Info" CFBundleExecutable)"
for binary in "$game" "$OPENRAC_VULKAN_LIBRARY"; do
    lipo "$binary" -verify_arch "$(uname -m)" ||
        { echo "$binary cannot run on this Mac's architecture"; exit 1; }
done

verified=0
verify_moltenvk() {
    grep -Fq "vulkan: $OPENRAC_VULKAN_LIBRARY" "$1" &&
        grep -q 'graphics: .*Vulkan' "$1" &&
        ! grep -Fq 'Vulkan could not be used' "$1" || return 1
    verified=1
}

for t in $tests; do
    if [ "$t" = render_equivalence ]; then
        OPENRAC_GL_API=desktop ./${t}_test --opengl-frames out/opengl-frames.bin \
            > "out/${t}_opengl.log" 2>&1 &&
            OPENRAC_GL_API=vulkan \
                ./${t}_test --against-opengl out/opengl-frames.bin > "out/${t}_vulkan.log" 2>&1
    else
        args=""
        enhanced=0
        [ "$t" = viewer ] && args=./viewer_test_level_vulkan
        [ "$t" = enhanced_rendering ] && enhanced=1
        OPENRAC_GL_API=vulkan OPENRAC_ENHANCED=$enhanced ./${t}_test $args > "out/${t}_vulkan.log" 2>&1
    fi
    status=$?
    case $status in 0) r=passed ;; 77) r=skipped ;; *) r="FAILED ($status)" ;; esac
    log="out/${t}_vulkan.log"
    if [ "$status" -eq 0 ] && ! verify_moltenvk "$log"; then
        r="FAILED (bundled MoltenVK was not used)"
    elif grep -Fq 'Vulkan could not be used' "$log" 2>/dev/null; then
        r="FAILED (fell back to OpenGL)"
    fi
    echo "$t: $r"
done
if [ -n "$data" ]; then
    for api in vulkan desktop; do
        OPENRAC_GL_API=$api OPENRAC_SHOT=300:out/shots/${api}_ \
            OPENRAC_OVERLAY=frame,renderer OPENRAC_OVERLAY_SHOT=300:out/shots/${api}_overlay_ \
            "$game" --data "$PWD/data" --window --frames "$frames" --skip-movies > "out/game_$api.log" 2>&1
        status=$?
        echo "game on $api: exit $status, $(grep -m1 'graphics:' "out/game_$api.log" | sed 's/.*graphics: //')"
        if [ "$status" -ne 0 ]; then
            echo "game on $api: FAILED ($status)"
        elif [ "$api" = vulkan ] && ! verify_moltenvk "out/game_$api.log"; then
            echo "game on Vulkan: FAILED (bundled MoltenVK was not used)"
        fi
    done
    # Input by script from the frame the player has control (c), the pause menu (Start) last; ticks
    # by frame and sound, so both APIs see the same frames.
    printf 'msaa = 4\n' > out/levels.ini
    for level in $levels; do
        for api in vulkan desktop; do
            mkdir -p "out/levels/${api}_$level"
            OPENRAC_GL_API=$api OPENRAC_HIDDEN=1 SDL_AUDIODRIVER=dummy OPENRAC_UNCAPPED=1 OPENRAC_AUDIO_TICKED=1 \
                OPENRAC_DIRECT=1 OPENRAC_CONTROL_FRAMES=760 OPENRAC_SHOT=60:out/levels/${api}_$level/ \
                OPENRAC_PRESS=100:4000:5,k30:8:5,c30:0:120:128:0,c160:0:150:128:128:255:128,c320:4000:5,c360:8000:5,c400:0:90:0:128,c490:0:100:128:0,c600:8000:5,c700:8:5 \
                "$game" --data "$PWD/data" --settings out/levels.ini --window --level "$level" --frames 9000 \
                --skip-movies > "out/levels/${api}_$level.log" 2>&1
            status=$?
            echo "level $level on $api: exit $status, $(ls "out/levels/${api}_$level" | wc -l | tr -d ' ') shots"
            if [ "$status" -ne 0 ]; then
                echo "level $level on $api: FAILED ($status)"
            elif [ "$api" = vulkan ] && ! verify_moltenvk "out/levels/${api}_$level.log"; then
                echo "level $level on Vulkan: FAILED (bundled MoltenVK was not used)"
            fi
        done
    done
fi
if [ "$verified" -eq 0 ]; then
    echo "FAILED (no successful run used bundled MoltenVK)"
fi
EOF
cat "$results/summary.txt"
[ "$remote_status" -eq 0 ] || exit "$remote_status"
rsync -a "$host:$remote/out/" "$results/"
for level in ${data:+$levels}; do
    python3 tools/shot_diff.py "$results/levels/desktop_$level" "$results/levels/vulkan_$level" \
        | tee "$results/levels/diff_$level.txt" | tail -1
done
echo "logs and screenshots: $results"
! grep -q 'FAILED' "$results/summary.txt"
