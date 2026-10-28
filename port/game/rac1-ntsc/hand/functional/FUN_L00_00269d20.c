/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/overlays/shared/effect_particle_updates.c (games/rac1): func_L00_0026ABC0, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* carry, not from a `same` function or an `exact` data address (read them):
 *   0x00FFFFFF: no place found, left as it is
 */

typedef short s16_c00_269d20;
typedef int s32_c00_269d20;
typedef unsigned char u8_c00_269d20;
typedef unsigned int u32_c00_269d20;
typedef float f32_c00_269d20;
typedef u32_c00_269d20 VecQuad_c00_269d20 __attribute__((mode(TI), aligned(16)));
typedef union {
    VecQuad_c00_269d20 q;
    f32_c00_269d20 f[4];
    s32_c00_269d20 i[4];
} Vec4_c00_269d20;
extern void func_001F9A10(void *, void *, void *);
extern float func_001FA6C0(int);
extern int func_001F9770(void *);
extern void func_L00_00267A08(void *);
struct PartBase_c00_269d20 {
    u8_c00_269d20 unk_00;
    u8_c00_269d20 unk_01;
    u8_c00_269d20 texture;
    u8_c00_269d20 blend;
    s32_c00_269d20 color;
    u8_c00_269d20 spin;
    u8_c00_269d20 unk_09;
    s16_c00_269d20 timer;
    f32_c00_269d20 size;
    Vec4_c00_269d20 pos;
    u8_c00_269d20 data[0x40];
};
extern int func_L00_001EFC70(void *, void *, int, int, int);
extern float func_001FA6C0(int);
extern void func_001F9A10(void *, void *, void *);
extern int func_001F9770(void *);
extern void func_L00_00267A08(void *);
extern float D_0015ED60 ;
extern void func_L00_00267A08(void *);
extern void func_L00_0025C2B0(void *, void *, void *, float);
extern void func_L00_001FF500(void *, void *, float);
extern float func_001FA6C0(int);
extern int func_001FA6D0(float);
extern void func_001FA050(void *, void *);
extern void func_001F9D20(void *, void *, void *);
extern void func_L00_001FF598(void *, void *, float);
extern void func_L00_00267A08(void *);
extern f32_c00_269d20 D_L00_00173E80[];
typedef int u128_6AB_c00_269d20 __attribute__((mode(TI)));
void FUN_L00_00269d20(struct PartBase_c00_269d20 *m) {
    unsigned char *t;
    unsigned char *e;
    float tmp[4];
    float o[4];
    float v2[4];
    float v3[4];
    float v4[16];
    float k;
    float f22, f23, f24;
    int fl;
    int bb;
    int r4, r5, r6, r7;
    e = (unsigned char *)m->data;
    t = *(unsigned char **)(e + 0xC);
    tmp[0] = m->pos.f[0] + *(float *)((unsigned char *)m + 0x20);
    tmp[1] = m->pos.f[1] + *(float *)(e + 0x4);
    tmp[2] = m->pos.f[2] + *(float *)(e + 0x8);
    if ((*(int *)(t + 0x70) & 1) && func_L00_001EFC70(&m->pos, tmp, 4, (int)m, 0)) {
        v2[0] = *(float *)((unsigned char *)m + 0x20);
        v2[1] = *(float *)(e + 0x4);
        v2[2] = *(float *)(e + 0x8);
        func_L00_0025C2B0(o, v2, D_L00_00173E80, *(float *)(t + 0x3C));
        *(float *)((unsigned char *)m + 0x20) = o[0];
        *(float *)(e + 0x4) = o[1];
        *(float *)(e + 0x8) = o[2];
        func_L00_001FF500(v3, D_L00_00173E80, 0.055f);
        func_001F9A10(&m->pos, (char *)D_L00_00173E80 - 0x20, v3);
    } else {
        unsigned char *q;
        if (q) *(u128_6AB_c00_269d20 *)&m->pos = *(u128_6AB_c00_269d20 *)tmp;
        else *(u128_6AB_c00_269d20 *)&m->pos = *(u128_6AB_c00_269d20 *)tmp;
    }
    fl = *(int *)(t + 0x70);
    if (fl & 2) {
        f24 = func_001FA6C0((*(unsigned char *)&m->color << 8) | e[0x18]) / 65535.0f;
        f23 = (float)func_001FA6D0((float)((m->color & 0xFF00) | e[0x19])) / 65535.0f;
        f22 = (float)func_001FA6D0((float)((int)(((unsigned int)m->color >> 8) & 0xFF00) | e[0x1A])) / 65535.0f;
        if (e[0x1B] & 0x10) {
            f24 = f24 - *(float *)(t + 0x4C) * D_0015ED60;
            k = D_0015ED60;
        } else {
            f24 = f24 + *(float *)(t + 0x4C) * D_0015ED60;
            k = D_0015ED60;
        }
        if (e[0x1B] & 0x20) {
            f23 = f23 - *(float *)(t + 0x50) * k;
        } else {
            f23 = f23 + *(float *)(t + 0x50) * k;
        }
        if (e[0x1B] & 0x40) {
            f22 = f22 - *(float *)(t + 0x54) * D_0015ED60;
        } else {
            f22 = f22 + *(float *)(t + 0x54) * D_0015ED60;
        }
        if (*(float *)(t + 0x64) < f24) {
            f24 = *(float *)(t + 0x64);
            if (*(int *)(t + 0x70) & 0x100) e[0x1B] ^= 0x10;
        }
        if (f24 < *(float *)(t + 0x58)) {
            f24 = *(float *)(t + 0x58);
            if (*(int *)(t + 0x70) & 0x10) e[0x1B] ^= 0x10;
        }
        if (*(float *)(t + 0x68) < f23) {
            f23 = *(float *)(t + 0x68);
            if (*(int *)(t + 0x70) & 0x200) e[0x1B] ^= 0x20;
        }
        if (f23 < *(float *)(t + 0x5C)) {
            f23 = *(float *)(t + 0x5C);
            if (*(int *)(t + 0x70) & 0x20) e[0x1B] ^= 0x20;
        }
        if (*(float *)(t + 0x6C) < f22) {
            f22 = *(float *)(t + 0x6C);
            if (*(int *)(t + 0x70) & 0x400) e[0x1B] ^= 0x40;
        }
        if (f22 < *(float *)(t + 0x60)) {
            f22 = *(float *)(t + 0x60);
            if (*(int *)(t + 0x70) & 0x40) e[0x1B] ^= 0x40;
        }
        {
        float c65 = 65535.0f;
        float c255;
        if (*(unsigned char *)&m->color & 0x100) e[0x18] = 0;
        c255 = 255.0f;
        e[0x18] = func_001FA6D0(f24 * c65);
        e[0x19] = func_001FA6D0(f23 * c65);
        e[0x1A] = func_001FA6D0(f22 * c65);
        r4 = func_001FA6D0(f24 * c255);
        r5 = func_001FA6D0(f23 * c255);
        r6 = func_001FA6D0(f22 * c255);
        r7 = func_001FA6D0(*(float *)(e + 0x10) * c255);
        }
        m->color = (r4 & 0xFF) | ((r5 & 0xFF) << 8) | ((r6 & 0xFF) << 16) | (r7 << 24);
        goto dtest;
    }
    if (!(fl & 4)) goto b0b0;
    r7 = func_001FA6D0(*(float *)(e + 0x10) * 255.0f);
    m->color = (m->color & 0xFFFFFF) | (r7 << 24);
dtest:
    if (*(int *)(t + 0x70) & 4) {
        k = D_0015ED60;
        if (e[0x1B] & 0x80) {
            *(float *)(e + 0x10) = *(float *)(e + 0x10) - *(float *)(t + 0x38) * k;
        } else {
            *(float *)(e + 0x10) = *(float *)(e + 0x10) + *(float *)(t + 0x38) * k;
        }
        if (*(float *)(e + 0x10) > *(float *)(t + 0x48)) {
            *(float *)(e + 0x10) = *(float *)(t + 0x48);
            if (*(int *)(t + 0x70) & 0x800) e[0x1B] ^= 0x80;
        }
        if (*(float *)(e + 0x10) < *(float *)(t + 0x44)) {
            *(float *)(e + 0x10) = *(float *)(t + 0x44);
            if (*(int *)(t + 0x70) & 0x80) e[0x1B] ^= 0x80;
        }
    }
b0b0:
    m->spin = func_001FA6D0(*(float *)(e + 0x14) * 255.0f);
    if (*(int *)(t + 0x70) & 8) {
        tmp[0] = *(float *)(e + 0x0);
        tmp[1] = *(float *)(e + 0x4);
        tmp[2] = *(float *)(e + 0x8);
        if (*(int *)(t + 0x70) & 0x8000) {
            func_001FA050(v4, *(char **)(t + 0x7C) + 0x40);
            func_001F9D20(o, t, v4);
        } else {
            *(u128_6AB_c00_269d20 *)o = *(u128_6AB_c00_269d20 *)t;
        }
        func_001F9A10(tmp, tmp, o);
        func_L00_001FF598(tmp, tmp, *(float *)(t + 0x40));
        *(float *)(e + 0x0) = tmp[0];
        *(float *)(e + 0x4) = tmp[1];
        *(float *)(e + 0x8) = tmp[2];
    }
    m->size = m->size + *(float *)(t + 0x30) * D_0015ED60;
    if (m->size < *(float *)(t + 0x74)) m->size = *(float *)(t + 0x74);
    if (m->size > *(float *)(t + 0x78)) m->size = *(float *)(t + 0x78);
    *(float *)(e + 0x14) = *(float *)(e + 0x14) + *(float *)(t + 0x34) * D_0015ED60;
    if (*(float *)(e + 0x10) <= 0.0f || func_001F9770(&m->timer) || m->pos.f[0] < *(float *)(t + 0x10) || m->pos.f[1] < *(float *)(t + 0x14) || m->pos.f[2] < *(float *)(t + 0x18) || *(float *)(t + 0x20) < m->pos.f[0] || *(float *)(t + 0x24) < m->pos.f[1] || *(float *)(t + 0x28) < m->pos.f[2]) {
        func_L00_00267A08(m);
    }
}
extern float func_001FA6C0(int);
extern void func_L00_001FF500(void *, void *, float);
extern void func_L00_001FF500(void *, void *, float);
extern int func_001FA6D0(float);
extern void func_L00_001FF500(void *, void *, float);
extern void func_001F9D20(void *, void *, void *);
extern int func_001FA6D0(float);
extern void func_001F9A10(void *, void *, void *);
extern void func_L00_00267A08(void *);
extern f32_c00_269d20 D_0015ED60 ;
void func_001F9D20(void *, void *, void *);
void func_001F9A10(void *, void *, void *);
void func_L00_001FF500(void *, void *, f32_c00_269d20);
s32_c00_269d20 func_001F9770(void *);
void func_L00_00267A08(void *);
s32_c00_269d20 func_001FA6D0(f32_c00_269d20);
void func_001F9D20(void *, void *, void *);
void func_001F9A10(void *, void *, void *);
s32_c00_269d20 func_001FA6D0(f32_c00_269d20);
extern s32_c00_269d20 func_001F9770(void *);
extern s32_c00_269d20 func_001FA6D0(f32_c00_269d20);
extern void func_L00_001FF500(void *, void *, f32_c00_269d20);
void func_L00_001FF500(void *, void *, f32_c00_269d20);
void func_001F9A10(void *, void *, void *);
void func_001F9D20(void *, void *, void *);
s32_c00_269d20 func_001FA6D0(f32_c00_269d20);
extern float D_0015ED60 ;
extern int func_001F9770(void *);
extern void func_L00_00267A08(void *);
extern int func_L00_001EFC70(void *, void *, int, int, int);
extern int func_001FA6D0(float);
extern f32_c00_269d20 D_L00_00173E80[];
extern f32_c00_269d20 D_0015ED60 ;
extern void func_L00_001FF500(void *, void *, f32_c00_269d20);
extern f32_c00_269d20 func_001FA6C0(s32_c00_269d20);
extern s32_c00_269d20 func_001FA6D0(f32_c00_269d20);
