/*
 * Tier: sol-high, task v1263 (19 source candidates).
 * Best: 1788/1788 bytes, 0/27 USA v1.01 placements; first mismatch +0x360.
 * Profile: -O2 -G8 -ffunction-sections -g, in a dedicated source unit.
 * 40 non-relocation instruction words differ. Retail preserves the segment
 * frame in s3 after copying it from s7 at the loop preheader; this C keeps s7.
 * The resulting loop allocation rotates the mask, saved color, and packet
 * corner pointers. The preheader also emits a nop where retail copies s7.
 * The entire non-relocation prefix through +0x35c agrees. Indexed points[i]
 * and points[i+1] recover the two induction pointers and the original spills;
 * a separate cursor variable does not. Ascending color assignment chains and
 * r.d/r.c/r.a copy order recover retail packet initialization. Separate
 * conversion/division statements with -g reproduce the two pre-division nops.
 * Three segment-pointer forms produced identical text (candidates 16/18/19).
 * Reopen with independent evidence for the segment-frame lifetime/split;
 * do not repeat pointer renaming. All required declarations follow.
 */
#include "common.h"
typedef unsigned int Q __attribute__((mode(TI)));
typedef union { Q q; float f[4]; } V;
typedef struct { V v[4]; unsigned color[4]; float uv[4][2]; long a,b,c,d; } Packet;
typedef struct { V row[3],pos; } Frame;
extern unsigned D_L00_001B1380 __attribute__((sda));
extern float D_L00_001B1384 __attribute__((sda)),D_L00_001B1388 __attribute__((sda)),D_L00_001B138C __attribute__((sda));
extern int D_L00_001B13D0 __attribute__((sda));
extern char D_L00_001B8C80[];
extern long func_0027C878(int);
extern void func_00282CF0(void*,void*,void*),func_00282F20(void*,void*,float),func_00282DD8(void*,void*,void*),func_00282D30(void*,void*,float),func_002830C0(void*,void*,void*),func_00280B90(void*,int,int);
extern float func_00283CE0(int),func_002A7AA8(float,float,float,float,float);
void func_L00_00413918(char *object,V *points,int alpha,int alpha2,int count)
{
    Packet p,r;
    V size, size2,dir;
    Frame frame;
    V first,second;
    Frame segment;
    char *state=*(char **)(object+0x68);
    unsigned color, color2;
    int i;
    float t, width;
    p.b=func_0027C878(0x17);
    p.d=0x8000000048ULL;
    p.c=0xff9000000260ULL;
    p.color[0]=p.color[1]=p.color[2]=p.color[3]=(D_L00_001B1380&0xffffff)|((unsigned)alpha<<24);
    p.a=0;
    r.b=func_0027C878(0xe);
    r.color[0]=r.color[1]=r.color[2]=r.color[3]=(*(unsigned *)(state+0x24)&0xffffff)|((unsigned)alpha2<<24);
    r.d=p.d;r.c=p.c;r.a=p.a;
    r.uv[0][0]=1;r.uv[0][1]=0;r.uv[1][0]=1;r.uv[1][1]=1;
    r.uv[2][0]=0;r.uv[2][1]=0;r.uv[3][0]=0;r.uv[3][1]=1;
    p.uv[0][0]=*(float *)(state+0x14);p.uv[0][1]=0;
    p.uv[1][0]=*(float *)(state+0x14);p.uv[1][1]=1;
    p.uv[2][0]=*(float *)(state+0x14)+1;p.uv[2][1]=0;
    p.uv[3][0]=*(float *)(state+0x14)+1;p.uv[3][1]=1;
    size.q=0;size.f[3]=1;size.f[2]=D_L00_001B1388;
    size2.q=0;size2.f[3]=1;size2.f[2]=D_L00_001B1388*D_L00_001B138C;
    func_00282CF0(&dir,state+0x30,object+0x10);
    func_00282F20(&dir,&dir,1);
    qcopy(&frame.row[0],&dir);
    func_00282DD8(&frame.row[1],&frame.row[0],object+0xe0);
    func_00282F20(&frame.row[1],&frame.row[1],1);
    func_00282DD8(&frame.row[2],&frame.row[1],&frame.row[0]);
    qcopy(&frame.pos,object+0x10);
    qcopy(&p.v[0],&size);func_00282D30(&p.v[1],&p.v[0],-1);
    qcopy(&r.v[0],&size2);
    func_00282D30(&r.v[1],&r.v[0],-1);
    func_002830C0(&first,&points[0],&frame);func_002830C0(&second,&points[1],&frame);
    func_00282CF0(&segment.row[0],&second,&first);func_00282F20(&segment.row[0],&segment.row[0],1);
    func_00282CF0(&segment.row[1],D_L00_001B8C80+0x140,&first);
    func_00282DD8(&segment.row[2],&segment.row[1],&segment.row[0]);
    func_00282F20(&segment.row[2],&segment.row[2],1);
    func_00282DD8(&segment.row[1],&segment.row[0],&segment.row[2]);qcopy(&segment.pos,&first);
    func_002830C0(&p.v[0],&p.v[0],&segment);func_002830C0(&p.v[1],&p.v[1],&segment);
    func_002830C0(&r.v[0],&r.v[0],&segment);func_002830C0(&r.v[1],&r.v[1],&segment);
    for(i=1;i<count;i++) {
        qcopy(&p.v[2],&size);func_00282D30(&p.v[3],&p.v[2],-1);
        qcopy(&r.v[2],&size2);func_00282D30(&r.v[3],&r.v[2],-1);
        if(i==count-1) func_002830C0(&segment.pos,&points[i],&frame);
        else {
            func_002830C0(&first,&points[i],&frame);func_002830C0(&second,&points[i+1],&frame);
            func_00282CF0(&segment.row[0],&second,&first);func_00282F20(&segment.row[0],&segment.row[0],1);
            func_00282CF0(&segment.row[1],D_L00_001B8C80+0x140,&first);
            func_00282DD8(&segment.row[2],&segment.row[1],&segment.row[0]);func_00282F20(&segment.row[2],&segment.row[2],1);
            func_00282DD8(&segment.row[1],&segment.row[0],&segment.row[2]);qcopy(&segment.pos,&first);
        }
        func_002830C0(&p.v[2],&p.v[2],&segment);func_002830C0(&p.v[3],&p.v[3],&segment);
        func_002830C0(&r.v[2],&r.v[2],&segment);func_002830C0(&r.v[3],&r.v[3],&segment);
        if(i==1 && D_L00_001B13D0==alpha2) {
            color=p.color[0];p.color[0]=p.color[1]=0xffffff;
            color2=r.color[0];r.color[0]=r.color[1]=*(unsigned *)(state+0x24)&0xffffff;
            func_00280B90(&p,0,0);func_00280B90(&r,0,0);
            p.color[0]=p.color[1]=p.color[2]=p.color[3]=color;
            r.color[0]=r.color[1]=r.color[2]=r.color[3]=color2;
        } else if(i==count-2 && D_L00_001B13D0==alpha2) {
            color=p.color[2];p.color[2]=p.color[3]=0xffffff;
            color2=r.color[2];r.color[2]=r.color[3]=*(unsigned *)(state+0x24)&0xffffff;
            func_00280B90(&p,0,0);func_00280B90(&r,0,0);
            p.color[0]=p.color[1]=p.color[2]=p.color[3]=color;
            r.color[0]=r.color[1]=r.color[2]=r.color[3]=color2;
        } else {func_00280B90(&p,0,0);func_00280B90(&r,0,0);}
        qcopy(&p.v[0],&p.v[2]);qcopy(&p.v[1],&p.v[3]);qcopy(&r.v[0],&r.v[2]);qcopy(&r.v[1],&r.v[3]);
        t=func_00283CE0(i);
        t /= func_00283CE0(count);
        width=func_002A7AA8(-1,0,1,0,t);
        size.f[2]=D_L00_001B1388+(D_L00_001B1384-D_L00_001B1388)*width;size2.f[2]=size.f[2]*D_L00_001B138C;
        p.uv[0][0]=*(float *)(state+0x14);p.uv[0][1]=0;p.uv[1][0]=*(float *)(state+0x14);p.uv[1][1]=1;
        p.uv[2][0]=*(float *)(state+0x14)+1;p.uv[2][1]=0;p.uv[3][0]=*(float *)(state+0x14)+1;p.uv[3][1]=1;
    }
}
