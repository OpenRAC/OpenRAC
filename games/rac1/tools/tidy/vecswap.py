"""vecswap.py FILE TYPE TARGET: drop FILE's private typedef TYPE and use the shared TARGET (include/vector.h).
Check the file afterwards (tools/overlay_file_check.py or the full build) and revert it if the bytes change."""
import re,sys
p,ty,tg=sys.argv[1:4]; t=open(p).read()
m=re.search(r'typedef (?:struct|union)\s*\w*\s*\{[^{}]*\}\s*%s\s*(?:__attribute__\(\([^)]*\)\))?\s*;\n?'%re.escape(ty),t)
if not m: sys.exit('no typedef')
t=t[:m.start()]+t[m.end():]
TOK=re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|.',re.S)
t=''.join(tg if x.group(0)==ty else x.group(0) for x in TOK.finditer(t))
if '#include "vector.h"' not in t:
    t=t.replace('#include "common.h"','#include "common.h"\n#include "vector.h"',1)
open(p,'w').write(t)
