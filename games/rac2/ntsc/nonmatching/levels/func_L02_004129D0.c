/* GPT-6.1 Sol (high), 29 October 2026: 892/892 bytes, 0/1 places
 * in USA v1.01 and USA v2.00. PAL has no catalogue row.
 * Profile: -O2 -G0 -ffunction-sections, current worker toolchain.
 * Indexed first-loop input with a walking output pointer recovers the complete
 * retail saved-register allocation. Separate loop indices are required.
 * Remaining: +0x184/+0x188 exchange addiu s1,sp,0x80 and daddu s0,s5,zero.
 * All other non-relocation instructions agree; relocation binding and linked
 * image equality remain unverified because check.py stops at that exchange.
 * Explicit indexed-input local and -g tie. Indexing both pointers adds a
 * separate W-store induction pointer (912 bytes); indexing only the output
 * restores the old register mismatch. Explicit persistent base is worse.
 * Reusing the loop index helps the older pointer-only draft, but is worse in
 * this indexed-input draft. Pointer-reset and countdown trials also worsen it.
 * Reopen with a supported source lifetime/scheduler explanation for that pair.
 * Declarations and approved helpers needed by this draft follow below.
 */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef struct {
    float x, y, z, w;
} Vec4;

struct F4 {
    float f[4];
};
struct F2 {
    float f[2];
};
struct W2 {
    u32 w[2];
};
struct B4 {
    u8 b[4];
};
struct B2 {
    u8 b[2];
};

extern const struct F4 D_L02_001B1C10;
extern const struct F2 D_L02_001B1C20;
extern const struct W2 D_L02_001B1C28;
extern const struct B4 D_L02_001B1C30;
extern const struct F2 D_L02_001B1C38;
extern const struct B2 D_L02_001B1C40;
extern const struct W2 D_L02_001B1C48;
extern u8 *D_L02_001B368C __attribute__((sda));
extern s32 D_L02_001A8F00 __attribute__((sda));

extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern float func_00283CE0(int);
extern void func_00282D30(void *, void *, float);
extern float func_00283198(float);
extern float func_00283B98(float, float);
extern int func_00283D38(int, int, float);
extern void func_L00_002EBE88(void *, int, int, int, int, int, float, float, float);
extern int func_002A77E0(int);
extern float func_002A7878(float, float);
extern int func_002A7820(int, int);
extern void func_00282D08(void *, void *, void *, float);
extern void func_002C3150(void *, void *, int, int, int, int, float);

void func_L02_004129D0(u8 *a0) {
    struct F4 a = D_L02_001B1C10;
    struct F2 b = D_L02_001B1C20;
    struct W2 c = D_L02_001B1C28;
    struct B4 d = D_L02_001B1C30;
    struct F2 e = D_L02_001B1C38;
    struct B2 f = D_L02_001B1C40;
    struct W2 g = D_L02_001B1C48;
    Vec4 buf[13] __attribute__((aligned(16)));
    Vec4 w __attribute__((aligned(16)));
    Vec4 v __attribute__((aligned(16)));
    Vec4 x __attribute__((aligned(16)));
    Vec4 y __attribute__((aligned(16)));
    Vec4 *in;
    Vec4 *out;
    u8 *state;
    float ph;
    float step;
    float amp;
    float t;
    int i;
    int j;
    int col0;
    int col1;
    int k;
    int n;

    amp = 1.0f;
    *(TI128 *)&v = 0;
    v.z = 1.5f;
    state = *(u8 **)(a0 + 0x68);
    func_00283098(&v, &v, a0 + 0xC0);
    func_00282CC0(&v, &v, a0 + 0x10);
    func_00282CF0(&w, D_L02_001B368C + (*(s32 *)state << 7) + 0x30, &v);
    func_00282D30(&w, &w, amp / func_00283CE0(12));
    qcopy(&buf[0], &v);
    out = buf + 1;
    for (i = 0; i < 12; i++) {
        func_00282CC0(out, &buf[i], &w);
        out->w = e.f[0];
        out++;
    }
    ph = *(float *)(state + 0x14);
    step = 1.0f / func_00283CE0(13) * 3.1415927f;
    for (j = 1; j < 13; j++) {
        Vec4 *q = &buf[j];

        t = func_00283198(ph);
        q->z += amp * t * func_00283198((float)j * step);
        ph = func_00283B98(ph, step + step);
    }
    col0 = func_00283D38(g.w[0] & 0xFFFFFF, g.w[0], *(float *)(state + 0xC));
    col1 = func_00283D38(g.w[1] & 0xFFFFFF, g.w[1], *(float *)(state + 0xC));
    func_L00_002EBE88(buf, f.b[0], f.b[1], 13, col0, col1, e.f[0], e.f[1],
                      *(float *)(state + 0x10));
    if (D_L02_001A8F00 == 0) {
        qzero_nc(&x);
        k = func_002A77E0(12);
        t = func_002A7878(0.0f, 1.0f);
        func_00282D08(&y, &buf[k], &buf[k + 1], t);
        x.z = -(func_002A7878(a.f[2], a.f[3]) * 0.016666668f);
        n = (u8)func_002A7820(d.b[2], d.b[3]);
        func_002C3150(&y, &x, c.w[1], n, (u8)func_002A77E0(255), 0, b.f[1]);
    }
}
