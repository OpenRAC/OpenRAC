#!/usr/bin/env bash
# Regenerate asm/ from your own baserom. asm/ is NOT in git: it is the
# disassembly of the retail executable (instruction bytes included), which
# the repo must never contain -- see LEGAL.md.
#
#   bash tools/setup_asm.sh
#
# Needs baserom/SCES_509.16 (your own dump) and the pinned packages from
# requirements.txt. Output is byte-identical to what the build and the audit
# tools were verified against; this script checks the inputs that decide
# that (baserom hash, splat/spimdisasm versions) before touching anything.
set -euo pipefail
cd "$(dirname "$0")/.."

want_sha1=79956931bd62fafd8d20fa2eae796dbaf2e15e83
[ -f baserom/SCES_509.16 ] || { echo "missing baserom/SCES_509.16 (dump your own copy; see LEGAL.md)"; exit 1; }
have_sha1=$(python -c "import hashlib;print(hashlib.sha1(open('baserom/SCES_509.16','rb').read()).hexdigest())")
[ "$have_sha1" = "$want_sha1" ] || { echo "baserom/SCES_509.16 sha1 $have_sha1, expected $want_sha1 (PAL v2.00)"; exit 1; }

python - <<'PY'
import importlib.metadata as m, sys
want = {"splat64": "0.50.0", "spimdisasm": "1.42.4", "rabbitizer": "1.16.2"}
bad = {k: (m.version(k) if k in {d.metadata["Name"] for d in m.distributions()} else None, v)
       for k, v in want.items()}
bad = {k: v for k, v in bad.items() if v[0] != v[1]}
if bad:
    for k, (have, need) in bad.items():
        print(f"{k}: have {have}, need {need}")
    sys.exit("pip install -r requirements.txt")
PY

rm -rf asm
# Existing src/*.c and include/ are left untouched: config/splat.yaml sets
# generate_asm_macros_files: False, and splat never overwrites a .c file.
python -m splat split config/splat.yaml
# Same post-processing the committed asm used to carry.
python tools/fix_vu0_macro.py asm
python tools/sn_regnames.py asm
python tools/fix_denormal_floats.py asm
python tools/organize_asm.py
echo "asm/ ready ($(find asm -type f -name '*.s' | wc -l) files)"

# Level code overlays (docs/OVERLAYS.md): asm/overlays/<name>.s for every
# distinct shared/level function, from baserom/overlays/ (assumes
# `python3 tools/overlays.py dump` has already populated it -- that needs
# baserom/SCES_509.16.iso, so it is not repeated here).
if [ -d baserom/overlays ]; then
    python tools/overlay_asm.py
else
    echo "baserom/overlays missing: skipping asm/overlays (run tools/overlays.py dump first)"
fi
