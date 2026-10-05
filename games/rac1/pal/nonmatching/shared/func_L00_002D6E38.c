/* NON_MATCHING func_L00_002D6E38 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: SIZE ours 1140 / retail 1116, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef unsigned int u128_2d5988 __attribute__((mode(TI), aligned(16)));
typedef union { u128_2d5988 q; float f[4]; } Vec4_2d5988;
typedef struct {
    u8 p0[0x60];
    float v60;
    float v64;
    float v68;
    float v6C;
} St_2d5988;
typedef struct {
    u8 p0[0x10];
    float v[3];
    u8 p1[0x20 - 0x1C];
    u8 flag;
    u8 p2[0x40 - 0x21];
    float f40;
    float f44;
    float f48;
    u8 p3[0x78 - 0x4C];
    St_2d5988 *st;
} Obj_2d5988;
typedef struct {
    s32 n;
    u8 pad[0x10 - 4];
    Vec4_2d5988 a[1];
    Vec4_2d5988 b[1];
} Path_2d5988;
typedef struct {
    u8 p0[0x80];
    float f80;
    float f84;
} Pl_2d5988;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE6C_D6E38b[2] __asm__("D_0015EE6C");
extern short D_L00_00161A60;
extern Pl_2d5988 D_0013F450;
extern void func_L00_0025EFC0(void *, void *, void *, s32 *, f32 *, s32, f32, f32, f32);
extern float func_L00_00259148(f32 *, f32, f32, f32, f32, f32);
extern float func_001F9C30(void *, void *, f32);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern float func_L00_001FF4B0_D6E38(void *, void *, f32) __asm__("func_L00_001FF4B0");
extern float func_001F9D10(void *, void *);
extern float func_001F9F90(f32);
extern float func_001F9FA8(f32);
extern float func_L00_001FF860(f32, f32);
extern float func_001FA790(f32, f32);
extern float func_001FA888(s32);
extern s32 func_001FA898(f32);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/unclassified_002cfcb8.c, FUN_L00_002d5988. */
void func_L00_002D6E38(Obj_2d5988 *o, Path_2d5988 *path, s32 p) {
    St_2d5988 *st;
    s32 idx;
    f32 t;
    Vec4_2d5988 v;
    Vec4_2d5988 w;
    f32 d;
    f32 w_;
    f32 c;
    f32 s;
    f32 l;
    f32 r;
    f32 a;
    f32 b;

    st = o->st;
    func_L00_0025EFC0(path, &o->v[0], &w, &idx, &t, 0, 999.0f, 5.0f, 0.0f);
    d = func_001FA888(idx) * path->a[0].f[3] + t;
    w_ = d + 2.0f;
    l = func_001F9D10(&o->v[0], (void *)((char *)path + path->n * 16));
    s = D_0015EE6C_D6E38b[1] * 16.0f;
    c = st->v60;
    if (l <= (c * c) / (s + s)) {
        f32 a1 = c - s;
        f32 b1 = (*(float *)&D_L00_00161A60) * D_0015EE6C;
        st->v60 = a1;
        if (a1 < b1) st->v60 = b1;
    } else {
        f32 a2 = c + s;
        f32 b2 = D_0015EE6C * 16.0f;
        st->v60 = a2;
        if (b2 < a2) st->v60 = b2;
    }
    idx = func_001FA898(w_ / path->a[0].f[3]);
    t = (w_ - func_001FA888(idx) * path->a[0].f[3]) / path->a[0].f[3];
    if (idx >= path->n - 1) {
        idx = path->n - 2;
        t = 1.0f;
    }
    func_001F9BF0(&w, &path->b[idx], &path->a[idx]);
    func_001F9C30(&w, &w, t);
    func_001F9BD8(&w, &w, &path->a[idx]);
    l = func_001F9D48((void *)((char *)path + path->n * 16), &o->v[0]);
    if (l < 1.0f) {
        if (st->v60 <= (*(float *)&D_L00_00161A60) * D_0015EE6C) {
            o->flag = p;
        }
    }
    if (idx + 1 < path->n - 8) {
        r = func_L00_001FF860(path->a[idx + 1].f[0] - o->v[0], path->a[idx + 1].f[1] - o->v[1]);
        o->f48 = func_L00_00259148(&st->v6C, o->f48, r, 0.01f, 0.3f, 0.1f);
    } else {
        r = func_L00_001FF860(D_0013F450.f80 - o->v[0], D_0013F450.f84 - o->v[1]);
        o->f48 = func_L00_00259148(&st->v6C, o->f48, r, 0.01f, 0.3f, 0.1f);
    }
    func_001F9BF0(&v, &w, &o->v[0]);
    l = func_001F9CB8(&v);
    if (st->v60 < l) l = st->v60;
    func_L00_001FF4B0_D6E38(&v, &v, l);
    func_001F9BD8(&o->v[0], &o->v[0], &v);
    r = func_001FA790(func_L00_001FF860(v.f[0], v.f[1]), o->f48);
    {
    f32 xa, xb;
    xa = st->v60 * 0.34906584f * func_001F9F90(r) / (D_0015EE6C * 16.0f);
    xb = st->v60 * -0.34906584f * func_001F9FA8(r) / (D_0015EE6C * 16.0f);
    { f32 q44, k70;
    q44 = func_L00_00259148(&st->v68, o->f44, xa, D_0015EE6C_D6E38b[1] * 0.52359879f, D_0015EE6C_D6E38b[1] * 1.04719758f, D_0015EE6C * 0.785398185f);
    k70 = D_0015EE6C_D6E38b[1];
    o->f44 = q44;
    o->f40 = func_L00_00259148(&st->v64, o->f40, xb, k70 * 1.04719758f, k70 * 2.09439516f, D_0015EE6C * 1.57079637f); }
    }
}
