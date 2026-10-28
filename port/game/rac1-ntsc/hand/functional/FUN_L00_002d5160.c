/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/overlays/shared/crates_and_veldin_ship.c (games/rac1): func_L00_002D6610, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* carry, not from a `same` function or an `exact` data address (read them):
 *   0x00330000: no place found, left as it is
 *   D_L00_00161A40 -> 00161970: between
 *   D_L00_00161A44 -> 00161974: between
 *   D_L00_00161A48 -> 00161978: between
 *   D_L00_00161A4C -> 0016197C: between
 *   D_L00_00161A58 -> 00161988: between
 *   D_L00_00161A5C -> 0016198C: between
 */

#include "qcopy.h"
typedef int s32_c00_2d5160;
typedef unsigned char u8_c00_2d5160;
typedef float f32_c00_2d5160;

extern s32_c00_2d5160 func_00213260(s32_c00_2d5160);
extern f32_c00_2d5160 g_frame_time_r_c00_2d5160 __asm__("D_0015ED6C") ;
extern u8_c00_2d5160 * level_paths_r2_c00_2d5160[] __asm__("D_L00_001B04B0");
void func_L00_002584A8_2D19E8_c00_2d5160(u8_c00_2d5160 *, s32_c00_2d5160, s32_c00_2d5160) __asm__("func_L00_00257470");
f32_c00_2d5160 func_002140F8_2D19E8_c00_2d5160(f32_c00_2d5160, f32_c00_2d5160) __asm__("func_002132A8");
extern int func_00213260(int);
typedef struct { float x, y, z, w; } Vx_c00_2d5160 __attribute__((aligned(16)));
extern s32_c00_2d5160 cinematic_mode_r_c00_2d5160 __asm__("D_L00_0015F5C4") ;
extern char hero_globals_base_r_c00_2d5160[] __asm__("D_0013E533");
extern float g_frame_time_sq_r_c00_2d5160 __asm__("D_0015ED70") ;
extern s32_c00_2d5160 ticks_2D4398_c00_2d5160(s32_c00_2d5160) __asm__("func_001F9688");
extern void func_00212F90(void *, int, int, int);
extern int func_001F9770(void *);
extern int truncate_float_to_s32_r_r_c00_2d5160(float) __asm__("func_001FA6D0");
extern void FUN_L00_002d5160(void *);
extern int truncate_float_to_s32_r_r_c00_2d5160(float) __asm__("func_001FA6D0");
extern int func_00213260(int);
extern char *func_L00_0025A420(void *, int, int);
extern int func_L00_0025A478(void *, void *, void *, int, int *, float *, int, int);
extern void func_L00_0025AB48(void *, float *, void *, void *);
extern void func_L00_0025C558(void *, void *, int, int, int, float);
extern void func_L00_0025D458(void *, void *);
extern void func_L00_0025D538(void *, void *);
extern int func_L00_0025FF38(float, void *, void *, int, int, void *, int);
extern int func_001F9770(void *);
extern float func_001F96B0(float);
extern short D_L00_00161988;
extern short D_L00_0016198C;
extern char *func_L00_0025A420(void *, int, int);
extern int func_L00_0025A478(void *, void *, void *, int, int *, float *, int, int);
extern void func_L00_0025AB48(void *, float *, void *, void *);
extern void func_L00_0025C558(void *, void *, int, int, int, float);
extern void func_L00_0025D458(void *, void *);
extern void func_L00_0025D538(void *, void *);
extern int func_L00_0025FF38(float, void *, void *, int, int, void *, int);
extern int func_001F9770(void *);
extern float func_001F96B0(float);
typedef struct {
    char pad0[0x20];
    float hp;
    char pad24[0x5C];
    float kb_gravity;
    float kb_approach;
    float kb_forward;
    float kb_up;
    int kb_amount;
    int kb_mode;
    float kb_unk_98;
    char pad9C[0x11];
    unsigned char kb_unk_AD;
    char padAE[0xE];
    float kb_unk_BC;
} ToadVars_c00_2d5160;
extern float toad_kb_gravity_c00_2d5160 __asm__("D_L00_00161970" "__gp") ;
extern float toad_kb_approach_c00_2d5160 __asm__("D_L00_00161974" "__gp") ;
extern float toad_kb_forward_c00_2d5160 __asm__("D_L00_00161978" "__gp") ;
extern float toad_kb_up_c00_2d5160 __asm__("D_L00_0016197C" "__gp") ;
void FUN_L00_002d5160(void *a0) {
    char *a = (char *)a0;
    char *d;
    char *b;
    char *q;
    Vx_c00_2d5160 vec;
    int i10;
    float f14;
    float t18;
    float f20;
    float f0, f1, f2, f3, f5, f6;
    int r1, r2, v, k;
    unsigned short h;
    d = *(char **)(a + 0x78);
    if (*(unsigned char *)(a + 0x20) == 0)
        return;
    if (cinematic_mode_r_c00_2d5160 == 2) {
        a[0x31] = 0;
        *(unsigned short *)(a + 0x34) |= 1;
    } else {
        a[0x31] = 1;
        *(unsigned short *)(a + 0x34) &= 0xFFFE;
    }
    f20 = 0.0f;
    f14 = f20;
    b = func_L00_0025A420(a, 0x330000, 0);
    r1 = func_L00_0025A478(a, b, d + 0x20, 0, &i10, &f14, 0, 4);
    (void)r1;
    if (i10 != 1 && *(unsigned char *)(a + 0x20) != 12 && f14 != 0.0f) {
        char *q1;
        {
        ToadVars_c00_2d5160 *v = (ToadVars_c00_2d5160 *)d;
        v->hp -= f14;
        v->kb_amount = 512;
        v->kb_unk_98 = 0.75f;
        v->kb_gravity = toad_kb_gravity_c00_2d5160 * g_frame_time_sq_r_c00_2d5160;
        v->kb_approach = toad_kb_approach_c00_2d5160 * g_frame_time_sq_r_c00_2d5160;
        v->kb_forward = toad_kb_forward_c00_2d5160 * g_frame_time_r_c00_2d5160;
        v->kb_up = toad_kb_up_c00_2d5160 * g_frame_time_r_c00_2d5160;
        v->kb_unk_AD = 0;
        v->kb_mode = 9;
        v->kb_unk_BC = 2.0f * g_frame_time_r_c00_2d5160;
        v->kb_mode |= 0x20;
        }
        q1 = d + 0x70;
        *(unsigned short *)(a + 0x34) &= 0xEFFF;
        vec = *(Vx_c00_2d5160 *)(b + 0x10);
        func_L00_0025AB48(&vec, &t18, d + 0x88, d + 0x8C);
        f20 = t18;
        r2 = func_00213260(2);
        func_L00_0025C558(a, q1, r2 + 6, 1, 0, f20);
        {
            float c0 = *(float *)&D_L00_0016198C;
            float c4 = *(float *)&D_L00_00161988;
            *(float *)(d + 0xC0) = c0;
            *(float *)(d + 0xC4) = c4;
        }
        *(unsigned char *)(a + 0x20) = 12;
        *(unsigned char *)(d + 0x67) = 0x78;
        func_L00_0025D458(a, d + 0x60);
        func_L00_002584A8_2D19E8_c00_2d5160((u8_c00_2d5160 *)a, 0, -1);
    }
    *(unsigned char *)(a + 0xA4) = 0xFF;
    func_L00_0025D538(a, d + 0x60);
    if (*(unsigned char *)(a + 0x20) == 3) {
        char *p;
        q = d + 0x180;
        k = *(int *)(d + 0x1E4);
        p = level_paths_r2_c00_2d5160[k];
        func_L00_0025FF38(64.0f, a, q, 0, 0, p + 0x10, *(int *)p);
    } else {
        char *p;
        q = d + 0x180;
        k = *(int *)(d + 0x1E0);
        p = level_paths_r2_c00_2d5160[k];
        func_L00_0025FF38(24.0f, a, q, 0, 0, p + 0x10, *(int *)p);
    }
    v = *(int *)(d + 0x1C0);
    if (v == 0) {
        char *h = hero_globals_base_r_c00_2d5160 + 0xE1D;
        *(int *)(d + 0x1C0) = *(int *)(h + 0x2080);
        qcopy(q, h + 0x80);
    }
    func_001F9770(d + 0x276);
    if (*(int *)(d + 0x38) != 0) {
        *(short *)(d + 0x276) = (short)truncate_float_to_s32_r_r_c00_2d5160(
            func_001F96B0(func_002140F8_2D19E8_c00_2d5160(180.0f, 240.0f)));
    }
    *(int *)(d + 0x38) = 0;
    if (*(unsigned char *)(a + 0x20) == 5 || *(unsigned char *)(a + 0x20) == 8) {
        if (*(short *)(d + 0x276) != 0) {
            *(unsigned char *)(a + 0x20) = 10;
            if (*(unsigned char *)(a + 0x53) != 4) {
                func_00212F90(a, 4, 0, ticks_2D4398_c00_2d5160(20));
            }
        }
    }
}
extern int truncate_float_to_s32_r_r_c00_2d5160(float) __asm__("func_001FA6D0");
extern int truncate_float_to_s32_r_r_c00_2d5160(float) __asm__("func_001FA6D0");
int func_00213260(int);
