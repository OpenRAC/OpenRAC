#include "include_asm.h"

/* Big level-side array: keep the absolute materialisation (`lui+addiu`), never $gp. */
extern unsigned char D_L03_00189E20[] __attribute__((nosda));

extern float D_L03_001AEF64 __attribute__((sda));

extern int D_L03_001B2548;
extern int D_L03_001A9028;
extern int D_L03_001A9024;
extern int D_L03_001C63A8[] __attribute__((nosda));

extern void func_2EF020(unsigned char *, unsigned char *, float);

extern void func_2EEFB0(unsigned char *, unsigned char *, unsigned char *);

void func_L03_0038A9D0(unsigned char *a0, unsigned char *a1) {
    unsigned char *q = a1;
    unsigned char *p = a0;

    func_2EF020(q, p + 0xC0, D_L03_001AEF64);
    func_2EEFB0(q, q, p + 0x10);
}

extern int D_L03_001AEF60 __attribute__((sda));

extern float D_L03_00237F40[] __attribute__((nosda));

extern float func_31BAE8(float, float);

void func_L03_0038B238(void) {
    float *p;
    int i;

    if (D_L03_001AEF60 >= D_L03_001B2548)
        return;
    p = D_L03_00237F40;
    for (i = 15; i >= 0; i--) {
        p[0] = func_31BAE8(0.0f, 1.0f);
        p[1] = func_31BAE8(0.0f, 1.0f);
        p[2] = func_31BAE8(0.0f, 1.0f);
        p[3] = 1.0f;
        p += 4;
    }
    D_L03_001AEF60 = D_L03_001B2548;
}

void func_L03_0032AD10(int a0) {
    if (D_L03_001B2548 != D_L03_001A9028) {
        D_L03_001A9028 = D_L03_001B2548;
        D_L03_001A9024 = 0;
    }
    if (D_L03_001A9024 < 0x20) {
        D_L03_001C63A8[D_L03_001A9024] = a0;
        D_L03_001A9024 = D_L03_001A9024 + 1;
    }
}

/* A 16-byte vector: the engine's VU0 helpers take and return these. */
typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

extern void func_00282CF0(void *, void *, void *);
extern void func_00283698(void *, void *);
extern void func_00283098(void *, void *, void *);
extern float func_00283248(float, float);
extern float func_00282E20(void *);

extern float D_L03_001B0040 __attribute__((sda));
extern float D_L03_001B0044 __attribute__((sda));
extern float D_L03_001B0048 __attribute__((sda));
extern float D_L03_001B004C __attribute__((sda));

/* Builds a vector with the engine's VU0 helpers, trims its z by the constant the state at
   0xAA names, and stores the two clamped, halved angles in the moby at 0x68. */
void func_L03_003C4A10(unsigned char *a0, void *a1) {
    Vec4 v;
    Vec4 buf[4];
    float f1;
    float f20;
    void *s2;

    if (*(short *)(a0 + 0xAA) == 0x93C) {
        if (a0[0x43] == 1)
            return;
    }

    s2 = *(void **)(a0 + 0x68);

    func_00282CF0(&v, a1, a0 + 0x10);
    func_00283698(buf, a0 + 0xC0);
    func_00283098(&v, &v, buf);

    switch (*(short *)(a0 + 0xAA)) {
    case 0x964:
        v.z -= D_L03_001B0040;
        break;
    case 0x93C:
        v.z -= D_L03_001B0044;
        break;
    case 0x940:
        v.z -= D_L03_001B0048;
        break;
    case 0xA59:
        v.z -= D_L03_001B004C;
        break;
    }

    f20 = func_00283248(v.x, v.y);
    f1 = -func_00283248(func_00282E20(&v), v.z);

    if (f20 > 1.5707964f)
        f20 = 1.5707964f;
    else if (f20 < -1.5707964f)
        f20 = -1.5707964f;
    if (f1 > 0.7853982f)
        f1 = 0.7853982f;
    else if (f1 < -0.7853982f)
        f1 = -0.7853982f;

    f20 = f20 * 0.5f;
    f1 = f1 * 0.5f;

    *(float *)(s2 + 0x2A8) = f20;
    *(float *)(s2 + 0x2A4) = f1;
    *(float *)(s2 + 0x324) = f1;
    *(float *)(s2 + 0x328) = f20;
}

/* The group count is read through $gp here; the group table is absolute and the entity base is
   the bare `lui`/`lw` pair (`sda` sized over 8 bytes in a -G8 unit). */
extern int D_L03_001B2A38 __attribute__((sda));

extern unsigned char *D_L03_001B2A1C[3] __attribute__((sda));

extern unsigned short *D_L03_001FB5A0[];

extern void func_00282C88(void *);

extern void func_00283410(void *, void *);

/* Walks the entities of the object's group (the others of its team at +0x95, not in state 0xFE or
   0xFD), puts each one's offset from the object into the frame that faces the point a1, and
   counts -1 when its y is then positive, +1 otherwise; returns the sum clamped to -5..5. */
