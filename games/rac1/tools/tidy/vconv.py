#!/usr/bin/env python3
"""Carry a moby-view conversion into the version copies of a file.

  python3 tools/tidy/vconv.py PALFILE TYPE...

Splices each version's own definitions (src/versions/<K>/<stem>/func_*.c) into
PALFILE as of HEAD (before the conversion), runs tools/tidy/conv.py (--target $TARGET, default moby; shared record $SHORT, default Moby) on
the private TYPEs there, and writes the converted functions back. Check with
tools/version_src.py build and tools/version_check.py afterwards.
"""
import sys,re,subprocess,glob,os
pal=sys.argv[1]; types=sys.argv[2:]
head=subprocess.run(['git','show','HEAD:'+pal],capture_output=True,text=True).stdout
def span(text,name):
    # [start,end) of NAME's definition: from its line start to the matching closing brace
    m=re.search(r'^[^\n;{}]*\b%s\s*\([^;{]*\)\s*\{'%re.escape(name),text,re.M)
    if not m: return None
    i=text.index('{',m.start()); d=0
    for j in range(i,len(text)):
        if text[j]=='{': d+=1
        elif text[j]=='}':
            d-=1
            if d==0: return m.start(),j+1
def defname(body):
    m=re.search(r'\b(\w+)\s*\([^;{]*\)\s*\{',body); return m.group(1)
stem=os.path.splitext(pal)[0][4:]
for vd in sorted(glob.glob(f'src/versions/*/{stem}')):
    files=sorted(glob.glob(vd+'/func_*.c')); text=head; names_=[]
    for f in files:
        body=open(f).read().rstrip('\n'); n=defname(body); sp=span(text,n)
        if not sp: sys.exit(f'{f}: {n} not found in HEAD {pal}')
        text=text[:sp[0]]+body+text[sp[1]:]; names_.append((f,n))
    if not any(re.search(r'\b(%s)\b'%'|'.join(types),open(f).read()) for f,_ in names_): continue
    tmp=os.path.join(os.path.dirname(pal),'zz_vconv_tmp.c'); open(tmp,'w').write(text)
    for t in types:
        subprocess.run(['python3','tools/tidy/conv.py',tmp,'--target',os.environ.get('TARGET','moby'),'--only',t,'--as',f"{t}={os.environ.get('SHORT','Moby')}"],capture_output=True)
    out=open(tmp).read(); os.remove(tmp)
    for f,n in names_:
        sp=span(out,n); new=out[sp[0]:sp[1]]+'\n'
        if new!=open(f).read(): open(f,'w').write(new); print('converted',f)
