#!/usr/bin/env python3
"""Raw byte-offset hero accesses -> struct Hero fields (include/hero.h).

  python3 tools/tidy/rawhero.py [--dry] [--func=NAME] [--skip=NAME] [--list] FILE...

Takes byte-pointer locals (char, unsigned char, u8, s8) whose only values are
`hero_globals_base + 0xE1D` (with or without a `(char *)` cast) and rewrites,
inside the local's scope, `*(T *)(p + OFF)`, `*(T *)((char *)p + OFF)`, `p[OFF]`
and `p + OFF` to `p->field`, `*(T *)&p->field` (type differs) and `&p->field`.
When every use of the local became a field access, the local turns into a
`struct Hero *`; otherwise the accesses read `((struct Hero *)p)->field`.
`*(T *)(hero_globals_base + 0xE1D + OFF)` is rewritten in place too.
Offsets with no field starting there (or only padding) stay raw.
The layout comes from clang's record dump of include/hero.h (EE n32 sizes).
--func=NAME limits it to functions, --skip=NAME leaves them out, --noinline=NAME keeps
the in-place `hero_globals_base + 0xE1D + OFF` reads of a function raw, --list prints
the functions it changes. The bytes must not change: check each file
(tools/overlay_file_check.py, or the full build) and drop the functions that do.
"""
import re, sys, subprocess, tempfile, os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
dry = '--dry' in sys.argv; LIST = '--list' in sys.argv
ONLY = [a[7:] for a in sys.argv[1:] if a.startswith('--func=')]
SKIP = [a[7:] for a in sys.argv[1:] if a.startswith('--skip=')]
NOINL = [a[11:] for a in sys.argv[1:] if a.startswith('--noinline=')]
files = [a for a in sys.argv[1:] if not a.startswith('--')]

# ---------------------------------------------------------------- layout
SCALAR = {'f32': 4, 's32': 4, 'u32': 4, 's16': 2, 'u16': 2, 'u8': 1, 's8': 1, 'VecQuad': 16,
          'float': 4, 'int': 4, 'unsigned int': 4, 'short': 2, 'unsigned short': 2,
          'char': 1, 'unsigned char': 1, 'signed char': 1}
CANON = {'f32': 'float', 's32': 'int', 'signed int': 'int', 'u32': 'unsigned int', 'unsigned': 'unsigned int',
         's16': 'short', 'signed short': 'short', 'u16': 'unsigned short', 'u8': 'unsigned char',
         's8': 'char', 'signed char': 'char', 'char': 'char'}


def canon(t):
    t = re.sub(r'\s+', ' ', t.strip()); t = re.sub(r'\s*\*', ' *', t).replace('* *', '**')
    stars = t.count('*'); base = t.replace('*', '').strip()
    return CANON.get(base, base) + ' ' + '*' * stars if stars else CANON.get(base, base)


class N:
    def __init__(s, off, name, typ):
        s.off, s.name, s.typ, s.kids = off, name, typ, []


def dump():
    with tempfile.TemporaryDirectory() as d:
        c = Path(d) / 'p.c'; c.write_text('#include "hero.h"\nint sz[sizeof(struct Hero)];\n')
        r = subprocess.run(['clang', '--target=mips64el-linux-gnuabin32', '-std=gnu89', '-fsyntax-only',
                            '-I', str(ROOT / 'include'), '-Xclang', '-fdump-record-layouts-complete',
                            '-Xclang', '-fdump-record-layouts', str(c)], capture_output=True, text=True)
    recs, sizes = {}, {}
    for blk in r.stdout.split('*** Dumping AST Record Layout')[1:]:
        lines = [l for l in blk.splitlines() if '|' in l]
        sz = re.search(r'sizeof=(\d+)', blk)
        stack = []
        for l in lines:
            m = re.match(r'\s*(\d+)(?::\S+)? \|( *)(.*)$', l)
            if not m: continue
            lvl = (len(m.group(2)) - 1) // 2; body = m.group(3)
            if lvl == 0:
                root = N(0, '', body); stack = [root]; continue
            tm = re.match(r'(.*?)\s*(\w+)$', body)
            n = N(int(m.group(1)), tm.group(2), tm.group(1))
            del stack[lvl:]; stack[-1].kids.append(n); stack.append(n)
        if stack:
            name = stack[0].typ
            if name not in recs: recs[name] = stack[0]; sizes[name] = int(sz.group(1))
            # nested expansions give the layout of typedef'd members (Vec4) for arrays of them
            def reg(n):
                for k in n.kids:
                    if k.kids and k.typ not in recs:
                        recs[k.typ] = k; sizes[k.typ] = None
                    reg(k)
            reg(stack[0])
    return recs, sizes