int func_L03_003C58F0(unsigned char *a0, float *a1) {
    float v[4];
    float t[4];
    float m[16];
    int n;
    unsigned short *q;
    unsigned char *e;

    if (a0[0x21] == 0xFF)
        return 0;
    func_00282C88(v);
    n = 0;
    v[2] = func_00283248(a1[0] - *(float *)(a0 + 0x10), a1[1] - *(float *)(a0 + 0x14));
    func_00283410(m, v);
    func_00283698(m, m);
    for (q = D_L03_001FB5A0[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L03_001B2A38 && q != 0 &&
         ((q != 0 && (short)q[-1] >= 0) || q == D_L03_001FB5A0[a0[0x21]]);
         q++) {
        e = D_L03_001B2A1C[0] + ((*q & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (e[0x20] == 0xFE)
            continue;
        if (e[0x20] == 0xFD)
            continue;
        if (e == a0)
            continue;
        if (e[0x95] != a0[0x95])
            continue;
        func_00282CF0(t, e + 0x10, a0 + 0x10);
        func_00283098(t, t, m);
        if (t[1] > 0.0f)
            n--;
        else
            n++;
    }
    if (n >= 6)
        n = 5;
    else if (n <= -6)
        n = -5;
    return n;
}

extern float func_00283B98(float, float);
extern float func_00283198(float);
extern int func_00283D38(int, int, float);
extern void func_L00_002E6CF0(void *, void *);
extern void func_L03_003C4800(unsigned char *);

/* This function is in levels 3 and 14, which keep the data it reads at other addresses: none of
   it is named by an address. _00 is the level's jump table for the object kind (nine words), _01
   to _18 the pairs of ids and the scales of the kinds, _19 the frame counter, _20 to _22 the group
   walk's tables. */
extern void *ref_L03_003C4578_00[];
extern int ref_L03_003C4578_01 __attribute__((sda));
extern int ref_L03_003C4578_02[3] __attribute__((sda));
extern float ref_L03_003C4578_03 __attribute__((sda));
extern int ref_L03_003C4578_04 __attribute__((sda));
extern int ref_L03_003C4578_05[3] __attribute__((sda));
extern float ref_L03_003C4578_06 __attribute__((sda));
extern float ref_L03_003C4578_07 __attribute__((sda));
extern int ref_L03_003C4578_08 __attribute__((sda));
extern int ref_L03_003C4578_09[3] __attribute__((sda));
extern int ref_L03_003C4578_10 __attribute__((sda));
extern int ref_L03_003C4578_11[3] __attribute__((sda));
extern int ref_L03_003C4578_12 __attribute__((sda));
extern int ref_L03_003C4578_13[3] __attribute__((sda));
extern int ref_L03_003C4578_14 __attribute__((sda));
extern int ref_L03_003C4578_15[3] __attribute__((sda));
extern int ref_L03_003C4578_16 __attribute__((sda));
extern int ref_L03_003C4578_17[3] __attribute__((sda));
extern float ref_L03_003C4578_18 __attribute__((sda));
extern int ref_L03_003C4578_19;
extern unsigned short *ref_L03_003C4578_20[];
extern int ref_L03_003C4578_21 __attribute__((sda));
extern unsigned char *ref_L03_003C4578_22[3] __attribute__((sda));

/* The parts of the object and of its state block (at +0x68) that this function reads. */
typedef struct {
    unsigned char pad0[0x35C];
    float phase;
    unsigned char pad1[0x378 - 0x360];
    int stamp;
} L03State;

typedef struct {
    unsigned char pad0[0x21];
    unsigned char group;
    unsigned char pad1[0x31 - 0x22];
    unsigned char live;
    unsigned char pad2[0x68 - 0x32];
    L03State *state;
    unsigned char pad3[0x78 - 0x6C];
    int tint;
    unsigned char pad4[0x95 - 0x7C];
    unsigned char kind;
} L03Obj;

/* Per object kind (a0->kind: 1 and 7, 8, 2..6, else) it picks a pair of ids and a scale, turns
   the object's phase (the float at 0x35C of its state) by that scale, stores the result of
   func_00283D38 on the pair in a0->tint, and, when the state's stamp is not the frame counter,
   stamps the counter into the state of every entity of its group and registers
   func_L03_003C4800 for the object with func_L00_002E6CF0. The kind selects its arm through the
   level's jump table; `cases` only keeps the labels alive, and the labels are named after the
   kind that reaches them. */
void func_L03_003C4578(L03Obj *a0) {
    static void *cases[9] = {&&c0, &&c1, &&c2, &&c3, &&c4, &&c5, &&c6, &&c1, &&c8};
    L03State *s1 = a0->state;
    int pair[2];
    float scale;
    float t;
    float r;
    unsigned short *q;
    L03Obj *e;
    unsigned int kind;

    if (a0->live != 0) {
        kind = a0->kind;
        if (kind < 9)
            goto *ref_L03_003C4578_00[kind];
        goto c0;
c1:
        pair[0] = ref_L03_003C4578_01;
        pair[1] = ref_L03_003C4578_02[0];
        scale = ref_L03_003C4578_03 * 0.00029088825f;
        goto join;
c8:
        pair[0] = ref_L03_003C4578_04;
        pair[1] = ref_L03_003C4578_05[0];
        scale = ref_L03_003C4578_06 * 0.00029088825f;
        goto join;
c2:
        pair[0] = ref_L03_003C4578_04;
        pair[1] = ref_L03_003C4578_05[0];
        scale = ref_L03_003C4578_07 * 0.00029088825f;
        goto join;
c3:
        pair[0] = ref_L03_003C4578_08;
        pair[1] = ref_L03_003C4578_09[0];
        scale = ref_L03_003C4578_07 * 0.00029088825f;
        goto join;
c4:
        pair[0] = ref_L03_003C4578_10;
        pair[1] = ref_L03_003C4578_11[0];
        scale = ref_L03_003C4578_07 * 0.00029088825f;
        goto join;
c5:
        pair[0] = ref_L03_003C4578_12;
        pair[1] = ref_L03_003C4578_13[0];
        scale = ref_L03_003C4578_07 * 0.00029088825f;
        goto join;
c6:
        pair[0] = ref_L03_003C4578_14;
        pair[1] = ref_L03_003C4578_15[0];
        scale = ref_L03_003C4578_07 * 0.00029088825f;
        goto join;
c0:
        pair[0] = ref_L03_003C4578_16;
        pair[1] = ref_L03_003C4578_17[0];
        scale = ref_L03_003C4578_18 * 0.00029088825f;
join:
        t = func_00283B98(s1->phase, scale);
        s1->phase = t;
        r = func_00283198(t);
        a0->tint = func_00283D38(pair[0], pair[1], r * 0.5f + 0.5f);
        if (s1->stamp != ref_L03_003C4578_19) {
            for (q = ref_L03_003C4578_20[a0->group];
                 a0->group != 0xFF && a0->group <= ref_L03_003C4578_21 && q != 0 &&
                 ((q != 0 && (short)q[-1] >= 0) || q == ref_L03_003C4578_20[a0->group]);
                 q++) {
                e = (L03Obj *)(ref_L03_003C4578_22[0] + ((*q & 0x7FFF) << 8));
                if (e == 0)
                    break;
                e->state->stamp = ref_L03_003C4578_19;
            }
            func_L00_002E6CF0(func_L03_003C4800, a0);
        }
    }
}

extern float func_00282DF0(float *);

extern float func_00283248(float, float);

extern float func_00283B98(float, float);

/* The level's own copies of the helpers this function calls, at another address in every level
   that has them. */
extern void ref_L03_003C4240_00(unsigned char *, float);

extern void ref_L03_003C4240_01(unsigned char *, void *, int, float, float, float);

extern int ref_L03_003C4240_02(void *, void *, void *);

extern void ref_L03_003C4240_03(void *, void *);

/* Two level constants read through $gp: they sit in the resident image at another address in
   every level that has this function. */
extern float ref_L03_003C4240_04 __attribute__((sda));

extern float ref_L03_003C4240_05 __attribute__((sda));

/* Turns the object with its state: it stops when the state has no time left, otherwise it
   spreads the turn over the frame and lets the state and the object react to it. */
void func_L03_003C4240(unsigned char *a0) {
    unsigned char *s1 = *(unsigned char **)(a0 + 0x68);
    unsigned char *v;
    float f;
    float g;

    f = func_00282DF0((float *)(s1 + 0x1B0));
    if (f <= 0.0f)
        return;
    g = func_00282DF0((float *)(s1 + 0x1B0));
    if (g > 0.016666668f) {
        f = func_00283248(*(float *)(s1 + 0x1B0), *(float *)(s1 + 0x1B4));
        f = func_00283B98(f, 3.14159274f);
        ref_L03_003C4240_00(a0, f);
    }
    ref_L03_003C4240_01(a0, a0 + 0x10, 0x10, *(float *)(a0 + 0xF8),
                        ref_L03_003C4240_05 * 0.016666668f,
                        ref_L03_003C4240_04 * 0.00027777778f);
    ref_L03_003C4240_02(*(void **)(s1 + 0x220), a0 + 0x10, s1 + 0x368);
    v = *(unsigned char **)(s1 + 0x194);
    if (v != 0) {
        if (*(short *)(v + 0xAA) == 0x940 || *(short *)(v + 0xAA) == 0x93C ||
            *(short *)(v + 0xAA) == 0x964)
            ref_L03_003C4240_03(*(unsigned char **)(s1 + 0x194), a0 + 0x10);
    }
}
