"""Record layouts by probing clang (offsetof/sizeof in array bounds), with EE type sizes."""
import re
import clast

SCALAR_TD = {'u8': 'unsigned char', 's8': 'signed char', 'u16': 'unsigned short', 's16': 'short',
             'u32': 'unsigned int', 's32': 'int', 's64': 'long', 'u64': 'unsigned long',
             'f32': 'float', 'f64': 'double'}


_ANON = {}


def anon_sig(m):
    """An unnamed record spelled by its body, so two copies of one union compare equal."""
    key = m.group(0)
    if key not in _ANON:
        try:
            lines = open(m.group(2), errors='replace').read().split('\n')
            txt = '\n'.join(lines[int(m.group(3)) - 1:])[int(m.group(4)) - 1:]
            depth, end = 0, None
            for i, ch in enumerate(txt):
                if ch == '{':
                    depth += 1
                elif ch == '}':
                    depth -= 1
                    if depth == 0:
                        end = i
                        break
            body = txt[txt.index('{') + 1:end]
            body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
            body = re.sub(r'\b(\w+?)_[0-9A-Fa-f]{4,}\b', r'\1', body)
            for k, v in SCALAR_TD.items():
                body = re.sub(r'\b%s\b' % k, v, body)
            _ANON[key] = m.group(1) + '{' + re.sub(r'\s+', '', body) + '}'
        except Exception:
            _ANON[key] = key
    return _ANON[key]


class Types:
    """Typedef and record tables of one AST."""

    def __init__(self, ast, mainfile):
        self.td = {}        # typedef name -> node
        self.rec = {}       # id -> RecordDecl node (definitions)
        self.tag = {}       # tag name -> id (definition)
        self.td_rec = {}    # typedef name -> record id
        for n in clast.walk(ast):
            k = n.get('kind')
            if k == 'RecordDecl' and n.get('completeDefinition'):
                self.rec[n['id']] = n
                if n.get('name'):
                    self.tag[n['name']] = n['id']
            elif k == 'TypedefDecl':
                self.td[n['name']] = n
                rid = None
                for m in clast.walk(n):
                    d = m.get('decl') or m.get('ownedTagDecl')
                    if isinstance(d, dict) and d.get('kind') == 'RecordDecl':
                        rid = d['id']
                        break
                if rid is not None and n['type']['qualType'].split()[0] in ('struct', 'union') or (
                        rid is not None and not n['type']['qualType'].endswith('*')):
                    self.td_rec[n['name']] = rid
        # a typedef of a record forward-declared before its definition points at the forward decl
        for name, rid in list(self.td_rec.items()):
            if rid not in self.rec:
                qt = self.td[name]['type']['qualType']
                m = re.match(r'(?:struct|union) (\w+)$', qt)
                if m and m.group(1) in self.tag:
                    self.td_rec[name] = self.tag[m.group(1)]

    def rec_name(self, rid):
        n = self.rec[rid]
        if n.get('name'):
            return n['tagUsed'] + ' ' + n['name']
        for name, r in self.td_rec.items():
            if r == rid:
                return name
        return None

    def norm(self, t, rename=None):
        """A comparable spelling of a type string: scalar typedefs expanded, record
        typedefs as `struct NAME`, private record names renamed."""
        rename = rename or {}
        t = clast.unee(t)
        t = re.sub(r'(struct|union) \((?:unnamed|anonymous)(?: struct| union)? at ([^:()]+):(\d+):(\d+)\)', anon_sig, t)
        t = re.sub(r'\b(struct|union) (\w+)', lambda m: 'struct ' + rename.get(m.group(2), m.group(2)), t)

        def ident(m):
            w = m.group(0)
            if w in ('struct', 'union', 'unsigned', 'signed', 'char', 'short', 'int', 'long', 'float',
                     'double', 'void', 'const', 'volatile'):
                return w
            if w in SCALAR_TD:
                return SCALAR_TD[w]
            if w in self.td_rec:
                return 'struct ' + rename.get(w, w)
            if w in self.td:
                return '(' + self.norm(self.td[w]['type']['qualType'], rename) + ')'
            return rename.get(w, w)
        # keep "struct NAME" names as they are
        parts = re.split(r'(struct \w+)', t)
        out = ''.join(p if p.startswith('struct ') else re.sub(r'\b[A-Za-z_]\w*\b', ident, p) for p in parts)
        out = re.sub(r'\((unsigned char|signed char|char|unsigned short|short|unsigned int|int|long|unsigned long|'
                     r'long long|unsigned long long|float|double)\)', r'\1', out)
        out = out.replace('unsigned long long', 'U128').replace('long long', 'S128')
        out = re.sub(r'\s+', ' ', out).replace(' *', '*').replace('* ', '*').strip()
        return out


def field_rec(types, field, parent):
    """The record id of a FieldDecl's type (unnamed records are the preceding sibling)."""
    qt = field['type'].get('desugaredQualType', field['type']['qualType'])
    base = re.sub(r'\[.*', '', qt).strip()
    if base.endswith('*'):
        return None
    if '(unnamed' in base or '(anonymous' in base:
        sibs = parent.get('inner', [])
        i = sibs.index(field)
        for s in reversed(sibs[:i]):
            if s.get('kind') == 'RecordDecl':
                return s['id']
        return None
    m = re.match(r'(?:struct|union) (\w+)$', base)
    if m and m.group(1) in types.tag:
        return types.tag[m.group(1)]
    w = field['type']['qualType']
    w = re.sub(r'\[.*', '', w).strip()
    w = re.sub(r'^(const |volatile )', '', w)
    if w in types.td_rec:
        return types.td_rec[w]
    return None


