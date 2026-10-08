/* NON_MATCHING func_L18_002EC290 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 32/2904 (98.9% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   D_L18_001DA420 viewed as {hdr; p[4]} with &...p[j - 1] for the previous point (gives retail's kmul-first addu)
 *   - separate V d[5] array at sp+0xA0 for the jitter deltas; f20 reused for the rotation angle in the j loop; sca
 *   for both rand(pi/6, pi/3) results (fixes FP regs in loop 2 and tail); h = h * FA8(g) + rand(0, 0.1) as one exp
 *   - D_L18_00162148/50/58/60 as `extern short X[] MACRO_ADDR;` (retail's lui/addiu for them are always adjacent: 
 *   Left (32 bytes): loop-1 strength-reduction order: eq giv and DA3E0 giv spill slots swapped (0x110/0x114) and t
 *   giv's init/increment come after [i+21]/i16 instead of first; tail third spawn call: move $8,r scheduled before
 *   mov.s $f13,$f20 / lw $6. Tried without effect: eq as D9FD0 + i - 1, &D9FD0[i-1] (worse), cur biv (worse), prev
 *   assigned at loop end (worse), swapped symbol roles, array-ref forms, compound assignments, int MACRO_ADDR zero
 */
typedef struct { float f[4]; } __attribute__((aligned(16))) V_ec290;
typedef struct { V_ec290 p[5]; } P_ec290;

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
extern V_ec290 D_L18_001D9FD0[];
extern V_ec290 D_L18_001DA2A0[];
extern float D_L18_001DA250[];
extern float D_L18_001DA3E0[];
typedef struct { V_ec290 hdr; V_ec290 p[4]; } H_ec290;
extern H_ec290 D_L18_001DA420[];
extern P_ec290 D_L18_001DA430[];
extern short D_L18_00162148[] MACRO_ADDR;
extern short D_L18_00162150[] MACRO_ADDR;
extern short D_L18_00162158[] MACRO_ADDR;
extern short D_L18_00162160[] MACRO_ADDR;
extern float D_L18_00167840[];
extern float D_L18_0015F660_a[] __asm__("D_L18_0015F660") MACRO_ADDR;

