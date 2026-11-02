#include "include_asm.h"

/* func_L23_00427CE8 is a unit of its own: it reads $gp data (-G8) and needs the opt-in
   quadword-copy profile and its scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier,
   config/cflags.txt), which give retail's grouped `lq`/`sq` with register addresses and keep
   the instructions around those copies on their own side of them. The copy into vecB goes
   through a TI16 temporary, which keeps it the plain displaced `lq`/`sq` retail has there. */

typedef unsigned char u8;

typedef unsigned short u16;

typedef int s32;

typedef int TI16 __attribute__((mode(TI)));

extern void func_002A7750(void *, s32, s32);

extern s32 func_L00_0035F5A0(s32, s32, s32);

extern void func_0029F338(u8 *object);

/* the probe record func_002A8C00 fills: only its tail bytes are set by the caller */
typedef struct {
    u8 pad0[0x18];
    u8 b18;
    u8 b19;
    u16 id;
    u8 pad1C[0x14];
} L23Probe;

/* the game block at 0x189E20: the camera vector at +0x80 and the player object at +0x2290 */
typedef struct {
    u8 pad0[0x80];
    u8 camera[0x10];
    u8 pad90[0x2290 - 0x90];
    u8 *player;
} L23Game;

/* the last ray hit: the object struck at +0x18 and the point at +0x20 */
typedef struct {
    u8 pad0[0x18];
    u8 *hit;
    u8 pad1C[4];
    float point[4];
} L23RayHit;

/* the flight state at object + 0x68 */
typedef struct {
    float dir[4];
    u8 pad10[4];
    float radius;
    u8 pad18[0xC];
    s32 frames;
    u8 pad28[8];
    float vel[4];
} L23Flight;

extern L23Game D_00189E20_2;

extern L23RayHit D_L23_001C0A80;

/* sda on this 16-byte array keeps its address one unsplit `la` (the assembler's lui/addiu
   pair), which no delay slot takes: retail's `beql` after func_002761F8 then fills from the
   no-hit arm */
extern float D_L23_001B36C0[4] __attribute__((sda));

extern float D_L23_001B1A10 __attribute__((sda));

extern float D_L23_001B1A18 __attribute__((sda));

extern float D_L23_001B1A1C __attribute__((sda));

extern float D_L23_001B1A24 __attribute__((sda));

extern float D_L23_001B1A28 __attribute__((sda));

extern float D_L23_001B1A30 __attribute__((sda));

extern float D_L23_001B1A34 __attribute__((sda));

extern float D_L23_001B1A3C __attribute__((sda));

extern float D_L23_001B1A40 __attribute__((sda));

extern float D_L23_001B1ADC __attribute__((sda));

extern void func_00282CC0(void *, void *, void *);

extern void func_00282D30(void *, void *, float);

extern void func_00282C88(void *);

extern float func_00282E48(void *, void *);

extern void func_002A8C00(void *, u8 *, s32, void *, float);

extern s32 func_002761F8(void *, void *, s32, void *, s32);

/* returns the spawned effect (`daddu $v0,$s4` before its return); with a void prototype the
   call keeps `$v0 = 1`'s stack store tied to the next `lq $v0`, and sched2 issues it early */
extern void *func_0032C700(void *, s32, s32, s32, s32, s32, s32, s32, float, float, s32, s32, s32, s32);

extern void func_L16_00406080(void *, s32, s32, s32, s32, s32, s32, s32, float, s32, s32);

extern void func_L23_0041E5B0(u8 *, s32, s32, void *, s32, s32);

extern void func_002ACEC0(void *, void *, void *, float);

extern void func_L00_002E6EF8(void *, void *);

extern void func_L12_004081F8(u8 *);

/* per-frame update of the object whose state at +0x68 holds a direction vector, a frame counter
   and a clamped velocity: it ends the object after 0xF0 frames; in state 0 it casts a ray along
   the direction and, on a hit, spawns the hit effects (the first arm when the player is struck
   or the hit is near, else a second probe) and leaves, otherwise it moves the object; state 1
   spawns the burst and removes it. It then integrates the velocity, turns two matrix rows and
   registers the next update. */