def probe_layouts(text, types_of, roots):
    """roots: {record id: C name}. Returns {rid: [field dicts]} with offsets relative to the record.
    Fields of unnamed sub-records are probed through their parent's path."""
    work = []   # (rid, cname, prefix path, prefix off-expr)
    plan = {}
    seen = set()

    def add(rid, cname, prefix):
        key = (rid, cname, prefix)
        if key in seen:
            return
        seen.add(key)
        rec = types_of.rec[rid]
        fields = []
        for f in rec.get('inner', []):
            if f.get('kind') != 'FieldDecl':
                continue
            name = f.get('name')
            if not name:   # anonymous member
                sub = field_rec(types_of, f, rec)
                if sub is not None:
                    fields.append({'anon': True, 'rid': sub, 'node': f})
                    add(sub, cname, prefix)
                continue
            if f.get('isBitfield'):
                fields.append({'name': name, 'bitfield': True, 'node': f})
                continue
            sub = field_rec(types_of, f, rec)
            path = prefix + name
            fields.append({'name': name, 'path': path, 'node': f, 'rid': sub})
            if sub is not None:
                named = types_of.rec_name(sub)
                if named and '(unnamed' not in types_of.rec[sub].get('name', ''):
                    if types_of.rec[sub].get('name') or named in types_of.td_rec:
                        add(sub, named, '')
                        continue
                add(sub, cname, path + '.')
        plan[(rid, cname, prefix)] = fields

    for rid, cname in roots.items():
        add(rid, cname, '')
    probes = []
    idx = 0
    for (rid, cname, prefix), fields in plan.items():
        for f in fields:
            if 'path' in f:
                f['probe'] = idx
                probes.append(f'char __pro_{idx}[__builtin_offsetof({cname}, {f["path"]}) + 1];'
                              f'char __prs_{idx}[sizeof(((({cname} *)0)->{f["path"]}))];')
                idx += 1
    ptext = text + '\n' + '\n'.join(probes) + '\n'
    ast, errs, fname = clast.clang_json(ptext, 'probe.c')
    vals = {}
    for n in ast.get('inner', []):
        if n.get('kind') == 'VarDecl' and n.get('name', '').startswith('__pr'):
            m = re.match(r'char\s*\[(\d+)\]', n['type']['qualType'])
            vals[n['name']] = int(m.group(1)) if m else None
    out = {}
    for (rid, cname, prefix), fields in plan.items():
        if (rid, cname, prefix) != next(k for k in plan if k[0] == rid):
            continue
    base_off = {}
    for (rid, cname, prefix), fields in plan.items():
        lay = []
        for f in fields:
            if 'probe' in f:
                o = vals.get(f'__pro_{f["probe"]}')
                s = vals.get(f'__prs_{f["probe"]}')
                lay.append(dict(f, off=None if o is None else o - 1, size=s))
            else:
                lay.append(f)
        out[(rid, cname, prefix)] = lay
    return out, errs


def flatten(types_of, lays, rid, cname, prefix='', base=0, rename=None, prefix_off=0):
    """Every (absolute offset, size, normalized type, path, field node) of a record, outer first.
    Offsets probed through a parent path are absolute within the root; subtract prefix_off."""
    out = []
    lay = lays.get((rid, cname, prefix))
    if lay is None:
        return out
    for f in lay:
        if f.get('anon'):
            out += flatten(types_of, lays, f['rid'], cname, prefix, base, rename, prefix_off)
            continue
        if f.get('bitfield') or f.get('off') is None:
            continue
        off = base + f['off'] - prefix_off
        t = types_of.norm(f['node']['type']['qualType'], rename)
        path = f['name'] if not prefix else prefix + f['name']
        rel = path[len(prefix):] if prefix else path
        out.append(dict(off=off, size=f['size'], type=t, path=path, node=f['node'], rid=f.get('rid'),
                        qual=f['node']['type']['qualType']))
        sub = f.get('rid')
        m = re.search(r'\[(\d+)\]$', t)
        if m and int(m.group(1)) <= 64 and f['size'] and not f['name'].startswith(('pad', '_pad')):
            cnt = int(m.group(1))
            esz = f['size'] // cnt
            et = re.sub(r'\[\d+\]$', '', t)
            eq = re.sub(r'\s*\[\d+\]\s*$', '', f['node']['type']['qualType'])
            nm = types_of.rec_name(sub) if sub is not None else None
            for k in range(cnt):
                ep = f'{path}[{k}]'
                eoff = off + k * esz
                out.append(dict(off=eoff, size=esz, type=et, path=ep, node=f['node'], rid=sub, qual=eq))
                if sub is not None and not et.endswith(']') and nm and (sub, nm, '') in lays:
                    out += [dict(x, path=ep + '.' + x['path']) for x in
                            flatten(types_of, lays, sub, nm, '', eoff, rename, 0)]
        if sub is not None and not t.endswith(']'):
            if (sub, cname, path + '.') in lays:
                out += [dict(x, path=x['path']) for x in
                        flatten(types_of, lays, sub, cname, path + '.', base, rename, prefix_off)]
            else:
                nm = types_of.rec_name(sub)
                if nm and (sub, nm, '') in lays:
                    out += [dict(x, path=path + '.' + x['path']) for x in
                            flatten(types_of, lays, sub, nm, '', off, rename, 0)]
    return out
