/* NON_MATCHING func_L18_002EC290 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: SIZE ours 2896 / retail 2904, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Remaining differences (seen via --diff on p2..p6):
 *   - retail spills ~9 pointer pseudos (F4..114) in one block before loop 1 and keeps p/q ($17/$20) in regs until 
 *   ours spills p/q early and keeps more in regs (retail has a bc1f with `lw $3,0x114` in the slot, ours bc1fl+div
 *   - retail never hoists lui/addiu of D_L18_001D9FC0 in loop 1 (recomputes %hi per use, plus a separate giv $30=i
 *   ours hoists the base into a saved reg. The alias-symbol trick (p6) did not reproduce it.
 *   - retail keeps (float)i twice: `mov.s $f24,$f20` copy (written as two (float)i in p7).
 *   Likely allocator/scheduling ties; not resolved.
 *   Declarations: gp floats as `extern short X` read via *(float*)&X; D_L18_001620E4 stored as int (already short 
 */
typedef struct { float f[4]; } __attribute__((aligned(16))) V_ec290;
typedef struct { V_ec290 a, b, c, d, e; } S_ec290;

extern float func_001FA748(float, float);
extern float func_001FA790(float, float);
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_002156E0(void *dst, void *vec, void *axis, float angle);
extern void func_001F9C30(void *, void *, float);
extern float func_001FA888(int);
extern float func_L00_0025F368(float);
extern float func_001F9FA8(float);
extern float func_L00_00258C80(float lo, float hi);
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9C08(void *, void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9F90(float);
extern int func_001F9938(void *);
extern int func_001F9850(int);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern void func_L00_00272F00(void *, int, int, int, int, void *, float, float, float);
extern char D_0013E633[];
extern char D_0013F6E0[];
extern short D_L18_00162098;
extern short D_L18_0016209C;
extern short D_L18_001620A0;
extern short D_L18_001620A4;
extern short D_L18_001620A8;
extern short D_L18_001620AC;
extern short D_L18_001620B0;
extern short D_L18_001620B4;
extern short D_L18_001620B8;
extern short D_L18_001620BC;
extern short D_L18_0016208C;
extern short D_L18_00162090;
extern short D_L18_00162094;
extern short D_L18_001620D0;
extern short D_L18_001620D4;
extern short D_L18_001620E4;
extern short D_L18_00162114;
extern short D_L18_0016211C;
extern float D_0015EE60 MACRO_ADDR;
extern V_ec290 D_L18_001D9FC0[];
extern V_ec290 D_L18_001DA2A0[];
extern float D_L18_001DA250[];
extern float D_L18_001DA3E0[];
extern S_ec290 D_L18_001DA420[];
extern short D_L18_00162148[];
extern short D_L18_00162150[];
extern short D_L18_00162158[];
extern short D_L18_00162160[];
extern float D_L18_00167840[];
extern char D_L18_0015F660[];

void func_L18_002EC290(char *moby) {
    V_ec290 s[15];
    unsigned char flag = *(unsigned char *)D_0013E633;
    char *data = *(char **)(moby + 0x78);
    char *p;
    char *q;
    float scale;
    int i;
    int k;
    V_ec290 *cur = &D_L18_001D9FC0[1];

    *(float *)&D_L18_00162098 = func_001FA748(*(float *)&D_L18_00162098, *(float *)&D_L18_0016209C);
    *(float *)&D_L18_001620A0 = func_001FA790(*(float *)&D_L18_001620A0, *(float *)&D_L18_001620A4);
    *(float *)&D_L18_001620B4 = func_001FA748(*(float *)&D_L18_001620B4, *(float *)&D_L18_001620B8);
    scale = func_001F9D10(moby + 0x10, data + 0x10) * 0.05f;
    if (flag) {
        *(int *)&D_L18_001620E4 = 0x7F2040;
    } else {
        *(int *)&D_L18_001620E4 = 0x7F2020;
    }
    q = data + 0x10;
    p = moby + 0x10;
    func_001F9BF0(&s[0], q, p);
    func_L00_001FF4B0(&s[0], &s[0], 1.0f);
    func_001F9CA0(&s[1], &s[0], D_0013F6E0);
    func_002156E0(&s[0], &s[0], &s[1], 0.17453292f);
    func_001F9C30(&s[3], &s[0], scale);
    qcopy(&D_L18_001D9FC0[1], p);
    qcopy(&D_L18_001D9FC0[21], p);
    for (i = 1; i < 20; i++) {
        float f21;
        float len;
        float len0;
        f21 = *(float *)&D_L18_00162090
              + (*(float *)&D_L18_00162094 - *(float *)&D_L18_00162090) * (func_001FA888(i) * 0.05f);
        f21 = f21 * func_001F9FA8(func_L00_0025F368(*(float *)&D_L18_00162098 + (float)i * *(float *)&D_L18_0016208C));
        f21 = f21 + func_L00_00258C80(*(float *)&D_L18_001620D0, *(float *)&D_L18_001620D4);
        D_L18_001D9FC0[i + 1].f[2] = D_L18_001D9FC0[i + 1].f[2] - D_L18_001DA250[i];
        D_L18_001DA250[i] = f21;
        {
            float a = func_00214158();
            float b = func_00214158();
            func_00215C00(&s[1], scale, a, b);
        }
        s[1].f[2] = s[1].f[2] * 0.5f;
        func_001F9C08(&s[2], &s[3], &s[1], *(float *)&D_L18_0016211C);
        len0 = func_001F9CB8(&s[2]);
        if (len0 == 0.0f) {
            qcopy(&D_L18_001D9FC0[i + 1], cur);
        } else {
            func_001F9C30(&s[2], &s[2], scale / len0);
            func_001F9BD8(&D_L18_001D9FC0[i + 1], &D_L18_001D9FC0[i], &s[2]);
            qcopy(&s[3], &s[2]);
        }
        D_L18_001D9FC0[i + 1].f[2] = D_L18_001D9FC0[i + 1].f[2] + f21;
        func_001F9BF0(&s[1], &D_L18_001D9FC0[i + 1], &D_L18_001D9FC0[i]);
        func_001F9BF0(&s[5], q, &D_L18_001D9FC0[i + 1]);
        len = func_001F9CB8(&s[5]);
        if (len != 0.0f) {
            float w;
            if (i == 19) {
                w = 0.5f;
                scale = len;
            } else if (i >= 16) {
                w = 0.5f;
                scale = len / func_001FA888(19 - i);
            } else {
                w = 0.1f;
            }
            func_001F9C30(&s[5], &s[5], scale / len);
            func_001F9C08(&s[3], &s[3], &s[5], w);
        }
        if (i < 10) {
            float g;
            float h;
            g = func_L00_0025F368(*(float *)&D_L18_001620A0 + (float)i * *(float *)&D_L18_001620A8);
            h = *(float *)&D_L18_001620AC
                * func_001F9FA8(func_L00_0025F368(*(float *)&D_L18_001620B4 + (float)i * *(float *)&D_L18_001620BC));
            qcopy(&D_L18_001D9FC0[i + 21], &D_L18_001D9FC0[i + 1]);
            h = h * func_001F9FA8(g);
            h = h + func_L00_00258C80(0.0f, 0.1f);
            D_L18_001D9FC0[i + 21].f[2] = D_L18_001D9FC0[i + 21].f[2] + h;
            D_L18_001DA3E0[i] = h;
            {
                float ang = func_001FA748(func_L00_001FF860(s[1].f[0], s[1].f[1]), 1.5707964f);
                float m = *(float *)&D_L18_001620B0 * func_001F9F90(g);
                s[4].f[0] = func_001F9F90(ang) * m;
                s[4].f[1] = func_001F9FA8(ang) * m;
                s[4].f[2] = 0.0f;
            }
            qcopy(&D_L18_001DA2A0[i], &s[4]);
            func_001F9BD8(&D_L18_001D9FC0[i + 21], &D_L18_001D9FC0[i + 21], &s[4]);
        } else {
            int r = 21 - i;
            qcopy(&D_L18_001D9FC0[i + 21], &D_L18_001D9FC0[i + 1]);
            D_L18_001D9FC0[i + 21].f[2] = D_L18_001D9FC0[i + 21].f[2] + D_L18_001DA3E0[r];
            func_001F9BD8(&D_L18_001D9FC0[i + 21], &D_L18_001D9FC0[i + 21], &D_L18_001DA2A0[r]);
            qcopy(&s[4], &D_L18_001DA2A0[i]);
        }
        cur++;
    }
    func_001F9CA0(&s[6], &s[0], D_0013F6E0);
    func_L00_001FF4B0(&s[6], &s[6], 1.0f);
    func_L00_001FF4B0(&s[7], D_0013F6E0, 1.0f);
    k = 0;
    do {
        float t;
        S_ec290 *e;
        if (k < 2) {
            qcopy(&s[8], p);
            t = 0.4f;
        } else {
            qcopy(&s[8], q);
            t = 0.5f;
        }
        if (func_001F9938(&D_L18_00162148[k])) {
            float f23;
            float f20;
            V_ec290 *c;
            V_ec290 *d;
            int j;
            D_L18_00162148[k] = func_001F9850(D_L18_00162158[k]);
            D_L18_00162150[k] = 0;
            f23 = func_L00_00258C80(0.5245f, 1.0475f);
            f20 = func_002140F8(-1.0475f, 0.17453292f);
            func_001F9C30(&s[5], &s[0], t);
            func_002156E0(&s[5], &s[5], &s[6], f20);
            func_002156E0(&s[5], &s[5], &s[7], f23);
            e = &D_L18_001DA420[k];
            qcopy(&e->b, &s[8]);
            func_001F9BD8(&e->c, &s[8], &s[5]);
            t = 1.0f;
            c = &e->c;
            d = &e->d;
            for (j = 2; j >= 0; j--) {
                float u;
                func_001F9BF0(&s[9], c, D_L18_00167840);
                u = func_002140F8(0.17453292f, 0.9609f) * t;
                t = -t;
                func_002156E0(&s[5], &s[5], &s[9], u);
                func_001F9BD8(d, c, &s[5]);
                d++;
                c++;
            }
        } else {
            V_ec290 *w = &s[10];
            V_ec290 *b;
            V_ec290 *c;
            int j;
            b = &D_L18_001DA420[k].b;
            c = &D_L18_001DA420[k].c;
            for (j = 3; j >= 0; j--) {
                func_001F9BF0(&w[1 + (3 - j)], c, b);
                c++;
                b++;
                w[1 + (3 - j)].f[0] += func_L00_00258C80(0.0f, 0.1f);
                w[1 + (3 - j)].f[1] += func_L00_00258C80(0.0f, 0.1f);
                w[1 + (3 - j)].f[2] += func_L00_00258C80(0.0f, 0.1f);
            }
            qcopy(&D_L18_001DA420[k].b, &s[8]);
            b = &D_L18_001DA420[k].b;
            c = &D_L18_001DA420[k].c;
            for (j = 3; j >= 0; j--) {
                func_001F9BD8(c, b, &w[1 + (3 - j)]);
                b++;
                c++;
            }
            {
                int x = D_L18_00162160[k];
                int y = D_L18_00162148[k];
                float fr;
                if (x < y) {
                    x = y - x;
                } else {
                    x = x - y;
                }
                fr = func_001FA888(x);
                fr = fr / func_001FA888(func_001F9850(15));
                D_L18_00162150[k] = func_001FA898_r((1.0f - fr) * 32.0f);
            }
        }
        k++;
    } while (k < 4);
    {
        float f23 = func_L00_00258C80(0.5245f, 1.0475f);
        float f20 = func_002140F8(-0.5245f, 1.0475f);
        int r;
        func_001F9C30(&s[5], &s[0], *(float *)&D_L18_00162114 * D_0015EE60);
        func_002156E0(&s[5], &s[5], &s[6], f20);
        func_002156E0(&s[5], &s[5], &s[7], f23);
        r = func_L00_00258BC8(1, 8);
        if (func_002140B0(2)) r = -r;
        f20 = func_002140F8(0.2f, 0.6f);
        func_L00_00272F00(p, func_001F9850(15), *(int *)&D_L18_001620E4 | 0x7F000000, 0, r, &s[5], f20 * 0.3f, f20, 0.0f);
        r = -r;
        func_L00_00272F00(p, func_001F9850(15), 0x307F7F7F, 0, r, &s[5], f20 * 0.15f, f20 * 0.5f, 0.0f);
        r = func_L00_00258BC8(1, 8);
        if (func_002140B0(2)) r = -r;
        f20 = func_002140F8(0.15f, 3.0f);
        func_L00_00272F00(q, func_001F9850(12), *(int *)&D_L18_001620E4 | 0x7F000000, 0, r, D_L18_0015F660, f20 * 0.1f, f20, 0.0f);
    }
}
