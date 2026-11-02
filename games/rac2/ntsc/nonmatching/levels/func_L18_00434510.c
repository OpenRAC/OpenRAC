/* sol-high, USA v1.01 game -O2 -G0 -ffunction-sections.
 * Best check.py: 1176/1176 bytes, 0 of 1 places match, 20 instruction
 * words differ outside relocated operands. No linked match.
 * The packet color chains, final color clear chains and UV stores now agree.
 * Remaining: width[2] is stored first instead of in the initial difference
 * call's delay slot, and the loop basis[2]/next-point pointers swap s6/s7.
 * Moving width initialization, a contiguous four-row matrix, and a TI/float
 * width union do not fix these residuals. Explicit next-point induction was
 * already worse in the earlier tier. Reopen with pointer-lifetime or scheduler
 * evidence, rather than another equivalent store spelling.
 * The four basis vectors form one 64-byte matrix read by func_002830C0;
 * they must not be represented as four independent C objects.
 * Declarations needed by this standalone draft follow.
 */
#include "common.h"
extern float D_L18_001B97C0[];
extern void func_00282CF0(float *, const float *, const float *);
extern void func_00282F20(char *, char *, float);
extern void func_00282D30(void *, void *, float);
typedef int TI128 __attribute__((mode(TI)));
extern unsigned long func_0027C878(int);
extern void func_L00_00412088(void *);
extern void func_00280B90(void *, int, int);
extern void func_00282DD8(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern float func_00283CE0(int);
typedef float B1351Vec[4] __attribute__((aligned(16)));
typedef struct { B1351Vec v[4]; int c[4]; float uv[8]; unsigned long gs[4]; } B1351Quad;
void func_L18_00434510(float (*p)[4], int count, int life, int color, int outer, float size, float speed) {
    B1351Vec basis[4];
    B1351Quad q;
    B1351Quad r;
    B1351Vec width, offset;
    float u,v;
    int i;
    float *next=p[1];
    q.gs[1]=func_0027C878(0x2B);
    q.gs[3]=((unsigned long)life<<32)|0x68;
    q.gs[2]=0xFF9000000260UL;
    q.c[0]=q.c[1]=q.c[2]=q.c[3]=color;
    q.gs[0]=0;
    func_L00_00412088(&q);
    q.uv[0]=0; q.uv[1]=0; q.uv[2]=0; q.uv[3]=1;
    q.uv[4]=1; q.uv[5]=0; q.uv[6]=1; q.uv[7]=1;
    r.gs[1]=func_0027C878(0x2B);
    r.gs[2]=0xFF9000000260UL;
    r.gs[0]=0;
    r.gs[3]=((unsigned long)(life/3)<<32)|0x68;
    r.c[0]=r.c[1]=r.c[2]=r.c[3]=outer;
    func_L00_00412088(&r);
    *(TI128 *)width=0; *(TI128 *)offset=0;
    width[2]=size;
    r.uv[0]=0; r.uv[1]=0; r.uv[2]=0; r.uv[3]=1;
    r.uv[4]=0.33f; r.uv[5]=0; r.uv[6]=0.33f; r.uv[7]=1;
    offset[2]=size*speed;
    width[3]=1; offset[3]=1;
    func_00282CF0(basis[0],next,p[0]);
    func_00282F20((char *)basis[0],(char *)basis[0],1);
    func_00282CF0(basis[1],D_L18_001B97C0,p[0]);
    func_00282DD8(basis[2],basis[1],basis[0]);
    func_00282F20((char *)basis[2],(char *)basis[2],1);
    func_00282DD8(basis[1],basis[0],basis[2]);
    qcopy(basis[3],p[0]);
    qcopy(q.v[0],width);
    func_00282D30(q.v[1],q.v[0],-1);
    func_002830C0(q.v[0],q.v[0],basis[0]);
    func_002830C0(q.v[1],q.v[1],basis[0]);
    qcopy(r.v[0],offset);
    func_00282D30(r.v[1],r.v[0],-1);
    func_002830C0(r.v[0],r.v[0],basis[0]);
    func_002830C0(r.v[1],r.v[1],basis[0]);
    for(i=1;i<count-1;i++) {
        qcopy(q.v[2],width);
        func_00282D30(q.v[3],q.v[2],-1);
        qcopy(r.v[2],offset);
        func_00282D30(r.v[3],r.v[2],-1);
        func_00282CF0(basis[0],p[i+1],p[i]);
        func_00282F20((char *)basis[0],(char *)basis[0],1);
        func_00282CF0(basis[1],D_L18_001B97C0,p[i]);
        func_00282DD8(basis[2],basis[1],basis[0]);
        func_00282F20((char *)basis[2],(char *)basis[2],1);
        func_00282DD8(basis[1],basis[0],basis[2]);
        qcopy(basis[3],p[i]);
        func_002830C0(q.v[2],q.v[2],basis[0]);
        func_002830C0(q.v[3],q.v[3],basis[0]);
        func_002830C0(r.v[2],r.v[2],basis[0]);
        func_002830C0(r.v[3],r.v[3],basis[0]);
        if(i==count-2) { q.c[3]=q.c[2]=0; r.c[3]=r.c[2]=0; }
        func_00280B90(&q,0,0); func_00280B90(&r,0,0);
        u=func_00283CE0(i)*0.33f; v=u+0.33f;
        if(u>=1) {u-=1; v-=1;}
        r.uv[0]=u; r.uv[4]=v; r.uv[2]=u; r.uv[6]=v;
        qcopy(q.v[0],q.v[2]); qcopy(q.v[1],q.v[3]);
        qcopy(r.v[0],r.v[2]); qcopy(r.v[1],r.v[3]);
    }
}
