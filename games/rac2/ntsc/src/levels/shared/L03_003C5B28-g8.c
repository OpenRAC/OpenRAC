#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;
typedef float f32;

typedef struct {
    u32 field00;
    u32 field04;
    f32 field08;
    f32 field0C;
} NativeCallbackTimer;

extern NativeCallbackTimer D_L04_001B98E0;
extern f32 D_L04_001B1C88 __attribute__((sda));

/* The group count is read through $gp here; the group table and the resident word are
   absolute, and the entity base (D_L03_001B2A1C in the -G0 sibling) is the bare `lui`/`lw` pair,
   a second name sized over 8 bytes with `sda` in this -G8 unit. */
extern int D_0018A15C __attribute__((nosda));

extern int D_L03_001B2A38 __attribute__((sda));

extern u8 *D_L03_001B2A1C_1[3] __attribute__((sda));

extern unsigned short *D_L03_001FB5A0[];

extern void func_00282CF0(float *, u8 *, u8 *);

extern void func_002AFDD8(u8 *, float *, float *, int);

extern float func_00282C48(float);

extern void func_002A7750(u8 *, int, int);

/* Puts into state 5 every entity of the object's group (state byte other than 1, kind byte
   below 2) that lies in front of it, within 1.0 ahead, 0.7 sideways and 0.5 up or down in the
   object's own frame, storing a1 at +0x374 of its state and clearing its halfword at +0x96. */
void func_L03_003C5B28(u8 *a0, float a1) {
    float v[4];
    unsigned short *p;
    u8 *e;
    u8 *s;

    if (a0[0x21] == 0xFF)
        return;
    for (p = D_L03_001FB5A0[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L03_001B2A38 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L03_001FB5A0[a0[0x21]]);
         p++) {
        e = D_L03_001B2A1C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (e[0x20] == 1)
            continue;
        if (e[0x95] < 2) {
            func_00282CF0(v, e + 0x10, a0 + 0x10);
            func_002AFDD8(a0, v, v, 0);
            if (v[0] > 0.0f && v[0] < 1.0f && func_00282C48(v[1]) < 0.7f &&
                func_00282C48(v[2]) < 0.5f) {
                s = *(u8 **)(e + 0x68);
                func_002A7750(e, 5, 1);
                *(float *)(s + 0x374) = a1;
                *(short *)(e + 0x96) = 0;
            }
        }
    }
}

/* Answers 1 when the resident word at 0x18A15C equals the object's state word at +0x358, or
   when the state of an entity of the object's group holds it at +0x198; 0 otherwise. */
int func_L03_003C5CE8(u8 *a0) {
    u8 *s = *(u8 **)(a0 + 0x68);
    unsigned short *p;
    u8 *e;

    if (D_0018A15C == *(int *)(s + 0x358))
        return 1;
    if (a0[0x21] == 0xFF)
        return 0;
    for (p = D_L03_001FB5A0[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L03_001B2A38 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L03_001FB5A0[a0[0x21]]);
         p++) {
        e = D_L03_001B2A1C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (*(int *)(*(u8 **)(e + 0x68) + 0x198) == *(int *)(s + 0x358))
            return 1;
    }
    return 0;
}

extern float func_00282E80(u8 *, u8 *);

/* Answers 0 when another entity of the object's group, whose state holds the same word at
   +0x358, is nearer to a1 than the object is; 1 otherwise. */
int func_L03_003C5DC8(u8 *a0, u8 *a1) {
    u8 *s = *(u8 **)(a0 + 0x68);
    float d;
    unsigned short *p;
    u8 *e;

    d = func_00282E80(a0 + 0x10, a1);
    for (p = D_L03_001FB5A0[a0[0x21]];
         a0[0x21] != 0xFF && a0[0x21] <= D_L03_001B2A38 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L03_001FB5A0[a0[0x21]]);
         p++) {
        e = D_L03_001B2A1C_1[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (e == a0)
            continue;
        if (*(int *)(*(u8 **)(e + 0x68) + 0x358) == *(int *)(s + 0x358) &&
            func_00282E80(e + 0x10, a1) < d)
            return 0;
    }
    return 1;
}

typedef struct { u8 b[0x10]; } Local16;

extern f32 D_L03_001B0344 __attribute__((sda));

extern f32 D_L03_001B0CA0 __attribute__((sda));

extern u8 D_L03_001B9BB0[] __attribute__((nosda));

extern void func_00282F20(u8 *, u8 *, f32);

extern void func_00282D30(u8 *, u8 *, f32);

extern void func_00282CC0(u8 *, u8 *, u8 *);

extern void func_002A0F48(u8 *);

void func_L03_003CAAF0(u8 *a0, u8 *a1, u8 *a2) {
    Local16 buf0;
    Local16 buf1;

    func_00282F20(buf0.b, a0 + 0xC0, D_L03_001B0344);
    func_00282D30(buf1.b, buf0.b, -1.0f);
    func_00282CC0(a1, buf0.b, a0 + 0x10);
    func_00282CC0(a2, buf1.b, a0 + 0x10);
}

