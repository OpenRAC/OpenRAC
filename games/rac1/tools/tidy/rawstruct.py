#!/usr/bin/env python3
"""Raw byte-offset accesses through one named local or parameter -> struct fields.

  python3 tools/tidy/rawstruct.py FILE --func=F[,G] --var=V[,W] --struct=TYPE [--union=M[,M]] [--dry] [--retype] [--list]
  python3 tools/tidy/rawstruct.py FILE --func=F[,G] --var=V --infer [--struct=NAME]

Like rawpvars.py, but the base is the variable V (a parameter or a local) of the
functions F, whatever its value. You decide what V points to; the tool only
rewrites. TYPE is a typedef (`Moby`, `Hero`, `CollisionHit`) or a tag (`struct Foo`,
or `Foo` when there is no typedef of that name). It may come from a shared header or
be defined in FILE, but it must be complete before F (else the tool stops). Its layout
is clang's record dump of the whole file (EE long sizes via clast.py).

Inside V's scope (each declaration of V separately; one variable at a time):
  *(T *)(v + OFF), *(T *)((char *)v + OFF), v[OFF], ((T *)v)[N], *(T *)v -> v->field
  (T *)(v + OFF), v + OFF, (s32)(v + OFF), (T *)v (T = the field's type) -> &v->field
  ((X *)v)->a.b (another view of the same bytes)                       -> v->field
  FastVecAdd(x, x, v) (vector callees, rawpvars BARE_OK)               -> &v->field
A read of another type gets `*(T *)&v->field` (in parentheses before ++, --, [, ., ->).
Offsets where no field starts (or only `pad*` members) stay raw, and so do reads that
need a cast and addresses past the start inside a byte array (`u8 data[0x40]`: an
unknown region; `v->data` converts). A struct copy of a non-scalar type takes the
outermost member there. Bare `v + OFF` and `v[OFF]` count as byte offsets only when V
is a byte pointer (char, u8, s8), `void *` (v + OFF) or an integer; for any other
pointer type only the `(char *)v + OFF` forms are taken.

When no raw use of V is left, V is retyped to `TYPE *` (local: `TYPE *v = (TYPE *)init;`,
assignments get the cast; parameter: only when the function's name is not used
elsewhere in FILE or declared in include/). Uses that do not care about the pointer type
do not block it: `v == 0`, `v != 0`, `!v`, `if (v)`, `&& v`, `|| v`, `v` passed to a
`void *` or `TYPE *` parameter of a prototype in FILE, `v = ...`. Else the accesses read
`((TYPE *)v)->field`. --retype retypes anyway when the raw uses are at most a quarter
of the conversions; they then read `(char *)v` (the old type). --list prints per
declaration: conversions, raw uses left, retyped.

--union=M picks the view of a union: where a union offers a member named M (at any
depth, e.g. `--union=options` for MenuScreen.data.options) only that member's fields
are used. Without it the tool takes rawpvars' choice (the last matching member), so
pass it for every struct with a union of per-user views.

--infer prints a draft struct from the offsets and types used through V (fields
`unk_<OFF>`); rename the fields, paste it after the includes, then convert.

The bytes must not change: check the file (overlays:
`bash tools/docker/run.sh python tools/overlay_file_check.py FILE`, src/game: the full
build) and leave the functions that change raw.
"""
import re, sys, subprocess, tempfile
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ARGS = sys.argv[1:]

# rawpvars.py's layout, field picking and prototype helpers (everything before its main)
_MAIN = '# ' + '-' * 64 + ' main'
_src = (HERE / 'rawpvars.py').read_text()
_src = _src[:_src.index(_MAIN)]
_argv = sys.argv; sys.argv = [sys.argv[0]]
L = {'__name__': 'rawpvars_lib', '__file__': str(HERE / 'rawpvars.py')}
exec(compile(_src, str(HERE / 'rawpvars.py'), 'exec'), L)
sys.argv = _argv
clast = L['clast']; N = L['N']; canon = L['canon']; tnorm = L['tnorm']; block_end = L['block_end']
_cands = L['cands']


def cands_pref(node, rel, path, base):
    """rawpvars' cands, keeping only the --union members when a union offers one of them."""
    cs = _cands(node, rel, path, base)
    if not UNION: return cs
    keep = [c for c in cs if not c[3] or set(re.sub(r'\[\d+\]', '', c[0]).split('.')) & UNION]
    return keep or cs
