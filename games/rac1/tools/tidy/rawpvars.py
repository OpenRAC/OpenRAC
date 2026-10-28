#!/usr/bin/env python3
"""Raw byte-offset moby pvars accesses -> fields of a pvars struct.

  python3 tools/tidy/rawpvars.py --struct=NAME [--dry] [--func=A,B] [--skip=A] [--list]
                                 [--retype] [--noinline=A] FILE
  python3 tools/tidy/rawpvars.py --infer [--struct=NAME] [--func=A,B] FILE

A moby class keeps its private variables in `moby->pvars`. The code reads them
through a byte-pointer local (char, unsigned char, u8, s8) whose only values are a
pvars read: `(char *)((Moby *)m)->pvars`, `m->pvars` or `*(char **)(m + 0x78)`.
Inside that local's scope these become fields of struct NAME:
  *(T *)(d + OFF), *(T *)((char *)d + OFF), d[OFF], ((T *)d)[N]  -> d->field
  (T *)(d + OFF), d + OFF, (s32)(d + OFF)                       -> &d->field
  ((Moby *)d)->unk80, ((OtherType *)d)->f180[i]                 -> d->field
  FastVecAdd(x, x, d) (vector callees only, see BARE_OK)        -> &d->field
A read of another type gets `*(T *)&d->field`; a plain integer store that only
differs in signedness does not. An address passed to a prototyped callee gets the
parameter's cast when the field type differs (no new warnings). When no raw use of
the local is left it is retyped to `struct NAME *d = (struct NAME *)...->pvars;`,
else the accesses read `((struct NAME *)d)->field`; --retype retypes it anyway when
the raw uses are few (at most a quarter of the accesses) and writes them `((char *)d)`.
`*(T *)(((char *)((Moby *)o)->pvars) + OFF)` is rewritten in place too; --noinline=F
keeps those raw in F (they may read another class's pvars). Offsets with no field
starting there (or only `pad*` members) stay raw.

struct NAME must be a named struct defined in FILE before the functions
(`struct NAME {...};`, a typedef of it is fine). Its layout comes from clang's record
dump of the whole file (EE long sizes via clast.py), so nested structs, arrays and
unions work. --func limits it to functions (repeat or comma list), --skip leaves
them out, --list prints the functions it changes and their counts.

--infer prints a draft struct from the offsets and types the selected functions use
(fields `unk_<OFF>`, padding between); rename the fields, paste it after the
includes, then convert.

The bytes must not change: check the file with
`bash tools/docker/run.sh python tools/overlay_file_check.py FILE` and drop (--skip)
the functions that do.
"""
import re, sys, subprocess, tempfile
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import clast  # EE long sizes for clang

ROOT = Path(__file__).resolve().parents[2]
ARGS = sys.argv[1:]
dry = '--dry' in ARGS; LIST = '--list' in ARGS; INFER = '--infer' in ARGS; RETYPE = '--retype' in ARGS


def opt_list(p):
    out = []
    for a in ARGS:
        if a.startswith(p): out += [x for x in a[len(p):].split(',') if x]
    return out


ONLY = opt_list('--func='); SKIP = opt_list('--skip='); NOINL = opt_list('--noinline=')
SNAME = (opt_list('--struct=') or [None])[0]
if SNAME and SNAME.startswith('struct '): SNAME = SNAME[7:]
files = [a for a in ARGS if not a.startswith('--')]

# ---------------------------------------------------------------- layout
SCALAR = {'f32': 4, 's32': 4, 'u32': 4, 's16': 2, 'u16': 2, 'u8': 1, 's8': 1, 'VecQuad': 16,
          'float': 4, 'int': 4, 'unsigned int': 4, 'short': 2, 'unsigned short': 2,
          'char': 1, 'unsigned char': 1, 'signed char': 1, 'long': 8, 'unsigned long': 8,
          's64': 8, 'u64': 8}
CANON = {'f32': 'float', 's32': 'int', 'signed int': 'int', 'u32': 'unsigned int', 'unsigned': 'unsigned int',
         's16': 'short', 'signed short': 'short', 'u16': 'unsigned short', 'u8': 'unsigned char',
         's8': 'char', 'signed char': 'char', 'char': 'char', 'struct Moby': 'Moby'}


