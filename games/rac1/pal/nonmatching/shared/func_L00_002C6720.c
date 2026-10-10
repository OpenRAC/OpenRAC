/* NON_MATCHING func_L00_002C6720 -- src/overlays/shared/vendor_002C12B0.c
 * Best so far: SIZE ours 2060 / retail 2076, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   FxDebrisGroupUpdateB (shared, all levels, moby class 204/213/214 and more): the debris update with a jump tabl
 *   Best so far p5.c: compiles, 2044 bytes against retail 2076 (32 short), so about 8 instructions missing. Left: 
 *   Stopped at the budget: it needs the scheduling of the stores and the pointer copies, which the source form did
 *   Note from the worker (hq9/s01): a claim listing was saved as build-sn/try/.hq9_s01_claim2.txt, outside any fun
 */
extern char D_0013D50F[];
extern char D_0013DE6E[];
extern char D_L00_00173F70[];
extern char D_00173F60_alias[] __asm__("D_L00_00173F60");
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern int D_L00_00161908 MACRO_ADDR;
extern float func_001F9CB8(void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_001F9BC0(void *);
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001F9B88(float);
extern float func_001F9D48(void *, void *);
extern int func_L00_002346C0(int, int);
extern void func_L00_00264E28(int a, int b, int c);
extern int func_001F9B70(int);
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern float func_L00_00251468(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_0020D678(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9D10(void *, void *);
extern float func_001F9F90(float);

typedef struct { int a, b, c; unsigned short d; unsigned short e; int f, g; } RecU;
typedef struct { char pad[0x34]; short s34; short s36; char rest[0x4C - 0x38]; } RecX;

/* FX debris group update: drifts a debris piece along its path, spins it, and sets its state from the hit results. */
void func_L00_002C6720(unsigned char *moby) {
    char *data = *(char **)(moby + 0x78);
    float orig[4];
    float delta[4];
    float v;
    int st;
    func_001F9908((int *)(data + 0x20));
    st = moby[0x20];
    if (st == 1 || st == 4 || st == 3) {
        char *p = data + 0x10;
        char *q = moby + 0x10;
        if (func_001F9CB8(p) != 0.0f) {
            qcopy(orig, q);
            orig[2] += 0.5f;
            if (func_L00_001F10E0(0.400000006f, orig, 2, 0)) {
                qcopy(q, D_L00_00173F70);
                *(float *)(moby + 0x18) -= 0.5f;
            }
        }
        *(float *)(data + 0x18) -= D_0015EE70 * 4.9000001f;
        func_001F9BD8(delta, q, p);
        *(float *)(data + 0x18) -= D_0015EE70 * 4.9000001f;
        qcopy(orig, q);
        orig[2] += 0.1f;
        if (func_L00_001EFFF0(orig, delta, 2, 0, 0)) {
            func_001F9BC0(p);
            qcopy(q, D_00173F60_alias);
        } else {
            qcopy(q, delta);
        }
        st = moby[0x20];
    }
    if ((unsigned int)st < 5) {
        switch (st) {
        case 0:
            moby[0x20] = 1;
            break;
        case 1: {
            int r;
            char *base;
            char *p;
            int flag;
            int d8;
            int r16;
            int t;
            int *p21;
            v = (float)moby[0x23];
            r = func_001F9850(30);
            func_00214D28(&v, 128.0f, (float)(128 / r));
            moby[0x23] = func_001FA898_r(v);
            if (*(int *)(data + 0x20) > 0) {
                return;
            }
            p = data + 0x10;
            if (func_001F9CB8(p) == 0.0f) {
                base = D_0013E633 + 0xE1D;
                flag = 1;
                if (*(float *)(base + 0x228C) < func_001F9B88(*(float *)(moby + 0x18) - *(float *)(base + 0x88))) {
                    flag = 0;
                } else if (*(float *)(base + 0x2288) < func_001F9D48(moby + 0x10, base + 0x80)) {
                    flag = 0;
                }
                d8 = *(int *)(data + 8);
                p21 = (int *)(D_0013D50F + 0x21);
                r16 = p21[d8];
                t = r16 < ((RecU *)D_L00_001C43B0)[d8].e;
                if (!t) {
                    flag = 0;
                }
                if (*(float *)(moby + 0x18) < *(float *)(data + 0x28) - 2.0f) {
                    flag = 1;
                }
                if (*(int *)(base + 0x22A8) == 0) {
                    flag = 0;
                }
                if (flag) {
                    short sv;
                    func_L00_002346C0(d8, *(int *)data);
                    d8 = *(int *)(data + 8);
                    if (r16 < p21[d8]) {
                        int dd = p21[d8] - r16;
                        if (dd == 1) {
                            sv = ((RecX *)D_L00_00179BC0)[d8].s36;
                        } else {
                            sv = ((RecX *)D_L00_00179BC0)[d8].s34;
                        }
                        func_L00_00264E28(sv, dd, -1);
                    }
                    {
                        int k = *(int *)(data + 8);
                        ((int *)(D_0013DE6E + 0xA2))[k] += ((int *)(D_0013D50F + 0x21))[k] - r16;
                    }
                    moby[0x20] = 2;
                    *(float *)(moby + 0x2C) *= D_0015EE60 * 0.200000048f + 1.0f;
                    func_001F9BC0(p);
                    *(float *)(data + 0x24) = D_0015EE6C * 8.0f;
                    r16 = func_001F9B70(D_L00_0015F6B0 - D_L00_00161908);
                    if (func_001F9850(10) < r16) {
                        func_L00_0028EF68(0, 0, (int)moby, 0xD5);
                        D_L00_00161908 = D_L00_0015F6B0;
                    }
                } else {
                    if (moby[0x23] < 0xFF) {
                        moby[0x23] = moby[0x23] + 1;
                    }
                    if (func_L00_00251468(moby, *(void **)(base + 0x2080)) <= -0.200000003f) {
                        moby[0x20] = 3;
                        *(short *)(data + 6) = func_001F9850(60);
                    }
                }
            }
            break;
        }
        case 2: {
            float t24;
            char *hero;
            float f20;
            char *p = data + 0x10;
            char *q = moby + 0x10;
            func_00214D28((float *)(data + 0x24), 0.0f, D_0015EE70 * 24.0f);
            *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + *(float *)(data + 0x24);
            t24 = func_001F9CB8(p);
            func_00214D28(&t24, D_0015EE6C * 64.0f, D_0015EE70 * 24.0f);
            hero = D_0013E633 + 0xEED;
            func_001F9BF0(p, hero, q);
            if (*(float *)(moby + 0x2C) == 0.0f || func_001F9CB8(p) < t24) {
                func_0020D678(moby);
                return;
            }
            func_L00_001FF4B0(p, p, t24);
            func_001F9BD8(q, q, p);
            f20 = func_001F9D10(q, hero) / ((float)func_001F9850(30) * t24 + 0.01f);
            if (1.0f < f20) {
                f20 = 1.0f;
            } else if (f20 < 0.300000012f) {
                f20 = 0.300000012f;
            }
            {
                float x = *(float *)(*(char **)(moby + 0x24) + 0x24);
                func_00214D28((float *)(moby + 0x2C), x * f20, x * 0.1f);
            }
            return;
        }
        case 3: {
            float ang;
            int rv;
            int rv2;
            float f20;
            v = (float)moby[0x23];
            ang = func_001F9F90((float)(D_L00_0015F6B0 % 60) / 60.0f * 6.28318024f - 3.14159012f);
            rv = func_001FA898_r(ang * 5.0f + 5.0f);
            f20 = (float)(rv + 10);
            rv2 = func_001F9850(30);
            func_00214D28(&v, f20, (float)(128 / rv2));
            moby[0x23] = func_001FA898_r(v);
            if (0.0f <= func_L00_00251468(moby, *(void **)(D_0013E633 + 0x2E9D))) {
                moby[0x20] = 1;
            } else {
                int d8 = *(int *)(data + 8);
                if (((int *)(D_0013D50F + 0x21))[d8] < ((RecU *)D_L00_001C43B0)[d8].e) {
                    moby[0x20] = 1;
                }
            }
            break;
        }
        case 4: {
            char *ptr = *(char **)(moby + 0x24);
            float cur = *(float *)(moby + 0x2C);
            float X = *(float *)(ptr + 0x24);
            *(float *)(moby + 0x2C) = cur + (X - cur) * (D_0015EE60 * 0.0299999993f);
            if ((*(float *)(ptr + 0x24) - *(float *)(moby + 0x2C)) / *(float *)(ptr + 0x24) < 0.00999999978f) {
                moby[0x20] = 1;
                *(float *)(moby + 0x2C) = *(float *)(ptr + 0x24);
                *(int *)(moby + 0x94) = *(int *)(ptr + 0x10);
            }
            break;
        }
        }
    }
}
