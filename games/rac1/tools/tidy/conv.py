#!/usr/bin/env python3
"""Replace a level file's private copies of a shared struct by the shared one.

  conv.py FILE [--dry] [--only NAME,...] [--skip NAME,...] [--cast-arrays] [--cast-stores]
          [--target moby|hero]

Every access through a private record becomes an access to the shared field at
the same offset. When the types agree it is the shared field itself; when they
differ, the access keeps its old type through a cast on the shared field's address.
"""
import sys, re, argparse, json
sys.setrecursionlimit(1000000)
import clast, layout
from pathlib import Path

TARGETS = {
    'moby': {'header': 'moby.h',
             'recs': {'Moby': 'struct Moby', 'MobyClass': 'struct MobyClass'},
             'clone': 'Vec4f', 'clone_name': r'V\w*'},
    'hero': {'header': 'hero.h',
             'recs': {n: 'struct ' + n for n in ('Hero', 'HeroState', 'HeroMotion', 'HeroItemSlot', 'HeroRandTimer',
                                                 'HeroHealth', 'HeroEase', 'HeroVelocityKey', 'MobyTrail', 'HeroManipulator')},
             'alias': {'Hero': r'(Hero|\w*MovementPlayer|HopPlayer|HeroPos|Hero52|Hero6E0)(_[0-9A-Fa-f_]+)?'},
             'clone': 'Vec4', 'clone_name': r'Vec4(_[0-9A-Fa-f]+)?'},
    'pad': {'header': 'pad.h', 'recs': {'PadState': 'struct PadState'},
            'alias': {'PadState': r'(Pad|PadState)(_[0-9A-Fa-f_]+)?'}, 'clone': None, 'clone_name': r'(?!)'},
    'camera': {'header': 'camera.h', 'recs': {'CameraSystem': 'struct CameraSystem', 'Camera': 'struct Camera'},
               'clone': None, 'clone_name': r'(?!)'},
    'vecf': {'header': 'vector.h', 'recs': {},
             'clone': 'Vec4f', 'clone_name': r'(?!.*(Quat|Col|Rgb|Mat))(V\w*|\w*Vec\w*)'},
    'vec': {'header': 'vector.h', 'recs': {},
            'clone': 'Vec4', 'clone_name': r'(?!.*(Quat|Col|Rgb|Mat))(V\w*|\w*Vec\w*)'},
}

PRIV_RE = {}


def canon_layouts(tname):
    t = TARGETS[tname]
    text = '#include "common.h"\n#include "%s"\n' % t['header']
    ast, errs, f = clast.clang_json(text, 'canon_%s.c' % tname)
    clast.annotate(ast, f)
    T = layout.Types(ast, f)
    out = {}
    for short, cname in t['recs'].items():
        rid = T.tag.get(cname.split()[1]) if cname.startswith('struct ') else T.td_rec[cname]
        lays, e = layout.probe_layouts(text, T, {rid: cname})
        flat = layout.flatten(T, lays, rid, cname)
        out[short] = {'flat': flat, 'cname': cname}
    probe = text + '\n'.join(f'char __csz_{k}[sizeof({v["cname"]})];' for k, v in out.items()) + '\n'
    pa, _, _ = clast.clang_json(probe, 'canon_sz_%s.c' % tname)
    for n in pa.get('inner', []):
        if n.get('kind') == 'VarDecl' and n.get('name', '').startswith('__csz_'):
            out[n['name'][6:]]['size'] = int(re.match(r'char\s*\[(\d+)\]', n['type']['qualType']).group(1))
    # clones the header defines (Vec4f): name -> field names
    return out, T