void func_L03_003E50E0(u8 *a0) {
    u8 *q = *(u8 **)(a0 + 0x68);
    u8 *p = *(u8 **)(q + 0x58);

    func_00282F20(p + 0x10, D_L03_001B9BB0, D_L03_001B0CA0);
    func_00282CC0(p + 0x10, p + 0x10, D_L03_001B9BB0 - 0x230);
    if (*(u32 *)(p + 0x64) != 0)
        (*(void (**)(u8 *))(*(u8 **)(q + 0x58) + 0x64))(p);
    func_002A0F48(*(u8 **)(q + 0x58));
}

/* updates the registered countdown state and clears its original completion flags */
void func_L03_0043ACD8(void) {
    NativeCallbackTimer *state = &D_L04_001B98E0;
    u32 mode = state->field04;
    state->field08 = 10000.0f;
    if (mode == 0) {
        state->field0C -= D_L04_001B1C88;
        if (state->field0C < 0.0f) state->field0C = 0.0f;
    } else {
        state->field04 = 0;
    }
    state->field00 = 0;
}

/* The $gp float each of the two mutual twins reads (3.0f and 1.0f). */
extern f32 D_L03_001B02F0 __attribute__((sda));

extern f32 D_L03_001B1A90 __attribute__((sda));

void func_L03_003CA230(u8 *a0, u8 *a1) {
    func_00282D30(a1, a0 + 0xC0, D_L03_001B02F0);
    func_00282CC0(a1, a1, a0 + 0x10);
}

/* The same body with func_00282F20 and 1.0f. */
void func_L03_004197C8(u8 *a0, u8 *a1) {
    func_00282F20(a1, a0 + 0xC0, D_L03_001B1A90);
    func_00282CC0(a1, a1, a0 + 0x10);
}

extern f32 D_L03_001B1FC4 __attribute__((sda));

extern f32 func_00283B98(f32, f32);

extern void func_0029F338(u8 *);

/* Ages one entity: scales the rate the owner record holds by the shared $gp factor, drops
   the entity's own 1/240 timer, blends two matrix columns and then the three angles, and
   releases the entity when its 0xBC countdown runs out. */
void func_L03_0042B930(u8 *a0) {
    u8 *q = *(u8 **)(a0 + 0x68);
    u8 *r = *(u8 **)(a0 + 0x24);

    *(f32 *)(a0 + 0x2C) = *(f32 *)(r + 0x24) * D_L03_001B1FC4;
    *(f32 *)(q + 0x8) = *(f32 *)(q + 0x8) - 0.0041666669f;
    func_00282CC0(a0 + 0x10, a0 + 0x10, q);
    func_00282CC0(a0 + 0xF0, a0 + 0xF0, q + 0x10);
    *(f32 *)(a0 + 0xF0) = func_00283B98(*(f32 *)(a0 + 0xF0), *(f32 *)(q + 0x10));
    *(f32 *)(a0 + 0xF4) = func_00283B98(*(f32 *)(a0 + 0xF4), *(f32 *)(q + 0x14));
    *(f32 *)(a0 + 0xF8) = func_00283B98(*(f32 *)(a0 + 0xF8), *(f32 *)(q + 0x18));
    a0[0xBC] = a0[0xBC] - 1;
    if (a0[0xBC] == 0)
        func_0029F338(a0);
}

/* One sixteen-byte entry per path step: the step's type and the two nodes chosen for it. */
typedef struct {
    s32 type;
    s32 first;
    s32 second;
    s32 flag;
} PathEntry;

typedef struct {
    u8 order[16];
    u8 visited[16];
    u8 path[16];
} PathState;

extern PathEntry D_L03_0029BCD0[];

extern s32 D_L03_001B0B50 __attribute__((sda));

extern s32 func_L03_003E2D98(u8 *, u32, s32, u32 *, s32);

/* Clears the entries of the first D_L03_001B0B50 path steps and refills them by type, with
   func_L03_003E2D98 (a neighbour of a node that is not on the path): type 1 picks one
   neighbour of the step's path node, type 2 also a neighbour of that pick, type 3 a second
   neighbour of the path node other than the first. */
void func_L03_003E2C08(u8 *p) {
    PathState *q = *(PathState **)(p + 0x68);
    s32 i;
    u32 node, prev, prev2;

    for (i = 0; i < D_L03_001B0B50; i++) {
        D_L03_0029BCD0[i].first = -1;
        D_L03_0029BCD0[i].second = -1;
        D_L03_0029BCD0[i].flag = -1;
        node = q->path[i];
        switch (D_L03_0029BCD0[i].type) {
        case 1:
            D_L03_0029BCD0[i].first = func_L03_003E2D98(p, node, i, 0, 0);
            break;
        case 2:
            D_L03_0029BCD0[i].first = func_L03_003E2D98(p, node, i, 0, 0);
            prev = D_L03_0029BCD0[i].first;
            D_L03_0029BCD0[i].second = func_L03_003E2D98(p, prev, i, &prev, 1);
            break;
        case 3:
            D_L03_0029BCD0[i].first = func_L03_003E2D98(p, node, i, 0, 0);
            prev2 = D_L03_0029BCD0[i].first;
            D_L03_0029BCD0[i].second = func_L03_003E2D98(p, node, i, &prev2, 1);
            break;
        }
    }
}