L['cands'] = cands_pref
NUM, TYP, BCAST, DEFANY = L['NUM'], L['TYP'], L['BCAST'], L['DEFANY']


def opt_list(p):
    out = []
    for a in ARGS:
        if a.startswith(p): out += [x for x in a[len(p):].split(',') if x]
    return out


dry = '--dry' in ARGS; LIST = '--list' in ARGS; INFER = '--infer' in ARGS; RETYPE = '--retype' in ARGS
FUNCS = opt_list('--func='); VARS = opt_list('--var=')
STRUCT = (opt_list('--struct=') or [None])[0]
UNION = set(opt_list('--union='))
files = [a for a in ARGS if not a.startswith('--')]
if not files or not FUNCS or not VARS or (not STRUCT and not INFER): sys.exit(__doc__)

BYTE_T = {'char', 'unsigned char', 'signed char', 'u8', 's8'}
INT_T = {'int', 's32', 'u32', 'unsigned int', 'unsigned', 'signed int', 'long', 'unsigned long', 'uintptr_t'}
KW = {'return', 'else', 'case', 'goto', 'break', 'continue', 'do', 'sizeof'}


def clang(path, text, extra=()):
    with tempfile.TemporaryDirectory() as d:
        c = Path(d) / 'v.c'; c.write_text(clast.ee(text)); clast.prep_includes()
        return subprocess.run(['clang', '--target=mips64el-linux-gnuabin32', '-std=gnu89', '-fsyntax-only', '-w',
                               '-ferror-limit=0', '-I', str(clast.INC_T), '-include', str(clast.PRELUDE),
                               '-I', str(Path(path).resolve().parent), *extra, str(c)], capture_output=True, text=True)


# ---------------------------------------------------------------- layout
def parse_dump(out):
    RECS, SIZES = L['RECS'], L['SIZES']
    for blk in clast.unee(out).split('*** Dumping AST Record Layout')[1:]:
        lines = [l for l in blk.splitlines() if '|' in l]
        sz = re.search(r'sizeof=(\d+)', blk)
        stack = []
        for l in lines:
            m = re.match(r'\s*(\d+)(?::\S+)? \|( *)(.*)$', l)
            if not m: continue
            lvl = (len(m.group(2)) - 1) // 2; body = m.group(3)
            if lvl == 0:
                stack = [N(0, '', body)]; continue
            tm = re.match(r'(.*?)\s*(\w+)$', body)
            if not tm: continue
            n = N(int(m.group(1)), tm.group(2), tm.group(1))
            del stack[lvl:]; stack[-1].kids.append(n); stack.append(n)
        if stack:
            nm = stack[0].typ
            RECS[nm] = stack[0]; SIZES[nm] = int(sz.group(1)) if sz else None

            def reg(n):
                for k in n.kids:
                    if k.kids and k.typ not in RECS:
                        RECS[k.typ] = k; SIZES[k.typ] = None
                    reg(k)
            reg(stack[0])


def dump(path, stype):
    """Root node of stype (its fields at offsets from 0); fills rawpvars' RECS/SIZES."""
    t = Path(path).read_text()
    casts = sorted(set(re.findall(L['CASTT'], t)))
    probes = ''.join(f'struct __rawpvars_p_{re.sub(r"\W", "_", x)} {{ {x} a; }};\n'
                     for x in casts if not x.startswith('struct '))
    kw = {'char', 'short', 'int', 'long', 'float', 'double', 'void', 'unsigned', 'signed', 'const', 'volatile',
          'struct', 'union'}
    named = sorted(set(x for x in re.findall(r'\(\s*([A-Za-z_]\w*)\s*\*+\s*\)', t) if x not in kw) - set(casts))
    probes += ''.join(f'struct __rawstruct_t_{x} {{ {x} a; }};\n' for x in named)  # sizes of cast types
    if stype: probes += f'struct __rawstruct_p {{ {stype} a; }};\n'
    r = clang(path, t + '\n' + probes, ['-Xclang', '-fdump-record-layouts-complete', '-Xclang', '-fdump-record-layouts'])
    parse_dump(r.stdout)
    for k in list(L['RECS']):
        m = re.fullmatch(r'struct __rawstruct_t_(\w+)', k)
        if m and L['SIZES'].get(k) and m.group(1) not in L['SIZES']:
            L['SIZES'][m.group(1)] = L['SIZES'][k]
            a = L['RECS'][k].kids[0] if L['RECS'][k].kids else None
            if a is not None and a.kids and m.group(1) not in L['RECS']: L['RECS'][m.group(1)] = a
    if not stype: return None
    p = L['RECS'].get('struct __rawstruct_p')
    a = p.kids[0] if p and p.kids else None
    if a is None or not a.kids:
        sys.exit(f'no layout for {stype}: not a complete struct in {path}')
    return a


