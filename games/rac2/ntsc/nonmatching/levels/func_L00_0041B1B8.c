/* GPT-6.1 Sol high worker, 29 October 2026: USA v1.01 near miss.
 * Best check: 1656 compiled / 1672 retail bytes, 0/27 placements.
 * Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * Frame 0x2D0 and saved register set now agree. Differences remain in the
 * third packet's initialization, pointer spills/copy setup, loop padding
 * and preservation of the raw preceding index before wrapping it to 34.
 * Retain the contiguous four-vector basis and copy the second layer's own
 * endpoint vertices after drawing: the earlier draft got both wrong.
 * Array packets, separate packets and typed ring/state views tied exactly;
 * an explicit first-point pointer and -g did not close the differences. Explicit
 * qcopy inhibits ordinary decay-constant hoisting; -fmove-all-movables
 * hoists 6.0f as well and adds an unwanted saved FP register. -fno-gcse
 * worsens allocation. Next: explain real pointer and raw-index lifetimes.
 * 0x700024A9/0x70002CA9 are compiler-reachable por clears, not evidence of
 * handwritten assembly. The full declarations needed are included below.
 */

#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 q; float f[4]; } TrailVector;
typedef struct { TrailVector v[4]; int color[4]; float uv[8]; long long tag[4]; } TrailPacket;
extern long long ref_L00_0041B1B8_0(int);
extern float ref_L00_0041B1B8_1(float, float);
extern void ref_L00_0041B1B8_2(void *);
extern void ref_L00_0041B1B8_3(void *, void *, void *);
extern void ref_L00_0041B1B8_4(void *, void *, float);
extern void ref_L00_0041B1B8_5(void *, void *, void *);
extern void ref_L00_0041B1B8_6(void *, void *, float);
extern void ref_L00_0041B1B8_7(void *, void *, void *);
extern void ref_L00_0041B1B8_8(void *, int, int);
/* A ring entry is 64 bytes; only its first position is used here. */
typedef struct { TrailVector position, other; char pad[32]; } TrailPoint;
typedef struct { TrailVector position; char pad[0x34]; int count, index; char pad4C[12]; float u, v; } TrailState;
extern TrailPoint *D_L00_001B144C __attribute__((sda));
extern TrailVector D_L00_001B8DC0;
/* Draw three textured layers along the 35-entry position ring. The outer
 * layer is six times wider, and both widths taper by 0.94 per segment. */