def canon(t):
    a = re.match(r'(.*?)\s*((?:\[\w+\])+)$', t.strip())
    if a: return canon(a.group(1)) + a.group(2)
    t = re.sub(r'\s+', ' ', t.strip()); t = re.sub(r'\s*\*', ' *', t).replace('* *', '**')
    stars = t.count('*'); base = t.replace('*', '').strip()
    return CANON.get(base, base) + ' ' + '*' * stars if stars else CANON.get(base, base)


class N:
    def __init__(s, off, name, typ):
        s.off, s.name, s.typ, s.kids = off, name, typ, []


RECS, SIZES = {}, {}


def struct_def_end(t, name):
    """End offset of the definition of struct NAME in t (after its `;`)."""
    m = re.search(r'\bstruct\s+%s\s*\{' % re.escape(name), t)
    if not m:
        m = re.search(r'typedef\s+struct\s*\{', t)  # anonymous typedef: find `} NAME;`
        e = re.search(r'\}\s*%s\s*;' % re.escape(name), t)
        if not e: return None
        return e.end()
    d = 0
    for j in range(m.end() - 1, len(t)):
        if t[j] == '{': d += 1
        elif t[j] == '}':
            d -= 1
            if d == 0: return t.index(';', j) + 1
    return None


CASTT = r'\(\s*\(\s*((?:struct\s+)?[A-Za-z_]\w*)\s*\*\s*\)\s*\w+\s*\)\s*->'


def dump(path, name):
    t = Path(path).read_text()
    if name is not None:
        if struct_def_end(t, name) is None: sys.exit(f'struct {name} not defined in {path}')
        tag = f'struct {name}' if re.search(r'\bstruct\s+%s\s*\{' % re.escape(name), t) else name
    else: tag = 'int'
    # the whole file (clang still dumps every complete record past errors in function bodies),
    # plus one probe record per type the pvars locals are cast to: `((X *)d)->field`
    probes = ''.join(f'struct __rawpvars_p_{re.sub(r"\\W", "_", x)} {{ {x} a; }};\n'
                     for x in sorted(set(re.findall(CASTT, t))) if not x.startswith('struct '))
    src = t + f'\nint __rawpvars_sz[sizeof({tag})];\n' + probes
    with tempfile.TemporaryDirectory() as d:
        c = Path(d) / 'p.c'; c.write_text(clast.ee(src)); clast.prep_includes()
        r = subprocess.run(['clang', '--target=mips64el-linux-gnuabin32', '-std=gnu89', '-fsyntax-only', '-w',
                            '-ferror-limit=0', '-I', str(clast.INC_T), '-include', str(clast.PRELUDE),
                            '-I', str(Path(path).resolve().parent),
                            '-Xclang', '-fdump-record-layouts-complete',
                            '-Xclang', '-fdump-record-layouts', str(c)], capture_output=True, text=True)
    for blk in clast.unee(r.stdout).split('*** Dumping AST Record Layout')[1:]:
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
            if not tm: continue
            n = N(int(m.group(1)), tm.group(2), tm.group(1))
            del stack[lvl:]; stack[-1].kids.append(n); stack.append(n)
        if stack:
            nm = stack[0].typ
            RECS[nm] = stack[0]; SIZES[nm] = int(sz.group(1))
            def reg(n):
                for k in n.kids:
                    if k.kids and k.typ not in RECS:
                        RECS[k.typ] = k; SIZES[k.typ] = None
                    reg(k)
            reg(stack[0])
    if name is None: return None
    key = f'struct {name}'
    if key not in RECS:
        errs = [l for l in r.stderr.splitlines() if 'error' in l][:5]
        sys.exit(f'no layout for {key}: ' + ' | '.join(errs))
    return RECS[key]


def dump_moby(path):
    """Layouts of the file's records (struct Moby, cast types), for --infer."""
    dump(path, None)


def tsize(t, node=None):
    t = t.strip()
    m = re.match(r'(.*)\[(\d+)\]$', t)
    if m: return tsize(m.group(1)) * int(m.group(2))
    if t.endswith('*'): return 4
    if t in SCALAR: return SCALAR[t]
    if SIZES.get(t): return SIZES[t]
    n = node if node is not None and node.kids else RECS.get(t)
    if n is None or not n.kids: raise KeyError(t)
    return max(k.off - n.off + tsize(k.typ, k) for k in n.kids)


def is_union(t, n):
    return t.startswith('union') or (n is not None and len(n.kids) > 1 and all(k.off == n.off for k in n.kids))