void func_L23_00427CE8(u8 *moby) {
    L23Flight *vars = *(L23Flight **)(moby + 0x68);
    TI16 target[1];
    L23Probe q1;
    TI16 vecA[1];
    TI16 vecB[1];
    L23Probe q2;

    if (moby[0xBC] == 0) {
        vars->frames += 1;
        if (vars->frames > 0xF0) {
            if (moby[0xBC] == 0) {
                func_002A7750(moby, 1, -1);
            }
        }
        switch (moby[0x20]) {
        case 0: {
            func_00282CC0(target, moby + 0x10, vars);
            func_002A8C00(&q1, moby, 0x90003, vars, vars->radius);
            q1.id = *(u16 *)(moby + 0xAA);
            if (func_002761F8(moby + 0x10, target, 0, moby, 0) != 0) {
                float dist = func_00282E48(D_L23_001B36C0, D_L23_001C0A80.point);
                u8 *player = D_00189E20_2.player;
                s32 isPlayer = D_L23_001C0A80.hit == player;

                if (isPlayer || dist < D_L23_001B1ADC) {
                    TI16 t;
                    s32 radius;

                    func_00282D30(vecA, player + 0xE0, 3.5f);
                    func_00282CC0(vecA, vecA, D_00189E20_2.camera);
                    func_0032C700(vecA, 1, 5, 5, 5, 0x19, 0x4B, 0, 1.5f, 20.0f, 0, 1, 0, 0);
                    radius = (s32)vars->radius;
                    t = *(TI16 *)(moby + 0x10);
                    vecB[0] = t;
                    func_L23_0041E5B0(moby, 0x10007, radius, vecB, 0, 0);
                    if (isPlayer) {
                        func_L00_0035F5A0(0x20, 0, 0);
                    }
                    if (moby[0xBC] == 0) {
                        func_002A7750(moby, 1, -1);
                    }
                } else {
                    vecA[0] = *(TI16 *)vars;
                    func_00282D30(vecA, vecA, 3.0f);
                    ((float *)vecA)[2] = 1.0f;
                    ((float *)vecA)[3] = 5627.9248046875f;
                    func_002A8C00(&q2, moby, 0x90003, vecA, vars->radius);
                    q2.b19 = 1;
                    q2.id = *(u16 *)(moby + 0xAA);
                    q2.b18 = 0;
                    if (moby[0xBC] == 0) {
                        func_002A7750(moby, 1, -1);
                    }
                }
                return;
            }
            if (moby[0xBC] == 0) {
                *(TI16 *)(moby + 0x10) = target[0];
            }
            func_00282D30(vecA, vars, -0.5f);
            func_00282C88(vecB);
            break;
        }
        case 1:
            func_L16_00406080(moby + 0x10, 0, 0x19, 0x19, 0x19, 0x19, 0, 0, 4.5f, 1, 0);
            func_0029F338(moby);
            return;
        }
    }
    vars->vel[0] += D_L23_001B1A18;
    vars->vel[1] += D_L23_001B1A30;
    vars->vel[2] += D_L23_001B1A3C;
    vars->vel[3] += D_L23_001B1A24;
    if (vars->vel[0] > D_L23_001B1A10) {
        vars->vel[0] = D_L23_001B1A10;
    }
    if (vars->vel[1] > D_L23_001B1A28) {
        vars->vel[1] = D_L23_001B1A28;
    }
    if (vars->vel[2] > D_L23_001B1A34) {
        vars->vel[2] = D_L23_001B1A34;
    }
    if (vars->vel[3] > D_L23_001B1A1C) {
        vars->vel[3] = D_L23_001B1A1C;
    }
    func_002ACEC0(moby + 0xE0, moby + 0xE0, moby + 0xD0, D_L23_001B1A40 * 0.017453294f);
    func_002ACEC0(moby + 0xC0, moby + 0xC0, moby + 0xD0, D_L23_001B1A40 * 0.017453294f);
    func_L00_002E6EF8((void *)func_L12_004081F8, moby);
}