def visible_before(path, text, stype, pos, fname):
    """Exit unless stype is a complete type at pos of text (the start of function fname)."""
    t = text[:pos] + f'\nint __rawstruct_v[sizeof({stype})];\n'
    r = clang(path, t)
    ln = t.count('\n')
    if any(re.search(r'v\.c:%d:\d+: error' % ln, l) for l in r.stderr.splitlines()):
        sys.exit(f'{stype} is not a complete type before {fname}: include its header or define it earlier')


def spell(path, name):
    """`X` when X is a typedef (or written `struct X`), else `struct X`."""
    if name.startswith('struct '): return name
    t = Path(path).read_text()
    inc = ' '.join(p.read_text(errors='replace') for p in (ROOT / 'include').glob('*.h'))
    if re.search(r'\}\s*%s\s*;|typedef\s+[^;]*\b%s\s*;' % (name, name), t + inc): return name
    return f'struct {name}'


# ---------------------------------------------------------------- source
def func_span(t, name):
    for m in DEFANY.finditer(t):
        if m.group(1) != name: continue
        start = t.index('{', m.end() - 1); end = block_end(t, start + 1)
        return m, start, end + 1
    return None


def split_params(s):
    ps, d, cur, pos = [], 0, '', 0
    for i, ch in enumerate(s):
        if ch == ',' and d == 0: ps.append((cur, pos)); cur = ''; pos = i + 1; continue
        d += ch == '('; d -= ch == ')'; cur += ch
    ps.append((cur, pos))
    return ps


def base_kind(typ, ptr):
    typ = re.sub(r'\b(const|volatile|register)\b', '', typ); typ = re.sub(r'\s+', ' ', typ).strip()
    if ptr == 1 and typ in BYTE_T: return 'byte', typ + ' *'
    if ptr == 1 and typ == 'void': return 'void', 'void *'
    if ptr == 0 and typ in INT_T: return 'int', typ
    if ptr >= 1: return 'other', typ + ' ' + '*' * ptr
    return None, typ


class Decl:
    """A declaration `T *v [= init]` up to its `;` or top-level `,`: groups 1-3 of the head,
    4 the init (or None), 5 the terminator."""
    def __init__(s, body, m):
        s.m = m; s.ok = False; j = m.end(); init = None
        if body[j] == '=':
            d = 0; k = j + 1
            while k < len(body):
                ch = body[k]
                if ch in '([{': d += 1
                elif ch in ')]}':
                    d -= 1
                    if d < 0: return
                elif ch in ';,' and d == 0: break
                k += 1
            if k >= len(body): return
            init = body[j + 1:k].strip(); j = k
        s.g = [None, m.group(1), m.group(2), m.group(3), init, body[j]]
        s._end = j + 1; s.ok = True

    def group(s, i): return s.g[i]
    def start(s): return s.m.start()
    def end(s): return s._end