def is_vec4_clone(T, rid, src):
    """A 16-byte quadword union (or struct) with members among q, f[4], i[4]."""
    rec = T.rec[rid]
    fs = [f for f in rec.get('inner', []) if f.get('kind') == 'FieldDecl']
    want = {'q': ('(U128)', 'U128', '(S128)', 'S128'), 'f': ('float[4]',), 'i': ('int[4]',)}
    if not fs or any(f.get('name') not in want or T.norm(f['type']['qualType']) not in want[f['name']] for f in fs):
        return False
    if rec.get('tagUsed') == 'struct' and len(fs) != 1:
        return False
    if not any(f['name'] == 'q' for f in fs):
        return False
    span = src[rec['_b']:rec['_e'] + 80]
    end = span.find(';', span.find('}'))
    attrs = re.findall(r'__attribute__\s*\(\((.*?)\)\)', span[:end])
    return all(re.fullmatch(r'aligned\s*\(\s*16\s*\)', a.strip()) for a in attrs)


def is_vec_clone(T, rid, src):
    rec = T.rec[rid]
    fs = [f for f in rec.get('inner', []) if f.get('kind') == 'FieldDecl']
    if rec.get('tagUsed') != 'struct' or [f.get('name') for f in fs] != ['x', 'y', 'z', 'w']:
        return False
    if any(T.norm(f['type']['qualType']) != 'float' for f in fs):
        return False
    span = src[rec['_b']:rec['_e'] + 80]
    end = span.find(';', span.find('}'))
    if 'attribute' in span[:end] or 'aligned' in span[:end]:
        return False
    return True


def strip_casts(n):
    while n.get('kind') in ('ImplicitCastExpr', 'ParenExpr') and n.get('inner'):
        n = n['inner'][0]
    return n


