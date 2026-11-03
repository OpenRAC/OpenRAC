#!/usr/bin/env bash
# The refactor gate: rebuild, check every version, refresh the reports, fail if the match drops.
#
#   bash tools/check_all.sh
#
# Order matters: version_sync runs in the container first, version_report on the host last. The
# container's Python sums floats differently from CI's, so the host must write the reports CI checks.
set -euo pipefail
cd "$(dirname "$0")/.."

# Set FORMAT_PATHS in the calling script while a tree is only part formatted.
read -r -a FORMAT_PATHS <<< "${FORMAT_PATHS:-}"

run() { bash tools/docker/run.sh "$@"; }

run bash tools/build_sn.sh | tail -6
rm -rf build-sn/overlays/report
run python3 tools/gen_progress_report.py --no-build | tail -1
run python3 tools/version_sync.py run | tail -3
for version in us pal_v1 jp; do
    python3 tools/version_report.py --version "$version" | tail -1
done
python3 tools/gen_progress_report.py --check
python3 tools/move_src.py --check
python3 tools/overlay_remnants.py --check
python3 tools/check_docs.py
python3 tools/data_in_c.py check
python3 tools/fakematch_scan.py --check
# A header git ignores builds here but not in a fresh clone.
if git ls-files -o -i --exclude-standard include | grep .; then
    echo "*** git ignores these headers"
    exit 1
fi
python3 tools/check_no_retail.py
python3 tools/check_syntax.py
python3 tools/ratchets.py --check
python3 tools/format.py --check "${FORMAT_PATHS[@]}"

python3 tools/check_regressions.py
