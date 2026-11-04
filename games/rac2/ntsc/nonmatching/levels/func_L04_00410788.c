/* sol-high, USA v1.01, game -O2 -G0 -ffunction-sections.
 * Best check.py: 1724/1724 bytes, 0/2 places (Barlow and Joba).
 * First relocated instruction mismatch: +0x74. Equal size is not a match.
 * Separate loop-counter, point-index, start/end and path lifetimes improve the
 * earlier 1704-byte draft to retail size and reproduce the save sequence.
 * Retail spills path at sp+0x28, base at sp+0x2C and branches at sp+0x30;
 * this draft retains path in a GPR and spills some indices/start values instead.
 * The backward-point loop also computes count-(j+1), versus retail's
 * (count-1)-j. Register allocation, scheduling and branch slots still differ.
 * Tried: shared counter/path lifetimes (1704-1716 bytes), a shared state-row
 * pointer (1632), typed state arrays (1732), and a typed path header/points
 * view (same bytes as this draft). No flags or toolchain changes were made.
 * Next: explain retail's spilled path and register-resident shared array
 * offset from source lifetimes before retrying equivalent array spellings.
 * The observed return is the float result of func_L04_0040F8F0; the old draft
 * discarded it. All declarations needed by this standalone draft follow.
 */
typedef int s32;
typedef short s16;
typedef unsigned char u8;
typedef float f32;
extern u8 *D_L04_001FE340[];
extern s32 func_00283CF0(f32);
extern void func_00282CF0(void *, void *, void *);
extern f32 func_L00_002EE290(void *);
extern f32 func_L04_0040F8F0(u8 *, u8 *, s32, s32, s32, s32);
#define I(p,o) (*(s32 *)((u8 *)(p)+(o)))
#define F(p,o) (*(f32 *)((u8 *)(p)+(o)))
float func_L04_00410788(u8 *obj, u8 *state, s32 *branch, s32 *point, s32 wrap)
{
    f32 delta[4] __attribute__((aligned(16)));
    f32 best = 9999.0f, dist;
    s32 old = *branch;
    u8 *base = D_L04_001FE340[I(state,0x1A8)];
    u8 *path, *other;
    s32 start, end, i, j, k, index, boundary;
    s32 *branches, *connections, *back;
    if (old == -1) {
        u8 *path; s32 start, end;
        start = *point - 5;
        if (start < 0) start += I(base,0)-1;
        branches=(s32 *)(state+0x1AC);
        { s32 i; for (i=0; i<4; i++) {
            if (*(s32 *)((u8 *)branches+i*4) == -1) continue;
            path = D_L04_001FE340[*(s32 *)((u8 *)branches+i*4)];
            end = start + 10;

            boundary = func_00283CF0(F(path,0x2C));
            if (end >= I(base,0)) end -= I(base,0);
            if ((I(state,0x1CC+i*4)==-1 && start<end && boundary>=start && boundary<end) ||
                (end<start && (boundary>=start || boundary<end))) {
                { s32 j; for (j=0;j<5;j++) {
                    func_00282CF0(delta,obj+0x10,path+0x10+j*0x10);
                    dist=func_L00_002EE290(delta);
                    if (dist<best) {best=dist; *branch=i; *point=j;}
                }}
            }
            connections=(s32 *)(state+0x1BC);
            boundary=func_00283CF0(F(path,0x3C));
            if ((*(s32 *)((u8 *)connections+i*4)==-1 && start<end && boundary>=start && boundary<end) ||
                (end<start && (boundary>=start || boundary<end))) {
                { s32 j; for(j=0;j<5;j++) {
                    s32 index;
                    index=I(path,0)-1-j;
                    func_00282CF0(delta,obj+0x10,path+0x10+index*0x10);
                    dist=func_L00_002EE290(delta);
                    if(dist<best) {best=dist; *branch=i; *point=index;}
                }}
            }
        }}
        { s32 i; for(i=0;i<10;i++) {
            s32 index;
            index=start+i;
            if(index>=I(base,0)) index-=I(base,0);
            func_00282CF0(delta,obj+0x10,base+0x10+index*0x10);
            dist=func_L00_002EE290(delta);
            if(dist<best) {best=dist; *branch=-1; *point=index;}
        }}
    } else {
        u8 *path; s32 start, end;
        branches=(s32 *)(state+0x1AC);
        path=D_L04_001FE340[branches[old]];
        start=*point-5;
        if(start<0) {
            start=0;
            end=func_00283CF0(F(path,0x2C))-5;
            if(end<0) {if(!wrap) end=0; else end=end-1+I(base,0);}
            { s32 i; for(i=0;i<10;i++) {
            s32 index;
                index=end+i;
                if(index>=I(base,0)) index-=I(base,0);
                func_00282CF0(delta,obj+0x10,base+0x10+index*0x10);
                dist=func_L00_002EE290(delta);
                if(dist<best) {best=dist; *branch=-1; *point=index;}
                { s32 j; for(j=0;j<4;j++) {
                    if(*(s32 *)((u8 *)branches+j*4)==-1 || j==old) continue;
                    other=D_L04_001FE340[*(s32 *)((u8 *)branches+j*4)];
                    if(func_00283CF0(F(other,0x2C))!=index) continue;
                    { s32 k; for(k=0;k<5;k++) {
                        func_00282CF0(delta,obj+0x10,other+0x10+k*0x10);
                        dist=func_L00_002EE290(delta);
                        if(dist<best) {best=dist; *branch=j; *point=k;}
                    }}
                }}
            }}
        } else if(*point>I(path,0)-4) {
            end=func_00283CF0(F(path,0x3C))-5;
            connections=(s32 *)(state+0x1BC);
            j=*(s32 *)((u8 *)connections+old*4);
            if(j!=-1) other=D_L04_001FE340[*(s32 *)((u8 *)branches+j*4)]; else other=base;
            if(end<0) {if(!wrap) end=0; else end=end-1+I(other,0);}
            { s32 i; for(i=0;i<10;i++) {
            s32 index;
                index=end+i;
                if(index>=I(other,0)) index-=I(other,0);
                func_00282CF0(delta,obj+0x10,other+0x10+index*0x10);
                dist=func_L00_002EE290(delta);
                if(dist<best) {best=dist; *branch=*(s32 *)((u8 *)connections+old*4); *point=index;}
            }}
        }
        { s32 i; for(i=0;i<10;i++) {
            s32 index;
            index=start+i;
            if(index>=I(base,0)) break;
            func_00282CF0(delta,obj+0x10,path+0x10+index*0x10);
            dist=func_L00_002EE290(delta);
            if(dist<best) {best=dist; *point=index; *branch=old;}
        }}
    }
    return func_L04_0040F8F0(obj,state,*branch,*point,wrap,*(s16 *)(obj+0xAA)==0xBBC);
}
