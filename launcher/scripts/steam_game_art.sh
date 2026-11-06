#!/usr/bin/env bash
# The pictures Steam's library shows for a game's own entry (core/src/steam.rs, a game's
# "Add to Steam" on its page), in src-tauri/steam/games/<game>/: the game's backdrop
# (public/img, src/lib/media.ts) cut to each size Steam asks for, with the game's name on it
# in Orbitron (port/game/common/fonts, OFL), and the name alone as the logo.
# Run from launcher/ after a backdrop changes. Needs ffmpeg 7 or newer (drawtext text_align).
set -euo pipefail

OUT=src-tauri/steam/games
FONT=../port/game/common/fonts/Orbitron.ttf
INK=0xe8f0ff # --text (src/app.css)
TMP=$(mktemp -d .steam-art.XXXXXX) # relative: a Windows ffmpeg reads it too
trap 'rm -rf "$TMP"' EXIT

# game | backdrop | the series' name above the title ("" for none) | the title, lines split by "|"
GAMES=(
  "rac1|rac1-bg.webp||RATCHET|& CLANK"
  "rac2|gc-bg.webp|RATCHET & CLANK|GOING|COMMANDO"
  "rac3|uya-bg.webp|RATCHET & CLANK|UP YOUR|ARSENAL"
  "rac4|deadlocked-bg.webp|RATCHET & CLANK|DEADLOCKED"
)

# Darkens a picture towards its bottom (from `from`, 0 to 1 of its height) or, with `x`, its left.
shade() {
  local axis=$1 from=$2 depth=$3
  local f
  if [ "$axis" = y ]; then
    f="(1-${depth}*clip((Y/H-${from})/(1-${from}),0,1))"
  else
    f="(1-${depth}*clip((${from}-X/W)/${from},0,1))"
  fi
  echo "format=gbrp,geq=r='r(X,Y)*${f}':g='g(X,Y)*${f}':b='b(X,Y)*${f}'"
}

# The name: the series' line (small, if any) over the title (centred lines), as drawtext filters.
# `x`, `y` place the block's top left; `big` is the title's size.
name() {
  local kicker=$1 title=$2 x=$3 y=$4 big=$5 align=$6
  local small=$((big * 2 / 5)) gap=$((big / 4)) filters=""
  local common="fontfile=${FONT}:fontcolor=${INK}:shadowcolor=black@0.7:shadowx=0:shadowy=$((big / 14 + 1)):text_align=${align}"
  if [ -n "$kicker" ]; then
    printf '%s' "$kicker" >"$TMP/kicker.txt"
    filters+="drawtext=${common}:textfile=${TMP}/kicker.txt:fontsize=${small}:x=${x}:y=${y},"
    y="${y}+${small}+${gap}"
  fi
  printf '%s' "$title" >"$TMP/title.txt"
  filters+="drawtext=${common}:textfile=${TMP}/title.txt:fontsize=${big}:line_spacing=$((big / 6)):x=${x}:y=${y}"
  echo "$filters"
}

for entry in "${GAMES[@]}"; do
  IFS='|' read -r game backdrop kicker first second <<<"$entry"
  title=$first
  [ -n "${second:-}" ] && title=$(printf '%s\n%s' "$first" "$second")
  one_line=${title//$'\n'/ }
  src=public/img/$backdrop
  dir=$OUT/$game
  mkdir -p "$dir"
  fit() { echo "scale=$1:$2:force_original_aspect_ratio=increase,crop=$1:$2"; }

  # Cover, 600 x 900: the name over the shaded foot. Centred lines; the block is centred too.
  ffmpeg -v error -y -i "$src" -frames:v 1 -q:v 3 -vf \
    "$(fit 600 900),$(shade y 0.4 0.88),$(name "$kicker" "$title" "(w-text_w)/2" "h-310" 72 C),format=yuvj420p" \
    "$dir/cover.jpg"
  # Wide, 920 x 430: the name on the shaded left.
  ffmpeg -v error -y -i "$src" -frames:v 1 -q:v 3 -vf \
    "$(fit 920 430),$(shade x 0.75 0.85),$(name "$kicker" "$title" 50 "h-250" 60 L),format=yuvj420p" \
    "$dir/wide.jpg"
  # Hero, 1920 x 620: no name; Steam draws the logo over it.
  ffmpeg -v error -y -i "$src" -frames:v 1 -q:v 3 -vf \
    "$(fit 1920 620),$(shade y 0.6 0.5),format=yuvj420p" "$dir/hero.jpg"
  # Logo: the name alone on a clear ground, cut to it (Steam places the picture).
  draw="$(name "$kicker" "$one_line" "(w-text_w)/2" 60 110 C)"
  # Where the name is: cropdetect weighs whole rows, then whole columns of the rows found (a
  # column of the full picture is mostly empty, and never counts).
  white="color=c=black:s=2200x500:d=1,${draw//fontcolor=${INK}/fontcolor=white}"
  # "first last" of axis `x` or `y` of the lit part of the lavfi picture `$1`.
  bounds() {
    ffmpeg -hide_banner -f lavfi -i "$1,format=yuvj420p,cropdetect=limit=0.02:round=1:reset=0:skip=0" \
      -frames:v 2 -f null - 2>&1 | grep -oE "$2""1:[0-9]+ $2""2:[0-9]+" | tail -1 | sed -E 's/[xy][12]://g'
  }
  read -r top bottom <<<"$(bounds "$white" y)"
  read -r left right <<<"$(bounds "${white},crop=iw:$((bottom - top + 1)):0:${top}" x)"
  pad=16
  ffmpeg -v error -y -f lavfi -i "color=c=black@0:s=2200x500,format=rgba" -frames:v 1 -vf \
    "${draw},crop=$((right - left + 1 + 2 * pad)):$((bottom - top + 1 + 2 * pad)):$((left - pad)):$((top - pad))" \
    "$dir/logo.png"
  echo "$dir: $(ls "$dir" | tr '\n' ' ')"
done