void func_L00_0041B1B8(char *self, float width)
{
    TrailPacket core, streak, glow;
    /* The vector-transform helper consumes all four consecutive columns. */
    TrailVector basis[4], narrowOffset, wideOffset, first, second;
    TrailState *p = *(TrailState **)(self + 0x68);
    int index, prev, next, i;
    float half;
    if (p->count == 0) return;
    core.tag[1] = ref_L00_0041B1B8_0(10);
    core.tag[2] = 0xFF9000000260LL;
    core.tag[3] = 0x4000000064LL;
    core.color[0] = core.color[1] = core.color[2] = core.color[3] = 0x609F9F9F;
    core.tag[0] = 0;
    streak.tag[1] = ref_L00_0041B1B8_0(14);
    streak.tag[2] = 0xFF9000000260LL;
    streak.tag[3] = 0x4000000068LL;
    streak.color[0] = streak.color[1] = streak.color[2] = streak.color[3] = 0x507F7F7F;
    streak.tag[0] = 0;
    glow.tag[1] = ref_L00_0041B1B8_0(43);
    glow.tag[2] = 0xFF9000000260LL;
    glow.tag[3] = 0x5000000068LL;
    glow.color[0] = glow.color[1] = glow.color[2] = glow.color[3] = 0x55103080;
    glow.tag[0] = 0;
    half = width * 0.5f;
    narrowOffset.q = 0;
    narrowOffset.f[3] = 1.0f;
    narrowOffset.f[2] = ref_L00_0041B1B8_1(half, width);
    wideOffset.q = 0;
    wideOffset.f[3] = 1.0f;
    wideOffset.f[2] = narrowOffset.f[2] * 6.0f;
    ref_L00_0041B1B8_2(&core);
    core.uv[0] = p->u; core.uv[1] = p->v;
    core.uv[2] = p->u; core.uv[3] = p->v+1;
    core.uv[4] = p->u+1; core.uv[5] = p->v;
    core.uv[6] = p->u+1; core.uv[7] = p->v+1;
    streak.uv[0]=p->u; streak.uv[1]=0;
    streak.uv[2]=p->u; streak.uv[3]=1;
    streak.uv[4]=p->u+1; streak.uv[5]=0;
    streak.uv[6]=p->u+1; streak.uv[7]=1;
    glow.uv[0]=p->u; glow.uv[1]=0;
    glow.uv[2]=p->u; glow.uv[3]=1;
    glow.uv[4]=p->u+1; glow.uv[5]=0;
    glow.uv[6]=p->u+1; glow.uv[7]=1;
    first.q = p->position.q;
    index=p->index;
    second.q = D_L00_001B144C[index].position.q;
    ref_L00_0041B1B8_3(&basis[0],&second,&first);
    ref_L00_0041B1B8_4(&basis[0],&basis[0],1.0f);
    ref_L00_0041B1B8_3(&basis[1],&D_L00_001B8DC0,&first);
    ref_L00_0041B1B8_5(&basis[2],&basis[1],&basis[0]);
    ref_L00_0041B1B8_4(&basis[2],&basis[2],1.0f);
    ref_L00_0041B1B8_5(&basis[1],&basis[0],&basis[2]);
    basis[3].q = first.q;
    core.v[0].q = narrowOffset.q;
    ref_L00_0041B1B8_6(&core.v[1],&core.v[0],-1.0f);
    ref_L00_0041B1B8_7(&core.v[0],&core.v[0],&basis[0]);
    ref_L00_0041B1B8_7(&core.v[1],&core.v[1],&basis[0]);
    streak.v[0].q = core.v[0].q; streak.v[1].q = core.v[1].q;
    glow.v[0].q = wideOffset.q;
    ref_L00_0041B1B8_6(&glow.v[1],&glow.v[0],-1.0f);
    ref_L00_0041B1B8_7(&glow.v[0],&glow.v[0],&basis[0]);
    ref_L00_0041B1B8_7(&glow.v[1],&glow.v[1],&basis[0]);
    for (i = 1; i < p->count; i++) {
        core.v[2].q = narrowOffset.q; ref_L00_0041B1B8_6(&core.v[3],&core.v[2],-1);
        glow.v[2].q = wideOffset.q; ref_L00_0041B1B8_6(&glow.v[3],&glow.v[2],-1);
        prev=index-1;
        next=prev>=0?prev:34;
        first.q = D_L00_001B144C[index].position.q;
        second.q = D_L00_001B144C[next].position.q;
        ref_L00_0041B1B8_3(&basis[0],&second,&first); ref_L00_0041B1B8_4(&basis[0],&basis[0],1);
        ref_L00_0041B1B8_3(&basis[1],&D_L00_001B8DC0,&first);
        ref_L00_0041B1B8_5(&basis[2],&basis[1],&basis[0]); ref_L00_0041B1B8_4(&basis[2],&basis[2],1);
        ref_L00_0041B1B8_5(&basis[1],&basis[0],&basis[2]); basis[3].q = first.q;
        ref_L00_0041B1B8_7(&core.v[2],&core.v[2],&basis[0]); ref_L00_0041B1B8_7(&core.v[3],&core.v[3],&basis[0]);
        ref_L00_0041B1B8_7(&glow.v[2],&glow.v[2],&basis[0]); ref_L00_0041B1B8_7(&glow.v[3],&glow.v[3],&basis[0]);
        streak.v[2].q = core.v[2].q; streak.v[3].q = core.v[3].q;
        ref_L00_0041B1B8_8(&core,0,0); ref_L00_0041B1B8_8(&streak,0,0); ref_L00_0041B1B8_8(&glow,0,0);
        core.v[0].q = core.v[2].q; core.v[1].q = core.v[3].q;
        /* Each draw may change its packet; advance each layer separately. */
        streak.v[0].q = streak.v[2].q; streak.v[1].q = streak.v[3].q;
        glow.v[0].q = glow.v[2].q; glow.v[1].q = glow.v[3].q;
        index=prev; if(index<0) index=34;
        width*=0.94f; half*=0.94f;
        core.color[0]=core.color[2]; core.color[1]=core.color[2];
        streak.color[0]=streak.color[2]; streak.color[1]=streak.color[2];
        glow.color[0]=glow.color[2]; glow.color[1]=glow.color[2];
        narrowOffset.f[2]=ref_L00_0041B1B8_1(half,width); wideOffset.f[2]=narrowOffset.f[2]*6;
    }
    basis[3].q = second.q;
    ref_L00_0041B1B8_7(&core.v[2],&core.v[2],&basis[0]); ref_L00_0041B1B8_7(&core.v[3],&core.v[3],&basis[0]);
}
