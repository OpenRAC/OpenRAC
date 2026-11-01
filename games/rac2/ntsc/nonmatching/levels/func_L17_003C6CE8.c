/* func_L17_003C6CE8 near miss (opus-1): 812/812 bytes, 0/1 places, four instructions differ.
 * Build as its own unit with -O2 -G8 -ffunction-sections (file named *-g8.c).
 * Only the yaw arguments differ: retail loads point.x/point.y into $f1/$f0, this compiler
 * into $f0/$f1 (and, with y computed first, the y subtraction goes first instead).
 * Cause, measured with -fsched-verbose-5: this compiler's haifa CALL_INSN path no longer
 * clears reg_last_uses of call-used hard registers, so every later call in the block
 * anti-depends on each $1 (frame-pointer) stack load. sched1's depend_count tie-break then
 * issues both point loads before the $s2 object loads, and local-alloc gives the
 * first-born/shorter-lived of x/y $f0. No source order of the two point loads gives
 * y=$f0 with retail's load/subtract order (all orders enumerated).
 * With -fno-schedule-insns the yaw block alone is exactly retail (direct wording below),
 * but the rest of the function loses retail's sched1 order (frame 0x70, no $s7).
 * Declarations needed: all of the externs below. */
#include "common.h"
typedef unsigned int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vec;
extern char *D_L17_001FE880[];
extern char D_00188660[];
extern unsigned char D_L17_001DBF82 NOT_SDA;
extern float D_L17_001B0344 __attribute__((sda));
extern float D_L17_001B0348 __attribute__((sda));
extern float D_L17_001B034C __attribute__((sda));
extern float D_L17_001B0350 __attribute__((sda));
extern float D_L17_001B0354 __attribute__((sda));
extern float D_L17_001B0358 __attribute__((sda));
extern void func_00282CF0(void *, void *, void *);
extern float func_00282DF0(void *);
extern float func_00282E48(void *, void *);
extern void func_002AA2E8(void *, float, float);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_00282E20(void *);
extern float func_00283248(float, float);
extern void func_002AAA00(void *, void *, float, float, float, float, int);
extern int func_002E59B0(void *, int);
extern int func_002E5DA0(int, int, void *, int);
extern void func_002E59F8(int);
int func_L17_003C6CE8(char *obj, int path, int *effect)
{
    Vec point, delta;
    char **entry = &D_L17_001FE880[path];
    char *state = *(char **)(obj + 0x68);
    int done = 0;
    float distance, accel, speed2, pitch, turn, x, y;
    char *yaw;
    qcopy(&point, *entry + *(int *)(state + 0x94) * 16 + 16);
    func_00282CF0(&delta, &point, obj + 0x10);
    if (func_00282DF0(&delta) < 2 * (D_L17_001B0344 * 0.016666668f)) {
        done = --*(int *)(state + 0x94) == -1;
    }
    distance = func_00282E48(obj + 0x10, *entry + 16);
    accel = D_L17_001B0348 * 0.00027777778450399637f;
    speed2 = *(float *)(state + 0x90) * *(float *)(state + 0x90);
    if (distance <= speed2 / (2 * accel))
        func_002AA2E8(state + 0x90, 0, speed2 / (2 * distance));
    else
        func_002AA2E8(state + 0x90, D_L17_001B0344 * 0.016666668f, accel);
    func_00282F20(&delta, &delta, *(float *)(state + 0x90));
    func_00282CC0(obj + 0x10, &delta, obj + 0x10);
    pitch = -func_00283248(func_00282E20(&delta), delta.f[2]);
    if (pitch > 0.26179942f) pitch = 0.26179942f;
    else if (pitch < -0.26179942f) pitch = -0.26179942f;
    yaw = obj + 0xF8;
    func_002AAA00(obj + 0xF4, state + 0x9C, pitch,
        D_L17_001B034C * 0.0000048481374f, D_L17_001B034C * 0.0000048481374f, 0.20943955f, 0);
    turn = func_00283248(point.f[0] - *(float *)(obj + 0x10), point.f[1] - *(float *)(obj + 0x14));
    func_002AAA00(yaw, state + 0x98, turn,
        D_L17_001B0350 * 0.0000048481374f, D_L17_001B0350 * 0.0000048481374f, 0.20943955f, 0);
    func_002AAA00(obj + 0xF0, state + 0xA0,
        *(float *)(state + 0x98) * *(float *)(state + 0x90) * D_L17_001B0358,
        D_L17_001B0354 * 0.0000048481374f, D_L17_001B0354 * 0.0000048481374f, 0.20943955f, 0);
    if (!done) {
        if (!func_002E59B0(obj, *effect) && D_L17_001DBF82 != 255)
            *effect = func_002E5DA0(0, 4, obj, 0x922);
    } else {
        int index = *effect;
        if (index != -1) {
            char *slot = D_00188660 + index * 0x70;
            if (*(char **)(slot + 0x88) == obj && *(unsigned char *)(slot + 0x74))
                func_002E59F8(index);
        }
        *effect = -1;
    }
    return done;
}
