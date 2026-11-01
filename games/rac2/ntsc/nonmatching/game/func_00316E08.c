/* Tier: sol-high, task v1032. USA v1.01, all 28 placements.
 * Best check.py: 1220 bytes / expected 1220; 0 of 28 places match.
 * Profile: -O2 -G0 -ffunction-sections, dedicated source unit.
 * First mismatch +0x94: the green color's split constant spills its intermediate
 * 0x80000000 value to sp+0x188 before the completed 0x8000FF00 store. Retail has
 * only the completed store. This shifts the prefix by one instruction until
 * retail's extra loop-alignment nop; the remaining loop and epilogue match.
 * Naming green, next, i in that order fixes the scalar spill slots. Writing the
 * next point as &points[i+1] fixes pointer-increment ordering. Writing texture
 * x components before z produces retail's z-then-x store order.
 * Signed/unsigned/decimal color wordings tie. A 64-bit color rematerializes it;
 * an explicit workspace struct prevents useful induction; no-sched1 and
 * mno-split-addresses worsen the body. Stop at the constant-spill blocker.
 * All required declarations are below. No linked build was run.
 */
#include "common.h"
typedef int TrailQuad __attribute__((mode(TI)));
typedef struct { float x,y,z,w; } TrailVector;
typedef struct {
    TrailVector vertices[4];
    unsigned int color[4];
    TrailVector uv[2];
    unsigned long long tag[4];
} TrailPacket;
extern unsigned long long func_0027C878(int);
extern void func_00316DA0(void *, int);
extern void func_00316B40(void *);
extern void func_00282CF0(void *,void *,void *);
extern void func_00282DD8(void *,void *,void *);
extern void func_00282F20(void *,void *,float);
extern void func_00282D30(void *,void *,float);
extern void func_002830C0(void *,void *,void *);
extern void func_00280B90(void *,int,int);
extern float func_00283CE0(int);
extern TrailVector D_001B5340;
void func_00316E08(int unused, int color, TrailVector *points, int count)
{
    TrailPacket a,b;
    TrailVector forward,side,up,origin;
    TrailVector thin, thick;
    int green;
    TrailVector *next = points + 1;
    int i;
    a.tag[1]=func_0027C878(43);
    a.tag[3]=((unsigned long long)(unsigned int)color << 32)|0x68;
    green=-2147418368;
    a.color[0]=a.color[1]=0x7FFFFFFF;
    a.tag[2]=0xFF9000000260ULL;
    a.tag[0]=0;
    b.tag[1]=func_0027C878(43);
    b.tag[2]=0xFF9000000260ULL;
    b.tag[0]=0;
    b.tag[3]=((unsigned long long)(unsigned int)(color/3)<<32)|0x68;
    *(TrailQuad *)&thin=0;
    *(TrailQuad *)&thick=0;
    a.color[1]=green; b.color[0]=green;
    thin.z=0.05f; thick.z=0.2f; thin.w=1; thick.w=1;
    func_00316DA0(points,count);
    func_00316B40(&a);
    func_00316B40(&b);
    a.uv[0].x=0; a.uv[0].y=0; a.uv[0].z=0; a.uv[0].w=1;
    a.uv[1].x=1; a.uv[1].y=0; a.uv[1].z=1; a.uv[1].w=1;
    b.uv[0].x=0; b.uv[0].y=0; b.uv[0].z=0; b.uv[0].w=1;
    b.uv[1].x=0.33f; b.uv[1].y=0; b.uv[1].z=0.33f; b.uv[1].w=1;
    func_00282CF0(&forward,next,points);
    func_00282F20(&forward,&forward,1);
    func_00282CF0(&side,&D_001B5340,points);
    func_00282DD8(&up,&side,&forward);
    func_00282F20(&up,&up,1);
    func_00282DD8(&side,&forward,&up);
    qcopy(&origin,points);
    qcopy(&a.vertices[0],&thin);
    func_00282D30(&a.vertices[1],&a.vertices[0],-1);
    func_002830C0(&a.vertices[0],&a.vertices[0],&forward);
    func_002830C0(&a.vertices[1],&a.vertices[1],&forward);
    qcopy(&b.vertices[0],&thick);
    func_00282D30(&b.vertices[1],&b.vertices[0],-1);
    func_002830C0(&b.vertices[0],&b.vertices[0],&forward);
    func_002830C0(&b.vertices[1],&b.vertices[1],&forward);
    for(i=1;i<count-1;i++) {
        float u,v;
        qcopy(&a.vertices[2],&thin);
        func_00282D30(&a.vertices[3],&a.vertices[2],-1);
        qcopy(&b.vertices[2],&thick);
        func_00282D30(&b.vertices[3],&b.vertices[2],-1);
        func_00282CF0(&forward,&points[i+1],&points[i]);
        func_00282F20(&forward,&forward,1);
        func_00282CF0(&side,&D_001B5340,points+i);
        func_00282DD8(&up,&side,&forward);
        func_00282F20(&up,&up,1);
        func_00282DD8(&side,&forward,&up);
        qcopy(&origin,points+i);
        func_002830C0(&a.vertices[2],&a.vertices[2],&forward);
        func_002830C0(&a.vertices[3],&a.vertices[3],&forward);
        func_002830C0(&b.vertices[2],&b.vertices[2],&forward);
        func_002830C0(&b.vertices[3],&b.vertices[3],&forward);
        a.color[2]=a.color[3]=0x7F7F7F7F;
        b.color[2]=b.color[3]=green;
        func_00280B90(&a,0,0);
        func_00280B90(&b,0,0);
        u=func_00283CE0(i)*0.33f;
        v=u+0.33f;
        if(u>=1) {u-=1;v-=1;}
        b.uv[0].x=u;b.uv[1].x=v;b.uv[0].z=u;b.uv[1].z=v;
        qcopy(&a.vertices[0],&a.vertices[2]);
        qcopy(&a.vertices[1],&a.vertices[3]);
        qcopy(&b.vertices[0],&b.vertices[2]);
        qcopy(&b.vertices[1],&b.vertices[3]);
    }
}
