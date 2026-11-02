/* opus-1 (v0763), 29 October 2026, upstream toolchain.
 * Unit flags: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier
 * (the body was checked inside src/levels/shared/L00_003B9CB8-qcopy.c, whose
 * Slot3BBB68 type and _6/_8 callee aliases it can reuse there).
 * Best check.py: 684/684 bytes, 0/27 places; 28 instruction words differ, all
 * FP register swaps: retail keeps pi in f20 and -pi in f21, 4.75 in f24 and
 * 3.25 in f25 (the 0.001/0.005 pair already matches: 0.005 f22, 0.001 f23).
 * Cause, measured with -dS -dl -fsched-verbose dumps: global.c allocates the
 * pairs by priority, ties by pseudo number (low arg first). Priority ties come
 * from sched1's order of preheader block 0: a pair's two hoisted constant
 * loads issue one per cycle on the memory unit, and the high constant's live
 * length is shorter only when another insn lands between the two loads. Here
 * sched1 always gives A B S1 CX H S2 L c1 c2 C c3 I c4 c5 c6 (A/B/C = arg
 * copies, S1/S2 = owner/obj+0x10, H/L = table hi/lo, CX = ctx load, I = i=0),
 * so only the middle pair is separated. Permuting the pre-loop statements does
 * not change this (boosted priorities decide it); inline addresses (not
 * hoisted, other code), pre-loop constant locals (gcse propagates them, 816
 * bytes), and src2 assigned inside the loop (not hoisted) all lose.
 * A hand model of the scheduler says retail's allocation follows if S2 were a
 * low-priority filler (unboosted, or hoisted after the constants); no natural
 * C found for that. -fmove-all-movables hoists it but strength-reduces the
 * slot address (724 bytes).
 */
#include "include_asm.h"

/* The grouped quadword copy: the R5900 lq/sq pair with both addresses in registers. */
typedef int Rac2TI128 __attribute__((mode(TI)));

/* One of the thirty 0xA0-byte records at D_L00_0023DDA0 (see L00_003B9CB8-qcopy.c). */
typedef struct {
    float out0[4];          /* 0x00 */
    float out1[4];          /* 0x10 */
    Rac2TI128 start;        /* 0x20: copied from the owner's position */
    Rac2TI128 pos;          /* 0x30 */
    float ang0[3];          /* 0x40 */
    float rate0[3];         /* 0x4C */
    float amp0[3];          /* 0x58 */
    float ang1[3];          /* 0x64 */
    float rate1[3];         /* 0x70 */
    float amp1[3];          /* 0x7C */
    int life;               /* 0x88 */
    char *owner;            /* 0x8C */
    char *target;           /* 0x90 */
    int time;               /* 0x94 */
    int duration;           /* 0x98 */
    int hit;                /* 0x9C */
} Slot3BBB68;

extern Slot3BBB68 D_L00_0023DDA0[];

extern float func_002A7878(float low, float high);
extern float func_002A78D8(float low, float high);
extern void func_00282C88(void *p);
extern char *func_002AB1F0(char *object);
extern void func_L00_003252A8(char *object, void *out, int mode);
extern void func_00282D30(void *out, void *in, float scale);
extern void func_00282CC0(void *out, void *in1, void *in2);

void func_L00_003BB8B8(char *owner, char *obj, void *arg)
{
    Rac2TI128 *src1 = (Rac2TI128 *)(owner + 0x10);
    Slot3BBB68 *base = D_L00_0023DDA0;
    Rac2TI128 *src2 = (Rac2TI128 *)(obj + 0x10);
    Slot3BBB68 *slot;
    char *other;
    float v[4];
    char *ctx;
    int i;

    ctx = *(char **)(owner + 0x68);
    for (i = 0; i < 30; i++) {
        slot = base + i;
        if (slot->owner != 0)
            continue;
        slot->owner = owner;
        (*(int *)(ctx + 0x54))++;
        slot->start = *src1;
        slot->ang0[0] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->ang0[1] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->ang0[2] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->ang1[0] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->ang1[1] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->ang1[2] = func_002A7878(-3.14159274f, 3.14159274f);
        slot->rate0[0] = func_002A78D8(0.001f, 0.005f);
        slot->rate0[1] = func_002A78D8(0.001f, 0.005f);
        slot->rate0[2] = func_002A78D8(0.001f, 0.005f);
        slot->rate1[0] = func_002A78D8(0.001f, 0.005f);
        slot->rate1[1] = func_002A78D8(0.001f, 0.005f);
        slot->rate1[2] = func_002A78D8(0.001f, 0.005f);
        slot->amp0[0] = func_002A7878(3.25f, 4.75f);
        slot->amp0[1] = func_002A7878(3.25f, 4.75f);
        slot->amp0[2] = func_002A7878(3.25f, 4.75f);
        slot->amp1[0] = func_002A7878(3.25f, 4.75f);
        slot->amp1[1] = func_002A7878(3.25f, 4.75f);
        slot->amp1[2] = func_002A7878(3.25f, 4.75f);
        func_00282C88(slot->out0);
        func_00282C88(slot->out1);
        slot->time = (int)arg;
        slot->duration = (int)arg;
        slot->life = 20;
        slot->target = obj;
        slot->hit = 0;
        other = func_002AB1F0(obj);
        if (other != 0) {
            func_L00_003252A8(obj, v, 1);
            func_00282D30(v, v, *(float *)(other + 0x10));
            func_00282CC0(&slot->pos, v, obj + 0x10);
        } else {
            slot->pos = *src2;
        }
        break;
    }
}
