/* NON_MATCHING func_L00_00273578 -- src/overlays/shared/partupd_00272158.c
 * Best so far: SIZE ours 348 / retail 352, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   This is a scheduler tie per `docs/WORKER.md`'s stop rule ("a register
 *   allocation [scheduling choice] that three different wordings leave
 *   unchanged"): our compiler's local scheduler is more aggressive about
 *   delay-slot filling than whatever produced retail's binary for this
 *   function, and no plain-C rewording of the condition or the drift code
 *   changed that. `p6.c` is the best candidate: right register set, right
 *   call order, right constants, only the one hoisted instruction differs
 *   from retail.
 */
#include "common.h"

extern void func_L00_002688A8(void *arg0);
extern float func_002140F8(float a, float b);
extern void func_L00_00250800(void *arg0, int arg1, void *arg2);
extern void func_001F9BF0(void *dst, void *a, void *b); /* dst = a - b (vector) */
extern float func_001F9CB8(void *a); /* |a| */
extern void func_L00_001FF4B0(void *dst, void *src, float len);
extern void func_001F9BD8(void *dst, void *a, void *b); /* dst = a + b */

/* Particle update: if the target moby (at +0x30) is gone, its short at
   +0xA6 no longer matches our cached class id at +0x38, or its state
   byte at +0x20 is 0xFE/0xFD, the particle is retired via
   func_L00_002688A8. Otherwise it drifts: func_L00_00250800 advances
   the target-relative position at +0x10 using the index at +0x34, the
   frame's delta from that is jittered to a random length near 0.0625
   and added into the velocity at +0x20; +0x1C gets a fresh random
   speed scale from the +0x30 record's +0xC field, and +0x2C is left
   unchanged. */
void func_L00_00273578(void *arg0) {
    char *self = (char *)arg0;
    char *ti = self + 0x30;
    void *target = *(void **)ti;
    void *vel;
    void *pos;
    float scale;
    float posY;
    float buf[4];
    float jitter;
    float len;

    if (target == 0) {
        goto retire;
    }
    if (*(short *)((char *)target + 0xA6) != *(int *)(ti + 8)) {
        goto retire;
    }
    if (*(unsigned char *)((char *)target + 0x20) == 0xFE) {
        goto retire;
    }
    if (*(unsigned char *)((char *)target + 0x20) == 0xFD) {
        goto retire;
    }
    goto drift;

retire:
    func_L00_002688A8(arg0);
    return;

drift:
    vel = self + 0x10;
    pos = vel;
    scale = *(float *)(ti + 0xC) * func_002140F8(0.5f, 1.0f);
    posY = *(float *)(self + 0x2C);

    qcopy(buf, vel);
    func_L00_00250800(*(void **)ti, *(int *)(ti + 4), vel);
    func_001F9BF0(buf, buf, vel);
    jitter = func_002140F8(-0.021f, 0.021f);
    if (func_001F9CB8(buf) < 0.0625f) {
        len = jitter + 0.0625f;
    } else {
        len = func_001F9CB8(buf) + jitter;
    }
    func_L00_001FF4B0(buf, buf, len);
    func_001F9BD8(self + 0x20, pos, buf);
    *(float *)(self + 0x2C) = posY;
    *(float *)(self + 0x1C) = scale;
}