/* Veldin 2 arc emitter: rebuilds the 20-point bolt and its mirror, regrows or jitters four side arcs, then spawns three particles. */
void func_L18_002EC290(void *moby) {
    V_ec290 s[10];
    V_ec290 d[5];
    unsigned char flag = *(unsigned char *)D_0013E633;
    char *data = *(char **)((char *)moby + 0x78);
    float scale;
    int i;
    int k;

    *(float *)&D_L18_00162098 = func_001FA748(*(float *)&D_L18_00162098, *(float *)&D_L18_0016209C);
    *(float *)&D_L18_001620A0 = func_001FA790(*(float *)&D_L18_001620A0, *(float *)&D_L18_001620A4);
    *(float *)&D_L18_001620B4 = func_001FA748(*(float *)&D_L18_001620B4, *(float *)&D_L18_001620B8);
    scale = func_001F9D10((char *)moby + 0x10, data + 0x10) * 0.05f;
    if (flag) {
        *(int *)&D_L18_001620E4 = 0x7F2040;
    } else {
        *(int *)&D_L18_001620E4 = 0x7F2020;
    }
    func_001F9BF0(&s[0], data + 0x10, (char *)moby + 0x10);
    func_L00_001FF4B0(&s[0], &s[0], 1.0f);
    func_001F9CA0(&s[1], &s[0], D_0013F6E0);
    func_002156E0(&s[0], &s[0], &s[1], 0.17453292f);
    func_001F9C30(&s[3], &s[0], scale);
    qcopy(&D_L18_001D9FD0[0], (char *)moby + 0x10);
    qcopy(&D_L18_001D9FD0[20], (char *)moby + 0x10);
    for (i = 1; i < 20; i++) {
        float f21;
        float len;
        float len0;
        f21 = *(float *)&D_L18_00162090
              + (*(float *)&D_L18_00162094 - *(float *)&D_L18_00162090) * (func_001FA888(i) * 0.05f);
        f21 = f21 * func_001F9FA8(func_L00_0025F368(*(float *)&D_L18_00162098 + (float)i * *(float *)&D_L18_0016208C));
        f21 = f21 + func_L00_00258C80(*(float *)&D_L18_001620D0, *(float *)&D_L18_001620D4);
        (D_L18_001D9FD0 + i)->f[2] = (D_L18_001D9FD0 + i)->f[2] - *(D_L18_001DA250 + i);
        *(D_L18_001DA250 + i) = f21;
        {
            float a = func_00214158();
            float b = func_00214158();
            func_00215C00(&s[1], scale, a, b);
        }
        s[1].f[2] = s[1].f[2] * 0.5f;
        func_001F9C08(&s[2], &s[3], &s[1], *(float *)&D_L18_0016211C);
        len0 = func_001F9CB8(&s[2]);
        if (len0 == 0.0f) {
            qcopy((D_L18_001D9FD0 + i), (D_L18_001D9FD0 + (i - 1)));
        } else {
            func_001F9C30(&s[2], &s[2], scale / len0);
            func_001F9BD8((D_L18_001D9FD0 + i), (D_L18_001D9FC0 + i), &s[2]);
            qcopy(&s[3], &s[2]);
        }
        (D_L18_001D9FD0 + i)->f[2] = (D_L18_001D9FD0 + i)->f[2] + f21;
        func_001F9BF0(&s[1], (D_L18_001D9FD0 + i), (D_L18_001D9FC0 + i));
        func_001F9BF0(&s[5], data + 0x10, (D_L18_001D9FD0 + i));
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
            qcopy((D_L18_001D9FD0 + (i + 20)), (D_L18_001D9FD0 + i));
            h = h * func_001F9FA8(g) + func_L00_00258C80(0.0f, 0.1f);
            D_L18_001D9FD0[i + 20].f[2] = D_L18_001D9FD0[i + 20].f[2] + h;
            D_L18_001DA3E0[i] = h;
            {
                float ang = func_001FA748(func_L00_001FF860(s[1].f[0], s[1].f[1]), 1.5707964f);
                float m = *(float *)&D_L18_001620B0 * func_001F9F90(g);
                s[4].f[0] = func_001F9F90(ang) * m;
                s[4].f[1] = func_001F9FA8(ang) * m;
                s[4].f[2] = 0.0f;
            }
            qcopy(&D_L18_001DA2A0[i], &s[4]);
            func_001F9BD8((D_L18_001D9FD0 + (i + 20)), (D_L18_001D9FD0 + (i + 20)), &s[4]);
        } else {
            int r = 21 - i;
            qcopy((D_L18_001D9FD0 + (i + 20)), (D_L18_001D9FD0 + i));
            D_L18_001D9FD0[i + 20].f[2] = D_L18_001D9FD0[i + 20].f[2] + *(D_L18_001DA3E0 + r);
            func_001F9BD8((D_L18_001D9FD0 + (i + 20)), (D_L18_001D9FD0 + (i + 20)), D_L18_001DA2A0 + r);
            qcopy(&s[4], &D_L18_001DA2A0[i]);
        }
    }
    func_001F9CA0(&s[6], &s[0], D_0013F6E0);
    func_L00_001FF4B0(&s[6], &s[6], 1.0f);
    func_L00_001FF4B0(&s[7], D_0013F6E0, 1.0f);
    for (k = 0; k < 4; k++) {
        float t;
        if (k < 2) {
            qcopy(&s[8], (char *)moby + 0x10);
            t = 0.4f;
        } else {
            qcopy(&s[8], data + 0x10);
            t = 0.5f;
        }
        if (func_001F9938(D_L18_00162148 + k)) {
            float f20;
            int j;
            D_L18_00162148[k] = func_001F9850(D_L18_00162158[k]);
            D_L18_00162150[k] = 0;
            scale = func_L00_00258C80(0.52359879f, 1.04719758f);
            f20 = func_002140F8(-1.04719758f, 0.17453292f);
            func_001F9C30(&s[5], &s[0], t);
            func_002156E0(&s[5], &s[5], &s[6], f20);
            func_002156E0(&s[5], &s[5], &s[7], scale);
            qcopy(&D_L18_001DA430[k].p[0], &s[8]);
            func_001F9BD8(&D_L18_001DA430[k].p[1], &s[8], &s[5]);
            t = 1.0f;
            for (j = 2; j < 5; j++) {
                func_001F9BF0(&s[9], &D_L18_001DA420[k].p[j - 1], D_L18_00167840);
                f20 = func_002140F8(0.17453292f, 0.959931076f) * t;
                t = -t;
                func_002156E0(&s[5], &s[5], &s[9], f20);
                func_001F9BD8((D_L18_001DA430[k].p + j), &D_L18_001DA420[k].p[j - 1], &s[5]);
            }
        } else {
            int j;
            for (j = 1; j < 5; j++) {
                func_001F9BF0(&d[j], (D_L18_001DA430[k].p + j), &D_L18_001DA420[k].p[j - 1]);
                d[j].f[0] += func_L00_00258C80(0.0f, 0.1f);
                d[j].f[1] += func_L00_00258C80(0.0f, 0.1f);
                d[j].f[2] += func_L00_00258C80(0.0f, 0.1f);
            }
            qcopy(&D_L18_001DA430[k].p[0], &s[8]);
            for (j = 1; j < 5; j++) {
                func_001F9BD8((D_L18_001DA430[k].p + j), &D_L18_001DA420[k].p[j - 1], &d[j]);
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
    }
    {
        float f20;
        int r;
        scale = func_L00_00258C80(0.52359879f, 1.04719758f);
        f20 = func_002140F8(-0.52359879f, 1.04719758f);
        func_001F9C30(&s[5], &s[0], *(float *)&D_L18_00162114 * D_0015EE60);
        func_002156E0(&s[5], &s[5], &s[6], f20);
        func_002156E0(&s[5], &s[5], &s[7], scale);
        r = func_L00_00258BC8(1, 8);
        if (func_002140B0(2)) r = -r;
        f20 = func_002140F8(0.2f, 0.6f);
        func_L00_00272F00((char *)moby + 0x10, func_001F9850(15), *(int *)&D_L18_001620E4 | 0x7F000000, 0, r, &s[5], f20 * 0.3f, f20, 0.0f);
        r = -r;
        func_L00_00272F00((char *)moby + 0x10, func_001F9850(15), 0x307F7F7F, 0, r, &s[5], f20 * 0.15f, f20 * 0.5f, 0.0f);
        r = func_L00_00258BC8(1, 8);
        if (func_002140B0(2)) r = -r;
        f20 = func_002140F8(0.15f, 3.0f);
        func_L00_00272F00(data + 0x10, func_001F9850(12), *(int *)&D_L18_001620E4 | 0x7F000000, 0, r, D_L18_0015F660_a, f20 * 0.1f, f20, 0.0f);
    }
}
