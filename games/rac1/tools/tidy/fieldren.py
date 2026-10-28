#!/usr/bin/env python3
"""fieldren.py HEADER MAP FILE...: rename fields declared in HEADER (include/<name>.h) per MAP
(line<TAB>old<TAB>new: the declaration's line in the header), only where a member access refers to
that declaration (clang AST)."""
import sys, re
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import clast
hdr, mapf, files = sys.argv[1], sys.argv[2], sys.argv[3:]
ROWS = [l.rstrip('\n').split('\t')[:3] for l in open(mapf) if l.count('\t') >= 2]
BYLINE = {(int(a), b): c for a, b, c in ROWS}
clast.prep_includes()
total = 0
for path in files:
    src = open(path).read()
    ast, errs, fname = clast.clang_json(src, 'src.c')
    clast.annotate(ast, fname)
    ids = {}
    for n in clast.walk(ast):
        if n.get('kind') == 'FieldDecl':
            loc = n.get('loc') or {}
            f = loc.get('_file') or ''
            if f.endswith(hdr) and 'offset' in loc:
                line = open(f).read().count('\n', 0, loc['offset']) + 1
                new = BYLINE.get((line, n.get('name')))
                if new:
                    ids[n['id']] = new
    edits = []
    for n in clast.walk(ast):
        if n.get('kind') == 'MemberExpr' and n.get('referencedMemberDecl') in ids:
            name = n.get('name')
            if not n.get('_main') or '_e' not in n:
                continue
            off = n['_e'] - len(name)   # the member name is the last token of the expression
            if src[off:off + len(name)] != name:
                continue
            edits.append((off, name, ids[n['referencedMemberDecl']]))
    if not edits:
        continue
    out = src
    for off, name, new in sorted(set(edits), reverse=True):
        out = out[:off] + new + out[off + len(name):]
    open(path, 'w').write(out)
    total += len(set(edits))
    print(f'{len(set(edits))}\t{path}')
print('total', total, file=sys.stderr)