def _copy(dst, tmpl):
    def cp(src, d):
        d.kids = []
        for c in src.kids:
            n = N(c.off - tmpl.off + dst.off, c.name, c.typ); d.kids.append(n); cp(c, n)
    cp(tmpl, dst)


def cands(node, rel, path, base):
    """Every member starting at byte rel of node (outermost first): (path, type, size, in_union)."""
    out = []
    t = node.typ.strip()
    m = re.match(r'(.*)\[(\d+)\]$', t)
    if m:
        et = m.group(1).strip(); es = tsize(et); k = rel // es
        if k >= int(m.group(2)): return out
        sub = N(base + k * es, '', et)
        tmpl = RECS.get(et)
        if tmpl is not None: _copy(sub, tmpl)
        p = f'{path}[{k}]'
        if rel == k * es: out.append((p, et, es, False))
        out += cands(sub, rel - k * es, p, sub.off)
        return out
    if not node.kids and t in RECS and RECS[t] is not node: _copy(node, RECS[t])
    un = is_union(t, node)
    for c in node.kids:
        crel = c.off - node.off
        try: cs = tsize(c.typ, c)
        except KeyError: continue
        if crel <= rel < crel + cs:
            if c.name.startswith('pad'): continue
            p = f'{path}.{c.name}' if path else c.name
            if rel == crel: out.append((p, c.typ.strip(), cs, un))
            out += [(a, b, s, u or un) for a, b, s, u in cands(c, rel - crel, p, c.off)]
    return out


ROOTN = None


def pick_value(off, T):
    """(path, cast) for a T read at off; cast True means `*(T *)&`."""
    cs = cands(ROOTN, off, '', 0)
    if not cs: return None
    ct = canon(T)
    ex = [c for c in cs if not re.search(r'\[\d+\]$', c[1]) and canon(c[1]) == ct]
    if ex: return ex[-1][0], False
    try: sz = tsize(T)
    except KeyError: sz = None
    same = [c for c in cs if c[2] == sz]
    if same:
        nonu = [c for c in same if not c[3]]
        return (nonu[0] if nonu else same[-1])[0], True
    return cs[-1][0], True


def pick_addr(off, T=None):
    """Field expression for the address of off: `&path`, or `path` for an array."""
    cs = cands(ROOTN, off, '', 0)
    if not cs: return None
    c = cs[0]
    if T is not None:   # a cast address: prefer the member whose type is the pointee
        ct = canon(T)
        ex = [x for x in cs if canon(x[1]) == ct]
        if ex: c = ex[-1]
    path, t, _, _ = c
    if re.search(r'\[\d+\]$', t): return path
    return '&' + path


def addr_type(off, T=None):
    """Pointer type of pick_addr's expression (an array decays to its element)."""
    cs = cands(ROOTN, off, '', 0)
    c = cs[0]
    if T is not None:
        ex = [x for x in cs if canon(x[1]) == canon(T)]
        if ex: c = ex[-1]
    t = c[1]
    m = re.match(r'(.*?)\s*\[\d+\]$', t)
    return (m.group(1) if m else t) + ' *'


PROTOS = {}  # callee -> parameter types, from the file's declarations


def read_protos(t):
    PROTOS.clear()
    for m in re.finditer(r'\b(\w+)\s*\(((?:[^;{}()]|\([^;{}()]*\))*)\)\s*(?:__asm__\s*\([^)]*\)\s*)?;', t):
        ps, d, cur = [], 0, ''
        for ch in m.group(2):
            if ch == ',' and d == 0: ps.append(cur.strip()); cur = ''; continue
            d += ch == '('; d -= ch == ')'; cur += ch
        ps.append(cur.strip())
        PROTOS.setdefault(m.group(1), ps)


def arg_cast(name, idx, ptype):
    """`(P)` when the callee's prototype wants a pointer type other than ptype (no warning)."""
    ps = PROTOS.get(name)
    if not ps or idx >= len(ps): return ''
    P = ps[idx]
    if '*' not in P or '(' in P or '...' in P: return ''
    P = re.sub(r'(\*)\s*\w+\s*(\[\s*\])?$', r'\1', P).strip()
    P = re.sub(r'^(const|register)\s+', '', P)
    unsign = lambda x: canon(x).replace('unsigned ', '')  # gcc 2.95 does not warn on signedness
    if canon(P) == 'void *' or unsign(P) == unsign(ptype): return ''
    return f'({P})'