def const_int(n):
    n = strip_casts(n)
    if n.get('kind') == 'IntegerLiteral':
        return int(n['value'])
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('--dry', action='store_true')
    ap.add_argument('--only', default='')
    ap.add_argument('--skip', default='')
    ap.add_argument('--cast-arrays', action='store_true', help='array element -> struct member through a cast')
    ap.add_argument('--cast-stores', action='store_true', help='plain stores of another int type through a cast')
    ap.add_argument('--cast-fns', default='', help='functions where every type change goes through a cast')
    ap.add_argument('--target', default='moby')
    ap.add_argument('--as', dest='as_', default='', help='extra private names: Priv=Shared,...')
    ap.add_argument('--no-vec', action='store_true', help='leave vector clones alone')
    a = ap.parse_args()
    clast.prep_includes()
    canon, CT = canon_layouts(a.target)
    tgt = TARGETS[a.target]
    path = Path(a.file)
    src = path.read_text()
    ast, errs0, fname = clast.clang_json(src, 'src.c')
    clast.annotate(ast, fname)
    T = layout.Types(ast, fname)
    only = set(filter(None, a.only.split(',')))
    skip = set(filter(None, a.skip.split(',')))
    cast_fns = set(filter(None, a.cast_fns.split(',')))

    # private records
    priv = {}      # rid -> (cname, short target, display name)
    clones = {}    # rid -> name
    for rid, rec in T.rec.items():
        if not rec.get('_main'):
            continue
        nm = T.rec_name(rid)
        if nm is None:
            continue
        bare = nm.split()[-1]
        extra = dict(x.split('=') for x in filter(None, a.as_.split(',')))
        if bare in extra and bare not in skip:
            priv[rid] = (nm, extra[bare], bare)
            continue
        for short in tgt['recs']:
            pat = tgt.get('alias', {}).get(short, short + r'(_[0-9A-Fa-f_]+)?')
            if re.fullmatch(pat, bare) and (not only or bare in only) and bare not in skip:
                priv[rid] = (nm, short, bare)
        if not a.no_vec and rid not in priv and re.fullmatch(tgt['clone_name'], bare) and bare not in skip and \
                (is_vec_clone if tgt['clone'] == 'Vec4f' else is_vec4_clone)(T, rid, src):
            clones[rid] = bare
    # a name the header defines, kept by the file for another type, would clash
    hdr_names = set(tgt['recs']) | ({tgt['clone']} if tgt['clone'] else set()) | ({'VecQuad', 'Vec4', 'Vec4f'} if a.target in ('hero', 'vec', 'vecf') else set())
    ok = set(priv) | set(clones)
    for name in hdr_names:
        bad_rec = [rid for rid, rec in T.rec.items() if rec.get('_main') and rec.get('name') == name and rid not in ok]
        tdn = T.td.get(name)
        bad_td = tdn is not None and tdn.get('_main') and T.td_rec.get(name) not in ok
        if bad_rec or bad_td:
            print(f'CLASH: the file keeps its own {name}; not converted')
            return 5
    rename = {}
    for rid, (nm, short, bare) in priv.items():
        rename[bare] = short
    for rid, bare in clones.items():
        rename[bare] = tgt['clone']
    # typedef names that point at private records
    for name, rid in T.td_rec.items():
        if rid in priv:
            rename[name] = priv[rid][1]
        elif rid in clones:
            rename[name] = tgt['clone']

    lays, perr = layout.probe_layouts(src, T, {rid: v[0] for rid, v in priv.items()})
    pflat = {rid: layout.flatten(T, lays, rid, v[0], rename=rename) for rid, v in priv.items()}
    report = []
    bad = set()
    for rid, (nm, short, bare) in priv.items():
        size = max([x['off'] + x['size'] for x in pflat[rid]] or [0])
        if size > canon[short]['size']:
            report.append(f'SKIP {bare}: {size:#x} bytes, more than {short}')
            bad.add(rid)
        if any(x['size'] is None for x in pflat[rid]):
            report.append(f'SKIP {bare}: a field without a size')
            bad.add(rid)

    # fields of private records -> (rid, path)
    fid = {}
    for rid in priv:
        for x in pflat[rid]:
            if '.' not in x['path'] and '[' not in x['path']:
                fid[x['node']['id']] = (rid, x['path'])

    # uses of a private record by value or in pointer arithmetic (sizeof matters then)
    def refers_priv(t, form):
        """Private records a type is (form 'val': the record or an array of it) or points at once ('ptr')."""
        t2 = T.norm(t)
        out = []
        for rid, v in priv.items():
            for nm in {v[2], v[0].split()[-1]}:
                pat = r'struct %s' % re.escape(nm)
                if form == 'val' and re.fullmatch(pat + r'(\[\d*\])*', t2) or \
                        form == 'ptr' and re.fullmatch(pat + r'\*', t2):
                    out.append(rid)
        return out

    for n in clast.walk(ast):
        if not n.get('_main'):
            continue
        k = n.get('kind')
        ty = n.get('type', {}).get('qualType', '') if isinstance(n.get('type'), dict) else ''
        hits = []
        if k == 'UnaryExprOrTypeTraitExpr':
            hits = refers_priv(n.get('argType', {}).get('qualType', '') or '', 'val')
            if not hits and n.get('inner'):
                hits = refers_priv(n['inner'][0].get('type', {}).get('qualType', ''), 'val')
        elif k == 'BinaryOperator' and n.get('opcode') in ('+', '-') or k == 'CompoundAssignOperator' \
                and n.get('opcode') in ('+=', '-='):
            for c in n.get('inner', []):
                t = c.get('type', {}).get('qualType', '')
                hits += refers_priv(t, 'ptr')
        elif k == 'UnaryOperator' and n.get('opcode') in ('++', '--'):
            t = n['inner'][0].get('type', {}).get('qualType', '')
            hits = refers_priv(t, 'ptr')
        elif k == 'ArraySubscriptExpr':
            if const_int(n['inner'][1]) != 0:
                hits = refers_priv(n.get('type', {}).get('qualType', ''), 'val')
        elif k in ('VarDecl', 'FieldDecl', 'ParmVarDecl') and n.get('storageClass') != 'extern':
            t = n.get('type', {}).get('qualType', '')
            hits = refers_priv(t, 'val')
        for rid in hits:
            size = max([x['off'] + x['size'] for x in pflat[rid]] or [0])
            if rid not in bad:
                # the size only matters if it differs; probe the real sizeof
                report.append(f'SIZE-USE {priv[rid][2]} ({k} at {n["_b"]}): private size covers {size:#x}')
                bad.add(('size', rid))

    # sizeof of private records, probed
    sz_probe = '\n'.join(f'char __szp_{i}[sizeof({v[0]})];' for i, (rid, v) in enumerate(priv.items()))
    pa, _, _ = clast.clang_json(src + '\n' + sz_probe + '\n', 'size.c')
    sizes = {}
    for n in pa.get('inner', []):
        if n.get('kind') == 'VarDecl' and n.get('name', '').startswith('__szp_'):
            i = int(n['name'][6:])
            m = re.match(r'char\s*\[(\d+)\]', n['type']['qualType'])
            sizes[list(priv)[i]] = int(m.group(1))
    for rid, v in priv.items():
        if ('size', rid) in bad and sizes.get(rid) != canon[v[1]]['size']:
            report.append(f'SKIP {v[2]}: used by value/sizeof/pointer arithmetic, size {sizes.get(rid):#x}')
            bad.add(rid)
    for rid in list(priv):
        if rid in bad:
            nm, short, bare = priv.pop(rid)
            rename.pop(bare, None)
            for name, r in T.td_rec.items():
                if r == rid:
                    rename.pop(name, None)
    if not priv and not clones:
        print('\n'.join(report))
        print('nothing to convert')
        return 2
    # recompute normalized types with the final renames
    pflat = {rid: layout.flatten(T, lays, rid, v[0], rename=rename) for rid, v in priv.items()}
    fid = {}
    for rid in priv:
        for x in pflat[rid]:
            if '.' not in x['path'] and '[' not in x['path']:
                fid[x['node']['id']] = (rid, x['path'])

    def pinfo(rid, p):
        for x in pflat[rid]:
            if x['path'] == p:
                return x
        return None

    def cfield_at(short, off, ty=None, size=None):
        """Canonical entries at an offset: exact type match first."""
        cands = [x for x in canon[short]['flat'] if x['off'] == off]
        if ty is not None:
            ex = [x for x in cands if x['type'] == ty]
            return ex[0] if ex else None
        if size is not None:
            same = [x for x in cands if x['size'] == size]
            if same:
                return same[0]
        return cands[0] if cands else None

    def ccontaining(short, off):
        best = None
        for x in canon[short]['flat']:
            if x['off'] <= off < x['off'] + x['size'] and '.' not in x['path'] and '[' not in x['path']:
                best = x
        return best

    def type_text(qual):
        """A private field's declared type, spelled for a cast, with the renames."""
        t = clast.unee(qual)
        for old, new in rename.items():
            t = re.sub(r'\b%s\b' % re.escape(old), new, t)
        return t

    def fn_of(n):
        while n is not None and n.get('kind') != 'FunctionDecl':
            n = n['_parent']
        return n.get('name') if n else None

    reps = []   # (b, e, generator)

    def render(b, e, skip=None):
        inner = sorted([r for r in reps if b <= r[0] and r[1] <= e and r is not skip], key=lambda r: (r[0], -r[1]))
        out, pos = [], b
        top = []
        for r in inner:
            if top and r[0] < top[-1][1]:
                continue
            top.append(r)
        for rb, re_, gen in top:
            out.append(src[pos:rb])
            out.append(gen())
            pos = re_
        out.append(src[pos:e])
        return ''.join(out)

    unspellable = set()
    stats = {'exact': 0, 'cast': 0, 'store': 0, 'ptr': 0, 'arr': 0}
    notes = []
    for n in clast.walk(ast):
        if n.get('kind') != 'MemberExpr' or not n.get('_main'):
            continue
        ref = n.get('referencedMemberDecl')
        if ref not in fid:
            continue
        rid, root = fid[ref]
        short = priv[rid][1]
        base = n['inner'][0]
        arrow = n.get('isArrow')
        fnname = fn_of(n)
        strict = fnname in cast_fns
        # build the chain upward
        chain = [(n, root, pinfo(rid, root))]
        cur = n
        while True:
            p = cur['_parent']
            info = chain[-1][2]
            if p.get('kind') == 'MemberExpr' and not p.get('isArrow') and p['inner'][0] is cur:
                q = pinfo(rid, chain[-1][1] + '.' + p['name'])
                if q is None:
                    break
                chain.append((p, chain[-1][1] + '.' + p['name'], q))
                cur = p
                continue
            if info and info['type'].endswith(']') and p.get('kind') == 'ImplicitCastExpr' and \
                    p.get('castKind') == 'ArrayToPointerDecay':
                pp = p['_parent']
                if pp.get('kind') == 'ArraySubscriptExpr' and pp['inner'][0] is p:
                    k = const_int(pp['inner'][1])
                    if k is None:
                        break
                    et = re.sub(r'\[\d+\]$', '', info['type'])
                    m = re.search(r'\[(\d+)\]$', info['type'])
                    esz = info['size'] // int(m.group(1))
                    eq = re.sub(r'\s*\[\d+\]\s*$', '', info['qual'])
                    chain.append((pp, chain[-1][1] + f'[{k}]',
                                  dict(off=info['off'] + k * esz, size=esz, type=et, qual=eq, path=None)))
                    cur = pp
                    continue
            break
        btext_range = (base['_b'], base['_e'])
        op = '->' if arrow else '.'
        # shortest prefix with an exact canonical match
        pick = None
        for i, (node, pth, info) in enumerate(chain):
            if info is None:
                break
            c = cfield_at(short, info['off'], info['type'])
            if c is None:
                continue
            if i > 0 and ('[' in pth and a.cast_arrays or strict):
                continue
            pick = (i, node, c)
            break

        def ctx(node):
            p = node['_parent']
            while p.get('kind') == 'ParenExpr':
                node, p = p, p['_parent']
            k = p.get('kind')
            if k == 'ImplicitCastExpr' and p.get('castKind') == 'LValueToRValue':
                return 'read'
            if k == 'BinaryOperator' and p.get('opcode') == '=' and p['inner'][0] is node:
                pp = p['_parent']
                used = pp.get('kind') not in ('CompoundStmt', 'CaseStmt', 'DefaultStmt', 'LabelStmt')
                return 'store-used' if used else 'store'
            if k == 'ImplicitCastExpr' and p.get('castKind') == 'ArrayToPointerDecay':
                return 'decay'
            return 'lvalue'

        if pick is not None:
            i, node, c = pick
            if c['path'] == chain[i][1] and not ('[' in chain[i][1]):
                continue  # same name and type: only the type name changes
            rng = (n['_b'], node['_e'])
            gen = (lambda br=btext_range, op=op, path=c['path']: render(*br) + op + path)
            reps.append((rng[0], rng[1], gen))
            stats['exact'] += 1
            if i:
                stats['arr'] += 1
            continue
        # no exact match: cast at the root, or at a constant array element
        # the deepest element of the chain whose type can be spelled
        use = chain[0]
        for el in (chain[:2] if strict and chain[0][2]['type'].endswith(']') and len(chain) > 1 and '[' in chain[1][1]
                   else chain[:1] if strict else chain):
            if el[2] is not None and '(unnamed' not in el[2]['qual'] and '(anonymous' not in el[2]['qual']:
                use = el
        node, pth, info = use
        if '(unnamed' in info['qual'] or '(anonymous' in info['qual']:
            notes.append(f'UNNAMED TYPE {pth} in {fnname}')
            unspellable.add(rid)
            continue
        off = info['off']
        c = cfield_at(short, off, size=info['size'])
        if c is None:
            cc = ccontaining(short, off)
            if cc is None:
                notes.append(f'NO FIELD at {off:#x} for {pth} in {fnname}')
                continue
            d = off - cc['off']
            if cc['type'].endswith(']') and cc['type'].startswith('unsigned char'):
                addr = lambda br=btext_range, op=op, cc=cc, d=d: '&' + render(*br) + op + cc['path'] + f'[{d}]'
            else:
                addr = lambda br=btext_range, op=op, cc=cc, d=d: f'((char *)&{render(*br)}{op}{cc["path"]} + {d:#x})'
                if not info['type'].endswith(']') and d % info['size'] == 0:
                    # an element of the field seen as an array of the old type
                    tt0 = type_text(info['qual'])
                    star0 = tt0 + ' *' if not tt0.endswith('*') else tt0 + '*'
                    k0 = d // info['size']
                    gen = lambda br=btext_range, op=op, cc=cc, star0=star0, k0=k0: \
                        f'(({star0})&{render(*br)}{op}{cc["path"]})[{k0}]'
                    reps.append((n['_b'], node['_e'], gen))
                    stats['cast'] += 1
                    continue
            cpath_type = None
        else:
            addr = lambda br=btext_range, op=op, c=c: '&' + render(*br) + op + c['path']
            cpath_type = c['type']
        k = ctx(node)
        is_arr = info['type'].endswith(']')
        cdesc = (c['path'] + ' ' + c['type']) if c else ('in ' + ccontaining(short, off)['path'])
        notes.append(f'CAST {priv[rid][2]} {pth} @{off:#x} {info["type"]} ({k}) -> {cdesc} [{fnname}]')
        qual = info['qual']
        if is_arr:
            et = type_text(re.sub(r'\s*\[\d+\]\s*$', '', qual))
            if k != 'decay':
                notes.append(f'ARRAY used whole: {pth} in {fnname} ({k})')
            fmt = '(({t} *){a})'
            if k == 'decay' and c is not None and c['type'].endswith(']') and \
                    re.sub(r'\[\d+\]$', '', c['type']) == re.sub(r'\[\d+\]$', '', info['type']):
                # the shared field is an array of the same elements: it decays to the same pointer
                gen = lambda br=btext_range, op=op, c=c: render(*br) + op + c['path']
                reps.append((n['_b'], node['_e'], gen))
                stats['arr'] += 1
                continue
            if k == 'decay':
                dp = node['_parent']
                while dp.get('kind') == 'ParenExpr':
                    dp = dp['_parent']
                gp = dp['_parent']
                if gp.get('kind') == 'ImplicitCastExpr' and re.fullmatch(r'(const )?void \*', gp['type']['qualType']):
                    fmt = '{a}'   # into a void * the address needs no cast
                elif gp.get('kind') in ('CallExpr', 'VarDecl', 'ReturnStmt') or \
                        gp.get('kind') == 'BinaryOperator' and gp.get('opcode') == '=' and gp['inner'][1] is dp or \
                        gp.get('kind') == 'ImplicitCastExpr' and gp['_parent'].get('kind') in ('CallExpr', 'VarDecl'):
                    fmt = '({t} *){a}'
            gen = lambda addr=addr, et=et, fmt=fmt: fmt.format(t=et, a=addr())
            stats['cast'] += 1
        else:
            tt = type_text(qual)
            intlike = lambda t: t in ('char', 'unsigned char', 'signed char', 'short', 'unsigned short', 'int',
                                      'unsigned int', 'long', 'unsigned long')
            psz = info['size']
            csz = c['size'] if c else None
            wordlike = lambda t: t is not None and (t in ('int', 'unsigned int') or t.endswith('*'))
            castp = node['_parent']
            if castp.get('kind') == 'ImplicitCastExpr' and castp.get('castKind') == 'LValueToRValue':
                castp = castp['_parent']
            explicit = castp.get('kind') == 'CStyleCastExpr' and c is not None and \
                T.norm(castp['type']['qualType'], rename) == cpath_type and psz == csz
            if explicit:
                # (T)m->old where T is the shared field's type: the shared field read directly
                gen = lambda br=btext_range, op=op, c=c: render(*br) + op + c['path']
                stats['store'] += 1
            elif c and k == 'store' and not (a.cast_stores or strict) and intlike(info['type']) and \
                    intlike(cpath_type) and psz == csz:
                gen = lambda br=btext_range, op=op, c=c: render(*br) + op + c['path']
                stats['store'] += 1
            elif c and k == 'store' and not strict and wordlike(info['type']) and wordlike(cpath_type) and psz == csz == 4:
                # the shared field stored directly, the value converted to its type
                asg = node['_parent']
                while asg.get('kind') == 'ParenExpr':
                    asg = asg['_parent']
                rhs = asg['inner'][1]
                ct = clast.unee(c['qual'])
                gen = lambda br=btext_range, op=op, c=c: render(*br) + op + c['path']
                rrep = [rhs['_b'], rhs['_e'], None]

                def rgen(rrep=rrep, ct=ct):
                    t = render(rrep[0], rrep[1], skip=rrep)
                    if t.strip() == '0':
                        return t   # a null pointer or zero needs no conversion
                    return f'({ct}){t}' if re.fullmatch(r'[\w.\[\]>-]+', t) and '-' not in t.replace('->', '') else f'({ct})({t})'
                rrep[2] = rgen
                reps.append(rrep)
                stats['store'] += 1
            elif c and k == 'read' and not strict and wordlike(info['type']) and wordlike(cpath_type) and psz == csz == 4:
                gp = node['_parent']
                if gp.get('kind') == 'ImplicitCastExpr':
                    gp = gp['_parent']
                if gp.get('kind') == 'ImplicitCastExpr':
                    gp = gp['_parent']
                bare = gp.get('kind') in ('CallExpr', 'VarDecl', 'ReturnStmt', 'ParenExpr') or \
                    gp.get('kind') == 'BinaryOperator' and gp.get('opcode') == '='
                fmt = '({t}){b}{o}{p}' if bare else '(({t}){b}{o}{p})'
                gen = lambda br=btext_range, op=op, c=c, tt=tt, fmt=fmt: fmt.format(t=tt, b=render(*br), o=op, p=c['path'])
                stats['ptr'] += 1
            else:
                star = tt + ' *' if not tt.endswith('*') else tt + '*'
                if '(*)' in tt:   # function pointer
                    star = tt.replace('(*)', '(**)')
                gen = lambda addr=addr, star=star: f'(*({star}){addr()})'
                stats['cast'] += 1
                # &(*(T *)&x) is (T *)&x
                up, pn = node, node['_parent']
                while pn.get('kind') == 'ParenExpr':
                    up, pn = pn, pn['_parent']
                if pn.get('kind') == 'UnaryOperator' and pn.get('opcode') == '&':
                    gp = pn['_parent']
                    bare = gp.get('kind') in ('CallExpr', 'VarDecl', 'ReturnStmt', 'ParenExpr') or \
                        gp.get('kind') == 'ImplicitCastExpr' and gp['_parent'].get('kind') in ('CallExpr', 'VarDecl') or \
                        gp.get('kind') == 'BinaryOperator' and gp.get('opcode') == '=' and gp['inner'][1] is pn
                    fmt = '({s}){a}' if bare else '(({s}){a})'
                    if gp.get('kind') == 'ImplicitCastExpr' and re.fullmatch(r'(const )?void \*', gp['type']['qualType']):
                        fmt = '{a}'   # into a void * the address needs no cast
                    gen = lambda addr=addr, star=star, fmt=fmt: fmt.format(s=star, a=addr())
                    reps.append((pn['_b'], pn['_e'], gen))
                    continue
        reps.append((n['_b'], node['_e'], gen))

    if unspellable:
        print('cannot spell a private field type in', ', '.join(priv[r][2] for r in unspellable), '; skip them')
        return 3
    # remove private definitions and their typedefs / forward declarations
    dels = []
    gone = set(priv) | set(clones)
    gone_names = {v[2] for v in priv.values()} | set(clones.values())
    for n in clast.walk(ast):
        if not n.get('_main') or n.get('_b') is None:
            continue
        k = n.get('kind')
        if k == 'RecordDecl' and n['_parent'] is ast:
            nm = n.get('name')
            if n['id'] in gone:
                dels.append((n['_b'], n['_e']))
            elif (nm in gone_names or nm in rename) and not n.get('completeDefinition'):
                # only a forward declaration standing alone: `struct X;`
                lb = src.rfind('\n', 0, n['_b']) + 1
                if src[lb:n['_b']].strip() == '' and re.match(r'\s*;', src[n['_e']:]):
                    dels.append((n['_b'], n['_e']))
        elif k == 'TypedefDecl' and n['_parent'] is ast:
            if n['name'] in rename:
                dels.append((n['_b'], n['_e']))
    # merge overlapping spans and extend over the ';' and the line end
    dels.sort()
    merged = []
    for b, e in dels:
        if merged and b <= merged[-1][1] + 1:
            merged[-1][1] = max(merged[-1][1], e)
        else:
            merged.append([b, e])
    spans = []
    for b, e in merged:
        m = re.match(r'[ \t]*;', src[e:])
        if not m:
            report.append(f'DEFINITION WITH A DECLARATOR at line {src.count(chr(10), 0, e) + 1}: {src[e:e + 40]!r}')
            print('\n'.join(report))
            return 4
        e += len(m.group(0))
        # whole lines
        lb = src.rfind('\n', 0, b) + 1
        if src[lb:b].strip() == '':
            b = lb
        le = src.find('\n', e)
        if le != -1 and src[e:le].strip() == '':
            e = le + 1
        spans.append((b, e))
    if a.dry:
        print('\n'.join(report))
        print('records:', ', '.join(v[2] for v in priv.values()), '| vec clones:', ', '.join(clones.values()))
        print('stats', stats)
        print('\n'.join(notes))
        return 0
    # assemble: apply deletions as replacements with ''
    for b, e in spans:
        reps.append((b, e, lambda: ''))
    out = render(0, len(src))
    for old, new in sorted(rename.items(), key=lambda kv: -len(kv[0])):
        if old != new:
            out = re.sub(r'\b%s\b' % re.escape(old), new, out)
    # forward declarations the header now gives
    fwd = '|'.join(list(tgt['recs']) + (['Manip', 'GifEntry', 'AnimSeq'] if a.target == 'moby' else []))
    out = re.sub(r'^struct (%s);[ \t]*\n' % fwd, '', out, flags=re.M)
    for tag in set(re.findall(r'^struct (\w+_[0-9A-Fa-f]+);[ \t]*$', out, flags=re.M)):
        if len(re.findall(r'\b%s\b' % tag, out)) == 1 and len(re.findall(r'\b%s\b' % tag, src)) > 1:
            out = re.sub(r'^struct %s;[ \t]*\n' % tag, '', out, flags=re.M)
    hdr = '#include "%s"' % tgt['header']
    if hdr not in out:
        m = re.search(r'^#include "include_asm.h"\n', out, re.M) or re.search(r'^#include "common.h"\n', out, re.M)
        out = out[:m.end()] + hdr + '\n' + out[m.end():]
    path.write_text(out)
    print('\n'.join(report))
    print('records:', ', '.join(v[2] for v in priv.values()), '| vec clones:', ', '.join(clones.values()))
    print('stats', stats)
    print('\n'.join(notes))
    return 0


if __name__ == '__main__':
    sys.exit(main())