def find_var(t, fm, start, end, v):
    """[(kind, old type, scope start, scope end, decl info)] for v in the function (one per declaration)."""
    params = fm.group(2); pstart = fm.start(2)
    for ptxt, off in split_params(params):
        pm = re.fullmatch(r'\s*((?:const\s+|volatile\s+|register\s+|unsigned\s+|signed\s+|struct\s+)*\w+)\s*((?:\*\s*)*)%s\s*' % v, ptxt)
        if pm:
            k, old = base_kind(pm.group(1), pm.group(2).count('*'))
            return [(k, old, start, end, ('param', pstart + off, pstart + off + len(ptxt)))]
    body = COMMENT.sub(lambda x: ' ' * len(x.group(0)), t[start:end])  # same length: positions stay
    DECLV = re.compile(r'(?:(?<=[;{}])|^)(\s*)((?:const\s+|volatile\s+|register\s+|unsigned\s+|signed\s+|struct\s+)*\w+)'
                       r'((?:\s+|\s*(?:\*\s*)+))%s\s*(?=[=;,])' % v, re.M)
    ds = [Decl(body, d) for d in DECLV.finditer(body) if d.group(2) not in KW]
    ds = [d for d in ds if d.ok]
    if not ds: return [(None, f'no declaration of {v}', 0, 0, None)]
    out = []
    for d in ds:
        k, old = base_kind(d.group(2), d.group(3).count('*'))
        s = start + d.end(); e = start + block_end(body, d.end())
        if any(s <= start + o.start() < e for o in ds if o is not d):
            out.append((None, f'{v} declared again inside its scope', 0, 0, None)); continue
        out.append((k, old, s, e, ('local', start + d.start(), start + d.end(), d)))
    return out


def proto_ok(name, idx, stype):
    """True when argument idx of callee name is a `void *` or `stype *` (any type is fine)."""
    ps = L['PROTOS'].get(name)
    if not ps or idx >= len(ps): return False
    P = re.sub(r'(\*)\s*\w+\s*(\[\s*\])?$', r'\1', ps[idx]).strip()
    P = re.sub(r'^(const|register)\s+', '', P)
    return canon(P) in ('void *', canon(stype + ' *'))


def neutral_uses(seg, v, stype, mk='#'):
    """seg with the uses of v that do not depend on its pointer type replaced by mk."""
    s = seg
    s = re.sub(r'(?<![\w.>])%s(?=\s*(?:==|!=)\s*(?:0|NULL)\b)' % v, mk, s)
    s = re.sub(r'(\b(?:0|NULL)\s*(?:==|!=)\s*)%s\b(?!\s*(?:->|\[|\.))' % v, lambda x: x.group(1) + mk, s)
    s = re.sub(r'(!\s*)%s\b(?!\s*(?:->|\[|\.|\(|\+|-|=))' % v, lambda x: x.group(1) + mk, s)
    s = re.sub(r'(\b(?:if|while)\s*\(\s*|&&\s*|\|\|\s*)%s(?=\s*(?:\)|&&|\|\||;))' % v, lambda x: x.group(1) + mk, s)
    s = re.sub(r'(\(\s*)%s(?=\s*(?:&&|\|\|))' % v, lambda x: x.group(1) + mk, s)

    def arg(x):
        cc = L['call_ctx'](x.string, x.end(1) - 1)
        return x.group(1) + mk if cc and proto_ok(cc[0], cc[1], stype) else x.group(0)
    s = re.sub(r'([(,]\s*)(%s)(?=\s*[,)])' % v, arg, s)
    return s


