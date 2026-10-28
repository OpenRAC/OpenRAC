#!/usr/bin/env python3
"""Raw byte-offset moby accesses -> shared Moby fields (include/moby.h).

  python3 tools/tidy/rawmoby.py [--dry] [--wide] [--func=NAME] [--list] FILE...

Works inside functions whose first parameter is a byte or void pointer the body
treats as a moby (it reads the pvars at +0x78). `*(T *)(m + OFF)`, `*(T *)((char *)m
+ OFF)` and `m[OFF]` become `((Moby *)m)->field` where T is the field's type.
--locals also takes byte/void pointer locals with the same pvars read.
--func=NAME limits it to one function; --list prints the functions it would change.
--wide also turns pvars reads into `((T *)((Moby *)m)->pvars)` and same-size type
differences into `*(T *)&((Moby *)m)->field`. The bytes must not change: check
each file (tools/overlay_file_check.py, or the full build) and revert it if they do.
"""
import re,sys
dry='--dry' in sys.argv; WIDE='--wide' in sys.argv; LOCALS='--locals' in sys.argv; LIST='--list' in sys.argv
ONLY=[a[7:] for a in sys.argv[1:] if a.startswith('--func=')]
files=[a for a in sys.argv[1:] if not a.startswith('--')]
F={}
def add(off,name,t): F[off]=(name,t)
for i,c in enumerate('xyzw'):
    add(0x00+4*i,f'bsphere.{c}','f32'); add(0x10+4*i,f'pos.{c}','f32'); add(0x40+4*i,f'rot.{c}','f32')
    add(0xC0+4*i,f'rot_row0.{c}','f32'); add(0xD0+4*i,f'rot_row1.{c}','f32'); add(0xE0+4*i,f'rot_row2.{c}','f32')
for off,n,t in [(0x20,'state','u8'),(0x21,'group','u8'),(0x22,'class_slot','u8'),(0x23,'alpha','u8'),(0x2C,'scale','f32'),
 (0x30,'update_dist','u8'),(0x31,'visible','u8'),(0x32,'draw_dist','s16'),(0x34,'flags','u16'),(0x36,'occlusion','u16'),
 (0x50,'frame','u8'),(0x51,'prev_frame','u8'),(0x52,'seq','u8'),(0x53,'prev_seq','u8'),(0x54,'anim_time','f32'),(0x58,'anim_speed','f32'),
 (0x6C+4,'anim_flags','u8'),(0x71,'unk71','u8'),(0x72,'unk72','u8'),(0x73,'unk73','u8'),(0x7C,'unk7C','u8'),(0x7D,'sound_slot','u8'),
 (0x7E,'anim_trigger_count','u8'),(0x7F,'shadow_range','u8'),(0x80,'unk80','s32'),(0x86,'unk86','s16'),(0x90,'glow_color','s32'),
 (0x94,'collision','u32'),(0x98,'collision_disable','s32'),(0xA4,'hit_slot','u8'),(0xA6,'oclass','s16'),(0xB0,'mission','u8'),
 (0xB1,'unkB1','u8'),(0xB2,'save_id','u16'),(0xB4,'unkB4','s16'),(0xB6,'unkB6','s16'),(0xBC,'command','u8')]: add(off,n,t)
SP={'f32':{'float','f32'},'u8':{'unsigned char','u8'},'s16':{'short','s16','signed short'},'u16':{'unsigned short','u16'},
    's32':{'int','s32','signed int'},'u32':{'unsigned int','u32','unsigned'}}
SIZE={'f32':4,'u8':1,'s16':2,'u16':2,'s32':4,'u32':4,'float':4,'char':1,'signed char':1,'s8':1,'unsigned char':1,'short':2,'unsigned short':2,'int':4,'unsigned int':4,'unsigned':4}
ELEM={'unsigned char':'u8','u8':'u8'}
DEF=re.compile(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\(\s*(?:const\s+)?(char|unsigned char|u8|void)\s*\*\s*(\w+)\s*[,)][^;{]*\{',re.M)
DEFANY=re.compile(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\(([^;{]*)\)\s*\{',re.M)
LOCAL=re.compile(r'(?:^|[;{(,])\s*(?:const\s+)?(char|unsigned char|u8|void)\s*\*\s*(\w+)\s*(?=[=;,)\[])',re.M)
PV=r'\(\s*(\(char \*\)\s*)?%s\s*\+\s*0x78\s*\)'
n=0

def rewrite(body,v,et):
    global n
    def rep(x):
        global n
        T=re.sub(r'\s*\*',' *',x.group(1).strip()).replace('  ',' '); off=int(x.group(2),16)
        after=body[x.end():x.end()+3]; before=body[max(0,x.start()-2):x.start()]
        store=re.match(r'\s*(=[^=]|\+=|-=|\|=|&=|\^=|\+\+|--)',after) is not None or before.endswith(('++','--'))
        if off in F and T in SP[F[off][1]]: n+=1; return f'((Moby *){v})->{F[off][0]}'
        if WIDE and off==0x78 and T.endswith('*') and not store: n+=1; return f'(({T})((Moby *){v})->pvars)'
        if WIDE and off in F and SIZE.get(T)==SIZE[F[off][1]] and not T.endswith('*'): n+=1; return f'*({T} *)&((Moby *){v})->{F[off][0]}'
        return x.group(0)
    nb=re.sub(r'\*\(\s*([A-Za-z_][\w ]*?(?:\s*\*)*)\s*\*\s*\)\s*\(\s*(?:\(\s*(?:char|unsigned char|u8)\s*\*\s*\)\s*)?%s\s*\+\s*(0x[0-9A-Fa-f]+)\s*\)'%v,rep,body)
    if et in ELEM:
        def rep2(x):
            global n
            off=int(x.group(1),16)
            if off in F and F[off][1]==ELEM[et]: n+=1; return f'((Moby *){v})->{F[off][0]}'
            return x.group(0)
        nb=re.sub(r'(?<![\w.>])%s\s*\[\s*(0x[0-9A-Fa-f]+)\s*\]'%v,rep2,nb)
    return nb

tot=0
for p in files:
    t=open(p).read(); res=[]; n=0
    for m in DEFANY.finditer(t):
        if ONLY and m.group(1) not in ONLY: continue
        start=t.index('{',m.end()-1); d=0
        for j in range(start,len(t)):
            if t[j]=='{': d+=1
            elif t[j]=='}':
                d-=1
                if d==0: end=j; break
        body=t[start:end+1]; cands=[]
        pm=re.match(r'\s*(?:const\s+)?(char|unsigned char|u8|void)\s*\*\s*(\w+)\s*(?:,|$)',m.group(2))
        if pm: cands.append((pm.group(2),pm.group(1)))
        if LOCALS:
            for lm in LOCAL.finditer(body):
                if (lm.group(2),lm.group(1)) not in cands: cands.append((lm.group(2),lm.group(1)))
        nb=body
        for v,et in cands:
            if re.search(PV%v,nb): nb=rewrite(nb,v,et)
        if nb!=body:
            if LIST: print('FUNC',m.group(1))
            res.append((start,end+1,nb))
    out=t
    for s_,e_,nb in reversed(res): out=out[:s_]+nb+out[e_:]
    if n:
        tot+=n; print(f'{n}\t{p}')
        if not dry: open(p,'w').write(out)
print('total',tot,file=sys.stderr)