def call_ctx(s, i):
    """(callee, argument index) of the argument that starts after position i."""
    d = 0; idx = 0; j = i
    while j >= 0:
        ch = s[j]
        if ch in ')]': d += 1
        elif ch in '([':
            if d == 0:
                if ch == '[': return None
                k = j - 1
                while k >= 0 and s[k] in ' \t\n': k -= 1
                m = re.search(r'(\w+)$', s[:k + 1])
                if not m or m.group(1) in ('if', 'while', 'for', 'switch', 'return', 'sizeof'): return None
                return m.group(1), idx
            d -= 1
        elif ch == ',' and d == 0: idx += 1
        elif ch in ';{}': return None
        j -= 1
    return None


# ---------------------------------------------------------------- source
BYTE = r'(?:unsigned\s+char|signed\s+char|char|u8|s8)'
CORE = r'(?:\(\s*\(\s*(?:struct\s+)?Moby\s*\*\s*\)\s*\w+\s*\)|\w+)\s*->\s*pvars'
BCAST = r'\(\s*(?:const\s+)?' + BYTE + r'\s*\*\s*\)'
DEFANY = re.compile(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\(([^;{]*)\)\s*\{', re.M)
NUM = r'(0x[0-9A-Fa-f]+|\d+)'
TYP = r'([A-Za-z_][\w ]*?(?:\s*\*)*)'
# inline: ((char *)((Moby *)m)->pvars) or (char *)m->pvars, as one parenthesised operand
INL = r'\(\s*' + BCAST + r'\s*(' + CORE + r')\s*\)'


RAWPV = r'\*\s*\(\s*(?:unsigned\s+)?char\s*\*\s*\*\s*\)\s*\(\s*(?:\(\s*char\s*\*\s*\)\s*)?\w+\s*\+\s*0x78\s*\)'


def pvars_init(s):
    """The CORE text when s is only a pvars read (optionally cast to a byte pointer)."""
    s = s.strip()
    while s.startswith('(') and s.endswith(')') and _bal(s[1:-1]): s = s[1:-1].strip()
    m = re.fullmatch(r'(?:' + BCAST + r'\s*)?(' + CORE + r')', s)
    return m.group(1) if m else None


def _bal(s):
    d = 0
    for ch in s:
        d += ch == '('; d -= ch == ')'
        if d < 0: return False
    return d == 0

MOBYF = r'\(\s*\(\s*((?:struct\s+)?[A-Za-z_]\w*)\s*\*\s*\)\s*%s\s*\)\s*->\s*(\w+(?:\.\w+)*)(?![\w.\[]|\s*->)'
CIDX = r'\(\s*\(\s*(?:const\s+)?' + TYP + r'\s*\*\s*\)\s*%s\s*\)\s*\[\s*' + NUM + r'\s*\]'


def moby_field(X, name):
    """(offset, type) of a member path `a` or `a.b` of type X (the pvars local cast to Moby
    or a file-local struct by older passes: `((Moby *)d)->unk80`, `->rot_row1.y`)."""
    X = re.sub(r'\s+', ' ', X.strip())
    if X == 'Moby': X = 'struct Moby'
    if X == f'struct {SNAME}': return None
    mo = RECS.get(X)
    if mo is None:
        pr = RECS.get(f'struct __rawpvars_p_{re.sub(r"\W", "_", X)}')
        mo = pr.kids[0] if pr and pr.kids else None
    off = 0; t = None
    for part in name.split('.'):
        kids = (mo.kids or (RECS[mo.typ.strip()].kids if mo.typ.strip() in RECS else [])) if mo else []
        k = next((k for k in kids if k.name == part), None)
        if k is None: return None
        off += k.off - mo.off if mo.kids else k.off - RECS[mo.typ.strip()].off
        mo, t = k, k.typ.strip()
    if t is None or re.search(r'[\[(]', t): return None
    return off, t


def cidx_off(T, n):
    try: return int(n, 0) * tsize(T)
    except KeyError: return None


def block_end(t, pos):
    d = 0
    for j in range(pos, len(t)):
        if t[j] == '{': d += 1
        elif t[j] == '}':
            d -= 1
            if d < 0: return j
    return len(t)


def tnorm(T):
    return re.sub(r'\s*((?:\*\s*)+)$', lambda m: ' ' + m.group(1).replace(' ', ''), T.strip())


# callees whose bare pvars argument is the vector at offset 0
BARE_OK = r'\w*[Vv]ec\w*|qcopy|qzero|build_look_at_matrix|clear_u64_value'
INTS = {'char': 1, 'unsigned char': 1, 'short': 2, 'unsigned short': 2, 'int': 4, 'unsigned int': 4}


def plain_store(x, T, path):
    """A `v[OFF] = ...` store of an integer whose field differs only in signedness: no cast needed."""
    s_ = x.string; j = x.end()
    while j < len(s_) and s_[j] in ' \t\n': j += 1
    if not (s_[j:j + 1] == '=' and s_[j + 1:j + 2] != '='): return False
    i = x.start() - 1
    while i >= 0 and s_[i] in ' \t\n': i -= 1
    if i >= 0 and s_[i] in '=*&': return False
    a, b = canon(T), canon(field_type(path))
    return a in INTS and b in INTS and INTS[a] == INTS[b]


def field_type(path):
    """Type of a field path like `a.b[2]`."""
    node = ROOTN; t = None
    for part in re.findall(r'\.?(\w+)|\[(\d+)\]', path):
        name, k = part
        if name:
            kids = node.kids or (RECS[node.typ.strip()].kids if node.typ.strip() in RECS else [])
            node = next(c for c in kids if c.name == name); t = node.typ.strip()
        else:
            et = re.match(r'(.*)\[\d+\]$', t).group(1).strip(); t = et
            node = RECS.get(et, N(0, '', et))
    return t


def deref(T, e):
    return f'*({T}{"" if T.endswith("*") else " "}*)&{e}'


DECL = re.compile(r'(?:(?<=[;{}])|^)(\s*)(?:const\s+)?(' + BYTE + r')\s*\*\s*(\w+)\s*(=\s*([^;,]*?))?\s*([;,])', re.M)


def locals_of(body):
    """(decl match, var, elem type, init core, start, end) of each pvars-base local, innermost first."""
    out = []
    for d in reversed(list(DECL.finditer(body))):
        v, et = d.group(3), re.sub(r'\s+', ' ', d.group(2))
        init = d.group(5)
        start = d.end(); end = block_end(body, start)
        seg = body[start:end]
        inner = [m for m in re.finditer(r'(?:^|[;{}(])\s*(?:const\s+|unsigned\s+|signed\s+|struct\s+|union\s+)*\w+\s*(?:\*\s*)*\b%s\s*[=;,\[)]' % v, seg, re.M)
                 if not re.match(r'\s*(return|else|case|goto)\b', seg[m.start() + 1:])]
        if inner: continue
        if init is not None and pvars_init(init) is None: continue
        asg = list(re.finditer(r'(?<![\w.>=!<])%s\s*=(?!=)\s*([^;]*);' % v, seg))
        if any(pvars_init(a.group(1)) is None for a in asg): continue
        if init is None and not asg: continue
        if re.search(r'(\+\+|--)\s*%s\b|\b%s\s*(\+\+|--|[-+|&^*/]=)|(?:(?<=[=(,;{}!?:])|(?<=return))\s*&\s*%s\b' % (v, v, v), seg): continue
        out.append((d, v, et, init, start, end))
    return out


def convert(body, fname, S):
    """Rewrite one function body; returns (new body, count)."""
    cnt = 0

    def inl(x):
        nonlocal cnt
        T = tnorm(x.group(1)); r = pick_value(int(x.group(3), 0), T)
        if not r: return x.group(0)
        cnt += 1; e = f'((struct {S} *){x.group(2)})->{r[0]}'
        return deref(T, e) if r[1] else e
    if fname not in NOINL:  # --noinline: in-place reads may be another class's pvars
        body = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*' + INL + r'\s*\+\s*' + NUM + r'\s*\)', inl, body)

    def inl_idx(x):
        nonlocal cnt
        et = re.sub(r'\s+', ' ', re.match(r'\(\s*(?:const\s+)?(.*?)\s*\*', x.group(1)).group(1))
        r = pick_value(int(x.group(3), 0), et)
        if not r: return x.group(0)
        cnt += 1; e = f'((struct {S} *){x.group(2)})->{r[0]}'
        return f'*({et} *)&{e}' if r[1] else e
    if fname not in NOINL:
        body = re.sub(r'\(\s*(' + BCAST + r')\s*(' + CORE + r')\s*\)\s*\[\s*' + NUM + r'\s*\]', inl_idx, body)
        body = re.sub(r'\(\s*\(\s*(' + BCAST + r')\s*(' + CORE + r')\s*\)\s*\)\s*\[\s*' + NUM + r'\s*\]', inl_idx, body)

    for d, v, et, init, start, end in locals_of(body):
        start = d.end(); end = block_end(body, start)
        seg = body[start:end]
        H = f'((struct {S} *){v})'
        c0 = cnt

        def val(x):
            nonlocal cnt
            T = tnorm(x.group(1)); r = pick_value(int(x.group(2), 0), T)
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return deref(T, e) if r[1] and not plain_store(x, T, r[0]) else e
        seg2 = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*(?:' + BCAST + r'\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM), val, seg)

        def idx(x):
            nonlocal cnt
            r = pick_value(int(x.group(1), 0), et)
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return f'*({et} *)&{e}' if r[1] and not plain_store(x, et, r[0]) else e
        seg2 = re.sub(r'(?<![\w.>])%s\s*\[\s*%s\s*\]' % (v, NUM), idx, seg2)

        def cidx(x):  # ((T *)v)[N]
            nonlocal cnt
            T = tnorm(x.group(1)); off = cidx_off(T, x.group(2))
            r = pick_value(off, T) if off is not None else None
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return deref(T, e) if r[1] and not plain_store(x, T, r[0]) else e
        seg2 = re.sub(CIDX % v, cidx, seg2)

        def mfield(x):  # ((Moby *)v)->unk80
            nonlocal cnt
            mf = moby_field(x.group(1), x.group(2))
            r = pick_value(mf[0], mf[1]) if mf else None
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return deref(mf[1], e) if r[1] else e
        def mfield_t(x):  # *(int *)&((Moby *)v)->collision
            nonlocal cnt
            mf = moby_field(x.group(2), x.group(3)); T = tnorm(x.group(1))
            r = pick_value(mf[0], T) if mf else None
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return deref(T, e) if r[1] else e
        seg2 = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*&\s*' + MOBYF % v, mfield_t, seg2)
        seg2 = re.sub(MOBYF % v, mfield, seg2)

        def mfield_same(x):  # ((Sentry *)d)->f180[i]: a member of the same type at the same offset
            nonlocal cnt
            X = re.sub(r'\s+', ' ', x.group(1).strip()); X = 'struct Moby' if X == 'Moby' else X
            if X == f'struct {S}': return x.group(0)
            mo = RECS.get(X) or (RECS.get(f'struct __rawpvars_p_{re.sub(r"\W", "_", X)}') or N(0, '', '')).kids[:1]
            mo = mo[0] if isinstance(mo, list) and mo else (mo if not isinstance(mo, list) else None)
            k = next((k for k in (mo.kids if mo else []) if k.name == x.group(2)), None)
            if k is None: return x.group(0)
            same = [c for c in cands(ROOTN, k.off, '', 0) if canon(c[1]) == canon(k.typ)]
            if not same: return x.group(0)
            cnt += 1; return f'{H}->{same[0][0]}'
        seg2 = re.sub(MOBYF.replace(r'(?![\w.\[]|\s*->)', r'(?![\w])') % v, mfield_same, seg2)

        def addr_e(off, T=None):
            r = pick_addr(off, T)
            if not r: return None
            return '&' + H + '->' + r[1:] if r.startswith('&') else H + '->' + r

        def caddr(x):
            nonlocal cnt
            if x.string[x.end():x.end() + 1] == '[': return x.group(0)
            T = tnorm(x.group(1)); pt = T[:-1].strip()
            e = addr_e(int(x.group(2), 0), pt)
            if not e: return x.group(0)
            cnt += 1
            # `(float *)&v->f` where f is a float needs no cast
            r = pick_addr(int(x.group(2), 0), pt)
            cs = [c for c in cands(ROOTN, int(x.group(2), 0), '', 0) if ('&' + c[0] == r or c[0] == r)]
            if cs and r.startswith('&') and canon(cs[0][1]) == canon(pt): return e
            return f'({T})' + e
        seg2 = re.sub(r'\(\s*' + TYP.replace('*)*)', '*)+)') + r'\s*\)\s*\(\s*(?:' + BCAST + r'\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM), caddr, seg2)

        def iaddr(x):  # (s32)(d + 0x270): an address passed as an integer
            nonlocal cnt
            e = addr_e(int(x.group(2), 0))
            if not e: return x.group(0)
            cnt += 1; return f'({x.group(1)})' + e
        seg2 = re.sub(r'\(\s*(s32|u32|int|unsigned int|unsigned|long)\s*\)\s*\(\s*(?:' + BCAST + r'\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM), iaddr, seg2)

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
            call = prev == '(' and k >= 0 and (s_[k].isalnum() or s_[k] in '_)') and not re.search(r'\b(if|while|for|switch|return|sizeof)$', s_[:k + 1])
            if (call or prev == ',') and nxt in ',)':
                cc = call_ctx(s_, x.start() - 1)
                return (arg_cast(cc[0], cc[1], addr_type(int(x.group(1), 0))) if cc else '') + e
            return f'({et} *)' + e
        seg2 = re.sub(r'(?<![\w.>)\]])(?:' + BCAST + r'\s*)?%s\s*\+\s*%s(?![\w\[(])' % (v, NUM), baddr, seg2)

        def bare(x):  # FastVecAdd(&m->pos, &m->pos, d): the vector at offset 0
            nonlocal cnt
            e = addr_e(0)
            if not e: return x.group(0)
            cc = call_ctx(x.string, x.end(1) - 1)
            cnt += 1; return x.group(1) + (arg_cast(cc[0], cc[1], addr_type(0)) if cc else '') + e
        while True:  # one bare argument per pass: FastVecAdd(d, d, x)
            n0 = cnt
            seg2 = re.sub(r'(\b(?:%s)\s*\((?:(?:[^;()]|\([^;()]*\))*?,)?\s*)%s(?=\s*[,)])' % (BARE_OK, v), bare, seg2)
            if cnt == n0: break
        if cnt == c0: continue
        # (v->a.b)->c  ->  v->a.b->c
        seg2 = re.sub(r'\((\(\(struct %s \*\)%s\)->[\w.\[\]]+|%s->[\w.\[\]]+)\)(?=\s*(?:->|\.\w|\[))' % (S, v, v), r'\1', seg2)
        rest = re.sub(r'\(\(struct %s \*\)%s\)' % (S, v), '', seg2)
        rest = re.sub(r'(?<![\w.>=!<])%s\s*=(?!=)\s*[^;]*;' % v, '', rest)
        nraw = len(re.findall(r'(?<![\w.>])%s\b' % v, rest))
        if RETYPE and nraw and nraw * 4 <= cnt - c0 and d.group(6) == ';' and v not in MACRO_IDS:
            # --retype: keep the few raw uses as `(char *)d` and retype the local anyway
            seg2 = seg2.replace(H, '\x01')
            seg2 = re.sub(r'(?<![\w.>\x01])%s\b(?!\s*=(?!=))(?!\s*->)' % v, f'(({et} *){v})', seg2)
            seg2 = seg2.replace('\x01', H)
            nraw = 0
        if not nraw and d.group(6) == ';' and v not in MACRO_IDS:
            seg2 = seg2.replace(H, v)
            seg2 = re.sub(r'((?<![\w.>=!<])%s\s*=(?!=)\s*)([^;]*);' % v,
                          lambda m: m.group(1) + f'(struct {S} *)' + pvars_init(m.group(2)) + ';', seg2)
            nd = f'{d.group(1)}struct {S} *{v}' + (f' = (struct {S} *){pvars_init(init)}' if init is not None else '') + ';'
            body = body[:d.start()] + nd + seg2 + body[end:]
        else:
            body = body[:start] + seg2 + body[end:]
    return body, cnt


# ---------------------------------------------------------------- infer
CANON_OUT = {'float': 'f32', 'int': 's32', 'unsigned int': 'u32', 'short': 's16', 'unsigned short': 'u16',
             'unsigned char': 'u8', 'char': 's8', 'long': 's64', 'unsigned long': 'u64'}


def observe(body, obs):
    def note(off, T, kind):
        obs[int(off, 0)][(canon(T) if T else None, kind)] += 1
        return '@'

    def sub(pat, f, s):
        return re.sub(pat, f, s)
    body = sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*' + INL + r'\s*\+\s*' + NUM + r'\s*\)',
               lambda x: note(x.group(3), tnorm(x.group(1)), 'val'), body)
    body = sub(r'\(\s*\(\s*(?:const\s+)?(.*?)\s*\*\s*\)\s*' + CORE + r'\s*\)\s*\[\s*' + NUM + r'\s*\]',
               lambda x: note(x.group(2), x.group(1), 'val'), body)
    for d, v, et, init, start, end in locals_of(body):
        seg = body[start:end]
        seg = sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*(?:' + BCAST + r'\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM),
                  lambda x: note(x.group(2), tnorm(x.group(1)), 'val'), seg)
        seg = sub(r'(?<![\w.>])%s\s*\[\s*%s\s*\]' % (v, NUM), lambda x: note(x.group(1), et, 'val'), seg)
        seg = sub(CIDX % v, lambda x: note(hex(cidx_off(tnorm(x.group(1)), x.group(2)) or 0), tnorm(x.group(1)), 'val'), seg)
        seg = sub(MOBYF % v, lambda x: note(hex(moby_field(x.group(1), x.group(2))[0]), moby_field(x.group(1), x.group(2))[1], 'val')
                  if moby_field(x.group(1), x.group(2)) else x.group(0), seg)
        seg = sub(r'\(\s*' + TYP.replace('*)*)', '*)+)') + r'\s*\)\s*\(\s*(?:' + BCAST + r'\s*)?%s\s*\+\s*%s\s*\)' % (v, NUM),
                  lambda x: note(x.group(2), tnorm(x.group(1))[:-1].strip(), 'ptr'), seg)
        seg = sub(r'(?<![\w.>)\]])(?:' + BCAST + r'\s*)?%s\s*\+\s*%s(?![\w\[(])' % (v, NUM),
                  lambda x: note(x.group(1), None, 'addr'), seg)