def convert(seg, v, kind, old, H, stype):
    """Rewrite one scope; returns (new seg, count)."""
    cnt = 0
    pick_addr, cands, plain_store = L['pick_addr'], L['cands'], L['plain_store']
    deref, arg_cast, addr_type, call_ctx = L['deref'], L['arg_cast'], L['addr_type'], L['call_ctx']
    ROOTN = L['ROOTN']
    et = old[:-1].strip() if kind in ('byte', 'void') else 'char'
    bare = kind in ('byte', 'void', 'int')
    BASE = (r'(?:' + BCAST + r'\s*)?' if bare else BCAST + r'\s*') + v

    def pick_value(off, T):
        r = L['pick_value'](off, T)
        if r and r[1] and not T.endswith('*') and canon(T) not in L['SCALAR'] and canon(T) not in L['CANON_OUT']:
            r = (cands(ROOTN, off, '', 0)[0][0], True)  # a struct copy: the outermost member there
        if r and r[1]:  # a cast read inside a byte array (an unknown region) stays raw
            ft = L['field_type'](r[0])
            if re.search(r'\[\d+\]$', r[0]) and canon(ft) in ('char', 'unsigned char'): return None
            if re.match(r'(?:u8|s8|unsigned char|signed char|char)\s*\[', ft): return None
        return r

    def post(x, e):  # (*(T *)&v->f)++: a postfix operator binds tighter than the cast
        return f'({e})' if re.match(r'\s*(\+\+|--|\[|\.|->)', x.string[x.end():]) else e

    def val(x):
        nonlocal cnt
        T = tnorm(x.group(1)); r = pick_value(int(x.group(2), 0), T)
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(T, e)) if r[1] and not plain_store(x, T, r[0]) else e
    seg = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)', val, seg)

    def valp(x):  # *((T *)(v + OFF))
        nonlocal cnt
        T = tnorm(x.group(1)); r = pick_value(int(x.group(2), 0), T)
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(T, e)) if r[1] and not plain_store(x, T, r[0]) else e
    seg = re.sub(r'\*\s*\(\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)\s*\)', valp, seg)

    def val0(x):  # *(T *)v: the field at offset 0
        nonlocal cnt
        T = tnorm(x.group(1)); r = pick_value(0, T)
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(T, e)) if r[1] and not plain_store(x, T, r[0]) else e
    seg = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*(?:\(\s*%s\s*\)|%s)(?![\w\[(.]|\s*(?:->|\[|\(|\.|\+\+|--))' % (v, v), val0, seg)

    if kind == 'byte':
        def idx(x):
            nonlocal cnt
            r = pick_value(int(x.group(1), 0), et)
            if not r: return x.group(0)
            cnt += 1; e = f'{H}->{r[0]}'
            return post(x, f'*({et} *)&{e}') if r[1] and not plain_store(x, et, r[0]) else e
        seg = re.sub(r'(?<![\w.>])%s\s*\[\s*%s\s*\]' % (v, NUM), idx, seg)

    def cidx(x):  # ((T *)v)[N]
        nonlocal cnt
        T = tnorm(x.group(1)); off = L['cidx_off'](T, x.group(2))
        r = pick_value(off, T) if off is not None else None
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(T, e)) if r[1] and not plain_store(x, T, r[0]) else e
    seg = re.sub(L['CIDX'] % v, cidx, seg)

    def same_t(X):
        X = re.sub(r'\s+', ' ', X.strip())
        return canon(X) == canon(stype) or canon('struct ' + X) == canon(stype)

    def mfield(x):  # ((Moby *)v)->unk80: another view of the same bytes
        nonlocal cnt
        if same_t(x.group(1)): return x.group(0)
        mf = L['moby_field'](x.group(1), x.group(2))
        r = pick_value(mf[0], mf[1]) if mf else None
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(mf[1], e)) if r[1] else e

    def mfield_t(x):  # *(int *)&((Moby *)v)->collision
        nonlocal cnt
        if same_t(x.group(2)): return x.group(0)
        mf = L['moby_field'](x.group(2), x.group(3)); T = tnorm(x.group(1))
        r = pick_value(mf[0], T) if mf else None
        if not r: return x.group(0)
        cnt += 1; e = f'{H}->{r[0]}'
        return post(x, deref(T, e)) if r[1] else e
    seg = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*&\s*' + L['MOBYF'] % v, mfield_t, seg)
    seg = re.sub(L['MOBYF'] % v, mfield, seg)

    def addr_e(off, T=None):
        r = pick_addr(off, T)
        if not r: return None
        a = re.match(r'&(.*)\[(\d+)\]$', r)
        if a and a.group(2) != '0' and canon(L['field_type'](r[1:])) in ('char', 'unsigned char'):
            return None  # inside a byte array (an unknown region): stays raw
        return '&' + H + '->' + r[1:] if r.startswith('&') else H + '->' + r

    def deref_of(x):  # *(T *)(v + OFF) that val left raw: keep it raw
        s_ = x.string; i = x.start() - 1
        while i >= 0 and s_[i] in ' \t\n': i -= 1
        return i >= 0 and s_[i] == '*' and not (i > 0 and s_[i - 1] in '*/')

    def caddr(x):
        nonlocal cnt
        if x.string[x.end():x.end() + 1] == '[' or deref_of(x): return x.group(0)
        T = tnorm(x.group(1)); pt = T[:-1].strip(); off = int(x.group(2), 0)
        e = addr_e(off, pt)
        if not e: return x.group(0)
        cnt += 1
        r = pick_addr(off, pt)
        cs = [c for c in cands(ROOTN, off, '', 0) if ('&' + c[0] == r or c[0] == r)]
        if cs and r.startswith('&') and canon(cs[0][1]) == canon(pt): return e
        return f'({T})' + e
    seg = re.sub(r'\(\s*' + TYP.replace('*)*)', '*)+)') + r'\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)', caddr, seg)

    def caddr0(x):  # (int *)v: the address of the field at offset 0 when it has that type
        nonlocal cnt
        if deref_of(x): return x.group(0)
        T = tnorm(x.group(1)); pt = T[:-1].strip()
        if canon(pt) in ('char', 'unsigned char', 'void'): return x.group(0)
        ex = [c for c in cands(ROOTN, 0, '', 0) if canon(c[1]) == canon(pt)]
        if not ex: return x.group(0)  # (Other *)v: a view, not a field address
        cnt += 1
        return f'&{H}->{ex[-1][0]}'
    seg = seg.replace(H, '\x01')
    seg = re.sub(r'\(\s*' + TYP.replace('*)*)', '*)+)') + r'\s*\)\s*%s(?![\w\[(.]|\s*(?:->|\[|\(|\.|\+|-(?!-)|\+\+|--))' % v, caddr0, seg)
    seg = seg.replace('\x01', H)

    def iaddr(x):  # (s32)(v + 0x270): an address as an integer
        nonlocal cnt
        e = addr_e(int(x.group(2), 0))
        if not e: return x.group(0)
        cnt += 1; return f'({x.group(1)})' + e
    seg = re.sub(r'\(\s*(s32|u32|int|unsigned int|unsigned|long)\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)', iaddr, seg)

    if kind != 'int':
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
            if re.search(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*$', s_[:x.start()]): return x.group(0)  # a deref val left raw
            e = addr_e(int(x.group(1), 0))
            if not e: return x.group(0)
            cnt += 1
            k = i - 1
            while k >= 0 and s_[k] in ' \t': k -= 1
            call = prev == '(' and k >= 0 and (s_[k].isalnum() or s_[k] in '_)') and \
                not re.search(r'\b(if|while|for|switch|return|sizeof)$', s_[:k + 1])
            if (call or prev == ',') and nxt in ',)':
                cc = call_ctx(s_, x.start() - 1)
                return (arg_cast(cc[0], cc[1], addr_type(int(x.group(1), 0))) if cc else '') + e
            return f'({et} *)' + e
        seg = re.sub(r'(?<![\w.>)\]])' + BASE + r'\s*\+\s*' + NUM + r'(?![\w\[(])', baddr, seg)

    if bare:
        def barg(x):  # FastVecAdd(&m->pos, &m->pos, v): the vector at offset 0
            nonlocal cnt
            e = addr_e(0)
            if not e: return x.group(0)
            cc = call_ctx(x.string, x.end(1) - 1)
            cnt += 1; return x.group(1) + (arg_cast(cc[0], cc[1], addr_type(0)) if cc else '') + e
        while True:
            n0 = cnt
            seg = re.sub(r'(\b(?:%s)\s*\((?:(?:[^;()]|\([^;()]*\))*?,)?\s*)%s(?=\s*[,)])' % (L['BARE_OK'], v), barg, seg)
            if cnt == n0: break
    Hr = re.escape(H)
    seg = re.sub(r'\((%s->[\w.\[\]]+)\)(?=\s*(?:->|\.\w|\[))' % Hr, r'\1', seg)
    seg = re.sub(r'\*\s*\(\s*&\s*(%s->[\w.\[\]]+)\s*\)' % Hr, r'\1', seg)  # *(&v->f) -> v->f
    return seg, cnt


COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)


def mask(seg):
    """Comments out of the way of the rewrites: (masked text, restore function)."""
    saved = []

    def m(x):
        saved.append(x.group(0)); return f'\x05{len(saved) - 1}\x05'
    return COMMENT.sub(m, seg), lambda s: re.sub(r'\x05(\d+)\x05', lambda x: saved[int(x.group(1))], s)


def raw_left(seg, v, H, stype):
    rest = seg.replace(H, '')
    rest = re.sub(r'(?<![\w.>=!<])%s\s*=(?!=)\s*[^;]*;' % v, '', rest)
    rest = neutral_uses(rest, v, stype)
    return len(re.findall(r'(?<![\w.>])%s\b' % v, rest))


def castv(stype, e):
    e = e.strip()
    if re.match(r'\(\s*%s\s*\*\s*\)' % re.escape(stype), e): return e
    simple = re.fullmatch(r'[\w.>\-\[\]]+', e) or re.fullmatch(r'\w+\s*\(.*\)', e) and L['_bal'](re.sub(r'^\w+\s*', '', e)[1:-1]) \
        or e.startswith('(') and e.endswith(')') and L['_bal'](e[1:-1])
    if not simple: e = f'({e})'
    return f'({stype} *){e}'


def retype_raw(seg, v, H, old, stype):
    """--retype: the raw uses left read `((old)v)`; neutral uses stay `v`."""
    s = seg.replace(H, '\x01')
    s = re.sub(r'(?<![\w.>=!<])%s(?=\s*=(?!=))' % v, '\x02', s)
    s = neutral_uses(s, v, stype, '\x02')
    s = re.sub(r'(\(\s*(?:const\s+|unsigned\s+|signed\s+|struct\s+)*\w+\s*\**\s*\)\s*)%s\b(?!\s*(?:->|\[))' % v,
               lambda x: x.group(1) + '\x02', s)  # (int)v: already cast
    s = re.sub(r'(?<![\w.>])%s(?=\s*[-+](?![->=+]))' % v, '\x03', s)  # *(T *)(v + 4) -> *(T *)((char *)v + 4)
    s = re.sub(r'((?:[(,=]|\breturn)\s*)%s(?=\s*[,);])' % v, lambda x: x.group(1) + '\x03', s)  # f(v), return v
    s = re.sub(r'(?<![\w.>])%s\b(?!\s*->)' % v, f'(({old}){v})', s)
    return s.replace('\x03', f'({old}){v}').replace('\x02', v).replace('\x01', H)


def declared_elsewhere(t, fname, fstart):
    rest = t[:fstart] + t[func_span(t, fname)[2]:]
    rest = re.sub(r'/\*.*?\*/|//[^\n]*', ' ', rest, flags=re.S)
    if re.search(r'\b%s\b' % fname, rest): return True
    for p in (ROOT / 'include').glob('*.h'):
        if re.search(r'\b%s\s*\(' % fname, p.read_text(errors='replace')): return True
    return False


# ---------------------------------------------------------------- infer
def observe(seg, v, kind, old, obs):
    def note(off, T, k):
        obs[int(off, 0) if isinstance(off, str) else off][(canon(T) if T else None, k)] += 1
        return '@'
    bare = kind in ('byte', 'void', 'int')
    BASE = (r'(?:' + BCAST + r'\s*)?' if bare else BCAST + r'\s*') + v
    seg = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)',
                 lambda x: note(x.group(2), tnorm(x.group(1)), 'val'), seg)
    seg = re.sub(r'\*\s*\(\s*' + TYP + r'\s*\*\s*\)\s*%s(?![\w\[(.]|\s*(?:->|\[|\(|\.|\+\+|--))' % v,
                 lambda x: note(0, tnorm(x.group(1)), 'val'), seg)
    if kind == 'byte':
        seg = re.sub(r'(?<![\w.>])%s\s*\[\s*%s\s*\]' % (v, NUM), lambda x: note(x.group(1), old[:-1].strip(), 'val'), seg)
    seg = re.sub(L['CIDX'] % v, lambda x: note(L['cidx_off'](tnorm(x.group(1)), x.group(2)) or 0, tnorm(x.group(1)), 'val'), seg)
    seg = re.sub(r'\(\s*' + TYP.replace('*)*)', '*)+)') + r'\s*\)\s*\(\s*' + BASE + r'\s*\+\s*' + NUM + r'\s*\)',
                 lambda x: note(x.group(2), tnorm(x.group(1))[:-1].strip(), 'ptr'), seg)
    if kind != 'int':
        seg = re.sub(r'(?<![\w.>)\]])' + BASE + r'\s*\+\s*' + NUM + r'(?![\w\[(])', lambda x: note(x.group(1), None, 'addr'), seg)