RECS, SIZES = dump()
HERO = RECS['struct Hero']


def tsize(t, node=None):
    t = t.strip()
    m = re.match(r'(.*)\[(\d+)\]$', t)
    if m: return tsize(m.group(1)) * int(m.group(2))
    if t.endswith('*'): return 4
    if t in SCALAR: return SCALAR[t]
    if SIZES.get(t): return SIZES[t]
    n = node if node is not None and node.kids else RECS.get(t)
    if n is None: raise KeyError(t)
    return max(k.off - n.off + tsize(k.typ, k) for k in n.kids)


def is_union(t, n):
    return t.startswith('union') or (n is not None and len(n.kids) > 1 and all(k.off == n.off for k in n.kids))


def cands(node, rel, path, base):
    """Every member starting at byte rel of node (outermost first): (path, type, size, in_union, pad)."""
    out = []
    t = node.typ.strip()
    m = re.match(r'(.*)\[(\d+)\]$', t)
    if m:
        et = m.group(1).strip(); es = tsize(et); k = rel // es
        if k >= int(m.group(2)): return out
        sub = N(base + k * es, '', et)
        tmpl = RECS.get(et)
        if tmpl is not None: sub.kids = [N(c.off - tmpl.off + sub.off, c.name, c.typ) for c in tmpl.kids]; _copy(sub, tmpl)
        p = f'{path}[{k}]'
        if rel == k * es: out.append((p, et, es, False))
        out += cands(sub, rel - k * es, p, sub.off)
        return out
    kids = node.kids or (RECS[t].kids if t in RECS and RECS[t] is not node else [])
    if kids and not node.kids: _copy(node, RECS[t]); kids = node.kids
    un = is_union(t, node)
    for c in kids:
        crel = c.off - node.off
        try: cs = tsize(c.typ, c)
        except KeyError: continue
        if crel <= rel < crel + cs:
            if c.name.startswith('pad'): continue
            p = f'{path}.{c.name}' if path else c.name
            if rel == crel: out.append((p, c.typ.strip(), cs, un))
            out += [(a, b, s, u or un) for a, b, s, u in cands(c, rel - crel, p, c.off)]
    return out


def _copy(dst, tmpl):
    """Give dst the nested layout of tmpl, rebased to dst.off."""
    def cp(src, d):
        d.kids = []
        for c in src.kids:
            n = N(c.off - tmpl.off + dst.off, c.name, c.typ); d.kids.append(n); cp(c, n)
    cp(tmpl, dst)


def pick_value(off, T):
    """(path, cast) for a T read at off; cast True means `*(T *)&`."""
    cs = cands(HERO, off, '', 0)
    if not cs: return None
    ct = canon(T)
    ex = [c for c in cs if not re.search(r'\[\d+\]$', c[1]) and canon(c[1]) == ct]
    if ex: return ex[-1][0], False
    try: sz = tsize(T if T in SCALAR or T.endswith('*') else T)
    except KeyError: sz = None
    same = [c for c in cs if c[2] == sz]
    if same:
        # the outermost whole member of that size (Vec4 over its q), else the deepest
        nonu = [c for c in same if not c[3]]
        return (nonu[0] if nonu else same[-1])[0], True
    return cs[-1][0], True


CONTAINERS = {'struct HeroMotion', 'struct HeroState', 'struct HeroHealth'}


def pick_addr(off):
    cs = cands(HERO, off, '', 0)
    if not cs: return None
    i = 0
    while cs[i][1] in CONTAINERS and i + 1 < len(cs): i += 1
    path, t, _, _ = cs[i]
    if re.search(r'\[\d+\]$', t): return path          # an array decays to its first element
    return '&' + path