def infer(obs, name):
    print(f'struct {name} {{')
    offs = sorted(obs); at = 0
    for i, off in enumerate(offs):
        nxt = offs[i + 1] if i + 1 < len(offs) else None
        typed = Counter()
        for (T, kind), n in obs[off].items():
            if T and T != 'void': typed[T] += n
        if typed:
            T = typed.most_common(1)[0][0]
            try: sz = tsize(T)
            except KeyError: sz = 4
            T = CANON_OUT.get(T, T)
            if T == 'f32' and nxt is not None and off % 16 == 0 and nxt >= off + 16 and \
                    any(k == 'addr' or k == 'ptr' for (_, k) in obs[off]):
                T, sz = 'Vec4', 16
        else:
            room = (nxt - off) if nxt is not None else 16
            if off % 16 == 0 and room >= 16: T, sz = 'Vec4', 16
            else: T, sz = 'u8', min(room, 16)
        if off < at:
            print(f'    /* 0x{off:X}: also {T} (overlaps the field before) */'); continue
        if off > at:
            print(f'    u8 pad_{at:02X}[0x{off - at:X}];')
        decl = f'{T} unk_{off:02X}' + (f'[0x{sz:X}]' if T == 'u8' and sz > 1 and not typed else '')
        other = ', '.join(sorted(CANON_OUT.get(t, t) for t in typed if CANON_OUT.get(t, t) != T))
        uses = sum(obs[off].values())
        cm = f'/* 0x{off:02X}' + (f' also {other}' if other else '') + f'; {uses} use{"s" if uses > 1 else ""} */'
        print(f'    {decl + ";":<36}{cm}')
        at = off + sz
    print('};')