# ---------------------------------------------------------------- run
def splice(t, res, stype):
    """Apply one variable's rewrites (descending, they do not overlap) and its retypes."""
    for s, e, seg2, decl, v in sorted(res, key=lambda r: -r[0]):
        t = t[:s] + seg2 + t[e:]
        if decl is None: continue
        if decl[0] == 'param':
            ptxt = t[decl[1]:decl[2]]
            lead = re.match(r'\s*', ptxt).group(0)
            t = t[:decl[1]] + f'{lead}{stype} *{v}' + t[decl[2]:]
        else:
            d = decl[3]; init = d.group(4)
            nd = f'{d.group(1)}{stype} *{v}' + (f' = {castv(stype, init)}' if init is not None else '') + ';'
            t = t[:decl[1]] + nd + t[decl[2]:]
    return t


MOVES = r'(\+\+|--)\s*%s\b(?!\s*(?:->|\[|\.))|(?<![.>])\b%s\s*(\+\+|--|[-+|&^*/]=)|(?:(?<=[=(,;{}!?:])|(?<=return))\s*&\s*%s\b(?!\s*(?:->|\[|\.))'
SHADOW = r'(?:^|[;{}(])\s*(?:const\s+|unsigned\s+|signed\s+|struct\s+)*\w+(?:\s+|\s*(?:\*\s*)+)%s\s*[=;,\[)]'
tot = 0
for p in files:
    t = open(p).read()
    STYPE = spell(p, STRUCT) if STRUCT else None
    L['ROOTN'] = dump(p, STYPE) if not INFER else dump(p, None)
    L['SNAME'] = (STYPE or '').replace('struct ', '')
    L['read_protos'](t)
    obs = defaultdict(Counter); n = 0
    for fn in FUNCS:
        if not func_span(t, fn): print(f'{fn}: not found', file=sys.stderr); continue
        if STYPE and not INFER: visible_before(p, t, STYPE, func_span(t, fn)[0].start(), fn)
        for v in VARS:  # one variable at a time: their scopes may overlap
            fm, fstart, fend = func_span(t, fn); res = []
            for kind, old, s, e, decl in find_var(t, fm, fstart, fend, v):
                if decl is None or kind is None:
                    if len(VARS) == 1 or decl is not None: print(f'{fn}/{v}: skipped ({old})', file=sys.stderr)
                    continue
                seg, unmask = mask(t[s:e])
                if re.search(MOVES % (v, v, v), seg):
                    print(f'{fn}/{v}: skipped (v moves, or &v is taken)', file=sys.stderr); continue
                if decl[0] == 'param' and re.search(SHADOW % v, seg, re.M):
                    print(f'{fn}/{v}: skipped (shadowed by a local)', file=sys.stderr); continue
                if INFER:
                    observe(seg, v, kind, old, obs); continue
                H = f'(({STYPE} *){v})'
                seg2, c = convert(seg, v, kind, old, H, STYPE)
                if not c: continue
                nraw = raw_left(seg2, v, H, STYPE)
                retyped = ''
                if kind == 'other' and canon(old) == canon(STYPE + ' *'):
                    seg2 = seg2.replace(H, v)
                else:
                    can = decl[0] == 'local' and decl[3].group(5) == ';' or \
                        decl[0] == 'param' and not declared_elsewhere(t, fn, fm.start())
                    if RETYPE and can and nraw and nraw * 4 <= c and kind != 'int':
                        seg2 = retype_raw(seg2, v, H, old, STYPE); nraw = 0
                    if can and not nraw:
                        seg2 = seg2.replace(H, v)
                        seg2 = re.sub(r'((?<![\w.>=!<])%s\s*=(?!=)\s*)([^;]*);' % v,
                                      lambda m: m.group(1) + castv(STYPE, m.group(2)) + ';', seg2)
                        retyped = ' retyped'
                if LIST: print(f'FUNC {fn} {v}: {c} converted, {nraw} raw left{retyped}')
                res.append((s, e, unmask(seg2), decl if retyped else None, v))
                n += c
            t = splice(t, res, STYPE)
    if INFER:
        L['infer'](obs, STRUCT or 'DraftStruct'); continue
    if n:
        tot += n; print(f'{n}\t{p}')
        if not dry: open(p, 'w').write(t)
if not INFER: print('total', tot, file=sys.stderr)