# ---------------------------------------------------------------- source
HEROX = r'(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?hero_globals_base\s*\+\s*0xE1D'
BYTE = r'(?:unsigned\s+char|signed\s+char|char|u8|s8)'
DEFANY = re.compile(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\(([^;{]*)\)\s*\{', re.M)
NUM = r'(0x[0-9A-Fa-f]+|\d+)'


def block_end(t, pos):
    d = 0
    for j in range(pos, len(t)):
        if t[j] == '{': d += 1
        elif t[j] == '}':
            d -= 1
            if d < 0: return j
    return len(t)


tot = 0
raw_left = 0


def convert(body, fname):
    """Rewrite one function body; returns (new body, count)."""
    global raw_left
    cnt = 0
    # inline: *(T *)(hero_globals_base + 0xE1D + OFF)
    def inl(x):
        nonlocal cnt
        T = x.group(1).strip(); r = pick_value(int(x.group(2), 0), T)
        if not r: return x.group(0)
        cnt += 1; e = f'((struct Hero *)(hero_globals_base + 0xE1D))->{r[0]}'
        return f'*({T}{"" if T.endswith("*") else " "}*)&{e}' if r[1] else e

    # inline: *(T *)(hero_globals_base + NUM) where NUM >= 0xE1D
    def inl_direct(x):
        nonlocal cnt
        T = x.group(1).strip()
        sym = x.group(2)
        raw_num = int(x.group(3), 0)
        if raw_num < 0xE1D: return x.group(0)
        off = raw_num - 0xE1D
        r = pick_value(off, T)
        if not r: return x.group(0)
        cnt += 1; e = f'((struct Hero *)((char *)&{sym} + 0xE1D))->{r[0]}'
        return f'*({T}{"" if T.endswith("*") else " "}*)&{e}' if r[1] else e

    # address: hero_globals_base + NUM where NUM > 0xE1D
    def addr_direct(x):
        nonlocal cnt
        nxt = x.string[x.end():].lstrip()
        if nxt.startswith(')[') or nxt.startswith('['): return x.group(0)
        sym = x.group(1)
        raw_num = int(x.group(2), 0)
        if raw_num <= 0xE1D: return x.group(0)
        off = raw_num - 0xE1D
        addr = pick_addr(off)
        if not addr: return x.group(0)
        cnt += 1
        if addr.startswith('&'):
            return f'&((struct Hero *)((char *)&{sym} + 0xE1D))->{addr[1:]}'
        else:
            return f'((struct Hero *)((char *)&{sym} + 0xE1D))->{addr}'

    if fname not in NOINL:
        body = re.sub(r'\*\s*\(\s*([A-Za-z_][\w ]*?(?:\s*\*)*)\s*\*\s*\)\s*\(\s*' + HEROX + r'\s*\+\s*' + NUM + r'\s*\)', inl, body)
        body = re.sub(r'\*\s*\(\s*([A-Za-z_][\w ]*?(?:\s*\*)*)\s*\*\s*\)\s*\(\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?&?\s*(hero_globals_base(?:_\w+)?)\s*\+\s*' + NUM + r'\s*\)', inl_direct, body)
        body = re.sub(r'(?<![\w.>])(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?&?\s*(hero_globals_base(?:_\w+)?)\s*\+\s*' + NUM + r'(?![\w\[(])', addr_direct, body)

    decl = re.compile(r'(?:(?<=[;{}])|^)(\s*)(?:const\s+)?(' + BYTE + r')\s*\*\s*(\w+)\s*(=\s*([^;,]*?))?\s*([;,])', re.M)
    ds = list(decl.finditer(body))
    # innermost (latest) first so outer scopes see rewritten inner ones
    done = []
    for d in reversed(ds):
        v, et = d.group(3), re.sub(r'\s+', ' ', d.group(2))
        init = d.group(5)
        start = d.end(); end = block_end(body, start)
        seg = body[start:end]
        # nested redeclarations of v (any type) end the region
        inner = [m for m in re.finditer(r'(?:^|[;{}(])\s*(?:const\s+|unsigned\s+|signed\s+|struct\s+|union\s+)*\w+\s*(?:\*\s*)*\b%s\s*[=;,\[)]' % v, seg, re.M)
                 if not re.match(r'\s*(return|else|case|goto)\b', seg[m.start() + 1:])]
        if inner: continue
        if init is not None and not re.fullmatch(HEROX, init.strip()): continue
        asg = list(re.finditer(r'(?<![\w.>])%s\s*=(?!=)\s*([^;]*);' % v, seg))
        if any(not re.fullmatch(HEROX, a.group(1).strip()) for a in asg): continue
        if init is None and not asg: continue
        if re.search(r'(\+\+|--)\s*%s\b|\b%s\s*(\+\+|--|[-+|&^*/]=)|&\s*%s\b' % (v, v, v), seg): continue
        H = f'((struct Hero *){v})'
        c0 = cnt

        def val(x):
            nonlocal cnt
            T = re.sub(r'\s*((?:\*\s*)+)$', lambda m: ' ' + m.group(1).replace(' ', ''), x.group(1).strip()); r = pick_value(int(x.group(2), 0), T)
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return f'*({T}{"" if T.endswith("*") else " "}*)&{e}' if r[1] else e
        seg2 = re.sub(r'\*\s*\(\s*([A-Za-z_][\w ]*?(?:\s*\*)*)\s*\*\s*\)\s*\(\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM), val, seg)

        def idx(x):
            nonlocal cnt
            r = pick_value(int(x.group(1), 0), et)
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return f'*({et} *)&{e}' if r[1] else e
        seg2 = re.sub(r'(?<![\w.>])%s\s*\[\s*%s\s*\]' % (v, NUM), idx, seg2)

        def addr_e(off):
            r = pick_addr(off)
            if not r: return None
            return '&' + H + '->' + r[1:] if r.startswith('&') else H + '->' + r

        def caddr(x):
            nonlocal cnt
            if x.string[x.end():x.end() + 1] == '[': return x.group(0)
            e = addr_e(int(x.group(2), 0))
            if not e: return x.group(0)
            cnt += 1; return f'({x.group(1).strip()})' + e
        seg2 = re.sub(r'\(\s*([A-Za-z_][\w ]*?(?:\s*\*)+)\s*\)\s*\(\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM), caddr, seg2)

        def baddr(x):
            nonlocal cnt
            s_ = x.string; i = x.start() - 1
            while i >= 0 and s_[i] in ' \t\n': i -= 1
            j = x.end()
            while j < len(s_) and s_[j] in ' \t\n': j += 1
            prev, nxt = s_[i] if i >= 0 else '', s_[j:j + 1]
            ret = re.search(r'\breturn$', s_[:i + 1]) is not None
            if not (prev in '(,=?:' or ret) or nxt not in '),;:?' or not nxt: return x.group(0)
            if prev in '=?:' and s_[i - 1:i + 1] in ('==', '!=', '<=', '>='): return x.group(0)
            e = addr_e(int(x.group(1), 0))
            if not e: return x.group(0)
            cnt += 1
            k = i - 1
            while k >= 0 and s_[k] in ' \t': k -= 1
            call = prev == '(' and k >= 0 and (s_[k].isalnum() or s_[k] == '_') and not re.search(r'\b(if|while|for|switch|return|sizeof)$', s_[:k + 1])
            if (call or prev == ',') and nxt in ',)': return e
            return f'({et} *)' + e
        seg2 = re.sub(r'(?<![\w.>)\]])(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?%s\s*\+\s*%s(?![\w\[(])' % (v, NUM), baddr, seg2)
        if cnt == c0: continue
        # retype when nothing raw is left
        rest = re.sub(r'\(\(struct Hero \*\)%s\)' % v, '', seg2)
        rest = re.sub(r'(?<![\w.>])%s\s*=\s*' % v + HEROX, '', rest)
        if not re.search(r'(?<![\w.>])%s\b' % v, rest) and d.group(6) == ';' and v not in MACRO_IDS:
            seg2 = seg2.replace(H, v)
            seg2 = re.sub(r'((?<![\w.>])%s\s*=\s*)(' % v + HEROX + ')', lambda m: m.group(1) + '(struct Hero *)(' + m.group(2) + ')', seg2)
            nd = f'{d.group(1)}struct Hero *{v}' + (f' = (struct Hero *)({init.strip()})' if init is not None else '') + ';'
            body = body[:d.start()] + nd + seg2 + body[end:]
        else:
            body = body[:start] + seg2 + body[end:]
    return body, cnt


MACRO_IDS = set()
for p in files:
    t = open(p).read(); res = []; n = 0
    # a macro body that names the local reads it raw: keep its byte type
    MACRO_IDS = set(re.findall(r'\w+', ' '.join(re.findall(r'^\s*#\s*define\s+\w+(?:\([^)]*\))?(.*)$', t, re.M))))
    for m in DEFANY.finditer(t):
        if ONLY and m.group(1) not in ONLY: continue
        if m.group(1) in SKIP: continue
        start = t.index('{', m.end() - 1); end = block_end(t, start + 1)
        body = t[start:end + 1]
        nb, c = convert(body, m.group(1))
        if nb != body:
            if LIST: print('FUNC', m.group(1), c)
            res.append((start, end + 1, nb)); n += c
    out = t
    for s_, e_, nb in reversed(res): out = out[:s_] + nb + out[e_:]
    if n:
        if '#include "hero.h"' not in out:
            incs = list(re.finditer(r'^#include .*\n', out, re.M))
            at = incs[-1].end() if incs else 0
            out = out[:at] + '#include "hero.h"\n' + out[at:]
        tot += n; print(f'{n}\t{p}')
        if not dry: open(p, 'w').write(out)
print('total', tot, file=sys.stderr)
