"""clang JSON AST helpers for the moby/hero conversions (analysis only, no objects).

The EE has 64-bit `long` and 128-bit `long long` with 32-bit pointers, which no
clang target has; the source and include/ are rewritten with same-length type
names (so every byte offset stays the same) and parsed for mips64el n32."""
import json, os, re, subprocess, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
S = ROOT / 'build-sn' / 'tidy'   # scratch: the clang inputs and the preprocessed headers
S.mkdir(parents=True, exist_ok=True)
INC_T = S / 'inc_t'
PRELUDE = S / 'prelude.h'

SUBS = [  # same length
    (r'\bunsigned long long\b', '_ULL16____________'),
    (r'\blong long\b', '_LL16____'),
    (r'\bunsigned long\b', '_UL8_________'),
    (r'\bsigned long\b', '_SL8_______'),
    (r'\blong\b', '_L8_'),
]
PRELUDE.write_text('typedef unsigned __int128 _ULL16____________;\ntypedef __int128 _LL16____;\n'
                   'typedef unsigned long long _UL8_________;\ntypedef long long _SL8_______;\n'
                   'typedef long long _L8_;\n')
UNSUB = {b: re.sub(r'\\b', '', a) for a, b in SUBS}


def ee(text):
    for a, b in SUBS:
        text = re.sub(a, b, text)
    return text


def unee(s):
    for b, a in UNSUB.items():
        s = s.replace(b, a)
    s = s.replace('unsigned __int128', 'unsigned long long').replace('__int128', 'long long')
    return s


def prep_includes():
    INC_T.mkdir(exist_ok=True)
    for p in (ROOT / 'include').iterdir():
        if p.suffix == '.h':
            (INC_T / p.name).write_text(ee(p.read_text(errors='replace')))


def clang_json(text, name='x.c'):
    f = S / ('ast_' + name)
    f.write_text(ee(text))
    cmd = ['clang', '--target=mips64el-linux-gnuabin32', '-std=gnu89', '-fsyntax-only', '-DM2CTX',
           '-I', str(INC_T), '-include', str(PRELUDE), '-w', '-ferror-limit=0',
           '-Wno-incompatible-pointer-types', '-Wno-int-conversion', '-Wno-implicit-function-declaration',
           '-Wno-implicit-int', '-Wno-return-type', '-Xclang', '-ast-dump=json', str(f)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    errs = [l for l in r.stderr.splitlines() if 'error:' in l]
    return json.loads(r.stdout), errs, str(f)


def clang_errors(text, name='chk.c'):
    f = S / ('err_' + name)
    f.write_text(ee(text))
    cmd = ['clang', '--target=mips64el-linux-gnuabin32', '-std=gnu89', '-fsyntax-only', '-DM2CTX',
           '-I', str(INC_T), '-include', str(PRELUDE), '-ferror-limit=0', '-Wno-everything',
           '-Wincompatible-pointer-types', '-Wno-error', str(f)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return [l.split(':', 1)[1] for l in r.stderr.splitlines() if 'error:' in l or 'warning:' in l]


def annotate(root, mainfile):
    """Give every node .parent, and every bare location its file (clang elides repeats)."""
    cur = [None]

    def fixloc(d):
        if not isinstance(d, dict):
            return
        if 'spellingLoc' in d or 'expansionLoc' in d:
            fixloc(d.get('spellingLoc'))
            fixloc(d.get('expansionLoc'))
            return
        if 'offset' in d:
            if 'file' in d:
                cur[0] = d['file']
            d['_file'] = cur[0]

    def ex(d):  # the expansion location of a loc
        if d and 'expansionLoc' in d:
            return d['expansionLoc']
        return d

    def walk(n, parent):
        n['_parent'] = parent
        fixloc(n.get('loc'))
        r = n.get('range')
        if r:
            fixloc(r.get('begin'))
            fixloc(r.get('end'))
            b, e = ex(r.get('begin')), ex(r.get('end'))
            if b and e and 'offset' in b and 'offset' in e:
                n['_b'] = b['offset']
                n['_e'] = e['offset'] + e.get('tokLen', 0)
                n['_main'] = b.get('_file') == mainfile and e.get('_file') == mainfile
        for c in n.get('inner', []):
            walk(c, n)
    walk(root, None)


def walk(n):
    yield n
    for c in n.get('inner', []):
        yield from walk(c)