# ---------------------------------------------------------------- main
MACRO_IDS = set()
tot = 0
if not SNAME and not INFER: sys.exit(__doc__)
for p in files:
    t = open(p).read(); res = []; n = 0
    if SNAME and not INFER: ROOTN = dump(p, SNAME)
    elif not RECS: dump_moby(p)
    read_protos(t)
    MACRO_IDS = set(re.findall(r'\w+', ' '.join(re.findall(r'^\s*#\s*define\s+\w+(?:\([^)]*\))?(.*)$', t, re.M))))
    obs = defaultdict(Counter)
    for m in DEFANY.finditer(t):
        if ONLY and m.group(1) not in ONLY: continue
        if m.group(1) in SKIP: continue
        if m.group(1) in ('if', 'while', 'for', 'switch'): continue
        start = t.index('{', m.end() - 1); end = block_end(t, start + 1)
        body = t[start:end + 1]
        if INFER:
            observe(body, obs); continue
        nb, c = convert(body, m.group(1), SNAME)
        if nb != body:
            if LIST: print('FUNC', m.group(1), c)
            res.append((start, end + 1, nb)); n += c
    if INFER:
        infer(obs, SNAME or 'DraftVars'); continue
    out = t
    for s_, e_, nb in reversed(res): out = out[:s_] + nb + out[e_:]
    if n:
        tot += n; print(f'{n}\t{p}')
        if not dry: open(p, 'w').write(out)
if not INFER: print('total', tot, file=sys.stderr)
